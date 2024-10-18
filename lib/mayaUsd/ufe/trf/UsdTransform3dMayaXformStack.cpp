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
#include "UsdTransform3dMayaXformStack.h"

#include <mayaUsd/fileio/utils/xformStack.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/ufe/trf/RotationUtils.h>

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/UfeNotifGuard.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/ufe/trf/UsdSetXformOpUndoableCommandBase.h>
#include <usdUfe/ufe/trf/UsdTransform3dUndoableCommands.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/undo/UsdUndoableItem.h>
#include <usdUfe/utils/editRouterContext.h>

#include <pxr/base/gf/math.h>

#include <maya/MEulerRotation.h>
#include <maya/MGlobal.h>

#include <cstring>
#include <functional>
#include <map>
#include <typeinfo>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

using BaseUndoableCommand = Ufe::BaseUndoableCommand;
using OpFunc = std::function<UsdGeomXformOp(const BaseUndoableCommand&, UsdUfe::UsdUndoableItem&)>;

using MayaUsd::ufe::UsdTransform3dMayaXformStack;

// Type traits for GfVec precision.
template <class V> struct OpPrecision
{
    static UsdGeomXformOp::Precision precision;
};

template <>
UsdGeomXformOp::Precision OpPrecision<GfVec3f>::precision = UsdGeomXformOp::PrecisionFloat;

template <>
UsdGeomXformOp::Precision OpPrecision<GfVec3d>::precision = UsdGeomXformOp::PrecisionDouble;

VtValue getValue(const PXR_NS::UsdAttribute& attr, const UsdTimeCode& time)
{
    VtValue value;
    attr.Get(&value, time);
    return value;
}

// This utility function is used to avoid the TF_VERIFY message thrown up
// when GetAttribute() is called with an empty token.
PXR_NS::UsdAttribute getUsdPrimAttribute(const UsdPrim& prim, const TfToken& attrName)
{
    return !attrName.IsEmpty() ? prim.GetAttribute(attrName) : PXR_NS::UsdAttribute();
}

// UsdMayaXformStack::FindOpIndex() requires an inconvenient isInvertedTwin
// argument, various rotate transform op equivalences in a separate
// UsdMayaXformStack::IsCompatibleType().  Just roll our own op name to
// Maya transform stack index position.
const std::unordered_map<TfToken, UsdTransform3dMayaXformStack::OpNdx, TfToken::HashFunctor>
    gOpNameToNdx {
        { TfToken("xformOp:translate"), UsdTransform3dMayaXformStack::NdxTranslate },
        // Note: this matches the USD common xformOp name.
        { TfToken("xformOp:translate:pivot"), UsdTransform3dMayaXformStack::NdxPivot },
        { TfToken("xformOp:translate:rotatePivotTranslate"),
          UsdTransform3dMayaXformStack::NdxRotatePivotTranslate },
        { TfToken("xformOp:translate:rotatePivot"), UsdTransform3dMayaXformStack::NdxRotatePivot },
        { TfToken("xformOp:rotateX"), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateY"), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateZ"), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateXYZ"), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateXZY"), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateYXZ"), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateYZX"), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateZXY"), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateZYX"), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:orient"), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateXYZ:rotateAxis"), UsdTransform3dMayaXformStack::NdxRotateAxis },
        { TfToken("!invert!xformOp:translate:rotatePivot"),
          UsdTransform3dMayaXformStack::NdxRotatePivotInverse },
        { TfToken("xformOp:translate:scalePivotTranslate"),
          UsdTransform3dMayaXformStack::NdxScalePivotTranslate },
        { TfToken("xformOp:translate:scalePivot"), UsdTransform3dMayaXformStack::NdxScalePivot },
        { TfToken("xformOp:transform:shear"), UsdTransform3dMayaXformStack::NdxShear },
        { TfToken("xformOp:scale"), UsdTransform3dMayaXformStack::NdxScale },
        { TfToken("!invert!xformOp:translate:scalePivot"),
          UsdTransform3dMayaXformStack::NdxScalePivotInverse },
        // Note: this matches the USD common xformOp name.
        { TfToken("!invert!xformOp:translate:pivot"),
          UsdTransform3dMayaXformStack::NdxPivotInverse }
    };

bool isAlmostZero(const Ufe::Vector3d& value)
{
    static const double epsilon = 0.0001;
    return PXR_NS::GfIsClose(0., value.x(), epsilon) && PXR_NS::GfIsClose(0., value.y(), epsilon)
        && PXR_NS::GfIsClose(0., value.z(), epsilon);
}

bool isThreeAxisRotation(TfToken attrName)
{
    static const std::set<TfToken> threeAxisNames({ TfToken("xformOp:rotateXYZ"),
                                                    TfToken("xformOp:rotateXZY"),
                                                    TfToken("xformOp:rotateYXZ"),
                                                    TfToken("xformOp:rotateYZX"),
                                                    TfToken("xformOp:rotateZXY"),
                                                    TfToken("xformOp:rotateZYX") });
    return threeAxisNames.find(attrName) != threeAxisNames.end();
}

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

bool setXformOpOrder(const UsdGeomXformable& xformable)
{
    // Simply adding a transform op appends to the op order vector.  Therefore,
    // after addition, we must sort the ops to preserve Maya transform stack
    // ordering.  Use the Maya transform stack indices to add to a map, then
    // simply traverse the map to obtain the transform ops in order.
    std::map<UsdTransform3dMayaXformStack::OpNdx, UsdGeomXformOp> orderedOps;
    bool                                                          resetsXformStack = false;
    auto oldOrder = xformable.GetOrderedXformOps(&resetsXformStack);
    for (const auto& op : oldOrder) {
        auto ndx = gOpNameToNdx.at(op.GetOpName());
        orderedOps[ndx] = op;
    }

    // Set the transform op order attribute.
    std::vector<UsdGeomXformOp> newOrder;
    newOrder.reserve(oldOrder.size());
    for (const auto& orderedOp : orderedOps) {
        const auto& op = orderedOp.second;
        newOrder.emplace_back(op);
    }

    return xformable.SetXformOpOrder(newOrder, resetsXformStack);
}

using NextTransform3dFn = std::function<Ufe::Transform3d::Ptr()>;

bool hasValidSuffix(const std::vector<UsdGeomXformOp>& xformOps)
{
    TF_FOR_ALL(iter, xformOps)
    {
        const UsdGeomXformOp& xformOp = *iter;
        auto                  ndx = gOpNameToNdx.find(xformOp.GetName());
        if (ndx == gOpNameToNdx.end())
            return false;
    }
    return true;
}

Ufe::Transform3d::Ptr
createTransform3d(const Ufe::SceneItem::Ptr& item, NextTransform3dFn nextTransform3dFn)
{
    auto usdItem = downcast(item);
    if (!usdItem) {
        return nullptr;
    }

    // If the prim isn't transformable, can't create a Transform3d interface
    // for it.
    UsdGeomXformable xformSchema(usdItem->prim());
    if (!xformSchema) {
        return nullptr;
    }
    bool resetsXformStack = false;
    auto xformOps = xformSchema.GetOrderedXformOps(&resetsXformStack);

    // Early out: if there are no transform ops yet, it's a match.
    if (xformOps.empty()) {
        return UsdTransform3dMayaXformStack::create(usdItem);
    }

    // reject tokens not in gOpNameToNdx
    if (!hasValidSuffix(xformOps))
        return nextTransform3dFn();

    // If the prim supports the Maya transform stack, create a Maya transform
    // stack interface for it, otherwise delegate to the next handler in the
    // chain of responsibility.
    auto stackOps = UsdMayaXformStack::MayaStack().MatchingSubstack(xformOps);

    return stackOps.empty() ? nextTransform3dFn() : UsdTransform3dMayaXformStack::create(usdItem);
}

// Helper class to factor out common code for translate, rotate, scale
// undoable commands.
//
// We must do a careful dance due to historic reasons and the way Maya handle
// interactive commands:
//
//     - These commands can be wrapped inside other commands which may
//       use their own UsdUndoBlock. In particular, we must not try to
//       undo an attribute creation if it was not yet created.
//
//     - Maya can call undo and set-value before first executing the
//       command. In particular, when using manipualtion tools, Maya
//       will usually do loops of undo/set-value/execute, thus beginning
//       by undoing a command that was never executed.
//
//     - As a general rule, when undoing, we want to remove any attributes
//       that were created when first executed.
//
//     - When redoing some commands after an undo, Maya will update the
//       value to be set with an incorrect value when operating in object
//       space, which must be ignored.
//
// Those things are what the prepare-op/recreate-op/remove-op functions are
// aimed to support. Also, we must only capture the initial value the first
// time thevalue is modified, to support both the inital undo/set-value and
// avoid losing the initial value on repeat set-value.
class UsdTRSUndoableCmdBase : public UsdUfe::UsdSetXformOpUndoableCommandBase
{
public:
    UsdTRSUndoableCmdBase(
        const VtValue&     newOpValue,
        const Ufe::Path&   path,
        OpFunc             opFunc,
        const UsdTimeCode& writeTime)
        : UsdUfe::UsdSetXformOpUndoableCommandBase(newOpValue, path, writeTime)
        , _op()
        , _opFunc(std::move(opFunc))
    {
    }

protected:
    void createOpIfNeeded(UsdUfe::UsdUndoableItem& undoableItem) override
    {
        if (_op)
            return;

        _op = _opFunc(*this, undoableItem);
    }

    void setValue(const VtValue& v, const UsdTimeCode& writeTime) override
    {
        if (!_op)
            return;

        if (v.IsEmpty())
            return;

        auto attr = _op.GetAttr();
        if (!attr)
            return;

        attr.Set(v, writeTime);
    }

    VtValue getValue(const UsdTimeCode& readTime) const override
    {
        if (!_op)
            return {};

        auto attr = _op.GetAttr();
        if (!attr)
            return {};

        VtValue value;
        attr.Get(&value, readTime);
        return value;
    }

private:
    UsdGeomXformOp _op;
    OpFunc         _opFunc;
};

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdSetXformOpUndoableCommandBase, UsdTRSUndoableCmdBase);

// UsdRotatePivotTranslateUndoableCmd uses hard-coded USD common transform API
// single pivot attribute name, not reusable.
template <class V> class UsdVecOpUndoableCmd : public UsdTRSUndoableCmdBase
{
public:
    UsdVecOpUndoableCmd(
        const V&           v,
        const Ufe::Path&   path,
        OpFunc             opFunc,
        const UsdTimeCode& writeTime)
        : UsdTRSUndoableCmdBase(VtValue(v), path, opFunc, writeTime)
    {
    }

    // Executes the command by setting the translation onto the transform op.
    bool set(double x, double y, double z) override
    {
        UsdUfe::OperationEditRouterContext editContext(
            UsdUfe::EditRoutingTokens->RouteTransform, getPrim());

        VtValue v;
        v = V(x, y, z);
        updateNewValue(v);
        return true;
    }
};

class UsdRotateOpUndoableCmd : public UsdTRSUndoableCmdBase
{
public:
    UsdRotateOpUndoableCmd(
        const GfVec3f&                                  r,
        const Ufe::Path&                                path,
        OpFunc                                          opFunc,
        UsdTransform3dMayaXformStack::CvtRotXYZToAttrFn cvt,
        const UsdTimeCode&                              writeTime)
        : UsdTRSUndoableCmdBase(VtValue(r), path, opFunc, writeTime)
        , _cvtRotXYZToAttr(cvt)
    {
    }

    // Executes the command by setting the rotation onto the transform op.
    bool set(double x, double y, double z) override
    {
        UsdUfe::OperationEditRouterContext editContext(
            UsdUfe::EditRoutingTokens->RouteTransform, getPrim());

        VtValue v;
        v = _cvtRotXYZToAttr(x, y, z);
        updateNewValue(v);
        return true;
    }

private:
    // Convert from UFE RotXYZ rotation to a value for the transform op.
    UsdTransform3dMayaXformStack::CvtRotXYZToAttrFn _cvtRotXYZToAttr;
};

struct SceneItemHolder
{
    SceneItemHolder(const BaseUndoableCommand& cmd)
    {
        _sceneItem = downcast(cmd.sceneItem());
        if (!_sceneItem) {
            throw std::runtime_error("Cannot transform invalid scene item");
        }
    }

    UsdUfe::UsdSceneItem& item() const { return *_sceneItem; }

private:
    UsdUfe::UsdSceneItem::Ptr _sceneItem;
};

} // namespace

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdTransform3dBase, UsdTransform3dMayaXformStack);

UsdTransform3dMayaXformStack::UsdTransform3dMayaXformStack(const UsdUfe::UsdSceneItem::Ptr& item)
    : UsdUfe::UsdTransform3dBase(item)
    , _xformable(prim())
    , _needPivotConversion(isPivotConversionNeeded())
{
    if (!TF_VERIFY(_xformable)) {
        throw std::runtime_error("Invalid scene item for transform stack");
    }
}

bool UsdTransform3dMayaXformStack::isPivotConversionNeeded() const
{
    // Note: USD and Maya use different pivots: USD has a single pivot that is used
    //       for both translation and scale, while Maya has separate ones. When working
    //       in this Maya transform stack mode, the USD pivot affects the position of
    //       the manipulators, so we need to convert it to a Maya-style pivot.
    //       Otherwise, prim with USD-style pivot won't work with the "center pivot"
    //       command. They would also not work well with the universal manipulator.
    TfToken pivotName
        = UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, UsdGeomTokens->pivot);
    auto pivotAttr = prim().GetAttribute(pivotName);
    if (!pivotAttr)
        return false;

    if (!pivotAttr.HasAuthoredValue())
        return false;

    Ufe::Vector3d pivot = getVector3d<GfVec3f>(pivotName);
    if (isAlmostZero(pivot))
        return false;

    return true;
}

void UsdTransform3dMayaXformStack::convertToMayaPivotIfNeeded()
{
    if (!_needPivotConversion)
        return;

    // Note: must reset flag immediately because we call functions that would trigger
    //       conversion again, resulting in infinite recursion.
    _needPivotConversion = false;

    // Extract and clear the USD common pivot. The exiesting pivot can be authored
    // with any precision, so we need to convert it if needed.
    GfVec3f commonPivotValue;
    {
        TfToken pivotName
            = UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, UsdGeomTokens->pivot);
        UsdAttribute pivotAttr = prim().GetAttribute(pivotName);

        VtValue currentValue;
        if (!pivotAttr.Get(&currentValue, getTime(path())))
            return;

        if (currentValue.IsHolding<GfVec3f>()) {
            commonPivotValue = currentValue.UncheckedGet<GfVec3f>();
        } else if (currentValue.IsHolding<GfVec3d>()) {
            auto val = currentValue.UncheckedGet<GfVec3d>();
            commonPivotValue.Set(val[0], val[1], val[2]);
        } else if (currentValue.IsHolding<GfVec3h>()) {
            auto val = currentValue.UncheckedGet<GfVec3h>();
            commonPivotValue.Set(val[0], val[1], val[2]);
        } else {
            commonPivotValue.Set(0, 0, 0);
        }
        pivotAttr.Set(GfVec3f(0, 0, 0));
    }

    // Adjust possibly existing Maya rotate pivot by the common pivot.
    // Note: must explicitly qualify the call to rotatePivot because
    //       the overload on this class hides the other.
    {
        Ufe::Vector3d currentPivotValue = rotatePivot();
        Ufe::Transform3d::rotatePivot(
            currentPivotValue.x() + commonPivotValue[0],
            currentPivotValue.y() + commonPivotValue[1],
            currentPivotValue.z() + commonPivotValue[2]);
    }

    // Adjust possibly existing Maya scale pivot by the common pivot.
    // Note: must explicitly qualify the call to scalePivot because
    //       the overload on this class hides the other.
    {
        Ufe::Vector3d currentPivotValue = scalePivot();
        Ufe::Transform3d::scalePivot(
            currentPivotValue.x() + commonPivotValue[0],
            currentPivotValue.y() + commonPivotValue[1],
            currentPivotValue.z() + commonPivotValue[2]);
    }
}

/* static */
UsdTransform3dMayaXformStack::Ptr
UsdTransform3dMayaXformStack::create(const UsdUfe::UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdTransform3dMayaXformStack>(item);
}

Ufe::Vector3d UsdTransform3dMayaXformStack::translation() const
{
    return getVector3d<GfVec3d>(
        UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, getTRSOpSuffix()));
}

Ufe::Vector3d UsdTransform3dMayaXformStack::rotation() const
{
    if (!hasOp(NdxRotate)) {
        return Ufe::Vector3d(0, 0, 0);
    }
    UsdGeomXformOp r = getOp(NdxRotate);
    TF_DEV_AXIOM(r);
    if (!r.GetAttr().HasValue()) {
        return Ufe::Vector3d(0, 0, 0);
    }

    CvtRotXYZFromAttrFn cvt = getCvtRotXYZFromAttrFn(r.GetOpName());
    return cvt(getValue(r.GetAttr(), getTime(path())));
}

Ufe::Vector3d UsdTransform3dMayaXformStack::scale() const
{
    if (!hasOp(NdxScale)) {
        return Ufe::Vector3d(1, 1, 1);
    }
    UsdGeomXformOp s = getOp(NdxScale);
    TF_DEV_AXIOM(s);
    if (!s.GetAttr().HasValue()) {
        return Ufe::Vector3d(1, 1, 1);
    }

    GfVec3f v;
    s.Get(&v, getTime(path()));
    return UsdUfe::toUfe(v);
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::translateCmd(double x, double y, double z)
{
    return setVector3dCmd(
        GfVec3d(x, y, z),
        UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, getTRSOpSuffix()),
        getTRSOpSuffix());
}

bool UsdTransform3dMayaXformStack::isFallback() const { return false; }

Ufe::RotateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::rotateCmd(double x, double y, double z)
{
    convertToMayaPivotIfNeeded();

    UsdGeomXformOp op;
    TfToken        attrName;
    bool           hasRotate = hasOp(NdxRotate);
    if (hasRotate) {
        op = getOp(NdxRotate);
        attrName = op.GetOpName();
    }

    // Rotation is special because there might already be a single-axis rotation
    // attribute and we would fail to rotate on all three axis if we used it.
    // Translation and scaling do no have these single-axis attributes, this is
    // specific to rotations. (Why, oh why?)
    //
    // Detect that the attribute is single-axis and use a new three-axis attribute
    // instead in this situation.
    //
    // OTOH, we must not do this when using the fallback implementation, which derives
    // from this class and reuse its code. That is because the fallback implementation
    // wants to preserve the original ops and add new ones after, so we must let it do
    // its intended job.
    if (!isThreeAxisRotation(attrName) && !isFallback()) {
        attrName = TfToken("xformOp:rotateXYZ");
        hasRotate = false;
    }

    // Return null command if the attribute edit is not allowed.
    if (!isAttributeEditAllowed(attrName)) {
        return nullptr;
    }

    // If there is no rotate transform op, we will create a RotXYZ.
    GfVec3f           v(x, y, z);
    CvtRotXYZToAttrFn cvt = hasRotate ? getCvtRotXYZToAttrFn(op.GetOpName()) : toXYZ;

    auto f
        = OpFunc([attrName, opSuffix = getTRSOpSuffix(), setXformOpOrderFn = getXformOpOrderFn()](
                     const BaseUndoableCommand& cmd, UsdUfe::UsdUndoableItem& undoableItem) {
              SceneItemHolder usdSceneItem(cmd);

              auto attr = getUsdPrimAttribute(usdSceneItem.item().prim(), attrName);
              if (attr) {
                  return UsdGeomXformOp(attr);
              } else {
                  UsdUfe::UsdUndoBlock undoBlock(&undoableItem);

                  // Use notification guard, otherwise will generate one notification
                  // for the xform op add, and another for the reorder.
                  UsdUfe::InTransform3dChange guard(cmd.path());
                  UsdGeomXformable            xformable(usdSceneItem.item().prim());

                  auto r = xformable.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, opSuffix);
                  if (!r) {
                      throw std::runtime_error("Cannot add rotation transform operation");
                  }
                  if (!setXformOpOrderFn(xformable)) {
                      throw std::runtime_error("Cannot set rotation transform operation");
                  }

                  return r;
              }
          });

    return std::make_shared<UsdRotateOpUndoableCmd>(
        v, path(), std::move(f), cvt, UsdTimeCode::Default());
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3dMayaXformStack::scaleCmd(double x, double y, double z)
{
    convertToMayaPivotIfNeeded();

    UsdGeomXformOp op;
    TfToken        attrName;
    if (hasOp(NdxScale)) {
        op = getOp(NdxScale);
        attrName = op.GetOpName();
    }

    // Return null command if the attribute edit is not allowed.
    if (!isAttributeEditAllowed(attrName)) {
        return nullptr;
    }

    GfVec3f v(x, y, z);
    auto    f
        = OpFunc([attrName, opSuffix = getTRSOpSuffix(), setXformOpOrderFn = getXformOpOrderFn()](
                     const BaseUndoableCommand& cmd, UsdUfe::UsdUndoableItem& undoableItem) {
              SceneItemHolder usdSceneItem(cmd);

              auto attr = getUsdPrimAttribute(usdSceneItem.item().prim(), attrName);
              if (attr) {
                  return UsdGeomXformOp(attr);
              } else {
                  UsdUfe::UsdUndoBlock undoBlock(&undoableItem);

                  UsdUfe::InTransform3dChange guard(cmd.path());
                  UsdGeomXformable            xformable(usdSceneItem.item().prim());

                  auto s = xformable.AddScaleOp(UsdGeomXformOp::PrecisionFloat, opSuffix);
                  if (!s) {
                      throw std::runtime_error("Cannot add scaling transform operation");
                  }
                  if (!setXformOpOrderFn(xformable)) {
                      throw std::runtime_error("Cannot set scaling transform operation");
                  }

                  return s;
              }
          });

    return std::make_shared<UsdVecOpUndoableCmd<GfVec3f>>(
        v, path(), std::move(f), UsdTimeCode::Default());
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::rotatePivotCmd(double x, double y, double z)
{
    convertToMayaPivotIfNeeded();

    return pivotCmd(getOpSuffix(NdxRotatePivot), x, y, z);
}

Ufe::Vector3d UsdTransform3dMayaXformStack::rotatePivot() const
{
    Ufe::Vector3d mayaPivot = getVector3d<GfVec3f>(
        UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, getOpSuffix(NdxRotatePivot)));

    if (_needPivotConversion) {
        Ufe::Vector3d commonPivot = getVector3d<GfVec3f>(
            UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, UsdGeomTokens->pivot));

        mayaPivot = Ufe::Vector3d(
            commonPivot.x() + mayaPivot.x(),
            commonPivot.y() + mayaPivot.y(),
            commonPivot.z() + mayaPivot.z());
    }

    return mayaPivot;
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::scalePivotCmd(double x, double y, double z)
{
    convertToMayaPivotIfNeeded();

    return pivotCmd(getOpSuffix(NdxScalePivot), x, y, z);
}

Ufe::Vector3d UsdTransform3dMayaXformStack::scalePivot() const
{
    Ufe::Vector3d mayaPivot = getVector3d<GfVec3f>(
        UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, getOpSuffix(NdxScalePivot)));

    if (_needPivotConversion) {
        Ufe::Vector3d commonPivot = getVector3d<GfVec3f>(
            UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, UsdGeomTokens->pivot));

        mayaPivot = Ufe::Vector3d(
            commonPivot.x() + mayaPivot.x(),
            commonPivot.y() + mayaPivot.y(),
            commonPivot.z() + mayaPivot.z());
    }

    return mayaPivot;
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::translateRotatePivotCmd(double x, double y, double z)
{
    convertToMayaPivotIfNeeded();

    auto opSuffix = getOpSuffix(NdxRotatePivotTranslate);
    auto attrName = UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, opSuffix);
    return setVector3dCmd(GfVec3f(x, y, z), attrName, opSuffix);
}

Ufe::Vector3d UsdTransform3dMayaXformStack::rotatePivotTranslation() const
{
    return getVector3d<GfVec3f>(UsdGeomXformOp::GetOpName(
        UsdGeomXformOp::TypeTranslate, getOpSuffix(NdxRotatePivotTranslate)));
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::translateScalePivotCmd(double x, double y, double z)
{
    convertToMayaPivotIfNeeded();

    auto opSuffix = getOpSuffix(NdxScalePivotTranslate);
    auto attrName = UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, opSuffix);
    return setVector3dCmd(GfVec3f(x, y, z), attrName, opSuffix);
}

Ufe::Vector3d UsdTransform3dMayaXformStack::scalePivotTranslation() const
{
    return getVector3d<GfVec3f>(UsdGeomXformOp::GetOpName(
        UsdGeomXformOp::TypeTranslate, getOpSuffix(NdxScalePivotTranslate)));
}

template <class V>
Ufe::Vector3d UsdTransform3dMayaXformStack::getVector3d(const TfToken& attrName) const
{
    // If the attribute doesn't exist or have a value yet, return a zero vector.
    auto attr = prim().GetAttribute(attrName);
    if (!attr || !attr.HasValue()) {
        return Ufe::Vector3d(0, 0, 0);
    }

    V v;
    UsdGeomXformOp(attr).Get(&v, getTime(path()));
    return UsdUfe::toUfe(v);
}

template <class V>
Ufe::SetVector3dUndoableCommand::Ptr UsdTransform3dMayaXformStack::setVector3dCmd(
    const V&       v,
    const TfToken& attrName,
    const TfToken& opSuffix)
{
    // Return null command if the attribute edit is not allowed.
    if (!isAttributeEditAllowed(attrName)) {
        return nullptr;
    }

    auto setXformOpOrderFn = getXformOpOrderFn();
    auto f = OpFunc(
        // MAYA-108612: generalized lambda capture below is incorrect with
        // gcc 6.3.1 on Linux.  Call to getXformOpOrderFn() is non-virtual;
        // work around by calling in function body.  PPT, 11-Jan-2021.
        // [opSuffix, setXformOpOrderFn = getXformOpOrderFn(), v](const BaseUndoableCommand&
        // cmd) {
        [attrName, opSuffix, setXformOpOrderFn](
            const BaseUndoableCommand& cmd, UsdUfe::UsdUndoableItem& undoableItem) {
            SceneItemHolder usdSceneItem(cmd);

            auto attr = getUsdPrimAttribute(usdSceneItem.item().prim(), attrName);
            if (attr) {
                return UsdGeomXformOp(attr);
            } else {
                UsdUfe::UsdUndoBlock undoBlock(&undoableItem);

                UsdUfe::InTransform3dChange guard(cmd.path());
                UsdGeomXformable            xformable(usdSceneItem.item().prim());
                auto op = xformable.AddTranslateOp(OpPrecision<V>::precision, opSuffix);
                if (!op) {
                    throw std::runtime_error("Cannot add translation transform operation");
                }
                if (!setXformOpOrderFn(xformable)) {
                    throw std::runtime_error("Cannot set translation transform operation");
                }
                return op;
            }
        });

    return std::make_shared<UsdVecOpUndoableCmd<V>>(
        v, path(), std::move(f), UsdTimeCode::Default());
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::pivotCmd(const TfToken& pvtOpSuffix, double x, double y, double z)
{
    auto pvtAttrName = UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, pvtOpSuffix);

    // Return null command if the attribute edit is not allowed.
    if (!isAttributeEditAllowed(pvtAttrName)) {
        return nullptr;
    }

    GfVec3f v(x, y, z);
    auto    f = OpFunc([pvtAttrName, pvtOpSuffix, setXformOpOrderFn = getXformOpOrderFn()](
                        const BaseUndoableCommand& cmd, UsdUfe::UsdUndoableItem& undoableItem) {
        SceneItemHolder usdSceneItem(cmd);

        auto attr = usdSceneItem.item().prim().GetAttribute(pvtAttrName);
        if (attr) {
            auto attr = usdSceneItem.item().prim().GetAttribute(pvtAttrName);
            return UsdGeomXformOp(attr);
        } else {
            // Without a notification guard each operation (each transform op
            // addition, setting the attribute value, and setting the transform
            // op order) will notify.  Observers would see an object in an
            // inconsistent state, especially after pivot is added but before
            // its inverse is added --- this does not match the Maya transform
            // stack.  Use of SdfChangeBlock is discouraged when calling USD
            // APIs above Sdf, so use our own guard.

            UsdUfe::UsdUndoBlock        undoBlock(&undoableItem);
            UsdUfe::InTransform3dChange guard(cmd.path());
            UsdGeomXformable            xformable(usdSceneItem.item().prim());
            auto p = xformable.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, pvtOpSuffix);

            auto pInv = xformable.AddTranslateOp(
                UsdGeomXformOp::PrecisionFloat, pvtOpSuffix, /* isInverseOp */ true);
            if (!(p && pInv)) {
                throw std::runtime_error("Cannot add translation transform operation");
            }
            if (!setXformOpOrderFn(xformable)) {
                throw std::runtime_error("Cannot set translation transform operation");
            }
            return p;
        }
    });

    return std::make_shared<UsdVecOpUndoableCmd<GfVec3f>>(
        v, path(), std::move(f), UsdTimeCode::Default());
}

Ufe::SetMatrix4dUndoableCommand::Ptr
UsdTransform3dMayaXformStack::setMatrixCmd(const Ufe::Matrix4d& m)
{
    convertToMayaPivotIfNeeded();

    // Note: UsdSetMatrix4dUndoableCommand uses separate calls to translate, rotate and scale,
    //       so check those 3 attributes.
    const TfToken attrs[]
        = { TfToken("xformOp:translate"), TfToken("xformOp:rotateXYZ"), TfToken("xformOp:scale") };
    if (!isAttributeEditAllowed(attrs, 3))
        return nullptr;

    return std::make_shared<UsdUfe::UsdSetMatrix4dUndoableCommand>(path(), m);
}

UsdTransform3dMayaXformStack::SetXformOpOrderFn
UsdTransform3dMayaXformStack::getXformOpOrderFn() const
{
    return setXformOpOrder;
}

std::map<UsdTransform3dMayaXformStack::OpNdx, UsdGeomXformOp>
UsdTransform3dMayaXformStack::getOrderedOps() const
{
    std::map<OpNdx, UsdGeomXformOp> orderedOps;
    bool                            resetsXformStack = false;
    auto                            ops = _xformable.GetOrderedXformOps(&resetsXformStack);
    for (const auto& op : ops) {
        auto ndx = gOpNameToNdx.at(op.GetOpName());
        orderedOps[ndx] = op;
    }
    return orderedOps;
}

bool UsdTransform3dMayaXformStack::hasOp(OpNdx ndx) const
{
    auto orderedOps = getOrderedOps();
    return orderedOps.find(ndx) != orderedOps.end();
}

UsdGeomXformOp UsdTransform3dMayaXformStack::getOp(OpNdx ndx) const
{
    auto orderedOps = getOrderedOps();
    return orderedOps.at(ndx);
}

TfToken UsdTransform3dMayaXformStack::getOpSuffix(OpNdx ndx) const
{
    static std::unordered_map<OpNdx, TfToken> opSuffix
        = { { NdxRotatePivotTranslate, UsdMayaXformStackTokens->rotatePivotTranslate },
            { NdxRotatePivot, UsdMayaXformStackTokens->rotatePivot },
            { NdxRotateAxis, UsdMayaXformStackTokens->rotateAxis },
            { NdxScalePivotTranslate, UsdMayaXformStackTokens->scalePivotTranslate },
            { NdxScalePivot, UsdMayaXformStackTokens->scalePivot },
            { NdxShear, UsdMayaXformStackTokens->shear } };
    return opSuffix.at(ndx);
}

TfToken UsdTransform3dMayaXformStack::getTRSOpSuffix() const { return TfToken(); }

UsdTransform3dMayaXformStack::CvtRotXYZFromAttrFn
UsdTransform3dMayaXformStack::getCvtRotXYZFromAttrFn(const TfToken& opName) const
{
    static std::unordered_map<TfToken, CvtRotXYZFromAttrFn, TfToken::HashFunctor> cvt = {
        { TfToken("xformOp:rotateX"), fromX },     { TfToken("xformOp:rotateY"), fromY },
        { TfToken("xformOp:rotateZ"), fromZ },     { TfToken("xformOp:rotateXYZ"), fromXYZ },
        { TfToken("xformOp:rotateXZY"), fromXZY }, { TfToken("xformOp:rotateYXZ"), fromYXZ },
        { TfToken("xformOp:rotateYZX"), fromYZX }, { TfToken("xformOp:rotateZXY"), fromZXY },
        { TfToken("xformOp:rotateZYX"), fromZYX }, { TfToken("xformOp:orient"), nullptr }
    }; // FIXME, unsupported.

    return cvt.at(opName);
}

UsdTransform3dMayaXformStack::CvtRotXYZToAttrFn
UsdTransform3dMayaXformStack::getCvtRotXYZToAttrFn(const TfToken& opName) const
{
    static std::unordered_map<TfToken, CvtRotXYZToAttrFn, TfToken::HashFunctor> cvt = {
        { TfToken("xformOp:rotateX"), toX },     { TfToken("xformOp:rotateY"), toY },
        { TfToken("xformOp:rotateZ"), toZ },     { TfToken("xformOp:rotateXYZ"), toXYZ },
        { TfToken("xformOp:rotateXZY"), toXZY }, { TfToken("xformOp:rotateYXZ"), toYXZ },
        { TfToken("xformOp:rotateYZX"), toYZX }, { TfToken("xformOp:rotateZXY"), toZXY },
        { TfToken("xformOp:rotateZYX"), toZYX }, { TfToken("xformOp:orient"), nullptr }
    }; // FIXME, unsupported.

    return cvt.at(opName);
}

//------------------------------------------------------------------------------
// UsdTransform3dMayaXformStackHandler
//------------------------------------------------------------------------------

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::Transform3dHandler, UsdTransform3dMayaXformStackHandler);

UsdTransform3dMayaXformStackHandler::UsdTransform3dMayaXformStackHandler(
    const Ufe::Transform3dHandler::Ptr& nextHandler)
    : Ufe::Transform3dHandler()
    , _nextHandler(nextHandler)
{
}

/*static*/
UsdTransform3dMayaXformStackHandler::Ptr
UsdTransform3dMayaXformStackHandler::create(const Ufe::Transform3dHandler::Ptr& nextHandler)
{
    return std::make_shared<UsdTransform3dMayaXformStackHandler>(nextHandler);
}

Ufe::Transform3d::Ptr
UsdTransform3dMayaXformStackHandler::transform3d(const Ufe::SceneItem::Ptr& item) const
{
    return createTransform3d(
        item, [&]() { return _nextHandler ? _nextHandler->transform3d(item) : nullptr; });
}

Ufe::Transform3d::Ptr UsdTransform3dMayaXformStackHandler::editTransform3d(
    const Ufe::SceneItem::Ptr&      item,
    const Ufe::EditTransform3dHint& hint) const
{
    // MAYA-109190: Moved the IsInstanceProxy() check here since it was causing the
    // camera framing not properly be applied.
    //
    // HS January 15, 2021: After speaking with Pierre, there is a more robust solution to move
    // this check entirely from here.

    // According to USD docs, editing scene description via instance proxies and their
    // properties is not allowed.
    // https://graphics.pixar.com/usd/docs/api/_usd__page__scenegraph_instancing.html#Usd_ScenegraphInstancing_InstanceProxies
    auto usdItem = downcast(item);
    if (usdItem->prim().IsInstanceProxy()) {
        MGlobal::displayError(
            MString("Authoring to the descendant of an instance [")
            + MString(usdItem->prim().GetName().GetString().c_str()) + MString("] is not allowed. ")
            + MString("Please mark 'instanceable=false' to author edits to instance proxies."));
        return nullptr;
    }

    std::string errMsg;
    if (!UsdUfe::isEditTargetLayerModifiable(usdItem->prim().GetStage(), &errMsg)) {
        MGlobal::displayError(errMsg.c_str());
        return nullptr;
    }

    return createTransform3d(
        item, [&]() { return _nextHandler ? _nextHandler->editTransform3d(item, hint) : nullptr; });
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
