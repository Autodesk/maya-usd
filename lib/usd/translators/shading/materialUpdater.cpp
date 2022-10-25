//
// Copyright 2022 Autodesk
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
#include "materialUpdater.h"

#include "shadingTokens.h"

#include <mayaUsd/fileio/primUpdaterRegistry.h>

#include <pxr/pxr.h>
#include <pxr/usd/usdShade/material.h>

#include <basePxrUsdPreviewSurface/usdPreviewSurface.h>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_UPDATER(
    UsdShadeMaterial,
    usdPreviewSurface,
    MaterialUpdater,
    UsdMayaPrimUpdater::Supports::Invalid);

MaterialUpdater::MaterialUpdater(
    const UsdMayaPrimUpdaterContext& context,
    const MFnDependencyNode&         depNodeFn,
    const Ufe::Path&                 path)
    : UsdMayaPrimUpdater(context, depNodeFn, path)
{
}

/* virtual */
bool MaterialUpdater::canEditAsMaya() const { return false; }

PXR_NAMESPACE_CLOSE_SCOPE
