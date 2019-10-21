//
// Copyright 2019 Pixar
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
#ifndef PXRUSDTRANSLATORS_LIGHT_WRITER_H
#define PXRUSDTRANSLATORS_LIGHT_WRITER_H

/// \file pxrUsdTranslators/lightWriter.h

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdLux/shadowAPI.h>
#include <pxr/usd/usdLux/sphereLight.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnNonExtendedLight.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Exports Maya lights to UsdLuxLight.
template <typename MayaLightMFn, typename UsdLuxSubtype>
class PxrUsdTranslators_LightWriter : public UsdMayaPrimWriter
{
public:
    PxrUsdTranslators_LightWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx)
        : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
    {
        if (!TF_VERIFY(GetDagPath().isValid())) {
            return;
        }

        UsdLuxSubtype primSchema = UsdLuxSubtype::Define(GetUsdStage(), GetUsdPath());
        if (!TF_VERIFY(
                primSchema,
                "Could not define %s at path '%s'\n",
                TfType::Find<UsdLuxSubtype>().GetTypeName().c_str(),
                GetUsdPath().GetText())) {
            return;
        }
        _usdPrim = primSchema.GetPrim();
        if (!TF_VERIFY(
                _usdPrim,
                "Could not get UsdPrim for %s at path '%s'\n",
                TfType::Find<UsdLuxSubtype>().GetTypeName().c_str(),
                primSchema.GetPath().GetText())) {
            return;
        }
    }

    void Write(const UsdTimeCode& usdTime) override
    {
        UsdMayaPrimWriter::Write(usdTime);

        // Since write() above will take care of any animation on the camera's
        // transform, we only want to proceed here if:
        // - We are at the default time and NO attributes on the shape are animated.
        //    OR
        // - We are at a non-default time and some attribute on the shape IS animated.
        if (usdTime.IsDefault() == _HasAnimCurves()) {
            return;
        }

        MStatus status;

        MayaLightMFn lightFn(GetDagPath(), &status);
        if (!status) {
            // Do this just to get the print
            CHECK_MSTATUS(status);
            return;
        }

        UsdLuxSubtype primSchema(_usdPrim);

        if (writeBaseLightAttrs(usdTime, primSchema, lightFn)) {
            writeLightAttrs(usdTime, primSchema, lightFn);
        }
    }

protected:
    bool
    writeBaseLightAttrs(const UsdTimeCode& usdTime, UsdLuxLight& primSchema, MayaLightMFn& lightFn)
    {
        // Since write() above will take care of any animation on the camera's
        // transform, we only want to proceed here if:
        // - We are at the default time and NO attributes on the shape are animated.
        //    OR
        // - We are at a non-default time and some attribute on the shape IS animated.
        if (usdTime.IsDefault() == _HasAnimCurves()) {
            return true;
        }

        MStatus status;

        const float intensity = lightFn.intensity(&status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetIntensityAttr(), intensity, usdTime, _GetSparseValueWriter());

        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetNormalizeAttr(), true, usdTime, _GetSparseValueWriter());

        const MColor color = lightFn.color(&status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetColorAttr(),
            GfVec3f(color.r, color.g, color.b),
            usdTime,
            _GetSparseValueWriter());

        // We use shadows if either ray-trace or depth-map shadows are
        // enabled
        bool useShadows = lightFn.useRayTraceShadows();
        if (std::is_base_of<MFnNonExtendedLight, MayaLightMFn>()) {
            if (!useShadows) {
                useShadows = lightFn.useDepthMapShadows();
            }
        }

        if (useShadows) {
            auto shadowAPI = UsdLuxShadowAPI::Apply(_usdPrim);
            UsdMayaWriteUtil::SetAttribute(
                shadowAPI.CreateShadowEnableAttr(), true, usdTime, _GetSparseValueWriter());

            const MColor shadowColor = lightFn.shadowColor(&status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            UsdMayaWriteUtil::SetAttribute(
                shadowAPI.CreateShadowColorAttr(),
                GfVec3f(shadowColor.r, shadowColor.g, shadowColor.b),
                usdTime,
                _GetSparseValueWriter());
        }

        return true;
    }

    // This isn't needed by all specializations, but wasn't any other
    // good place to put this code...
    bool writeSphereLightAttrs(
        const UsdTimeCode&   usdTime,
        UsdLuxSphereLight&   primSchema,
        MFnNonExtendedLight& lightFn)
    {
        MStatus status;

        const float radius = lightFn.shadowRadius(&status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetRadiusAttr(), radius, usdTime, _GetSparseValueWriter());

        if (radius == 0.0f) {
            UsdMayaWriteUtil::SetAttribute(
                primSchema.GetTreatAsPointAttr(), true, usdTime, _GetSparseValueWriter());
        }
        return true;
    }

    // default implementation does nothing
    bool
    writeLightAttrs(const UsdTimeCode& usdTime, UsdLuxSubtype& primSchema, MayaLightMFn& lightFn)
    {
        return true;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
