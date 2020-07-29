//
// Copyright 2017 Animal Logic
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

#include "DirectionalLight.h"

#include "AL/usdmaya/fileio/AnimationTranslator.h"
#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"
#include "AL/usdmaya/utils/DgNodeHelper.h"

#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdLux/distantLight.h>

#include <maya/MAngle.h>
#include <maya/MFnDirectionalLight.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MNodeClass.h>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

AL_USDMAYA_DEFINE_TRANSLATOR(DirectionalLight, PXR_NS::UsdLuxDistantLight)

//----------------------------------------------------------------------------------------------------------------------
MObject DirectionalLight::m_pointWorld;
MObject DirectionalLight::m_lightAngle;
MObject DirectionalLight::m_color;
MObject DirectionalLight::m_intensity;
MObject DirectionalLight::m_exposure;
MObject DirectionalLight::m_diffuse;
MObject DirectionalLight::m_specular;
MObject DirectionalLight::m_normalize;
MObject DirectionalLight::m_enableColorTemperature;

//----------------------------------------------------------------------------------------------------------------------
MStatus DirectionalLight::initialize()
{
    MStatus status = MS::kSuccess;

    MNodeClass        dirLight("directionalLight");
    const char* const errorString
        = "DirectionalLightTranslator: error retrieving maya directional light attributes";
    m_pointWorld = dirLight.attribute("pw", &status);
    m_lightAngle = dirLight.attribute("lang", &status);
    m_color = dirLight.attribute("cl", &status);
    m_intensity = dirLight.attribute(
        "in", &status); // Maya intensity has a different form of 2, need to check
    AL_MAYA_CHECK_ERROR(status, errorString);

    MFnNumericAttribute numAttr;
    if (!dirLight.hasAttribute("ex", &status)) {
        m_exposure = numAttr.create("exposure", "ex", MFnNumericData::kFloat);
        status = dirLight.addExtensionAttribute(m_exposure);
        AL_MAYA_CHECK_ERROR2(status, "Initialize: Failed to create extension attribute: exposure.");
    }

    if (!dirLight.hasAttribute("dif", &status)) {
        m_diffuse = numAttr.create("diffuse", "dif", MFnNumericData::kFloat, 1.0);
        status = dirLight.addExtensionAttribute(m_diffuse);
        AL_MAYA_CHECK_ERROR2(status, "Initialize: Failed to create extension attribute: diffuse.");
    }

    if (!dirLight.hasAttribute("spe", &status)) {
        m_specular = numAttr.create("specular", "spe", MFnNumericData::kFloat, 1.0);
        status = dirLight.addExtensionAttribute(m_specular);
        AL_MAYA_CHECK_ERROR2(status, "Initialize: Failed to create extension attribute: specular.");
    }

    if (!dirLight.hasAttribute("nor", &status)) {
        m_normalize = numAttr.create("normalize", "nor", MFnNumericData::kBoolean);
        status = dirLight.addExtensionAttribute(m_normalize);
        AL_MAYA_CHECK_ERROR2(
            status, "Initialize: Failed to create extension attribute: normalize.");
    }

    if (!dirLight.hasAttribute("ect", &status)) {
        m_enableColorTemperature
            = numAttr.create("enableColorTemperature", "ect", MFnNumericData::kBoolean);
        status = dirLight.addExtensionAttribute(m_enableColorTemperature);
        AL_MAYA_CHECK_ERROR2(
            status, "Initialize: Failed to create extension attribute: enableColorTemperature.");
    }

    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DirectionalLight::import(const UsdPrim& prim, MObject& parent, MObject& createObj)
{
    MStatus status = MS::kSuccess;

    MFnDirectionalLight fnDirectionLight;
    createObj = fnDirectionLight.create(parent, true, false, &status);
    AL_MAYA_CHECK_ERROR2(status, MString("unable to create directional light "));

    TranslatorContextPtr ctx = context();
    if (ctx) {
        ctx->insertItem(prim, createObj);
    }
    status = updateMayaAttributes(createObj, prim);

    return status;
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim DirectionalLight::exportObject(
    UsdStageRefPtr        stage,
    MDagPath              dagPath,
    const SdfPath&        usdPath,
    const ExporterParams& params)
{
    MStatus             status = MS::kSuccess;
    MFnDirectionalLight fnDireLight(dagPath, &status);
    AL_MAYA_CHECK_ERROR2(
        status, "Export: Failed to attach function set to directional light dagPath.");

    const MObject direLightObj = fnDireLight.object(&status);
    AL_MAYA_CHECK_ERROR2(status, "Export: Failed to retrieve object.");

    updateUsdPrim(stage, usdPath, direLightObj);

    // If there is animation keyed on this light
    AnimationTranslator* animTranslator = params.m_animTranslator;
    UsdLuxDistantLight   usdLight(stage->GetPrimAtPath(usdPath));
    if (animTranslator) {
        animTranslator->addPlug(MPlug(direLightObj, m_lightAngle), usdLight.GetAngleAttr(), true);
        animTranslator->addPlug(MPlug(direLightObj, m_color), usdLight.GetColorAttr(), true);
        animTranslator->addPlug(
            MPlug(direLightObj, m_intensity), usdLight.GetIntensityAttr(), true);
        animTranslator->addPlug(MPlug(direLightObj, m_exposure), usdLight.GetExposureAttr(), true);
        animTranslator->addPlug(MPlug(direLightObj, m_diffuse), usdLight.GetDiffuseAttr(), true);
        animTranslator->addPlug(MPlug(direLightObj, m_specular), usdLight.GetSpecularAttr(), true);
        animTranslator->addPlug(
            MPlug(direLightObj, m_normalize), usdLight.GetNormalizeAttr(), true);
        animTranslator->addPlug(
            MPlug(direLightObj, m_enableColorTemperature),
            usdLight.GetEnableColorTemperatureAttr(),
            true);
    }

    return stage->GetPrimAtPath(usdPath);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DirectionalLight::preTearDown(UsdPrim& prim)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("DirectionalLightTranslator::preTearDown prim=%s\n", prim.GetPath().GetText());
    if (!prim.IsValid()) {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg("DirectionalLightTranslator::preTearDown prim invalid\n");
        return MS::kFailure;
    }
    TranslatorBase::preTearDown(prim);

    // Write the overrides back to the path it was imported at
    MStatus       status = MS::kSuccess;
    MObjectHandle handleToLight;
    if (!context()->getMObject(prim, handleToLight, MFn::kDirectionalLight)) {
        MGlobal::displayError("unable to locate directional light");
        status = MS::kFailure;
        return status;
    }

    updateUsdPrim(prim.GetStage(), prim.GetPath(), handleToLight.object());

    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DirectionalLight::tearDown(const SdfPath& path)
{
    MObjectHandle obj;
    bool          result = context()->getMObject(path, obj, MFn::kDirectionalLight);

    if (result) {
        context()->removeItems(path);
        return MS::kSuccess;
    } else
        return MS::kFailure;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DirectionalLight::updateMayaAttributes(MObject mayaObj, const UsdPrim& prim)
{
    MStatus status = MS::kSuccess;

    const char* const errorString
        = "DirectionalLightTranslator: error setting maya directional light parameters";

    UsdLuxDistantLight usdDistLight(prim);

    GfVec3f pointWorld(1.0f, 1.0f, 1.0f);
    float   angle = 0.0f;
    float   intensity = 0.0f;
    float   exposure = 0.0f;
    float   diffuse = 0.0f;
    float   specular = 0.0f;
    bool    normalize = false;
    GfVec3f color(1.0f, 1.0f, 1.0f);
    bool    enableColorTemperature = false;

    if (usdDistLight.GetPrim().HasAttribute(TfToken("pointWorld")))
        usdDistLight.GetPrim().GetAttribute(TfToken("pointWorld")).Get(&pointWorld);
    usdDistLight.GetAngleAttr().Get(&angle);
    usdDistLight.GetIntensityAttr().Get(
        &intensity); // right now just use Maya's intensity value, not using the lightIntensity
    usdDistLight.GetExposureAttr().Get(&exposure);
    usdDistLight.GetDiffuseAttr().Get(&diffuse);
    usdDistLight.GetSpecularAttr().Get(&specular);
    usdDistLight.GetNormalizeAttr().Get(&normalize);
    usdDistLight.GetColorAttr().Get(&color);
    usdDistLight.GetEnableColorTemperatureAttr().Get(&enableColorTemperature);

    // Update prim attribute values to maya node attibute values
    AL_MAYA_CHECK_ERROR(
        DgNodeTranslator::setVec3(
            mayaObj, m_pointWorld, pointWorld[0], pointWorld[1], pointWorld[2]),
        errorString);
    AL_MAYA_CHECK_ERROR(
        DgNodeTranslator::setAngle(mayaObj, m_lightAngle, MAngle(angle, MAngle::Unit::kRadians)),
        errorString);
    AL_MAYA_CHECK_ERROR(
        DgNodeTranslator::setVec3(mayaObj, m_color, color[0], color[1], color[2]), errorString);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setFloat(mayaObj, m_intensity, intensity), errorString);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setFloat(mayaObj, m_exposure, exposure), errorString);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setFloat(mayaObj, m_diffuse, diffuse), errorString);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setFloat(mayaObj, m_specular, specular), errorString);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setBool(mayaObj, m_normalize, normalize), errorString);
    AL_MAYA_CHECK_ERROR(
        DgNodeTranslator::setBool(mayaObj, m_enableColorTemperature, enableColorTemperature),
        errorString);

    return status;
}

//----------------------------------------------------------------------------------------------------------------------
bool DirectionalLight::updateUsdPrim(
    const UsdStageRefPtr& stage,
    const SdfPath&        usdPath,
    const MObject&        mayaObj)
{
    const char* const errorString
        = "DirectionalLightTranslator: error getting maya directional light parameters";
    float  pointWorld[3] = { 1.0f, 1.0f, 1.0f };
    MAngle angle;
    float  intensity = 0.0f;
    float  exposure = 0.0f;
    float  diffuse = 1.0f;
    float  specular = 1.0f;
    bool   normalize = false;
    float  color[3] = { 1.0f, 1.0f, 1.0f };
    bool   enableColorTemperature = false;

    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getVec3(mayaObj, m_pointWorld, pointWorld), errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getAngle(mayaObj, m_lightAngle, angle), errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getVec3(mayaObj, m_color, color), errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getFloat(mayaObj, m_intensity, intensity), errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getFloat(mayaObj, m_exposure, exposure), errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getFloat(mayaObj, m_diffuse, diffuse), errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getFloat(mayaObj, m_specular, specular), errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getBool(mayaObj, m_normalize, normalize), errorString);
    AL_MAYA_CHECK_ERROR2(
        AL::usdmaya::utils::DgNodeHelper::getBool(
            mayaObj, m_enableColorTemperature, enableColorTemperature),
        errorString);

    bool               result = true;
    UsdLuxDistantLight usdLight = UsdLuxDistantLight::Define(stage, usdPath);

    if (!(pointWorld[0] == 1.0f && pointWorld[1] == 1.0f && pointWorld[2] == 1.0f)) {
        // Need to create an attribute for "pointWorld" if it does not exist yet
        if (UsdAttribute pwAttr
            = usdLight.GetPrim().CreateAttribute(TfToken("pointWorld"), SdfValueTypeNames->Float3))
            result &= pwAttr.Set(GfVec3f(pointWorld));
    }
    if (angle.asRadians() != 0.0f)
        result &= usdLight.GetAngleAttr().Set(float(angle.asRadians()));
    if (!(color[0] == 1.0f && color[1] == 1.0f && color[2] == 1.0f))
        result &= usdLight.GetColorAttr().Set(GfVec3f(color));
    if (intensity != 1.0f)
        result &= usdLight.GetIntensityAttr().Set(intensity);
    if (exposure != 0.0f)
        result &= usdLight.GetExposureAttr().Set(exposure);
    if (diffuse != 1.0f)
        result &= usdLight.GetDiffuseAttr().Set(diffuse);
    if (specular != 1.0f)
        result &= usdLight.GetSpecularAttr().Set(specular);
    if (normalize != false)
        result &= usdLight.GetNormalizeAttr().Set(normalize);
    if (enableColorTemperature != false)
        result &= usdLight.GetEnableColorTemperatureAttr().Set(enableColorTemperature);

    return result;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DirectionalLight::update(const UsdPrim& prim)
{
    MStatus status = MS::kSuccess;

    MObjectHandle handleToLight;
    if (context() && !context()->getMObject(prim, handleToLight, MFn::kDirectionalLight)) {
        MGlobal::displayError("unable to locate directional light");
        return MS::kFailure;
    }

    status = updateMayaAttributes(handleToLight.object(), prim);

    return status;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
