//
// Copyright 2021 Autodesk
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
#include "translatorLight.h"

#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/primReaderRegistry.h>
#include <mayaUsd/fileio/primWriterArgs.h>
#include <mayaUsd/fileio/primWriterContext.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/fileio/translators/translatorXformable.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/light.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdLux/shadowAPI.h>
#include <pxr/usd/usdLux/shapingAPI.h>
#include <pxr/usd/usdLux/sphereLight.h>

#include <maya/MColor.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya light types.
    ((SpotLightMayaTypeName, "spotLight"))
    ((DirectionalLightMayaTypeName, "directionalLight"))
    ((PointLightMayaTypeName, "pointLight"))
    ((AreaLightMayaTypeName, "areaLight"))
    ((normalizeAttrName, "normalize"))
    // Maya light plug names.
    ((IntensityPlugName, "intensity"))
    ((ColorPlugName, "color"))
    ((EmitDiffusePlugName, "emitDiffuse"))
    ((EmitSpecularPlugName, "emitSpecular"))
    ((UseRayTraceShadowsPlugName, "useRayTraceShadows"))
    ((ShadowColorPlugName, "shadowColor"))
    ((NormalizePlugName, "normalize"))
    ((LightAnglePlugName, "lightAngle"))
    ((DropoffPlugName, "dropoff"))
    ((PenumbraAnglePlugName, "penumbraAngle"))
    ((ConeAnglePlugName, "coneAngle"))
    ((LightRadiusPlugName, "lightRadius"))
);
// clang-format on

// Export the "common" light attributes from MFnLights to UsdLuxLight
bool UsdMayaTranslatorLight::WriteLightAttrs(const UsdTimeCode &usdTime, UsdLuxLight &usdLight, 
                                MFnLight &mayaLight, UsdUtilsSparseValueWriter* valueWriter)
{
    MStatus status;
    float intensity = mayaLight.intensity(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);    
    UsdMayaWriteUtil::SetAttribute(
            usdLight.GetIntensityAttr(), intensity, usdTime, valueWriter);
    
    UsdPrim prim = usdLight.GetPrim();

    MColor color = mayaLight.color(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    UsdMayaWriteUtil::SetAttribute(
            usdLight.GetColorAttr(), GfVec3f(color.r, color.g, color.b), usdTime, valueWriter);

    // Note that normalize doesn't exist in the Maya light, but might exist as extension attributes
    // in renderers. We won't be authoring it here, so that it follows USD default (false)
    
    bool rayTraceShadows = mayaLight.useRayTraceShadows(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    // Here we're just considering "useRayTracedShadows" to enable UsdLux shadows,
    // and we're ignoring maya's "depthMapShadows" attribute
    if (rayTraceShadows) {
        UsdLuxShadowAPI shadowAPI = UsdLuxShadowAPI::Apply(prim);
        UsdMayaWriteUtil::SetAttribute(
                shadowAPI.CreateShadowEnableAttr(), true, usdTime, valueWriter);

        const MColor shadowColor = mayaLight.shadowColor(&status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        UsdMayaWriteUtil::SetAttribute(
            shadowAPI.CreateShadowColorAttr(),
            GfVec3f(shadowColor.r, shadowColor.g, shadowColor.b),
            usdTime,
            valueWriter);
    }

    // Some renderers have a float value for diffuse and specular just like UsdLuxLight does (it defaults to 1)
    // But Maya lights also have a checkbox to enable/disable diffuse and specular. We can just set 
    // it to 0 or 1 depending on this boolean
    bool lightDiffuse = mayaLight.lightDiffuse(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);    
    UsdMayaWriteUtil::SetAttribute(
        usdLight.GetDiffuseAttr(), lightDiffuse ? 1.f : 0.f, usdTime, valueWriter);
    
    bool lightSpecular = mayaLight.lightSpecular(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    UsdMayaWriteUtil::SetAttribute(
        usdLight.GetSpecularAttr(), lightSpecular ? 1.f : 0.f, usdTime, valueWriter);

    return true;
}

// Import the common light attributes from UsdLuxLight.
// As opposed to the writer, we can't rely on the MFnLight attribute
// accessors, as we need to support animations. Instead we're getting 
// the Maya plugs from MFnDependencyNode
static bool _ReadLightAttrs(const UsdLuxLight& lightSchema, 
                            MFnDependencyNode &depFn,  
                            const UsdMayaPrimReaderArgs& args,
                            UsdMayaPrimReaderContext*    context)
{
    MStatus status;
    bool success = true;
    UsdPrim prim = lightSchema.GetPrim();

    // We need to specify a time when getting an attribute, otherwise we can get invalid data
    // for single time-samples
    UsdTimeCode timeCode(args.GetTimeInterval().GetMin());

    // ReadUsdAttribute will read a Usd attribute, accounting for eventual animations
    success &= UsdMayaReadUtil::ReadUsdAttribute(lightSchema.GetIntensityAttr(), depFn,
                     _tokens->IntensityPlugName,
                     args, context);

    success &= UsdMayaReadUtil::ReadUsdAttribute(lightSchema.GetColorAttr(), depFn,
                     _tokens->ColorPlugName,
                     args, context);

    // For diffuse & specular, the USD value is a [0,1] float, but it's a boolean in Maya.
    // We can't really import this properly, but at least we're enabling the maya attibute 
    // as soon as the input floating value is non-null
    MPlug emitDiffusePlug = depFn.findPlug(_tokens->EmitDiffusePlugName.GetText(), &status);
    success &= (status == MS::kSuccess);
    if (status == MS::kSuccess) {
        float diffuse = 1.f;
        lightSchema.GetDiffuseAttr().Get(&diffuse, timeCode);
        emitDiffusePlug.setBool(diffuse > 0);
    } else {
        success = false;
    }
    MPlug emitSpecularPlug = depFn.findPlug(_tokens->EmitSpecularPlugName.GetText(), &status);
    success &= (status == MS::kSuccess);
    if (status == MS::kSuccess) {
        float specular = 1.f;
        lightSchema.GetSpecularAttr().Get(&specular, timeCode);
        emitSpecularPlug.setBool(specular > 0);
    } else {
        success = false;
    }

    // Check if this primitive has a shadow API
    UsdLuxShadowAPI shadowAPI(prim);
    if (shadowAPI) {
        // We set maya'slight "useRayTracedShadows" if the usd shadows are enabled
        success &= UsdMayaReadUtil::ReadUsdAttribute(shadowAPI.GetShadowEnableAttr(), depFn,
                     _tokens->UseRayTraceShadowsPlugName,
                     args, context);
        success &= UsdMayaReadUtil::ReadUsdAttribute(shadowAPI.GetShadowColorAttr(), depFn,
                     _tokens->ShadowColorPlugName,
                     args, context);
    }
    return success;
}

// Export the specialized MFnDirectionalLight attributes
bool UsdMayaTranslatorLight::WriteDirectionalLightAttrs(
            const UsdTimeCode &usdTime, UsdLuxDistantLight &usdLight, 
            MFnDirectionalLight &mayaLight, UsdUtilsSparseValueWriter* valueWriter)
{
    MStatus status;
    // UsdLuxDistantLight have an attribute "angle" that is similar 
    // to Maya's directional light's shadowAngle attribute
    float shadowAngle = mayaLight.shadowAngle(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    UsdMayaWriteUtil::SetAttribute(
            usdLight.GetAngleAttr(), shadowAngle, usdTime, valueWriter);
    return true;
}

// Import the specialized MFnDirectionalLight attributes
static bool _ReadDirectionalLight(const UsdLuxLight& lightSchema, 
                            MFnDependencyNode &depFn,  
                            const UsdMayaPrimReaderArgs& args,
                            UsdMayaPrimReaderContext*    context)
{
    MStatus status;
    bool success = true;
    UsdPrim prim = lightSchema.GetPrim();
    UsdLuxDistantLight distantLight(lightSchema.GetPrim());
    if (!distantLight) {
        return false;
    }
    success &= UsdMayaReadUtil::ReadUsdAttribute(distantLight.GetAngleAttr(), depFn,
                     _tokens->LightAnglePlugName,
                     args, context);
    return success;
}

// Export the specialized MFnPointLight attributes
bool UsdMayaTranslatorLight::WritePointLightAttrs(
        const UsdTimeCode &usdTime, UsdLuxSphereLight &usdLight, 
        MFnPointLight &mayaLight, UsdUtilsSparseValueWriter* valueWriter)
{
    MStatus status;
    // A pointLight is simply a sphere light with a null radius. 
    // We check however the parameter lightRadius that is used for shadows, and set this as the sphere radius 
    MPlug lightRadiusPlug = mayaLight.findPlug(_tokens->LightRadiusPlugName.GetText(), true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    float lightRadius = lightRadiusPlug.asFloat();

    UsdMayaWriteUtil::SetAttribute(
            usdLight.GetRadiusAttr(), lightRadius, usdTime, valueWriter);
    UsdMayaWriteUtil::SetAttribute(
            usdLight.GetTreatAsPointAttr(), (lightRadius == 0.f), usdTime, valueWriter);
    return true;
}

// Import the specialized MFnPointLight attributes
static bool _ReadPointLight(const UsdLuxLight& lightSchema, 
                            MFnDependencyNode &depFn,  
                            const UsdMayaPrimReaderArgs& args,
                            UsdMayaPrimReaderContext*    context)
{
    MStatus status;
    bool success = true;
    UsdPrim prim = lightSchema.GetPrim();
    UsdLuxSphereLight sphereLight(lightSchema.GetPrim());
    if (!sphereLight) {
        return false;
    }
    success &= UsdMayaReadUtil::ReadUsdAttribute(sphereLight.GetRadiusAttr(), depFn,
                     _tokens->LightRadiusPlugName,
                     args, context);
    return success;
}

// Export the specialized MFnSpotLight attributes
bool UsdMayaTranslatorLight::WriteSpotLightAttrs(const UsdTimeCode &usdTime, UsdLuxSphereLight &usdLight, 
                                MFnSpotLight &mayaLight, UsdUtilsSparseValueWriter* valueWriter)
{
    MStatus status;
    // A spot light is similar to point lights, but it has a shaping API for the spot cone
    MPlug lightRadiusPlug = mayaLight.findPlug(_tokens->LightRadiusPlugName.GetText(), true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    float lightRadius = lightRadiusPlug.asFloat();

    UsdMayaWriteUtil::SetAttribute(
            usdLight.GetRadiusAttr(), lightRadius, usdTime, valueWriter);
    UsdMayaWriteUtil::SetAttribute(
            usdLight.GetTreatAsPointAttr(), (lightRadius == 0.f), usdTime, valueWriter);
    
    UsdPrim prim = usdLight.GetPrim();
    UsdLuxShapingAPI shapingAPI = UsdLuxShapingAPI::Apply(prim);

    // We need some magic conversions between maya dropOff, coneAngle, penumbraAngle, 
    // and Usd shapingFocus, shapingConeAngle, shapingConeSoftness
    const double dropOff = mayaLight.dropOff(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    if (dropOff > 0) {
        UsdMayaWriteUtil::SetAttribute(
            shapingAPI.CreateShapingFocusAttr(),
            static_cast<float>(dropOff),
            usdTime,
            valueWriter);
    }

    double coneAngle = GfRadiansToDegrees(mayaLight.coneAngle(&status)) * 0.5;
    CHECK_MSTATUS_AND_RETURN(status, false);
    double penumbraAngle = GfRadiansToDegrees(mayaLight.penumbraAngle(&status));
    CHECK_MSTATUS_AND_RETURN(status, false);

    float cutoff = static_cast<float>(coneAngle + penumbraAngle);
    UsdMayaWriteUtil::SetAttribute(
        shapingAPI.CreateShapingConeAngleAttr(), cutoff, usdTime, valueWriter);

    float softness = static_cast<float>(cutoff > 0 ? penumbraAngle / cutoff : 0.f);
    if (softness > 0) {
        UsdMayaWriteUtil::SetAttribute(
            shapingAPI.CreateShapingConeSoftnessAttr(), softness, usdTime, valueWriter);
    }

    return true;
}

// Import the specialized MFnSpotLight attributes
static bool _ReadSpotLight(const UsdLuxLight& lightSchema, 
                            MFnDependencyNode &depFn,  
                            const UsdMayaPrimReaderArgs& args,
                            UsdMayaPrimReaderContext*    context)
{

    MStatus status;
    bool success = true;
    UsdPrim prim = lightSchema.GetPrim();
    UsdLuxSphereLight sphereLight(prim);
    if (!sphereLight) {
        return false;
    }
    success &= UsdMayaReadUtil::ReadUsdAttribute(sphereLight.GetRadiusAttr(), depFn,
                     _tokens->LightRadiusPlugName,
                     args, context);

    UsdLuxShapingAPI shapingAPI(prim);
    if (!shapingAPI) {
        return false;
    }

    // We need to specify a time when getting an attribute, otherwise we can get invalid data
    // for single time-samples    
    UsdTimeCode timeCode(args.GetTimeInterval().GetMin());
    // We need some magic conversions between maya dropOff, coneAngle, penumbraAngle, 
    // and Usd shapingFocus, shapingConeAngle, shapingConeSoftness
    success &= UsdMayaReadUtil::ReadUsdAttribute(shapingAPI.GetShapingFocusAttr(), depFn,
                     _tokens->DropoffPlugName,
                     args, context);

    float UsdConeAngle = 1.f;
    shapingAPI.GetShapingConeAngleAttr().Get(&UsdConeAngle, timeCode);
    float coneSoftness = 0.f;
    shapingAPI.GetShapingConeSoftnessAttr().Get(&coneSoftness, timeCode);

    float penumbraAngle = UsdConeAngle * coneSoftness;
    float mayaConeAngle = 2.f * (UsdConeAngle - penumbraAngle);

    // Note that the roundtrip might not return the exact same result as originally,
    // e.g. a negative penumbra angle would become positive. It would result in the same
    // illumination, though with different values
    MPlug penumbraAnglePlug = depFn.findPlug(_tokens->PenumbraAnglePlugName.GetText(), &status);
    success &= (status == MS::kSuccess);
    if (status == MS::kSuccess) {
        penumbraAnglePlug.setFloat(GfDegreesToRadians(penumbraAngle));
    } else {
        success = false;
    }
    MPlug coneAnglePlug = depFn.findPlug(_tokens->ConeAnglePlugName.GetText(), &status);
    success &= (status == MS::kSuccess);
    if (status == MS::kSuccess) {
        coneAnglePlug.setFloat(GfDegreesToRadians(mayaConeAngle));
    } else {
        success = false;
    }
    return success;
}

// Export the specialized MFnAreaLight attributes
bool UsdMayaTranslatorLight::WriteAreaLightAttrs(const UsdTimeCode &usdTime, UsdLuxRectLight &usdLight, 
                                MFnAreaLight &mayaLight, UsdUtilsSparseValueWriter* valueWriter)
{
    MStatus status;

    // Area lights "normalize" isn't exposed through the MFnAreaLight API. 
    // So we're getting it with MFnDependencyNode::findPlug
    MPlug normalizePlug = mayaLight.findPlug(_tokens->normalizeAttrName.GetText(), true, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    UsdMayaWriteUtil::SetAttribute(
            usdLight.GetNormalizeAttr(), normalizePlug.asBool(), usdTime, valueWriter);
    return true;
}

/// Read the parameters from UsdLuxRectLight into a Maya area light
static bool _ReadAreaLight(const UsdLuxLight& lightSchema, 
                            MFnDependencyNode &depFn,  
                            const UsdMayaPrimReaderArgs& args,
                            UsdMayaPrimReaderContext*    context)
{
    MStatus status;
    bool success = true;
    UsdPrim prim = lightSchema.GetPrim();
    UsdLuxRectLight rectLight(prim);
    if (!rectLight) {
        return false;
    }
    
    success &= UsdMayaReadUtil::ReadUsdAttribute(rectLight.GetNormalizeAttr(), depFn,
                     _tokens->normalizeAttrName,
                     args, context);

    return success;
}



/* static */
bool UsdMayaTranslatorLight::Read(
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext*    context)
{   
    // Get the Usd primitive we're reading 
    const UsdPrim& usdPrim = args.GetUsdPrim();
    if (!usdPrim) {
        return false;
    }
    const UsdLuxLight lightSchema(usdPrim);
    if (!lightSchema) {
        TF_RUNTIME_ERROR("Failed to read UsdLuxLight prim for light %s", usdPrim.GetPath().GetText());
        return false;
    }
    // Find the corresponding maya light type depending on 
    // the usd light schema
    TfToken mayaLightTypeToken;
    if (usdPrim.IsA<UsdLuxDistantLight>()) {
        // USD distant light => Maya Directional Light
        mayaLightTypeToken = _tokens->DirectionalLightMayaTypeName;
    } else if (usdPrim.IsA<UsdLuxRectLight>()) {
        // USD Rect Light => Maya Area Light
        mayaLightTypeToken = _tokens->AreaLightMayaTypeName;
    } else if (usdPrim.IsA<UsdLuxSphereLight>()) {
        // For USD Sphere Lights, if they have a shapingAPI with 
        // a non-null cone angle, we import it as a Maya Spot light.
        // Otherwise it's a Point light
        UsdLuxShapingAPI shapingAPI(lightSchema);
        mayaLightTypeToken = (shapingAPI) ? _tokens->SpotLightMayaTypeName : _tokens->PointLightMayaTypeName;
    }    
    if (mayaLightTypeToken.IsEmpty()) {
        TF_RUNTIME_ERROR("Could not determine Maya light type for UsdLuxLight prim %s", usdPrim.GetPath().GetText());
        return false;
    }

    // Find which maya node needs to be our light's parent
    MObject parentNode = context->GetMayaNode(lightSchema.GetPath().GetParentPath(), false);
    MStatus status;
    MObject mayaNodeTransformObj;
    // First create the transform node
    if (!UsdMayaTranslatorUtil::CreateTransformNode(
            usdPrim, parentNode, args, context, &status, &mayaNodeTransformObj)) {
        TF_RUNTIME_ERROR("Failed to create transform node for %s", lightSchema.GetPath().GetText());
        return false;
    }

    // Create the Maya light (shape) node of the desired type
    const MString nodeName = TfStringPrintf("%sShape", usdPrim.GetName().GetText()).c_str();
    MObject lightObj;
    if (!UsdMayaTranslatorUtil::CreateShaderNode(
            nodeName,
            MString(mayaLightTypeToken.GetText()),
            UsdMayaShadingNodeType::Light,
            &status,
            &lightObj,
            mayaNodeTransformObj)) {
        TF_RUNTIME_ERROR(TfStringPrintf("Failed to create %s node for light %s", mayaLightTypeToken.GetText(), lightSchema.GetPath()));
        return false;
    }

    const std::string nodePath
        = lightSchema.GetPath().AppendChild(TfToken(nodeName.asChar())).GetString();
    context->RegisterNewMayaNode(nodePath, lightObj);

    MFnDependencyNode depFn(lightObj, &status);
    if (status != MS::kSuccess) {
        TF_RUNTIME_ERROR("Failed to get Maya light %s", lightSchema.GetPath().GetText());
        return false;
    }
    
    // Whatever the light type is, we always want to read the "common" UsdLuxLight attributes
    _ReadLightAttrs(lightSchema, depFn, args, context);
    // Read the specialized light attributes depending on the maya light type
    if (mayaLightTypeToken == _tokens->DirectionalLightMayaTypeName) {
        _ReadDirectionalLight(lightSchema, depFn, args, context);
    } else if (mayaLightTypeToken == _tokens->PointLightMayaTypeName) {
        _ReadPointLight(lightSchema, depFn, args, context);
    } else if (mayaLightTypeToken == _tokens->SpotLightMayaTypeName) {
        _ReadSpotLight(lightSchema, depFn, args, context);
    } else if (mayaLightTypeToken == _tokens->AreaLightMayaTypeName) {
        _ReadAreaLight(lightSchema, depFn, args, context);
    } 
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
