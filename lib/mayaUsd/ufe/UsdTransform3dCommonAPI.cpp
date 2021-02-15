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

#include <mayaUsd/ufe/UsdUndoableCommand.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usdGeom/xformCache.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

// Extract translation, rotation, scale or pivot point from the common API transform.
void extractXform(
    const UsdSceneItem::Ptr& item,
    const UsdTimeCode&       time,
    GfVec3d&                 t,
    GfVec3f&                 r,
    GfVec3f&                 s,
    GfVec3f&                 pvt)
{
    UsdGeomXformCommonAPI commonAPI(item->prim());

    UsdGeomXformCommonAPI::RotationOrder rotOrder;
    if (!commonAPI.GetXformVectorsByAccumulation(&t, &r, &s, &pvt, &rotOrder, time)) {
        TF_FATAL_ERROR(
            "Cannot read common API transform values for prim %s",
            item->prim().GetPath().GetText());
    }
}

GfVec3d extractTranslation(const UsdSceneItem::Ptr& item, const UsdTimeCode& time)
{
    GfVec3d t;
    GfVec3f r, s, pvt;
    extractXform(item, time, t, r, s, pvt);

    return t;
}

GfVec3d extractRotation(const UsdSceneItem::Ptr& item, const UsdTimeCode& time)
{
    GfVec3d t;
    GfVec3f r, s, pvt;
    extractXform(item, time, t, r, s, pvt);

    return r;
}

GfVec3d extractScale(const UsdSceneItem::Ptr& item, const UsdTimeCode& time)
{
    GfVec3d t;
    GfVec3f r, s, pvt;
    extractXform(item, time, t, r, s, pvt);

    return s;
}

GfVec3d extractPivot(const UsdSceneItem::Ptr& item, const UsdTimeCode& time)
{
    GfVec3d t;
    GfVec3f r, s, pvt;
    extractXform(item, time, t, r, s, pvt);

    return pvt;
}

// Common base class for the translation, rotation, scale and pivot commands.
//
// Only the handleSet() function, which sets the correct part of the transform
// is done in the sub-classes.
class UsdTRSUndoableCmdBase : public UsdUndoableCmdBase<Ufe::SetVector3dUndoableCommand>
{
public:
    UsdTRSUndoableCmdBase(
        const GfVec3d&           value,
        const UsdSceneItem::Ptr& item,
        const UsdTimeCode&       time)
        : UsdUndoableCmdBase<Ufe::SetVector3dUndoableCommand>(VtValue(value), item->path(), time)
        , _commonAPI(item->prim())
    {
    }

    UsdTRSUndoableCmdBase(
        const GfVec3f&           value,
        const UsdSceneItem::Ptr& item,
        const UsdTimeCode&       time)
        : UsdUndoableCmdBase<Ufe::SetVector3dUndoableCommand>(VtValue(value), item->path(), time)
        , _commonAPI(item->prim())
    {
    }

    // Executes the command by setting the translation/rotation/scale/pivot into the value
    // that will be used when we execute the transform op.
    bool set(double x, double y, double z) override
    {
        setNewValue(VtValue(GfVec3d(x, y, z)));
        execute();
        return true;
    }

protected:
    UsdGeomXformCommonAPI _commonAPI;
};

class UsdTranslateUndoableCmd : public UsdTRSUndoableCmdBase
{
public:
    UsdTranslateUndoableCmd(const UsdSceneItem::Ptr& item, const UsdTimeCode& time)
        : UsdTRSUndoableCmdBase(extractTranslation(item, time), item, time)
    {
    }

    void handleSet(State previousState, State newState, const VtValue& v) override
    {
        const GfVec3d t(v.Get<GfVec3d>());
        _commonAPI.SetTranslate(t, writeTime());
    }
};

class UsdRotateUndoableCmd : public UsdTRSUndoableCmdBase
{
public:
    UsdRotateUndoableCmd(const UsdSceneItem::Ptr& item, const UsdTimeCode& time)
        : UsdTRSUndoableCmdBase(extractRotation(item, time), item, time)
    {
    }

    void handleSet(State previousState, State newState, const VtValue& v) override
    {
        // Note: rotations are kept as float in common API.
        const GfVec3f r(v.Get<GfVec3d>());
        _commonAPI.SetRotate(r, UsdGeomXformCommonAPI::RotationOrderXYZ, writeTime());
    }
};

class UsdScaleUndoableCmd : public UsdTRSUndoableCmdBase
{
public:
    UsdScaleUndoableCmd(const UsdSceneItem::Ptr& item, const UsdTimeCode& time)
        : UsdTRSUndoableCmdBase(extractScale(item, time), item, time)
    {
    }

    void handleSet(State previousState, State newState, const VtValue& v) override
    {
        // Note: scales are kept as float in common API.
        const GfVec3f s(v.Get<GfVec3d>());
        _commonAPI.SetScale(s, writeTime());
    }
};

class UsdTranslatePivotUndoableCmd : public UsdTRSUndoableCmdBase
{
public:
    UsdTranslatePivotUndoableCmd(const UsdSceneItem::Ptr& item, const UsdTimeCode& time)
        : UsdTRSUndoableCmdBase(extractPivot(item, time), item, time)
    {
    }

    void handleSet(State previousState, State newState, const VtValue& v) override
    {
        // Note: pivot are kept as float in common API.
        const GfVec3f pvt(v.Get<GfVec3d>());
        _commonAPI.SetPivot(pvt, writeTime());
    }
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
