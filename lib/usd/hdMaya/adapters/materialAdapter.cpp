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

#include <hdMaya/adapters/adapterRegistry.h>
#include <hdMaya/adapters/materialNetworkConverter.h>
#include <hdMaya/adapters/mayaAttrs.h>
#include <hdMaya/adapters/tokens.h>
#include <hdMaya/utils.h>

#include <pxr/base/tf/stl.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

#if USD_VERSION_NUM < 2011
#include <pxr/imaging/hdSt/textureResourceHandle.h>
#endif // USD_VERSION_NUM < 2011

PXR_NAMESPACE_OPEN_SCOPE

namespace {

const VtValue       _emptyValue;
const TfToken       _emptyToken;
const TfTokenVector _stSamplerCoords = { TfToken("st") };
// const TfTokenVector _stSamplerCoords;

} // namespace

HdMayaMaterialAdapter::HdMayaMaterialAdapter(
    const SdfPath&     id,
    HdMayaDelegateCtx* delegate,
    const MObject&     node)
    : HdMayaAdapter(node, id, delegate)
{
}

bool HdMayaMaterialAdapter::IsSupported() const
{
    return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->material);
}

bool HdMayaMaterialAdapter::HasType(const TfToken& typeId) const
{
    return typeId == HdPrimTypeTokens->material;
}

void HdMayaMaterialAdapter::MarkDirty(HdDirtyBits dirtyBits)
{
    GetDelegate()->GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
}

void HdMayaMaterialAdapter::RemovePrim()
{
    if (!_isPopulated) {
        return;
    }
    GetDelegate()->RemoveSprim(HdPrimTypeTokens->material, GetID());
    _isPopulated = false;
}

void HdMayaMaterialAdapter::Populate()
{
    TF_DEBUG(HDMAYA_ADAPTER_GET).Msg("HdMayaMaterialAdapter::Populate() - %s\n", GetID().GetText());
    if (_isPopulated) {
        return;
    }
    GetDelegate()->InsertSprim(HdPrimTypeTokens->material, GetID(), HdMaterial::AllDirty);
    _isPopulated = true;
}

#if USD_VERSION_NUM < 2011

HdTextureResource::ID HdMayaMaterialAdapter::GetTextureResourceID(const TfToken& paramName)
{
    return {};
}

HdTextureResourceSharedPtr HdMayaMaterialAdapter::GetTextureResource(const SdfPath& textureShaderId)
{
    return {};
}

#endif // USD_VERSION_NUM < 2011

VtValue HdMayaMaterialAdapter::GetMaterialResource()
{
    TF_DEBUG(HDMAYA_ADAPTER_MATERIALS).Msg("HdMayaMaterialAdapter::GetMaterialResource()\n");
    return GetPreviewMaterialResource(GetID());
}

VtValue HdMayaMaterialAdapter::GetPreviewMaterialResource(const SdfPath& materialID)
{
    HdMaterialNetworkMap map;
    HdMaterialNetwork    network;
    HdMaterialNode       node;
    node.path = materialID;
    node.identifier = UsdImagingTokens->UsdPreviewSurface;
    map.terminals.push_back(node.path);
    for (const auto& it : HdMayaMaterialNetworkConverter::GetPreviewShaderParams()) {
        node.parameters.emplace(it.name, it.fallbackValue);
    }
    network.nodes.push_back(node);
    map.map.emplace(HdMaterialTerminalTokens->surface, network);
    return VtValue(map);
}

class HdMayaShadingEngineAdapter : public HdMayaMaterialAdapter
{
public:
    typedef HdMayaMaterialNetworkConverter::PathToMobjMap PathToMobjMap;

    HdMayaShadingEngineAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate, const MObject& obj)
        : HdMayaMaterialAdapter(id, delegate, obj)
        , _surfaceShaderCallback(0)
    {
        _CacheNodeAndTypes();
    }

    ~HdMayaShadingEngineAdapter() override
    {
        if (_surfaceShaderCallback != 0) {
            MNodeMessage::removeCallback(_surfaceShaderCallback);
        }
    }

    void CreateCallbacks() override
    {
        TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
            .Msg("Creating shading engine adapter callbacks for prim (%s).\n", GetID().GetText());

        MStatus status;
        auto    obj = GetNode();
        auto    id = MNodeMessage::addNodeDirtyCallback(obj, _DirtyMaterialParams, this, &status);
        if (ARCH_LIKELY(status)) {
            AddCallback(id);
        }
        _CreateSurfaceMaterialCallback();
        HdMayaAdapter::CreateCallbacks();
    }

    void Populate() override
    {
        HdMayaMaterialAdapter::Populate();
#ifdef HDMAYA_OIT_ENABLED
        _isTranslucent = IsTranslucent();
#endif
    }

private:
    static void _DirtyMaterialParams(MObject& /*node*/, void* clientData)
    {
        auto* adapter = reinterpret_cast<HdMayaShadingEngineAdapter*>(clientData);
        adapter->_CreateSurfaceMaterialCallback();
        adapter->MarkDirty(HdMaterial::AllDirty);
    }

    static void _DirtyShaderParams(MObject& /*node*/, void* clientData)
    {
        auto* adapter = reinterpret_cast<HdMayaShadingEngineAdapter*>(clientData);
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
            TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
                .Msg(
                    "Found surfaceShader %s[%s]\n",
                    surfaceNode.name().asChar(),
                    _surfaceShaderType.GetText());
        }
    }

#if USD_VERSION_NUM < 2011

    HdTextureResourceSharedPtr GetTextureResource(const SdfPath& textureShaderId) override
    {
        TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
            .Msg(
                "HdMayaShadingEngineAdapter::GetTextureResource(%s): %s\n",
                textureShaderId.GetText(),
                GetID().GetText());
        if (GetDelegate()->IsHdSt()) {
            auto* mObjPtr = TfMapLookupPtr(_materialPathToMobj, textureShaderId);
            if (!mObjPtr || (*mObjPtr) == MObject::kNullObj) {
                return {};
            }
            return GetFileTextureResource(
                *mObjPtr,
                GetFileTexturePath(MFnDependencyNode(*mObjPtr)),
                GetDelegate()->GetParams().textureMemoryPerTexture);
        }
        return {};
    }

#endif // USD_VERSION_NUM < 2011

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

#if USD_VERSION_NUM < 2011

    inline HdTextureResource::ID
    _GetTextureResourceID(const MObject& fileObj, const TfToken& filePath)
    {
        auto       hash = filePath.Hash();
        const auto wrapping = GetFileTextureWrappingParams(fileObj);
        boost::hash_combine(hash, GetDelegate()->GetParams().textureMemoryPerTexture);
        boost::hash_combine(hash, std::get<0>(wrapping));
        boost::hash_combine(hash, std::get<1>(wrapping));
        return HdTextureResource::ID(hash);
    }

#endif // USD_VERSION_NUM < 2011

    VtValue GetMaterialResource() override
    {
        TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
            .Msg("HdMayaShadingEngineAdapter::GetMaterialResource(): %s\n", GetID().GetText());
        HdMaterialNetwork              materialNetwork;
        HdMayaMaterialNetworkConverter converter(materialNetwork, GetID(), &_materialPathToMobj);
        if (!converter.GetMaterial(_surfaceShader)) {
            return GetPreviewMaterialResource(GetID());
        }

        HdMaterialNetworkMap materialNetworkMap;
        materialNetworkMap.map[HdMaterialTerminalTokens->surface] = materialNetwork;
        if (!materialNetwork.nodes.empty()) {
            materialNetworkMap.terminals.push_back(materialNetwork.nodes.back().path);
        }

        // HdMaterialNetwork displacementNetwork;
        // materialNetworkMap.map[HdMaterialTerminalTokens->displacement] =
        // displacementNetwork;

        return VtValue(materialNetworkMap);
    };

#ifdef HDMAYA_OIT_ENABLED
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
        if (_surfaceShaderType == HdMayaAdapterTokens->usdPreviewSurface
            || _surfaceShaderType == HdMayaAdapterTokens->pxrUsdPreviewSurface) {
            MFnDependencyNode node(_surfaceShader);
            const auto        plug = node.findPlug(HdMayaAdapterTokens->opacity.GetText(), true);
            if (!plug.isNull() && (plug.asFloat() < 1.0f || plug.isConnected())) {
                return true;
            }
        }
        return false;
    }

#endif // HDMAYA_OIT_ENABLED

    PathToMobjMap _materialPathToMobj;

    MObject _surfaceShader;
    TfToken _surfaceShaderType;
    // So they live long enough

#if USD_VERSION_NUM < 2011

    std::unordered_map<TfToken, HdStTextureResourceHandleSharedPtr, TfToken::HashFunctor>
        _textureResourceHandles;

#endif // USD_VERSION_NUM < 2011

    MCallbackId _surfaceShaderCallback;
#ifdef HDMAYA_OIT_ENABLED
    bool _isTranslucent = false;
#endif
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaMaterialAdapter, TfType::Bases<HdMayaAdapter>>();
    TfType::Define<HdMayaShadingEngineAdapter, TfType::Bases<HdMayaMaterialAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, shadingEngine)
{
    HdMayaAdapterRegistry::RegisterMaterialAdapter(
        TfToken("shadingEngine"),
        [](const SdfPath&     id,
           HdMayaDelegateCtx* delegate,
           const MObject&     obj) -> HdMayaMaterialAdapterPtr {
            return HdMayaMaterialAdapterPtr(new HdMayaShadingEngineAdapter(id, delegate, obj));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
