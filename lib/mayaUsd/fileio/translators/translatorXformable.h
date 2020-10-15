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
#ifndef PXRUSDMAYA_TRANSLATOR_XFORMABLE_H
#define PXRUSDMAYA_TRANSLATOR_XFORMABLE_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/pxr.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Provides helper functions for reading UsdGeomXformable.
struct UsdMayaTranslatorXformable
{
    /// \brief reads xform attributes from \p xformable and converts them into
    /// maya transform values.
    MAYAUSD_CORE_PUBLIC
    static void Read(
        const UsdGeomXformable&      xformable,
        MObject                      mayaNode,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext*    context);

    /// \brief Convenince function for decomposing \p usdMatrix.
    MAYAUSD_CORE_PUBLIC
    static bool ConvertUsdMatrixToComponents(
        const GfMatrix4d& usdMatrix,
        GfVec3d*          trans,
        GfVec3d*          rot,
        GfVec3d*          scale);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
