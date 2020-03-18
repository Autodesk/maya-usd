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

/// \file usdMaya/translatorMesh.h

#ifndef PXRUSDMAYA_TRANSLATOR_MESH_H
#define PXRUSDMAYA_TRANSLATOR_MESH_H

#include "../../base/api.h"

#include "../primReaderArgs.h"
#include "../primReaderContext.h"

#include "pxr/pxr.h"

#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvar.h"

#include <maya/MFnMesh.h>
#include <maya/MObject.h>


PXR_NAMESPACE_OPEN_SCOPE


/// Provides helper functions for translating UsdGeomMesh prims into Maya
/// meshes.
class UsdMayaTranslatorMesh
{
    public:
        /// Creates an MFnMesh under \p parentNode from \p mesh.
        MAYAUSD_CORE_PUBLIC
        static bool Create(
                const UsdGeomMesh& mesh,
                MObject parentNode,
                const UsdMayaPrimReaderArgs& args,
                UsdMayaPrimReaderContext* context);

    private:
        static bool _AssignSubDivTagsToMesh(
                const UsdGeomMesh& primSchema,
                MObject& meshObj,
                MFnMesh& meshFn);

        static bool _AssignUVSetPrimvarToMesh(
                const UsdGeomPrimvar& primvar,
                MFnMesh& meshFn);

        static bool _AssignColorSetPrimvarToMesh(
                const UsdGeomMesh& primSchema,
                const UsdGeomPrimvar& primvar,
                MFnMesh& meshFn);

        static bool _AssignConstantPrimvarToMesh(
                const UsdGeomPrimvar& primvar,
                MFnMesh& meshFn);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
