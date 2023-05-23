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
// Copyright 2023 Autodesk, Inc. All rights reserved.
//
#include "sceneDelegate.h"

#include <mayaHydraLib/adapters/adapterRegistry.h>
#include <mayaHydraLib/adapters/materialNetworkConverter.h>
#include <mayaHydraLib/adapters/mayaAttrs.h>
#include <mayaHydraLib/adapters/renderItemAdapter.h>
#include <mayaHydraLib/delegates/delegateDebugCodes.h>
#include <mayaHydraLib/delegates/delegateRegistry.h>
#include <mayaHydraLib/utils.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/range3d.h>
#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/basisCurves.h>
#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/mesh.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/imaging/hdx/renderTask.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MDGMessage.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnComponent.h>
#include <maya/MFnMesh.h>
#include <maya/MItDag.h>
#include <maya/MMatrixArray.h>
#include <maya/MObjectHandle.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MProfiler.h>
#include <maya/MSelectionList.h>
#include <maya/MShaderManager.h>
#include <maya/MString.h>

#include <cassert>

int _profilerCategory = MProfiler::addCategory(
    "MayaHydraSceneDelegate (mayaHydra)",
    "Events for MayaHydraSceneDelegate");

#if PXR_VERSION < 2211
#error USD version v0.22.11+ required
#endif

#if MAYA_API_VERSION < 20240000
#error Maya API version 2024+ required
#endif

PXR_NAMESPACE_OPEN_SCOPE

SdfPath MayaHydraSceneDelegate::_fallbackMaterial;
SdfPath MayaHydraSceneDelegate::_mayaDefaultMaterialPath; // Common to all scene delegates
VtValue MayaHydraSceneDelegate::_mayaDefaultMaterial;

namespace {

void _onDagNodeAdded(MObject& obj, void* clientData)
{
    reinterpret_cast<MayaHydraSceneDelegate*>(clientData)->OnDagNodeAdded(obj);
}

void _onDagNodeRemoved(MObject& obj, void* clientData)
{
    reinterpret_cast<MayaHydraSceneDelegate*>(clientData)->OnDagNodeRemoved(obj);
}

const MString defaultLightSet("defaultLightSet");

void _connectionChanged(MPlug& srcPlug, MPlug& destPlug, bool made, void* clientData)
{
    TF_UNUSED(made);
    const auto srcObj = srcPlug.node();
    if (!srcObj.hasFn(MFn::kTransform)) {
        return;
    }
    const auto destObj = destPlug.node();
    if (!destObj.hasFn(MFn::kSet)) {
        return;
    }
    if (srcPlug != MayaAttrs::dagNode::instObjGroups) {
        return;
    }
    MStatus           status;
    MFnDependencyNode destNode(destObj, &status);
    if (ARCH_UNLIKELY(!status)) {
        return;
    }
    if (destNode.name() != defaultLightSet) {
        return;
    }
    auto*    delegate = reinterpret_cast<MayaHydraSceneDelegate*>(clientData);
    MDagPath dag;
    status = MDagPath::getAPathTo(srcObj, dag);
    if (ARCH_UNLIKELY(!status)) {
        return;
    }
    unsigned int shapesBelow = 0;
    dag.numberOfShapesDirectlyBelow(shapesBelow);
    for (auto i = decltype(shapesBelow) { 0 }; i < shapesBelow; ++i) {
        auto dagCopy = dag;
        dagCopy.extendToShapeDirectlyBelow(i);
        delegate->UpdateLightVisibility(dagCopy);
    }
}

template <typename T, typename F> inline bool _FindAdapter(const SdfPath&, F) { return false; }

template <typename T, typename M0, typename F, typename... M>
inline bool _FindAdapter(const SdfPath& id, F f, const M0& m0, const M&... m)
{
    auto* adapterPtr = TfMapLookupPtr(m0, id);
    if (adapterPtr == nullptr) {
        return _FindAdapter<T>(id, f, m...);
    } else {
        f(static_cast<T*>(adapterPtr->get()));
        return true;
    }
}

template <typename T, typename F> inline bool _RemoveAdapter(const SdfPath&, F) { return false; }

template <typename T, typename M0, typename F, typename... M>
inline bool _RemoveAdapter(const SdfPath& id, F f, M0& m0, M&... m)
{
    auto* adapterPtr = TfMapLookupPtr(m0, id);
    if (adapterPtr == nullptr) {
        return _RemoveAdapter<T>(id, f, m...);
    } else {
        f(static_cast<T*>(adapterPtr->get()));
        m0.erase(id);
        return true;
    }
}

template <typename R> inline R _GetDefaultValue() { return {}; }

// This will be nicer to use with automatic parameter deduction for lambdas in
// C++14.
template <typename T, typename R, typename F> inline R _GetValue(const SdfPath&, F)
{
    return _GetDefaultValue<R>();
}

template <typename T, typename R, typename F, typename M0, typename... M>
inline R _GetValue(const SdfPath& id, F f, const M0& m0, const M&... m)
{
    auto* adapterPtr = TfMapLookupPtr(m0, id);
    if (adapterPtr == nullptr) {
        return _GetValue<T, R>(id, f, m...);
    } else {
        return f(static_cast<T*>(adapterPtr->get()));
    }
}

template <typename T, typename F> inline void _MapAdapter(F)
{
    // Do nothing.
}

template <typename T, typename M0, typename F, typename... M>
inline void _MapAdapter(F f, const M0& m0, const M&... m)
{
    for (auto& it : m0) {
        f(static_cast<T*>(it.second.get()));
    }
    _MapAdapter<T>(f, m...);
}

} // namespace

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (MayaHydraSceneDelegate)
    ((MayaDefaultMaterial, "__maya_default_material__"))
    (diffuseColor)
    (emissiveColor)
    (roughness)
    (MayaHydraMeshPoints)
    (constantLighting)
);
// clang-format on

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<MayaHydraSceneDelegate, TfType::Bases<MayaHydraDelegate>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(MayaHydraDelegateRegistry, MayaHydraSceneDelegate)
{
    MayaHydraDelegateRegistry::RegisterDelegate(
        _tokens->MayaHydraSceneDelegate,
        [](const MayaHydraDelegate::InitData& initData) -> MayaHydraDelegatePtr {
            return std::static_pointer_cast<MayaHydraDelegate>(
                std::make_shared<MayaHydraSceneDelegate>(initData));
        });
}

// MayaHydraSceneDelegate is an Hydra custom scene delegate used to translate from a Maya scene to
// hydra. If you want to know how to add a custom scene index to this plug-in, then please see the
// registration.cpp file.
MayaHydraSceneDelegate::MayaHydraSceneDelegate(const InitData& initData)
    : MayaHydraDelegateCtx(initData)
{
    // TfDebug::Enable(MAYAHYDRALIB_DELEGATE_GET_MATERIAL_ID);//Enable this line to print to the
    // output window all SceneDelegate::GetMaterialID(...) calls
    // TfDebug::Enable(MAYAHYDRALIB_DELEGATE_GET); //Enable this line to print to the output window
    // all SceneDelegate::Get(...) calls

    // Enable the following line to print to the output window the materials parameters type and
    // values when there is a change in one of them.
    // TfDebug::Enable(MAYAHYDRALIB_ADAPTER_MATERIALS_PRINT_PARAMETERS_VALUES);

    // Enable the following line to print to the output window the lights parameters type and
    // values. TfDebug::Enable(MAYAHYDRALIB_DELEGATE_PRINT_LIGHTS_PARAMETERS_VALUES);

    static std::once_flag once;
    std::call_once(once, []() {
        _mayaDefaultMaterialPath = SdfPath::AbsoluteRootPath().AppendChild(
            _tokens->MayaDefaultMaterial); // Is an absolute path, not linked to a scene delegate
        _mayaDefaultMaterial = MayaHydraSceneDelegate::CreateMayaDefaultMaterial();
        _fallbackMaterial = SdfPath::EmptyPath(); // Empty path for hydra fallback material
    });
}

MayaHydraSceneDelegate::~MayaHydraSceneDelegate()
{
    for (auto callback : _callbacks) {
        MMessage::removeCallback(callback);
    }
    _MapAdapter<MayaHydraAdapter>(
        [](MayaHydraAdapter* a) { a->RemoveCallbacks(); },
        _renderItemsAdapters,
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
        _shapeAdapters,
#endif
        _lightAdapters,
        _materialAdapters);
}

VtValue MayaHydraSceneDelegate::CreateMayaDefaultMaterial()
{
    static const MColor kDefaultGrayColor = MColor(0.5f, 0.5f, 0.5f) * 0.8f;

    HdMaterialNetworkMap networkMap;
    HdMaterialNetwork    network;
    HdMaterialNode       node;
    node.identifier = UsdImagingTokens->UsdPreviewSurface;
    node.path = _mayaDefaultMaterialPath;
    node.parameters.insert(
        { _tokens->diffuseColor,
          VtValue(GfVec3f(kDefaultGrayColor[0], kDefaultGrayColor[1], kDefaultGrayColor[2])) });
    network.nodes.push_back(std::move(node));
    networkMap.map.insert({ HdMaterialTerminalTokens->surface, std::move(network) });
    networkMap.terminals.push_back(_mayaDefaultMaterialPath);
    return VtValue(networkMap);
}

void MayaHydraSceneDelegate::_AddRenderItem(const MayaHydraRenderItemAdapterPtr& ria)
{
    const SdfPath& primPath = ria->GetID();
    _renderItemsAdaptersFast.insert({ ria->GetFastID(), ria });
    _renderItemsAdapters.insert({ primPath, ria });
}

void MayaHydraSceneDelegate::_RemoveRenderItem(const MayaHydraRenderItemAdapterPtr& ria)
{
    const SdfPath& primPath = ria->GetID();
    _renderItemsAdaptersFast.erase(ria->GetFastID());
    _renderItemsAdapters.erase(primPath);
}

void MayaHydraSceneDelegate::HandleCompleteViewportScene(
    const MDataServerOperation::MViewportScene& scene,
    MFrameContext::DisplayStyle                 displayStyle)
{
    const bool playbackRunning = MAnimControl::isPlaying();

    if (_isPlaybackRunning != playbackRunning) {
        // The value has changed, we are calling SetPlaybackChanged so that every render item that
        // has its visibility dependent on the playback should dirty its hydra visibility flag so
        // its gets recomputed.
        for (auto it = _renderItemsAdapters.begin(); it != _renderItemsAdapters.end(); it++) {
            it->second->SetPlaybackChanged();
        }

        _isPlaybackRunning = playbackRunning;
    }

    // First loop to get rid of removed items
    constexpr int kInvalidId = 0;
    for (size_t i = 0; i < scene.mRemovalCount; i++) {
        int fastId = scene.mRemovals[i];
        if (fastId == kInvalidId)
            continue;
        MayaHydraRenderItemAdapterPtr ria = nullptr;
        if (_GetRenderItem(fastId, ria)) {
            _RemoveRenderItem(ria);
        }
        assert(ria != nullptr);
    }

    // My version, does minimal update
    // This loop could, in theory, be parallelized.  Unclear how large the gains would be, but maybe
    // nothing to lose unless there is some internal contention in USD.
    for (size_t i = 0; i < scene.mCount; i++) {
        auto flags = scene.mFlags[i];
        if (flags == 0) {
            continue;
        }

        auto& ri = *scene.mItems[i];

        MColor                   wireframeColor;
        MHWRender::DisplayStatus displayStatus = MHWRender::kNoStatus;

        MDagPath dagPath = ri.sourceDagPath();
        if (dagPath.isValid()) {
            wireframeColor = MGeometryUtilities::wireframeColor(
                dagPath); // This is a color managed VP2 color, it will need to be unmanaged at some
                          // point
            displayStatus = MGeometryUtilities::displayStatus(dagPath);
        }

        int                           fastId = ri.InternalObjectId();
        MayaHydraRenderItemAdapterPtr ria = nullptr;
        if (!_GetRenderItem(fastId, ria)) {
            const SdfPath slowId = GetRenderItemPrimPath(ri);
            if (slowId.IsEmpty()){
                continue;
            }
            // MAYA-128021: We do not currently support maya instances.
            MDagPath dagPath(ri.sourceDagPath());
            ria = std::make_shared<MayaHydraRenderItemAdapter>(dagPath, slowId, fastId, this, ri);
            _AddRenderItem(ria);
        }

        SdfPath material;
        MObject shadingEngineNode;
        if (!_GetRenderItemMaterial(ri, material, shadingEngineNode)) {
            if (material != kInvalidMaterial) {
                _CreateMaterial(material, shadingEngineNode);
            }
        }

        if (flags & MDataServerOperation::MViewportScene::MVS_changedEffect) {
            ria->SetMaterial(material);
        }

        const MayaHydraRenderItemAdapter::UpdateFromDeltaData data(
            ri, flags, wireframeColor, displayStatus);
        ria->UpdateFromDelta(data);
        if (flags & MDataServerOperation::MViewportScene::MVS_changedMatrix) {
            ria->UpdateTransform(ri);
        }
    }
}

void MayaHydraSceneDelegate::Populate()
{
    MayaHydraAdapterRegistry::LoadAllPlugin();
    auto&   renderIndex = GetRenderIndex();
    MStatus status;
#ifndef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    MItDag dagIt(MItDag::kDepthFirst);
    dagIt.traverseUnderWorld(true);
    for (; !dagIt.isDone(); dagIt.next()) {
        MObject node = dagIt.currentItem(&status);
        if (status != MS::kSuccess)
            continue;
        OnDagNodeAdded(node);
    }
#else
    MItDag dagIt2(MItDag::kDepthFirst, MFn::kInvalid);
    dagIt2.traverseUnderWorld(true);
    for (; !dagIt2.isDone(); dagIt2.next()) {
        MDagPath path;
        dagIt2.getPath(path);
        InsertDag(path);
    }
#endif

    auto id = MDGMessage::addNodeAddedCallback(_onDagNodeAdded, "dagNode", this, &status);
    if (status) {
        _callbacks.push_back(id);
    }
    id = MDGMessage::addNodeRemovedCallback(_onDagNodeRemoved, "dagNode", this, &status);
    if (status) {
        _callbacks.push_back(id);
    }
    id = MDGMessage::addConnectionCallback(_connectionChanged, this, &status);
    if (status) {
        _callbacks.push_back(id);
    }

    // Adding materials sprim to the render index.
    if (renderIndex.IsSprimTypeSupported(HdPrimTypeTokens->material)) {
        renderIndex.InsertSprim(HdPrimTypeTokens->material, this, _mayaDefaultMaterialPath);
    }
}

//
void MayaHydraSceneDelegate::PreFrame(const MHWRender::MDrawContext& context)
{
    bool useDefaultMaterial
        = (context.getDisplayStyle() & MHWRender::MFrameContext::kDefaultMaterial);
    if (useDefaultMaterial != _useDefaultMaterial) {
        _useDefaultMaterial = useDefaultMaterial;
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
        for (const auto& shape : _shapeAdapters)
            shape.second->MarkDirty(HdChangeTracker::DirtyMaterialId);
#endif
    }

    const bool xRayEnabled = (context.getDisplayStyle() & MHWRender::MFrameContext::kXray);
    if (xRayEnabled != _xRayEnabled) {
        _xRayEnabled = xRayEnabled;
        for (auto& matAdapter : _materialAdapters)
            matAdapter.second->EnableXRayShadingMode(_xRayEnabled);
    }

    if (!_materialTagsChanged.empty()) {
        if (IsHdSt()) {
            for (const auto& id : _materialTagsChanged) {
                if (_GetValue<MayaHydraMaterialAdapter, bool>(
                        id,
                        [](MayaHydraMaterialAdapter* a) { return a->UpdateMaterialTag(); },
                        _materialAdapters)) {
                    auto& renderIndex = GetRenderIndex();
                    for (const auto& rprimId : renderIndex.GetRprimIds()) {
                        const auto* rprim = renderIndex.GetRprim(rprimId);
                        if (rprim != nullptr && rprim->GetMaterialId() == id) {
                            RebuildAdapterOnIdle(
                                rprim->GetId(), MayaHydraDelegateCtx::RebuildFlagPrim);
                        }
                    }
                }
            }
        }
        _materialTagsChanged.clear();
    }
#ifndef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    if (!_lightsToAdd.empty()) {
        for (auto& lightToAdd : _lightsToAdd) {
            MDagPath dag;
            MStatus  status = MDagPath::getAPathTo(lightToAdd.first, dag);
            if (!status) {
                return;
            }
            MayaHydraSceneDelegate::Create(dag, lightToAdd.second, _lightAdapters, true);
        }
        _lightsToAdd.clear();
    }
#else
    if (!_addedNodes.empty()) {
        for (const auto& obj : _addedNodes) {
            if (obj.isNull()) {
                continue;
            }
            MDagPath dag;
            MStatus  status = MDagPath::getAPathTo(obj, dag);
            if (!status) {
                return;
            }
            // We need to check if there is an instanced shape below this dag
            // and insert it as well, because they won't be inserted.
            if (dag.hasFn(MFn::kTransform)) {
                const auto childCount = dag.childCount();
                for (auto child = decltype(childCount) { 0 }; child < childCount; ++child) {
                    auto dagCopy = dag;
                    dagCopy.push(dag.child(child));
                    if (dagCopy.isInstanced() && dagCopy.instanceNumber() > 0) {
                        AddNewInstance(dagCopy);
                    }
                }
            } else {
                InsertDag(dag);
            }
        }
        _addedNodes.clear();
    }
#endif
    // We don't need to rebuild something that's already being recreated.
    // Since we have a few elements, linear search over vectors is going to
    // be okay.
    if (!_adaptersToRecreate.empty()) {
        for (const auto& it : _adaptersToRecreate) {
            RecreateAdapter(std::get<0>(it), std::get<1>(it));
            for (auto itr = _adaptersToRebuild.begin(); itr != _adaptersToRebuild.end(); ++itr) {
                if (std::get<0>(it) == std::get<0>(*itr)) {
                    _adaptersToRebuild.erase(itr);
                    break;
                }
            }
        }
        _adaptersToRecreate.clear();
    }
    if (!_adaptersToRebuild.empty()) {
        for (const auto& it : _adaptersToRebuild) {
            _FindAdapter<MayaHydraAdapter>(
                std::get<0>(it),
                [&](MayaHydraAdapter* a) {
                    if (std::get<1>(it) & MayaHydraDelegateCtx::RebuildFlagCallbacks) {
                        a->RemoveCallbacks();
                        a->CreateCallbacks();
                    }
                    if (std::get<1>(it) & MayaHydraDelegateCtx::RebuildFlagPrim) {
                        a->RemovePrim();
                        a->Populate();
                    }
                },
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
                _shapeAdapters,
#endif
                _lightAdapters,
                _materialAdapters);
        }
        _adaptersToRebuild.clear();
    }
    if (!IsHdSt()) {
        return;
    }

    // Some 3rd parties lights may be ignored by the call to
    // MHWRender::MDrawContext::numberOfActiveLights (like the Arnold lights which are seen by Maya
    // as locators)
    std::vector<MDagPath> activelightPaths
        = _arnoldLightPaths; // We suppose the Arnold lights are always active

    constexpr auto considerAllSceneLights = MHWRender::MDrawContext::kFilteredIgnoreLightLimit;
    MStatus        status;
    const auto     numLights = context.numberOfActiveLights(considerAllSceneLights, &status);

    if ((!status || numLights == 0) && (0 == activelightPaths.size())) {
        _MapAdapter<MayaHydraLightAdapter>(
            [](MayaHydraLightAdapter* a) { a->SetLightingOn(false); },
            _lightAdapters); // Turn off all lights
        return;
    }

    MIntArray intVals;
    MMatrix   matrixVal;
    for (auto i = decltype(numLights) { 0 }; i < numLights; ++i) {
        auto* lightParam = context.getLightParameterInformation(i, considerAllSceneLights);
        if (lightParam == nullptr) {
            continue;
        }
        const auto lightPath = lightParam->lightPath();
        if (!lightPath.isValid()) {
            continue;
        }

        activelightPaths.push_back(lightPath);

        if (!lightParam->getParameter(MHWRender::MLightParameterInformation::kShadowOn, intVals)
            || intVals.length() < 1 || intVals[0] != 1) {
            continue;
        }

        if (lightParam->getParameter(
                MHWRender::MLightParameterInformation::kShadowViewProj, matrixVal)) {
            _FindAdapter<MayaHydraLightAdapter>(
                GetPrimPath(lightPath, true),
                [&matrixVal](MayaHydraLightAdapter* a) {
                    a->SetShadowProjectionMatrix(MAYAHYDRA_NS::GetGfMatrixFromMaya(matrixVal));
                },
                _lightAdapters);
        }
    }

    // Turn on active lights, turn off non-active lights, and add non-created active lights.
    _MapAdapter<MayaHydraLightAdapter>(
        [&](MayaHydraLightAdapter* a) {
            auto it = std::find(activelightPaths.begin(), activelightPaths.end(), a->GetDagPath());
            if (it != activelightPaths.end()) {
                a->SetLightingOn(true);
                activelightPaths.erase(it);
            } else {
                a->SetLightingOn(false);
            }
        },
        _lightAdapters);
    for (const auto& lightPath : activelightPaths) {
        Create(
            lightPath,
            MayaHydraAdapterRegistry::GetLightAdapterCreator(lightPath),
            _lightAdapters,
            true);
    }
}

void MayaHydraSceneDelegate::RemoveAdapter(const SdfPath& id)
{
    if (!_RemoveAdapter<MayaHydraAdapter>(
            id,
            [](MayaHydraAdapter* a) {
                a->RemoveCallbacks();
                a->RemovePrim();
            },
            _renderItemsAdapters,
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
            _shapeAdapters,
#endif
            _lightAdapters,
            _materialAdapters)) {
        TF_WARN(
            "MayaHydraSceneDelegate::RemoveAdapter(%s) -- Adapter does not exists", id.GetText());
    }
}

void MayaHydraSceneDelegate::RecreateAdapterOnIdle(const SdfPath& id, const MObject& obj)
{
    // We expect this to be a small number of objects, so using a simple linear
    // search and a vector is generally a good choice.
    for (auto& it : _adaptersToRecreate) {
        if (std::get<0>(it) == id) {
            std::get<1>(it) = obj;
            return;
        }
    }
    _adaptersToRecreate.emplace_back(id, obj);
}

void MayaHydraSceneDelegate::MaterialTagChanged(const SdfPath& id)
{
    if (std::find(_materialTagsChanged.begin(), _materialTagsChanged.end(), id)
        == _materialTagsChanged.end()) {
        _materialTagsChanged.push_back(id);
    }
}

void MayaHydraSceneDelegate::RebuildAdapterOnIdle(const SdfPath& id, uint32_t flags)
{
    // We expect this to be a small number of objects, so using a simple linear
    // search and a vector is generally a good choice.
    for (auto& it : _adaptersToRebuild) {
        if (std::get<0>(it) == id) {
            std::get<1>(it) |= flags;
            return;
        }
    }
    _adaptersToRebuild.emplace_back(id, flags);
}

void MayaHydraSceneDelegate::RecreateAdapter(const SdfPath& id, const MObject& obj)
{
    if (_RemoveAdapter<MayaHydraAdapter>(
            id,
            [](MayaHydraAdapter* a) {
                a->RemoveCallbacks();
                a->RemovePrim();
            },
            _lightAdapters)) {
        if (MObjectHandle(obj).isValid()) {
            OnDagNodeAdded(obj);
        } else {
            TF_DEBUG(MAYAHYDRALIB_DELEGATE_RECREATE_ADAPTER)
                .Msg(
                    "Shape/light prim (%s) not re-created because node no "
                    "longer valid\n",
                    id.GetText());
        }
        return;
    }
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    if (_RemoveAdapter<MayaHydraAdapter>(
            id,
            [](MayaHydraAdapter* a) {
                a->RemoveCallbacks();
                a->RemovePrim();
            },
            _shapeAdapters,
            _lightAdapters)) {
        MFnDagNode dgNode(obj);
        MDagPath   path;
        dgNode.getPath(path);
        if (path.isValid() && MObjectHandle(obj).isValid()) {
            TF_DEBUG(MAYAHYDRALIB_DELEGATE_RECREATE_ADAPTER)
                .Msg(
                    "Shape/light prim (%s) re-created for dag path (%s)\n",
                    id.GetText(),
                    path.fullPathName().asChar());
            InsertDag(path);
        } else {
            TF_DEBUG(MAYAHYDRALIB_DELEGATE_RECREATE_ADAPTER)
                .Msg(
                    "Shape/light prim (%s) not re-created because node no "
                    "longer valid\n",
                    id.GetText());
        }
        return;
    }
#endif
    if (_RemoveAdapter<MayaHydraMaterialAdapter>(
            id,
            [](MayaHydraMaterialAdapter* a) {
                a->RemoveCallbacks();
                a->RemovePrim();
            },
            _materialAdapters)) {
        auto& renderIndex = GetRenderIndex();
        auto& changeTracker = renderIndex.GetChangeTracker();
        for (const auto& rprimId : renderIndex.GetRprimIds()) {
            const auto* rprim = renderIndex.GetRprim(rprimId);
            if (rprim != nullptr && rprim->GetMaterialId() == id) {
                changeTracker.MarkRprimDirty(rprimId, HdChangeTracker::DirtyMaterialId);
            }
        }
        if (MObjectHandle(obj).isValid()) {
            TF_DEBUG(MAYAHYDRALIB_DELEGATE_RECREATE_ADAPTER)
                .Msg(
                    "Material prim (%s) re-created for node (%s)\n",
                    id.GetText(),
                    MFnDependencyNode(obj).name().asChar());
            _CreateMaterial(GetMaterialPath(obj), obj);
        } else {
            TF_DEBUG(MAYAHYDRALIB_DELEGATE_RECREATE_ADAPTER)
                .Msg(
                    "Material prim (%s) not re-created because node no "
                    "longer valid\n",
                    id.GetText());
        }

    } else {
        TF_WARN(
            "MayaHydraSceneDelegate::RecreateAdapterOnIdle(%s) -- Adapter does "
            "not exists",
            id.GetText());
    }
}

MayaHydraShapeAdapterPtr MayaHydraSceneDelegate::GetShapeAdapter(const SdfPath& id)
{
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    auto iter = _shapeAdapters.find(id);
    return iter == _shapeAdapters.end() ? nullptr : iter->second;
#else
    return nullptr;
#endif
}

MayaHydraLightAdapterPtr MayaHydraSceneDelegate::GetLightAdapter(const SdfPath& id)
{
    auto iter = _lightAdapters.find(id);
    return iter == _lightAdapters.end() ? nullptr : iter->second;
}

MayaHydraMaterialAdapterPtr MayaHydraSceneDelegate::GetMaterialAdapter(const SdfPath& id)
{
    auto iter = _materialAdapters.find(id);
    return iter == _materialAdapters.end() ? nullptr : iter->second;
}

template <typename AdapterPtr, typename Map>
AdapterPtr MayaHydraSceneDelegate::Create(
    const MDagPath&                                                          dag,
    const std::function<AdapterPtr(MayaHydraDelegateCtx*, const MDagPath&)>& adapterCreator,
    Map&                                                                     adapterMap,
    bool                                                                     isSprim)
{
    if (!adapterCreator) {
        return {};
    }

    TF_DEBUG(MAYAHYDRALIB_DELEGATE_INSERTDAG)
        .Msg(
            "MayaHydraSceneDelegate::Create::"
            "found %s: %s\n",
            MFnDependencyNode(dag.node()).typeName().asChar(),
            dag.fullPathName().asChar());

    const auto id = GetPrimPath(dag, isSprim);
    if (TfMapLookupPtr(adapterMap, id) != nullptr) {
        return {};
    }
    auto adapter = adapterCreator(this, dag);
    if (adapter == nullptr || !adapter->IsSupported()) {
        return {};
    }
    adapter->Populate();
    adapter->CreateCallbacks();
    adapterMap.insert({ id, adapter });
    return adapter;
}

namespace {
bool GetShadingEngineNode(const MRenderItem& ri, MObject& shadingEngineNode)
{
    MDagPath dagPath = ri.sourceDagPath();
    if (dagPath.isValid()) {
        MFnDagNode   dagNode(dagPath.node());
        MObjectArray sets, comps;
        dagNode.getConnectedSetsAndMembers(dagPath.instanceNumber(), sets, comps, true);
        assert(sets.length() == comps.length());
        for (uint32_t i = 0; i < sets.length(); ++i) {
            const MObject& object = sets[i];
            if (object.apiType() == MFn::kShadingEngine) {
                // To support per-face shading, find the shading node matched with the render item
                const MObject& comp = comps[i];
                MObject        shadingComp = ri.shadingComponent();
                if (shadingComp.isNull() || comp.isNull()
                    || MFnComponent(comp).isEqual(shadingComp)) {
                    shadingEngineNode = object;
                    return true;
                }
            }
        }
    }
    return false;
}
} // namespace

bool MayaHydraSceneDelegate::_GetRenderItemMaterial(
    const MRenderItem& ri,
    SdfPath&           material,
    MObject&           shadingEngineNode)
{
    if (MHWRender::MGeometry::Primitive::kLines == ri.primitive()
        || MHWRender::MGeometry::Primitive::kLineStrip == ri.primitive()) {
        material = _fallbackMaterial; // Use fallbackMaterial + constantLighting + displayColor
        return true;
    }

    if (GetShadingEngineNode(ri, shadingEngineNode))
    // Else try to find associated material node if this is a material shader.
    // NOTE: The existing maya material support in hydra expects a shading engine node
    {
        material = GetMaterialPath(shadingEngineNode);
        if (TfMapLookupPtr(_materialAdapters, material) != nullptr) {
            return true;
        }
    }

    return false;
}

// Analogous to MayaHydraSceneDelegate::InsertDag
bool MayaHydraSceneDelegate::_GetRenderItem(int fastId, MayaHydraRenderItemAdapterPtr& ria)
{
    // Using SdfPath as the hash table key is extremely slow.  The cost appears to be GetPrimPath,
    // which would depend on MdagPath, which is a wrapper on TdagPath.  TdagPath is a very slow
    // class and best to avoid in any performance- critical area. Simply workaround for the
    // prototype is an additional lookup index based on InternalObjectID.  Long term goal would be
    // that the plug-in rarely, if ever, deals with TdagPath.
    MayaHydraRenderItemAdapterPtr* result = TfMapLookupPtr(_renderItemsAdaptersFast, fastId);

    if (result != nullptr) {
        // adapter already exists, return it
        ria = *result;
        return true;
    }

    return false;
}

void MayaHydraSceneDelegate::OnDagNodeAdded(const MObject& obj)
{
    if (obj.isNull())
        return;

    // We care only about lights for this callback, it is used to create a LightAdapter when adding
    // a new light in the scene while being in hydra
    if (auto lightFn = MayaHydraAdapterRegistry::GetLightAdapterCreator(obj)) {
        _lightsToAdd.push_back({ obj, lightFn });
    }
}

void MayaHydraSceneDelegate::OnDagNodeRemoved(const MObject& obj)
{
    const auto it
        = std::remove_if(_lightsToAdd.begin(), _lightsToAdd.end(), [&obj](const auto& item) {
              return item.first == obj;
          });

    if (it != _lightsToAdd.end()) {
        _lightsToAdd.erase(it, _lightsToAdd.end());
    }
}

#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
void MayaHydraSceneDelegate::InsertDag(const MDagPath& dag)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_INSERTDAG)
        .Msg(
            "MayaHydraSceneDelegate::InsertDag::"
            "GetLightsEnabled()=%i\n",
            GetLightsEnabled());
    // We don't care about transforms.
    if (dag.hasFn(MFn::kTransform)) {
        return;
    }

    MFnDagNode dagNode(dag);
    if (dagNode.isIntermediateObject()) {
        return;
    }

    // Skip UFE nodes coming from USD runtime
    // UFE stands for Universal Front End : the goal of the Universal Front End is to create a
    // DCC-agnostic component that will allow a DCC to browse and edit data in multiple data models.
    // Those will be handled by USD Imaging delegate
    MStatus              status;
    static const MString ufeRuntimeStr = "ufeRuntime";
    MPlug                ufeRuntimePlug = dagNode.findPlug(ufeRuntimeStr, false, &status);
    if ((status == MS::kSuccess) && ufeRuntimePlug.asString() == "USD") {
        return;
    }

    // Custom lights don't have MFn::kLight.
    if (GetLightsEnabled()) {
        if (Create(
                dag, MayaHydraAdapterRegistry::GetLightAdapterCreator(dag), _lightAdapters, true))
            return;
    }
    if (Create(
            dag, MayaHydraAdapterRegistry::GetCameraAdapterCreator(dag), _cameraAdapters, true)) {
        return;
    }
    // We are inserting a single prim and
    // instancer for every instanced mesh.
    if (dag.isInstanced() && dag.instanceNumber() > 0) {
        return;
    }

    auto adapter
        = Create(dag, MayaHydraAdapterRegistry::GetShapeAdapterCreator(dag), _shapeAdapters);
    if (adapter) {
        auto material = adapter->GetMaterial();
        if (material != MObject::kNullObj) {
            const auto materialId = GetMaterialPath(material);
            if (TfMapLookupPtr(_materialAdapters, materialId) == nullptr) {
                _CreateMaterial(materialId, material);
            }
        }
    }
}
#endif

void MayaHydraSceneDelegate::UpdateLightVisibility(const MDagPath& dag)
{
    const auto id = GetPrimPath(dag, true);
    _FindAdapter<MayaHydraLightAdapter>(
        id,
        [](MayaHydraLightAdapter* a) {
            if (a->UpdateVisibility()) {
                a->RemovePrim();
                a->Populate();
                a->InvalidateTransform();
            }
        },
        _lightAdapters);
}

//
void MayaHydraSceneDelegate::AddNewInstance(const MDagPath& dag)
{
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    MDagPathArray dags;
    MDagPath::getAllPathsTo(dag.node(), dags);
    const auto dagsLength = dags.length();
    if (dagsLength == 0) {
        return;
    }
    const auto                             masterDag = dags[0];
    const auto                             id = GetPrimPath(masterDag, false);
    std::shared_ptr<MayaHydraShapeAdapter> masterAdapter;
    if (!TfMapLookup(_shapeAdapters, id, &masterAdapter) || masterAdapter == nullptr) {
        return;
    }
    // If dags is 1, we have to recreate the adapter.
    if (dags.length() == 1 || !masterAdapter->IsInstanced()) {
        RecreateAdapterOnIdle(id, masterDag.node());
    } else {
        // If dags is more than one, trigger rebuilding callbacks next call and
        // mark dirty.
        RebuildAdapterOnIdle(id, MayaHydraDelegateCtx::RebuildFlagCallbacks);
        masterAdapter->MarkDirty(
            HdChangeTracker::DirtyInstancer | HdChangeTracker::DirtyInstanceIndex
            | HdChangeTracker::DirtyPrimvar);
    }
#endif
}

void MayaHydraSceneDelegate::SetParams(const MayaHydraParams& params)
{
    const auto& oldParams = GetParams();
    if (oldParams.displaySmoothMeshes != params.displaySmoothMeshes) {
        // I couldn't find any other way to turn this on / off.
        // I can't convert HdRprim to HdMesh easily and no simple way
        // to get the type of the HdRprim from the render index.
        // If we want to allow creating multiple rprims and returning an id
        // to a subtree, we need to use the HasType function and the mark dirty
        // from each adapter.
        _MapAdapter<MayaHydraRenderItemAdapter>(
            [](MayaHydraRenderItemAdapter* a) {
                if (a->HasType(HdPrimTypeTokens->mesh) || a->HasType(HdPrimTypeTokens->basisCurves)
                    || a->HasType(HdPrimTypeTokens->points)) {
                    a->MarkDirty(HdChangeTracker::DirtyTopology);
                }
            },
            _renderItemsAdapters);
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
        _MapAdapter<MayaHydraDagAdapter>(
            [](MayaHydraDagAdapter* a) {
                if (a->HasType(HdPrimTypeTokens->mesh)) {
                    a->MarkDirty(HdChangeTracker::DirtyTopology);
                }
            },
            _shapeAdapters);
#endif
    }
    if (oldParams.motionSampleStart != params.motionSampleStart
        || oldParams.motionSampleEnd != params.motionSampleEnd) {
        _MapAdapter<MayaHydraRenderItemAdapter>(
            [](MayaHydraRenderItemAdapter* a) {
                if (a->HasType(HdPrimTypeTokens->mesh) || a->HasType(HdPrimTypeTokens->basisCurves)
                    || a->HasType(HdPrimTypeTokens->points)) {
                    a->InvalidateTransform();
                    a->MarkDirty(HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyTransform);
                }
            },
            _renderItemsAdapters);
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
        _MapAdapter<MayaHydraDagAdapter>(
            [](MayaHydraDagAdapter* a) {
                if (a->HasType(HdPrimTypeTokens->mesh)) {
                    a->MarkDirty(HdChangeTracker::DirtyPoints);
                } else if (a->HasType(HdPrimTypeTokens->camera)) {
                    a->MarkDirty(HdCamera::DirtyParams);
                }
                a->InvalidateTransform();
                a->MarkDirty(HdChangeTracker::DirtyTransform);
            },
            _shapeAdapters,
            _lightAdapters,
            _cameraAdapters);
#endif
    }
    // We need to trigger rebuilding shaders.
    if (oldParams.textureMemoryPerTexture != params.textureMemoryPerTexture) {
        _MapAdapter<MayaHydraMaterialAdapter>(
            [](MayaHydraMaterialAdapter* a) { a->MarkDirty(HdMaterial::AllDirty); },
            _materialAdapters);
    }
    if (oldParams.maximumShadowMapResolution != params.maximumShadowMapResolution) {
        _MapAdapter<MayaHydraLightAdapter>(
            [](MayaHydraLightAdapter* a) { a->MarkDirty(HdLight::AllDirty); }, _lightAdapters);
    }
    MayaHydraDelegate::SetParams(params);
}

//! \brief  Try to obtain maya object corresponding to HdxPickHit and add it to a maya selection
//! list \return whether the conversion was a success
bool MayaHydraSceneDelegate::AddPickHitToSelectionList(
    const HdxPickHit&                hit,
    const MHWRender::MSelectionInfo& selectInfo,
    MSelectionList&                  selectionList,
    MPointArray&                     worldSpaceHitPts)
{
    SdfPath hitId = hit.objectId;
    // validate that hit is indeed a maya item. Alternatively, the rprim hit could be an rprim
    // defined by a scene index such as maya usd.
    if (hitId.HasPrefix(GetRprimPath())) {
        _FindAdapter<MayaHydraRenderItemAdapter>(
            hitId,
            [&selectionList, &worldSpaceHitPts, &hit](MayaHydraRenderItemAdapter* a) {
                selectionList.add(a->GetDagPath());
                worldSpaceHitPts.append(
                    hit.worldSpaceHitPoint[0],
                    hit.worldSpaceHitPoint[1],
                    hit.worldSpaceHitPoint[2]);
            },
            _renderItemsAdapters);
        return true;
    }

    return false;
}

HdMeshTopology MayaHydraSceneDelegate::GetMeshTopology(const SdfPath& id)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_MESH_TOPOLOGY)
        .Msg("MayaHydraSceneDelegate::GetMeshTopology(%s)\n", id.GetText());
    return _GetValue<MayaHydraAdapter, HdMeshTopology>(
        id,
        [](MayaHydraAdapter* a) -> HdMeshTopology { return a->GetMeshTopology(); },
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
        _shapeAdapters,
#endif
        _renderItemsAdapters);
}

HdBasisCurvesTopology MayaHydraSceneDelegate::GetBasisCurvesTopology(const SdfPath& id)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_CURVE_TOPOLOGY)
        .Msg("MayaHydraSceneDelegate::GetBasisCurvesTopology(%s)\n", id.GetText());
    return _GetValue<MayaHydraAdapter, HdBasisCurvesTopology>(
        id,
        [](MayaHydraAdapter* a) -> HdBasisCurvesTopology { return a->GetBasisCurvesTopology(); },
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
        _shapeAdapters,
#endif
        _renderItemsAdapters);
}

PxOsdSubdivTags MayaHydraSceneDelegate::GetSubdivTags(const SdfPath& id)
{
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_SUBDIV_TAGS)
        .Msg("MayaHydraSceneDelegate::GetSubdivTags(%s)\n", id.GetText());
    return _GetValue<MayaHydraShapeAdapter, PxOsdSubdivTags>(
        id,
        [](MayaHydraShapeAdapter* a) -> PxOsdSubdivTags { return a->GetSubdivTags(); },
        _shapeAdapters);
#else
    return {};
#endif
}

GfRange3d MayaHydraSceneDelegate::GetExtent(const SdfPath& id)
{
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_EXTENT)
        .Msg("MayaHydraSceneDelegate::GetExtent(%s)\n", id.GetText());
    return _GetValue<MayaHydraShapeAdapter, GfRange3d>(
        id, [](MayaHydraShapeAdapter* a) -> GfRange3d { return a->GetExtent(); }, _shapeAdapters);
#else
    return GfRange3d();
#endif
}

GfMatrix4d MayaHydraSceneDelegate::GetTransform(const SdfPath& id)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_TRANSFORM)
        .Msg("MayaHydraSceneDelegate::GetTransform(%s)\n", id.GetText());
    return _GetValue<MayaHydraAdapter, GfMatrix4d>(
        id,
        [](MayaHydraAdapter* a) -> GfMatrix4d { return a->GetTransform(); },
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
        _shapeAdapters,
#endif
        _renderItemsAdapters,
        _cameraAdapters,
        _lightAdapters);
}

size_t MayaHydraSceneDelegate::SampleTransform(
    const SdfPath& id,
    size_t         maxSampleCount,
    float*         times,
    GfMatrix4d*    samples)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_SAMPLE_TRANSFORM)
        .Msg(
            "MayaHydraSceneDelegate::SampleTransform(%s, %u)\n",
            id.GetText(),
            static_cast<unsigned int>(maxSampleCount));
    return _GetValue<MayaHydraDagAdapter, size_t>(
        id,
        [maxSampleCount, times, samples](MayaHydraDagAdapter* a) -> size_t {
            return a->SampleTransform(maxSampleCount, times, samples);
        },
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
        _shapeAdapters,
#endif
        _cameraAdapters,
        _lightAdapters);
}

bool MayaHydraSceneDelegate::IsEnabled(const TfToken& option) const
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_IS_ENABLED)
        .Msg("MayaHydraSceneDelegate::IsEnabled(%s)\n", option.GetText());
    // Maya scene can't be accessed on multiple threads,
    // so I don't think this is safe to enable.
    if (option == HdOptionTokens->parallelRprimSync) {
        return false;
    }

    TF_WARN("MayaHydraSceneDelegate::IsEnabled(%s) -- Unsupported option.\n", option.GetText());
    return false;
}

VtValue MayaHydraSceneDelegate::Get(const SdfPath& id, const TfToken& key)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET)
        .Msg("MayaHydraSceneDelegate::Get(%s, %s)\n", id.GetText(), key.GetText());

#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    if (id.IsPropertyPath()) {
        return _GetValue<MayaHydraDagAdapter, VtValue>(
            id.GetPrimPath(),
            [&key](MayaHydraDagAdapter* a) -> VtValue { return a->GetInstancePrimvar(key); },
            _shapeAdapters);
    } else {
#endif
        return _GetValue<MayaHydraAdapter, VtValue>(
            id,
            [&key](MayaHydraAdapter* a) -> VtValue { return a->Get(key); },
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
            _shapeAdapters,
#endif
            _renderItemsAdapters,
            _cameraAdapters,
            _lightAdapters,
            _materialAdapters);
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    }
#endif
}

size_t MayaHydraSceneDelegate::SamplePrimvar(
    const SdfPath& id,
    const TfToken& key,
    size_t         maxSampleCount,
    float*         times,
    VtValue*       samples)
{
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_SAMPLE_PRIMVAR)
        .Msg(
            "MayaHydraSceneDelegate::SamplePrimvar(%s, %s, %u)\n",
            id.GetText(),
            key.GetText(),
            static_cast<unsigned int>(maxSampleCount));
    if (maxSampleCount < 1) {
        return 0;
    }
    if (id.IsPropertyPath()) {
        times[0] = 0.0f;
        samples[0] = _GetValue<MayaHydraDagAdapter, VtValue>(
            id.GetPrimPath(),
            [&key](MayaHydraDagAdapter* a) -> VtValue { return a->GetInstancePrimvar(key); },
            _shapeAdapters);
        return 1;
    } else {
        return _GetValue<MayaHydraShapeAdapter, size_t>(
            id,
            [&key, maxSampleCount, times, samples](MayaHydraShapeAdapter* a) -> size_t {
                return a->SamplePrimvar(key, maxSampleCount, times, samples);
            },
            _shapeAdapters);
    }
#else
    return HdSceneDelegate::SamplePrimvar(id, key, maxSampleCount, times, samples);
#endif
}

TfToken MayaHydraSceneDelegate::GetRenderTag(const SdfPath& id)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_RENDER_TAG)
        .Msg("MayaHydraSceneDelegate::GetRenderTag(%s)\n", id.GetText());
    return _GetValue<MayaHydraAdapter, TfToken>(
        id.GetPrimPath(),
        [](MayaHydraAdapter* a) -> TfToken { return a->GetRenderTag(); },
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
        _shapeAdapters,
#endif
        _renderItemsAdapters);
}

HdPrimvarDescriptorVector
MayaHydraSceneDelegate::GetPrimvarDescriptors(const SdfPath& id, HdInterpolation interpolation)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_PRIMVAR_DESCRIPTORS)
        .Msg(
            "MayaHydraSceneDelegate::GetPrimvarDescriptors(%s, %i)\n", id.GetText(), interpolation);
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    if (id.IsPropertyPath()) {
        return _GetValue<MayaHydraDagAdapter, HdPrimvarDescriptorVector>(
            id.GetPrimPath(),
            [&interpolation](MayaHydraDagAdapter* a) -> HdPrimvarDescriptorVector {
                return a->GetInstancePrimvarDescriptors(interpolation);
            },
            _shapeAdapters);
    } else {
#endif
        return _GetValue<MayaHydraAdapter, HdPrimvarDescriptorVector>(
            id,
            [&interpolation](MayaHydraAdapter* a) -> HdPrimvarDescriptorVector {
                return a->GetPrimvarDescriptors(interpolation);
            },
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
            _shapeAdapters,
#endif
            _renderItemsAdapters);
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    }
#endif
}

VtValue MayaHydraSceneDelegate::GetLightParamValue(const SdfPath& id, const TfToken& paramName)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_LIGHT_PARAM_VALUE)
        .Msg(
            "MayaHydraSceneDelegate::GetLightParamValue(%s, %s)\n",
            id.GetText(),
            paramName.GetText());

    const VtValue val = _GetValue<MayaHydraLightAdapter, VtValue>(
        id,
        [&paramName](MayaHydraLightAdapter* a) -> VtValue {
            return a->GetLightParamValue(paramName);
        },
        _lightAdapters);

    if (TfDebug::IsEnabled(MAYAHYDRALIB_DELEGATE_PRINT_LIGHTS_PARAMETERS_VALUES)) {
        // Print the lights parameters to the output window
        std::string valueAsString;
        MAYAHYDRA_NS_DEF::ConvertVtValueAsText(val, valueAsString);
        cout << "Light : " << id.GetText() << " Parameter : " << paramName.GetText()
             << " Value : " << valueAsString << endl;
    }

    return val;
}

VtValue
MayaHydraSceneDelegate::GetCameraParamValue(const SdfPath& cameraId, const TfToken& paramName)
{
    return _GetValue<MayaHydraCameraAdapter, VtValue>(
        cameraId,
        [&paramName](MayaHydraCameraAdapter* a) -> VtValue {
            return a->GetCameraParamValue(paramName);
        },
        _cameraAdapters);
}

VtIntArray
MayaHydraSceneDelegate::GetInstanceIndices(const SdfPath& instancerId, const SdfPath& prototypeId)
{
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_INSTANCE_INDICES)
        .Msg(
            "MayaHydraSceneDelegate::GetInstanceIndices(%s, %s)\n",
            instancerId.GetText(),
            prototypeId.GetText());
    return _GetValue<MayaHydraDagAdapter, VtIntArray>(
        instancerId.GetPrimPath(),
        [&prototypeId](MayaHydraDagAdapter* a) -> VtIntArray {
            return a->GetInstanceIndices(prototypeId);
        },
        _shapeAdapters);
#else
    return VtIntArray();
#endif
}

SdfPathVector MayaHydraSceneDelegate::GetInstancerPrototypes(SdfPath const& instancerId)
{
    return { instancerId.GetPrimPath() };
}

SdfPath MayaHydraSceneDelegate::GetInstancerId(const SdfPath& primId)
{
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_INSTANCER_ID)
        .Msg("MayaHydraSceneDelegate::GetInstancerId(%s)\n", primId.GetText());
    // Instancers don't have any instancers yet.
    if (primId.IsPropertyPath()) {
        return SdfPath();
    }
    return _GetValue<MayaHydraDagAdapter, SdfPath>(
        primId,
        [](MayaHydraDagAdapter* a) -> SdfPath { return a->GetInstancerID(); },
        _shapeAdapters);
#else
    return SdfPath();
#endif
}

GfMatrix4d MayaHydraSceneDelegate::GetInstancerTransform(SdfPath const& instancerId)
{
    return GfMatrix4d(1.0);
}

SdfPath MayaHydraSceneDelegate::GetScenePrimPath(
    const SdfPath&      rprimPath,
    int                 instanceIndex,
    HdInstancerContext* instancerContext)
{
    return rprimPath;
}

bool MayaHydraSceneDelegate::GetVisible(const SdfPath& id)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_VISIBLE)
        .Msg("MayaHydraSceneDelegate::GetVisible(%s)\n", id.GetText());

    return _GetValue<MayaHydraAdapter, bool>(
        id,
        [](MayaHydraAdapter* a) -> bool { return a->GetVisible(); },
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
        _shapeAdapters,
#endif
        _renderItemsAdapters,
        _lightAdapters);
}

bool MayaHydraSceneDelegate::GetDoubleSided(const SdfPath& id)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_DOUBLE_SIDED)
        .Msg("MayaHydraSceneDelegate::GetDoubleSided(%s)\n", id.GetText());
    return _GetValue<MayaHydraAdapter, bool>(
        id,
        [](MayaHydraAdapter* a) -> bool { return a->GetDoubleSided(); },
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
        _shapeAdapters,
#endif
        _renderItemsAdapters);
}

HdCullStyle MayaHydraSceneDelegate::GetCullStyle(const SdfPath& id)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_CULL_STYLE)
        .Msg("MayaHydraSceneDelegate::GetCullStyle(%s)\n", id.GetText());
    // HdCullStyleNothing means no culling, HdCullStyledontCare means : let the renderer choose
    // between back or front faces culling. We don't want culling, since we want to see the
    // backfaces being unlit with MayaHydraSceneDelegate::GetDoubleSided returning false.
    return HdCullStyleNothing;
}

HdDisplayStyle MayaHydraSceneDelegate::GetDisplayStyle(const SdfPath& id)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_DISPLAY_STYLE)
        .Msg("MayaHydraSceneDelegate::GetDisplayStyle(%s)\n", id.GetText());
    return _GetValue<MayaHydraAdapter, HdDisplayStyle>(
        id,
        [](MayaHydraAdapter* a) -> HdDisplayStyle { return a->GetDisplayStyle(); },
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
        _shapeAdapters,
#endif
        _renderItemsAdapters);
}

SdfPath MayaHydraSceneDelegate::GetMaterialId(const SdfPath& id)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_MATERIAL_ID)
        .Msg("MayaHydraSceneDelegate::GetMaterialId(%s)\n", id.GetText());

    if (_useDefaultMaterial) {
        return _mayaDefaultMaterialPath;
    }

    auto result = TfMapLookupPtr(_renderItemsAdapters, id);
    if (result != nullptr) {
        auto& renderItemAdapter = *result;

        // Check if this render item is a wireframe primitive
        if (MHWRender::MGeometry::Primitive::kLines == renderItemAdapter->GetPrimitive()
            || MHWRender::MGeometry::Primitive::kLineStrip == renderItemAdapter->GetPrimitive()) {
            return _fallbackMaterial;
        }

        auto& material = renderItemAdapter->GetMaterial();

        if (material == kInvalidMaterial) {
            return _fallbackMaterial;
        }

        if (TfMapLookupPtr(_materialAdapters, material) != nullptr) {
            return material;
        }
    }
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY

    auto shapeAdapter = TfMapLookupPtr(_shapeAdapters, id);
    if (shapeAdapter == nullptr) {
        return _fallbackMaterial;
    }
    auto material = shapeAdapter->get()->GetMaterial();
    if (material == MObject::kNullObj) {
        return _fallbackMaterial;
    }
    auto materialId = GetMaterialPath(material);
    if (TfMapLookupPtr(_materialAdapters, materialId) != nullptr) {
        return materialId;
    }

    return _CreateMaterial(materialId, material) ? materialId : _fallbackMaterial;
#else
    return _fallbackMaterial;
#endif
}

VtValue MayaHydraSceneDelegate::GetMaterialResource(const SdfPath& id)
{
    TF_DEBUG(MAYAHYDRALIB_DELEGATE_GET_MATERIAL_RESOURCE)
        .Msg("MayaHydraSceneDelegate::GetMaterialResource(%s)\n", id.GetText());

    if (id == _mayaDefaultMaterialPath) {
        return _mayaDefaultMaterial;
    }

    if (id == _fallbackMaterial) {
        return MayaHydraMaterialAdapter::GetPreviewMaterialResource(id);
    }

    auto ret = _GetValue<MayaHydraMaterialAdapter, VtValue>(
        id,
        [](MayaHydraMaterialAdapter* a) -> VtValue { return a->GetMaterialResource(); },
        _materialAdapters);
    return ret.IsEmpty() ? MayaHydraMaterialAdapter::GetPreviewMaterialResource(id) : ret;
}

bool MayaHydraSceneDelegate::_CreateMaterial(const SdfPath& id, const MObject& obj)
{
    TF_DEBUG(MAYAHYDRALIB_ADAPTER_MATERIALS)
        .Msg("MayaHydraSceneDelegate::_CreateMaterial(%s)\n", id.GetText());

    auto materialCreator = MayaHydraAdapterRegistry::GetMaterialAdapterCreator(obj);
    if (materialCreator == nullptr) {
        return false;
    }
    auto materialAdapter = materialCreator(id, this, obj);
    if (materialAdapter == nullptr || !materialAdapter->IsSupported()) {
        return false;
    }

    if (_xRayEnabled) {
        materialAdapter->EnableXRayShadingMode(_xRayEnabled); // Enable XRay shading mode
    }
    materialAdapter->Populate();
    materialAdapter->CreateCallbacks();
    _materialAdapters.emplace(id, std::move(materialAdapter));
    return true;
}

SdfPath MayaHydraSceneDelegate::SetCameraViewport(const MDagPath& camPath, const GfVec4d& viewport)
{
    const SdfPath camID = GetPrimPath(camPath, true);
    auto&&        cameraAdapter = TfMapLookupPtr(_cameraAdapters, camID);
    if (cameraAdapter) {
        (*cameraAdapter)->SetViewport(viewport);
        return camID;
    }
    return {};
}

void MayaHydraSceneDelegate::AddArnoldLight(const MDagPath& dag)
{
    _arnoldLightPaths.push_back(dag);
}

void MayaHydraSceneDelegate::RemoveArnoldLight(const MDagPath& dag)
{
    const auto it = std::find(_arnoldLightPaths.begin(), _arnoldLightPaths.end(), dag);
    if (it != _arnoldLightPaths.end()) {
        _arnoldLightPaths.erase(it);
    }
}

VtValue MayaHydraSceneDelegate::GetShadingStyle(SdfPath const& id)
{
    if (auto&& ri = TfMapLookupPtr(_renderItemsAdapters, id)) {
        auto primitive = (*ri)->GetPrimitive();
        if (MHWRender::MGeometry::Primitive::kLines == primitive
            || MHWRender::MGeometry::Primitive::kLineStrip == primitive) {
            return VtValue(
                _tokens
                    ->constantLighting); // Use fallbackMaterial + constantLighting + displayColor
        }
    }
    return MayaHydraDelegateCtx::GetShadingStyle(id);
}

PXR_NAMESPACE_CLOSE_SCOPE
