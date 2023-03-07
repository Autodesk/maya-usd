//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "proxyAdapter.h"

#include <hdMaya/adapters/adapterRegistry.h>
#include <hdMaya/debugCodes.h>
#include <hdMaya/delegates/proxyDelegate.h>
#include <hdMaya/delegates/sceneDelegate.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#include <maya/MGlobal.h>
#include <maya/MTime.h>

#if WANT_UFE_BUILD
#include <ufe/rtid.h>
#include <ufe/runTimeMgr.h>
#endif // WANT_UFE_BUILD

PXR_NAMESPACE_OPEN_SCOPE

HdMayaProxyAdapter::HdMayaProxyAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
    : HdMayaShapeAdapter(delegate->GetPrimPath(dag, false), delegate, dag)
{
    MStatus           status;
    MFnDependencyNode mfnNode(_node, &status);
    if (!TF_VERIFY(status, "Error getting MFnDependencyNode")) {
        return;
    }

    _proxy = dynamic_cast<MayaUsdProxyShapeBase*>(mfnNode.userNode());
    if (!TF_VERIFY(
            _proxy, "Error getting MayaUsdProxyShapeBase* for %s", mfnNode.name().asChar())) {
        return;
    }

    TfWeakPtr<HdMayaProxyAdapter> me(this);
    TfNotice::Register(me, &HdMayaProxyAdapter::_OnStageSet);

    HdMayaProxyDelegate::AddAdapter(this);
}

HdMayaProxyAdapter::~HdMayaProxyAdapter() { HdMayaProxyDelegate::RemoveAdapter(this); }

void HdMayaProxyAdapter::Populate()
{
    if (_isPopulated || !_proxy) {
        return;
    }

    TF_DEBUG(HDMAYA_AL_POPULATE)
        .Msg("HdMayaProxyDelegate::Populating %s\n", _proxy->name().asChar());

    auto stage = _proxy->getUsdStage();
    if (!stage) {
        MGlobal::displayError(MString("Could not get stage for proxyShape: ") + _proxy->name());
        return;
    }

    if (!_usdDelegate) {
        CreateUsdImagingDelegate();
    }
    if (!TF_VERIFY(_usdDelegate)) {
        return;
    }

    _usdDelegate->Populate(stage->GetPseudoRoot());

    _isPopulated = true;
}

bool HdMayaProxyAdapter::IsSupported() const { return _proxy != nullptr; }

void HdMayaProxyAdapter::MarkDirty(HdDirtyBits dirtyBits)
{
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

VtValue HdMayaProxyAdapter::Get(const TfToken& key)
{
    TF_DEBUG(HDMAYA_ADAPTER_GET)
        .Msg(
            "Called HdMayaProxyAdapter::Get(%s) - %s\n",
            key.GetText(),
            GetDagPath().partialPathName().asChar());
    return {};
}

bool HdMayaProxyAdapter::HasType(const TfToken& typeId) const { return false; }

void HdMayaProxyAdapter::PopulateSelectedPaths(
    const MDagPath&                             selectedDag,
    SdfPathVector&                              selectedSdfPaths,
    std::unordered_set<SdfPath, SdfPath::Hash>& selectedMasters,
    const HdSelectionSharedPtr&                 selection)
{
    // TODO: if the AL proxy shape is ever updated to work properly
    // when instanced, update this to work with instances as well.
    // May require a fair amount of reworking... perhaps instance
    // handling should be moved up (into the non-virtual-overridden
    // code) if possible?

    MStatus    status;
    MObject    proxyMObj;
    MFnDagNode proxyMFnDag;

    proxyMObj = _proxy->thisMObject();
    if (!TF_VERIFY(!proxyMObj.isNull())) {
        return;
    }
    if (!TF_VERIFY(proxyMFnDag.setObject(proxyMObj))) {
        return;
    }

    // First, we check to see if the entire proxy shape is selected
    if (selectedDag.node() == proxyMObj) {
#if defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 11
        selectedSdfPaths.push_back(SdfPath::AbsoluteRootPath());
#else
        selectedSdfPaths.push_back(_usdDelegate->GetDelegateID());
#endif
        _usdDelegate->PopulateSelection(
            HdSelection::HighlightModeSelect,
            selectedSdfPaths.back(),
            UsdImagingDelegate::ALL_INSTANCES,
            selection);
        return;
    }
}

void HdMayaProxyAdapter::CreateUsdImagingDelegate()
{
    // Why do this reset when we do another right below? Because we want
    // to make sure we delete the old delegate before creating a new one
    // (the reset statement below will first create a new one, THEN delete
    // the old one). Why do we care? In case they have the same _renderIndex
    // - if so, the delete may clear out items from the renderIndex that the
    // constructor potentially adds
    _usdDelegate.reset();
    _usdDelegate.reset(new HdMayaProxyUsdImagingDelegate(
        &GetDelegate()->GetRenderIndex(),
        _id.AppendChild(
            TfToken(TfStringPrintf("ProxyDelegate_%s_%p", _proxy->name().asChar(), _proxy))),
        _proxy,
        GetDagPath()));
    _isPopulated = false;
}

void HdMayaProxyAdapter::PreFrame(const MHWRender::MDrawContext& context)
{
    _usdDelegate->SetSceneMaterialsEnabled(
        !(context.getDisplayStyle() & MHWRender::MFrameContext::kDefaultMaterial));
    _usdDelegate->ApplyPendingUpdates();
    // TODO: set this only when time is actually changed
    _usdDelegate->SetTime(_proxy->getTime());
    _usdDelegate->PostSyncCleanup();
}

void HdMayaProxyAdapter::_OnStageSet(const MayaUsdProxyStageSetNotice& notice)
{
    if (&notice.GetProxyShape() == _proxy) {
        // Real work done by delegate->createUsdImagingDelegate
        TF_DEBUG(HDMAYA_AL_CALLBACKS)
            .Msg(
                "HdMayaProxyAdapter - called StageLoadedCallback "
                "(ProxyShape: "
                "%s)\n",
                GetDagPath().partialPathName().asChar());

        CreateUsdImagingDelegate();
        auto stage = _proxy->getUsdStage();
        if (_usdDelegate && stage) {
            _usdDelegate->Populate(stage->GetPseudoRoot());
            _isPopulated = true;
        }
    }
}

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaProxyAdapter, TfType::Bases<HdMayaDagAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, MayaUsd_ProxyShape)
{
    HdMayaAdapterRegistry::RegisterShapeAdapter(
        TfToken(MayaUsdProxyShapeBase::typeName.asChar()),
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> HdMayaShapeAdapterPtr {
            return HdMayaShapeAdapterPtr(new HdMayaProxyAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
