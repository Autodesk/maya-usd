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
#include <mayaUsd/utils/converter.h>

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
    // Since write() above will take care of any animation on the light's
    // transform, we only want to proceed here if:
    // - We are at the default time and NO attributes on the shape are animated.
    //    OR
    // - We are at a non-default time and some attribute on the shape IS animated.
    if (usdTime.IsDefault() == _HasAnimCurves()) {
        return;
    }
    UsdLuxDistantLight  primSchema(_usdPrim);
    MFnDirectionalLight lightFn(GetDagPath(), &status);
    if (!status) {
        // Do this just to get the print
        CHECK_MSTATUS(status);
        return;
    }

    // First write the base light attributes
    UsdMayaTranslatorLight::WriteLightAttrs(
        usdTime, primSchema.LightAPI(), lightFn, _GetSparseValueWriter());
    // Then write the specialized attributes for directional lights
    UsdMayaTranslatorLight::WriteDirectionalLightAttrs(
        usdTime, primSchema, lightFn, _GetSparseValueWriter());
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
    // Since write() above will take care of any animation on the light's
    // transform, we only want to proceed here if:
    // - We are at the default time and NO attributes on the shape are animated.
    //    OR
    // - We are at a non-default time and some attribute on the shape IS animated.
    if (usdTime.IsDefault() == _HasAnimCurves()) {
        return;
    }
    UsdLuxSphereLight primSchema(_usdPrim);
    MFnPointLight     lightFn(GetDagPath(), &status);
    if (!status) {
        // Do this just to get the print
        CHECK_MSTATUS(status);
        return;
    }

    // First write the base light attributes
    UsdMayaTranslatorLight::WriteLightAttrs(
        usdTime, primSchema.LightAPI(), lightFn, _GetSparseValueWriter());
    // Then write the specialized attributes for point lights
    UsdMayaTranslatorLight::WritePointLightAttrs(
        usdTime, primSchema, lightFn, _GetSparseValueWriter());
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
    // Since write() above will take care of any animation on the light's
    // transform, we only want to proceed here if:
    // - We are at the default time and NO attributes on the shape are animated.
    //    OR
    // - We are at a non-default time and some attribute on the shape IS animated.
    if (usdTime.IsDefault() == _HasAnimCurves()) {
        return;
    }
    UsdLuxSphereLight primSchema(_usdPrim);
    MFnSpotLight      lightFn(GetDagPath(), &status);
    if (!status) {
        // Do this just to get the print
        CHECK_MSTATUS(status);
        return;
    }
    // First write the base light attributes
    UsdMayaTranslatorLight::WriteLightAttrs(
        usdTime, primSchema.LightAPI(), lightFn, _GetSparseValueWriter());
    // Then write the specialized attributes for spot lights
    UsdMayaTranslatorLight::WriteSpotLightAttrs(
        usdTime, primSchema, lightFn, _GetSparseValueWriter());
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
    // Since write() above will take care of any animation on the light's
    // transform, we only want to proceed here if:
    // - We are at the default time and NO attributes on the shape are animated.
    //    OR
    // - We are at a non-default time and some attribute on the shape IS animated.
    if (usdTime.IsDefault() == _HasAnimCurves()) {
        return;
    }
    UsdLuxRectLight primSchema(_usdPrim);
    MFnAreaLight    lightFn(GetDagPath(), &status);
    if (!status) {
        // Do this just to get the print
        CHECK_MSTATUS(status);
        return;
    }

    // First write the base light attributes
    UsdMayaTranslatorLight::WriteLightAttrs(
        usdTime, primSchema.LightAPI(), lightFn, _GetSparseValueWriter());
    // Then write the specialized attributes for spot lights
    UsdMayaTranslatorLight::WriteAreaLightAttrs(
        usdTime, primSchema, lightFn, _GetSparseValueWriter());
}

#ifdef UFE_VOLUME_LIGHTS_SUPPORT
PXRUSDMAYA_REGISTER_WRITER(volumeLight, PxrUsdTranslators_VolumeLightWriter);

PxrUsdTranslators_VolumeLightWriter::PxrUsdTranslators_VolumeLightWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    MPlug   plug = depNodeFn.findPlug("lightShape");
    VtValue lightShape
        = UsdMayaWriteUtil::GetVtValue(plug, MayaUsd::Converter::getUsdTypeName(plug), false);
    if (lightShape.IsEmpty()) {
        return;
    }

    if (lightShape == 1) {
        UsdLuxDomeLight domeLight = UsdLuxDomeLight::Define(GetUsdStage(), GetUsdPath());
        _usdPrim = domeLight.GetPrim();
    } else {
        // Both cylinder and disk lights have the light shape as 2
        MPlug   plug2 = depNodeFn.findPlug("faceAxis");
        VtValue faceAxis
            = UsdMayaWriteUtil::GetVtValue(plug2, MayaUsd::Converter::getUsdTypeName(plug2), false);

        // A way to tell if the volume Light is a cylinder or a disk is where the major axis is
        // Cylinder light will have the major axis along the x-axis
        // Disk light will emit light along the negative z-axis
        if (faceAxis == 0) {
            UsdLuxCylinderLight cylinderLight
                = UsdLuxCylinderLight::Define(GetUsdStage(), GetUsdPath());
            _usdPrim = cylinderLight.GetPrim();

        } else {
            UsdLuxDiskLight diskLight = UsdLuxDiskLight::Define(GetUsdStage(), GetUsdPath());
            _usdPrim = diskLight.GetPrim();
        }
    }

    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdLuxCylinderLight at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }
}

/* virtual */
void PxrUsdTranslators_VolumeLightWriter::Write(const UsdTimeCode& usdTime)
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
    if (_usdPrim.IsA<UsdLuxCylinderLight>()) {
        UsdLuxCylinderLight primSchema(_usdPrim);
        MFnVolumeLight      lightFn(GetDagPath(), &status);
        if (!status) {
            // Do this just to get the print
            CHECK_MSTATUS(status);
            return;
        }
        // First write the base light attributes
        UsdMayaTranslatorLight::WriteLightAttrs(
            usdTime, primSchema.LightAPI(), lightFn, _GetSparseValueWriter());
        // Then write the specialized attributes for spot lights
        UsdMayaTranslatorLight::WriteCylinderLightAttrs(
            usdTime, primSchema, lightFn, _GetSparseValueWriter());

    } else if (_usdPrim.IsA<UsdLuxDiskLight>()) {
        UsdLuxDiskLight primSchema(_usdPrim);
        MFnVolumeLight  lightFn(GetDagPath(), &status);
        if (!status) {
            // Do this just to get the print
            CHECK_MSTATUS(status);
            return;
        }
        // First write the base light attributes
        UsdMayaTranslatorLight::WriteLightAttrs(
            usdTime, primSchema.LightAPI(), lightFn, _GetSparseValueWriter());
        // Then write the specialized attributes for spot lights
        UsdMayaTranslatorLight::WriteDiskLightAttrs(
            usdTime, primSchema, lightFn, _GetSparseValueWriter());

    } else if (_usdPrim.IsA<UsdLuxDomeLight>()) {
        UsdLuxDomeLight primSchema(_usdPrim);
        MFnVolumeLight  lightFn(GetDagPath(), &status);
        if (!status) {
            // Do this just to get the print
            CHECK_MSTATUS(status);
            return;
        }
        // First write the base light attributes
        UsdMayaTranslatorLight::WriteLightAttrs(
            usdTime, primSchema.LightAPI(), lightFn, _GetSparseValueWriter());
        // Then write the specialized attributes for spot lights
        UsdMayaTranslatorLight::WriteDomeLightAttrs(
            usdTime, primSchema, lightFn, _GetSparseValueWriter());
    } else {
        return;
    }
}
#endif // UFE_VOLUME_LIGHTS_SUPPORT

PXR_NAMESPACE_CLOSE_SCOPE
