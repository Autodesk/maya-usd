//
// Copyright 2024 Autodesk
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
#include "pxr/base/gf/math.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/sdf/types.h"
#include "shadingTokens.h"
#include "usdMaterialWriter.h"

#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

#include <cmath>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

template <class T> T _GetMayaAttributeValue(const MPlug& attrPlug)
{
    SdfValueTypeName typeName = SdfValueTypeNames->Float;
    if (std::is_same<T, GfVec3f>()) {
        typeName = SdfValueTypeNames->Color3f;
    }

    VtValue value = UsdMayaWriteUtil::GetVtValue(attrPlug, typeName);

    if (!TF_VERIFY(
            !value.IsEmpty() && value.IsHolding<T>(),
            "No value found for '%s'. Incorrect type?\n",
            MFnAttribute(attrPlug.attribute()).name().asChar())) {
        return {};
    }

    return value.UncheckedGet<T>();
}

template <class T>
T _GetMayaAttributeValue(const MFnDependencyNode& depNodeFn, const TfToken& attrName)
{
    MStatus status;
    MPlug   attrPlug = depNodeFn.findPlug(
        depNodeFn.attribute(attrName.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (!TF_VERIFY(
            status == MS::kSuccess && !attrPlug.isNull(),
            "Invalid plug for attribute '%s'\n",
            MFnAttribute(attrPlug.attribute()).name().asChar())) {
        return {};
    }

    return _GetMayaAttributeValue<T>(attrPlug);
}

void _AuthorEmissionAndDiffuse(
    const MFnDependencyNode& depNodeFn,
    UsdShadeShader&          shaderSchema,
    const UsdTimeCode        usdTime)
{
    // All these OpenPBR attribute contribute to Diffuse:
    const auto baseWeight
        = _GetMayaAttributeValue<float>(depNodeFn, TrMayaOpenPBRTokens->baseWeight);
    const auto baseColor
        = _GetMayaAttributeValue<GfVec3f>(depNodeFn, TrMayaOpenPBRTokens->baseColor);
    const auto baseMetalness
        = _GetMayaAttributeValue<float>(depNodeFn, TrMayaOpenPBRTokens->baseMetalness);
    const auto specularWeight
        = _GetMayaAttributeValue<float>(depNodeFn, TrMayaOpenPBRTokens->specularWeight);
    const auto subsurfaceWeight
        = _GetMayaAttributeValue<float>(depNodeFn, TrMayaOpenPBRTokens->subsurfaceWeight);
    const auto subsurfaceColor
        = _GetMayaAttributeValue<GfVec3f>(depNodeFn, TrMayaOpenPBRTokens->subsurfaceColor);
    const auto coatWeight
        = _GetMayaAttributeValue<float>(depNodeFn, TrMayaOpenPBRTokens->coatWeight);
    const auto coatColor
        = _GetMayaAttributeValue<GfVec3f>(depNodeFn, TrMayaOpenPBRTokens->coatColor);
    const auto coatIOR = _GetMayaAttributeValue<float>(depNodeFn, TrMayaOpenPBRTokens->coatIOR);
    const auto coatDarkening
        = _GetMayaAttributeValue<float>(depNodeFn, TrMayaOpenPBRTokens->coatDarkening);

    // Diffuse: Converting from OpenPBR to StandardSurface requires a bit of math to compute coat
    // darkening effects.
    const auto Emetal = baseColor * specularWeight;
    const auto Edielectric = GfLerp(subsurfaceWeight, baseColor, subsurfaceColor);
    const auto Ebase = GfLerp(baseMetalness, Edielectric, Emetal);
    const auto coat_ior_to_F0_sqrt = (coatIOR - 1) / (coatIOR + 1);
    const auto coat_ior_to_F0 = coat_ior_to_F0_sqrt * coat_ior_to_F0_sqrt;
    const auto one_minus_coat_F0_over_eta2 = (1 - coat_ior_to_F0) / (coatIOR * coatIOR);
    const auto Kcoat = 1 - one_minus_coat_F0_over_eta2;
    const auto base_darkening = GfCompDiv(GfVec3f(1 - Kcoat), (GfVec3f(1.0) - (Ebase * Kcoat)));
    const auto modulated_base_darkening
        = GfLerp(coatWeight * coatDarkening, GfVec3f(1.0), base_darkening);

    // Intermediate StandardSurface values (until we get a direct OpenPBR to UsdPreviewSurface
    // graph)
    const auto ss_base_color = GfCompMult(baseColor, modulated_base_darkening);
    const auto ss_base = baseWeight;
    const auto ss_coat_color = coatColor;
    const auto ss_coat = coatWeight;

    // Using NG_standard_surface_to_UsdPreviewSurface
    const auto scaledBaseColor = ss_base_color * ss_base;
    const auto coatAttenuation = GfLerp(ss_coat, GfVec3f { 1.0F }, ss_coat_color);

    const auto albedoOpaqueDielectric = GfLerp(subsurfaceWeight, scaledBaseColor, subsurfaceColor);
    const auto ps_diffuseColor = GfCompMult(albedoOpaqueDielectric, coatAttenuation);
    auto       diffuseColorInput = shaderSchema.CreateInput(
        PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName, SdfValueTypeNames->Color3f);
    diffuseColorInput.Set(ps_diffuseColor, usdTime);

    // EmissionColor requires checking for OpenPBR Surface v1.2 for a potential emissionWeight:
    const auto emissionLuminance
        = _GetMayaAttributeValue<float>(depNodeFn, TrMayaOpenPBRTokens->emissionLuminance);
    if (emissionLuminance == 0.0) {
        return;
    }

    const auto emissionColor
        = _GetMayaAttributeValue<GfVec3f>(depNodeFn, TrMayaOpenPBRTokens->emissionColor);

    GfVec3f scaledEmissionColor;
    if (depNodeFn.hasAttribute(TrMayaOpenPBRTokens->emissionWeight.GetText())) {
        const auto emissionWeight
            = _GetMayaAttributeValue<float>(depNodeFn, TrMayaOpenPBRTokens->emissionWeight);
        if (emissionWeight == 0.0) {
            return;
        }
        scaledEmissionColor = emissionColor * emissionWeight;
    } else {
        scaledEmissionColor = emissionColor * emissionLuminance / 1000.0F;
    }

    const auto ps_emissiveColor = GfCompMult(scaledEmissionColor, coatAttenuation);
    auto       emissiveColorInput = shaderSchema.CreateInput(
        PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName, SdfValueTypeNames->Color3f);
    emissiveColorInput.Set(ps_emissiveColor, usdTime);
}

void _AuthorClearcoat(
    const MFnDependencyNode& depNodeFn,
    UsdShadeShader&          shaderSchema,
    const UsdTimeCode        usdTime)
{
    // Clearcoat:
    //  GfVec3f ss_coat_color = op_coat_color;
    //  float ss_coat = op_coat_weight;
    //  GfVec3f coatColor = ss_coat * ss_coat_color;
    //  float ps_clearcoat = GfDot(coatColor, GfVec3(1/3));
    // So:
    //  GfVec3f coatColor = op_coat_weight * op_coat_color;
    //  float ps_clearcoat = GfDot(coatColor, GfVec3(1/3));
    // Trigger:
    //  Can only happen if coat_weight > 0
    MStatus status;

    MPlug weightNodePlug = depNodeFn.findPlug(
        depNodeFn.attribute(TrMayaOpenPBRTokens->coatWeight.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (!UsdMayaUtil::IsAuthored(weightNodePlug)) {
        // Ignore this unauthored Maya attribute and return success.
        return;
    }

    const bool isDestination = weightNodePlug.isDestination(&status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeInput shaderInput = shaderSchema.CreateInput(
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName, SdfValueTypeNames->Float);

    if (isDestination) {
        return;
    }

    const auto coatColor
        = _GetMayaAttributeValue<GfVec3f>(depNodeFn, TrMayaOpenPBRTokens->coatColor);
    const auto coatWeight = _GetMayaAttributeValue<float>(weightNodePlug);

    float clearcoat = GfDot(coatColor * coatWeight, GfVec3f(1.0f / 3.0f));

    shaderInput.Set(clearcoat, usdTime);
}

void _AuthorRoughness(
    const MFnDependencyNode& depNodeFn,
    UsdShadeShader&          shaderSchema,
    const UsdTimeCode        usdTime)
{
    // Roughness:
    //  float ss_specular_roughness = GfLerp(op_coat_weight, op_specular_roughness,
    //  op_coat_roughness); float ps_roughness = ss_specular_roughness;
    MStatus status;

    MPlug specularRoughnessPlug = depNodeFn.findPlug(
        depNodeFn.attribute(TrMayaOpenPBRTokens->specularRoughness.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return;
    }

    const bool isDestination = specularRoughnessPlug.isDestination(&status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeInput shaderInput = shaderSchema.CreateInput(
        PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName, SdfValueTypeNames->Float);

    if (isDestination) {
        return;
    }

    float       previewRoughness = _GetMayaAttributeValue<float>(specularRoughnessPlug);
    const float coatWeight
        = _GetMayaAttributeValue<float>(depNodeFn, TrMayaOpenPBRTokens->coatWeight);
    if (coatWeight) {
        const auto coatRoughness
            = _GetMayaAttributeValue<float>(depNodeFn, TrMayaOpenPBRTokens->coatRoughness);
        previewRoughness = GfLerp(coatWeight, previewRoughness, coatRoughness);
    }

    shaderInput.Set(previewRoughness, usdTime);
}
} // namespace

class PxrUsdTranslators_OpenPBRSurfaceWriter : public PxrUsdTranslators_MaterialWriter
{
    typedef PxrUsdTranslators_MaterialWriter BaseClass;

public:
    PxrUsdTranslators_OpenPBRSurfaceWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;

    TfToken GetShadingAttributeNameForMayaAttrName(const TfToken& mayaAttrName) override;
};

PXRUSDMAYA_REGISTER_SHADER_WRITER(openPBRSurface, PxrUsdTranslators_OpenPBRSurfaceWriter);

PxrUsdTranslators_OpenPBRSurfaceWriter::PxrUsdTranslators_OpenPBRSurfaceWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : BaseClass(depNodeFn, usdPath, jobCtx)
{
}

/* virtual */
void PxrUsdTranslators_OpenPBRSurfaceWriter::Write(const UsdTimeCode& usdTime)
{
    BaseClass::Write(usdTime);

    MStatus status;

    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeShader shaderSchema(_usdPrim);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not get UsdShadeShader schema for UsdPrim at path '%s'\n",
            _usdPrim.GetPath().GetText())) {
        return;
    }

    // We are basing the exporter on a concatenation of the MaterialX math found in
    // NG_open_pbr_surface_to_standard_surface
    //

    // Emission and Diffuse are non-trivial and affected by coat color:
    _AuthorEmissionAndDiffuse(depNodeFn, shaderSchema, usdTime);

    // Metallic: trivial
    //  float ss_metalness = op_base_metalness;
    //  float ps_metallic = ss_metalness;

    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        TrMayaOpenPBRTokens->baseMetalness,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName,
        usdTime);

    // Please note that the MaterialX recommended translation completely ignores
    // useSpecularWorkflow and specularColor, so these are never converted.

    // IOR: trivial
    //  float ss_specular_IOR = op_specular_ior;
    //  float ps_ior = ss_specular_IOR
    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        TrMayaOpenPBRTokens->specularIOR,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->IorAttrName,
        usdTime);

    // Roughness is non-trivial:
    _AuthorRoughness(depNodeFn, shaderSchema, usdTime);

    // Clearcoat is complex:
    _AuthorClearcoat(depNodeFn, shaderSchema, usdTime);

    // ClearcoatRoughness:
    //  float ss_coat_roughness = op_coat_roughness;
    //  float ps_clearcoatRoughness = ss_coat_roughness;
    // So a direct copy:
    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        TrMayaOpenPBRTokens->coatRoughness,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName,
        usdTime);

    // Opacity:
    //  GfVec3f ss_opacity = GfVec3f(op_geometry_opacity);
    //  float ps_opacity = GfDot(ss_opacity, GfVec3f(1.0f/3.0f));
    // Reducing to:
    //  float ps_opacity = op_geometry_opacity
    MPlug opacityPlug = depNodeFn.findPlug(
        depNodeFn.attribute(TrMayaOpenPBRTokens->geometryOpacity.GetText()),
        /* wantNetworkedPlug = */ true,
        &status);
    if (UsdMayaUtil::IsAuthored(opacityPlug) || opacityPlug.isDestination()) {
        UsdShadeInput opacityInput = shaderSchema.CreateInput(
            PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName, SdfValueTypeNames->Float);

        if (!opacityPlug.isDestination()) {
            VtValue opacityValue
                = UsdMayaWriteUtil::GetVtValue(opacityPlug, SdfValueTypeNames->Float);

            opacityInput.Set(opacityValue, usdTime);
        }
    }

    // Exported, but unsupported in hdStorm.
    AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        TrMayaOpenPBRTokens->normalCamera,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->NormalAttrName,
        usdTime,
        /* ignoreIfUnauthored = */ true,
        /* inputTypeName = */ SdfValueTypeNames->Normal3f);
}

/* virtual */
TfToken PxrUsdTranslators_OpenPBRSurfaceWriter::GetShadingAttributeNameForMayaAttrName(
    const TfToken& mayaAttrName)
{
    TfToken usdAttrName;

    if (mayaAttrName == TrMayaOpenPBRTokens->baseColor) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName;
    } else if (mayaAttrName == TrMayaOpenPBRTokens->emissionColor) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName;
    } else if (mayaAttrName == TrMayaOpenPBRTokens->baseMetalness) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName;
    } else if (mayaAttrName == TrMayaOpenPBRTokens->specularIOR) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->IorAttrName;
    } else if (mayaAttrName == TrMayaOpenPBRTokens->specularRoughness) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName;
    } else if (mayaAttrName == TrMayaOpenPBRTokens->coatWeight) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName;
    } else if (mayaAttrName == TrMayaOpenPBRTokens->coatRoughness) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName;
    } else if (mayaAttrName == TrMayaOpenPBRTokens->geometryOpacity) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName;
    } else if (mayaAttrName == TrMayaOpenPBRTokens->normalCamera) {
        usdAttrName = PxrMayaUsdPreviewSurfaceTokens->NormalAttrName;
    } else {
        return BaseClass::GetShadingAttributeNameForMayaAttrName(mayaAttrName);
    }

    return UsdShadeUtils::GetFullName(usdAttrName, UsdShadeAttributeType::Input);
}

PXR_NAMESPACE_CLOSE_SCOPE
