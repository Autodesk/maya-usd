//
// Copyright 2018 Pixar
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
#include "locatorWriter.h"

#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_WRITER(locator, PxrUsdTranslators_LocatorWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(locator, UsdGeomXform);

PxrUsdTranslators_LocatorWriter::PxrUsdTranslators_LocatorWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    if (!TF_VERIFY(GetDagPath().isValid())) {
        return;
    }

    UsdGeomXform xformSchema = UsdGeomXform::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            xformSchema, "Could not define UsdGeomXform at path '%s'\n", GetUsdPath().GetText())) {
        return;
    }

    _usdPrim = xformSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdGeomXform at path '%s'\n",
            xformSchema.GetPath().GetText())) {
        return;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
