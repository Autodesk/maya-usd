//
// Copyright 2020 Autodesk
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
#include "UsdTransform3dBase.h"

#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

namespace {
const char* const kLocalTransformFailed = "Obtaining local transform failed for object %s";
const char* const kReadOnly = "Illegal %s call on read-only interface UsdTransform3dBase for object %s";
}

MAYAUSD_NS_DEF {
namespace ufe {

UsdTransform3dBase::UsdTransform3dBase(const UsdSceneItem::Ptr& item)
    : Transform3d(), fItem(item), fPrim(item->prim())
{}

/* static */
UsdTransform3dBase::Ptr UsdTransform3dBase::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdTransform3dBase>(item);
}

const Ufe::Path& UsdTransform3dBase::path() const
{
    return fItem->path();
}

Ufe::SceneItem::Ptr UsdTransform3dBase::sceneItem() const
{
    return fItem;
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3dBase::translateCmd(
    double /* x */, double /* y */, double /* z */)
{
    return nullptr;
}

void UsdTransform3dBase::translate(double /* x */, double /* y */, double /* z */)
{
    TF_CODING_ERROR(kReadOnly, "translate()", pathCStr());
}

Ufe::Vector3d UsdTransform3dBase::translation() const
{
    UsdGeomXformable xformable(prim());
    GfMatrix4d m;
    bool unused;
    if (!xformable.GetLocalTransformation(&m, &unused, getTime(path()))) {
        TF_WARN(kLocalTransformFailed, pathCStr());
        return Ufe::Vector3d(0, 0, 0);
    }
    return toUfe(m.ExtractTranslation());
}

Ufe::Vector3d UsdTransform3dBase::rotation() const
{
    UsdGeomXformable xformable(prim());
    GfMatrix4d m;
    bool unused;
    if (!xformable.GetLocalTransformation(&m, &unused, getTime(path()))) {
        TF_WARN(kLocalTransformFailed, pathCStr());
        return Ufe::Vector3d(0, 0, 0);
    }
    return toUfe(m.DecomposeRotation(GfVec3d::XAxis(), GfVec3d::YAxis(), GfVec3d::ZAxis()));
}

Ufe::Vector3d UsdTransform3dBase::scale() const
{
    UsdGeomXformable xformable(prim());
    GfMatrix4d m;
    bool unused;
    if (!xformable.GetLocalTransformation(&m, &unused, getTime(path()))) {
        TF_WARN(kLocalTransformFailed, pathCStr());
        return Ufe::Vector3d(1, 1, 1);
    }
    GfMatrix4d unusedR, unusedP, unusedU;
    GfVec3d    s, unusedT;
    if (!m.Factor(&unusedR, &s, &unusedU, &unusedT, &unusedP)) {
        TF_WARN("Cannot decompose local transform for %s", pathCStr());
        return Ufe::Vector3d(1, 1, 1);
    }
    
    return toUfe(s);
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3dBase::rotateCmd(
    double /* x */, double /* y */, double /* z */
)
{
    return nullptr;
}

void UsdTransform3dBase::rotate(double /* x */, double /* y */, double /* z */)
{
    TF_CODING_ERROR(kReadOnly, "rotate()", pathCStr());
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3dBase::scaleCmd(
    double /* x */, double /* y */, double /* z */
)
{
    return nullptr;
}

void UsdTransform3dBase::scale(double /* x */, double /* y */, double /* z */)
{
    TF_CODING_ERROR(kReadOnly, "scale()", pathCStr());
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3dBase::rotatePivotTranslateCmd()
{
    return nullptr;
}

void UsdTransform3dBase::rotatePivotTranslate(
    double /* x */, double /* y */, double /* z */
)
{
    TF_CODING_ERROR(kReadOnly, "rotatePivotTranslate()", pathCStr());
}

Ufe::Vector3d UsdTransform3dBase::rotatePivot() const
{
    // Called by Maya during transform editing.
    return Ufe::Vector3d(0, 0, 0);
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3dBase::scalePivotTranslateCmd()
{
    return nullptr;
}

void UsdTransform3dBase::scalePivotTranslate(double x, double y, double z)
{
    TF_CODING_ERROR(kReadOnly, "scalePivotTranslate()", pathCStr());
}

Ufe::Vector3d UsdTransform3dBase::scalePivot() const
{
    // Called by Maya during transform editing.
    return Ufe::Vector3d(0, 0, 0);
}

Ufe::Matrix4d UsdTransform3dBase::segmentInclusiveMatrix() const
{
    UsdGeomXformCache xformCache(getTime(path()));
    return toUfe(xformCache.GetLocalToWorldTransform(fPrim));
}
 
Ufe::Matrix4d UsdTransform3dBase::segmentExclusiveMatrix() const
{
    UsdGeomXformCache xformCache(getTime(path()));
    return toUfe(xformCache.GetParentToWorldTransform(fPrim));
}

const char* UsdTransform3dBase::pathCStr() const
{
    return sceneItem()->path().string().c_str();
}

//------------------------------------------------------------------------------
// UsdTransform3dBaseHandler
//------------------------------------------------------------------------------

UsdTransform3dBaseHandler::UsdTransform3dBaseHandler() 
  : Ufe::Transform3dHandler()
{}

/*static*/
UsdTransform3dBaseHandler::Ptr UsdTransform3dBaseHandler::create()
{
    return std::make_shared<UsdTransform3dBaseHandler>();
}

Ufe::Transform3d::Ptr UsdTransform3dBaseHandler::transform3d(
    const Ufe::SceneItem::Ptr& item
) const
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif

    return UsdTransform3dBase::create(usdItem);
}

} // namespace ufe
} // namespace MayaUsd
