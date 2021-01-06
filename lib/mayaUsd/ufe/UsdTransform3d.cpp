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
#include "UsdTransform3d.h"

#include "private/Utils.h"

#include <mayaUsd/ufe/UsdRotatePivotTranslateUndoableCommand.h>
#include <mayaUsd/ufe/UsdRotateUndoableCommand.h>
#include <mayaUsd/ufe/UsdScaleUndoableCommand.h>
#include <mayaUsd/ufe/UsdTranslateUndoableCommand.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usdGeom/xformCache.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {
Ufe::Matrix4d convertFromUsd(const GfMatrix4d& matrix)
{
    // Even though memory layout of Ufe::Matrix4d and double[4][4] are identical
    // we need to return a copy of the matrix so we cannot cast.
    double m[4][4];
    matrix.Get(m);
    Ufe::Matrix4d uMat;
    uMat.matrix = { { { { m[0][0], m[0][1], m[0][2], m[0][3] } },
                      { { m[1][0], m[1][1], m[1][2], m[1][3] } },
                      { { m[2][0], m[2][1], m[2][2], m[2][3] } },
                      { { m[3][0], m[3][1], m[3][2], m[3][3] } } } };
    return uMat;
}

Ufe::Matrix4d primToUfeXform(const UsdPrim& prim, const UsdTimeCode& time)
{
    UsdGeomXformCache xformCache(time);
    GfMatrix4d        usdMatrix = xformCache.GetLocalToWorldTransform(prim);
    Ufe::Matrix4d     xform = convertFromUsd(usdMatrix);
    return xform;
}

Ufe::Matrix4d primToUfeExclusiveXform(const UsdPrim& prim, const UsdTimeCode& time)
{
    UsdGeomXformCache xformCache(time);
    GfMatrix4d        usdMatrix = xformCache.GetParentToWorldTransform(prim);
    Ufe::Matrix4d     xform = convertFromUsd(usdMatrix);
    return xform;
}
} // namespace

UsdTransform3d::UsdTransform3d()
    : Transform3d()
{
}

UsdTransform3d::UsdTransform3d(const UsdSceneItem::Ptr& item)
    : Transform3d()
    , fItem(item)
{
}

/*static*/
UsdTransform3d::Ptr UsdTransform3d::create() { return std::make_shared<UsdTransform3d>(); }

/* static */
UsdTransform3d::Ptr UsdTransform3d::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdTransform3d>(item);
}

void UsdTransform3d::setItem(const UsdSceneItem::Ptr& item) { fItem = item; }

//------------------------------------------------------------------------------
// Ufe::Transform3d overrides
//------------------------------------------------------------------------------

const Ufe::Path& UsdTransform3d::path() const { return fItem->path(); }

Ufe::SceneItem::Ptr UsdTransform3d::sceneItem() const { return fItem; }

#ifdef UFE_V2_FEATURES_AVAILABLE
Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::translateCmd(double x, double y, double z)
{
    return UsdTranslateUndoableCommand::create(path(), x, y, z);
}
#endif

void UsdTransform3d::translate(double x, double y, double z)
{
    translateOp(prim(), fItem->path(), x, y, z);
}

Ufe::Vector3d UsdTransform3d::translation() const
{
    double        x { 0 }, y { 0 }, z { 0 };
    const TfToken xlate("xformOp:translate");
    if (prim().HasAttribute(xlate)) {
        // Initially, attribute can be created, but have no value.
        GfVec3d v;
        if (prim().GetAttribute(xlate).Get<GfVec3d>(&v, getTime(path()))) {
            x = v[0];
            y = v[1];
            z = v[2];
        }
    }
    return Ufe::Vector3d(x, y, z);
}

#ifdef UFE_V2_FEATURES_AVAILABLE
Ufe::Vector3d UsdTransform3d::rotation() const
{
    double        x { 0 }, y { 0 }, z { 0 };
    const TfToken rotXYZ("xformOp:rotateXYZ");
    if (prim().HasAttribute(rotXYZ)) {
        // Initially, attribute can be created, but have no value.
        GfVec3f v;
        if (prim().GetAttribute(rotXYZ).Get<GfVec3f>(&v, getTime(path()))) {
            x = v[0];
            y = v[1];
            z = v[2];
        }
    }
    return Ufe::Vector3d(x, y, z);
}

Ufe::Vector3d UsdTransform3d::scale() const
{
    double        x { 0 }, y { 0 }, z { 0 };
    const TfToken scaleTok("xformOp:scale");
    if (prim().HasAttribute(scaleTok)) {
        // Initially, attribute can be created, but have no value.
        GfVec3f v;
        if (prim().GetAttribute(scaleTok).Get<GfVec3f>(&v, getTime(path()))) {
            x = v[0];
            y = v[1];
            z = v[2];
        }
    }
    return Ufe::Vector3d(x, y, z);
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3d::rotateCmd(double x, double y, double z)
{
    return UsdRotateUndoableCommand::create(path(), x, y, z);
}
#endif

void UsdTransform3d::rotate(double x, double y, double z)
{
    rotateOp(prim(), fItem->path(), x, y, z);
}

#ifdef UFE_V2_FEATURES_AVAILABLE
Ufe::ScaleUndoableCommand::Ptr UsdTransform3d::scaleCmd(double x, double y, double z)
{
    return UsdScaleUndoableCommand::create(path(), x, y, z);
}

#else
Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::translateCmd()
{
    return UsdTranslateUndoableCommand::create(fItem, 0, 0, 0);
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3d::rotateCmd()
{
    return UsdRotateUndoableCommand::create(fItem, 0, 0, 0);
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3d::scaleCmd()
{
    return UsdScaleUndoableCommand::create(fItem, 1, 1, 1);
}
#endif

#ifdef UFE_V2_FEATURES_AVAILABLE
Ufe::SetMatrix4dUndoableCommand::Ptr UsdTransform3d::setMatrixCmd(const Ufe::Matrix4d& m)
{
    // TODO: HS Aug25,2020 dummy code to pass the compiler errors
    return nullptr;
}

Ufe::Matrix4d UsdTransform3d::matrix() const
{
    UsdGeomXformable xformable(prim());
    bool             unused;
    auto             ops = xformable.GetOrderedXformOps(&unused);

    GfMatrix4d m(1);
    if (!UsdGeomXformable::GetLocalTransformation(&m, ops, getTime(path()))) {
        TF_FATAL_ERROR(
            "Local transformation computation for prim %s failed.", prim().GetPath().GetText());
    }

    return convertFromUsd(m);
}
#endif

void UsdTransform3d::scale(double x, double y, double z)
{
    scaleOp(prim(), fItem->path(), x, y, z);
}

Ufe::TranslateUndoableCommand::Ptr
#ifdef UFE_V2_FEATURES_AVAILABLE
UsdTransform3d::rotatePivotCmd(double, double, double)
#else
UsdTransform3d::rotatePivotTranslateCmd()
#endif
{
    // As of 12-Oct-2020, setting rotate pivot on command creation
    // unsupported.  Use translate() method on returned command.
    return UsdRotatePivotTranslateUndoableCommand::create(
#ifdef UFE_V2_FEATURES_AVAILABLE
        fItem->path()
#else
        prim(), fItem->path(), fItem
#endif
    );
}

void
#ifdef UFE_V2_FEATURES_AVAILABLE
UsdTransform3d::rotatePivot(
#else
UsdTransform3d::rotatePivotTranslate(
#endif
    double x, double y, double z
)
{
    rotatePivotTranslateOp(prim(), path(), x, y, z);
}

Ufe::Vector3d UsdTransform3d::rotatePivot() const
{
    double        x { 0 }, y { 0 }, z { 0 };
    const TfToken xpivot("xformOp:translate:pivot");
    if (prim().HasAttribute(xpivot)) {
        // Initially, attribute can be created, but have no value.
        GfVec3f v;
        if (prim().GetAttribute(xpivot).Get<GfVec3f>(&v, getTime(path()))) {
            x = v[0];
            y = v[1];
            z = v[2];
        }
    }
    return Ufe::Vector3d(x, y, z);
}

Ufe::TranslateUndoableCommand::Ptr
#ifdef UFE_V2_FEATURES_AVAILABLE
UsdTransform3d::scalePivotCmd(double, double, double)
#else
UsdTransform3d::scalePivotTranslateCmd()
#endif
{
    return nullptr;
}

void
#ifdef UFE_V2_FEATURES_AVAILABLE
UsdTransform3d::scalePivot(
#else
UsdTransform3d::scalePivotTranslate(
#endif
    double, double, double
)
{
}

Ufe::Vector3d UsdTransform3d::scalePivot() const { return rotatePivot(); }

#ifdef UFE_V2_FEATURES_AVAILABLE
Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::translateRotatePivotCmd(double, double, double)
{
    // USD common transform API does not support rotate pivot correction.
    return nullptr;
}

Ufe::Vector3d UsdTransform3d::rotatePivotTranslation() const
{
    // USD common transform API does not support rotate pivot correction.
    return Ufe::Vector3d(0, 0, 0);
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::translateScalePivotCmd(double, double, double)
{
    // USD common transform API does not support scale pivot correction.
    return nullptr;
}

Ufe::Vector3d UsdTransform3d::scalePivotTranslation() const
{
    // USD common transform API does not support scale pivot correction.
    return Ufe::Vector3d(0, 0, 0);
}
#endif

Ufe::Matrix4d UsdTransform3d::segmentInclusiveMatrix() const
{
    return primToUfeXform(prim(), getTime(path()));
}

Ufe::Matrix4d UsdTransform3d::segmentExclusiveMatrix() const
{
    return primToUfeExclusiveXform(prim(), getTime(path()));
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
