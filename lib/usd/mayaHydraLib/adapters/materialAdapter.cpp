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
#include "materialAdapter.h"

#include <mayaHydraLib/adapters/adapterRegistry.h>
#include <mayaHydraLib/adapters/materialNetworkConverter.h>
#include <mayaHydraLib/adapters/mayaAttrs.h>
#include <mayaHydraLib/adapters/tokens.h>
#include <mayaHydraLib/utils.h>

#include <pxr/base/tf/stl.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

const VtValue       _emptyValue;
const TfToken       _emptyToken;
const TfTokenVector _stSamplerCoords = { TfToken("st") };

} // namespace

MayaHydraMaterialAdapter::MayaHydraMaterialAdapter(
    const SdfPath&     id,
    MayaHydraDelegateCtx* delegate,
    const MObject&     node)
    : MayaHydraAdapter(node, id, delegate)
{
}

bool MayaHydraMaterialAdapter::IsSupported() const
{
    return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->material);
}

bool MayaHydraMaterialAdapter::HasType(const TfToken& typeId) const
{
    return typeId == HdPrimTypeTokens->material;
}

void MayaHydraMaterialAdapter::MarkDirty(HdDirtyBits dirtyBits)
{
    GetDelegate()->GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
}

void MayaHydraMaterialAdapter::RemovePrim()
{
    if (!_isPopulated) {
        return;
    }
    GetDelegate()->RemoveSprim(HdPrimTypeTokens->material, GetID());
    _isPopulated = false;
}

void MayaHydraMaterialAdapter::Populate()
{
    TF_DEBUG(MAYAHYDRALIB_ADAPTER_GET).Msg("MayaHydraMaterialAdapter::Populate() - %s\n", GetID().GetText());
    if (_isPopulated) {
        return;
    }
    GetDelegate()->InsertSprim(HdPrimTypeTokens->material, GetID(), HdMaterial::AllDirty);
    _isPopulated = true;
}

void MayaHydraMaterialAdapter::EnableXRayShadingMode(bool enable)
{
    _enableXRayShadingMode = enable;
    MarkDirty(HdMaterial::DirtyParams);
}

VtValue MayaHydraMaterialAdapter::GetMaterialResource()
{
    TF_DEBUG(MAYAHYDRALIB_ADAPTER_MATERIALS).Msg("MayaHydraMaterialAdapter::GetMaterialResource()\n");
    return GetPreviewMaterialResource(GetID());
}

VtValue MayaHydraMaterialAdapter::GetPreviewMaterialResource(const SdfPath& materialID)
{
    HdMaterialNetworkMap map;
    HdMaterialNetwork    network;
    HdMaterialNode       node;
    node.path = materialID;
    node.identifier = UsdImagingTokens->UsdPreviewSurface;
    map.terminals.push_back(node.path);
    for (const auto& it : MayaHydraMaterialNetworkConverter::GetPreviewShaderParams()) {
        node.parameters.emplace(it.name, it.fallbackValue);
    }
    network.nodes.push_back(node);
    map.map.emplace(HdMaterialTerminalTokens->surface, network);
    return VtValue(map);
}

class MayaHydraShadingEngineAdapter : public MayaHydraMaterialAdapter
{
public:
    typedef MayaHydraMaterialNetworkConverter::PathToMobjMap PathToMobjMap;

    MayaHydraShadingEngineAdapter(const SdfPath& id, MayaHydraDelegateCtx* delegate, const MObject& obj)
        : MayaHydraMaterialAdapter(id, delegate, obj)
        , _surfaceShaderCallback(0)
    {
        _CacheNodeAndTypes();
    }

    ~MayaHydraShadingEngineAdapter() override
    {
        if (_surfaceShaderCallback != 0) {
            MNodeMessage::removeCallback(_surfaceShaderCallback);
        }
    }

    void CreateCallbacks() override
    {
        TF_DEBUG(MAYAHYDRALIB_ADAPTER_CALLBACKS)
            .Msg("Creating shading engine adapter callbacks for prim (%s).\n", GetID().GetText());

        MStatus status;
        auto    obj = GetNode();
        auto    id = MNodeMessage::addNodeDirtyCallback(obj, _DirtyMaterialParams, this, &status);
        if (ARCH_LIKELY(status)) {
            AddCallback(id);
        }
        _CreateSurfaceMaterialCallback();
        MayaHydraAdapter::CreateCallbacks();
    }

    void Populate() override
    {
        MayaHydraMaterialAdapter::Populate();
#ifdef MAYAHYDRALIB_OIT_ENABLED
        _isTranslucent = IsTranslucent();
#endif
    }

private:
    static void _DirtyMaterialParams(MObject& /*node*/, void* clientData)
    {
        auto* adapter = reinterpret_cast<MayaHydraShadingEngineAdapter*>(clientData);
        adapter->_CreateSurfaceMaterialCallback();
        adapter->MarkDirty(HdMaterial::AllDirty);
    }

    static void _DirtyShaderParams(MObject& /*node*/, void* clientData)
    {
        auto* adapter = reinterpret_cast<MayaHydraShadingEngineAdapter*>(clientData);
        adapter->MarkDirty(HdMaterial::AllDirty);
        if (adapter->GetDelegate()->IsHdSt()) {
            adapter->GetDelegate()->MaterialTagChanged(adapter->GetID());
        }
    }

    void _CacheNodeAndTypes()
    {
        _surfaceShader = MObject::kNullObj;
        _surfaceShaderType = _emptyToken;
        MStatus           status;
        MFnDependencyNode node(GetNode(), &status);
        if (ARCH_UNLIKELY(!status)) {
            return;
        }

        auto       p = node.findPlug(MayaAttrs::shadingEngine::surfaceShader, true);
        MPlugArray conns;
        p.connectedTo(conns, true, false);
        if (conns.length() > 0) {
            _surfaceShader = conns[0].node();
            MFnDependencyNode surfaceNode(_surfaceShader, &status);
            if (ARCH_UNLIKELY(!status)) {
                return;
            }
            _surfaceShaderType = TfToken(surfaceNode.typeName().asChar());
            TF_DEBUG(MAYAHYDRALIB_ADAPTER_MATERIALS)
                .Msg(
                    "Found surfaceShader %s[%s]\n",
                    surfaceNode.name().asChar(),
                    _surfaceShaderType.GetText());
        }
    }

    void _CreateSurfaceMaterialCallback()
    {
        _CacheNodeAndTypes();
        if (_surfaceShaderCallback != 0) {
            MNodeMessage::removeCallback(_surfaceShaderCallback);
            _surfaceShaderCallback = 0;
        }

        if (_surfaceShader != MObject::kNullObj) {
            _surfaceShaderCallback
                = MNodeMessage::addNodeDirtyCallback(_surfaceShader, _DirtyShaderParams, this);
        }
    }

    VtValue GetMaterialResource() override
    {
        TF_DEBUG(MAYAHYDRALIB_ADAPTER_MATERIALS)
            .Msg("MayaHydraShadingEngineAdapter::GetMaterialResource(): %s\n", GetID().GetText());
        MayaHydraMaterialNetworkConverter::MayaHydraMaterialNetworkConverterInit initStruct(GetID(), _enableXRayShadingMode, &_materialPathToMobj);
        
        MayaHydraMaterialNetworkConverter converter(initStruct);
        if (!converter.GetMaterial(_surfaceShader)) {
            return GetPreviewMaterialResource(GetID());
        }

        HdMaterialNetworkMap materialNetworkMap;
        materialNetworkMap.map[HdMaterialTerminalTokens->surface] = initStruct._materialNetwork;
        if (!initStruct._materialNetwork.nodes.empty()) {
            materialNetworkMap.terminals.push_back(initStruct._materialNetwork.nodes.back().path);
        }

        // HdMaterialNetwork displacementNetwork;
        // materialNetworkMap.map[HdMaterialTerminalTokens->displacement] =
        // displacementNetwork;

        return VtValue(materialNetworkMap);
    };

#ifdef MAYAHYDRALIB_OIT_ENABLED
    bool UpdateMaterialTag() override
    {
        if (IsTranslucent() != _isTranslucent) {
            _isTranslucent = !_isTranslucent;
            return true;
        }
        return false;
    }

    bool IsTranslucent()
    {
        if (_surfaceShaderType == MayaHydraAdapterTokens->usdPreviewSurface
            || _surfaceShaderType == MayaHydraAdapterTokens->pxrUsdPreviewSurface) {
            MFnDependencyNode node(_surfaceShader);
            const auto        plug = node.findPlug(MayaHydraAdapterTokens->opacity.GetText(), true);
            if (!plug.isNull() && (plug.asFloat() < 1.0f || plug.isConnected())) {
                return true;
            }
        }
        return false;
    }

#endif // MAYAHYDRALIB_OIT_ENABLED

    PathToMobjMap _materialPathToMobj;

    MObject _surfaceShader;
    TfToken _surfaceShaderType;
    // So they live long enough

    MCallbackId _surfaceShaderCallback;
#ifdef MAYAHYDRALIB_OIT_ENABLED
    bool _isTranslucent = false;
#endif
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<MayaHydraMaterialAdapter, TfType::Bases<MayaHydraAdapter>>();
    TfType::Define<MayaHydraShadingEngineAdapter, TfType::Bases<MayaHydraMaterialAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(MayaHydraAdapterRegistry, shadingEngine)
{
    MayaHydraAdapterRegistry::RegisterMaterialAdapter(
        TfToken("shadingEngine"),
        [](const SdfPath&     id,
           MayaHydraDelegateCtx* delegate,
           const MObject&     obj) -> MayaHydraMaterialAdapterPtr {
            return MayaHydraMaterialAdapterPtr(new MayaHydraShadingEngineAdapter(id, delegate, obj));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
