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
#include "primActivation.h"

#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/prim.h>

namespace MAYAUSD_NS_DEF {

using namespace PXR_NS;

namespace {

void activate(
    const UsdStagePtr& stage,
    const SdfPath&     path,
    SdfPathSet&        previouslyInactive,
    SdfPathSet&        forcedActive)
{
    if (!stage)
        return;

    SdfLayerHandle sessionLayer = stage->GetSessionLayer();
    UsdEditContext editContext(stage, sessionLayer);

    // The last prefix is the path itself and we don't need to activate
    // it, so skip processing if there is only one prefix, otherwise
    // remove the last prefix since it is the path and we don't want
    // to explicitly activate it.
    SdfPathVector prefixes = path.GetPrefixes();
    if (prefixes.size() < 2)
        return;
    prefixes.pop_back();

    for (const SdfPath& prefixPath : prefixes) {
        UsdPrim prim = stage->GetPrimAtPath(prefixPath);
        if (prim.IsActive())
            continue;

        // If the prim at the path has a "active" field in the session
        // layer, then we must remember to set the opinion back to deactivated.
        // Otherwise, we must remember to clear the opinion we are authoring.
        if (sessionLayer->HasField(prefixPath, TfToken("active"))) {
            previouslyInactive.insert(prefixPath);
        } else {
            forcedActive.insert(prefixPath);
        }

        prim.SetActive(true);
    }
}

void deactivate(const UsdStagePtr& stage, SdfPathSet& previouslyInactive, SdfPathSet& forcedActive)
{
    if (!stage)
        return;

    SdfLayerHandle sessionLayer = stage->GetSessionLayer();
    UsdEditContext editContext(stage, sessionLayer);

    for (const SdfPath& path : previouslyInactive) {
        UsdPrim prim = stage->GetPrimAtPath(path);
        prim.SetActive(false);
    }

    previouslyInactive.clear();

    for (const SdfPath& path : forcedActive) {
        UsdPrim prim = stage->GetPrimAtPath(path);
        prim.ClearActive();
    }

    forcedActive.clear();
}

} // namespace

PrimActivation::PrimActivation(const UsdStagePtr& stage, const SdfPath& path)
    : _stage(stage)
{
    if (!_stage)
        throw std::runtime_error("Cannot find stage to activate prims.");

    activate(stage, path, _previouslyInactive, _forcedActive);
}

PrimActivation::PrimActivation(const Ufe::Path& path)
    : _stage(MayaUsd::ufe::getStage(path))
{
    if (!_stage)
        throw std::runtime_error("Cannot find stage to activate prims.");

    const auto    segments = path.getSegments();
    const SdfPath usdPath = (segments.size() < 2) ? SdfPath("/") : SdfPath(segments[1].string());
    activate(_stage, usdPath, _previouslyInactive, _forcedActive);
}

PrimActivation::~PrimActivation() { restore(); }

void PrimActivation::restore() { deactivate(_stage, _previouslyInactive, _forcedActive); }

} // namespace MAYAUSD_NS_DEF
