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
#include "UsdTransform3dCommonAPI.h"

#include <mayaUsd/ufe/UsdSetXformOpUndoableCommandBase.h>
#include <mayaUsd/ufe/UsdTransform3dUndoableCommands.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usdGeom/xformCache.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

class CommonAPITranslateUndoableCmd : public UsdSetXformOpUndoableCommandBase<GfVec3d>
{
public:
    CommonAPITranslateUndoableCmd(const UsdSceneItem::Ptr& item, const UsdTimeCode& writeTime)
        : UsdSetXformOpUndoableCommandBase(item->path(), writeTime)
        , _commonAPI(item->prim())
    {
    }

    void setValue(const GfVec3d& v) override { _commonAPI.SetTranslate(v, writeTime()); }

    bool set(double x, double y, double z) override
    {
        handleSet(GfVec3d(x, y, z));
        return true;
    }

private:
    UsdGeomXformCommonAPI _commonAPI;
};

class CommonAPIRotateUndoableCmd : public UsdSetXformOpUndoableCommandBase<GfVec3f>
{
public:
    CommonAPIRotateUndoableCmd(const UsdSceneItem::Ptr& item, const UsdTimeCode& writeTime)
        : UsdSetXformOpUndoableCommandBase(item->path(), writeTime)
        , _commonAPI(item->prim())
    {
    }

    void setValue(const GfVec3f& v) override
    {
        _commonAPI.SetRotate(v, UsdGeomXformCommonAPI::RotationOrderXYZ, writeTime());
    }

    bool set(double x, double y, double z) override
    {
        handleSet(GfVec3f(x, y, z));
        return true;
    }

private:
    UsdGeomXformCommonAPI _commonAPI;
};

class CommonAPIScaleUndoableCmd : public UsdSetXformOpUndoableCommandBase<GfVec3f>
{

public:
    CommonAPIScaleUndoableCmd(const UsdSceneItem::Ptr& item, const UsdTimeCode& writeTime)
        : UsdSetXformOpUndoableCommandBase(item->path(), writeTime)
        , _commonAPI(item->prim())
    {
    }

    void setValue(const GfVec3f& v) override { _commonAPI.SetScale(v, writeTime()); }

    bool set(double x, double y, double z) override
    {
        handleSet(GfVec3f(x, y, z));
        return true;
    }

private:
    UsdGeomXformCommonAPI _commonAPI;
};

class CommonAPIPivotUndoableCmd : public UsdSetXformOpUndoableCommandBase<GfVec3f>
{
public:
    CommonAPIPivotUndoableCmd(const UsdSceneItem::Ptr& item, const UsdTimeCode& writeTime)
        : UsdSetXformOpUndoableCommandBase(item->path(), writeTime)
        , _commonAPI(item->prim())
    {
    }

    void setValue(const GfVec3f& v) override { _commonAPI.SetPivot(v, writeTime()); }

    bool set(double x, double y, double z) override
    {
        handleSet(GfVec3f(x, y, z));
        return true;
    }

private:
    UsdGeomXformCommonAPI _commonAPI;
};

} // namespace

UsdTransform3dCommonAPI::UsdTransform3dCommonAPI(const UsdSceneItem::Ptr& item)
    : UsdTransform3dBase(item)
    , _commonAPI(prim())
{
}

/* static */
UsdTransform3dCommonAPI::Ptr UsdTransform3dCommonAPI::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdTransform3dCommonAPI>(item);
}

Ufe::Vector3d UsdTransform3dCommonAPI::translation() const
{
    GfVec3d                              t;
    GfVec3f                              r, s, pvt;
    UsdGeomXformCommonAPI::RotationOrder rotOrder;

    if (!_commonAPI.GetXformVectorsByAccumulation(&t, &r, &s, &pvt, &rotOrder, getTime(path()))) {
        TF_FATAL_ERROR(
            "Cannot read common API transform values for prim %s", prim().GetPath().GetText());
    }

    return toUfe(t);
}

Ufe::Vector3d UsdTransform3dCommonAPI::rotation() const
{
    GfVec3d                              t;
    GfVec3f                              r, s, pvt;
    UsdGeomXformCommonAPI::RotationOrder rotOrder;

    if (!_commonAPI.GetXformVectorsByAccumulation(&t, &r, &s, &pvt, &rotOrder, getTime(path()))) {
        TF_FATAL_ERROR(
            "Cannot read common API transform values for prim %s", prim().GetPath().GetText());
    }

    return toUfe(r);
}

Ufe::Vector3d UsdTransform3dCommonAPI::scale() const
{
    GfVec3d                              t;
    GfVec3f                              r, s, pvt;
    UsdGeomXformCommonAPI::RotationOrder rotOrder;

    if (!_commonAPI.GetXformVectorsByAccumulation(&t, &r, &s, &pvt, &rotOrder, getTime(path()))) {
        TF_FATAL_ERROR(
            "Cannot read common API transform values for prim %s", prim().GetPath().GetText());
    }

    return toUfe(s);
}

void UsdTransform3dCommonAPI::translate(double x, double y, double z)
{
    _commonAPI.SetTranslate(GfVec3d(x, y, z), UsdTimeCode::Default());
}

void UsdTransform3dCommonAPI::rotate(double x, double y, double z)
{
    _commonAPI.SetRotate(
        GfVec3f(x, y, z), UsdGeomXformCommonAPI::RotationOrderXYZ, UsdTimeCode::Default());
}

void UsdTransform3dCommonAPI::scale(double x, double y, double z)
{
    _commonAPI.SetScale(GfVec3f(x, y, z), UsdTimeCode::Default());
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dCommonAPI::translateCmd(double x, double y, double z)
{
    if (!isAttributeEditAllowed(prim(), TfToken("xformOp:translate"))) {
        return nullptr;
    }

    return std::make_shared<CommonAPITranslateUndoableCmd>(usdSceneItem(), UsdTimeCode::Default());
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3dCommonAPI::rotateCmd(double x, double y, double z)
{
    if (!isAttributeEditAllowed(prim(), TfToken("xformOp:rotateXYZ"))) {
        return nullptr;
    }

    return std::make_shared<CommonAPIRotateUndoableCmd>(usdSceneItem(), UsdTimeCode::Default());
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3dCommonAPI::scaleCmd(double x, double y, double z)
{
    if (!isAttributeEditAllowed(prim(), TfToken("xformOp:scale"))) {
        return nullptr;
    }

    return std::make_shared<CommonAPIScaleUndoableCmd>(usdSceneItem(), UsdTimeCode::Default());
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dCommonAPI::rotatePivotCmd(double x, double y, double z)
{
    if (!isAttributeEditAllowed(prim(), TfToken("xformOp:translate:pivot"))) {
        return nullptr;
    }

    return std::make_shared<CommonAPIPivotUndoableCmd>(usdSceneItem(), UsdTimeCode::Default());
}

void UsdTransform3dCommonAPI::rotatePivot(double x, double y, double z)
{
    _commonAPI.SetPivot(GfVec3f(x, y, z), UsdTimeCode::Default());
}

Ufe::Vector3d UsdTransform3dCommonAPI::rotatePivot() const
{
    GfVec3d                              t;
    GfVec3f                              r, s, pvt;
    UsdGeomXformCommonAPI::RotationOrder rotOrder;

    if (!_commonAPI.GetXformVectorsByAccumulation(&t, &r, &s, &pvt, &rotOrder, getTime(path()))) {
        TF_FATAL_ERROR(
            "Cannot read common API transform values for prim %s", prim().GetPath().GetText());
    }

    return toUfe(pvt);
}

Ufe::SetMatrix4dUndoableCommand::Ptr UsdTransform3dCommonAPI::setMatrixCmd(const Ufe::Matrix4d& m)
{
    if (!isAttributeEditAllowed(prim(), TfToken("xformOp:translate"))
        || !isAttributeEditAllowed(prim(), TfToken("xformOp:rotateXYZ"))
        || !isAttributeEditAllowed(prim(), TfToken("xformOp:scale"))) {
        return nullptr;
    }

    return std::make_shared<UsdSetMatrix4dUndoableCommand>(path(), m);
}

//------------------------------------------------------------------------------
// UsdTransform3dCommonAPIHandler
//------------------------------------------------------------------------------

UsdTransform3dCommonAPIHandler::UsdTransform3dCommonAPIHandler(
    const Ufe::Transform3dHandler::Ptr& nextHandler)
    : Ufe::Transform3dHandler()
    , _nextHandler(nextHandler)
{
}

/*static*/
UsdTransform3dCommonAPIHandler::Ptr
UsdTransform3dCommonAPIHandler::create(const Ufe::Transform3dHandler::Ptr& nextHandler)
{
    return std::make_shared<UsdTransform3dCommonAPIHandler>(nextHandler);
}

Ufe::Transform3d::Ptr
UsdTransform3dCommonAPIHandler::transform3d(const Ufe::SceneItem::Ptr& item) const
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);

    if (!usdItem) {
        return nullptr;
    }

    // If the prim supports the common transform API, create a common API
    // interface for it, otherwise delegate to the next handler in the chain of
    // responsibility.
    auto commonAPI = UsdGeomXformCommonAPI(usdItem->prim());

    return commonAPI ? UsdTransform3dCommonAPI::create(usdItem) : _nextHandler->transform3d(item);
}

Ufe::Transform3d::Ptr UsdTransform3dCommonAPIHandler::editTransform3d(
    const Ufe::SceneItem::Ptr& item UFE_V2(, const Ufe::EditTransform3dHint& hint)) const
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);

    if (!usdItem) {
        return nullptr;
    }

    // If the prim supports the common transform API, create a common API
    // interface for it, otherwise delegate to the next handler in the chain of
    // responsibility.
    auto commonAPI = UsdGeomXformCommonAPI(usdItem->prim());

    return commonAPI ? UsdTransform3dCommonAPI::create(usdItem)
                     : _nextHandler->editTransform3d(item UFE_V2(, hint));
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
