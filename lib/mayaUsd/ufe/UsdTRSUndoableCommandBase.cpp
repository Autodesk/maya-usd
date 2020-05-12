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

#include <pxr/base/gf/rotation.h>

#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

#include <iostream>

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
    // os.environ['UFE_XFORM_OP_XFORM']='1' to create single xformOp:transform
    const char* UFE_XFORM_OP_XFORM = ::getenv("UFE_XFORM_OP_XFORM");

    return UFE_XFORM_OP_XFORM && UFE_XFORM_OP_XFORM[0] != '0' ?
        UsdGeomXformOp::TypeTransform : opType;
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
    virtual bool doIt(UsdGeomXformable& xformable, VtValue& value)
    {
        return fXformOp.Set(value);
    }
    virtual const VtValue& initialValue() const {
        static const VtValue sValue;
        return sValue;
    }

    virtual bool undoIt(UsdGeomXformable& xformable, const XformOpState& state) = 0;

    operator const UsdGeomXformOp& () const { return fXformOp; }
};

// Undo/Redo on a single UsdGeomXformOp
//
class XformOpUndo : public XformOpHandler
{
    VtValue fInitialValue;

public:
    XformOpUndo(UsdGeomXformOp op) : XformOpHandler(std::move(op))
    {
        fXformOp.GetAttr().Get(&fInitialValue);
    }

    bool undoIt(UsdGeomXformable& xformable, const XformOpState& state) override
    {
        return fXformOp.Set(fInitialValue);
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

public:

    XformStackUndo(const XformStackUndo&) = delete;

    XformStackUndo(UsdGeomXformOp op, XformStackUndo&& other)
        : XformOpHandler(std::move(op))
        , fXformStack(std::move(other.fXformStack))
        , fOpIndex(other.fOpIndex)
        , fResetsXformStack(other.fResetsXformStack)
    {}

    bool doIt(UsdGeomXformable& xformable, VtValue& value) override
    {
        // Set the op value
        if (!XformOpHandler::doIt(xformable, value))
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

    bool undoIt(UsdGeomXformable& xformable, const XformOpState& state) override
    {
        bool result1 = xformable.SetXformOpOrder(std::move(fXformStack), fResetsXformStack);
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
        fXformStack = xformable.GetOrderedXformOps(&fResetsXformStack);
    }

    std::unique_ptr<XformOpHandler> insertOp(UsdGeomXformable& xformable,
        const XformOpState& state, std::vector<UsdGeomXformOp>* prevOps)
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

                return std::unique_ptr<XformOpHandler>(new XformOpUndo(*ritr));
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

        return std::unique_ptr<XformOpHandler>(
            new XformStackUndo(UsdGeomXformOp(std::move(attr), false, {}), std::move(*this)));
    }
};

} // anonymous namespace

class UsdTRSUndoableCommandBase::ExtendedUndo
{
    struct Decomposed {
        GfVec3d t, s;
        GfMatrix4d u;
        Decomposed(const GfMatrix4d& xform)
        {
            GfMatrix4d r, p;
            xform.Factor(&r, &s, &u, &t, &p);
        }
    };

    std::unique_ptr<XformOpState> fInitialState;
    std::unique_ptr<XformOpHandler> fHandler;

    VtValue fCurValue;

    // Used when setting to an 'xformOp:transform' op
    const UsdGeomXformOp::Type fOrigOpType;
    std::unique_ptr<Decomposed> fDecomposed;

    // Workaround UFE sending rotation including ops previous to the one being edited
#if UFE_PREVIEW_VERSION_NUM < 2013
    std::unique_ptr<GfVec3d> fOffset;
#endif


    bool initialize(UsdGeomXformable& xformable, const XformOpState& state)
    {
#if UFE_PREVIEW_VERSION_NUM < 2013
        std::vector<UsdGeomXformOp> prevOps;
        std::vector<UsdGeomXformOp> *prevOpPtr =
            (fOrigOpType == UsdGeomXformOp::TypeRotateXYZ) ? &prevOps : nullptr;
#else
        std::vector<UsdGeomXformOp> *prevOpPtr = nullptr;
#endif


        XformStackUndo builder(xformable);
        fHandler = builder.insertOp(xformable, state, prevOpPtr);

        const VtValue& prevVal = fHandler->initialValue();
        if (!prevVal.IsEmpty())
        {
            // Do the decomposition now when setting to a GfMatrix4d
            if (state.fOpType == UsdGeomXformOp::TypeTransform)
                fDecomposed.reset(new Decomposed(prevVal.UncheckedGet<GfMatrix4d>()));

            // Only needed for accumulation when fOrigOpType == UsdGeomXformOp::TypeTranslate
            fCurValue = prevVal;
        }


#if UFE_PREVIEW_VERSION_NUM < 2013
        if (!prevOps.empty())
        {
            GfMatrix4d transform;
            if (UsdGeomXformable::GetLocalTransformation(&transform, prevOps, UsdTimeCode::Default()))
            {
                fOffset.reset(new GfVec3d);
                *fOffset = transform.DecomposeRotation(GfVec3d::XAxis(), GfVec3d::YAxis(), GfVec3d::ZAxis());
            }
        }
#endif

        return static_cast<bool>(fHandler);
    }

    template <typename V>
    void setVector(V value)
    {
        if (fOrigOpType == UsdGeomXformOp::TypeTranslate)
            value += fCurValue.UncheckedGet<V>();

        fCurValue.UncheckedSwap<V>(value);
    }

    void setValue(GfVec3d value)
    {
#if UFE_PREVIEW_VERSION_NUM < 2013
        if (fOffset)
            value -= *fOffset;
#endif

        if (fCurValue.IsHolding<GfVec3f>())
        {
            setVector<GfVec3f>(GfVec3f(value));
            return;
        }

        if (fCurValue.IsHolding<GfVec3d>())
        {
            setVector<GfVec3d>(value);
            return;
        }

        if (fCurValue.IsHolding<GfMatrix4d>())
        {
            GfVec3d t, s;
            GfMatrix4d u;

            if (fDecomposed)
            {
                u = fDecomposed->u;
                t = fDecomposed->t;
                s = fDecomposed->s;
            }
            else
            {
                u = GfMatrix4d(1.0);
                t = GfVec3d(0,0,0);
                s = GfVec3d(1,1,1);
            }

            if (fOrigOpType == UsdGeomXformOp::TypeTranslate)
            {
                t = value + fCurValue.UncheckedGet<GfMatrix4d>().ExtractTranslation();
            }
            else if (fOrigOpType == UsdGeomXformOp::TypeScale)
                s = value;
            else
            {
                u.SetRotate(GfRotation(GfVec3d::XAxis(), value[0]) *
                            GfRotation(GfVec3d::YAxis(), value[1]) *
                            GfRotation(GfVec3d::ZAxis(), value[2]));
            }

            fCurValue = (GfMatrix4d(GfVec4d(s[0], s[1], s[2], 1.0)) * u).SetTranslateOnly(t);
            return;
        }
    }

public:
    template <typename T>
    ExtendedUndo(const UsdPrim& prim, T vec, UsdGeomXformOp::Type opType,
        UsdGeomXformOp::Precision prec)
        : fInitialState(new XformOpState(opType, prec))
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
#if UFE_PREVIEW_VERSION_NUM < 2013
        fOffset.reset();
#endif

        // Done with fHandler until reborn in doIt
        std::unique_ptr<XformOpHandler> oldHandler(fHandler.release());
        return oldHandler->undoIt(xformable, *fInitialState);
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

        return fHandler->doIt(xformable, fCurValue);
    }
};

UsdTRSUndoableCommandBase::UsdTRSUndoableCommandBase(
    const UsdSceneItem::Ptr& item, GfVec3f vec, UsdGeomXformOp::Type opType)
    : fItem(item)
    , fExtendedUndo(new ExtendedUndo(fItem->prim(), std::move(vec),
        opType, UsdGeomXformOp::PrecisionFloat))
{
}

UsdTRSUndoableCommandBase::UsdTRSUndoableCommandBase(
    const UsdSceneItem::Ptr& item, GfVec3d vec, UsdGeomXformOp::Type opType)
    : fItem(item)
    , fExtendedUndo(new ExtendedUndo(fItem->prim(), std::move(vec),
        opType, UsdGeomXformOp::PrecisionDouble))
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
