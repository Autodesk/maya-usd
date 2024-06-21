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

#include <usdUfe/ufe/Utils.h>

#include <pxr/base/tf/stringUtils.h>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_NOT_MOVE_OR_COPY(UsdTransform3dReadImpl);

UsdTransform3dReadImpl::UsdTransform3dReadImpl(const UsdSceneItem::Ptr& item)
    : _item(item)
    , _prim(item->prim())
{
}

Ufe::Matrix4d UsdTransform3dReadImpl::matrix() const
{
    PXR_NS::GfMatrix4d       m(1);
    PXR_NS::UsdGeomXformable xformable(prim());
    if (xformable) {
        bool unused;
        auto ops = xformable.GetOrderedXformOps(&unused);
        if (!PXR_NS::UsdGeomXformable::GetLocalTransformation(&m, ops, getTime(path()))) {
            std::string msg = PXR_NS::TfStringPrintf(
                "Local transformation computation for prim %s failed.", prim().GetPath().GetText());
            throw std::runtime_error(msg.c_str());
        }
    }

    return toUfe(m);
}

Ufe::Matrix4d UsdTransform3dReadImpl::segmentInclusiveMatrix() const
{
    PXR_NS::UsdGeomXformCache xformCache(getTime(path()));
    return toUfe(xformCache.GetLocalToWorldTransform(_prim));
}

Ufe::Matrix4d UsdTransform3dReadImpl::segmentExclusiveMatrix() const
{
    PXR_NS::UsdGeomXformCache xformCache(getTime(path()));
    return toUfe(xformCache.GetParentToWorldTransform(_prim));
}

} // namespace USDUFE_NS_DEF
