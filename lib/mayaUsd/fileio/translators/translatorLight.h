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
#ifndef PXRUSDMAYA_TRANSLATOR_LIGHT_H
#define PXRUSDMAYA_TRANSLATOR_LIGHT_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/primWriterArgs.h>
#include <mayaUsd/fileio/primWriterContext.h>
#include <mayaUsd/fileio/utils/writeUtil.h>

#include <pxr/pxr.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/light.h>
#include <pxr/usd/usdLux/rectLight.h>
#include <pxr/usd/usdLux/sphereLight.h>

#include <maya/MFnAreaLight.h>
#include <maya/MFnDirectionalLight.h>
#include <maya/MFnPointLight.h>
#include <maya/MFnSpotLight.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Provides helper functions for translating to/from UsdLux
struct UsdMayaTranslatorLight
{
    /// Exports the UsdLuxLight schema attributes from a generic Maya MFnLight.
    /// This function should be called when exporting to any of the specialized light schemas.
    /// Return true if all the parameters were exported properly.
    MAYAUSD_CORE_PUBLIC
    static bool WriteLightAttrs(
        const UsdTimeCode&         usdTime,
        UsdLuxLight&               usdLight,
        MFnLight&                  mayaLight,
        UsdUtilsSparseValueWriter* valueWriter = nullptr);

    /// Exports Maya's directional light attributes using UsdLuxDistantLight schema
    MAYAUSD_CORE_PUBLIC
    static bool WriteDirectionalLightAttrs(
        const UsdTimeCode&         usdTime,
        UsdLuxDistantLight&        usdLight,
        MFnDirectionalLight&       mayaLight,
        UsdUtilsSparseValueWriter* valueWriter = nullptr);

    /// Exports Maya's point light attributes using UsdLuxSphereLight schema
    MAYAUSD_CORE_PUBLIC
    static bool WritePointLightAttrs(
        const UsdTimeCode&         usdTime,
        UsdLuxSphereLight&         usdLight,
        MFnPointLight&             mayaLight,
        UsdUtilsSparseValueWriter* valueWriter = nullptr);

    /// Exports Maya's spot light attributes using UsdLuxSphereLight and UsdLuxShapingAPI schemas
    MAYAUSD_CORE_PUBLIC
    static bool WriteSpotLightAttrs(
        const UsdTimeCode&         usdTime,
        UsdLuxSphereLight&         usdLight,
        MFnSpotLight&              mayaLight,
        UsdUtilsSparseValueWriter* valueWriter = nullptr);

    /// Exports Maya's area light attributes using UsdLuxRectLight schema
    MAYAUSD_CORE_PUBLIC
    static bool WriteAreaLightAttrs(
        const UsdTimeCode&         usdTime,
        UsdLuxRectLight&           usdLight,
        MFnAreaLight&              mayaLight,
        UsdUtilsSparseValueWriter* valueWriter = nullptr);

    /// Import a UsdLuxLight schema as a corresponding Maya light.
    /// Return true if the maya light was properly created and imported
    MAYAUSD_CORE_PUBLIC
    static bool Read(const UsdMayaPrimReaderArgs& args, UsdMayaPrimReaderContext& context);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
