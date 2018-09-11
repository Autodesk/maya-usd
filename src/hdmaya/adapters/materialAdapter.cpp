//
// Copyright 2018 Luma Pictures
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

#include <pxr/imaging/glf/glslfx.h>
#include <pxr/imaging/glf/textureRegistry.h>
#ifdef USD_HDST_UDIM_BUILD
#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/glf/udimTexture.h>
#include <pxr/usdImaging/usdImaging/textureUtils.h>
#endif

#include <pxr/usdImaging/usdImaging/tokens.h>

#include <pxr/usdImaging/usdImagingGL/package.h>

#include <pxr/imaging/hdSt/textureResource.h>

#include <pxr/usd/sdf/types.h>

#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/adapters/materialNetworkConverter.h>
#include <hdmaya/adapters/mayaAttrs.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

const VtValue _emptyValue;
const TfToken _emptyToken;
const TfTokenVector _stSamplerCoords = {TfToken("st")};
// const TfTokenVector _stSamplerCoords;
const MString _fileTextureName("fileTextureName");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens, (roughness)(clearcoat)(clearcoatRoughness)(emissiveColor)(
                 specularColor)(metallic)(useSpecularWorkflow)(occlusion)(ior)(
                 normal)(opacity)(diffuseColor)(displacement)
    // Supported material tokens.
    (lambert)(blinn)(file)(place2dTexture)
    // Other tokens
    (fileTextureName)(color)(incandescence)(out)(st)(uvCoord)(rgb)(r)(varname)(
        result)(eccentricity));

// Specialized version of :
// https://en.cppreference.com/w/cpp/algorithm/lower_bound
HdMayaShaderParams::const_iterator _FindPreviewParam(const TfToken& id) {
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
        if (it->_param.GetName() < id) {
            first = ++it;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    return first != previewShaderParams.cend()
               ? (first->_param.GetName() == id ? first
                                                : previewShaderParams.cend())
               : first;
};

static const std::pair<std::string, std::string> _previewShaderSource =
    []() -> std::pair<std::string, std::string> {
    GlfGLSLFX gfx(UsdImagingGLPackagePreviewSurfaceShader());
    return {gfx.GetSurfaceSource(), gfx.GetDisplacementSource()};
}();

std::unordered_map<
    TfToken, std::vector<std::pair<TfToken, TfToken>>, TfToken::HashFunctor>
    _materialParamRemaps{{_tokens->lambert,
                          {
                              {_tokens->diffuseColor, _tokens->color},
                              {_tokens->emissiveColor, _tokens->incandescence},
                          }},
                         {_tokens->blinn,
                          {
                              {_tokens->diffuseColor, _tokens->color},
                              {_tokens->emissiveColor, _tokens->incandescence},
                              {_tokens->specularColor, _tokens->specularColor},
                              {_tokens->roughness, _tokens->eccentricity},
                          }}};

#ifdef USD_HDST_UDIM_BUILD

class UdimTextureFactory : public GlfTextureFactoryBase {
public:
    virtual GlfTextureRefPtr New(
        TfToken const& texturePath,
        GlfImage::ImageOriginLocation originLocation =
            GlfImage::OriginUpperLeft) const override {
        const GlfContextCaps& caps = GlfContextCaps::GetInstance();
        return GlfUdimTexture::New(
            texturePath, originLocation,
            UsdImaging_GetUdimTiles(texturePath, caps.maxArrayTextureLayers));
    }

    virtual GlfTextureRefPtr New(
        TfTokenVector const& texturePaths,
        GlfImage::ImageOriginLocation originLocation =
            GlfImage::OriginUpperLeft) const override {
        return nullptr;
    }
};

#endif

} // namespace

HdMayaMaterialAdapter::HdMayaMaterialAdapter(
    const SdfPath& id, HdMayaDelegateCtx* delegate, const MObject& node)
    : HdMayaAdapter(node, id, delegate) {}

bool HdMayaMaterialAdapter::IsSupported() {
    return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(
        HdPrimTypeTokens->material);
}

bool HdMayaMaterialAdapter::HasType(const TfToken& typeId) {
    return typeId == HdPrimTypeTokens->material;
}

void HdMayaMaterialAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
}

void HdMayaMaterialAdapter::RemovePrim() {
    GetDelegate()->RemoveSprim(HdPrimTypeTokens->material, GetID());
}

void HdMayaMaterialAdapter::Populate() {
    TF_DEBUG(HDMAYA_ADAPTER_GET)
        .Msg("HdMayaMaterialAdapter::Populate() - %s\n", GetID().GetText());
    GetDelegate()->InsertSprim(
        HdPrimTypeTokens->material, GetID(), HdMaterial::AllDirty);
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
    return GetPreviewMaterialResource(GetID());
}

const HdMaterialParamVector& HdMayaMaterialAdapter::GetPreviewMaterialParams() {
    return HdMayaMaterialNetworkConverter::GetPreviewMaterialParamVector();
}

const std::string& HdMayaMaterialAdapter::GetPreviewSurfaceSource() {
    return _previewShaderSource.first;
}

const std::string& HdMayaMaterialAdapter::GetPreviewDisplacementSource() {
    return _previewShaderSource.second;
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
    return it->_param.GetFallbackValue();
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
            it._param.GetName(), it._param.GetFallbackValue());
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

        auto p = node.findPlug("surfaceShader");
        MPlugArray conns;
        p.connectedTo(conns, true, false);
        if (conns.length() > 0) {
            _surfaceShader = conns[0].node();
            MFnDependencyNode surfaceNode(_surfaceShader, &status);
            if (ARCH_UNLIKELY(!status)) { return; }
            _surfaceShaderType = TfToken(surfaceNode.typeName().asChar());
        }
    }

    HdMaterialParamVector GetMaterialParams() override {
        MStatus status;
        MFnDependencyNode node(_surfaceShader, &status);
        if (ARCH_UNLIKELY(!status)) { return GetPreviewMaterialParams(); }
        auto mIt = _materialParamRemaps.end();
        if (_surfaceShaderType != UsdImagingTokens->UsdPreviewSurface) {
            mIt = _materialParamRemaps.find(_surfaceShaderType);
            if (mIt == _materialParamRemaps.end()) {
                return GetPreviewMaterialParams();
            }
        }

        HdMaterialParamVector ret;
        ret.reserve(GetPreviewMaterialParams().size());
        for (const auto& it : GetPreviewMaterialParams()) {
#ifdef USD_HDST_UDIM_BUILD
            auto textureType = HdTextureType::Uv;
#endif
            auto remappedName = it.GetName();
            if (mIt != _materialParamRemaps.end()) {
                std::find_if(
                    mIt->second.begin(), mIt->second.end(),
                    [&remappedName](
                        const std::pair<TfToken, TfToken>& p) -> bool {
                        if (p.first == remappedName) {
                            remappedName = p.second;
                            return true;
                        }
                        return false;
                    });
            }
            if (_RegisterTexture(
                    node, remappedName
#ifdef USD_HDST_UDIM_BUILD
                    ,
                    textureType
#endif
                    )) {
                ret.emplace_back(
                    HdMaterialParam::ParamTypeTexture, it.GetName(),
                    it.GetFallbackValue(), GetID().AppendProperty(remappedName),
                    _stSamplerCoords,
#ifdef USD_HDST_UDIM_BUILD
                    textureType
#else
                    false
#endif
                );
            } else {
                ret.emplace_back(it);
            }
        }

        return ret;
    }

    inline bool _RegisterTexture(
        const MFnDependencyNode& node, const TfToken& paramName
#ifdef USD_HDST_UDIM_BUILD
        ,
        HdTextureType& textureType
#endif
    ) {
        const auto connectedFileObj = GetConnectedFileNode(node, paramName);
        if (connectedFileObj != MObject::kNullObj) {
            const auto filePath =
                _GetTextureFilePath(MFnDependencyNode(connectedFileObj));
            auto textureId = _GetTextureResourceID(filePath);
            if (textureId != HdTextureResource::ID(-1)) {
                const auto& resourceRegistry =
                    GetDelegate()->GetRenderIndex().GetResourceRegistry();
                HdInstance<HdTextureResource::ID, HdTextureResourceSharedPtr>
                    textureInstance;
                auto regLock = resourceRegistry->RegisterTextureResource(
                    textureId, &textureInstance);
                if (textureInstance.IsFirstInstance()) {
                    auto textureResource = _GetTextureResource(filePath);
                    _textureResources[paramName] = textureResource;
                    textureInstance.SetValue(textureResource);
                } else {
                    _textureResources[paramName] = textureInstance.GetValue();
                }
#ifdef USD_HDST_UDIM_BUILD
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

        auto remappedParam = paramName;
        if (_surfaceShaderType != UsdImagingTokens->UsdPreviewSurface) {
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

        const auto p = node.findPlug(remappedParam.GetText());
        if (ARCH_UNLIKELY(p.isNull())) {
            return HdMayaMaterialAdapter::GetPreviewMaterialParamValue(
                paramName);
        }
        const auto previewIt = _FindPreviewParam(paramName);
        if (ARCH_UNLIKELY(
                previewIt ==
                HdMayaMaterialNetworkConverter::GetPreviewShaderParams()
                    .cend())) {
            return HdMayaMaterialAdapter::GetPreviewMaterialParamValue(
                paramName);
        }
        const auto ret = HdMayaMaterialNetworkConverter::ConvertPlugToValue(
            p, previewIt->_type);
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
            _GetTextureFilePath(MFnDependencyNode(fileObj)));
    }

    inline HdTextureResource::ID _GetTextureResourceID(
        const TfToken& filePath) {
        size_t hash = filePath.Hash();
        boost::hash_combine(
            hash, GetDelegate()->GetParams().textureMemoryPerTexture);
        return HdTextureResource::ID(hash);
    }

    inline TfToken _GetTextureFilePath(const MFnDependencyNode& fileNode) {
#ifdef USD_HDST_UDIM_BUILD
        if (fileNode.findPlug(MayaAttrs::file::uvTilingMode).asShort() != 0) {
            auto ret =
                fileNode.findPlug(MayaAttrs::file::fileTextureNamePattern)
                    .asString();
            if (ret.length() == 0) {
                ret = fileNode
                          .findPlug(
                              MayaAttrs::file::computedFileTextureNamePattern)
                          .asString();
            }
            return TfToken(ret.asChar());
        }
#endif
        return TfToken(fileNode.findPlug(_fileTextureName).asString().asChar());
    }

    HdTextureResourceSharedPtr GetTextureResource(
        const TfToken& paramName) override {
        auto fileObj = GetConnectedFileNode(_surfaceShader, paramName);
        if (fileObj == MObject::kNullObj) { return {}; }
        return _GetTextureResource(
            _GetTextureFilePath(MFnDependencyNode(fileObj)));
    }

    inline HdTextureResourceSharedPtr _GetTextureResource(
        const TfToken& filePath) {
        if (filePath.IsEmpty()) { return {}; }
#ifdef USD_HDST_UDIM_BUILD
        auto textureType = HdTextureType::Uv;
        if (GlfIsSupportedUdimTexture(filePath)) {
            textureType = HdTextureType::Udim;
        }
#endif
        if (
#ifdef USD_HDST_UDIM_BUILD
            textureType != HdTextureType::Udim &&
#endif
            !TfPathExists(filePath)) {
            return {};
        }
        // TODO: handle origin
        const auto origin = GlfImage::OriginUpperLeft;
        GlfTextureHandleRefPtr texture = nullptr;
#ifdef USD_HDST_UDIM_BUILD
        if (textureType == HdTextureType::Udim) {
            UdimTextureFactory factory;
            texture = GlfTextureRegistry::GetInstance().GetTextureHandle(
                filePath, origin, &factory);
        } else
#endif
        {
            texture = GlfTextureRegistry::GetInstance().GetTextureHandle(
                filePath, origin);
        }

        // We can't really mimic texture wrapping and mirroring settings from
        // the uv placement node, so we don't touch those for now.
        return HdTextureResourceSharedPtr(new HdStSimpleTextureResource(
            texture,
#ifdef USD_HDST_UDIM_BUILD
            textureType,
#else
            false,
#endif
            HdWrapClamp, HdWrapClamp, HdMinFilterLinearMipmapLinear,
            HdMagFilterLinear,
            GetDelegate()->GetParams().textureMemoryPerTexture));
    }

    MObject GetConnectedFileNode(const MObject& obj, const TfToken& paramName) {
        MStatus status;
        MFnDependencyNode node(obj, &status);
        if (ARCH_UNLIKELY(!status)) { return MObject::kNullObj; }
        return GetConnectedFileNode(node, paramName);
    }

    MObject GetConnectedFileNode(
        const MFnDependencyNode& node, const TfToken& paramName) {
        MPlugArray conns;
        node.findPlug(paramName.GetText()).connectedTo(conns, true, false);
        if (conns.length() == 0) { return MObject::kNullObj; }
        const auto ret = conns[0].node();
        if (ret.apiType() == MFn::kFileTexture) { return ret; }
        return MObject::kNullObj;
    }

    VtValue GetMaterialResource() override {
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
