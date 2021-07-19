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
#ifndef PXRUSDTRANSLATORS_LIGHT_WRITER_H
#define PXRUSDTRANSLATORS_LIGHT_WRITER_H

/// \file

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdLux/distantLight.h>
#include <pxr/usd/usdLux/light.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnDirectionalLight.h>
#include <maya/MString.h>

#include <set>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Exports Maya directional lights to UsdLux distant lights
class PxrUsdTranslators_DirectionalLightWriter : public UsdMayaPrimWriter
{
public:
    PxrUsdTranslators_DirectionalLightWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;
};

/// Exports Maya point lights to UsdLux sphere lights
class PxrUsdTranslators_PointLightWriter : public UsdMayaPrimWriter
{
public:
    PxrUsdTranslators_PointLightWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;
};

/// Exports Maya spot lights to UsdLux sphere lights
class PxrUsdTranslators_SpotLightWriter : public UsdMayaPrimWriter
{
public:
    PxrUsdTranslators_SpotLightWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;
};

/// Exports Maya area lights to UsdLux rect lights
class PxrUsdTranslators_AreaLightWriter : public UsdMayaPrimWriter
{
public:
    PxrUsdTranslators_AreaLightWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    void Write(const UsdTimeCode& usdTime) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
