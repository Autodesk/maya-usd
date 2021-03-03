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

#include "private/UfeNotifGuard.h"
#include "private/Utils.h"

#include <mayaUsd/fileio/utils/xformStack.h>
#include <mayaUsd/ufe/RotationUtils.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/UsdUndoBlock.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <maya/MEulerRotation.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>

#include <cstring>
#include <functional>
#include <map>

namespace {

#if UFE_PREVIEW_VERSION_NUM >= 2031
using BaseUndoableCommand = Ufe::BaseUndoableCommand;
#else
using BaseUndoableCommand = Ufe::BaseTransformUndoableCommand;
#endif
using OpFunc = std::function<UsdGeomXformOp(const BaseUndoableCommand&)>;

using namespace MayaUsd::ufe;

// Type traits for GfVec precision.
template <class V> struct OpPrecision
{
    static UsdGeomXformOp::Precision precision;
};

template <>
UsdGeomXformOp::Precision OpPrecision<GfVec3f>::precision = UsdGeomXformOp::PrecisionFloat;

template <>
UsdGeomXformOp::Precision OpPrecision<GfVec3d>::precision = UsdGeomXformOp::PrecisionDouble;

VtValue getValue(const UsdAttribute& attr, const UsdTimeCode& time)
{
    VtValue value;
    attr.Get(&value, time);
    return value;
}

// UsdMayaXformStack::FindOpIndex() requires an inconvenient isInvertedTwin
// argument, various rotate transform op equivalences in a separate
// UsdMayaXformStack::IsCompatibleType().  Just roll our own op name to
// Maya transform stack index position.
const std::unordered_map<TfToken, UsdTransform3dMayaXformStack::OpNdx, TfToken::HashFunctor>
    gOpNameToNdx {
        { TfToken("xformOp:translate"), UsdTransform3dMayaXformStack::NdxTranslate },
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
          UsdTransform3dMayaXformStack::NdxScalePivotInverse }
    };

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

void setXformOpOrder(const UsdGeomXformable& xformable)
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

    xformable.SetXformOpOrder(newOrder, resetsXformStack);
}

using NextTransform3dFn = std::function<Ufe::Transform3d::Ptr()>;

Ufe::Transform3d::Ptr
createTransform3d(const Ufe::SceneItem::Ptr& item, NextTransform3dFn nextTransform3dFn)
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
    if (!usdItem) {
        TF_FATAL_ERROR(
            "Could not create Maya transform stack Transform3d interface for null item.");
    }
#endif

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

    // If the prim supports the Maya transform stack, create a Maya transform
    // stack interface for it, otherwise delegate to the next handler in the
    // chain of responsibility.
    auto stackOps = UsdMayaXformStack::MayaStack().MatchingSubstack(xformOps);

    return stackOps.empty() ? nextTransform3dFn() : UsdTransform3dMayaXformStack::create(usdItem);
}

// Class for setMatrixCmd() implementation.  UsdUndoBlock data member and
// undo() / redo() should be factored out into a future command base class.
class UsdSetMatrix4dUndoableCmd : public Ufe::SetMatrix4dUndoableCommand
{
public:
    UsdSetMatrix4dUndoableCmd(const Ufe::Path& path, const Ufe::Matrix4d& newM)
        : Ufe::SetMatrix4dUndoableCommand(path)
    {
        // Decompose new matrix to extract TRS.  Neither GfMatrix4d::Factor
        // nor GfTransform decomposition provide results that match Maya,
        // so use MTransformationMatrix.
        MMatrix m;
        std::memcpy(m[0], &newM.matrix[0][0], sizeof(double) * 16);
        MTransformationMatrix                xformM(m);
        auto                                 t = xformM.getTranslation(MSpace::kTransform);
        double                               r[3];
        double                               s[3];
        MTransformationMatrix::RotationOrder rotOrder;
        xformM.getRotation(r, rotOrder);
        xformM.getScale(s, MSpace::kTransform);
        constexpr double radToDeg = 57.295779506;

        _newT = Ufe::Vector3d(t[0], t[1], t[2]);
        _newR = Ufe::Vector3d(r[0] * radToDeg, r[1] * radToDeg, r[2] * radToDeg);
        _newS = Ufe::Vector3d(s[0], s[1], s[2]);
    }

    ~UsdSetMatrix4dUndoableCmd() override { }

    bool set(const Ufe::Matrix4d&) override
    {
        // No-op: Maya does not set matrices through interactive manipulation.
        TF_WARN("Illegal call to UsdSetMatrix4dUndoableCmd::set()");
        return true;
    }

    void execute() override
    {
        UsdUndoBlock undoBlock(&_undoableItem);

        auto t3d = Ufe::Transform3d::transform3d(sceneItem());
        t3d->translate(_newT.x(), _newT.y(), _newT.z());
        t3d->rotate(_newR.x(), _newR.y(), _newR.z());
        t3d->scale(_newS.x(), _newS.y(), _newS.z());

#if !defined(REMOVE_PR122_WORKAROUND_MAYA_109685)
        _executeCalled = true;
#endif
    }

    void undo() override { _undoableItem.undo(); }
    void redo() override
    {
#if !defined(REMOVE_PR122_WORKAROUND_MAYA_109685)
        if (!_executeCalled) {
            execute();
            return;
        }
#endif
        _undoableItem.redo();
    }

private:
#if !defined(REMOVE_PR122_WORKAROUND_MAYA_109685)
    bool _executeCalled { false };
#endif
    UsdUndoableItem _undoableItem;
    Ufe::Vector3d   _newT;
    Ufe::Vector3d   _newR;
    Ufe::Vector3d   _newS;
};

// Helper class to factor out common code for translate, rotate, scale
// undoable commands.
class UsdTRSUndoableCmdBase : public Ufe::SetVector3dUndoableCommand
{
private:
    const UsdTimeCode _readTime;
    const UsdTimeCode _writeTime;
    VtValue           _newOpValue;
    UsdGeomXformOp    _op;
    OpFunc            _opFunc;
    UsdUndoableItem   _undoableItem;

public:
    struct State
    {
        virtual const char* name() const = 0;
        virtual void        handleUndo(UsdTRSUndoableCmdBase*)
        {
            TF_CODING_ERROR(
                "Illegal handleUndo() call in UsdTRSUndoableCmdBase for state '%s'.", name());
        }
        virtual void handleSet(UsdTRSUndoableCmdBase*, const VtValue&)
        {
            TF_CODING_ERROR(
                "Illegal handleSet() call in UsdTRSUndoableCmdBase for state '%s'.", name());
        }
    };

    struct InitialState : public State
    {
        const char* name() const override { return "initial"; }
        void        handleUndo(UsdTRSUndoableCmdBase* cmd) override
        {
            // Maya triggers an undo on command creation, ignore it.
            cmd->_state = &UsdTRSUndoableCmdBase::_initialUndoCalledState;
        }
        void handleSet(UsdTRSUndoableCmdBase* cmd, const VtValue& v) override
        {
            // Add undoblock to capture edits
            UsdUndoBlock undoBlock(&cmd->_undoableItem);

            // Going from initial to executing / executed state, save value.
            cmd->_op = cmd->_opFunc(*cmd);
            cmd->_newOpValue = v;

            const PXR_NS::UsdAttribute& attr = cmd->_op.GetAttr();
            auto isSetAttrAllowed = MayaUsd::ufe::isAttributeEditAllowed(attr);
            if (isSetAttrAllowed) { 
                cmd->setValue(v);
            }

            cmd->_state = &UsdTRSUndoableCmdBase::_executeState;
        }
    };

    struct InitialUndoCalledState : public State
    {
        const char* name() const override { return "initial undo called"; }
        void        handleSet(UsdTRSUndoableCmdBase* cmd, const VtValue&) override
        {
            // Maya triggers a redo on command creation, ignore it.
            cmd->_state = &UsdTRSUndoableCmdBase::_initialState;
        }
    };

    struct ExecuteState : public State
    {
        const char* name() const override { return "execute"; }
        void        handleUndo(UsdTRSUndoableCmdBase* cmd) override
        {
            // Undo
            cmd->_undoableItem.undo();
            cmd->_state = &UsdTRSUndoableCmdBase::_undoneState;
        }
        void handleSet(UsdTRSUndoableCmdBase* cmd, const VtValue& v) override
        {
            cmd->_newOpValue = v;

            const PXR_NS::UsdAttribute& attr = cmd->_op.GetAttr();
            auto isSetAttrAllowed = MayaUsd::ufe::isAttributeEditAllowed(attr, false);
            if (isSetAttrAllowed) { 
                cmd->setValue(v);
            }
        }
    };

    struct UndoneState : public State
    {
        const char* name() const override { return "undone"; }
        void        handleSet(UsdTRSUndoableCmdBase* cmd, const VtValue&) override
        {
            // Redo
            cmd->_undoableItem.redo();
            cmd->_state = &UsdTRSUndoableCmdBase::_redoneState;
        }
    };

    struct RedoneState : public State
    {
        const char* name() const override { return "redone"; }
        void        handleUndo(UsdTRSUndoableCmdBase* cmd) override
        {
            // Undo
            cmd->_undoableItem.undo();
            cmd->_state = &UsdTRSUndoableCmdBase::_undoneState;
        }
        // The redone state should normally be reached only once manipulation
        // is over, after undo, so setting new values in the redone state seems
        // illogical.  However, during point snapping manipulation, within a
        // single drag, the Maya move command repeatedly calls undo, then redo,
        // setting new values after the redo.  Treat such events identically to
        // the Execute state.
        void handleSet(UsdTRSUndoableCmdBase* cmd, const VtValue& v) override
        {
            cmd->_newOpValue = v;

            const PXR_NS::UsdAttribute& attr = cmd->_op.GetAttr();
            auto isSetAttrAllowed = MayaUsd::ufe::isAttributeEditAllowed(attr, false);
            if (isSetAttrAllowed) { 
                cmd->setValue(v);
            }
        }
    };

    UsdTRSUndoableCmdBase(
        const VtValue&     newOpValue,
        const Ufe::Path&   path,
        OpFunc             opFunc,
        const UsdTimeCode& writeTime_)
        : Ufe::SetVector3dUndoableCommand(path)
        ,
        // Always read from proxy shape time.
        _readTime(getTime(path))
        , _writeTime(writeTime_)
        , _newOpValue(newOpValue)
        , _op()
        , _opFunc(std::move(opFunc))
    {
    }

    // Ufe::UndoableCommand overrides.
    void execute() override { handleSet(_newOpValue); }
    void undo() override { _state->handleUndo(this); }
    void redo() override { handleSet(_newOpValue); }

    void handleSet(const VtValue& v) { _state->handleSet(this, v); }

    void setValue(const VtValue& v) { _op.GetAttr().Set(v, _writeTime); }

    UsdTimeCode readTime() const { return _readTime; }
    UsdTimeCode writeTime() const { return _writeTime; }

    static InitialState           _initialState;
    static InitialUndoCalledState _initialUndoCalledState;
    static ExecuteState           _executeState;
    static UndoneState            _undoneState;
    static RedoneState            _redoneState;

    State* _state { &_initialState };
};

UsdTRSUndoableCmdBase::InitialState           UsdTRSUndoableCmdBase::_initialState;
UsdTRSUndoableCmdBase::InitialUndoCalledState UsdTRSUndoableCmdBase::_initialUndoCalledState;
UsdTRSUndoableCmdBase::ExecuteState           UsdTRSUndoableCmdBase::_executeState;
UsdTRSUndoableCmdBase::UndoneState            UsdTRSUndoableCmdBase::_undoneState;
UsdTRSUndoableCmdBase::RedoneState            UsdTRSUndoableCmdBase::_redoneState;

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
        VtValue v;
        v = V(x, y, z);
        handleSet(v);
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
        VtValue v;
        v = _cvtRotXYZToAttr(x, y, z);
        handleSet(v);
        return true;
    }

private:
    // Convert from UFE RotXYZ rotation to a value for the transform op.
    UsdTransform3dMayaXformStack::CvtRotXYZToAttrFn _cvtRotXYZToAttr;
};

} // namespace

UsdTransform3dMayaXformStack::UsdTransform3dMayaXformStack(const UsdSceneItem::Ptr& item)
    : UsdTransform3dBase(item)
    , _xformable(prim())
{
    TF_AXIOM(_xformable);
}

/* static */
UsdTransform3dMayaXformStack::Ptr
UsdTransform3dMayaXformStack::create(const UsdSceneItem::Ptr& item)
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
    TF_AXIOM(r);

    CvtRotXYZFromAttrFn cvt = getCvtRotXYZFromAttrFn(r.GetOpName());
    return cvt(getValue(r.GetAttr(), getTime(path())));
}

Ufe::Vector3d UsdTransform3dMayaXformStack::scale() const
{
    if (!hasOp(NdxScale)) {
        return Ufe::Vector3d(1, 1, 1);
    }
    UsdGeomXformOp s = getOp(NdxScale);
    TF_AXIOM(s);

    GfVec3f v;
    s.Get(&v, getTime(path()));
    return toUfe(v);
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::translateCmd(double x, double y, double z)
{
    return setVector3dCmd(
        GfVec3d(x, y, z),
        UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, getTRSOpSuffix()),
        getTRSOpSuffix());
}

Ufe::RotateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::rotateCmd(double x, double y, double z)
{
    GfVec3f        v(x, y, z);
    UsdGeomXformOp op;
    TfToken        attrName;
    const bool     hasRotate = hasOp(NdxRotate);
    if (hasRotate) {
        op = getOp(NdxRotate);
        attrName = op.GetOpName();
    }
    // If there is no rotate transform op, we will create a RotXYZ.
    CvtRotXYZToAttrFn cvt = hasRotate ? getCvtRotXYZToAttrFn(op.GetOpName()) : toXYZ;
    OpFunc            f = hasRotate
        ? OpFunc([attrName](const BaseUndoableCommand& cmd) {
              auto usdSceneItem = std::dynamic_pointer_cast<UsdSceneItem>(cmd.sceneItem());
              TF_AXIOM(usdSceneItem);
              auto attr = usdSceneItem->prim().GetAttribute(attrName);
              return UsdGeomXformOp(attr);
          })
        : OpFunc([opSuffix = getTRSOpSuffix(), setXformOpOrderFn = getXformOpOrderFn(), v](
                     const BaseUndoableCommand& cmd) {
              // Use notification guard, otherwise will generate one notification
              // for the xform op add, and another for the reorder.
              InTransform3dChange guard(cmd.path());
              auto usdSceneItem = std::dynamic_pointer_cast<UsdSceneItem>(cmd.sceneItem());
              TF_AXIOM(usdSceneItem);
              UsdGeomXformable xformable(usdSceneItem->prim());
              auto r = xformable.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, opSuffix);
              TF_AXIOM(r);
              r.Set(v);
              setXformOpOrderFn(xformable);
              return r;
          });

    return std::make_shared<UsdRotateOpUndoableCmd>(
        v, path(), std::move(f), cvt, UsdTimeCode::Default());
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3dMayaXformStack::scaleCmd(double x, double y, double z)
{
    GfVec3f        v(x, y, z);
    UsdGeomXformOp op;
    TfToken        attrName;
    const bool     hasScale = hasOp(NdxScale);
    if (hasScale) {
        op = getOp(NdxScale);
        attrName = op.GetOpName();
    }
    OpFunc f = hasScale
        ? OpFunc([attrName](const BaseUndoableCommand& cmd) {
              auto usdSceneItem = std::dynamic_pointer_cast<UsdSceneItem>(cmd.sceneItem());
              TF_AXIOM(usdSceneItem);
              auto attr = usdSceneItem->prim().GetAttribute(attrName);
              return UsdGeomXformOp(attr);
          })
        : OpFunc([opSuffix = getTRSOpSuffix(), setXformOpOrderFn = getXformOpOrderFn(), v](
                     const BaseUndoableCommand& cmd) {
              InTransform3dChange guard(cmd.path());
              auto usdSceneItem = std::dynamic_pointer_cast<UsdSceneItem>(cmd.sceneItem());
              TF_AXIOM(usdSceneItem);
              UsdGeomXformable xformable(usdSceneItem->prim());
              auto             s = xformable.AddScaleOp(UsdGeomXformOp::PrecisionFloat, opSuffix);
              TF_AXIOM(s);
              s.Set(v);
              setXformOpOrderFn(xformable);
              return s;
          });

    return std::make_shared<UsdVecOpUndoableCmd<GfVec3f>>(
        v, path(), std::move(f), UsdTimeCode::Default());
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::rotatePivotCmd(double x, double y, double z)
{
    return pivotCmd(getOpSuffix(NdxRotatePivot), x, y, z);
}

Ufe::Vector3d UsdTransform3dMayaXformStack::rotatePivot() const
{
    return getVector3d<GfVec3f>(
        UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, getOpSuffix(NdxRotatePivot)));
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::scalePivotCmd(double x, double y, double z)
{
    return pivotCmd(getOpSuffix(NdxScalePivot), x, y, z);
}

Ufe::Vector3d UsdTransform3dMayaXformStack::scalePivot() const
{
    return getVector3d<GfVec3f>(
        UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, getOpSuffix(NdxScalePivot)));
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::translateRotatePivotCmd(double x, double y, double z)
{
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
    // If the attribute doesn't exist yet, return a zero vector.
    auto attr = prim().GetAttribute(attrName);
    if (!attr) {
        return Ufe::Vector3d(0, 0, 0);
    }

    UsdGeomXformOp op(attr);
    TF_AXIOM(op);

    V v;
    op.Get(&v, getTime(path()));
    return toUfe(v);
}

template <class V>
Ufe::SetVector3dUndoableCommand::Ptr UsdTransform3dMayaXformStack::setVector3dCmd(
    const V&       v,
    const TfToken& attrName,
    const TfToken& opSuffix)
{
    auto   attr = prim().GetAttribute(attrName);
    auto   setXformOpOrderFn = getXformOpOrderFn();
    OpFunc f = attr
        ? OpFunc([attrName](const BaseUndoableCommand& cmd) {
              auto usdSceneItem = std::dynamic_pointer_cast<UsdSceneItem>(cmd.sceneItem());
              TF_AXIOM(usdSceneItem);
              auto attr = usdSceneItem->prim().GetAttribute(attrName);
              return UsdGeomXformOp(attr);
          })
        : OpFunc(
            // MAYA-108612: generalized lambda capture below is incorrect with
            // gcc 6.3.1 on Linux.  Call to getXformOpOrderFn() is non-virtual;
            // work around by calling in function body.  PPT, 11-Jan-2021.
            // [opSuffix, setXformOpOrderFn = getXformOpOrderFn(), v](const BaseUndoableCommand&
            // cmd) {
            [opSuffix, setXformOpOrderFn, v](const BaseUndoableCommand& cmd) {
                InTransform3dChange guard(cmd.path());
                auto usdSceneItem = std::dynamic_pointer_cast<UsdSceneItem>(cmd.sceneItem());
                TF_AXIOM(usdSceneItem);
                UsdGeomXformable xformable(usdSceneItem->prim());
                auto             op = xformable.AddTranslateOp(OpPrecision<V>::precision, opSuffix);
                TF_AXIOM(op);
                op.Set(v);
                setXformOpOrderFn(xformable);
                return op;
            });

    return std::make_shared<UsdVecOpUndoableCmd<V>>(
        v, path(), std::move(f), UsdTimeCode::Default());
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::pivotCmd(const TfToken& pvtOpSuffix, double x, double y, double z)
{
    auto pvtAttrName = UsdGeomXformOp::GetOpName(UsdGeomXformOp::TypeTranslate, pvtOpSuffix);

    GfVec3f v(x, y, z);
    auto    attr = prim().GetAttribute(pvtAttrName);
    OpFunc  f = attr
        ? OpFunc([pvtAttrName](const BaseUndoableCommand& cmd) {
              auto usdSceneItem = std::dynamic_pointer_cast<UsdSceneItem>(cmd.sceneItem());
              TF_AXIOM(usdSceneItem);
              auto attr = usdSceneItem->prim().GetAttribute(pvtAttrName);
              return UsdGeomXformOp(attr);
          })
        : OpFunc([pvtOpSuffix, setXformOpOrderFn = getXformOpOrderFn(), v](
                     const BaseUndoableCommand& cmd) {
              // Without a notification guard each operation (each transform op
              // addition, setting the attribute value, and setting the transform
              // op order) will notify.  Observers would see an object in an
              // inconsistent state, especially after pivot is added but before
              // its inverse is added --- this does not match the Maya transform
              // stack.  Use of SdfChangeBlock is discouraged when calling USD
              // APIs above Sdf, so use our own guard.
              InTransform3dChange guard(cmd.path());
              auto usdSceneItem = std::dynamic_pointer_cast<UsdSceneItem>(cmd.sceneItem());
              TF_AXIOM(usdSceneItem);
              UsdGeomXformable xformable(usdSceneItem->prim());
              auto p = xformable.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, pvtOpSuffix);
              auto pInv = xformable.AddTranslateOp(
                  UsdGeomXformOp::PrecisionFloat, pvtOpSuffix, /* isInverseOp */ true);
              TF_AXIOM(p && pInv);
              p.Set(v);
              setXformOpOrderFn(xformable);
              return p;
          });

    return std::make_shared<UsdVecOpUndoableCmd<GfVec3f>>(
        v, path(), std::move(f), UsdTimeCode::Default());
}

Ufe::SetMatrix4dUndoableCommand::Ptr
UsdTransform3dMayaXformStack::setMatrixCmd(const Ufe::Matrix4d& m)
{
    return std::make_shared<UsdSetMatrix4dUndoableCmd>(path(), m);
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
    return createTransform3d(item, [&]() { return _nextHandler->transform3d(item); });
}

Ufe::Transform3d::Ptr UsdTransform3dMayaXformStackHandler::editTransform3d(
    const Ufe::SceneItem::Ptr& item UFE_V2(, const Ufe::EditTransform3dHint& hint)) const
{
    // MAYA-109190: Moved the IsInstanceProxy() check here since it was causing the
    // camera framing not properly be applied.
    //
    // HS January 15, 2021: After speaking with Pierre, there is a more robust solution to move this
    // check entirely from here.

    // According to USD docs, editing scene description via instance proxies and their properties is
    // not allowed.
    // https://graphics.pixar.com/usd/docs/api/_usd__page__scenegraph_instancing.html#Usd_ScenegraphInstancing_InstanceProxies
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
    if (usdItem->prim().IsInstanceProxy()) {
        MGlobal::displayError(
            MString("Authoring to the descendant of an instance [")
            + MString(usdItem->prim().GetName().GetString().c_str()) + MString("] is not allowed. ")
            + MString("Please mark 'instanceable=false' to author edits to instance proxies."));
        return nullptr;
    }

    auto stage = usdItem->prim().GetStage();
    if (stage) {
        const SdfLayerHandle& editLayer = stage->GetEditTarget().GetLayer();
        if (editLayer && stage->IsLayerMuted(editLayer->GetIdentifier())) {
            MGlobal::displayError("Editing a muted layer is not allowed.");
            return nullptr;
        }
    }

    return createTransform3d(
        item, [&]() { return _nextHandler->editTransform3d(item UFE_V2(, hint)); });
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
