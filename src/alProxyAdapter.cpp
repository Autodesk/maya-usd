//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "alProxyAdapter.h"

#include "alProxyDelegate.h"
#include "debugCodes.h"

#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/delegates/sceneDelegate.h>
#include <hdmaya/hdmaya.h>

#include <maya/MTime.h>

#ifdef HD_MAYA_AL_OVERRIDE_PROXY_SELECTION
#include <AL/usdmaya/nodes/Engine.h>

#ifdef HDMAYA_USD_001907_BUILD
#include <pxr/imaging/hdx/pickTask.h>
#else
#include <pxr/imaging/hdx/intersector.h>
#endif

#include <pxr/imaging/hdx/tokens.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>

#include <maya/M3dView.h>
#endif // HD_MAYA_AL_OVERRIDE_PROXY_SELECTION

#if HDMAYA_UFE_BUILD
#include <ufe/rtid.h>
#include <ufe/runTimeMgr.h>
#endif // HDMAYA_UFE_BUILD

using AL::usdmaya::nodes::ProxyShape;

namespace {

#if HDMAYA_UFE_BUILD
constexpr auto USD_UFE_RUNTIME_NAME = "USD";
// static UFE_NS::Rtid usdUfeRtid = 0;
#endif // HDMAYA_UFE_BUILD

void StageLoadedCallback(void* userData, AL::event::NodeEvents* node) {
    // Bunch of conversions + sanity checks...
    auto* adapter = static_cast<PXR_NS::HdMayaALProxyAdapter*>(userData);
    if (!TF_VERIFY(
            adapter, "StageLoadedCallback called with null userData ptr")) {
        return;
    }

    // Real work done by delegate->createUsdImagingDelegate
    TF_DEBUG(HDMAYA_AL_CALLBACKS)
        .Msg(
            "HdMayaALProxyAdapter - called StageLoadedCallback "
            "(ProxyShape: "
            "%s)\n",
            adapter->GetDagPath().partialPathName().asChar());
    adapter->CreateUsdImagingDelegate();
}

#ifdef HD_MAYA_AL_OVERRIDE_PROXY_SELECTION

constexpr auto HD_STORM_OVERRIDE_NAME =
#ifdef HDMAYA_USD_001910_BUILD
    "mtohRenderOverride_HdStormRendererPlugin";
#else
    "mtohRenderOverride_HdStreamRendererPlugin";
#endif

ProxyShape::FindPickedPrimsRunner oldFindPickedPrimsRunner(nullptr, nullptr);

/// Alternate picking mechanism for the AL proxy shape, which
/// uses our own renderIndex
bool FindPickedPrimsMtoh(
    ProxyShape& proxy, const MDagPath& proxyDagPath,
    const GfMatrix4d& viewMatrix, const GfMatrix4d& projectionMatrix,
    const GfMatrix4d& worldToLocalSpace, const SdfPathVector& paths,
    const UsdImagingGLRenderParams& params, bool nearestOnly,
    unsigned int pickResolution, ProxyShape::HitBatch& outHit, void* userData) {
    TF_DEBUG(HDMAYA_AL_SELECTION).Msg("FindPickedPrimsMtoh\n");

    auto doOldFindPickedPrims = [&]() {
        if (!oldFindPickedPrimsRunner) {
            TF_WARN(
                "called FindPickedPrimsMtoh before oldFindPickedPrimsRunner "
                "set");
            return false;
        }
        return oldFindPickedPrimsRunner(
            proxy, proxyDagPath, viewMatrix, projectionMatrix,
            worldToLocalSpace, paths, params, nearestOnly, pickResolution,
            outHit);
    };

    if (!userData) { return doOldFindPickedPrims(); }

    auto delegateCtx = reinterpret_cast<HdMayaDelegateCtx*>(userData);

    if (!delegateCtx->IsHdSt()) { return doOldFindPickedPrims(); }

    // Unless the current viewport renderer is an mtoh HdStorm one, use
    // the normal AL picking function.
    MStatus status;
    auto view = M3dView::active3dView(&status);
    if (!status) {
        TF_WARN("Error getting active3dView\n");
        return doOldFindPickedPrims();
    }
    auto rendererEnum = view.getRendererName(&status);
    if (!status) {
        TF_WARN("Error calling getRendererName\n");
        return doOldFindPickedPrims();
    }
    if (rendererEnum != M3dView::kViewport2Renderer) {
        return doOldFindPickedPrims();
    }
    MString overrideName = view.renderOverrideName(&status);
    if (!status) {
        TF_WARN("Error calling renderOverrideName\n");
        return doOldFindPickedPrims();
    }
    if (overrideName != HD_STORM_OVERRIDE_NAME) {
        return doOldFindPickedPrims();
    }

    const SdfPath& proxyAdapterID =
        delegateCtx->GetPrimPath(proxyDagPath, false);

    // We found the HdSt proxy delegate, use it's engine / renderIndex to do
    // selection!

    HdRprimCollection intersectCollect;
    TfTokenVector renderTags;

#ifdef HDMAYA_USD_001907_BUILD
    HdxPickHitVector hdxHits;
    auto& intersectionMode = nearestOnly ? HdxPickTokens->resolveNearestToCamera
                                         : HdxPickTokens->resolveUnique;
#else
    HdxIntersector::HitVector hdxHits;
    auto& intersectionMode = nearestOnly
                                 ? HdxIntersectionModeTokens->nearestToCamera
                                 : HdxIntersectionModeTokens->unique;
#endif

    if (!AL::usdmaya::nodes::Engine::TestIntersectionBatch(
            viewMatrix, projectionMatrix, worldToLocalSpace, paths, params,
            intersectionMode, pickResolution, intersectCollect,
            *delegateCtx->GetTaskController(), delegateCtx->GetEngine(),
            hdxHits)) {
        return false;
    }

    // Store the shape adapter here just to keep it's ref count
    HdMayaShapeAdapterPtr shapeAdapter = nullptr;
    HdMayaALProxyAdapter* proxyAdapter = nullptr;

    bool foundHit = false;
    for (const auto& hit : hdxHits) {
        const SdfPath protoIndexPath = hit.objectId;

        // TODO: improve handling of multiple AL proxy shapes
        // if we have multiple proxy shapes, we will run a selection
        // query once for each shape, and then we throw away any results
        // that aren't in our proxy - clearly, this is wasteful - we should
        // run the selection once, cache it, and use that for all shapes...
        if (!protoIndexPath.HasPrefix(proxyAdapterID)) { continue; }

        // We delay checking for a valid HdMayaSceneDelegate until here
        // because if we don't have any hits on the proxy shape, it doesn't
        // matter
        if (proxyAdapter == nullptr) {
            auto* hdSceneDelegate =
                dynamic_cast<HdMayaSceneDelegate*>(delegateCtx);
            if (hdSceneDelegate == nullptr) {
                TF_WARN(
                    "User data passed to FindPickedPrimsMtoh was not a valid "
                    "HdMayaSceneDelegate*");
                return false;
            }

            shapeAdapter = hdSceneDelegate->GetShapeAdapter(proxyAdapterID);
            if (shapeAdapter == nullptr) {
                TF_WARN(
                    "Could not find an adapter for proxy shape %s",
                    proxyDagPath.fullPathName().asChar());
                return false;
            }

            proxyAdapter =
                dynamic_cast<HdMayaALProxyAdapter*>(shapeAdapter.get());
            if (proxyAdapter == nullptr) {
                TF_WARN(
                    "Adapter for proxy shape %s was not a HdMayaALProxyAdapter",
                    proxyDagPath.fullPathName().asChar());
                return false;
            }
        }

        foundHit = true;

        const SdfPath instancerPath = hit.instancerId;
        const int instanceIndex = hit.instanceIndex;

        auto primIndexPath = proxyAdapter->GetPathForInstanceIndex(
            protoIndexPath, instanceIndex, nullptr);

        if (primIndexPath.IsEmpty()) {
            primIndexPath = protoIndexPath.StripAllVariantSelections();
        }

        auto usdPath = proxyAdapter->ConvertIndexPathToCachePath(primIndexPath);

        TF_DEBUG(HDMAYA_AL_SELECTION)
            .Msg(
                "FindPickedPrimsMtoh - hit (usdPath): %s\n", usdPath.GetText());

        auto& worldSpaceHitPoint = outHit[usdPath];
        worldSpaceHitPoint = GfVec3d(
            hit.worldSpaceHitPoint[0], hit.worldSpaceHitPoint[1],
            hit.worldSpaceHitPoint[2]);
    }

    return foundHit;
}

#endif // HD_MAYA_AL_OVERRIDE_PROXY_SELECTION

} // namespace

PXR_NAMESPACE_OPEN_SCOPE

HdMayaALProxyAdapter::HdMayaALProxyAdapter(
    HdMayaDelegateCtx* delegate, const MDagPath& dag)
    : HdMayaShapeAdapter(delegate->GetPrimPath(dag, false), delegate, dag) {
#ifdef HD_MAYA_AL_OVERRIDE_PROXY_SELECTION
    if (GetDelegate()->IsHdSt()) {
        if (!oldFindPickedPrimsRunner) {
            TF_DEBUG(HDMAYA_AL_SELECTION)
                .Msg(
                    "HdMayaALProxyDelegate - installing alt "
                    "FindPickedPrimsFunction\n");
            oldFindPickedPrimsRunner = ProxyShape::getFindPickedPrimsRunner();
            ProxyShape::setFindPickedPrimsFunction(
                FindPickedPrimsMtoh, GetDelegate());
        }
    }
#endif // HD_MAYA_AL_OVERRIDE_PROXY_SELECTION

    MStatus status;
    MFnDependencyNode mfnNode(_node, &status);
    if (!TF_VERIFY(status, "Error getting MFnDependencyNode")) { return; }

    _proxy = dynamic_cast<ProxyShape*>(mfnNode.userNode());
    if (!TF_VERIFY(
            _proxy, "Error getting ProxyShape* for %s",
            mfnNode.name().asChar())) {
        return;
    }

    auto* scheduler = _proxy->scheduler();
    if (!TF_VERIFY(
            scheduler, "Error getting scheduler for %s",
            _proxy->name().asChar())) {
        _proxy = nullptr;
        return;
    }

    TF_DEBUG(HDMAYA_AL_CALLBACKS)
        .Msg(
            "HdMayaALProxyAdapter - creating PreStageLoaded callback "
            "for %s\n",
            _proxy->name().asChar());
    _proxyShapeCallbacks.push_back(scheduler->registerCallback(
        _proxy->getId("PreStageLoaded"),     // eventId
        "HdMayaALProxyDelegate_onStageLoad", // tag
        StageLoadedCallback,                 // functionPointer
        10000,                               // weight
        this                                 // userData
        ));

    HdMayaALProxyDelegate::AddAdapter(this);
}

HdMayaALProxyAdapter::~HdMayaALProxyAdapter() {
    for (auto& callbackId : _proxyShapeCallbacks) {
        _proxy->scheduler()->unregisterCallback(callbackId);
    }
    HdMayaALProxyDelegate::RemoveAdapter(this);
}

void HdMayaALProxyAdapter::Populate() {
    if (_isPopulated || !_proxy) { return; }

    TF_DEBUG(HDMAYA_AL_POPULATE)
        .Msg("HdMayaALProxyDelegate::Populating %s\n", _proxy->name().asChar());

    auto stage = _proxy->getUsdStage();
    if (!stage) {
        MGlobal::displayError(
            MString("Could not get stage for proxyShape: ") + _proxy->name());
        return;
    }

    if (!_usdDelegate) { CreateUsdImagingDelegate(); }
    if (!TF_VERIFY(_usdDelegate)) { return; }

    _usdDelegate->Populate(stage->GetPseudoRoot());

    _isPopulated = true;
}

bool HdMayaALProxyAdapter::IsSupported() const { return _proxy != nullptr; }

void HdMayaALProxyAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    if (dirtyBits != 0) {
        if (dirtyBits & HdChangeTracker::DirtyTransform) {
            // At the time this is called, the proxy shape's transform may not
            // yet be in a state where it's "new" xform can be queried...
            // however, we call UpdateRootTransform anyway. Why? Because doing
            // so will mark all sub-prim's transforms dirty, so that they will
            // then call _usdDelegate's GetTransform, which will then calculate
            // the "updated" root xform at "render time."
            _usdDelegate->UpdateRootTransform();
            _usdDelegate->SetRootTransformDirty();
        }
        if (dirtyBits & HdChangeTracker::DirtyVisibility) {
            // See note above for DirtyTransform - same logic applies here
            _usdDelegate->UpdateRootVisibility();
            _usdDelegate->SetRootVisibilityDirty();
        }
    }
}

VtValue HdMayaALProxyAdapter::Get(const TfToken& key) {
    TF_DEBUG(HDMAYA_ADAPTER_GET)
        .Msg(
            "Called HdMayaALProxyAdapter::Get(%s) - %s\n", key.GetText(),
            GetDagPath().partialPathName().asChar());
    return {};
}

bool HdMayaALProxyAdapter::HasType(const TfToken& typeId) const {
    return false;
}

void HdMayaALProxyAdapter::PopulateSelectedPaths(
    const MDagPath& selectedDag, SdfPathVector& selectedSdfPaths,
    std::unordered_set<SdfPath, SdfPath::Hash>& selectedMasters,
    const HdSelectionSharedPtr& selection) {
    // TODO: if the AL proxy shape is ever updated to work properly
    // when instanced, update this to work with instances as well.
    // May require a fair amount of reworking... perhaps instance
    // handling should be moved up (into the non-virtual-overridden
    // code) if possible?

    MStatus status;
    MObject proxyMObj;
    MFnDagNode proxyMFnDag;

    proxyMObj = _proxy->thisMObject();
    if (!TF_VERIFY(!proxyMObj.isNull())) { return; }
    if (!TF_VERIFY(proxyMFnDag.setObject(proxyMObj))) { return; }

    // First, we check to see if the entire proxy shape is selected
    if (selectedDag.node() == proxyMObj) {
        selectedSdfPaths.push_back(_usdDelegate->GetDelegateID());
        _usdDelegate->PopulateSelection(
            HdSelection::HighlightModeSelect, selectedSdfPaths.back(),
            UsdImagingDelegate::ALL_INSTANCES, selection);
        return;
    }
}

void HdMayaALProxyAdapter::CreateUsdImagingDelegate() {
    // Why do this release when we do a reset right below? Because we want
    // to make sure we delete the old delegate before creating a new one
    // (the reset statement below will first create a new one, THEN delete
    // the old one). Why do we care? In case they have the same _renderIndex
    // - if so, the delete may clear out items from the renderIndex that the
    // constructor potentially adds
    _usdDelegate.release();
    _usdDelegate.reset(new HdMayaAlProxyUsdImagingDelegate(
        &GetDelegate()->GetRenderIndex(),
        _id.AppendChild(TfToken(TfStringPrintf(
            "ALProxyDelegate_%s_%p", _proxy->name().asChar(), _proxy))),
        _proxy, GetDagPath()));
    _isPopulated = false;
}

void HdMayaALProxyAdapter::PreFrame() {
    _usdDelegate->ApplyPendingUpdates();
    // TODO: set this only when time is actually changed
    _usdDelegate->SetTime(
        UsdTimeCode(_proxy->outTimePlug().asMTime().as(MTime::uiUnit())));
    _usdDelegate->PostSyncCleanup();
}

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<HdMayaALProxyAdapter, TfType::Bases<HdMayaDagAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, AL_usdmaya_ProxyShape) {
    HdMayaAdapterRegistry::RegisterShapeAdapter(
        TfToken(ProxyShape::kTypeName.asChar()),
        [](HdMayaDelegateCtx* delegate,
           const MDagPath& dag) -> HdMayaShapeAdapterPtr {
            return HdMayaShapeAdapterPtr(
                new HdMayaALProxyAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
