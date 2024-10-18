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
#ifndef USDUFE_USDTRANSFORM3DREADIMPL_H
#define USDUFE_USDTRANSFORM3DREADIMPL_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/transform3d.h>
#include <ufe/transform3dHandler.h>

namespace USDUFE_NS_DEF {

//! \brief Read-only implementation for USD object 3D transform information.
//
// Note that all calls to specify time use the default time, but this
// could be changed to use the current time, using getTime(path()).

class USDUFE_PUBLIC UsdTransform3dReadImpl
{
public:
    UsdTransform3dReadImpl(const UsdSceneItem::Ptr& item);
    ~UsdTransform3dReadImpl() = default;

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdTransform3dReadImpl);

    // Ufe::Transform3d overrides
    const Ufe::Path&    path() const { return _item->path(); }
    Ufe::SceneItem::Ptr sceneItem() const { return _item; }

    inline UsdSceneItem::Ptr usdSceneItem() const { return _item; }
    inline PXR_NS::UsdPrim   prim() const { return _prim; }

    Ufe::Matrix4d matrix() const;

    Ufe::Matrix4d segmentInclusiveMatrix() const;
    Ufe::Matrix4d segmentExclusiveMatrix() const;

private:
    const UsdSceneItem::Ptr _item;
    PXR_NS::UsdPrim         _prim;

}; // UsdTransform3dReadImpl

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDTRANSFORM3DREADIMPL_H
