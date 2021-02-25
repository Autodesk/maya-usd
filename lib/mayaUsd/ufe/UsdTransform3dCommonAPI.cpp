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

#include <mayaUsd/ufe/UsdUndoableInteractiveCommand.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usdGeom/xformCache.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

class UsdTranslateUndoableCmd : public UsdUndoableInteractiveCommand<Ufe::TranslateUndoableCommand>
{
public:
    UsdTranslateUndoableCmd(const UsdSceneItem::Ptr& item, const UsdTimeCode& time)
        : UsdUndoableInteractiveCommand<Ufe::TranslateUndoableCommand>(item->path())
        , _time(time)
        , _commonAPI(item->prim())
    {
        GfVec3d                              t;
        GfVec3f                              r, s, pvt;
        UsdGeomXformCommonAPI::RotationOrder rotOrder;

        if (!_commonAPI.GetXformVectorsByAccumulation(&t, &r, &s, &pvt, &rotOrder, time)) {
            TF_FATAL_ERROR(
                "Cannot read common API transform values for prim %s",
                item->prim().GetPath().GetText());
        }

        _t = t;
    }

    // Executes the command by setting the translation onto the transform op.
    bool set(double x, double y, double z) override
    {
        _t = GfVec3d(x, y, z);
        execute();
        return true;
    }

protected:
    void executeUndoBlock() override { _commonAPI.SetTranslate(_t, _time); }

private:
    const UsdTimeCode     _time;
    UsdGeomXformCommonAPI _commonAPI;
    GfVec3d               _t;
};

class UsdRotateUndoableCmd : public UsdUndoableInteractiveCommand<Ufe::RotateUndoableCommand>
{
public:
    UsdRotateUndoableCmd(const UsdSceneItem::Ptr& item, const UsdTimeCode& time)
        : UsdUndoableInteractiveCommand<Ufe::RotateUndoableCommand>(item->path())
        , _time(time)
        , _commonAPI(item->prim())
    {
        GfVec3d                              t;
        GfVec3f                              r, s, pvt;
        UsdGeomXformCommonAPI::RotationOrder rotOrder;

        if (!_commonAPI.GetXformVectorsByAccumulation(&t, &r, &s, &pvt, &rotOrder, time)) {
            TF_FATAL_ERROR(
                "Cannot read common API transform values for prim %s",
                item->prim().GetPath().GetText());
        }

        _r = r;
    }

    // Executes the command by setting the rotation onto the transform op.
    bool set(double x, double y, double z) override
    {
        _r = GfVec3f(x, y, z);
        execute();
        return true;
    }

protected:
    void executeUndoBlock() override
    {
        _commonAPI.SetRotate(_r, UsdGeomXformCommonAPI::RotationOrderXYZ, _time);
    }

private:
    const UsdTimeCode     _time;
    UsdGeomXformCommonAPI _commonAPI;
    GfVec3f               _r;
};

class UsdScaleUndoableCmd : public UsdUndoableInteractiveCommand<Ufe::ScaleUndoableCommand>
{

public:
    UsdScaleUndoableCmd(const UsdSceneItem::Ptr& item, const UsdTimeCode& time)
        : UsdUndoableInteractiveCommand<Ufe::ScaleUndoableCommand>(item->path())
        , _time(time)
        , _commonAPI(item->prim())
    {
        GfVec3d                              t;
        GfVec3f                              r, s, pvt;
        UsdGeomXformCommonAPI::RotationOrder rotOrder;

        if (!_commonAPI.GetXformVectorsByAccumulation(&t, &r, &s, &pvt, &rotOrder, time)) {
            TF_FATAL_ERROR(
                "Cannot read common API transform values for prim %s",
                item->prim().GetPath().GetText());
        }

        _s = s;
    }

    // Executes the command by setting the rotation onto the transform op.
    bool set(double x, double y, double z) override
    {
        _s = GfVec3f(x, y, z);
        execute();
        return true;
    }

protected:
    void executeUndoBlock() override { _commonAPI.SetScale(_s, _time); }

private:
    const UsdTimeCode     _time;
    UsdGeomXformCommonAPI _commonAPI;
    GfVec3f               _s;
};

class UsdTranslatePivotUndoableCmd : public UsdUndoableInteractiveCommand<Ufe::TranslateUndoableCommand>
{
public:
    UsdTranslatePivotUndoableCmd(const UsdSceneItem::Ptr& item, const UsdTimeCode& time)
        : UsdUndoableInteractiveCommand<Ufe::TranslateUndoableCommand>(item->path())
        , _time(time)
        , _commonAPI(item->prim())
    {
        GfVec3d                              t;
        GfVec3f                              r, s, pvt;
        UsdGeomXformCommonAPI::RotationOrder rotOrder;

        if (!_commonAPI.GetXformVectorsByAccumulation(&t, &r, &s, &pvt, &rotOrder, time)) {
            TF_FATAL_ERROR(
                "Cannot read common API transform values for prim %s",
                item->prim().GetPath().GetText());
        }

        _pvt = pvt;
    }

    // Executes the command by setting the translation onto the transform op.
    bool set(double x, double y, double z) override
    {
        _pvt = GfVec3f(x, y, z);
        execute();
        return true;
    }

protected:
    void executeUndoBlock() override { _commonAPI.SetPivot(_pvt, _time); }

private:
    const UsdTimeCode     _time;
    UsdGeomXformCommonAPI _commonAPI;
    GfVec3f               _pvt;
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
    return std::make_shared<UsdTranslateUndoableCmd>(usdSceneItem(), UsdTimeCode::Default());
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3dCommonAPI::rotateCmd(double x, double y, double z)
{
    return std::make_shared<UsdRotateUndoableCmd>(usdSceneItem(), UsdTimeCode::Default());
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3dCommonAPI::scaleCmd(double x, double y, double z)
{
    return std::make_shared<UsdScaleUndoableCmd>(usdSceneItem(), UsdTimeCode::Default());
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dCommonAPI::rotatePivotCmd(double x, double y, double z)
{
    return std::make_shared<UsdTranslatePivotUndoableCmd>(usdSceneItem(), UsdTimeCode::Default());
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
#if !defined(NDEBUG)
    assert(usdItem);
#endif

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
#if !defined(NDEBUG)
    if (!usdItem) {
        TF_FATAL_ERROR("Could not create common API Transform3d interface for null item.");
    }
#endif

    // If the prim supports the common transform API, create a common API
    // interface for it, otherwise delegate to the next handler in the chain of
    // responsibility.
    auto commonAPI = UsdGeomXformCommonAPI(usdItem->prim());

    return commonAPI ? UsdTransform3dCommonAPI::create(usdItem)
                     : _nextHandler->editTransform3d(item UFE_V2(, hint));
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
