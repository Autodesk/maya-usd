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
#ifndef PXRUSDMAYA_TRANSLATOR_MATERIAL_H
#define PXRUSDMAYA_TRANSLATOR_MATERIAL_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderContext.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usd/usdShade/material.h>

#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Provides helper functions for reading UsdShadeMaterial
struct UsdMayaTranslatorMaterial
{
    /// Reads \p material according to the shading mode found in \p jobArguments. Some shading modes
    /// may want to know the \p boundPrim. This returns an MObject that is the Maya shadingEngine
    /// that corresponds to \p material.
    MAYAUSD_CORE_PUBLIC
    static MObject Read(
        const UsdMayaJobImportArgs& jobArguments,
        const UsdShadeMaterial&     material,
        const UsdGeomGprim&         boundPrim,
        UsdMayaPrimReaderContext*   context);

    /// Given a \p prim, assigns a material to it according to the shading mode found in
    /// \p jobArguments. This will see which UsdShadeMaterial is bound to \p prim. If the material
    /// has not been read already, it will read it. The created/retrieved shadingEngine will be
    /// assigned to \p shapeObj.
    MAYAUSD_CORE_PUBLIC
    static bool AssignMaterial(
        const UsdMayaJobImportArgs& jobArguments,
        const UsdGeomGprim&         prim,
        MObject                     shapeObj,
        UsdMayaPrimReaderContext*   context);

    /// Finds shadingEngines in the Maya scene and exports them to the USD
    /// stage contained in \p writeJobContext.
    MAYAUSD_CORE_PUBLIC
    static void ExportShadingEngines(
        UsdMayaWriteJobContext&                  writeJobContext,
        const UsdMayaUtil::MDagPathMap<SdfPath>& dagPathToUsdMap);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
