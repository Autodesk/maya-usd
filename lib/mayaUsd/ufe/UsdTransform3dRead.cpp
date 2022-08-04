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
#include "UsdTransform3dRead.h"

#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdTransform3dRead::Ptr UsdTransform3dRead::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdTransform3dRead>(item);
}

UsdTransform3dRead::UsdTransform3dRead(const UsdSceneItem::Ptr& item)
    : Transform3dRead()
    , fItem(item)
    , fPrim(item->prim())
{
}

const Ufe::Path& UsdTransform3dRead::path() const { return fItem->path(); }

Ufe::SceneItem::Ptr UsdTransform3dRead::sceneItem() const { return fItem; }

Ufe::Matrix4d UsdTransform3dRead::matrix() const
{
    GfMatrix4d       m(1);
    UsdGeomXformable xformable(prim());
    if (xformable) {
        bool unused;
        auto ops = xformable.GetOrderedXformOps(&unused);
        if (!UsdGeomXformable::GetLocalTransformation(&m, ops, getTime(path()))) {
            TF_FATAL_ERROR(
                "Local transformation computation for prim %s failed.", prim().GetPath().GetText());
        }
    }

    return toUfe(m);
}

Ufe::Matrix4d UsdTransform3dRead::segmentInclusiveMatrix() const
{
    UsdGeomXformCache xformCache(getTime(path()));
    return toUfe(xformCache.GetLocalToWorldTransform(fPrim));
}

Ufe::Matrix4d UsdTransform3dRead::segmentExclusiveMatrix() const
{
    UsdGeomXformCache xformCache(getTime(path()));
    return toUfe(xformCache.GetParentToWorldTransform(fPrim));
}

//------------------------------------------------------------------------------
// UsdTransform3dReadHandler
//------------------------------------------------------------------------------

UsdTransform3dReadHandler::UsdTransform3dReadHandler(
    const Ufe::Transform3dHandler::Ptr& nextHandler)
    : Ufe::Transform3dHandler()
    , _nextHandler(nextHandler)
{
}

/* static */
UsdTransform3dReadHandler::Ptr
UsdTransform3dReadHandler::create(const Ufe::Transform3dHandler::Ptr& nextHandler)
{
    return std::make_shared<UsdTransform3dReadHandler>(nextHandler);
}

namespace {

bool isUsdScope(const UsdSceneItem& usdItem)
{
    // Test the type of the USD prim, verify it is a scope.
    return PXR_NS::UsdGeomScope(usdItem.prim()) && !PXR_NS::UsdGeomXformable(usdItem.prim());
}

} // namespace

/* override */
Ufe::Transform3d::Ptr UsdTransform3dReadHandler::transform3d(const Ufe::SceneItem::Ptr& item) const
{
    return _nextHandler->transform3d(item);
}

/* override */
Ufe::Transform3dRead::Ptr
UsdTransform3dReadHandler::transform3dRead(const Ufe::SceneItem::Ptr& item) const
{
    UsdSceneItem::Ptr usdItem = downcast(item);

    if (!usdItem || !isUsdScope(*usdItem))
        return _nextHandler->transform3dRead(item);

    return UsdTransform3dRead::create(usdItem);
}

/* override */
Ufe::Transform3d::Ptr UsdTransform3dReadHandler::editTransform3d(
    const Ufe::SceneItem::Ptr&      item,
    const Ufe::EditTransform3dHint& hint) const
{
    return _nextHandler->editTransform3d(item, hint);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
