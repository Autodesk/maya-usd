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
#ifndef USDUFE_MERGEPRIMS_H
#define USDUFE_MERGEPRIMS_H

#include <usdUfe/base/api.h>
#include <usdUfe/utils/mergePrimsOptions.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/common.h>

namespace USDUFE_NS_DEF {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  merges prims starting at a source path from a source layer and stage to a destination.
/// \param  srcStage the stage containing the modified prims.
/// \param  srcLayer the layer at which to examine the modified prims.
/// \param  srcPath the path to the modified starting prims.
/// \param  dstStage the stage containing the baseline prims that receive the modifications.
/// \param  dstLayer the layer containing the baseline prims that receive the modifications.
/// \param  dstPath the path to the baseline prims that receive the modifications.
/// \param  options merging options.
/// \return true if the merge was successful.
//----------------------------------------------------------------------------------------------------------------------
USDUFE_PUBLIC
bool mergePrims(
    const PXR_NS::UsdStageRefPtr& srcStage,
    const PXR_NS::SdfLayerRefPtr& srcLayer,
    const PXR_NS::SdfPath&        srcPath,
    const PXR_NS::UsdStageRefPtr& dstStage,
    const PXR_NS::SdfLayerRefPtr& dstLayer,
    const PXR_NS::SdfPath&        dstPath,
    const MergePrimsOptions&      options);

//! \brief merge prims with default options, but no verbosity.
USDUFE_PUBLIC
bool mergePrims(
    const PXR_NS::UsdStageRefPtr& srcStage,
    const PXR_NS::SdfLayerRefPtr& srcLayer,
    const PXR_NS::SdfPath&        srcPath,
    const PXR_NS::UsdStageRefPtr& dstStage,
    const PXR_NS::SdfLayerRefPtr& dstLayer,
    const PXR_NS::SdfPath&        dstPath);

} // namespace USDUFE_NS_DEF

#endif // USDUFE_MERGEPRIMS_H
