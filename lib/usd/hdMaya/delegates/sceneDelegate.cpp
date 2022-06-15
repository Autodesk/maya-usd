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
#include "sceneDelegate.h"

#include <hdMaya/adapters/adapterRegistry.h>
#include <hdMaya/adapters/mayaAttrs.h>
#include <hdMaya/adapters/renderItemAdapter.h>
#include <hdMaya/adapters/materialNetworkConverter.h>

#include <hdMaya/delegates/delegateDebugCodes.h>
#include <hdMaya/delegates/delegateRegistry.h>
#include <hdMaya/utils.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/range3d.h>
#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/mesh.h>
#include <pxr/imaging/hd/rprim.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/hd/basisCurves.h>

#include <maya/MDGMessage.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MItDag.h>
#include <maya/MMatrixArray.h>
#include <maya/MObjectHandle.h>
#include <maya/MString.h>
#include <maya/MShaderManager.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

#include <pxr/usdImaging/usdImaging/tokens.h>
#include <pxr/imaging/hdx/renderTask.h>


PXR_NAMESPACE_OPEN_SCOPE

namespace {

void _nodeAdded(MObject& obj, void* clientData)
{
    // In case of creating new instances, the instance below the
    // dag will be empty and not initialized properly.
    auto* delegate = reinterpret_cast<HdMayaSceneDelegate*>(clientData);
    delegate->NodeAdded(obj);
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
    auto*    delegate = reinterpret_cast<HdMayaSceneDelegate*>(clientData);
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

#if PXR_VERSION < 2011

// Default return value for HdTextureResource::ID, if not found, should be
// -1, not {} - which would be 0
template <> inline HdTextureResource::ID _GetDefaultValue<HdTextureResource::ID>()
{
    return HdTextureResource::ID(-1);
}

#endif // PXR_VERSION < 2011

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

    (HdMayaSceneDelegate)
    ((FallbackMaterial, "__fallback_material__"))
	(HdMayaMeshPoints)
);
// clang-format on

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaSceneDelegate, TfType::Bases<HdMayaDelegate>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaDelegateRegistry, HdMayaSceneDelegate)
{
    HdMayaDelegateRegistry::RegisterDelegate(
        _tokens->HdMayaSceneDelegate,
        [](const HdMayaDelegate::InitData& initData) -> HdMayaDelegatePtr {
            return std::static_pointer_cast<HdMayaDelegate>(
                std::make_shared<HdMayaSceneDelegate>(initData));
        });
}

HdMayaSceneDelegate::HdMayaSceneDelegate(const InitData& initData)
    : HdMayaDelegateCtx(initData)
    , _fallbackMaterial(initData.delegateID.AppendChild(_tokens->FallbackMaterial))
{
}

HdMayaSceneDelegate::~HdMayaSceneDelegate()
{
    for (auto callback : _callbacks) {
        MMessage::removeCallback(callback);
    }
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
	_MapAdapter<HdMayaAdapter>(
		[](HdMayaAdapter* a) { a->RemoveCallbacks(); },
		_renderItemsAdapters,
		_lightAdapters,
		_materialAdapters);
#else
    _MapAdapter<HdMayaAdapter>(
        [](HdMayaAdapter* a) { a->RemoveCallbacks(); },
        _shapeAdapters,
        _lightAdapters,
        _materialAdapters);
#endif
}

//void HdMayaSceneDelegate::_TransformNodeDirty(MObject& node, MPlug& plug, void* clientData)
void HdMayaSceneDelegate::HandleCompleteViewportScene(const MViewportScene& scene)
{
	for (auto it : _renderItemsAdapters)
	// Mark all render items as stale
	{
		auto ria = it.second;
		ria->IsStale(true);		
	}

	for (int i = 0; i < scene.mCount; i++)
	{
		MRenderItem& ri = *scene.mItems[i];

		HdMayaShaderInstanceData sd;
		InsertRenderItemMaterial(ri, sd);

		HdMayaRenderItemAdapterPtr ria;
		InsertRenderItem(ri, sd, ria);	
		ria->UpdateTopology(ri);							
		ria->UpdateTransform(*scene.mItems[i]);
	
		
		ria->IsStale(false);
	}

	for (auto it : _renderItemsAdapters)
	// Remove all stale render items
	{
		auto ria = it.second;
		if (ria->IsStale())
		{
			RemoveAdapter(ria->GetID());
		}
	}

}

void HdMayaSceneDelegate::Populate()
{
    HdMayaAdapterRegistry::LoadAllPlugin();
    auto&  renderIndex = GetRenderIndex();

#ifndef HDMAYA_SCENE_RENDER_DATASERVER
    MItDag dagIt(MItDag::kDepthFirst, MFn::kInvalid);
    dagIt.traverseUnderWorld(true);
    for (; !dagIt.isDone(); dagIt.next()) {
        MDagPath path;
        dagIt.getPath(path);
        InsertDag(path);
    }
    MStatus status;
    auto    id = MDGMessage::addNodeAddedCallback(_nodeAdded, "dagNode", this, &status);
    if (status) {
        _callbacks.push_back(id);
    }
    id = MDGMessage::addConnectionCallback(_connectionChanged, this, &status);
    if (status) {
        _callbacks.push_back(id);
    }
#endif

    // Adding fallback materials sprim to the render index.
    if (renderIndex.IsSprimTypeSupported(HdPrimTypeTokens->material)) {
        renderIndex.InsertSprim(HdPrimTypeTokens->material, this, _fallbackMaterial);
		// TODO remove
		renderIndex.InsertSprim(HdPrimTypeTokens->material, this, _wireframeMaterial);
		renderIndex.InsertSprim(HdPrimTypeTokens->material, this, _vertexMaterial);
    }
	

	// Add a meshPoints repr since it isn't populated in 
	// HdRenderIndex::_ConfigureReprs
	//HdMesh::ConfigureRepr(_tokens->HdMayaMeshPoints,
	//	HdMeshReprDesc(
	//		HdMeshGeomStylePoints,
	//		HdCullStyleNothing,
	//		HdMeshReprDescTokens->pointColor,
	//		/*flatShadingEnabled=*/true,
	//		/*blendWireframeColor=*/false));
}

// 
void HdMayaSceneDelegate::PreFrame(const MHWRender::MDrawContext& context)
{
    bool enableMaterials
        = !(context.getDisplayStyle() & MHWRender::MFrameContext::kDefaultMaterial);
    if (enableMaterials != _enableMaterials) {
        _enableMaterials = enableMaterials;
        for (const auto& shape : _shapeAdapters)
            shape.second->MarkDirty(HdChangeTracker::DirtyMaterialId);
    }

    if (!_materialTagsChanged.empty()) {
        if (IsHdSt()) {
            for (const auto& id : _materialTagsChanged) {
                if (_GetValue<HdMayaMaterialAdapter, bool>(
                        id,
                        [](HdMayaMaterialAdapter* a) { return a->UpdateMaterialTag(); },
                        _materialAdapters)) {
                    auto& renderIndex = GetRenderIndex();
                    for (const auto& rprimId : renderIndex.GetRprimIds()) {
                        const auto* rprim = renderIndex.GetRprim(rprimId);
                        if (rprim != nullptr && rprim->GetMaterialId() == id) {
                            RebuildAdapterOnIdle(
                                rprim->GetId(), HdMayaDelegateCtx::RebuildFlagPrim);
                        }
                    }
                }
            }
        }
        _materialTagsChanged.clear();
    }
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
            _FindAdapter<HdMayaAdapter>(
                std::get<0>(it),
                [&](HdMayaAdapter* a) {
                    if (std::get<1>(it) & HdMayaDelegateCtx::RebuildFlagCallbacks) {
                        a->RemoveCallbacks();
                        a->CreateCallbacks();
                    }
                    if (std::get<1>(it) & HdMayaDelegateCtx::RebuildFlagPrim) {
                        a->RemovePrim();
                        a->Populate();
                    }
                },
                _shapeAdapters,
                _lightAdapters,
                _materialAdapters);
        }
        _adaptersToRebuild.clear();
    }
    if (!IsHdSt()) {
        return;
    }
    constexpr auto considerAllSceneLights = MHWRender::MDrawContext::kFilteredIgnoreLightLimit;
    MStatus        status;
    const auto     numLights = context.numberOfActiveLights(considerAllSceneLights, &status);
    if (!status || numLights == 0) {
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
        if (!lightParam->getParameter(MHWRender::MLightParameterInformation::kShadowOn, intVals)
            || intVals.length() < 1 || intVals[0] != 1) {
            continue;
        }
        if (lightParam->getParameter(
                MHWRender::MLightParameterInformation::kShadowViewProj, matrixVal)) {
            _FindAdapter<HdMayaLightAdapter>(
                GetPrimPath(lightPath, true),
                [&matrixVal](HdMayaLightAdapter* a) {
                    // TODO: Mark Dirty?
                    a->SetShadowProjectionMatrix(GetGfMatrixFromMaya(matrixVal));
                },
                _lightAdapters);
        }
    }
}

void HdMayaSceneDelegate::RemoveAdapter(const SdfPath& id)
{
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
	if (!_RemoveAdapter<HdMayaAdapter>(
		id,
		[](HdMayaAdapter* a) {
			a->RemoveCallbacks();
			a->RemovePrim();
		},
		_renderItemsAdapters,
		_lightAdapters,
		_materialAdapters)) {
		TF_WARN("HdMayaSceneDelegate::RemoveAdapter(%s) -- Adapter does not exists", id.GetText());
	}
#else
    if (!_RemoveAdapter<HdMayaAdapter>(
            id,
            [](HdMayaAdapter* a) {
                a->RemoveCallbacks();
                a->RemovePrim();
            },
            _shapeAdapters,
            _lightAdapters,
            _materialAdapters)) {
        TF_WARN("HdMayaSceneDelegate::RemoveAdapter(%s) -- Adapter does not exists", id.GetText());
    }
#endif
}

void HdMayaSceneDelegate::RecreateAdapterOnIdle(const SdfPath& id, const MObject& obj)
{
    // TODO: Thread safety?
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

void HdMayaSceneDelegate::MaterialTagChanged(const SdfPath& id)
{
    if (std::find(_materialTagsChanged.begin(), _materialTagsChanged.end(), id)
        == _materialTagsChanged.end()) {
        _materialTagsChanged.push_back(id);
    }
}

void HdMayaSceneDelegate::RebuildAdapterOnIdle(const SdfPath& id, uint32_t flags)
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

void HdMayaSceneDelegate::RecreateAdapter(const SdfPath& id, const MObject& obj)
{
    if (_RemoveAdapter<HdMayaAdapter>(
            id,
            [](HdMayaAdapter* a) {
                a->RemoveCallbacks();
                a->RemovePrim();
            },
            _shapeAdapters,
            _lightAdapters)) {
        MFnDagNode dgNode(obj);
        MDagPath   path;
        dgNode.getPath(path);
        if (path.isValid() && MObjectHandle(obj).isValid()) {
            TF_DEBUG(HDMAYA_DELEGATE_RECREATE_ADAPTER)
                .Msg(
                    "Shape/light prim (%s) re-created for dag path (%s)\n",
                    id.GetText(),
                    path.fullPathName().asChar());
            InsertDag(path);
        } else {
            TF_DEBUG(HDMAYA_DELEGATE_RECREATE_ADAPTER)
                .Msg(
                    "Shape/light prim (%s) not re-created because node no "
                    "longer valid\n",
                    id.GetText());
        }
        return;
    }
    if (_RemoveAdapter<HdMayaMaterialAdapter>(
            id,
            [](HdMayaMaterialAdapter* a) {
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
            TF_DEBUG(HDMAYA_DELEGATE_RECREATE_ADAPTER)
                .Msg(
                    "Material prim (%s) re-created for node (%s)\n",
                    id.GetText(),
                    MFnDependencyNode(obj).name().asChar());
            _CreateMaterial(GetMaterialPath(obj), obj);
        } else {
            TF_DEBUG(HDMAYA_DELEGATE_RECREATE_ADAPTER)
                .Msg(
                    "Material prim (%s) not re-created because node no "
                    "longer valid\n",
                    id.GetText());
        }

    } else {
        TF_WARN(
            "HdMayaSceneDelegate::RecreateAdapterOnIdle(%s) -- Adapter does "
            "not exists",
            id.GetText());
    }
}

HdMayaShapeAdapterPtr HdMayaSceneDelegate::GetShapeAdapter(const SdfPath& id)
{
    auto iter = _shapeAdapters.find(id);
    return iter == _shapeAdapters.end() ? nullptr : iter->second;
}


HdMayaRenderItemAdapterPtr HdMayaSceneDelegate::GetRenderItemAdapter(const SdfPath& id)
{
	auto iter = _renderItemsAdapters.find(id);
	return iter == _renderItemsAdapters.end() ? nullptr : iter->second;
}

HdMayaLightAdapterPtr HdMayaSceneDelegate::GetLightAdapter(const SdfPath& id)
{
    auto iter = _lightAdapters.find(id);
    return iter == _lightAdapters.end() ? nullptr : iter->second;
}

HdMayaMaterialAdapterPtr HdMayaSceneDelegate::GetMaterialAdapter(const SdfPath& id)
{
    auto iter = _materialAdapters.find(id);
    return iter == _materialAdapters.end() ? nullptr : iter->second;
}

template <typename AdapterPtr, typename Map>
AdapterPtr HdMayaSceneDelegate::Create(
    const MDagPath&                                                       dag,
    const std::function<AdapterPtr(HdMayaDelegateCtx*, const MDagPath&)>& adapterCreator,
    Map&                                                                  adapterMap,
    bool                                                                  isSprim)
{
    if (!adapterCreator) {
        return {};
    }

    TF_DEBUG(HDMAYA_DELEGATE_INSERTDAG)
        .Msg(
            "HdMayaSceneDelegate::Create::"
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

namespace
{
	static constexpr char kOutColorString[] = "outColor";

	bool GetShadingEngineNode(const MObject& shaderNode, MObject& shadingEngineNode)
	{
		MFnDependencyNode depNode(shaderNode);
		MStatus ms;
		MPlug plug = depNode.findPlug(kOutColorString, ms);

		if (ms == MS::kSuccess)
		{
			MPlugArray destinations;
			plug.connectedTo(destinations, false, true);
			for (auto dest : destinations)
			{
				if (dest.node().isNull()) continue;
				if (dest.node().apiType() != MFn::Type::kShadingEngine) continue;
				shadingEngineNode = dest.node();
				return true;
			}
		}

		return false;
	}
}

bool HdMayaSceneDelegate::InsertRenderItemMaterial(
	const MRenderItem& ri,
	HdMayaShaderInstanceData& sd
	)
{
	MObject shaderNode;
	MObject shadingEngineNode;	
	if (HdMayaRenderItemShaderConverter::ExtractShapeUIShaderData(ri, sd))
		// Determine whether this is a supported UI shader
	{
		SdfPath id = SdfPath(sd.ShapeUIShader->Name);
		if (!TfMapLookupPtr(_renderItemShaderAdapters, id))
		{
			_renderItemShaderAdapters.insert(
				{
					id,
					HdMayaShaderAdapterPtr(new HdMayaShapeUIShaderAdapter(this, *sd.ShapeUIShader))
				});

			GetChangeTracker().MarkTaskDirty(id, HdChangeTracker::DirtyCollection);
		}

		return true;
	}
	else if (
		ri.getShaderNode(shaderNode) == MS::kSuccess &&
		GetShadingEngineNode(shaderNode, shadingEngineNode)
		)
		// Else try to find associated material node if this is a material shader.
		// NOTE: The existing maya material support in hydra expects a shading engine node
	{
		sd.ShapeUIShader = nullptr;
		sd.Material = GetMaterialPath(shadingEngineNode);
		if (TfMapLookupPtr(_materialAdapters, sd.Material) == nullptr)
		{
			_CreateMaterial(sd.Material, shadingEngineNode);
		}

		return true;
	}

	return false;
}

// Analogous to HdMayaSceneDelegate::InsertDag
bool HdMayaSceneDelegate::InsertRenderItem(
	const MRenderItem& ri,
	const HdMayaShaderInstanceData& sd,
	HdMayaRenderItemAdapterPtr& ria
	)
{
    TF_DEBUG(HDMAYA_DELEGATE_INSERTDAG)
        .Msg(
            "HdMayaSceneDelegate::InsertRenderItem::"
            "found shape: %s\n",
            ri.name().asChar());

	const SdfPath id = GetRenderItemPrimPath(ri);
	HdMayaRenderItemAdapterPtr* result = TfMapLookupPtr(_renderItemsAdapters, id);
    if (result != nullptr) 
	{
		ria = *result;
        return false;
    }

    ria = HdMayaRenderItemAdapterPtr(new HdMayaRenderItemAdapter(id, this, ri, sd));        
    _renderItemsAdapters.insert({ id, ria });
	return true;
}

void HdMayaSceneDelegate::InsertDag(const MDagPath& dag)
{
    TF_DEBUG(HDMAYA_DELEGATE_INSERTDAG)
        .Msg(
            "HdMayaSceneDelegate::InsertDag::"
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
    // Those will be handled by USD Imaging delegate
    MStatus              status;
    static const MString ufeRuntimeStr = "ufeRuntime";
    MPlug                ufeRuntimePlug = dagNode.findPlug(ufeRuntimeStr, false, &status);
    if ((status == MS::kSuccess) && ufeRuntimePlug.asString() == "USD") {
        return;
    }

    // Custom lights don't have MFn::kLight.
    if (GetLightsEnabled()) {
        if (Create(dag, HdMayaAdapterRegistry::GetLightAdapterCreator(dag), _lightAdapters, true))
            return;
    }
    if (Create(dag, HdMayaAdapterRegistry::GetCameraAdapterCreator(dag), _cameraAdapters, true)) {
        return;
    }
    // We are inserting a single prim and
    // instancer for every instanced mesh.
    if (dag.isInstanced() && dag.instanceNumber() > 0) {
        return;
    }

    auto adapter = Create(dag, HdMayaAdapterRegistry::GetShapeAdapterCreator(dag), _shapeAdapters);
    if (!adapter) {
        // Proxy shape is registered as base class type but plugins can derive from it
        // Check the object type and if matches proxy base class find an adapter for it.
        adapter
            = Create(dag, HdMayaAdapterRegistry::GetProxyShapeAdapterCreator(dag), _shapeAdapters);
    }
    if (adapter) {
        auto material = adapter->GetMaterial();
        if (material != MObject::kNullObj) {
            const auto materialId = GetMaterialPath(material);
            if (TfMapLookupPtr(_materialAdapters, materialId) == nullptr) {
                _CreateMaterial(materialId, material);
            }
        }
    }

    auto material = adapter->GetMaterial();
    if (material != MObject::kNullObj) {
        const auto materialId = GetMaterialPath(material);
        if (TfMapLookupPtr(_materialAdapters, materialId) == nullptr) {
            _CreateMaterial(materialId, material);
        }
    }
}

void HdMayaSceneDelegate::NodeAdded(const MObject& obj) { _addedNodes.push_back(obj); }

void HdMayaSceneDelegate::UpdateLightVisibility(const MDagPath& dag)
{
    const auto id = GetPrimPath(dag, true);
    _FindAdapter<HdMayaLightAdapter>(
        id,
        [](HdMayaLightAdapter* a) {
            if (a->UpdateVisibility()) {
                a->RemovePrim();
                a->Populate();
                a->InvalidateTransform();
            }
        },
        _lightAdapters);
}

//
void HdMayaSceneDelegate::AddNewInstance(const MDagPath& dag)
{
    MDagPathArray dags;
    MDagPath::getAllPathsTo(dag.node(), dags);
    const auto dagsLength = dags.length();
    if (dagsLength == 0) {
        return;
    }
    const auto                          masterDag = dags[0];
    const auto                          id = GetPrimPath(masterDag, false);
    std::shared_ptr<HdMayaShapeAdapter> masterAdapter;
    if (!TfMapLookup(_shapeAdapters, id, &masterAdapter) || masterAdapter == nullptr) {
        return;
    }
    // If dags is 1, we have to recreate the adapter.
    if (dags.length() == 1 || !masterAdapter->IsInstanced()) {
        RecreateAdapterOnIdle(id, masterDag.node());
    } else {
        // If dags is more than one, trigger rebuilding callbacks next call and
        // mark dirty.
        RebuildAdapterOnIdle(id, HdMayaDelegateCtx::RebuildFlagCallbacks);
        masterAdapter->MarkDirty(
            HdChangeTracker::DirtyInstancer | HdChangeTracker::DirtyInstanceIndex
            | HdChangeTracker::DirtyPrimvar);
    }
}

void HdMayaSceneDelegate::SetParams(const HdMayaParams& params)
{
    const auto& oldParams = GetParams();
    if (oldParams.displaySmoothMeshes != params.displaySmoothMeshes) {
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
        // I couldn't find any other way to turn this on / off.
        // I can't convert HdRprim to HdMesh easily and no simple way
        // to get the type of the HdRprim from the render index.
        // If we want to allow creating multiple rprims and returning an id
        // to a subtree, we need to use the HasType function and the mark dirty
        // from each adapter.
		_MapAdapter<HdMayaRenderItemAdapter>(
			[](HdMayaRenderItemAdapter* a) {
				if (
					a->HasType(HdPrimTypeTokens->mesh)
					|| a->HasType(HdPrimTypeTokens->basisCurves)
					|| a->HasType(HdPrimTypeTokens->points)
					)
				{
					a->MarkDirty(HdChangeTracker::DirtyTopology);
				}
			},
			_renderItemsAdapters);
#else
        _MapAdapter<HdMayaDagAdapter>(
            [](HdMayaDagAdapter* a) {
                if (a->HasType(HdPrimTypeTokens->mesh)) {
                    a->MarkDirty(HdChangeTracker::DirtyTopology);
                }
            },
            _shapeAdapters);
#endif
    }
    if (oldParams.motionSampleStart != params.motionSampleStart
        || oldParams.motionSampleEnd != params.motionSampleEnd) {
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
		_MapAdapter<HdMayaRenderItemAdapter>(
			[](HdMayaRenderItemAdapter* a) {
			if (
				a->HasType(HdPrimTypeTokens->mesh)
				|| a->HasType(HdPrimTypeTokens->basisCurves)
				|| a->HasType(HdPrimTypeTokens->points))
			{
				a->InvalidateTransform();
				a->MarkDirty(HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyTransform);
			}
			},
			_renderItemsAdapters);
#else
        _MapAdapter<HdMayaDagAdapter>(
            [](HdMayaDagAdapter* a) {
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
        _MapAdapter<HdMayaMaterialAdapter>(
            [](HdMayaMaterialAdapter* a) { a->MarkDirty(HdMaterial::AllDirty); },
            _materialAdapters);
    }
    if (oldParams.maximumShadowMapResolution != params.maximumShadowMapResolution) {
        _MapAdapter<HdMayaLightAdapter>(
            [](HdMayaLightAdapter* a) { a->MarkDirty(HdLight::AllDirty); }, _lightAdapters);
    }
    HdMayaDelegate::SetParams(params);
}

void HdMayaSceneDelegate::PopulateSelectedPaths(
    const MSelectionList&       mayaSelection,
    SdfPathVector&              selectedSdfPaths,
    const HdSelectionSharedPtr& selection)
{
    TF_DEBUG(HDMAYA_DELEGATE_SELECTION)
        .Msg("HdMayaSceneDelegate::PopulateSelectedPaths - %s\n", GetMayaDelegateID().GetText());

    // We need to track selected masters (but not non-instanced prims)
    // because they may not be unique when we iterate over selected items -
    // each dag path should only be iterated over once, but multiple dag
    // paths might map to the same master prim. So we use selectedMasters
    // to ensure we don't add the same master prim to selectedSdfPaths
    // more than once.
    // While there may be a LOT of instances, hopefully there shouldn't
    // be a huge number of different types of instances, so tracking this
    // won't be too bad...
    std::unordered_set<SdfPath, SdfPath::Hash> selectedMasters;
    MapSelectionDescendents(
        mayaSelection,
        [this, &selectedSdfPaths, &selectedMasters, &selection](const MDagPath& dagPath) {
            SdfPath primId;
            if (dagPath.isInstanced()) {
                auto masterDag = MDagPath();
                if (!TF_VERIFY(MDagPath::getAPathTo(dagPath.node(), masterDag))) {
                    return;
                }
                primId = GetPrimPath(masterDag, false);
            } else {
                primId = GetPrimPath(dagPath, false);
            }
            auto adapter = _shapeAdapters.find(primId);
            if (adapter == _shapeAdapters.end()) {
                return;
            }

            TF_DEBUG(HDMAYA_DELEGATE_SELECTION)
                .Msg(
                    "HdMayaSceneDelegate::PopulateSelectedPaths - calling "
                    "adapter PopulateSelectedPaths for: %s\n",
                    adapter->second->GetID().GetText());
            adapter->second->PopulateSelectedPaths(
                dagPath, selectedSdfPaths, selectedMasters, selection);
        },
        MFn::kShape);
}

#if MAYA_API_VERSION >= 20210000
void HdMayaSceneDelegate::PopulateSelectionList(
    const HdxPickHitVector&          hits,
    const MHWRender::MSelectionInfo& selectInfo,
    MSelectionList&                  selectionList,
    MPointArray&                     worldSpaceHitPts)
{
    for (const HdxPickHit& hit : hits) {
        _FindAdapter<HdMayaDagAdapter>(
            hit.objectId,
            [&hit, &selectionList, &worldSpaceHitPts](HdMayaDagAdapter* a) {
                if (a->IsInstanced()) {
                    MDagPathArray dagPaths;
                    MDagPath::getAllPathsTo(a->GetDagPath().node(), dagPaths);

                    const int numInstances = dagPaths.length();
                    if (hit.instanceIndex >= 0 && hit.instanceIndex < numInstances) {
                        selectionList.add(dagPaths[hit.instanceIndex]);
                        worldSpaceHitPts.append(
                            hit.worldSpaceHitPoint[0],
                            hit.worldSpaceHitPoint[1],
                            hit.worldSpaceHitPoint[2]);
                    }
                } else {
                    selectionList.add(a->GetDagPath());
                    worldSpaceHitPts.append(
                        hit.worldSpaceHitPoint[0],
                        hit.worldSpaceHitPoint[1],
                        hit.worldSpaceHitPoint[2]);
                }
            },
            _shapeAdapters);
    }
}
#endif

HdMeshTopology HdMayaSceneDelegate::GetMeshTopology(const SdfPath& id)
{
    TF_DEBUG(HDMAYA_DELEGATE_GET_MESH_TOPOLOGY)
        .Msg("HdMayaSceneDelegate::GetMeshTopology(%s)\n", id.GetText());
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
    return _GetValue<HdMayaRenderItemAdapter, HdMeshTopology>(
        id,
        [](HdMayaRenderItemAdapter* a) -> HdMeshTopology 
		{ 
			return std::dynamic_pointer_cast<HdMeshTopology>(a->GetTopology()) ?
				*std::dynamic_pointer_cast<HdMeshTopology>(a->GetTopology()) :
				HdMeshTopology();
		},
        _renderItemsAdapters);
#else
	return _GetValue<HdMayaShapeAdapter, HdMeshTopology>(
		id,
		[](HdMayaShapeAdapter* a) -> HdMeshTopology { return a->GetMeshTopology(); },
		_shapeAdapters);
#endif
}

//TODO HDMAYA_SCENE_RENDER_DATASERVER
HdBasisCurvesTopology HdMayaSceneDelegate::GetBasisCurvesTopology(const SdfPath& id)
{
    TF_DEBUG(HDMAYA_DELEGATE_GET_CURVE_TOPOLOGY)
        .Msg("HdMayaSceneDelegate::GetBasisCurvesTopology(%s)\n", id.GetText());
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
	return _GetValue<HdMayaRenderItemAdapter, HdBasisCurvesTopology>(
		id,
		[](HdMayaRenderItemAdapter* a) -> HdBasisCurvesTopology
		{
			return std::dynamic_pointer_cast<HdBasisCurvesTopology>(a->GetTopology()) ?
				*std::dynamic_pointer_cast<HdBasisCurvesTopology>(a->GetTopology()) :
				HdBasisCurvesTopology();
		},
		_renderItemsAdapters);
#else
    return _GetValue<HdMayaShapeAdapter, HdBasisCurvesTopology>(
        id,
        [](HdMayaShapeAdapter* a) -> HdBasisCurvesTopology { return a->GetBasisCurvesTopology(); },
        _shapeAdapters);
#endif
}

//TODO HDMAYA_SCENE_RENDER_DATASERVER
PxOsdSubdivTags HdMayaSceneDelegate::GetSubdivTags(const SdfPath& id)
{
    TF_DEBUG(HDMAYA_DELEGATE_GET_SUBDIV_TAGS)
        .Msg("HdMayaSceneDelegate::GetSubdivTags(%s)\n", id.GetText());
    return _GetValue<HdMayaShapeAdapter, PxOsdSubdivTags>(
        id,
        [](HdMayaShapeAdapter* a) -> PxOsdSubdivTags { return a->GetSubdivTags(); },
        _shapeAdapters);
}

GfRange3d HdMayaSceneDelegate::GetExtent(const SdfPath& id)
{
	// TODO HDMAYA_SCENE_RENDER_DATASERVER GetExtent, _CalculateExtent
	TF_DEBUG(HDMAYA_DELEGATE_GET_EXTENT).Msg("HdMayaSceneDelegate::GetExtent(%s)\n", id.GetText());
	return _GetValue<HdMayaShapeAdapter, GfRange3d>(
		id, [](HdMayaShapeAdapter* a) -> GfRange3d { return a->GetExtent(); }, _shapeAdapters);
}

GfMatrix4d HdMayaSceneDelegate::GetTransform(const SdfPath& id)
{
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
    TF_DEBUG(HDMAYA_DELEGATE_GET_TRANSFORM)
        .Msg("HdMayaSceneDelegate::GetTransform(%s)\n", id.GetText());
    return _GetValue<HdMayaRenderItemAdapter, GfMatrix4d>(
        id,
        [](HdMayaRenderItemAdapter* a) -> GfMatrix4d { return a->GetTransform(); },
        _renderItemsAdapters
		);
#else
    TF_DEBUG(HDMAYA_DELEGATE_GET_TRANSFORM)
        .Msg("HdMayaSceneDelegate::GetTransform(%s)\n", id.GetText());
    return _GetValue<HdMayaDagAdapter, GfMatrix4d>(
        id,
        [](HdMayaDagAdapter* a) -> GfMatrix4d { return a->GetTransform(); },
        _shapeAdapters,
        _cameraAdapters,
        _lightAdapters);
#endif
}

//TODO HDMAYA_SCENE_RENDER_DATASERVER
size_t HdMayaSceneDelegate::SampleTransform(
    const SdfPath& id,
    size_t         maxSampleCount,
    float*         times,
    GfMatrix4d*    samples)
{
    TF_DEBUG(HDMAYA_DELEGATE_SAMPLE_TRANSFORM)
        .Msg(
            "HdMayaSceneDelegate::SampleTransform(%s, %u)\n",
            id.GetText(),
            static_cast<unsigned int>(maxSampleCount));
    return _GetValue<HdMayaDagAdapter, size_t>(
        id,
        [maxSampleCount, times, samples](HdMayaDagAdapter* a) -> size_t {
            return a->SampleTransform(maxSampleCount, times, samples);
        },
        _shapeAdapters,
        _cameraAdapters,
        _lightAdapters);
}

bool HdMayaSceneDelegate::IsEnabled(const TfToken& option) const
{
    TF_DEBUG(HDMAYA_DELEGATE_IS_ENABLED)
        .Msg("HdMayaSceneDelegate::IsEnabled(%s)\n", option.GetText());
    // Maya scene can't be accessed on multiple threads,
    // so I don't think this is safe to enable.
    if (option == HdOptionTokens->parallelRprimSync) {
        return false;
    }

    TF_WARN("HdMayaSceneDelegate::IsEnabled(%s) -- Unsupported option.\n", option.GetText());
    return false;
}

VtValue HdMayaSceneDelegate::Get(const SdfPath& id, const TfToken& key)
{

#ifdef HDMAYA_SCENE_RENDER_DATASERVER
	return _GetValue<HdMayaAdapter, VtValue>(
		id,
		[&key](HdMayaAdapter* a) -> VtValue { return a->Get(key); },
		_renderItemsAdapters,
		_renderItemShaderAdapters
		);
#else
    TF_DEBUG(HDMAYA_DELEGATE_GET)
        .Msg("HdMayaSceneDelegate::Get(%s, %s)\n", id.GetText(), key.GetText());
    if (id.IsPropertyPath()) {
        return _GetValue<HdMayaDagAdapter, VtValue>(
            id.GetPrimPath(),
            [&key](HdMayaDagAdapter* a) -> VtValue { return a->GetInstancePrimvar(key); },
            _shapeAdapters);
    } else {
        return _GetValue<HdMayaAdapter, VtValue>(
            id,
            [&key](HdMayaAdapter* a) -> VtValue { return a->Get(key); },
            _shapeAdapters,
            _cameraAdapters,
            _lightAdapters,
            _materialAdapters);
    }
#endif
}

size_t HdMayaSceneDelegate::SamplePrimvar(
    const SdfPath& id,
    const TfToken& key,
    size_t         maxSampleCount,
    float*         times,
    VtValue*       samples)
{
    TF_DEBUG(HDMAYA_DELEGATE_SAMPLE_PRIMVAR)
        .Msg(
            "HdMayaSceneDelegate::SamplePrimvar(%s, %s, %u)\n",
            id.GetText(),
            key.GetText(),
            static_cast<unsigned int>(maxSampleCount));
    if (maxSampleCount < 1) {
        return 0;
    }
    if (id.IsPropertyPath()) {
        times[0] = 0.0f;
        samples[0] = _GetValue<HdMayaDagAdapter, VtValue>(
            id.GetPrimPath(),
            [&key](HdMayaDagAdapter* a) -> VtValue { return a->GetInstancePrimvar(key); },
            _shapeAdapters);
        return 1;
    } else {
        return _GetValue<HdMayaShapeAdapter, size_t>(
            id,
            [&key, maxSampleCount, times, samples](HdMayaShapeAdapter* a) -> size_t {
                return a->SamplePrimvar(key, maxSampleCount, times, samples);
            },
            _shapeAdapters);
    }
}

//virtual
TfTokenVector HdMayaSceneDelegate::GetTaskRenderTags(SdfPath const& taskId)
{
	return _GetValue<HdMayaShapeUIShaderAdapter, TfTokenVector>(
		taskId,
		[](HdMayaShapeUIShaderAdapter* a) -> TfTokenVector	{ return TfTokenVector{ a->GetShaderData().Name }; },
		_renderItemShaderAdapters);
}

void HdMayaSceneDelegate::ScheduleRenderTasks(HdTaskSharedPtrVector& tasks)
{
	for (auto shader : _renderItemShaderAdapters)
	{
		tasks.push_back(GetRenderIndex().GetTask(shader.first));
	}
}

TfToken HdMayaSceneDelegate::GetRenderTag(const SdfPath& id)
{
    TF_DEBUG(HDMAYA_DELEGATE_GET_RENDER_TAG)
        .Msg("HdMayaSceneDelegate::GetRenderTag(%s)\n", id.GetText());
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
	return _GetValue<HdMayaRenderItemAdapter, TfToken>(
		id.GetPrimPath(),
		[](HdMayaRenderItemAdapter* a) -> TfToken { return a->GetRenderTag(); },
		_renderItemsAdapters);
#else
    return _GetValue<HdMayaShapeAdapter, TfToken>(
        id.GetPrimPath(),
        [](HdMayaShapeAdapter* a) -> TfToken { return a->GetRenderTag(); },
        _shapeAdapters);
#endif
}

// TODO
HdPrimvarDescriptorVector
HdMayaSceneDelegate::GetPrimvarDescriptors(const SdfPath& id, HdInterpolation interpolation)
{
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
	TF_DEBUG(HDMAYA_DELEGATE_GET_PRIMVAR_DESCRIPTORS)
		.Msg("HdMayaSceneDelegate::GetPrimvarDescriptors(%s, %i)\n", id.GetText(), interpolation);
	return _GetValue<HdMayaRenderItemAdapter, HdPrimvarDescriptorVector>(
		id,
		[&interpolation](HdMayaRenderItemAdapter* a) -> HdPrimvarDescriptorVector {
		return a->GetPrimvarDescriptors(interpolation);
		},
		_renderItemsAdapters);
#else
    TF_DEBUG(HDMAYA_DELEGATE_GET_PRIMVAR_DESCRIPTORS)
        .Msg("HdMayaSceneDelegate::GetPrimvarDescriptors(%s, %i)\n", id.GetText(), interpolation);
    if (id.IsPropertyPath()) {
        return _GetValue<HdMayaDagAdapter, HdPrimvarDescriptorVector>(
            id.GetPrimPath(),
            [&interpolation](HdMayaDagAdapter* a) -> HdPrimvarDescriptorVector {
                return a->GetInstancePrimvarDescriptors(interpolation);
            },
            _shapeAdapters);
    } else {
        return _GetValue<HdMayaShapeAdapter, HdPrimvarDescriptorVector>(
            id,
            [&interpolation](HdMayaShapeAdapter* a) -> HdPrimvarDescriptorVector {
                return a->GetPrimvarDescriptors(interpolation);
            },
            _shapeAdapters);
    }
#endif
}

VtValue HdMayaSceneDelegate::GetLightParamValue(const SdfPath& id, const TfToken& paramName)
{
    TF_DEBUG(HDMAYA_DELEGATE_GET_LIGHT_PARAM_VALUE)
        .Msg(
            "HdMayaSceneDelegate::GetLightParamValue(%s, %s)\n", id.GetText(), paramName.GetText());
    return _GetValue<HdMayaLightAdapter, VtValue>(
        id,
        [&paramName](HdMayaLightAdapter* a) -> VtValue { return a->GetLightParamValue(paramName); },
        _lightAdapters);
}

VtValue HdMayaSceneDelegate::GetCameraParamValue(const SdfPath& cameraId, const TfToken& paramName)
{
    return _GetValue<HdMayaCameraAdapter, VtValue>(
        cameraId,
        [&paramName](HdMayaCameraAdapter* a) -> VtValue {
            return a->GetCameraParamValue(paramName);
        },
        _cameraAdapters);
}

VtIntArray
HdMayaSceneDelegate::GetInstanceIndices(const SdfPath& instancerId, const SdfPath& prototypeId)
{
    TF_DEBUG(HDMAYA_DELEGATE_GET_INSTANCE_INDICES)
        .Msg(
            "HdMayaSceneDelegate::GetInstanceIndices(%s, %s)\n",
            instancerId.GetText(),
            prototypeId.GetText());
    return _GetValue<HdMayaDagAdapter, VtIntArray>(
        instancerId.GetPrimPath(),
        [&prototypeId](HdMayaDagAdapter* a) -> VtIntArray {
            return a->GetInstanceIndices(prototypeId);
        },
        _shapeAdapters);
}

#if defined(HD_API_VERSION) && HD_API_VERSION >= 39
SdfPathVector HdMayaSceneDelegate::GetInstancerPrototypes(SdfPath const& instancerId)
{
    return { instancerId.GetPrimPath() };
}
#endif

#if defined(HD_API_VERSION) && HD_API_VERSION >= 36
SdfPath HdMayaSceneDelegate::GetInstancerId(const SdfPath& primId)
{
    TF_DEBUG(HDMAYA_DELEGATE_GET_INSTANCER_ID)
        .Msg("HdMayaSceneDelegate::GetInstancerId(%s)\n", primId.GetText());
    // Instancers don't have any instancers yet.
    if (primId.IsPropertyPath()) {
        return SdfPath();
    }
    return _GetValue<HdMayaDagAdapter, SdfPath>(
        primId, [](HdMayaDagAdapter* a) -> SdfPath { return a->GetInstancerID(); }, _shapeAdapters);
}
#endif

GfMatrix4d HdMayaSceneDelegate::GetInstancerTransform(SdfPath const& instancerId)
{
    return GfMatrix4d(1.0);
}

#if defined(HD_API_VERSION) && HD_API_VERSION >= 34
SdfPath HdMayaSceneDelegate::GetScenePrimPath(
    const SdfPath&      rprimPath,
    int                 instanceIndex,
    HdInstancerContext* instancerContext)
{
    return rprimPath;
}
#elif defined(HD_API_VERSION) && HD_API_VERSION >= 33
SdfPath HdMayaSceneDelegate::GetScenePrimPath(const SdfPath& rprimPath, int instanceIndex)
{
    return rprimPath;
}
#else
SdfPath HdMayaSceneDelegate::GetPathForInstanceIndex(
    const SdfPath& protoPrimPath,
    int            instanceIndex,
    int*           absoluteInstanceIndex,
    SdfPath*       rprimPath,
    SdfPathVector* instanceContext)
{
    if (absoluteInstanceIndex != nullptr) {
        *absoluteInstanceIndex = instanceIndex;
    }
    return {};
}
#endif

bool HdMayaSceneDelegate::GetVisible(const SdfPath& id)
{
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
	TF_DEBUG(HDMAYA_DELEGATE_GET_VISIBLE)
		.Msg("HdMayaSceneDelegate::GetVisible(%s)\n", id.GetText());
	return _GetValue<HdMayaRenderItemAdapter, bool>(
		id,
		[](HdMayaRenderItemAdapter* a) -> bool { return a->GetVisible(); },
		_renderItemsAdapters
		);
#else
    TF_DEBUG(HDMAYA_DELEGATE_GET_VISIBLE)
        .Msg("HdMayaSceneDelegate::GetVisible(%s)\n", id.GetText());
    return _GetValue<HdMayaDagAdapter, bool>(
        id,
        [](HdMayaDagAdapter* a) -> bool { return a->GetVisible(); },
        _shapeAdapters,
        _lightAdapters);
#endif
}

bool HdMayaSceneDelegate::GetDoubleSided(const SdfPath& id)
{
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
    TF_DEBUG(HDMAYA_DELEGATE_GET_DOUBLE_SIDED)
        .Msg("HdMayaSceneDelegate::GetDoubleSided(%s)\n", id.GetText());
    return _GetValue<HdMayaRenderItemAdapter, bool>(
        id, [](HdMayaRenderItemAdapter* a) -> bool { return a->GetDoubleSided(); }, _renderItemsAdapters);
#else
	TF_DEBUG(HDMAYA_DELEGATE_GET_DOUBLE_SIDED)
		.Msg("HdMayaSceneDelegate::GetDoubleSided(%s)\n", id.GetText());
	return _GetValue<HdMayaShapeAdapter, bool>(
		id, [](HdMayaShapeAdapter* a) -> bool { return a->GetDoubleSided(); }, _shapeAdapters);
#endif
}

HdCullStyle HdMayaSceneDelegate::GetCullStyle(const SdfPath& id)
{
    TF_DEBUG(HDMAYA_DELEGATE_GET_CULL_STYLE)
        .Msg("HdMayaSceneDelegate::GetCullStyle(%s)\n", id.GetText());
    return HdCullStyleDontCare;
}

HdDisplayStyle HdMayaSceneDelegate::GetDisplayStyle(const SdfPath& id)
{
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
	TF_DEBUG(HDMAYA_DELEGATE_GET_DISPLAY_STYLE)
		.Msg("HdMayaSceneDelegate::GetDisplayStyle(%s)\n", id.GetText());
	return _GetValue<HdMayaRenderItemAdapter, HdDisplayStyle>(
		id,
		[](HdMayaRenderItemAdapter* a) -> HdDisplayStyle { return a->GetDisplayStyle(); },
		_renderItemsAdapters);
#else
    TF_DEBUG(HDMAYA_DELEGATE_GET_DISPLAY_STYLE)
        .Msg("HdMayaSceneDelegate::GetDisplayStyle(%s)\n", id.GetText());
    return _GetValue<HdMayaShapeAdapter, HdDisplayStyle>(
        id,
        [](HdMayaShapeAdapter* a) -> HdDisplayStyle { return a->GetDisplayStyle(); },
        _shapeAdapters);
#endif
}

SdfPath HdMayaSceneDelegate::GetMaterialId(const SdfPath& id)
{
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
	if (!_enableMaterials)
		return {};
	auto result = TfMapLookupPtr(_renderItemsAdapters, id);
	if (result == nullptr) {
		return _fallbackMaterial;
	}

	auto& renderItemAdapter = *result;
	auto& shaderData = renderItemAdapter->GetShaderData();
	if (renderItemAdapter->GetShaderData().ShapeUIShader)
	// Do not return material for shape UI,
	// we do not want those drawn in the beauty pass,
	// these are handled via a separate draw pass
	{
		return {};
	}
	
	if (shaderData.Material ==  kInvalidMaterial) 
	{
		return _fallbackMaterial;
	}
	
	if (TfMapLookupPtr(_materialAdapters, shaderData.Material) != nullptr) 
	{
		return shaderData.Material;
	}

	// TODO
	// Why would we get here with render item prototype?
	//return _CreateMaterial(materialId, material) ? materialId : _fallbackMaterial;
	return {};
#else
	TF_DEBUG(HDMAYA_DELEGATE_GET_MATERIAL_ID)
		.Msg("HdMayaSceneDelegate::GetMaterialId(%s)\n", id.GetText());
	if (!_enableMaterials)
		return {};
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
#endif
}

VtValue HdMayaSceneDelegate::GetMaterialResource(const SdfPath& id)
{
#ifdef HDMAYA_SCENE_RENDER_DATASERVER
	// TODO this is the same as !HDMAYA_SCENE_RENDER_DATASERVER
	if (
		id == _fallbackMaterial
		)
	{
		return HdMayaMaterialAdapter::GetPreviewMaterialResource(id);
	}
	auto ret = _GetValue<HdMayaMaterialAdapter, VtValue>(
		id,
		[](HdMayaMaterialAdapter* a) -> VtValue { return a->GetMaterialResource(); },
		_materialAdapters);
	return ret.IsEmpty() ? HdMayaMaterialAdapter::GetPreviewMaterialResource(id) : ret;

#else
	TF_DEBUG(HDMAYA_DELEGATE_GET_MATERIAL_RESOURCE)
		.Msg("HdMayaSceneDelegate::GetMaterialResource(%s)\n", id.GetText());
	if (
		id == _fallbackMaterial
		) 
	{
		return HdMayaMaterialAdapter::GetPreviewMaterialResource(id);
	}
	auto ret = _GetValue<HdMayaMaterialAdapter, VtValue>(
		id,
		[](HdMayaMaterialAdapter* a) -> VtValue { return a->GetMaterialResource(); },
		_materialAdapters);
	return ret.IsEmpty() ? HdMayaMaterialAdapter::GetPreviewMaterialResource(id) : ret;
#endif
}

#if PXR_VERSION < 2011

HdTextureResource::ID HdMayaSceneDelegate::GetTextureResourceID(const SdfPath& textureId)
{
    TF_DEBUG(HDMAYA_DELEGATE_GET_TEXTURE_RESOURCE_ID)
        .Msg("HdMayaSceneDelegate::GetTextureResourceID(%s)\n", textureId.GetText());
    return _GetValue<HdMayaMaterialAdapter, HdTextureResource::ID>(
        textureId.GetPrimPath(),
        [&textureId](HdMayaMaterialAdapter* a) -> HdTextureResource::ID {
            return a->GetTextureResourceID(textureId.GetNameToken());
        },
        _materialAdapters);
}

HdTextureResourceSharedPtr HdMayaSceneDelegate::GetTextureResource(const SdfPath& textureId)
{
    TF_DEBUG(HDMAYA_DELEGATE_GET_TEXTURE_RESOURCE)
        .Msg("HdMayaSceneDelegate::GetTextureResource(%s)\n", textureId.GetText());

    auto* adapterPtr = TfMapLookupPtr(_materialAdapters, textureId);

    if (!adapterPtr) {
        // For texture nodes we may have only inserted an adapter for the material
        // not for the texture itself.
        //
        // UsdShade has the rule that a UsdShade node must be nested inside the
        // UsdMaterial scope. We traverse the parent paths to find the material.
        //
        // Example for texture prim:
        //    /Materials/Woody/BootMaterial/UsdShadeNodeGraph/Tex
        // We want to find Sprim:
        //    /Materials/Woody/BootMaterial

        // While-loop to account for nesting of UsdNodeGraphs and DrawMode
        // adapter with prototypes.
        SdfPath parentPath = textureId;
        while (!adapterPtr && !parentPath.IsRootPrimPath()) {
            parentPath = parentPath.GetParentPath();
            adapterPtr = TfMapLookupPtr(_materialAdapters, parentPath);
        }
    }

    if (adapterPtr) {
        return adapterPtr->get()->GetTextureResource(textureId);
    }

    return nullptr;
}

#endif // PXR_VERSION < 2011

bool HdMayaSceneDelegate::_CreateMaterial(const SdfPath& id, const MObject& obj)
{
    TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
        .Msg("HdMayaSceneDelegate::_CreateMaterial(%s)\n", id.GetText());

    auto materialCreator = HdMayaAdapterRegistry::GetMaterialAdapterCreator(obj);
    if (materialCreator == nullptr) {
        return false;
    }
    auto materialAdapter = materialCreator(id, this, obj);
    if (materialAdapter == nullptr || !materialAdapter->IsSupported()) {
        return false;
    }

    materialAdapter->Populate();
    materialAdapter->CreateCallbacks();
    _materialAdapters.emplace(id, std::move(materialAdapter));
    return true;
}

SdfPath HdMayaSceneDelegate::SetCameraViewport(const MDagPath& camPath, const GfVec4d& viewport)
{
    const SdfPath camID = GetPrimPath(camPath, true);
    auto&&        cameraAdapter = TfMapLookupPtr(_cameraAdapters, camID);
    if (cameraAdapter) {
        (*cameraAdapter)->SetViewport(viewport);
        return camID;
    }
    return {};
}

PXR_NAMESPACE_CLOSE_SCOPE
