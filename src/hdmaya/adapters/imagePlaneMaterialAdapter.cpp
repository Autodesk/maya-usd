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
#ifdef LUMA_USD_BUILD
#include <hdmaya/adapters/materialAdapter.h>

#include <pxr/base/tf/fileUtils.h>

#include <pxr/imaging/hd/instanceRegistry.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/resourceRegistry.h>

#include <pxr/imaging/glf/glslfx.h>
#include <pxr/imaging/glf/textureRegistry.h>

#include <pxr/usdImaging/usdImagingGL/package.h>

#include <pxr/imaging/hdSt/textureResource.h>

#include <pxr/usd/sdf/types.h>

#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MRenderUtil.h>

#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/adapters/mayaAttrs.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

const char* _simpleTexturedSurfaceSource =
    R"SURFACE(-- glslfx version 0.1

#import $TOOLS/glf/shaders/simpleLighting.glslfx

-- configuration
{
    "techniques": {
        "default": {
            "surfaceShader": {
                "source": [ "simpleTexturedSurface.Surface" ]
            }
        }
    }
}

-- glsl simpleTexturedSurface.Surface

vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord)
{
#if defined(HD_HAS_emissiveColor)
    return HdGet_emissiveColor();
#else
    return vec4(1.0, 0.0, 0.0, 1.0);
#endif
})SURFACE";

static const std::pair<std::string, std::string> _textureShaderSource =
    []() -> std::pair<std::string, std::string> {
    std::istringstream ss(_simpleTexturedSurfaceSource);
    GlfGLSLFX gfx(ss);
    return {gfx.GetSurfaceSource(), gfx.GetDisplacementSource()};
}();

const TfTokenVector _stSamplerCoords = {TfToken("st")};
TF_DEFINE_PRIVATE_TOKENS(_tokens, (emissiveColor));
const MString _imageName("imageName");

} // namespace

class HdMayaImagePlaneMaterialAdapter : public HdMayaMaterialAdapter {
public:
    HdMayaImagePlaneMaterialAdapter(
        const SdfPath& id, HdMayaDelegateCtx* delegate, const MObject& obj)
        : HdMayaMaterialAdapter(id, delegate, obj) {}

    std::string GetSurfaceShaderSource() { return _textureShaderSource.first; }

    std::string GetDisplacementShaderSource() {
        return _textureShaderSource.second;
    }

    void CreateCallbacks() override {
        MStatus status;
        auto obj = GetNode();
        auto id = MNodeMessage::addNodeDirtyCallback(
            obj, _DirtyMaterialParams, this, &status);
        if (ARCH_LIKELY(status)) { AddCallback(id); }
        HdMayaAdapter::CreateCallbacks();
    }

    inline bool _RegisterTexture(
        const MFnDependencyNode& node, const TfToken& paramName) {
        const auto filePath = _GetTextureFilePath(node);
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
            return true;
        } else {
            _textureResources[paramName].reset();
        }
        return false;
    }

    HdMaterialParamVector GetMaterialParams() override {
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg("HdMayaImagePlaneMaterialAdapter::GetMaterialParams()\n");
        MStatus status;
        MFnDependencyNode node(_node, &status);
        if (ARCH_UNLIKELY(!status)) { return {}; }

        if (_RegisterTexture(node, _tokens->emissiveColor)) {
            HdMaterialParam emission(
                HdMaterialParam::ParamTypeTexture, _tokens->emissiveColor,
                VtValue(GfVec4f(0.0f, 0.0f, 0.0f, 1.0f)),
                GetID().AppendProperty(_tokens->emissiveColor),
                _stSamplerCoords);
            return {emission};
        }
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg("Unexpected failure to register texture\n");
        return {};
    }

    VtValue GetMaterialParamValue(const TfToken& paramName) override {
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg("Unexpected call to GetMaterialParamValue\n");
        return VtValue(GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
    }

private:
    static void _DirtyMaterialParams(
        MObject& node, MPlug& plug, void* clientData) {
        auto* adapter =
            reinterpret_cast<HdMayaImagePlaneMaterialAdapter*>(clientData);
        if (plug == MayaAttrs::imagePlane::imageName ||
            plug == MayaAttrs::imagePlane::frameExtension ||
            plug == MayaAttrs::imagePlane::frameOffset ||
            plug == MayaAttrs::imagePlane::useFrameExtension) {
            adapter->MarkDirty(HdMaterial::AllDirty);
        }
    }

    inline HdTextureResource::ID _GetTextureResourceID(
        const TfToken& filePath) {
        size_t hash = filePath.Hash();
        boost::hash_combine(
            hash, GetDelegate()->GetParams().textureMemoryPerTexture);
        return HdTextureResource::ID(hash);
    }

    inline TfToken _GetTextureFilePath(
        const MFnDependencyNode& imagePlaneNode) {
        MString imageNameExtracted =
            MRenderUtil::exactImagePlaneFileName(imagePlaneNode.object());
        return TfToken(std::string(imageNameExtracted.asChar()));
    }

    inline HdTextureResourceSharedPtr _GetTextureResource(
        const TfToken& filePath) {
        if (filePath.IsEmpty() || !TfPathExists(filePath)) { return {}; }
        // TODO: handle origin
        auto texture =
            GlfTextureRegistry::GetInstance().GetTextureHandle(filePath);
        // We can't really mimic texture wrapping and mirroring settings from
        // the uv placement node, so we don't touch those for now.
        return HdTextureResourceSharedPtr(new HdStSimpleTextureResource(
            texture, false, false, HdWrapClamp, HdWrapClamp,
            HdMinFilterLinearMipmapLinear, HdMagFilterLinear,
            GetDelegate()->GetParams().textureMemoryPerTexture));
    }

    HdTextureResourceSharedPtr GetTextureResource(
        const TfToken& paramName) override {
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg(
                "Called "
                "HdMayaImagePlaneMaterialAdapter::GetTextureResource()\n");
        if (_node == MObject::kNullObj) { return {}; }
        return _GetTextureResource(
            _GetTextureFilePath(MFnDependencyNode(_node)));
    }

    HdTextureResource::ID GetTextureResourceID(
        const TfToken& paramName) override {
        if (_node == MObject::kNullObj) { return {}; }
        return _GetTextureResourceID(
            _GetTextureFilePath(MFnDependencyNode(_node)));
    }

    // So they live long enough
    std::unordered_map<
        TfToken, HdTextureResourceSharedPtr, TfToken::HashFunctor>
        _textureResources;
};

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<
        HdMayaImagePlaneMaterialAdapter,
        TfType::Bases<HdMayaMaterialAdapter> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, shadingEngine) {
    HdMayaAdapterRegistry::RegisterMaterialAdapter(
        TfToken("imagePlane"),
        [](const SdfPath& id, HdMayaDelegateCtx* delegate,
           const MObject& obj) -> HdMayaMaterialAdapterPtr {
            return HdMayaMaterialAdapterPtr(
                new HdMayaImagePlaneMaterialAdapter(id, delegate, obj));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
#endif // LUMA_USD_BUILD