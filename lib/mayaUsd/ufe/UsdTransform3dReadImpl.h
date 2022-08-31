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
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/transform3d.h>
#include <ufe/transform3dHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Read-only implementation for USD object 3D transform information.
//
// Note that all calls to specify time use the default time, but this
// could be changed to use the current time, using getTime(path()).

class MAYAUSD_CORE_PUBLIC UsdTransform3dReadImpl
{
public:
    UsdTransform3dReadImpl(const UsdSceneItem::Ptr& item);
    ~UsdTransform3dReadImpl() = default;

    // Delete the copy/move constructors assignment operators.
    UsdTransform3dReadImpl(const UsdTransform3dReadImpl&) = delete;
    UsdTransform3dReadImpl& operator=(const UsdTransform3dReadImpl&) = delete;
    UsdTransform3dReadImpl(UsdTransform3dReadImpl&&) = delete;
    UsdTransform3dReadImpl& operator=(UsdTransform3dReadImpl&&) = delete;

    // Ufe::Transform3d overrides
    const Ufe::Path&    path() const { return fItem->path(); }
    Ufe::SceneItem::Ptr sceneItem() const { return fItem; }

    inline UsdSceneItem::Ptr usdSceneItem() const { return fItem; }
    inline PXR_NS::UsdPrim   prim() const { return fPrim; }

#ifdef UFE_V2_FEATURES_AVAILABLE
    Ufe::Matrix4d matrix() const;
#endif

    Ufe::Matrix4d segmentInclusiveMatrix() const;
    Ufe::Matrix4d segmentExclusiveMatrix() const;

private:
    const UsdSceneItem::Ptr fItem;
    PXR_NS::UsdPrim         fPrim;

}; // UsdTransform3dReadImpl

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
