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
#include "UsdTransform3dReadImpl.h"

#include <mayaUsd/ufe/Utils.h>

#include <pxr/base/tf/stringUtils.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_NOT_MOVE_OR_COPY(UsdTransform3dReadImpl);

UsdTransform3dReadImpl::UsdTransform3dReadImpl(const UsdUfe::UsdSceneItem::Ptr& item)
    : fItem(item)
    , fPrim(item->prim())
{
}

Ufe::Matrix4d UsdTransform3dReadImpl::matrix() const
{
    GfMatrix4d       m(1);
    UsdGeomXformable xformable(prim());
    if (xformable) {
        bool unused;
        auto ops = xformable.GetOrderedXformOps(&unused);
        if (!UsdGeomXformable::GetLocalTransformation(&m, ops, getTime(path()))) {
            std::string msg = TfStringPrintf(
                "Local transformation computation for prim %s failed.", prim().GetPath().GetText());
            throw std::runtime_error(msg.c_str());
        }
    }

    return toUfe(m);
}

Ufe::Matrix4d UsdTransform3dReadImpl::segmentInclusiveMatrix() const
{
    UsdGeomXformCache xformCache(getTime(path()));
    return toUfe(xformCache.GetLocalToWorldTransform(fPrim));
}

Ufe::Matrix4d UsdTransform3dReadImpl::segmentExclusiveMatrix() const
{
    UsdGeomXformCache xformCache(getTime(path()));
    return toUfe(xformCache.GetParentToWorldTransform(fPrim));
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
