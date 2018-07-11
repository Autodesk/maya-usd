#include <hdmaya/adapters/materialAdapter.h>

#include <pxr/imaging/hd/material.h>

#include <pxr/imaging/glf/glslfx.h>

#include <pxr/usdImaging/usdImagingGL/package.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

static const std::pair<std::string, std::string> _previewShaderSource = []() -> std::pair<std::string, std::string> {
    GlfGLSLFX gfx(UsdImagingGLPackagePreviewSurfaceShader());
    return {gfx.GetSurfaceSource(), gfx.GetDisplacementSource()};
} ();

const VtValue _emptyValue;

const HdMaterialParamVector _defaultShaderParams = {
    {
        HdMaterialParam::ParamTypeFallback,
        TfToken("roughness"),
        VtValue(0.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("clearcoat"),
        VtValue(0.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("clearcoatRoughness"),
        VtValue(0.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("emissiveColor"),
        VtValue(GfVec3f(0.0f, 0.0f, 0.0f))
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("specularColor"),
        VtValue(GfVec3f(0.0f, 0.0f, 0.0f))
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("metallic"),
        VtValue(0.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("useSpecularWorkflow"),
        VtValue(1)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("occlusion"),
        VtValue(1.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("ior"),
        VtValue(1.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("normal"),
        VtValue(GfVec3f(1.0f, 1.0f, 1.0f))
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("opacity"),
        VtValue(1.0f)
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("diffuseColor"),
        VtValue(GfVec3f(1.0, 1.0, 1.0))
    }, {
        HdMaterialParam::ParamTypeFallback,
        TfToken("displacement"),
        VtValue(0.0f)
    },
};

}

HdMayaMaterialAdapter::HdMayaMaterialAdapter(const MObject& node, HdMayaDelegateCtx* delegate)
    : HdMayaAdapter(node, delegate->GetMaterialPath(node), delegate) {

}

bool
HdMayaMaterialAdapter::IsSupported() {
    return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->material);
}

bool
HdMayaMaterialAdapter::HasType(const TfToken& typeId) {
    return typeId == HdPrimTypeTokens->material;
}

void
HdMayaMaterialAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
}

void
HdMayaMaterialAdapter::RemovePrim() {
    GetDelegate()->RemoveSprim(HdPrimTypeTokens->material, GetID());
}


void
HdMayaMaterialAdapter::Populate() {
    GetDelegate()->InsertSprim(HdPrimTypeTokens->material, GetID(), HdMaterial::AllDirty);
}

std::string
HdMayaMaterialAdapter::GetSurfaceShaderSource() {
    return GetPreviewSurfaceSource();
}

std::string
HdMayaMaterialAdapter::GetDisplacementShaderSource() {
    return GetPreviewDisplacementSource();
}

VtValue
HdMayaMaterialAdapter::GetMaterialParamValue(const TfToken& paramName) {
    return GetPreviewMaterialParamValue(paramName);
}

HdMaterialParamVector
HdMayaMaterialAdapter::GetMaterialParams() {
    return GetPreviewParams();
}

const HdMaterialParamVector&
HdMayaMaterialAdapter::GetPreviewParams() {
    return _defaultShaderParams;
}

const std::string&
HdMayaMaterialAdapter::GetPreviewSurfaceSource() {
    return _previewShaderSource.first;
}

const std::string&
HdMayaMaterialAdapter::GetPreviewDisplacementSource() {
    return _previewShaderSource.second;
}

const VtValue&
HdMayaMaterialAdapter::GetPreviewMaterialParamValue(const TfToken& paramName) {
    auto it = std::find_if(
        _defaultShaderParams.begin(), _defaultShaderParams.end(),
        [paramName] (const HdMaterialParam& param) -> bool
        {
            return param.GetName() == paramName;
        });
    if (ARCH_UNLIKELY(it == _defaultShaderParams.end())) {
        TF_CODING_ERROR("Incorrect name passed to GetMaterialParamValue: %s", paramName.GetText());
        return _emptyValue;
    }
    return it->GetFallbackValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
