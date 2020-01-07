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

#include <hdMaya/hdMaya.h>

#include <pxr/base/tf/fileUtils.h>

#include <pxr/imaging/hd/instanceRegistry.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/resourceRegistry.h>
#include <pxr/imaging/hdSt/resourceRegistry.h>

#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/glf/textureRegistry.h>

#if USD_VERSION_NUM >= 1905
#include <pxr/imaging/hio/glslfx.h>
#else
#include <pxr/imaging/glf/glslfx.h>
typedef PXR_NS::GlfGLSLFX HioGlslfx;
namespace {
auto& HioGlslfxTokens = PXR_NS::GlfGLSLFXTokens;
}
#endif // USD_VERSION_NUM >= 1905

#if USD_VERSION_NUM >= 1901
#include <pxr/imaging/glf/udimTexture.h>
#include <pxr/usdImaging/usdImaging/textureUtils.h>
#endif

#include <pxr/usdImaging/usdImaging/tokens.h>

#include <pxr/usdImaging/usdImagingGL/package.h>

#include <pxr/imaging/hdSt/textureResource.h>

#include <pxr/usd/sdf/types.h>

#include <pxr/usd/sdr/registry.h>

#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

#include "adapterRegistry.h"
#include "materialNetworkConverter.h"
#include "mayaAttrs.h"
#include "tokens.h"
#include "../utils.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

const VtValue _emptyValue;
const TfToken _emptyToken;
const TfTokenVector _stSamplerCoords = {TfToken("st")};
// const TfTokenVector _stSamplerCoords;

struct _ShaderSourceAndMeta {
    std::string surfaceCode;
    std::string displacementCode;
    VtDictionary metadata;
#ifdef HDMAYA_OIT_ENABLED
    VtDictionary translucentMetadata;
#endif
};
// We can't store the shader here explicitly, since it causes a deadlock
// due to library dependencies.
auto _PreviewShader = []() -> const _ShaderSourceAndMeta& {
    static const auto ret = []() -> _ShaderSourceAndMeta {
        auto& registry = SdrRegistry::GetInstance();
        auto sdrNode = registry.GetShaderNodeByIdentifierAndType(
            UsdImagingTokens->UsdPreviewSurface, HioGlslfxTokens->glslfx);
        if (!sdrNode) { return {"", ""}; }
        HioGlslfx gfx(sdrNode->GetSourceURI());
        _ShaderSourceAndMeta ret;
        ret.surfaceCode = gfx.GetSurfaceSource();
        ret.displacementCode = gfx.GetDisplacementSource();
        ret.metadata = gfx.GetMetadata();
#ifdef HDMAYA_OIT_ENABLED
        ret.translucentMetadata = gfx.GetMetadata();
        ret.translucentMetadata[HdShaderTokens->materialTag] =
            VtValue(HdxMaterialTagTokens->translucent);
#endif
        return ret;
    }();
    return ret;
};

#if USD_VERSION_NUM < 1901
enum class HdTextureType { Uv, Ptex, Udim };
#endif

} // namespace

HdMayaMaterialAdapter::HdMayaMaterialAdapter(
    const SdfPath& id, HdMayaDelegateCtx* delegate, const MObject& node)
    : HdMayaAdapter(node, id, delegate) {}

bool HdMayaMaterialAdapter::IsSupported() const {
    return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(
        HdPrimTypeTokens->material);
}

bool HdMayaMaterialAdapter::HasType(const TfToken& typeId) const {
    return typeId == HdPrimTypeTokens->material;
}

void HdMayaMaterialAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
}

void HdMayaMaterialAdapter::RemovePrim() {
    if (!_isPopulated) { return; }
    GetDelegate()->RemoveSprim(HdPrimTypeTokens->material, GetID());
    _isPopulated = false;
}

void HdMayaMaterialAdapter::Populate() {
    TF_DEBUG(HDMAYA_ADAPTER_GET)
        .Msg("HdMayaMaterialAdapter::Populate() - %s\n", GetID().GetText());
    if (_isPopulated) { return; }
    GetDelegate()->InsertSprim(
        HdPrimTypeTokens->material, GetID(), HdMaterial::AllDirty);
    _isPopulated = true;
}

#if USD_VERSION_NUM <= 1911

std::string HdMayaMaterialAdapter::GetSurfaceShaderSource() {
    return GetPreviewSurfaceSource();
}

std::string HdMayaMaterialAdapter::GetDisplacementShaderSource() {
    return GetPreviewDisplacementSource();
}

VtValue HdMayaMaterialAdapter::GetMaterialParamValue(const TfToken& paramName) {
    return GetPreviewMaterialParamValue(paramName);
}

HdMaterialParamVector HdMayaMaterialAdapter::GetMaterialParams() {
    return GetPreviewMaterialParams();
}

VtDictionary HdMayaMaterialAdapter::GetMaterialMetadata() {
    return _PreviewShader().metadata;
}

const HdMaterialParamVector& HdMayaMaterialAdapter::GetPreviewMaterialParams() {
    return HdMayaMaterialNetworkConverter::GetPreviewMaterialParamVector();
}

const std::string& HdMayaMaterialAdapter::GetPreviewSurfaceSource() {
    return _PreviewShader().surfaceCode;
}

const std::string& HdMayaMaterialAdapter::GetPreviewDisplacementSource() {
    return _PreviewShader().displacementCode;
}

// Specialized version of :
// https://en.cppreference.com/w/cpp/algorithm/lower_bound
HdMayaShaderParams::const_iterator _FindPreviewParam(const TfToken& id) {
    TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
        .Msg("_FindPreviewParam(id=%s)\n", id.GetText());
    HdMayaShaderParams::const_iterator it;
    typename std::iterator_traits<
        HdMayaShaderParams::const_iterator>::difference_type count,
        step;
    const auto& previewShaderParams =
        HdMayaMaterialNetworkConverter::GetPreviewShaderParams();
    auto first = previewShaderParams.cbegin();
    count = std::distance(first, previewShaderParams.cend());

    while (count > 0) {
        it = first;
        step = count / 2;
        std::advance(it, step);
#if USD_VERSION_NUM >= 1911
        if (it->param.name < id) {
#else
        if (it->param.GetName() < id) {
#endif
            first = ++it;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    return first != previewShaderParams.cend()
#if USD_VERSION_NUM >= 1911
               ? (first->param.name == id ? first
#else
               ? (first->param.GetName() == id ? first
#endif
                                               : previewShaderParams.cend())
               : first;
}

const VtValue& HdMayaMaterialAdapter::GetPreviewMaterialParamValue(
    const TfToken& paramName) {
    const auto it = _FindPreviewParam(paramName);
    if (ARCH_UNLIKELY(
            it ==
            HdMayaMaterialNetworkConverter::GetPreviewShaderParams().cend())) {
        TF_CODING_ERROR(
            "Incorrect name passed to GetMaterialParamValue: %s",
            paramName.GetText());
        return _emptyValue;
    }
#if USD_VERSION_NUM >= 1911
    return it->param.fallbackValue;
#else
    return it->param.GetFallbackValue();
#endif
}

#endif // USD_VERSION_NUM <= 1911

HdTextureResource::ID HdMayaMaterialAdapter::GetTextureResourceID(
    const TfToken& paramName) {
    return {};
}

HdTextureResourceSharedPtr HdMayaMaterialAdapter::GetTextureResource(
    const TfToken& paramName) {
    return {};
}

VtValue HdMayaMaterialAdapter::GetMaterialResource() {
    TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
        .Msg("HdMayaMaterialAdapter::GetMaterialResource()\n");
    return GetPreviewMaterialResource(GetID());
}

VtValue HdMayaMaterialAdapter::GetPreviewMaterialResource(
    const SdfPath& materialID) {
    HdMaterialNetworkMap map;
    HdMaterialNetwork network;
    HdMaterialNode node;
    node.path = materialID;
    node.identifier = UsdImagingTokens->UsdPreviewSurface;
#if USD_VERSION_NUM >= 1911
    map.terminals.push_back(node.path);
#endif
    for (const auto& it :
         HdMayaMaterialNetworkConverter::GetPreviewShaderParams()) {
        node.parameters.emplace(
#if USD_VERSION_NUM >= 1911
            it.param.name, it.param.fallbackValue);
#else
            it.param.GetName(), it.param.GetFallbackValue());
#endif
    }
    network.nodes.push_back(node);
#if USD_VERSION_NUM >= 1911
    map.map.emplace(HdMaterialTerminalTokens->surface, network);
#else
    map.map.emplace(UsdImagingTokens->bxdf, network);
#endif
    return VtValue(map);
}

class HdMayaShadingEngineAdapter : public HdMayaMaterialAdapter {
public:
    HdMayaShadingEngineAdapter(
        const SdfPath& id, HdMayaDelegateCtx* delegate, const MObject& obj)
        : HdMayaMaterialAdapter(id, delegate, obj), _surfaceShaderCallback(0) {
        _CacheNodeAndTypes();
    }

    ~HdMayaShadingEngineAdapter() override {
        if (_surfaceShaderCallback != 0) {
            MNodeMessage::removeCallback(_surfaceShaderCallback);
        }
    }

    void CreateCallbacks() override {
        TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
            .Msg(
                "Creating shading engine adapter callbacks for prim (%s).\n",
                GetID().GetText());

        MStatus status;
        auto obj = GetNode();
        auto id = MNodeMessage::addNodeDirtyCallback(
            obj, _DirtyMaterialParams, this, &status);
        if (ARCH_LIKELY(status)) { AddCallback(id); }
        _CreateSurfaceMaterialCallback();
        HdMayaAdapter::CreateCallbacks();
    }

    void Populate() override {
        HdMayaMaterialAdapter::Populate();
#ifdef HDMAYA_OIT_ENABLED
        _isTranslucent = IsTranslucent();
#endif
    }

private:
    static void _DirtyMaterialParams(MObject& /*node*/, void* clientData) {
        auto* adapter =
            reinterpret_cast<HdMayaShadingEngineAdapter*>(clientData);
        adapter->_CreateSurfaceMaterialCallback();
        adapter->MarkDirty(HdMaterial::AllDirty);
    }

    static void _DirtyShaderParams(MObject& /*node*/, void* clientData) {
        auto* adapter =
            reinterpret_cast<HdMayaShadingEngineAdapter*>(clientData);
        adapter->MarkDirty(HdMaterial::AllDirty);
        if (adapter->GetDelegate()->IsHdSt()) {
            adapter->GetDelegate()->MaterialTagChanged(adapter->GetID());
        }
    }

    void _CacheNodeAndTypes() {
        _surfaceShader = MObject::kNullObj;
        _surfaceShaderType = _emptyToken;
        MStatus status;
        MFnDependencyNode node(GetNode(), &status);
        if (ARCH_UNLIKELY(!status)) { return; }

        auto p = node.findPlug(MayaAttrs::shadingEngine::surfaceShader, true);
        MPlugArray conns;
        p.connectedTo(conns, true, false);
        if (conns.length() > 0) {
            _surfaceShader = conns[0].node();
            MFnDependencyNode surfaceNode(_surfaceShader, &status);
            if (ARCH_UNLIKELY(!status)) { return; }
            _surfaceShaderType = TfToken(surfaceNode.typeName().asChar());
            TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
                .Msg(
                    "Found surfaceShader %s[%s]\n", surfaceNode.name().asChar(),
                    _surfaceShaderType.GetText());
        }
    }

#if USD_VERSION_NUM <= 1911

    inline bool _RegisterTexture(
        const MFnDependencyNode& node, const TfToken& paramName,
        HdTextureType& textureType) {
        const auto connectedFileObj = GetConnectedFileNode(node, paramName);
        if (connectedFileObj != MObject::kNullObj) {
            const auto filePath =
                GetFileTexturePath(MFnDependencyNode(connectedFileObj));

            if (TfDebug::IsEnabled(HDMAYA_ADAPTER_MATERIALS)) {
                const char* textureTypeName;
                switch (textureType) {
                    case HdTextureType::Uv:
                        textureTypeName = "Uv";
                        break;
                    case HdTextureType::Ptex:
                        textureTypeName = "Ptex";
                        break;
                    case HdTextureType::Udim:
                        textureTypeName = "Udim";
                        break;
                    default:
                        textureTypeName = "<Unknown texture type>";
                }
                TfDebug::Helper().Msg(
                    "HdMayaShadingEngineAdapter::_RegisterTexture(%s, %s, "
                    "%s)\n",
                    node.name().asChar(), paramName.GetText(), textureTypeName);
                TfDebug::Helper().Msg(
                    "  texture filepath: %s\n", filePath.GetText());
            }

            auto textureId = _GetTextureResourceID(connectedFileObj, filePath);
            if (textureId != HdTextureResource::ID(-1)) {
                HdResourceRegistry::TextureKey textureKey =
                    GetDelegate()->GetRenderIndex().GetTextureKey(textureId);
                const auto& resourceRegistry =
                    GetDelegate()->GetRenderIndex().GetResourceRegistry();

                HdInstance<
                    HdResourceRegistry::TextureKey, HdTextureResourceSharedPtr>
                    textureInstance;
                auto regLock = resourceRegistry->RegisterTextureResource(
                    textureKey, &textureInstance);
                if (textureInstance.IsFirstInstance()) {
                    auto textureResource = GetFileTextureResource(
                        connectedFileObj, filePath,
                        GetDelegate()->GetParams().textureMemoryPerTexture);
                    _textureResources[paramName] = boost::static_pointer_cast<HdTextureResource>(textureResource);
                    textureInstance.SetValue(boost::static_pointer_cast<HdTextureResource>(textureResource));
                }
                else {
                    _textureResources[paramName] = textureInstance.GetValue();
                }

#if USD_VERSION_NUM >= 1901
                if (GlfIsSupportedUdimTexture(filePath)) {
                    if (TfDebug::IsEnabled(HDMAYA_ADAPTER_MATERIALS) &&
                        textureType != HdTextureType::Udim) {
                        TfDebug::Helper().Msg(
                            "  ...changing textureType to Udim\n");
                    }
                    textureType = HdTextureType::Udim;
                }
                TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
                    .Msg(
                        "  ...successfully registered texture with id: %lu\n",
                        textureId);
#endif
                return true;
            } else {
                TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
                    .Msg(
                        "  ...failed registering texture - could not get "
                        "textureID\n");
                _textureResources[paramName].reset();
            }
        }
        return false;
    }

    HdMaterialParamVector GetMaterialParams() override {
        MStatus status;
        MFnDependencyNode node(_surfaceShader, &status);
        if (ARCH_UNLIKELY(!status)) { return GetPreviewMaterialParams(); }

        auto* nodeConverter =
            HdMayaMaterialNodeConverter::GetNodeConverter(_surfaceShaderType);
        if (!nodeConverter) { return GetPreviewMaterialParams(); }

        HdMaterialParamVector ret;
        ret.reserve(GetPreviewMaterialParams().size());
        for (const auto& it :
             HdMayaMaterialNetworkConverter::GetPreviewShaderParams()) {
            auto textureType = HdTextureType::Uv;
#if USD_VERSION_NUM >= 1911
            auto remappedName = it.param.name;
#else
            auto remappedName = it.param.GetName();
#endif
            auto attrConverter = nodeConverter->GetAttrConverter(remappedName);
            if (attrConverter) {
                TfToken tempName = attrConverter->GetPlugName(remappedName);
                if (!tempName.IsEmpty()) { remappedName = tempName; }
            }

            if (_RegisterTexture(node, remappedName, textureType)) {
                ret.emplace_back(
#if USD_VERSION_NUM >= 1911
                    HdMaterialParam::ParamTypeTexture, it.param.name,
                    it.param.fallbackValue,
#else
                    HdMaterialParam::ParamTypeTexture, it.param.GetName(),
                    it.param.GetFallbackValue(),
#endif
                    GetID().AppendProperty(remappedName), _stSamplerCoords,
#if USD_VERSION_NUM >= 1901
                    textureType);
#else
                    false);
#endif
                TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
                    .Msg(
                        "HdMayaShadingEngineAdapter: registered texture with "
                        "connection path: %s\n",
#if USD_VERSION_NUM >= 1911
                        ret.back().connection.GetText());
#else
                        ret.back().GetConnection().GetText());
#endif
            } else {
                ret.emplace_back(it.param);
            }
        }

        return ret;
    }

    VtValue GetMaterialParamValue(const TfToken& paramName) override {
        if (ARCH_UNLIKELY(_surfaceShaderType.IsEmpty())) {
            return GetPreviewMaterialParamValue(paramName);
        }

        MStatus status;
        MFnDependencyNode node(_surfaceShader, &status);
        if (ARCH_UNLIKELY(!status)) {
            return GetPreviewMaterialParamValue(paramName);
        }

        const auto previewIt = _FindPreviewParam(paramName);
        if (ARCH_UNLIKELY(
                previewIt ==
                HdMayaMaterialNetworkConverter::GetPreviewShaderParams()
                    .cend())) {
            return HdMayaMaterialAdapter::GetPreviewMaterialParamValue(
                paramName);
        }

        auto* nodeConverter =
            HdMayaMaterialNodeConverter::GetNodeConverter(_surfaceShaderType);
        if (!nodeConverter) { return GetPreviewMaterialParamValue(paramName); }
        auto attrConverter = nodeConverter->GetAttrConverter(paramName);
        if (attrConverter) {
            return attrConverter->GetValue(
#if USD_VERSION_NUM >= 1911
                node, previewIt->param.name, previewIt->type,
                &previewIt->param.fallbackValue);
#else
                node, previewIt->param.GetName(), previewIt->type,
                &previewIt->param.GetFallbackValue());
#endif
        } else {
            return GetPreviewMaterialParamValue(paramName);
        }
    }

#endif // USD_VERSION_NUM <= 1911

    void _CreateSurfaceMaterialCallback() {
        _CacheNodeAndTypes();
        if (_surfaceShaderCallback != 0) {
            MNodeMessage::removeCallback(_surfaceShaderCallback);
            _surfaceShaderCallback = 0;
        }

        if (_surfaceShader != MObject::kNullObj) {
            _surfaceShaderCallback = MNodeMessage::addNodeDirtyCallback(
                _surfaceShader, _DirtyShaderParams, this);
        }
    }

    HdTextureResource::ID GetTextureResourceID(
        const TfToken& paramName) override {
        auto fileObj = GetConnectedFileNode(_surfaceShader, paramName);
        if (fileObj == MObject::kNullObj) { return HdTextureResource::ID(-1); }
        return _GetTextureResourceID(
            fileObj, GetFileTexturePath(MFnDependencyNode(fileObj)));
    }

    inline HdTextureResource::ID _GetTextureResourceID(
        const MObject& fileObj, const TfToken& filePath) {
        auto hash = filePath.Hash();
        const auto wrapping = GetFileTextureWrappingParams(fileObj);
        boost::hash_combine(
            hash, GetDelegate()->GetParams().textureMemoryPerTexture);
        boost::hash_combine(hash, std::get<0>(wrapping));
        boost::hash_combine(hash, std::get<1>(wrapping));
        return HdTextureResource::ID(hash);
    }

    HdTextureResourceSharedPtr GetTextureResource(
        const TfToken& paramName) override {
        auto fileObj = GetConnectedFileNode(_surfaceShader, paramName);
        if (fileObj == MObject::kNullObj) { return {}; }
        return GetFileTextureResource(
            fileObj, GetFileTexturePath(MFnDependencyNode(fileObj)),
            GetDelegate()->GetParams().textureMemoryPerTexture);
    }

    VtValue GetMaterialResource() override {
        TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
            .Msg("HdMayaShadingEngineAdapter::GetMaterialResource()\n");
        HdMaterialNetwork materialNetwork;
        HdMayaMaterialNetworkConverter converter(materialNetwork, GetID());
        if (!converter.GetMaterial(_surfaceShader)) {
            return GetPreviewMaterialResource(GetID());
        }

        HdMaterialNetworkMap materialNetworkMap;
#if USD_VERSION_NUM >= 1911
        materialNetworkMap.map[HdMaterialTerminalTokens->surface] = materialNetwork;
        if (!materialNetwork.nodes.empty()) {
            materialNetworkMap.terminals.push_back(
                materialNetwork.nodes.back().path);
        }
#else
        materialNetworkMap.map[UsdImagingTokens->bxdf] = materialNetwork;
#endif
        // HdMaterialNetwork displacementNetwork;
        // materialNetworkMap.map[HdMaterialTerminalTokens->displacement] =
        // displacementNetwork;

        return VtValue(materialNetworkMap);
    };

#ifdef HDMAYA_OIT_ENABLED
    bool UpdateMaterialTag() override {
        if (IsTranslucent() != _isTranslucent) {
            _isTranslucent = !_isTranslucent;
            return true;
        }
        return false;
    }

    bool IsTranslucent() {
        if (_surfaceShaderType == UsdImagingTokens->UsdPreviewSurface ||
            _surfaceShaderType == HdMayaAdapterTokens->pxrUsdPreviewSurface) {
            MFnDependencyNode node(_surfaceShader);
            const auto plug =
                node.findPlug(HdMayaAdapterTokens->opacity.GetText(), true);
            if (!plug.isNull() &&
                (plug.asFloat() < 1.0f || plug.isConnected())) {
                return true;
            }
        }
        return false;
    }

#if USD_VERSION_NUM <= 1911

    VtDictionary GetMaterialMetadata() override {
        // We only need the delayed update of the tag for the HdSt backend.
        if (!GetDelegate()->IsHdSt()) { _isTranslucent = IsTranslucent(); }
        return _isTranslucent ? _PreviewShader().translucentMetadata
                              : _PreviewShader().metadata;
    };

#endif // USD_VERSION_NUM <= 1911

#endif // HDMAYA_OIT_ENABLED

    MObject _surfaceShader;
    TfToken _surfaceShaderType;
    // So they live long enough
    std::unordered_map<
        TfToken, HdTextureResourceSharedPtr, TfToken::HashFunctor>
        _textureResources;
    MCallbackId _surfaceShaderCallback;
#ifdef HDMAYA_OIT_ENABLED
    bool _isTranslucent = false;
#endif
};

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<HdMayaMaterialAdapter, TfType::Bases<HdMayaAdapter>>();
    TfType::Define<
        HdMayaShadingEngineAdapter, TfType::Bases<HdMayaMaterialAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, shadingEngine) {
    HdMayaAdapterRegistry::RegisterMaterialAdapter(
        TfToken("shadingEngine"),
        [](const SdfPath& id, HdMayaDelegateCtx* delegate,
           const MObject& obj) -> HdMayaMaterialAdapterPtr {
            return HdMayaMaterialAdapterPtr(
                new HdMayaShadingEngineAdapter(id, delegate, obj));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
