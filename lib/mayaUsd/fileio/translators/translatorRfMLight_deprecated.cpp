//
// Copyright 2017 Pixar
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
#include "pxr/usd/sdf/types.h"
#include "translatorRfMLight.h"

#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/gamma.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/type.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/domeLight.h>
#if PXR_VERSION < 2111
#include <pxr/usd/usdLux/light.h>
#else
#include <pxr/usd/usdLux/lightAPI.h>
#endif
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdLux/shadowAPI.h>
#include <pxr/usd/usdLux/shapingAPI.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((UsdSchemaBase, "UsdSchemaBase"))

    // RenderMan for Maya light types.
    ((AovLightMayaTypeName, "PxrAovLight"))
    ((EnvDayLightMayaTypeName, "PxrEnvDayLight"))

    // Light plug names.
    ((IntensityPlugName, "intensity"))
    ((ExposurePlugName, "exposure"))
    ((DiffuseAmountPlugName, "diffuse"))
    ((SpecularAmountPlugName, "specular"))
    ((NormalizePowerPlugName, "areaNormalize"))
    ((ColorPlugName, "lightColor"))
    ((EnableTemperaturePlugName, "enableTemperature"))
    ((TemperaturePlugName, "temperature"))

    // Type-specific Light plug names.
    ((DistantLightAnglePlugName, "angleExtent"))
    ((TextureFilePlugName, "lightColorMap"))

    // PxrAovLight plug names.
    ((AovNamePlugName, "aovName"))
    ((InPrimaryHitPlugName, "inPrimaryHit"))
    ((InReflectionPlugName, "inReflection"))
    ((InRefractionPlugName, "inRefraction"))
    ((InvertPlugName, "invert"))
    ((OnVolumeBoundariesPlugName, "onVolumeBoundaries"))
    ((UseColorPlugName, "useColor"))
    ((UseThroughputPlugName, "useThroughput"))

    // PxrEnvDayLight plug names.
    ((DayPlugName, "day"))
    ((HazinessPlugName, "haziness"))
    ((HourPlugName, "hour"))
    ((LatitudePlugName, "latitude"))
    ((LongitudePlugName, "longitude"))
    ((MonthPlugName, "month"))
    ((SkyTintPlugName, "skyTint"))
    ((SunDirectionPlugName, "sunDirection"))
    ((SunSizePlugName, "sunSize"))
    ((SunTintPlugName, "sunTint"))
    ((YearPlugName, "year"))
    ((ZonePlugName, "zone"))

    // ShapingAPI plug names.
    ((FocusPlugName, "emissionFocus"))
    ((FocusTintPlugName, "emissionFocusTint"))
    ((ConeAnglePlugName, "coneAngle"))
    ((ConeSoftnessPlugName, "coneSoftness"))
    ((ProfileFilePlugName, "iesProfile"))
    ((ProfileScalePlugName, "iesProfileScale"))
    ((ProfileNormalizePlugName, "iesProfileNormalize"))

    // ShadowAPI plug names.
    ((EnableShadowsPlugName, "enableShadows"))
    ((ShadowColorPlugName, "shadowColor"))
    ((ShadowDistancePlugName, "shadowDistance"))
    ((ShadowFalloffPlugName, "shadowFalloff"))
    ((ShadowFalloffGammaPlugName, "shadowFalloffGamma"))
);
// clang-format on

static inline TfToken _ShaderAttrName(const std::string& shaderParamName)
{
    return TfToken(UsdShadeTokens->inputs.GetString() + shaderParamName);
}

static inline std::string _PrefixRiLightAttrNamespace(const std::string& shaderParamName)
{
    static const std::string& riLightNS = "ri:light:";
    return riLightNS + shaderParamName;
}

// Adapted from UsdSchemaBase::_CreateAttr
static UsdAttribute _SetLightPrimAttr(
    UsdPrim&                lightPrim,
    TfToken const&          attrName,
    SdfValueTypeName const& typeName,
    bool                    custom,
    SdfVariability          variability,
    VtValue const&          defaultValue,
    bool                    writeSparsely)
{

    const TfToken& attrToken = _ShaderAttrName(_PrefixRiLightAttrNamespace(attrName));

    if (writeSparsely && !custom) {
        UsdAttribute attr = lightPrim.GetAttribute(attrToken);
        VtValue      fallback;
        if (defaultValue.IsEmpty()
            || (!attr.HasAuthoredValue() && attr.Get(&fallback) && fallback == defaultValue)) {
            return attr;
        }
    }
    UsdAttribute attr(lightPrim.CreateAttribute(attrToken, typeName, custom, variability));
    if (attr && !defaultValue.IsEmpty()) {
        attr.Set(defaultValue);
    }

    return attr;
}

// INTENSITY

#if PXR_VERSION < 2111
static bool _WriteLightIntensity(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteLightIntensity(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    MStatus     status;
    const MPlug lightIntensityPlug = depFn.findPlug(_tokens->IntensityPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float mayaLightIntensity = 1.0f;
    status = lightIntensityPlug.getValue(mayaLightIntensity);
    if (status != MS::kSuccess) {
        return false;
    }

    lightSchema.CreateIntensityAttr(VtValue(mayaLightIntensity), true);

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadLightIntensity(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadLightIntensity(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    MStatus status;
    MPlug   lightIntensityPlug = depFn.findPlug(_tokens->IntensityPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightIntensity = 1.0f;
    lightSchema.GetIntensityAttr().Get(&lightIntensity);

    status = lightIntensityPlug.setValue(lightIntensity);

    return (status == MS::kSuccess);
}

// EXPOSURE

#if PXR_VERSION < 2111
static bool _WriteLightExposure(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteLightExposure(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    MStatus     status;
    const MPlug lightExposurePlug = depFn.findPlug(_tokens->ExposurePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float mayaLightExposure = 0.0f;
    status = lightExposurePlug.getValue(mayaLightExposure);
    if (status != MS::kSuccess) {
        return false;
    }

    lightSchema.CreateExposureAttr(VtValue(mayaLightExposure), true);

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadLightExposure(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadLightExposure(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    MStatus status;
    MPlug   lightExposurePlug = depFn.findPlug(_tokens->ExposurePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightExposure = 0.0f;
    lightSchema.GetExposureAttr().Get(&lightExposure);

    status = lightExposurePlug.setValue(lightExposure);

    return (status == MS::kSuccess);
}

// DIFFUSE

#if PXR_VERSION < 2111
static bool _WriteLightDiffuse(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteLightDiffuse(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    MStatus     status;
    const MPlug lightDiffusePlug
        = depFn.findPlug(_tokens->DiffuseAmountPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float mayaLightDiffuseAmount = 1.0f;
    status = lightDiffusePlug.getValue(mayaLightDiffuseAmount);
    if (status != MS::kSuccess) {
        return false;
    }

    lightSchema.CreateDiffuseAttr(VtValue(mayaLightDiffuseAmount), true);

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadLightDiffuse(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadLightDiffuse(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    MStatus status;
    MPlug   lightDiffusePlug = depFn.findPlug(_tokens->DiffuseAmountPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightDiffuseAmount = 1.0f;
    lightSchema.GetDiffuseAttr().Get(&lightDiffuseAmount);

    status = lightDiffusePlug.setValue(lightDiffuseAmount);

    return (status == MS::kSuccess);
}

// SPECULAR

#if PXR_VERSION < 2111
static bool _WriteLightSpecular(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteLightSpecular(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    MStatus     status;
    const MPlug lightSpecularPlug
        = depFn.findPlug(_tokens->SpecularAmountPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float mayaLightSpecularAmount = 1.0f;
    status = lightSpecularPlug.getValue(mayaLightSpecularAmount);
    if (status != MS::kSuccess) {
        return false;
    }

    lightSchema.CreateSpecularAttr(VtValue(mayaLightSpecularAmount), true);

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadLightSpecular(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadLightSpecular(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    MStatus status;
    MPlug   lightSpecularPlug = depFn.findPlug(_tokens->SpecularAmountPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightSpecularAmount = 1.0f;
    lightSchema.GetSpecularAttr().Get(&lightSpecularAmount);

    status = lightSpecularPlug.setValue(lightSpecularAmount);

    return (status == MS::kSuccess);
}

// NORMALIZE POWER

#if PXR_VERSION < 2111
static bool _WriteLightNormalizePower(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteLightNormalizePower(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    MStatus     status;
    const MPlug lightNormalizePowerPlug
        = depFn.findPlug(_tokens->NormalizePowerPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool mayaLightNormalizePower = false;
    status = lightNormalizePowerPlug.getValue(mayaLightNormalizePower);
    if (status != MS::kSuccess) {
        return false;
    }

    lightSchema.CreateNormalizeAttr(VtValue(mayaLightNormalizePower), true);

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadLightNormalizePower(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadLightNormalizePower(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    MStatus status;
    MPlug   lightNormalizePowerPlug
        = depFn.findPlug(_tokens->NormalizePowerPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightNormalizePower = false;
    lightSchema.GetNormalizeAttr().Get(&lightNormalizePower);

    status = lightNormalizePowerPlug.setValue(lightNormalizePower);

    return (status == MS::kSuccess);
}

// COLOR

#if PXR_VERSION < 2111
static bool _WriteLightColor(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteLightColor(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    MStatus     status;
    const MPlug lightColorPlug = depFn.findPlug(_tokens->ColorPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    const GfVec3f lightColor = GfConvertDisplayToLinear(GfVec3f(
        lightColorPlug.child(0).asFloat(),
        lightColorPlug.child(1).asFloat(),
        lightColorPlug.child(2).asFloat()));
    lightSchema.CreateColorAttr(VtValue(lightColor), true);

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadLightColor(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadLightColor(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    MStatus status;
    MPlug   lightColorPlug = depFn.findPlug(_tokens->ColorPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    GfVec3f lightColor(1.0f);
    lightSchema.GetColorAttr().Get(&lightColor);
    lightColor = GfConvertLinearToDisplay(lightColor);

    status = lightColorPlug.child(0).setValue(lightColor[0]);
    status = lightColorPlug.child(1).setValue(lightColor[1]);
    status = lightColorPlug.child(2).setValue(lightColor[2]);

    return (status == MS::kSuccess);
}

// TEMPERATURE

#if PXR_VERSION < 2111
static bool _WriteLightTemperature(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteLightTemperature(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    MStatus     status;
    const MPlug lightEnableTemperaturePlug
        = depFn.findPlug(_tokens->EnableTemperaturePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool mayaLightEnableTemperature = false;
    status = lightEnableTemperaturePlug.getValue(mayaLightEnableTemperature);
    if (status != MS::kSuccess) {
        return false;
    }

    const MPlug lightTemperaturePlug
        = depFn.findPlug(_tokens->TemperaturePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float mayaLightTemperature = 6500.0f;
    status = lightTemperaturePlug.getValue(mayaLightTemperature);
    if (status != MS::kSuccess) {
        return false;
    }

    lightSchema.CreateEnableColorTemperatureAttr(VtValue(mayaLightEnableTemperature), true);
    lightSchema.CreateColorTemperatureAttr(VtValue(mayaLightTemperature), true);

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadLightTemperature(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadLightTemperature(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    MStatus status;
    MPlug   lightEnableTemperaturePlug
        = depFn.findPlug(_tokens->EnableTemperaturePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    MPlug lightTemperaturePlug = depFn.findPlug(_tokens->TemperaturePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightEnableTemperature = false;
    lightSchema.GetEnableColorTemperatureAttr().Get(&lightEnableTemperature);

    status = lightEnableTemperaturePlug.setValue(lightEnableTemperature);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightTemperature = 6500.0f;
    lightSchema.GetColorTemperatureAttr().Get(&lightTemperature);

    status = lightTemperaturePlug.setValue(lightTemperature);
    if (status != MS::kSuccess) {
        return false;
    }

    return true;
}

// DISTANT LIGHT ANGLE

#if PXR_VERSION < 2111
static bool _WriteDistantLightAngle(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteDistantLightAngle(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    UsdLuxDistantLight distantLightSchema(lightSchema);
    if (!distantLightSchema) {
        return false;
    }

    MStatus     status;
    const MPlug lightAnglePlug
        = depFn.findPlug(_tokens->DistantLightAnglePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float mayaLightAngle = 0.53f;
    status = lightAnglePlug.getValue(mayaLightAngle);
    if (status != MS::kSuccess) {
        return false;
    }

    distantLightSchema.CreateAngleAttr(VtValue(mayaLightAngle), true);

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadDistantLightAngle(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadDistantLightAngle(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    const UsdLuxDistantLight distantLightSchema(lightSchema);
    if (!distantLightSchema) {
        return false;
    }

    MStatus status;
    MPlug   lightAnglePlug = depFn.findPlug(_tokens->DistantLightAnglePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightAngle = 0.53f;
    distantLightSchema.GetAngleAttr().Get(&lightAngle);

    status = lightAnglePlug.setValue(lightAngle);

    return (status == MS::kSuccess);
}

// LIGHT TEXTURE FILE

#if PXR_VERSION < 2111
static bool _WriteLightTextureFile(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteLightTextureFile(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    UsdLuxRectLight rectLightSchema(lightSchema);
    UsdLuxDomeLight domeLightSchema(lightSchema);
    if (!rectLightSchema && !domeLightSchema) {
        return false;
    }

    MStatus     status;
    const MPlug lightTextureFilePlug
        = depFn.findPlug(_tokens->TextureFilePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    MString mayaLightTextureFile;
    status = lightTextureFilePlug.getValue(mayaLightTextureFile);
    if (status != MS::kSuccess) {
        return false;
    }

    if (mayaLightTextureFile.numChars() < 1u) {
        return false;
    }

    const SdfAssetPath lightTextureAssetPath(mayaLightTextureFile.asChar());
    if (rectLightSchema) {
        rectLightSchema.CreateTextureFileAttr(VtValue(lightTextureAssetPath), true);
    } else if (domeLightSchema) {
        domeLightSchema.CreateTextureFileAttr(VtValue(lightTextureAssetPath), true);
    }

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadLightTextureFile(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadLightTextureFile(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    const UsdLuxRectLight rectLightSchema(lightSchema);
    const UsdLuxDomeLight domeLightSchema(lightSchema);
    if (!rectLightSchema && !domeLightSchema) {
        return false;
    }

    MStatus status;
    MPlug   lightTextureFilePlug = depFn.findPlug(_tokens->TextureFilePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    SdfAssetPath lightTextureAssetPath;
    if (rectLightSchema) {
        rectLightSchema.GetTextureFileAttr().Get(&lightTextureAssetPath);
    } else if (domeLightSchema) {
        domeLightSchema.GetTextureFileAttr().Get(&lightTextureAssetPath);
    }
    const std::string lightTextureFile = lightTextureAssetPath.GetAssetPath();

    status = lightTextureFilePlug.setValue(MString(lightTextureFile.c_str()));

    return (status == MS::kSuccess);
}

// AOV LIGHT
#if PXR_VERSION < 2111
static bool _WriteAovLight(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteAovLight(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    // Early out
    UsdPrim              lightPrim = lightSchema.GetPrim();
    static const TfType& usdSchemaBase = TfType::FindByName(_tokens->UsdSchemaBase);
    static const TfType& pxrAovLightType
        = usdSchemaBase.FindDerivedByName(_tokens->AovLightMayaTypeName);

    const TfType& lightType = usdSchemaBase.FindDerivedByName(lightPrim.GetTypeName());
    if (!lightType.IsA(pxrAovLightType)) {
        return false;
    }

    MStatus status;

    // AOV Name.
    MPlug aovNamePlug = depFn.findPlug(_tokens->AovNamePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(aovNamePlug)) {
        MString mayaAovName;
        status = aovNamePlug.getValue(mayaAovName);
        if (status != MS::kSuccess) {
            return false;
        }
        _SetLightPrimAttr(
            lightPrim,
            _tokens->AovNamePlugName,
            SdfValueTypeNames->String,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaAovName.asChar()),
            true);
    }

    // In Primary Hit.
    MPlug inPrimaryHitPlug = depFn.findPlug(_tokens->InPrimaryHitPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(inPrimaryHitPlug)) {
        bool mayaInPrimaryHit = true;
        status = inPrimaryHitPlug.getValue(mayaInPrimaryHit);
        if (status != MS::kSuccess) {
            return false;
        }
        _SetLightPrimAttr(
            lightPrim,
            _tokens->InPrimaryHitPlugName,
            SdfValueTypeNames->Bool,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaInPrimaryHit),
            true);
    }

    // In Reflection.
    MPlug inReflectionPlug = depFn.findPlug(_tokens->InReflectionPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(inReflectionPlug)) {
        bool mayaInReflection = true;
        status = inReflectionPlug.getValue(mayaInReflection);
        if (status != MS::kSuccess) {
            return false;
        }
        _SetLightPrimAttr(
            lightPrim,
            _tokens->InReflectionPlugName,
            SdfValueTypeNames->Bool,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaInReflection),
            true);
    }

    // In Refraction.
    MPlug inRefractionPlug = depFn.findPlug(_tokens->InRefractionPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(inRefractionPlug)) {
        bool mayaInRefraction = true;
        status = inRefractionPlug.getValue(mayaInRefraction);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->InRefractionPlugName,
            SdfValueTypeNames->Bool,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaInRefraction),
            true);
    }

    // Invert.
    MPlug invertPlug = depFn.findPlug(_tokens->InvertPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(invertPlug)) {
        bool mayaInvert = true;
        status = invertPlug.getValue(mayaInvert);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->InvertPlugName,
            SdfValueTypeNames->Bool,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaInvert),
            true);
    }

    // On Volume Boundaries.
    MPlug onVolumeBoundariesPlug
        = depFn.findPlug(_tokens->OnVolumeBoundariesPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(onVolumeBoundariesPlug)) {
        bool mayaOnVolumeBoundaries = true;
        status = onVolumeBoundariesPlug.getValue(mayaOnVolumeBoundaries);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->OnVolumeBoundariesPlugName,
            SdfValueTypeNames->Bool,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaOnVolumeBoundaries),
            true);
    }

    // Use Color.
    MPlug useColorPlug = depFn.findPlug(_tokens->UseColorPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(useColorPlug)) {
        bool mayaUseColor = true;
        status = useColorPlug.getValue(mayaUseColor);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->UseColorPlugName,
            SdfValueTypeNames->Bool,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaUseColor),
            true);
    }

    // Use Throughput.
    MPlug useThroughputPlug = depFn.findPlug(_tokens->UseThroughputPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(useThroughputPlug)) {
        bool mayaUseThroughput = true;
        status = useThroughputPlug.getValue(mayaUseThroughput);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->UseThroughputPlugName,
            SdfValueTypeNames->Bool,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaUseThroughput),
            true);
    }

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadAovLight(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadAovLight(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    // Early out
    const UsdPrim&       lightPrim = lightSchema.GetPrim();
    static const TfType& usdSchemaBase = TfType::FindByName(_tokens->UsdSchemaBase);
    static const TfType& pxrAovLightType
        = usdSchemaBase.FindDerivedByName(_tokens->AovLightMayaTypeName);

    const TfType& lightType = usdSchemaBase.FindDerivedByName(lightPrim.GetTypeName());
    if (!lightType.IsA(pxrAovLightType)) {
        return false;
    }

    MStatus status;

    // AOV Name.
    MPlug lightAovNamePlug = depFn.findPlug(_tokens->AovNamePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    std::string lightAovName;
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->AovNamePlugName)))
        .Get(&lightAovName);
    status = lightAovNamePlug.setValue(MString(lightAovName.c_str()));
    if (status != MS::kSuccess) {
        return false;
    }

    // In Primary Hit.
    MPlug lightInPrimaryHitPlug = depFn.findPlug(_tokens->InPrimaryHitPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    bool lightInPrimaryHit = true;
    lightPrim
        .GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->InPrimaryHitPlugName)))
        .Get(&lightInPrimaryHit);
    status = lightInPrimaryHitPlug.setValue(lightInPrimaryHit);
    if (status != MS::kSuccess) {
        return false;
    }

    // In Reflection.
    MPlug lightInReflectionPlug = depFn.findPlug(_tokens->InReflectionPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    bool lightInReflection = true;
    lightPrim
        .GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->InReflectionPlugName)))
        .Get(&lightInReflection);
    status = lightInReflectionPlug.setValue(lightInReflection);
    if (status != MS::kSuccess) {
        return false;
    }

    // In Refraction.
    MPlug lightInRefractionPlug = depFn.findPlug(_tokens->InRefractionPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    bool lightInRefraction = true;
    lightPrim
        .GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->InRefractionPlugName)))
        .Get(&lightInRefraction);
    status = lightInRefractionPlug.setValue(lightInRefraction);
    if (status != MS::kSuccess) {
        return false;
    }

    // Invert.
    MPlug lightInvertPlug = depFn.findPlug(_tokens->InvertPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    bool lightInvert = true;
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->InvertPlugName)))
        .Get(&lightInvert);
    status = lightInvertPlug.setValue(lightInvert);
    if (status != MS::kSuccess) {
        return false;
    }

    // On Volume Boundaries.
    MPlug lightOnVolumeBoundariesPlug
        = depFn.findPlug(_tokens->OnVolumeBoundariesPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    bool lightOnVolumeBoundaries = true;
    lightPrim
        .GetAttribute(
            _ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->OnVolumeBoundariesPlugName)))
        .Get(&lightOnVolumeBoundaries);
    status = lightOnVolumeBoundariesPlug.setValue(lightOnVolumeBoundaries);
    if (status != MS::kSuccess) {
        return false;
    }

    // Use Color.
    MPlug lightUseColorPlug = depFn.findPlug(_tokens->UseColorPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    bool lightUseColor = true;
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->UseColorPlugName)))
        .Get(&lightUseColor);
    status = lightUseColorPlug.setValue(lightUseColor);
    if (status != MS::kSuccess) {
        return false;
    }

    // Use Throughput.
    MPlug lightUseThroughputPlug
        = depFn.findPlug(_tokens->UseThroughputPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    bool lightUseThroughput = true;
    lightPrim
        .GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->UseThroughputPlugName)))
        .Get(&lightUseThroughput);
    status = lightUseThroughputPlug.setValue(lightUseThroughput);
    return status == MS::kSuccess;
}

// ENVDAY LIGHT
#if PXR_VERSION < 2111
static bool _WriteEnvDayLight(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteEnvDayLight(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    // Early out
    UsdPrim              lightPrim = lightSchema.GetPrim();
    static const TfType& usdSchemaBase = TfType::FindByName(_tokens->UsdSchemaBase);
    static const TfType& pxrEnvDayLightType
        = usdSchemaBase.FindDerivedByName(_tokens->EnvDayLightMayaTypeName);

    const TfType& lightType = usdSchemaBase.FindDerivedByName(lightPrim.GetTypeName());
    if (!lightType.IsA(pxrEnvDayLightType)) {
        return false;
    }

    MStatus status;

    // Day.
    MPlug dayPlug = depFn.findPlug(_tokens->DayPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(dayPlug)) {
        int mayaDay;
        status = dayPlug.getValue(mayaDay);
        if (status != MS::kSuccess) {
            return false;
        }
        _SetLightPrimAttr(
            lightPrim,
            _tokens->DayPlugName,
            SdfValueTypeNames->Int,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaDay),
            true);
    }

    // Haziness.
    MPlug hazinessPlug = depFn.findPlug(_tokens->HazinessPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(hazinessPlug)) {
        float mayaHaziness;
        status = hazinessPlug.getValue(mayaHaziness);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->HazinessPlugName,
            SdfValueTypeNames->Float,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaHaziness),
            true);
    }

    // Hour.
    MPlug hourPlug = depFn.findPlug(_tokens->HourPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(hourPlug)) {
        float mayaHour;
        status = hourPlug.getValue(mayaHour);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->HourPlugName,
            SdfValueTypeNames->Float,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaHour),
            true);
    }

    // Latitude.
    MPlug latitudePlug = depFn.findPlug(_tokens->LatitudePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(latitudePlug)) {
        float mayaLatitude;
        status = latitudePlug.getValue(mayaLatitude);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->LatitudePlugName,
            SdfValueTypeNames->Float,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaLatitude),
            true);
    }

    // Longitude.
    MPlug longitudePlug = depFn.findPlug(_tokens->LongitudePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(longitudePlug)) {
        float mayaLongitude;
        status = longitudePlug.getValue(mayaLongitude);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->LongitudePlugName,
            SdfValueTypeNames->Float,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaLongitude),
            true);
    }

    // Month.
    MPlug monthPlug = depFn.findPlug(_tokens->MonthPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(monthPlug)) {
        int mayaMonth;
        status = monthPlug.getValue(mayaMonth);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->MonthPlugName,
            SdfValueTypeNames->Int,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaMonth),
            true);
    }

    // Sky tint.
    MPlug skyTintPlug = depFn.findPlug(_tokens->SkyTintPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(skyTintPlug)) {
        const GfVec3f mayaSkyTint = GfConvertDisplayToLinear(GfVec3f(
            skyTintPlug.child(0).asFloat(),
            skyTintPlug.child(1).asFloat(),
            skyTintPlug.child(2).asFloat()));

        _SetLightPrimAttr(
            lightPrim,
            _tokens->SkyTintPlugName,
            SdfValueTypeNames->Color3f,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaSkyTint),
            true);
    }

    // Sun direction.
    MPlug sunDirectionPlug = depFn.findPlug(_tokens->SunDirectionPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(sunDirectionPlug)) {
        const GfVec3f mayaSunDirection(
            sunDirectionPlug.child(0).asFloat(),
            sunDirectionPlug.child(1).asFloat(),
            sunDirectionPlug.child(2).asFloat());

        _SetLightPrimAttr(
            lightPrim,
            _tokens->SunDirectionPlugName,
            SdfValueTypeNames->Vector3f,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaSunDirection),
            true);
    }

    // Sun size.
    MPlug sunSizePlug = depFn.findPlug(_tokens->SunSizePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(sunSizePlug)) {
        float mayaSunSize;
        status = sunSizePlug.getValue(mayaSunSize);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->SunSizePlugName,
            SdfValueTypeNames->Float,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaSunSize),
            true);
    }

    // Sun tint.
    MPlug sunTintPlug = depFn.findPlug(_tokens->SunTintPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(sunTintPlug)) {
        const GfVec3f mayaSunTint = GfConvertDisplayToLinear(GfVec3f(
            sunTintPlug.child(0).asFloat(),
            sunTintPlug.child(1).asFloat(),
            sunTintPlug.child(2).asFloat()));

        _SetLightPrimAttr(
            lightPrim,
            _tokens->SunTintPlugName,
            SdfValueTypeNames->Color3f,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaSunTint),
            true);
    }

    // Year.
    MPlug yearPlug = depFn.findPlug(_tokens->YearPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(yearPlug)) {
        int mayaYear;
        status = yearPlug.getValue(mayaYear);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->YearPlugName,
            SdfValueTypeNames->Int,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaYear),
            true);
    }

    // Zone.
    MPlug zonePlug = depFn.findPlug(_tokens->ZonePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(zonePlug)) {
        float mayaZone;
        status = zonePlug.getValue(mayaZone);
        if (status != MS::kSuccess) {
            return false;
        }

        _SetLightPrimAttr(
            lightPrim,
            _tokens->ZonePlugName,
            SdfValueTypeNames->Float,
            /* custom */ false,
            SdfVariabilityVarying,
            VtValue(mayaZone),
            true);
    }

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadEnvDayLight(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadEnvDayLight(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    const UsdPrim&       lightPrim = lightSchema.GetPrim();
    static const TfType& usdSchemaBase = TfType::FindByName(_tokens->UsdSchemaBase);
    static const TfType& pxrEnvDayLightType
        = usdSchemaBase.FindDerivedByName(_tokens->EnvDayLightMayaTypeName);

    const TfType& lightType = usdSchemaBase.FindDerivedByName(lightPrim.GetTypeName());
    if (!lightType.IsA(pxrEnvDayLightType)) {
        return false;
    }

    MStatus status;

    // Day.
    MPlug lightDayPlug = depFn.findPlug(_tokens->DayPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    int lightDay = 1;
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->DayPlugName)))
        .Get(&lightDay);
    status = lightDayPlug.setValue(lightDay);
    if (status != MS::kSuccess) {
        return false;
    }

    // Haziness.
    MPlug lightHazinessPlug = depFn.findPlug(_tokens->HazinessPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    float lightHaziness = 2.0f;
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->HazinessPlugName)))
        .Get(&lightHaziness);
    status = lightHazinessPlug.setValue(lightHaziness);
    if (status != MS::kSuccess) {
        return false;
    }

    // Hour.
    MPlug lightHourPlug = depFn.findPlug(_tokens->HourPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    float lightHour = 14.633333f;
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->HourPlugName)))
        .Get(&lightHour);
    status = lightHourPlug.setValue(lightHour);
    if (status != MS::kSuccess) {
        return false;
    }

    // Latitude.
    MPlug lightLatitudePlug = depFn.findPlug(_tokens->LatitudePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    float lightLatitude = 47.602f;
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->LatitudePlugName)))
        .Get(&lightLatitude);
    status = lightLatitudePlug.setValue(lightLatitude);
    if (status != MS::kSuccess) {
        return false;
    }

    // Longitude.
    MPlug lightLongitudePlug = depFn.findPlug(_tokens->LongitudePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    float lightLongitude = -122.332f;
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->LongitudePlugName)))
        .Get(&lightLongitude);
    status = lightLongitudePlug.setValue(lightLongitude);
    if (status != MS::kSuccess) {
        return false;
    }

    // Month.
    MPlug lightMonthPlug = depFn.findPlug(_tokens->MonthPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    int lightMonth = 0;
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->MonthPlugName)))
        .Get(&lightMonth);
    status = lightMonthPlug.setValue(lightMonth);
    if (status != MS::kSuccess) {
        return false;
    }

    // Sky tint.
    MPlug lightSkyTintPlug = depFn.findPlug(_tokens->SkyTintPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    GfVec3f lightSkyTint(1.0f);
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->SkyTintPlugName)))
        .Get(&lightSkyTint);
    lightSkyTint = GfConvertLinearToDisplay(lightSkyTint);
    status = lightSkyTintPlug.child(0).setValue(lightSkyTint[0]);
    status = lightSkyTintPlug.child(1).setValue(lightSkyTint[1]);
    status = lightSkyTintPlug.child(2).setValue(lightSkyTint[2]);
    if (status != MS::kSuccess) {
        return false;
    }

    // Sun direction.
    MPlug lightSunDirectionPlug = depFn.findPlug(_tokens->SunDirectionPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    GfVec3f lightSunDirection(0.0f, 0.0f, 1.0f);
    lightPrim
        .GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->SunDirectionPlugName)))
        .Get(&lightSunDirection);
    status = lightSunDirectionPlug.child(0).setValue(lightSunDirection[0]);
    status = lightSunDirectionPlug.child(1).setValue(lightSunDirection[1]);
    status = lightSunDirectionPlug.child(2).setValue(lightSunDirection[2]);
    if (status != MS::kSuccess) {
        return false;
    }

    // Sun size.
    MPlug lightSunSizePlug = depFn.findPlug(_tokens->SunSizePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    float lightSunSize = 1.0f;
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->SunSizePlugName)))
        .Get(&lightSunSize);
    status = lightSunSizePlug.setValue(lightSunSize);
    if (status != MS::kSuccess) {
        return false;
    }

    // Sun tint.
    MPlug lightSunTintPlug = depFn.findPlug(_tokens->SunTintPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    GfVec3f lightSunTint(1.0f);
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->SunTintPlugName)))
        .Get(&lightSunTint);
    lightSunTint = GfConvertLinearToDisplay(lightSunTint);
    status = lightSunTintPlug.child(0).setValue(lightSunTint[0]);
    status = lightSunTintPlug.child(1).setValue(lightSunTint[1]);
    status = lightSunTintPlug.child(2).setValue(lightSunTint[2]);
    if (status != MS::kSuccess) {
        return false;
    }

    // Year.
    MPlug lightYearPlug = depFn.findPlug(_tokens->YearPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    int lightYear = 2015;
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->YearPlugName)))
        .Get(&lightYear);
    status = lightYearPlug.setValue(lightYear);
    if (status != MS::kSuccess) {
        return false;
    }

    // Zone.
    MPlug lightZonePlug = depFn.findPlug(_tokens->ZonePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }
    float lightZone = -8.0f;
    lightPrim.GetAttribute(_ShaderAttrName(_PrefixRiLightAttrNamespace(_tokens->ZonePlugName)))
        .Get(&lightZone);
    status = lightZonePlug.setValue(lightZone);
    return status == MS::kSuccess;
}

// SHAPING API

#if PXR_VERSION < 2111
static bool _WriteLightShapingAPI(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteLightShapingAPI(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    UsdLuxShapingAPI shapingAPI
        = UsdMayaTranslatorUtil::GetAPISchemaForAuthoring<UsdLuxShapingAPI>(lightSchema.GetPrim());
    if (!shapingAPI) {
        return false;
    }

    MStatus status;

    // Focus.
    MPlug lightFocusPlug = depFn.findPlug(_tokens->FocusPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(lightFocusPlug)) {
        float mayaLightFocus = 0.0f;
        status = lightFocusPlug.getValue(mayaLightFocus);
        if (status != MS::kSuccess) {
            return false;
        }

        shapingAPI.CreateShapingFocusAttr(VtValue(mayaLightFocus), true);
    }

    // Focus Tint.
    MPlug lightFocusTintPlug = depFn.findPlug(_tokens->FocusTintPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(lightFocusTintPlug)) {
        const GfVec3f lightFocusTint = GfConvertDisplayToLinear(GfVec3f(
            lightFocusTintPlug.child(0).asFloat(),
            lightFocusTintPlug.child(1).asFloat(),
            lightFocusTintPlug.child(2).asFloat()));

        shapingAPI.CreateShapingFocusTintAttr(VtValue(lightFocusTint), true);
    }

    // Cone Angle.
    MPlug lightConeAnglePlug = depFn.findPlug(_tokens->ConeAnglePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(lightConeAnglePlug)) {
        float mayaLightConeAngle = 90.0f;
        status = lightConeAnglePlug.getValue(mayaLightConeAngle);
        if (status != MS::kSuccess) {
            return false;
        }

        shapingAPI.CreateShapingConeAngleAttr(VtValue(mayaLightConeAngle), true);
    }

    // Cone Softness.
    MPlug lightConeSoftnessPlug = depFn.findPlug(_tokens->ConeSoftnessPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(lightConeSoftnessPlug)) {
        float mayaLightConeSoftness = 0.0f;
        status = lightConeSoftnessPlug.getValue(mayaLightConeSoftness);
        if (status != MS::kSuccess) {
            return false;
        }

        shapingAPI.CreateShapingConeSoftnessAttr(VtValue(mayaLightConeSoftness), true);
    }

    // Profile File.
    MPlug lightProfileFilePlug = depFn.findPlug(_tokens->ProfileFilePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(lightProfileFilePlug)) {
        MString mayaLightProfileFile;
        status = lightProfileFilePlug.getValue(mayaLightProfileFile);
        if (status != MS::kSuccess) {
            return false;
        }

        if (mayaLightProfileFile.numChars() > 0u) {
            const SdfAssetPath lightProfileAssetPath(mayaLightProfileFile.asChar());
            shapingAPI.CreateShapingIesFileAttr(VtValue(lightProfileAssetPath), true);
        }
    }

    // Profile Scale.
    MPlug lightProfileScalePlug = depFn.findPlug(_tokens->ProfileScalePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(lightProfileScalePlug)) {
        float mayaLightProfileScale = 1.0f;
        status = lightProfileScalePlug.getValue(mayaLightProfileScale);
        if (status != MS::kSuccess) {
            return false;
        }

        shapingAPI.CreateShapingIesAngleScaleAttr(VtValue(mayaLightProfileScale), true);
    }

    // Profile Normalize.
    MPlug lightProfileNormalizePlug
        = depFn.findPlug(_tokens->ProfileNormalizePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(lightProfileNormalizePlug)) {
        bool mayaLightProfileNormalize = false;
        status = lightProfileNormalizePlug.getValue(mayaLightProfileNormalize);
        if (status != MS::kSuccess) {
            return false;
        }

        shapingAPI.CreateShapingIesNormalizeAttr(VtValue(mayaLightProfileNormalize), true);
    }

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadLightShapingAPI(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadLightShapingAPI(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    const UsdLuxShapingAPI shapingAPI(lightSchema);
    if (!shapingAPI) {
        return false;
    }

    MStatus status;

    // Focus.
    MPlug lightFocusPlug = depFn.findPlug(_tokens->FocusPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightFocus = 0.0f;
    shapingAPI.GetShapingFocusAttr().Get(&lightFocus);

    status = lightFocusPlug.setValue(lightFocus);
    if (status != MS::kSuccess) {
        return false;
    }

    // Focus Tint.
    MPlug lightFocusTintPlug = depFn.findPlug(_tokens->FocusTintPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    GfVec3f lightFocusTint(0.0f);
    shapingAPI.GetShapingFocusTintAttr().Get(&lightFocusTint);
    lightFocusTint = GfConvertLinearToDisplay(lightFocusTint);

    status = lightFocusTintPlug.child(0).setValue(lightFocusTint[0]);
    status = lightFocusTintPlug.child(1).setValue(lightFocusTint[1]);
    status = lightFocusTintPlug.child(2).setValue(lightFocusTint[2]);
    if (status != MS::kSuccess) {
        return false;
    }

    // Cone Angle.
    MPlug lightConeAnglePlug = depFn.findPlug(_tokens->ConeAnglePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightConeAngle = 90.0f;
    shapingAPI.GetShapingConeAngleAttr().Get(&lightConeAngle);

    status = lightConeAnglePlug.setValue(lightConeAngle);
    if (status != MS::kSuccess) {
        return false;
    }

    // Cone Softness.
    MPlug lightConeSoftnessPlug = depFn.findPlug(_tokens->ConeSoftnessPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightConeSoftness = 0.0f;
    shapingAPI.GetShapingConeSoftnessAttr().Get(&lightConeSoftness);

    status = lightConeSoftnessPlug.setValue(lightConeSoftness);
    if (status != MS::kSuccess) {
        return false;
    }

    // Profile File.
    MPlug lightProfileFilePlug = depFn.findPlug(_tokens->ProfileFilePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    SdfAssetPath lightProfileAssetPath;
    shapingAPI.GetShapingIesFileAttr().Get(&lightProfileAssetPath);
    const std::string lightProfileFile = lightProfileAssetPath.GetAssetPath();

    status = lightProfileFilePlug.setValue(MString(lightProfileFile.c_str()));
    if (status != MS::kSuccess) {
        return false;
    }

    // Profile Scale.
    MPlug lightProfileScalePlug = depFn.findPlug(_tokens->ProfileScalePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightProfileScale = 1.0f;
    shapingAPI.GetShapingIesAngleScaleAttr().Get(&lightProfileScale);

    status = lightProfileScalePlug.setValue(lightProfileScale);

    // Profile Normalize.
    MPlug lightProfileNormalizePlug
        = depFn.findPlug(_tokens->ProfileNormalizePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightProfileNormalize = false;
    shapingAPI.GetShapingIesNormalizeAttr().Get(&lightProfileNormalize);

    status = lightProfileNormalizePlug.setValue(lightProfileNormalize);
    return status == MS::kSuccess;
}

// SHADOW API

#if PXR_VERSION < 2111
static bool _WriteLightShadowAPI(const MFnDependencyNode& depFn, UsdLuxLight& lightSchema)
#else
static bool _WriteLightShadowAPI(const MFnDependencyNode& depFn, UsdLuxLightAPI& lightSchema)
#endif
{
    UsdLuxShadowAPI shadowAPI
        = UsdMayaTranslatorUtil::GetAPISchemaForAuthoring<UsdLuxShadowAPI>(lightSchema.GetPrim());
    if (!shadowAPI) {
        return false;
    }

    MStatus status;

    // Enable Shadows.
    MPlug lightEnableShadowsPlug
        = depFn.findPlug(_tokens->EnableShadowsPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(lightEnableShadowsPlug)) {
        bool mayaLightEnableShadows = true;
        status = lightEnableShadowsPlug.getValue(mayaLightEnableShadows);
        if (status != MS::kSuccess) {
            return false;
        }

        shadowAPI.CreateShadowEnableAttr(VtValue(mayaLightEnableShadows), true);
    }

    // Shadow Include.
    // XXX: Not yet implemented.

    // Shadow Exclude.
    // XXX: Not yet implemented.

    // Shadow Color.
    MPlug lightShadowColorPlug = depFn.findPlug(_tokens->ShadowColorPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(lightShadowColorPlug)) {
        const GfVec3f lightShadowColor = GfConvertDisplayToLinear(GfVec3f(
            lightShadowColorPlug.child(0).asFloat(),
            lightShadowColorPlug.child(1).asFloat(),
            lightShadowColorPlug.child(2).asFloat()));

        shadowAPI.CreateShadowColorAttr(VtValue(lightShadowColor), true);
    }

    // Shadow Distance.
    MPlug lightShadowDistancePlug
        = depFn.findPlug(_tokens->ShadowDistancePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(lightShadowDistancePlug)) {
        float mayaLightShadowDistance = 0.0f;
        status = lightShadowDistancePlug.getValue(mayaLightShadowDistance);
        if (status != MS::kSuccess) {
            return false;
        }

        shadowAPI.CreateShadowDistanceAttr(VtValue(mayaLightShadowDistance), true);
    }

    // Shadow Falloff.
    MPlug lightShadowFalloffPlug
        = depFn.findPlug(_tokens->ShadowFalloffPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(lightShadowFalloffPlug)) {
        float mayaLightShadowFalloff = 0.0f;
        status = lightShadowFalloffPlug.getValue(mayaLightShadowFalloff);
        if (status != MS::kSuccess) {
            return false;
        }

        shadowAPI.CreateShadowFalloffAttr(VtValue(mayaLightShadowFalloff), true);
    }

    // Shadow Falloff Gamma.
    MPlug lightShadowFalloffGammaPlug
        = depFn.findPlug(_tokens->ShadowFalloffGammaPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(lightShadowFalloffGammaPlug)) {
        float mayaLightShadowFalloffGamma = 1.0f;
        status = lightShadowFalloffGammaPlug.getValue(mayaLightShadowFalloffGamma);
        if (status != MS::kSuccess) {
            return false;
        }

        shadowAPI.CreateShadowFalloffGammaAttr(VtValue(mayaLightShadowFalloffGamma), true);
    }

    return true;
}

#if PXR_VERSION < 2111
static bool _ReadLightShadowAPI(const UsdLuxLight& lightSchema, MFnDependencyNode& depFn)
#else
static bool _ReadLightShadowAPI(const UsdLuxLightAPI& lightSchema, MFnDependencyNode& depFn)
#endif
{
    const UsdLuxShadowAPI shadowAPI(lightSchema);
    if (!shadowAPI) {
        return false;
    }

    MStatus status;

    // Enable Shadows.
    MPlug lightEnableShadowsPlug
        = depFn.findPlug(_tokens->EnableShadowsPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    bool lightEnableShadows = true;
    shadowAPI.GetShadowEnableAttr().Get(&lightEnableShadows);

    status = lightEnableShadowsPlug.setValue(lightEnableShadows);
    if (status != MS::kSuccess) {
        return false;
    }

    // Shadow Include.
    // XXX: Not yet implemented.

    // Shadow Exclude.
    // XXX: Not yet implemented.

    // Shadow Color.
    MPlug lightShadowColorPlug = depFn.findPlug(_tokens->ShadowColorPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    GfVec3f lightShadowColor(0.0f);
    shadowAPI.GetShadowColorAttr().Get(&lightShadowColor);
    lightShadowColor = GfConvertLinearToDisplay(lightShadowColor);

    status = lightShadowColorPlug.child(0).setValue(lightShadowColor[0]);
    status = lightShadowColorPlug.child(1).setValue(lightShadowColor[1]);
    status = lightShadowColorPlug.child(2).setValue(lightShadowColor[2]);
    if (status != MS::kSuccess) {
        return false;
    }

    // Shadow Distance.
    MPlug lightShadowDistancePlug
        = depFn.findPlug(_tokens->ShadowDistancePlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightShadowDistance = 0.0f;
    shadowAPI.GetShadowDistanceAttr().Get(&lightShadowDistance);

    status = lightShadowDistancePlug.setValue(lightShadowDistance);
    if (status != MS::kSuccess) {
        return false;
    }

    // Shadow Falloff.
    MPlug lightShadowFalloffPlug
        = depFn.findPlug(_tokens->ShadowFalloffPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightShadowFalloff = 0.0f;
    shadowAPI.GetShadowFalloffAttr().Get(&lightShadowFalloff);

    status = lightShadowFalloffPlug.setValue(lightShadowFalloff);
    if (status != MS::kSuccess) {
        return false;
    }

    // Shadow Falloff Gamma.
    MPlug lightShadowFalloffGammaPlug
        = depFn.findPlug(_tokens->ShadowFalloffGammaPlugName.GetText(), &status);
    if (status != MS::kSuccess) {
        return false;
    }

    float lightShadowFalloffGamma = 1.0f;
    shadowAPI.GetShadowFalloffGammaAttr().Get(&lightShadowFalloffGamma);

    status = lightShadowFalloffGammaPlug.setValue(lightShadowFalloffGamma);
    return status == MS::kSuccess;
}

void _ReadShaderAttributesFromUsdPrim_deprecated(
#if PXR_VERSION < 2111
    const UsdLuxLight lightSchema,
#else
    const UsdLuxLightAPI lightSchema,
#endif
    MFnDependencyNode& depFn)
{
    _ReadLightIntensity(lightSchema, depFn);
    _ReadLightExposure(lightSchema, depFn);
    _ReadLightDiffuse(lightSchema, depFn);
    _ReadLightSpecular(lightSchema, depFn);
    _ReadLightNormalizePower(lightSchema, depFn);
    _ReadLightColor(lightSchema, depFn);
    _ReadLightTemperature(lightSchema, depFn);

    // XXX: LightFilters not yet implemented.
    // XXX: GeometryLight geometry not yet implemented.
    // XXX: DomeLight LightPortals not yet implemented.

    _ReadDistantLightAngle(lightSchema, depFn);

    _ReadLightTextureFile(lightSchema, depFn);

    _ReadAovLight(lightSchema, depFn);

    _ReadEnvDayLight(lightSchema, depFn);

    _ReadLightShapingAPI(lightSchema, depFn);

    _ReadLightShadowAPI(lightSchema, depFn);
}

void _WriteShaderAttributesToUsdPrim_deprecated(
    const MFnDependencyNode& depFn,
#if PXR_VERSION < 2111
    UsdLuxLight& lightSchema
#else
    UsdLuxLightAPI&      lightSchema
#endif
)
{
    _WriteLightIntensity(depFn, lightSchema);
    _WriteLightExposure(depFn, lightSchema);
    _WriteLightDiffuse(depFn, lightSchema);
    _WriteLightSpecular(depFn, lightSchema);
    _WriteLightNormalizePower(depFn, lightSchema);
    _WriteLightColor(depFn, lightSchema);
    _WriteLightTemperature(depFn, lightSchema);

    // XXX: Light filters not yet implemented.
    // XXX: PxrMeshLight geometry not yet implemented.
    // XXX: PxrDomeLight portals not yet implemented.

    _WriteDistantLightAngle(depFn, lightSchema);

    _WriteLightTextureFile(depFn, lightSchema);

    _WriteAovLight(depFn, lightSchema);

    _WriteEnvDayLight(depFn, lightSchema);

    _WriteLightShapingAPI(depFn, lightSchema);

    _WriteLightShadowAPI(depFn, lightSchema);
}

PXR_NAMESPACE_CLOSE_SCOPE
