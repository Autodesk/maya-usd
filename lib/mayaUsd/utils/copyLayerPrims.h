//
// Copyright 2024 Autodesk
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
#ifndef MAYAUSD_COPY_LAYER_PRIMS_H
#define MAYAUSD_COPY_LAYER_PRIMS_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/progressBarScope.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include <map>
#include <vector>

namespace MAYAUSD_NS_DEF {

//! Options for the copyLayerPrims function.
struct CopyLayerPrimsOptions
{
    // The relationships of the prims will be followed and the destination of the
    // relations will also get copied. This is
    bool followRelationships = true;

    // Optional progress bar.
    MayaUsd::ProgressBarScope* progressBar = nullptr;
};

//! The result of the copyLayerPrims function.
struct CopyLayerPrimsResult
{
    // Map of copied source paths to destination paths.
    std::map<PXR_NS::SdfPath, PXR_NS::SdfPath> copiedPaths;

    // A map of the original destination SdfPath to
    // renamed destination SdfPath. Used after the copy is done to
    // rename relationships to a prim that was renamed.
    MayaUsd::ufe::ReplicateExtrasToUSD::RenamedPaths renamedPaths;
};

//! \brief copy the given list of paths from the source layer to the target layer.
//!
//!        Only copies the prims from the single given layer, and thus assumes
//!        that all needed information is in that single layer. The typical use case
//!        is to copy from a temporary exported layer to a destination.
//!
MAYAUSD_CORE_PUBLIC
CopyLayerPrimsResult copyLayerPrims(
    const PXR_NS::UsdStageRefPtr&       srcStage,
    const PXR_NS::SdfLayerRefPtr&       srcLayer,
    const PXR_NS::SdfPath&              srcParentPath,
    const PXR_NS::UsdStageRefPtr&       dstStage,
    const PXR_NS::SdfLayerRefPtr&       dstLayer,
    const PXR_NS::SdfPath&              dstParentPath,
    const std::vector<PXR_NS::SdfPath>& primsToCopy,
    const CopyLayerPrimsOptions&        options);

} // namespace MAYAUSD_NS_DEF

#endif
