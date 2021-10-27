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
/// MergeVerbosity level flags.

enum class MergeVerbosity
{
    None = 0,
    Same = 1 << 0,
    Differ = 1 << 1,
    Child = 1 << 2,
    Children = 1 << 3,
    Failure = 1 << 4,
    Default = Differ | Children | Failure,
};

MergeVerbosity operator|(MergeVerbosity a, MergeVerbosity b)
{
    return MergeVerbosity(uint32_t(a) | uint32_t(b));
}
MergeVerbosity operator&(MergeVerbosity a, MergeVerbosity b)
{
    return MergeVerbosity(uint32_t(a) & uint32_t(b));
}
MergeVerbosity operator^(MergeVerbosity a, MergeVerbosity b)
{
    return MergeVerbosity(uint32_t(a) ^ uint32_t(b));
}
bool contains(MergeVerbosity a, MergeVerbosity b) { return (a & b) != MergeVerbosity::None; }

//----------------------------------------------------------------------------------------------------------------------
/// \brief  merges prims starting at a source path from a source layer and stage to a destination.
/// \param  srcStage the stage containing the modified prims.
/// \param  srcLayer the layer at which to examine the modified prims.
/// \param  srcPath the path to the modified starting prims.
/// \param  dstStage the stage containing the baseline prims that receive the modifications.
/// \param  dstLayer the layer containing the baseline prims that receive the modifications.
/// \param  dstPath the path to the baseline prims that receive the modifications.
/// \param  verbosity how much info is printed about the merge in the console.
/// \return true if the merge was successful.
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool mergePrims(
    const PXR_NS::UsdStageRefPtr& srcStage,
    const PXR_NS::SdfLayerRefPtr& srcLayer,
    const PXR_NS::SdfPath&        srcPath,
    const PXR_NS::UsdStageRefPtr& dstStage,
    const PXR_NS::SdfLayerRefPtr& dstLayer,
    const PXR_NS::SdfPath&        dstPath,
    MergeVerbosity                verbosity = MergeVerbosity::Default);

} // namespace MayaUsdUtils
