//
// Copyright 2021 Autodesk
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
#pragma once

#include <mayaUsdUtils/Api.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/common.h>

namespace MayaUsdUtils {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  merges prims starting at a source path from a source layer and stage to a destination.
/// \param  srcStage the stage containing the modified prims.
/// \param  srcLayer the layer at which to examine the modified prims.
/// \param  srcPath the path to the modified starting prims.
/// \param  dstStage the stage containing the baseline prims that receive the modifications.
/// \param  dstLayer the layer containing the baseline prims that receive the modifications.
/// \param  dstPath the path to the baseline prims that receive the modifications.
/// \return true if the merge was successful.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool mergePrims(
    PXR_NS::UsdStageRefPtr       srcStage,
    const PXR_NS::SdfLayerRefPtr srcLayer,
    const PXR_NS::SdfPath&       srcPath,
    PXR_NS::UsdStageRefPtr       dstStage,
    PXR_NS::SdfLayerRefPtr       dstLayer,
    const PXR_NS::SdfPath&       dstPath);

} // namespace MayaUsdUtils
