//
// Copyright 2016 Pixar
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
#ifndef PXRUSDMAYA_SHADING_MODE_EXPORTER_H
#define PXRUSDMAYA_SHADING_MODE_EXPORTER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/shading/shadingModeExporterContext.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usdShade/material.h>

#include <functional>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaShadingModeExporter
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaShadingModeExporter();
    MAYAUSD_CORE_PUBLIC
    virtual ~UsdMayaShadingModeExporter();

    MAYAUSD_CORE_PUBLIC
    void DoExport(
        UsdMayaWriteJobContext&                  writeJobContext,
        const UsdMayaUtil::MDagPathMap<SdfPath>& dagPathToUsdMap);

    /// Called once, before any exports are started.
    ///
    /// Because it is called before the per-shading-engine loop, the
    /// shadingEngine in the passed UsdMayaShadingModeExportContext will be a
    /// null MObject.
    MAYAUSD_CORE_PUBLIC
    virtual void PreExport(UsdMayaShadingModeExportContext* /* context */) {};

    /// Called inside of a loop, per-shading-engine
    MAYAUSD_CORE_PUBLIC
    virtual void Export(
        const UsdMayaShadingModeExportContext& context,
        UsdShadeMaterial* const                mat,
        SdfPathSet* const                      boundPrimPaths)
        = 0;

    /// Called once, after Export is called for all shading engines.
    ///
    /// Because it is called after the per-shading-engine loop, the
    /// shadingEngine in the passed UsdMayaShadingModeExportContext will be a
    /// null MObject.
    MAYAUSD_CORE_PUBLIC
    virtual void PostExport(const UsdMayaShadingModeExportContext& /* context */) {};
};

using UsdMayaShadingModeExporterPtr = std::shared_ptr<UsdMayaShadingModeExporter>;
using UsdMayaShadingModeExporterCreator
    = std::function<std::shared_ptr<UsdMayaShadingModeExporter>()>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
