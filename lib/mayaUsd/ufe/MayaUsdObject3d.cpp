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
#include "MayaUsdObject3d.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MayaUsdObject3d::MayaUsdObject3d(const UsdSceneItem::Ptr& item)
    : UsdUfe::UsdObject3d(item)
{
}

MayaUsdObject3d::~MayaUsdObject3d() { }

/*static*/
MayaUsdObject3d::Ptr MayaUsdObject3d::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<MayaUsdObject3d>(item);
}

PXR_NS::TfTokenVector MayaUsdObject3d::getPurposes(const Ufe::Path& path) const
{
    return getProxyShapePurposes(path);
}

void MayaUsdObject3d::adjustBBoxExtents(PXR_NS::GfBBox3d& bbox, const PXR_NS::UsdTimeCode time)
    const
{
    UsdMayaUtil::AddMayaExtents(bbox, prim(), time);
}

Ufe::BBox3d
MayaUsdObject3d::adjustAlignedBBox(const Ufe::BBox3d& bbox, const PXR_NS::UsdTimeCode time) const
{
    Ufe::BBox3d pulledBBox = getPulledPrimsBoundingBox(sceneItem()->path());
    return combineUfeBBox(bbox, pulledBBox);
}


} // namespace ufe
} // namespace MAYAUSD_NS_DEF
