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
#include <mayaUsd/utils/copyLayerPrims.h>

#include <pxr/base/tf/pyResultConversions.h>

#include <boost/python/def.hpp>

using namespace boost::python;

namespace {

// Note: due to a limitation of boost::python, we cannot pass shared-pointer
//       between Python and C++. See this stack overflow answer for an explanation:
//       https://stackoverflow.com/questions/20825662/boost-python-argument-types-did-not-match-c-signature
//
//       That is why stages and layers are passed by raw C++ references and a
//       smart pointer is created on-the-fly. Otherwise, the stage you pass it
//       in Python would become invalid the call.
std::map<PXR_NS::SdfPath, PXR_NS::SdfPath> _copyLayerPrims(
    PXR_NS::UsdStage&                   srcStage,
    PXR_NS::SdfLayer&                   srcLayer,
    const PXR_NS::SdfPath&              srcParentPath,
    PXR_NS::UsdStage&                   dstStage,
    PXR_NS::SdfLayer&                   dstLayer,
    const PXR_NS::SdfPath&              dstParentPath,
    const std::vector<PXR_NS::SdfPath>& primsToCopy,
    const bool                          followRelationships)
{
    MayaUsd::CopyLayerPrimsOptions options;
    options.followRelationships = followRelationships;

    PXR_NS::UsdStageRefPtr srcStagePtr(&srcStage);
    PXR_NS::SdfLayerRefPtr srcLayerPtr(&srcLayer);
    PXR_NS::UsdStageRefPtr dstStagePtr(&dstStage);
    PXR_NS::SdfLayerRefPtr dstLayerPtr(&dstLayer);

    MayaUsd::CopyLayerPrimsResult result = MayaUsd::copyLayerPrims(
        srcStagePtr,
        srcLayerPtr,
        srcParentPath,
        dstStagePtr,
        dstLayerPtr,
        dstParentPath,
        primsToCopy,
        options);

    return result.copiedPaths;
}

std::map<PXR_NS::SdfPath, PXR_NS::SdfPath> _copyLayerPrim(
    PXR_NS::UsdStage&      srcStage,
    PXR_NS::SdfLayer&      srcLayer,
    const PXR_NS::SdfPath& srcParentPath,
    PXR_NS::UsdStage&      dstStage,
    PXR_NS::SdfLayer&      dstLayer,
    const PXR_NS::SdfPath& dstParentPath,
    const PXR_NS::SdfPath& primToCopy,
    const bool             followRelationships)
{
    return _copyLayerPrims(
        srcStage,
        srcLayer,
        srcParentPath,
        dstStage,
        dstLayer,
        dstParentPath,
        { primToCopy },
        followRelationships);
}

} // namespace

void wrapCopyLayerPrims()
{
    def("copyLayerPrim", _copyLayerPrim);
    def("copyLayerPrims", _copyLayerPrims);
}
