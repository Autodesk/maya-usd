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
#include <hdmaya/adapters/materialAdapter.h>

#include <pxr/base/tf/fileUtils.h>

#include <pxr/imaging/hd/instanceRegistry.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/resourceRegistry.h>

#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/glf/glslfx.h>
#include <pxr/imaging/glf/textureRegistry.h>
#ifdef USD_001901_BUILD
#include <pxr/imaging/glf/udimTexture.h>
#include <pxr/usdImaging/usdImaging/textureUtils.h>
#endif

#include <pxr/usdImaging/usdImaging/tokens.h>

#include <pxr/usdImaging/usdImagingGL/package.h>

#include <pxr/imaging/glf/glslfx.h>
#include <pxr/imaging/hdSt/textureResource.h>

#include <pxr/usd/sdf/types.h>

#include <pxr/usd/sdr/registry.h>

#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/adapters/materialNetworkConverter.h>
#include <hdmaya/adapters/mayaAttrs.h>
#include <hdmaya/adapters/tokens.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

const VtValue _emptyValue;
const TfToken _emptyToken;
const TfTokenVector _stSamplerCoords = {TfToken("st")};
// const TfTokenVector _stSamplerCoords;

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
        if (it->param.GetName() < id) {
            first = ++it;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    return first != previewShaderParams.cend()
               ? (first->param.GetName() == id ? first
                                               : previewShaderParams.cend())
               : first;
};
// We can't store the shader here explicitly, since it causes a deadlock
// due to library dependencies.
auto _PreviewShaderSource = []() -> const std::pair<std::string, std::string>& {
    static const auto ret = []() -> std::pair<std::string, std::string> {
        auto& registry = SdrRegistry::GetInstance();
        auto sdrNode = registry.GetShaderNodeByIdentifierAndType(
            UsdImagingTokens->UsdPreviewSurface, GlfGLSLFXTokens->glslfx);
        if (!sdrNode) { return {"", ""}; }
        GlfGLSLFX gfx(sdrNode->GetSourceURI());
        return {gfx.GetSurfaceSource(), gfx.GetDisplacementSource()};
    }();
    return ret;
};

std::unordered_map<
    TfToken, std::vector<std::pair<TfToken, TfToken>>, TfToken::HashFunctor>
    _materialParamRemaps{
        {HdMayaAdapterTokens->lambert,
         {
             {HdMayaAdapterTokens->diffuseColor, HdMayaAdapterTokens->color},
             {HdMayaAdapterTokens->emissiveColor,
              HdMayaAdapterTokens->incandescence},
         }},
        {HdMayaAdapterTokens->phong,
         {{HdMayaAdapterTokens->diffuseColor, HdMayaAdapterTokens->color},
          {HdMayaAdapterTokens->emissiveColor,
           HdMayaAdapterTokens->incandescence}}},
        {HdMayaAdapterTokens->blinn,
         {
             {HdMayaAdapterTokens->diffuseColor, HdMayaAdapterTokens->color},
             {HdMayaAdapterTokens->emissiveColor,
              HdMayaAdapterTokens->incandescence},
             {HdMayaAdapterTokens->specularColor,
              HdMayaAdapterTokens->specularColor},
             {HdMayaAdapterTokens->roughness,
              HdMayaAdapterTokens->eccentricity},
         }}};

#ifndef USD_001901_BUILD
enum class HdTextureType { Uv, Ptex, Udim };
#endif

#ifdef USD_001901_BUILD

class UdimTextureFactory : public GlfTextureFactoryBase {
public:
    virtual GlfTextureRefPtr New(
        TfToken const& texturePath,
        GlfImage::ImageOriginLocation originLocation =
            GlfImage::OriginLowerLeft) const override {
        const GlfContextCaps& caps = GlfContextCaps::GetInstance();
        return GlfUdimTexture::New(
            texturePath, originLocation,
            UsdImaging_GetUdimTiles(texturePath, caps.maxArrayTextureLayers));
    }

    virtual GlfTextureRefPtr New(
        TfTokenVector const& texturePaths,
        GlfImage::ImageOriginLocation originLocation =
            GlfImage::OriginLowerLeft) const override {
        return nullptr;
    }
};

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

const HdMaterialParamVector& HdMayaMaterialAdapter::GetPreviewMaterialParams() {
    return HdMayaMaterialNetworkConverter::GetPreviewMaterialParamVector();
}

const std::string& HdMayaMaterialAdapter::GetPreviewSurfaceSource() {
    return _PreviewShaderSource().first;
}

const std::string& HdMayaMaterialAdapter::GetPreviewDisplacementSource() {
    return _PreviewShaderSource().second;
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
    return it->param.GetFallbackValue();
}

VtValue HdMayaMaterialAdapter::GetPreviewMaterialResource(
    const SdfPath& materialID) {
    HdMaterialNetworkMap map;
    HdMaterialNetwork network;
    HdMaterialNode node;
    node.path = materialID;
    node.identifier = UsdImagingTokens->UsdPreviewSurface;
    for (const auto& it :
         HdMayaMaterialNetworkConverter::GetPreviewShaderParams()) {
        node.parameters.emplace(
            it.param.GetName(), it.param.GetFallbackValue());
    }
    network.nodes.push_back(node);
    map.map.emplace(UsdImagingTokens->bxdf, network);
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
        MStatus status;
        auto obj = GetNode();
        auto id = MNodeMessage::addNodeDirtyCallback(
            obj, _DirtyMaterialParams, this, &status);
        if (ARCH_LIKELY(status)) { AddCallback(id); }
        _CreateSurfaceMaterialCallback();
        HdMayaAdapter::CreateCallbacks();
    }

private:
    static void _DirtyMaterialParams(MObject& /*node*/, void* clientData) {
        auto* adapter =
            reinterpret_cast<HdMayaShadingEngineAdapter*>(clientData);
        adapter->_CreateSurfaceMaterialCallback();
        adapter->MarkDirty(
            HdMaterial::DirtyParams | HdMaterial::DirtySurfaceShader |
            HdMaterial::DirtyResource);
    }

    static void _DirtyShaderParams(MObject& /*node*/, void* clientData) {
        auto* adapter =
            reinterpret_cast<HdMayaShadingEngineAdapter*>(clientData);
        adapter->MarkDirty(
            HdMaterial::DirtyParams | HdMaterial::DirtySurfaceShader |
            HdMaterial::DirtyResource);
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

    HdMaterialParamVector GetMaterialParams() override {
        MStatus status;
        MFnDependencyNode node(_surfaceShader, &status);
        if (ARCH_UNLIKELY(!status)) { return GetPreviewMaterialParams(); }
        auto mIt = _materialParamRemaps.end();
        if (_surfaceShaderType != UsdImagingTokens->UsdPreviewSurface &&
            _surfaceShaderType != HdMayaAdapterTokens->pxrUsdPreviewSurface) {
            mIt = _materialParamRemaps.find(_surfaceShaderType);
            if (mIt == _materialParamRemaps.end()) {
                return GetPreviewMaterialParams();
            }
        }

        HdMaterialParamVector ret;
        ret.reserve(GetPreviewMaterialParams().size());
        for (const auto& it : GetPreviewMaterialParams()) {
            auto textureType = HdTextureType::Uv;
            auto remappedName = it.GetName();
            if (mIt != _materialParamRemaps.end()) {
                std::find_if(
                    mIt->second.begin(), mIt->second.end(),
                    [&remappedName](
                        const std::pair<TfToken, TfToken>& p) -> bool {
                        if (p.first == remappedName) {
                            remappedName = p.second;
                            TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
                                .Msg(
                                    "Found remapped material param %s\n",
                                    remappedName.GetText());
                            return true;
                        }
                        return false;
                    });
            }
            if (_RegisterTexture(node, remappedName, textureType)) {
                ret.emplace_back(
                    HdMaterialParam::ParamTypeTexture, it.GetName(),
                    it.GetFallbackValue(), GetID().AppendProperty(remappedName),
                    _stSamplerCoords,
#ifdef USD_001901_BUILD
                    textureType);
#else
                    false);
#endif
            } else {
                ret.emplace_back(it);
            }
        }

        return ret;
    }

    inline bool _RegisterTexture(
        const MFnDependencyNode& node, const TfToken& paramName,
        HdTextureType& textureType) {
        const auto connectedFileObj = GetConnectedFileNode(node, paramName);
        if (connectedFileObj != MObject::kNullObj) {
            const auto filePath =
                _GetTextureFilePath(MFnDependencyNode(connectedFileObj));
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
                    auto textureResource =
                        _GetTextureResource(connectedFileObj, filePath);
                    _textureResources[paramName] = textureResource;
                    textureInstance.SetValue(textureResource);
                } else {
                    _textureResources[paramName] = textureInstance.GetValue();
                }
#ifdef USD_001901_BUILD
                if (GlfIsSupportedUdimTexture(filePath)) {
                    textureType = HdTextureType::Udim;
                }
#endif
                return true;
            } else {
                _textureResources[paramName].reset();
            }
        }
        return false;
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

        auto remappedParam = paramName;
        if (_surfaceShaderType != UsdImagingTokens->UsdPreviewSurface &&
            _surfaceShaderType != HdMayaAdapterTokens->pxrUsdPreviewSurface) {
            auto mIt = _materialParamRemaps.find(_surfaceShaderType);
            if (mIt != _materialParamRemaps.end()) {
                const auto remapIt = std::find_if(
                    mIt->second.begin(), mIt->second.end(),
                    [&paramName](const std::pair<TfToken, TfToken>& p) -> bool {
                        return p.first == paramName;
                    });
                if (remapIt != mIt->second.end()) {
                    remappedParam = remapIt->second;
                }
            } else {
                return GetPreviewMaterialParamValue(paramName);
            }
        }

        const auto ret = HdMayaMaterialNetworkConverter::ConvertMayaAttrToValue(
            node, remappedParam.GetText(), previewIt->type);
        if (ARCH_UNLIKELY(ret.IsEmpty())) {
            return HdMayaMaterialAdapter::GetPreviewMaterialParamValue(
                paramName);
        }
        return ret;
    }

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
            fileObj, _GetTextureFilePath(MFnDependencyNode(fileObj)));
    }

    inline HdTextureResource::ID _GetTextureResourceID(
        const MObject& fileObj, const TfToken& filePath) {
        auto hash = filePath.Hash();
        const auto wrapping = _GetWrappingParams(fileObj);
        boost::hash_combine(
            hash, GetDelegate()->GetParams().textureMemoryPerTexture);
        boost::hash_combine(hash, std::get<0>(wrapping));
        boost::hash_combine(hash, std::get<1>(wrapping));
        return HdTextureResource::ID(hash);
    }

    inline TfToken _GetTextureFilePath(const MFnDependencyNode& fileNode) {
        if (fileNode.findPlug(MayaAttrs::file::uvTilingMode, true).asShort() !=
            0) {
            auto ret =
                fileNode.findPlug(MayaAttrs::file::fileTextureNamePattern, true)
                    .asString();
            if (ret.length() == 0) {
                ret = fileNode
                          .findPlug(
                              MayaAttrs::file::computedFileTextureNamePattern,
                              true)
                          .asString();
            }
            return TfToken(ret.asChar());
        }
        return TfToken(fileNode.findPlug(MayaAttrs::file::fileTextureName, true)
                           .asString()
                           .asChar());
    }

    HdTextureResourceSharedPtr GetTextureResource(
        const TfToken& paramName) override {
        auto fileObj = GetConnectedFileNode(_surfaceShader, paramName);
        if (fileObj == MObject::kNullObj) { return {}; }
        return _GetTextureResource(
            fileObj, _GetTextureFilePath(MFnDependencyNode(fileObj)));
    }

    inline HdTextureResourceSharedPtr _GetTextureResource(
        const MObject& fileObj, const TfToken& filePath) {
        if (filePath.IsEmpty()) { return {}; }
        auto textureType = HdTextureType::Uv;
#ifdef USD_001901_BUILD
        if (GlfIsSupportedUdimTexture(filePath)) {
            textureType = HdTextureType::Udim;
        }
#endif
        if (textureType != HdTextureType::Udim && !TfPathExists(filePath)) {
            return {};
        }
        // TODO: handle origin
        const auto origin = GlfImage::OriginLowerLeft;
        GlfTextureHandleRefPtr texture = nullptr;
        if (textureType == HdTextureType::Udim) {
#ifdef USD_001901_BUILD
            UdimTextureFactory factory;
            texture = GlfTextureRegistry::GetInstance().GetTextureHandle(
                filePath, origin, &factory);
#else
            return nullptr;
#endif
        } else {
            texture = GlfTextureRegistry::GetInstance().GetTextureHandle(
                filePath, origin);
        }

        const auto wrapping = _GetWrappingParams(fileObj);

        // We can't really mimic texture wrapping and mirroring settings from
        // the uv placement node, so we don't touch those for now.
        return HdTextureResourceSharedPtr(new HdStSimpleTextureResource(
            texture,
#ifdef USD_001901_BUILD
            textureType,
#else
            false,
#endif
            std::get<0>(wrapping), std::get<1>(wrapping),
            HdMinFilterLinearMipmapLinear, HdMagFilterLinear,
            GetDelegate()->GetParams().textureMemoryPerTexture));
    }

    inline std::tuple<HdWrap, HdWrap> _GetWrappingParams(
        const MObject& fileObj) {
        constexpr std::tuple<HdWrap, HdWrap> def{HdWrapClamp, HdWrapClamp};
        MStatus status;
        MFnDependencyNode fileNode(fileObj, &status);
        if (!status) { return def; }

        auto getWrap = [&fileNode](MObject& wrapAttr, MObject& mirrorAttr) {
            if (fileNode.findPlug(wrapAttr, true).asBool()) {
                if (fileNode.findPlug(mirrorAttr, true).asBool()) {
                    return HdWrapMirror;
                } else {
                    return HdWrapRepeat;
                }
            } else {
                return HdWrapClamp;
            }
        };
        return std::tuple<HdWrap, HdWrap>{
            getWrap(MayaAttrs::file::wrapU, MayaAttrs::file::mirrorU),
            getWrap(MayaAttrs::file::wrapV, MayaAttrs::file::mirrorV)};
    };

    MObject GetConnectedFileNode(const MObject& obj, const TfToken& paramName) {
        MStatus status;
        MFnDependencyNode node(obj, &status);
        if (ARCH_UNLIKELY(!status)) { return MObject::kNullObj; }
        return GetConnectedFileNode(node, paramName);
    }

    MObject GetConnectedFileNode(
        const MFnDependencyNode& node, const TfToken& paramName) {
        MPlugArray conns;
        node.findPlug(paramName.GetText(), true)
            .connectedTo(conns, true, false);
        if (conns.length() == 0) { return MObject::kNullObj; }
        const auto ret = conns[0].node();
        if (ret.apiType() == MFn::kFileTexture) { return ret; }
        return MObject::kNullObj;
    }

    VtValue GetMaterialResource() override {
        TF_DEBUG(HDMAYA_ADAPTER_MATERIALS)
            .Msg("HdMayaShadingEngineAdapter::GetMaterialResource()\n");
        HdMaterialNetwork materialNetwork;
        HdMayaMaterialNetworkConverter converter(materialNetwork, GetID());
        if (converter.GetMaterial(_surfaceShader).IsEmpty()) {
            return GetPreviewMaterialResource(GetID());
        }

        HdMaterialNetworkMap materialNetworkMap;
        materialNetworkMap.map[UsdImagingTokens->bxdf] = materialNetwork;
        // HdMaterialNetwork displacementNetwork;
        // materialNetworkMap.map[UsdImagingTokens->displacement] =
        // displacementNetwork;

        return VtValue(materialNetworkMap);
    };

    MObject _surfaceShader;
    TfToken _surfaceShaderType;
    // So they live long enough
    std::unordered_map<
        TfToken, HdTextureResourceSharedPtr, TfToken::HashFunctor>
        _textureResources;
    MCallbackId _surfaceShaderCallback;
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
