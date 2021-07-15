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

#include <maya/MFnDirectionalLight.h>
#include <maya/MGlobal.h>
#include <pxr/usd/usdLux/light.h>
#include <pxr/usd/usdLux/distantLight.h>

PXR_NAMESPACE_OPEN_SCOPE

/// directionalLight
PXRUSDMAYA_REGISTER_WRITER(directionalLight, PxrUsdTranslators_DirectionalLightWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(directionalLight, UsdLuxDistantLight);

PxrUsdTranslators_DirectionalLightWriter::PxrUsdTranslators_DirectionalLightWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx)  : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
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
    // Since write() above will take care of any animation on the light's
    // transform, we only want to proceed here if:
    // - We are at the default time and NO attributes on the shape are animated.
    //    OR
    // - We are at a non-default time and some attribute on the shape IS animated.
    if (usdTime.IsDefault() == _HasAnimCurves()) {
        return;
    }
    UsdLuxDistantLight primSchema(_usdPrim);
    MFnDirectionalLight lightFn(GetDagPath(), &status);
    if (!status) {
        // Do this just to get the print
        CHECK_MSTATUS(status);
        return;
    }
    
    // First write the base light attributes
    UsdMayaTranslatorLight::WriteLightAttrs(usdTime, primSchema, lightFn, _GetSparseValueWriter());
    // Then write the specialized attributes for directional lights
    UsdMayaTranslatorLight::WriteDirectionalLightAttrs(usdTime, primSchema, lightFn, _GetSparseValueWriter());
}

/// pointLight
PXRUSDMAYA_REGISTER_WRITER(pointLight, PxrUsdTranslators_PointLightWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(pointLight, UsdLuxSphereLight);


PxrUsdTranslators_PointLightWriter::PxrUsdTranslators_PointLightWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx)  : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
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
    // Since write() above will take care of any animation on the light's
    // transform, we only want to proceed here if:
    // - We are at the default time and NO attributes on the shape are animated.
    //    OR
    // - We are at a non-default time and some attribute on the shape IS animated.
    if (usdTime.IsDefault() == _HasAnimCurves()) {
        return;
    }
    UsdLuxSphereLight primSchema(_usdPrim);
    MFnPointLight lightFn(GetDagPath(), &status);
    if (!status) {
        // Do this just to get the print
        CHECK_MSTATUS(status);
        return;
    }

    // First write the base light attributes
    UsdMayaTranslatorLight::WriteLightAttrs(usdTime, primSchema, lightFn, _GetSparseValueWriter());
    // Then write the specialized attributes for point lights
    UsdMayaTranslatorLight::WritePointLightAttrs(usdTime, primSchema, lightFn, _GetSparseValueWriter());
}

/// spotLight
PXRUSDMAYA_REGISTER_WRITER(spotLight, PxrUsdTranslators_SpotLightWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(spotLight, UsdLuxSphereLight);

PxrUsdTranslators_SpotLightWriter::PxrUsdTranslators_SpotLightWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx)  : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
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
    // Since write() above will take care of any animation on the light's
    // transform, we only want to proceed here if:
    // - We are at the default time and NO attributes on the shape are animated.
    //    OR
    // - We are at a non-default time and some attribute on the shape IS animated.
    if (usdTime.IsDefault() == _HasAnimCurves()) {
        return;
    }
    UsdLuxSphereLight primSchema(_usdPrim);
    MFnSpotLight lightFn(GetDagPath(), &status);
    if (!status) {
        // Do this just to get the print
        CHECK_MSTATUS(status);
        return;
    }
    // First write the base light attributes
    UsdMayaTranslatorLight::WriteLightAttrs(usdTime, primSchema, lightFn, _GetSparseValueWriter());
    // Then write the specialized attributes for spot lights
    UsdMayaTranslatorLight::WriteSpotLightAttrs(usdTime, primSchema, lightFn, _GetSparseValueWriter());
}

/// areaLight
PXRUSDMAYA_REGISTER_WRITER(areaLight, PxrUsdTranslators_AreaLightWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(areaLight, UsdLuxRectLight);

PxrUsdTranslators_AreaLightWriter::PxrUsdTranslators_AreaLightWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx)  : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
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
    // Since write() above will take care of any animation on the light's
    // transform, we only want to proceed here if:
    // - We are at the default time and NO attributes on the shape are animated.
    //    OR
    // - We are at a non-default time and some attribute on the shape IS animated.
    if (usdTime.IsDefault() == _HasAnimCurves()) {
        return;
    }
    UsdLuxRectLight primSchema(_usdPrim);
    MFnAreaLight lightFn(GetDagPath(), &status);
    if (!status) {
        // Do this just to get the print
        CHECK_MSTATUS(status);
        return;
    }
    
    // First write the base light attributes
    UsdMayaTranslatorLight::WriteLightAttrs(usdTime, primSchema, lightFn, _GetSparseValueWriter());
    // Then write the specialized attributes for spot lights
    UsdMayaTranslatorLight::WriteAreaLightAttrs(usdTime, primSchema, lightFn, _GetSparseValueWriter());
}



PXR_NAMESPACE_CLOSE_SCOPE
