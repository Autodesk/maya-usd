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
#include "lightWriter.h"

#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/translators/translatorLight.h>
#include <mayaUsd/fileio/utils/adaptor.h>

#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/lightAPI.h>

#include <maya/MFnDirectionalLight.h>
#include <maya/MGlobal.h>

PXR_NAMESPACE_OPEN_SCOPE

/// directionalLight
PXRUSDMAYA_REGISTER_WRITER(directionalLight, PxrUsdTranslators_DirectionalLightWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(directionalLight, UsdLuxDistantLight);

PxrUsdTranslators_DirectionalLightWriter::PxrUsdTranslators_DirectionalLightWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    UsdLuxDistantLight distantLight = UsdLuxDistantLight::Define(GetUsdStage(), GetUsdPath());
    _usdPrim = distantLight.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdLuxDistantLight at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }
}

/* virtual */
void PxrUsdTranslators_DirectionalLightWriter::Write(const UsdTimeCode& usdTime)
{
    MStatus status;
    UsdMayaPrimWriter::Write(usdTime);

    UsdLuxDistantLight  primSchema(_usdPrim);
    MFnDirectionalLight lightFn(GetDagPath(), &status);
    if (!status) {
        // Do this just to get the print
        CHECK_MSTATUS(status);
        return;
    }

    auto animType = _writeJobCtx.GetArgs().animationType;
    if (usdTime.IsDefault() && animType != UsdMayaJobExportArgsTokens->timesamples) {
        UsdMayaTranslatorLight::WriteLightSplinesAttrs(primSchema.LightAPI(), lightFn);
        UsdMayaTranslatorLight::WriteDirectionalLightSplineAttrs(primSchema.LightAPI(), lightFn);
    }

    const bool exportTimeSamples = animType != UsdMayaJobExportArgsTokens->curves;
    // First write the base light attributes
    UsdMayaTranslatorLight::WriteLightAttrs(
        usdTime, primSchema.LightAPI(), lightFn, exportTimeSamples, _GetSparseValueWriter());

    if (animType != UsdMayaJobExportArgsTokens->curves) {
        // Then write the specialized attributes for Directional lights
        UsdMayaTranslatorLight::WriteDirectionalLightAttrs(
            usdTime, primSchema, lightFn, _GetSparseValueWriter());
    }
}

/// pointLight
PXRUSDMAYA_REGISTER_WRITER(pointLight, PxrUsdTranslators_PointLightWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(pointLight, UsdLuxSphereLight);

PxrUsdTranslators_PointLightWriter::PxrUsdTranslators_PointLightWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    UsdLuxSphereLight sphereLight = UsdLuxSphereLight::Define(GetUsdStage(), GetUsdPath());
    _usdPrim = sphereLight.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdLuxSphereLight at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }
}

/* virtual */
void PxrUsdTranslators_PointLightWriter::Write(const UsdTimeCode& usdTime)
{
    MStatus status;
    UsdMayaPrimWriter::Write(usdTime);

    UsdLuxSphereLight primSchema(_usdPrim);
    MFnPointLight     lightFn(GetDagPath(), &status);
    if (!status) {
        // Do this just to get the print
        CHECK_MSTATUS(status);
        return;
    }

    auto animType = _writeJobCtx.GetArgs().animationType;
    if (usdTime.IsDefault() && animType != UsdMayaJobExportArgsTokens->timesamples) {
        UsdMayaTranslatorLight::WriteLightSplinesAttrs(primSchema.LightAPI(), lightFn);
        UsdMayaTranslatorLight::WritePointLightSplineAttrs(
            primSchema.LightAPI(), lightFn, _metersPerUnitScalingFactor);
    }

    const bool exportTimeSamples = animType != UsdMayaJobExportArgsTokens->curves;
    // First write the base light attributes
    UsdMayaTranslatorLight::WriteLightAttrs(
        usdTime, primSchema.LightAPI(), lightFn, exportTimeSamples, _GetSparseValueWriter());

    if (exportTimeSamples) {
        // Then write the specialized attributes for Point lights
        UsdMayaTranslatorLight::WritePointLightAttrs(
            usdTime, primSchema, lightFn, _metersPerUnitScalingFactor, _GetSparseValueWriter());
    }
}

/// spotLight
PXRUSDMAYA_REGISTER_WRITER(spotLight, PxrUsdTranslators_SpotLightWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(spotLight, UsdLuxSphereLight);

PxrUsdTranslators_SpotLightWriter::PxrUsdTranslators_SpotLightWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    UsdLuxSphereLight sphereLight = UsdLuxSphereLight::Define(GetUsdStage(), GetUsdPath());
    _usdPrim = sphereLight.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdLuxSphereLight at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }
}

/* virtual */
void PxrUsdTranslators_SpotLightWriter::Write(const UsdTimeCode& usdTime)
{
    MStatus status;
    UsdMayaPrimWriter::Write(usdTime);

    UsdLuxSphereLight primSchema(_usdPrim);
    MFnSpotLight      lightFn(GetDagPath(), &status);
    if (!status) {
        // Do this just to get the print
        CHECK_MSTATUS(status);
        return;
    }

    auto animType = _writeJobCtx.GetArgs().animationType;
    if (usdTime.IsDefault() && animType != UsdMayaJobExportArgsTokens->timesamples) {
        UsdMayaTranslatorLight::WriteLightSplinesAttrs(primSchema.LightAPI(), lightFn);
        UsdMayaTranslatorLight::WriteSpotLightSplineAttrs(
            primSchema.LightAPI(), lightFn, _metersPerUnitScalingFactor);
    }

    const bool exportTimeSamples = animType != UsdMayaJobExportArgsTokens->curves;
    // First write the base light attributes
    UsdMayaTranslatorLight::WriteLightAttrs(
        usdTime, primSchema.LightAPI(), lightFn, exportTimeSamples, _GetSparseValueWriter());

    if (exportTimeSamples) {
        // Then write the specialized attributes for spot lights
        UsdMayaTranslatorLight::WriteSpotLightAttrs(
            usdTime, primSchema, lightFn, _metersPerUnitScalingFactor, _GetSparseValueWriter());
    }
}

/// areaLight
PXRUSDMAYA_REGISTER_WRITER(areaLight, PxrUsdTranslators_AreaLightWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(areaLight, UsdLuxRectLight);

PxrUsdTranslators_AreaLightWriter::PxrUsdTranslators_AreaLightWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    UsdLuxRectLight rectLight = UsdLuxRectLight::Define(GetUsdStage(), GetUsdPath());
    _usdPrim = rectLight.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdLuxSphereLight at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }
}

/* virtual */
void PxrUsdTranslators_AreaLightWriter::Write(const UsdTimeCode& usdTime)
{
    MStatus status;
    UsdMayaPrimWriter::Write(usdTime);

    UsdLuxRectLight primSchema(_usdPrim);
    MFnAreaLight    lightFn(GetDagPath(), &status);
    if (!status) {
        // Do this just to get the print
        CHECK_MSTATUS(status);
        return;
    }

    auto animType = _writeJobCtx.GetArgs().animationType;
    if (usdTime.IsDefault() && animType != UsdMayaJobExportArgsTokens->timesamples) {
        UsdMayaTranslatorLight::WriteLightSplinesAttrs(primSchema.LightAPI(), lightFn);
    }

    UsdMayaTranslatorLight::WriteLightAttrs(
        usdTime,
        primSchema.LightAPI(),
        lightFn,
        animType != UsdMayaJobExportArgsTokens->curves,
        _GetSparseValueWriter());

    // No splines for area lights, but we call the function to ensure that all attribute are written
    UsdMayaTranslatorLight::WriteAreaLightAttrs(
        usdTime, primSchema, lightFn, _GetSparseValueWriter());
}

PXR_NAMESPACE_CLOSE_SCOPE
