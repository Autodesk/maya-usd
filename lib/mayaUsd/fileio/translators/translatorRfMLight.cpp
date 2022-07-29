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
#include "translatorRfMLight.h"

#include "pxr/usd/sdf/types.h"

#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/primWriterArgs.h>
#include <mayaUsd/fileio/primWriterContext.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>

#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdLux/cylinderLight.h>
#include <pxr/usd/usdLux/diskLight.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/domeLight.h>
#include <pxr/usd/usdLux/geometryLight.h>
#if PXR_VERSION < 2111
#include <pxr/usd/usdLux/light.h>
#else
#include <pxr/usd/usdLux/lightAPI.h>
#endif
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdLux/shadowAPI.h>
#include <pxr/usd/usdLux/shapingAPI.h>
#include <pxr/usd/usdLux/sphereLight.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
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
    ((CylinderLightMayaTypeName, "PxrCylinderLight"))
    ((DiskLightMayaTypeName, "PxrDiskLight"))
    ((DistantLightMayaTypeName, "PxrDistantLight"))
    ((DomeLightMayaTypeName, "PxrDomeLight"))
    ((EnvDayLightMayaTypeName, "PxrEnvDayLight"))
    ((GeometryLightMayaTypeName, "PxrMeshLight"))
    ((RectLightMayaTypeName, "PxrRectLight"))
    ((SphereLightMayaTypeName, "PxrSphereLight"))
);
// clang-format on

// Instead of hard-coding all of the attributes, we can use Sdr to query the
// args file to figure out what to translate.  This is currently guarded by the
// Usd version (22.09).
static constexpr bool _USE_SDR_TO_TRANSLATE =
#if PXR_VERSION < 2209
    false;
#else
    true;
#endif

void _ReadShaderAttributesFromUsdPrim_deprecated(
#if PXR_VERSION < 2111
    const UsdLuxLight lightSchema,
#else
    const UsdLuxLightAPI lightSchema,
#endif

void _WriteShaderAttributesToUsdPrim_deprecated(
    const MFnDependencyNode& depFn,
#if PXR_VERSION < 2111
    UsdLuxLight& lightSchema
#else
    UsdLuxLightAPI&      lightSchema
#endif
);

static bool _ReportError(const std::string& msg, const SdfPath& primPath = SdfPath())
{
    TF_RUNTIME_ERROR(
        "%s%s",
        msg.c_str(),
        primPath.IsPrimPath() ? TfStringPrintf(" for Light <%s>", primPath.GetText()).c_str() : "");
    return false;
}

static TfToken _GetMayaLightTypeToken(const MFnDependencyNode& depFn)
{
    MStatus       status;
    const MString mayaLightTypeName = depFn.typeName(&status);
    return TfToken(mayaLightTypeName.asChar());
}

#if PXR_VERSION < 2111
static UsdLuxLight _DefineUsdLuxLightForMayaLight(
    const MFnDependencyNode&  depFn,
    const TfToken&            mayaLightTypeToken,
    UsdMayaPrimWriterContext* context)
{
    UsdLuxLight lightSchema;

    UsdStageRefPtr stage = context->GetUsdStage();
    const SdfPath& authorPath = context->GetAuthorPath();

    if (mayaLightTypeToken == _tokens->AovLightMayaTypeName) {
        lightSchema = UsdLuxLight(stage->DefinePrim(authorPath, _tokens->AovLightMayaTypeName));
    } else if (mayaLightTypeToken == _tokens->CylinderLightMayaTypeName) {
        lightSchema = UsdLuxCylinderLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->DiskLightMayaTypeName) {
        lightSchema = UsdLuxDiskLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->DistantLightMayaTypeName) {
        lightSchema = UsdLuxDistantLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->DomeLightMayaTypeName) {
        lightSchema = UsdLuxDomeLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->EnvDayLightMayaTypeName) {
        lightSchema = UsdLuxLight(stage->DefinePrim(authorPath, _tokens->EnvDayLightMayaTypeName));
    } else if (mayaLightTypeToken == _tokens->GeometryLightMayaTypeName) {
        lightSchema = UsdLuxGeometryLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->RectLightMayaTypeName) {
        lightSchema = UsdLuxRectLight::Define(stage, authorPath);
    } else if (mayaLightTypeToken == _tokens->SphereLightMayaTypeName) {
        lightSchema = UsdLuxSphereLight::Define(stage, authorPath);
    } else {
        _ReportError("Could not determine UsdLux schema for Maya light", authorPath);
    }

    return lightSchema;
}
#else
static UsdLuxLightAPI _DefineUsdLuxLightForMayaLight(
    const MFnDependencyNode&  depFn,
    const TfToken&            mayaLightTypeToken,
    UsdMayaPrimWriterContext* context)
{
    UsdLuxLightAPI lightSchema;

    UsdStageRefPtr stage = context->GetUsdStage();
    const SdfPath& authorPath = context->GetAuthorPath();

    if (mayaLightTypeToken == _tokens->AovLightMayaTypeName) {
        lightSchema = UsdLuxLightAPI(stage->DefinePrim(authorPath, _tokens->AovLightMayaTypeName));
    } else if (mayaLightTypeToken == _tokens->CylinderLightMayaTypeName) {
        lightSchema = UsdLuxCylinderLight::Define(stage, authorPath).LightAPI();
    } else if (mayaLightTypeToken == _tokens->DiskLightMayaTypeName) {
        lightSchema = UsdLuxDiskLight::Define(stage, authorPath).LightAPI();
    } else if (mayaLightTypeToken == _tokens->DistantLightMayaTypeName) {
        lightSchema = UsdLuxDistantLight::Define(stage, authorPath).LightAPI();
    } else if (mayaLightTypeToken == _tokens->DomeLightMayaTypeName) {
        lightSchema = UsdLuxDomeLight::Define(stage, authorPath).LightAPI();
    } else if (mayaLightTypeToken == _tokens->EnvDayLightMayaTypeName) {
        lightSchema
            = UsdLuxLightAPI(stage->DefinePrim(authorPath, _tokens->EnvDayLightMayaTypeName));
    } else if (mayaLightTypeToken == _tokens->GeometryLightMayaTypeName) {
        lightSchema = UsdLuxGeometryLight::Define(stage, authorPath).LightAPI();
    } else if (mayaLightTypeToken == _tokens->RectLightMayaTypeName) {
        lightSchema = UsdLuxRectLight::Define(stage, authorPath).LightAPI();
    } else if (mayaLightTypeToken == _tokens->SphereLightMayaTypeName) {
        lightSchema = UsdLuxSphereLight::Define(stage, authorPath).LightAPI();
    } else {
        _ReportError("Could not determine UsdLux schema for Maya light", authorPath);
    }

    return lightSchema;
}
#endif

/* static */
bool UsdMayaTranslatorRfMLight::Write(
    const UsdMayaPrimWriterArgs& args,
    UsdMayaPrimWriterContext*    context)
{
    const SdfPath& authorPath = context->GetAuthorPath();

    MStatus                 status;
    const MObject&          lightObj = args.GetMObject();
    const MFnDependencyNode depFn(lightObj, &status);
    if (status != MS::kSuccess) {
        return _ReportError("Failed to get Maya light", authorPath);
    }

    const TfToken mayaLightTypeToken = _GetMayaLightTypeToken(depFn);
    if (mayaLightTypeToken.IsEmpty()) {
        TF_RUNTIME_ERROR("Could not determine Maya light type for node %s", depFn.name().asChar());
        return false;
    }

#if PXR_VERSION < 2111
    UsdLuxLight lightSchema = _DefineUsdLuxLightForMayaLight(depFn, mayaLightTypeToken, context);
#else
    UsdLuxLightAPI lightSchema = _DefineUsdLuxLightForMayaLight(depFn, mayaLightTypeToken, context);
#endif
    if (!lightSchema) {
        return _ReportError("Failed to create UsdLuxLightAPI prim", authorPath);
    }

    if (_USE_SDR_TO_TRANSLATE) {
        UsdMayaTranslatorUtil::WriteShaderAttributesToUsdPrim(
            depFn, mayaLightTypeToken, lightSchema.GetPrim());
    } else {
        _WriteShaderAttributesToUsdPrim_deprecated(depFn, lightSchema);
    }

    return true;
}

#if PXR_VERSION < 2111
static TfToken _GetMayaTypeTokenForUsdLuxLight(const UsdLuxLight& lightSchema)
#else
static TfToken _GetMayaTypeTokenForUsdLuxLight(const UsdLuxLightAPI& lightSchema)
#endif
{
    const UsdPrim& lightPrim = lightSchema.GetPrim();

    static const TfType& usdSchemaBase = TfType::FindByName(_tokens->UsdSchemaBase);
    static const TfType& pxrAovLightType
        = usdSchemaBase.FindDerivedByName(_tokens->AovLightMayaTypeName);
    static const TfType& pxrEnvDayLightType
        = usdSchemaBase.FindDerivedByName(_tokens->EnvDayLightMayaTypeName);

    const TfType& lightType = usdSchemaBase.FindDerivedByName(lightPrim.GetTypeName());

    if (lightType.IsA(pxrAovLightType)) {
        return _tokens->AovLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxCylinderLight>()) {
        return _tokens->CylinderLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxDiskLight>()) {
        return _tokens->DiskLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxDistantLight>()) {
        return _tokens->DistantLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxDomeLight>()) {
        return _tokens->DomeLightMayaTypeName;
    } else if (lightType.IsA(pxrEnvDayLightType)) {
        return _tokens->EnvDayLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxGeometryLight>()) {
        return _tokens->GeometryLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxRectLight>()) {
        return _tokens->RectLightMayaTypeName;
    } else if (lightPrim.IsA<UsdLuxSphereLight>()) {
        return _tokens->SphereLightMayaTypeName;
    }

    return TfToken();
}

/* static */
bool UsdMayaTranslatorRfMLight::Read(
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext&    context)
{
    const UsdPrim& usdPrim = args.GetUsdPrim();
    if (!usdPrim) {
        return false;
    }

#if PXR_VERSION < 2111
    const UsdLuxLight lightSchema(usdPrim);
#else
    const UsdLuxLightAPI lightSchema(usdPrim);
#endif
    if (!lightSchema) {
        return _ReportError("Failed to read UsdLuxLightAPI prim", usdPrim.GetPath());
    }

    const TfToken mayaLightTypeToken = _GetMayaTypeTokenForUsdLuxLight(lightSchema);
    if (mayaLightTypeToken.IsEmpty()) {
        return _ReportError(
            "Could not determine Maya light type for UsdLuxLightAPI prim", lightSchema.GetPath());
    }

    MObject parentNode = context.GetMayaNode(lightSchema.GetPath().GetParentPath(), false);

    MStatus status;
    MObject mayaNodeTransformObj;
    if (!UsdMayaTranslatorUtil::CreateTransformNode(
            usdPrim, parentNode, args, &context, &status, &mayaNodeTransformObj)) {
        return _ReportError("Failed to create transform node", lightSchema.GetPath());
    }

    const MString nodeName = TfStringPrintf("%sShape", usdPrim.GetName().GetText()).c_str();

    MObject lightObj;
    if (!UsdMayaTranslatorUtil::CreateShaderNode(
            nodeName,
            MString(mayaLightTypeToken.GetText()),
            UsdMayaShadingNodeType::Light,
            &status,
            &lightObj,
            mayaNodeTransformObj)) {
        return _ReportError(
            TfStringPrintf("Failed to create %s node", mayaLightTypeToken.GetText()),
            lightSchema.GetPath());
    }

    const std::string nodePath
        = lightSchema.GetPath().AppendChild(TfToken(nodeName.asChar())).GetString();
    context.RegisterNewMayaNode(nodePath, lightObj);

    MFnDependencyNode depFn(lightObj, &status);
    if (status != MS::kSuccess) {
        return _ReportError("Failed to get Maya light", lightSchema.GetPath());
    }

    if (_USE_SDR_TO_TRANSLATE) {
        UsdMayaTranslatorUtil::ReadShaderAttributesFromUsdPrim(
            lightSchema.GetPrim(), mayaLightTypeToken, depFn);
    } else {
        _ReadShaderAttributesFromUsdPrim_deprecated(lightSchema, depFn);
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
