//
// Copyright 2023 Autodesk
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
#ifndef MTLXUSDTRANSLATORS_SHADING_ASSET_H
#define MTLXUSDTRANSLATORS_SHADING_ASSET_H

#include <mayaUsd/fileio/jobs/jobArgs.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdShade/shader.h>

#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE

// Resolve an asset (for example a texture) to be imported into Maya.
bool ResolveTextureAssetPath(
    const UsdPrim&              prim,
    const UsdShadeShader&       shaderSchema,
    MFnDependencyNode&          depFn,
    const UsdMayaJobImportArgs& jobArgs);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MTLXUSDTRANSLATORS_SHADING_ASSET_H
