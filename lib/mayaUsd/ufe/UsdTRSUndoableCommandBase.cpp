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

#include "UsdTRSUndoableCommandBase.h"
#include "private/Utils.h"
#include "Utils.h"

#include <pxr/base/gf/rotation.h>

#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

MAYAUSD_NS_DEF {
namespace ufe {

namespace {

// Placeholder for delivering an op-suffix via env-var or Maya setting
std::string getOpSuffix()
{
    // os.environ['UFE_XFORMOP_SUFFIX']='??' to use a new suffix
    const char* UFE_XFORMOP_SUFFIX = ::getenv("UFE_XFORM_OP_SUFFIX");

    return UFE_XFORMOP_SUFFIX ? UFE_XFORMOP_SUFFIX : "";
}

UsdGeomXformOp::Type getOpType(UsdGeomXformOp::Type opType)
{
    // os.environ['UFE_XFORM_OP_VECTOR']='1' to create single for tr, rot, sc
    const char* UFE_XFORM_OP_VECTOR = ::getenv("UFE_XFORM_OP_VECTOR");

    return UFE_XFORM_OP_VECTOR && UFE_XFORM_OP_VECTOR[0] != '0' ?
        opType : UsdGeomXformOp::TypeTransform;
}

std::string getOpName(UsdGeomXformOp::Type opType)
{
    // While you can put UsdGeomXformOp into a namespace other than xformOp
    // and it does work, USD will complain.
    return SdfPath::JoinIdentifier({"xformOp",
        UsdGeomXformOp::GetOpTypeToken(opType).GetString(),
        getOpSuffix()});
}

UsdGeomXformOp::Precision getOpPrecision(
    UsdGeomXformOp::Type opType, UsdGeomXformOp::Precision prec)
{
    return opType == UsdGeomXformOp::TypeTransform ?
        UsdGeomXformOp::PrecisionDouble : prec;

    return prec;
}

struct XformOpState {
    const UsdGeomXformOp::Type fOpType;
    const UsdGeomXformOp::Precision fOpPrecision;
    const TfToken fAttribute;

    XformOpState(UsdGeomXformOp::Type opType, UsdGeomXformOp::Precision prec)
        : fOpType(getOpType(opType))
        , fOpPrecision(getOpPrecision(fOpType, prec))
        , fAttribute(getOpName(fOpType))
    {}

    XformOpState(const UsdGeomXformOp& op)
        : fOpType(op.GetOpType())
        , fOpPrecision(op.GetPrecision())
        , fAttribute(op.GetName())
    {} 
};

class XformOpHandler
{
protected:
    UsdGeomXformOp fXformOp;

    XformOpHandler(UsdGeomXformOp op = {}) : fXformOp(std::move(op))
    {}

public:
    virtual bool doIt(UsdGeomXformable& xformable, VtValue& value,
        const UsdTimeCode& t)
    {
        return fXformOp.Set(value, t);
    }
    virtual const VtValue& initialValue() const {
        static const VtValue sValue;
        return sValue;
    }

    virtual bool undoIt(UsdGeomXformable& xformable, const XformOpState& state,
        const UsdTimeCode& t) = 0;

    operator const UsdGeomXformOp& () const { return fXformOp; }
};

// Undo/Redo on a single UsdGeomXformOp
//
class XformOpUndo : public XformOpHandler
{
    VtValue fInitialValue;
    bool fHadTimeSamples = false;
    bool fClearValue = false;

    UsdTimeCode time(const UsdTimeCode& t) const
    {
        return fHadTimeSamples ? t : UsdTimeCode::Default();
    }

public:
    XformOpUndo(UsdGeomXformOp op, const UsdTimeCode& t)
        : XformOpHandler(std::move(op))
    {
        auto attr = fXformOp.GetAttr();
        attr.Get(&fInitialValue, t);
        fHadTimeSamples = attr.GetNumTimeSamples() > 0;
        fClearValue = attr.GetPropertyStack().size() <= 2;
    }

    bool doIt(UsdGeomXformable& xformable, VtValue& value,
        const UsdTimeCode& t) override
    {
        return XformOpHandler::doIt(xformable, value, time(t));
    }

    bool undoIt(UsdGeomXformable& xformable, const XformOpState& state,
        const UsdTimeCode& t) override
    {
        if (!fClearValue)
            return XformOpHandler::doIt(xformable, fInitialValue, time(t));

        return xformable.GetPrim().RemoveProperty(fXformOp.GetName());
    }

    const VtValue& initialValue() const override
    {
        return fInitialValue;
    }
};

// Undo/Redo on a UsdGeomXformOp inserted into the xformOpOrder
//
class XformStackUndo : public XformOpHandler
{
    enum { npos = unsigned(-1) };

    std::vector<UsdGeomXformOp> fXformStack;
    unsigned fOpIndex;
    bool fResetsXformStack;
    bool fRemoveStack = true;

public:

    XformStackUndo(const XformStackUndo&) = delete;

    XformStackUndo(UsdGeomXformOp op, XformStackUndo&& other)
        : XformOpHandler(std::move(op))
        , fXformStack(std::move(other.fXformStack))
        , fOpIndex(other.fOpIndex)
        , fResetsXformStack(other.fResetsXformStack)
        , fRemoveStack(other.fRemoveStack)
    {}

    bool doIt(UsdGeomXformable& xformable, VtValue& value,
        const UsdTimeCode& t) override
    {
        // Set the op value
        if (!XformOpHandler::doIt(xformable, value, t))
            return false;

        // If its already been added to the xform-stack, we're done
        if (fOpIndex == npos)
            return true;

        // Mark it as added
        unsigned prevIndex = fOpIndex;
        fOpIndex = npos;

        // Add it to the xform-stack
        std::vector<UsdGeomXformOp> xformStack = fXformStack;
        xformStack.emplace(xformStack.begin()+prevIndex, fXformOp);
        return xformable.SetXformOpOrder(xformStack, fResetsXformStack);
    }

    bool undoIt(UsdGeomXformable& xformable, const XformOpState& state,
        const UsdTimeCode& t) override
    {
        bool result1;
        if (!fRemoveStack)
            result1 = xformable.SetXformOpOrder(std::move(fXformStack), fResetsXformStack);
        else
            result1 = xformable.GetPrim().RemoveProperty(UsdGeomTokens->xformOpOrder);

        bool result2 = xformable.GetPrim().RemoveProperty(state.fAttribute);
        return result1 && result2;
    }

    // Slightly odd pattern, but it's easier to use a XformStackUndo
    // as a builder for either XformOpUndo or XformStackUndo
    //
    // fHandler = XformStackUndo(xformable).insertOp(...)
    //
    XformStackUndo(UsdGeomXformable& xformable)
    {
        // Rather than always getting the GetXformOpOrderAttr twice (as GetOrderedXformOps will do so),
        // use the emptiness as a signal this attribute may have not existed at all.
        //
        fXformStack = xformable.GetOrderedXformOps(&fResetsXformStack);
        if (fXformStack.empty()) {
            auto attr = xformable.GetXformOpOrderAttr();
            fRemoveStack = !attr.IsValid() || !attr.HasAuthoredValue();
        }
    }

    std::pair<std::unique_ptr<XformOpHandler>,bool>
    insertOp(UsdGeomXformable& xformable, const XformOpState& state,
        const UsdTimeCode& t, std::vector<UsdGeomXformOp>* prevOps)
    {
        const std::string opSuffix = getOpSuffix();

        auto endOps = fXformStack.end(),
             firstOp = endOps;

        // Walk backwards looking for the first op of the same suffix & type
        // to set values to
        for (auto ritr = fXformStack.rbegin(), rend = fXformStack.rend(); ritr != rend; ++ritr)
        {
            if (ritr->IsInverseOp())
                continue;

            auto tokens = ritr->SplitName();
            auto curSuffix = tokens.size() == 3 ? tokens.back() : std::string();
            if (curSuffix != opSuffix)
                continue;

            if (ritr->GetOpType() == state.fOpType)
            {
                if (prevOps)
                    prevOps->assign(fXformStack.begin(), (ritr+1).base());

                return { std::unique_ptr<XformOpHandler>(new XformOpUndo(*ritr, t)), false };
            }

            firstOp = ritr.base();
        }

        // Add a new xformOp at the proper index in the exiting xformOpOrder
        auto opIndex = [&](UsdGeomXformOp::Type opType) -> size_t
        {
            if (firstOp == endOps)
                return fXformStack.size();

            if (opType == UsdGeomXformOp::TypeTranslate)
                return std::distance(fXformStack.begin(), firstOp);

            if (opType == UsdGeomXformOp::TypeScale)
                return fXformStack.size();

            if (firstOp->GetOpType() == UsdGeomXformOp::TypeTranslate)
                return std::distance(fXformStack.begin(), firstOp) + 1;

            return std::distance(fXformStack.begin(), firstOp);
        };
        fOpIndex = opIndex(state.fOpType);

        const SdfValueTypeName &typeName = UsdGeomXformOp::GetValueTypeName(state.fOpType, state.fOpPrecision);
        UsdAttribute attr = xformable.GetPrim().CreateAttribute(state.fAttribute, typeName, false);

        if (prevOps)
            prevOps->assign(fXformStack.begin(), fXformStack.begin()+fOpIndex);

        return { std::unique_ptr<XformOpHandler>(
            new XformStackUndo(UsdGeomXformOp(std::move(attr), false, {}), std::move(*this))),
            true };
    }
};

} // anonymous namespace

class UsdTRSUndoableCommandBase::ExtendedUndo
{
    struct Decomposed {
        GfVec3d t, s;
        GfMatrix4d u;
        GfMatrix4d inverse = GfMatrix4d(1);
        Decomposed() : t(0,0,0), s(1,1,1), u(1) {}
        Decomposed(const GfMatrix4d& xform)
        {
            GfMatrix4d r, p;
            xform.Factor(&r, &s, &u, &t, &p);
        }
    };

    std::unique_ptr<XformOpState> fInitialState;
    std::unique_ptr<XformOpHandler> fHandler;
    std::unique_ptr<GfMatrix4d> fInverse;

    VtValue fCurValue;
    GfVec3d fVecOffset;
    const UsdTimeCode fTime;

    // Used when setting to an 'xformOp:transform' op
    const UsdGeomXformOp::Type fOrigOpType;
    std::unique_ptr<Decomposed> fDecomposed;


    bool initialize(UsdGeomXformable& xformable, const XformOpState& state)
    {
        std::vector<UsdGeomXformOp> prevOps;

        bool append = false;
        XformStackUndo builder(xformable);
        std::tie(fHandler, append) = builder.insertOp(xformable, state, fTime, &prevOps);

        if (state.fOpType == UsdGeomXformOp::TypeTransform)
        {
            const VtValue& initialVal = fHandler->initialValue();
            fDecomposed.reset(initialVal.IsEmpty() ? new Decomposed() :
                new Decomposed(initialVal.UncheckedGet<GfMatrix4d>()));
        }
        else
            fDecomposed.reset();

        bool resets;
        GfMatrix4d transform;

        if (!prevOps.empty())
        {
            if (!xformable.GetLocalTransformation(&transform, &resets, prevOps, fTime))
                return false;
        }
        else
            transform = GfMatrix4d(1);

        fVecOffset.Set(0,0,0);
        fInverse.reset();

        const bool isRotate = fOrigOpType >= UsdGeomXformOp::TypeRotateX && fOrigOpType <= UsdGeomXformOp::TypeRotateZYX;
        if (isRotate)
            fVecOffset -= transform.DecomposeRotation(GfVec3d::XAxis(), GfVec3d::YAxis(), GfVec3d::ZAxis());
        else if (fOrigOpType == UsdGeomXformOp::TypeTranslate)
        {
            fVecOffset -= transform.Transform(GfVec3d(0,0,0));
            transform.SetTranslateOnly(GfVec3d(0,0,0));
            fInverse.reset(new GfMatrix4d(transform.GetInverse()));
        }
        else if (fOrigOpType == UsdGeomXformOp::TypeScale)
            fVecOffset.Set(0,0,0);
        else
            return false;

        return static_cast<bool>(fHandler);
    }

    template <typename V>
    void setVector(V value)
    {
        fCurValue.UncheckedSwap<V>(value);
    }

    void setValue(GfVec3d value)
    {
        value += fVecOffset;
        if (fInverse)
            value = fInverse->Transform(value);

        if (fCurValue.IsHolding<GfVec3f>())
            return setVector<GfVec3f>(GfVec3f(value));

        if (fCurValue.IsHolding<GfVec3d>())
            return setVector<GfVec3d>(value);

        if (fCurValue.IsHolding<GfMatrix4d>())
        {
            assert(fDecomposed && "Setting a matrix without a decomposition");

            GfVec3d t = fDecomposed->t;
            GfVec3d s = fDecomposed->s;
            GfMatrix4d u = fDecomposed->u;

            if (fOrigOpType == UsdGeomXformOp::TypeTranslate)
                t = value;
            else if (fOrigOpType == UsdGeomXformOp::TypeScale)
                s = value;
            else
                u.SetRotate(GfRotation(GfVec3d::XAxis(), value[0]) *
                            GfRotation(GfVec3d::YAxis(), value[1]) *
                            GfRotation(GfVec3d::ZAxis(), value[2]));

            fCurValue = (GfMatrix4d(GfVec4d(s[0], s[1], s[2], 1.0)) * u).SetTranslateOnly(t);
            return;
        }
    }

public:
    template <typename T>
    ExtendedUndo(const UsdPrim& prim, T vec, UsdGeomXformOp::Type opType,
        UsdGeomXformOp::Precision prec, const UsdTransform3d& item)
        : fInitialState(new XformOpState(opType, prec))
        , fVecOffset(0,0,0)
        , fTime(getTime(item.sceneItem()->path()))
        , fOrigOpType(opType)
    {
        if (fInitialState->fOpType == UsdGeomXformOp::TypeTransform)
            fCurValue = GfMatrix4d(1.0);
        else
            fCurValue = vec;
    }

    bool undoIt(UsdGeomXformable& xformable)
    {
        // These guards are largely for Maya & UsdTranslateUndoableCommand which
        // spams UsdTranslateUndoableCommand::undo before ever having called
        // UsdTranslateUndoableCommand::redo or UsdTranslateUndoableCommand::translate
        //
        if (fInitialState || !fHandler)
            return false;

        fInitialState.reset(new XformOpState(*fHandler));
        fDecomposed.reset();

        // Done with fHandler until reborn in doIt
        std::unique_ptr<XformOpHandler> oldHandler(fHandler.release());
        return oldHandler->undoIt(xformable, *fInitialState, fTime);
    }

    bool doIt(UsdGeomXformable& xformable, const GfVec3d* value = nullptr)
    {
        if (fInitialState)
        {
            if (!initialize(xformable, *fInitialState))
                return false;
            // Done with this until reborn in undoIt
            fInitialState.reset();
        }

        if (value)
            setValue(*value);

        return fHandler->doIt(xformable, fCurValue, fTime);
    }
};

UsdTRSUndoableCommandBase::UsdTRSUndoableCommandBase(
    const UsdTransform3d& item, const GfVec3f& vec, UsdGeomXformOp::Type opType)
    : fItem(std::static_pointer_cast<UsdSceneItem>(item.sceneItem()))
    , fExtendedUndo(new ExtendedUndo(fItem->prim(), vec,
        opType, UsdGeomXformOp::PrecisionFloat, item))
{
}

UsdTRSUndoableCommandBase::UsdTRSUndoableCommandBase(
    const UsdTransform3d& item, const GfVec3d& vec, UsdGeomXformOp::Type opType)
    : fItem(std::static_pointer_cast<UsdSceneItem>(item.sceneItem()))
    , fExtendedUndo(new ExtendedUndo(fItem->prim(), vec,
        opType, UsdGeomXformOp::PrecisionDouble, item))
{
}

UsdTRSUndoableCommandBase::~UsdTRSUndoableCommandBase()
{}

bool UsdTRSUndoableCommandBase::initialize()
{
    return Ufe::Scene::instance().addObjectPathChangeObserver(shared_from_this());
}

void UsdTRSUndoableCommandBase::operator()(
    const Ufe::Notification& n
)
{
    if (auto renamed = dynamic_cast<const Ufe::ObjectRename*>(&n)) {
        checkNotification(renamed);
    }
    else if (auto reparented = dynamic_cast<const Ufe::ObjectReparent*>(&n)) {
        checkNotification(reparented);
    }
}

template<class N>
void UsdTRSUndoableCommandBase::checkNotification(const N* notification)
{
    if (notification->previousPath() == path()) {
        fItem = std::dynamic_pointer_cast<UsdSceneItem>(notification->item());
    }
}

void UsdTRSUndoableCommandBase::undo()
{
    UsdGeomXformable xformable(prim());
    if (!xformable)
    {
        std::string err = TfStringPrintf("UsdGeomXformable::UsdGeomXformable failed for %s:", prim().GetName().GetText());
        throw std::runtime_error(err.c_str());
    }

    fExtendedUndo->undoIt(xformable);
}

void UsdTRSUndoableCommandBase::redo()
{
    UsdGeomXformable xformable(prim());
    if (!xformable)
    {
        std::string err = TfStringPrintf("UsdGeomXformable::UsdGeomXformable failed for %s:", prim().GetName().GetText());
        throw std::runtime_error(err.c_str());
    }

    fExtendedUndo->doIt(xformable);
}

void UsdTRSUndoableCommandBase::perform(double x, double y, double z)
{
    UsdGeomXformable xformable(prim());
    if (!xformable)
    {
        std::string err = TfStringPrintf("UsdGeomXformable::UsdGeomXformable failed for %s:", prim().GetName().GetText());
        throw std::runtime_error(err.c_str());
    }

    GfVec3d value(x, y, z);
    fExtendedUndo->doIt(xformable, &value);
}


} // namespace ufe
} // namespace MayaUsd
