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
#ifndef PXRUSDTRANSLATORS_LOCATOR_WRITER_H
#define PXRUSDTRANSLATORS_LOCATOR_WRITER_H

/// \file

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE

/// A simple USD prim writer for Maya locator shape nodes.
///
/// Having this dedicated prim writer for locators ensures that we get the
/// correct resulting USD whether mergeTransformAndShape is turned on or off.
///
/// Note that there is currently no "Locator" type in USD and that Maya locator
/// nodes are exported as UsdGeomXform prims. This means that locators will not
/// currently round-trip out of Maya to USD and back because the importer is
/// not able to differentiate between Xform prims that were the result of
/// exporting Maya "transform" type nodes and those that were the result of
/// exporting Maya "locator" type nodes.
class PxrUsdTranslators_LocatorWriter : public UsdMayaPrimWriter
{
public:
    PxrUsdTranslators_LocatorWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
