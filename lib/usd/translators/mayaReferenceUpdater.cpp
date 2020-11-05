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
#include "mayaReferenceUpdater.h"

#include <mayaUsd/fileio/primUpdaterRegistry.h>
#include <mayaUsd/fileio/translators/translatorMayaReference.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd_Schemas/ALMayaReference.h>
#include <mayaUsd_Schemas/MayaReference.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdUtils/pipeline.h>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_UPDATER(
    MayaUsd_SchemasMayaReference,
    PxrUsdTranslators_MayaReferenceUpdater,
    (UsdMayaPrimUpdater::Supports::Push | UsdMayaPrimUpdater::Supports::Clear));
PXRUSDMAYA_REGISTER_UPDATER(
    MayaUsd_SchemasALMayaReference,
    PxrUsdTranslators_MayaReferenceUpdater,
    (UsdMayaPrimUpdater::Supports::Push | UsdMayaPrimUpdater::Supports::Clear));

PxrUsdTranslators_MayaReferenceUpdater::PxrUsdTranslators_MayaReferenceUpdater(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath)
    : UsdMayaPrimUpdater(depNodeFn, usdPath)
{
}

/* virtual */
bool PxrUsdTranslators_MayaReferenceUpdater::Pull(UsdMayaPrimUpdaterContext* context)
{
    const UsdPrim& usdPrim = GetUsdPrim<MayaUsd_SchemasMayaReference>(*context);
    const MObject& parentNode = GetMayaObject();

    UsdMayaTranslatorMayaReference::update(usdPrim, parentNode);

    return true;
}

/* virtual */
void PxrUsdTranslators_MayaReferenceUpdater::Clear(UsdMayaPrimUpdaterContext* context)
{
    const MObject& parentNode = GetMayaObject();

    UsdMayaTranslatorMayaReference::UnloadMayaReference(parentNode);
}

PXR_NAMESPACE_CLOSE_SCOPE
