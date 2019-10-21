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
#include "lightWriter.h"

#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/translators/translatorRfMLight.h>
#include <mayaUsd/fileio/utils/adaptor.h>

#include <pxr/usd/usdLux/shapingAPI.h>

#include <maya/MFnPointLight.h>
#include <maya/MFnSpotLight.h>

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

// The typedef below is needed because PXRUSDMAYA_REGISTER_WRITER
// needs it's second param to be something that can be inserted into
// a c++ name
#define DEFINE_MAYA_LIGHT_WRITER(mayaLightTypeName, MayaLightMFn, UsdLuxType)            \
    typedef PxrUsdTranslators_LightWriter<MayaLightMFn, UsdLuxType>                      \
        PxrUsdTranslators_LightWriter_##MayaLightMFn##_##UsdLuxType;                     \
    PXRUSDMAYA_REGISTER_WRITER(                                                          \
        mayaLightTypeName, PxrUsdTranslators_LightWriter_##MayaLightMFn##_##UsdLuxType); \
    PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(mayaLightTypeName, UsdLuxType);                   \
    template <>                                                                          \
    bool PxrUsdTranslators_LightWriter<MayaLightMFn, UsdLuxType>::writeLightAttrs(       \
        const UsdTimeCode& usdTime, UsdLuxType& primSchema, MayaLightMFn& lightFn)

DEFINE_MAYA_LIGHT_WRITER(pointLight, MFnPointLight, UsdLuxSphereLight)
{
    return writeSphereLightAttrs(usdTime, primSchema, lightFn);
}

DEFINE_MAYA_LIGHT_WRITER(spotLight, MFnSpotLight, UsdLuxSphereLight)
{
    if (!writeSphereLightAttrs(usdTime, primSchema, lightFn)) {
        return false;
    }

    MStatus status;

    auto shapingAPI = UsdLuxShapingAPI::Apply(_usdPrim);

    const double dropOff = lightFn.dropOff(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    if (dropOff != 0) {
        // Not sure what formula maya uses for "dropOff", so just
        // translating straight for now
        UsdMayaWriteUtil::SetAttribute(
            shapingAPI.CreateShapingFocusAttr(),
            static_cast<float>(dropOff),
            usdTime,
            _GetSparseValueWriter());
    }

    double coneAngle = GfRadiansToDegrees(lightFn.coneAngle(&status)) * 0.5;
    CHECK_MSTATUS_AND_RETURN(status, false);
    double penumbraAngle = GfRadiansToDegrees(lightFn.penumbraAngle(&status));
    CHECK_MSTATUS_AND_RETURN(status, false);

    float cutoff = static_cast<float>(coneAngle + penumbraAngle);
    float softness = static_cast<float>(cutoff == 0 ? 0 : penumbraAngle / cutoff);

    UsdMayaWriteUtil::SetAttribute(
        shapingAPI.CreateShapingConeAngleAttr(), cutoff, usdTime, _GetSparseValueWriter());
    if (softness != 0) {
        UsdMayaWriteUtil::SetAttribute(
            shapingAPI.CreateShapingConeSoftnessAttr(), softness, usdTime, _GetSparseValueWriter());
    }

    return true;
}

// TODO: move into a plugin
// Renderman specific lights

PXRUSDMAYA_DEFINE_WRITER(PxrAovLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrCylinderLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrDiskLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrDistantLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrDomeLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrEnvDayLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrMeshLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrRectLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXRUSDMAYA_DEFINE_WRITER(PxrSphereLight, args, context)
{
    return UsdMayaTranslatorRfMLight::Write(args, context);
}

PXR_NAMESPACE_CLOSE_SCOPE
