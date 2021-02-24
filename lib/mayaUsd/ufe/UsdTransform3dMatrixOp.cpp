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
#include "UsdTransform3dMatrixOp.h"

#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/UsdTransform3dSetObjectMatrix.h>
#include <mayaUsd/ufe/UsdXformOpUndoableCommandBase.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/ufe/XformOpUtils.h>

#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/transform.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/xformOp.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <ufe/transform3dUndoableCommands.h>

#include <algorithm>

UFE_NS_DEF
{
    // Ufe::Matrix4d does not provide equality operator, but VtValue needs it.
    // Create an implementation. It needs to live in the UFE namespace so that
    // template instantiation can find it.
    bool operator==(const Ufe::Matrix4d& lhs, const Ufe::Matrix4d& rhs)
    {
        return lhs.matrix == rhs.matrix;
    }
}

namespace {

using namespace MayaUsd;
using namespace MayaUsd::ufe;

VtValue getValue(const UsdAttribute& attr, const UsdTimeCode& time)
{
    VtValue value;
    attr.Get(&value, time);
    return value;
}

const char* getMatrixOp() { return std::getenv("MAYA_USD_MATRIX_XFORM_OP_NAME"); }

template <bool INCLUSIVE>
GfMatrix4d
computeLocalTransform(const UsdPrim& prim, const UsdGeomXformOp& op, const UsdTimeCode& time)
{
    UsdGeomXformable xformable(prim);
    bool             unused;
    auto             ops = xformable.GetOrderedXformOps(&unused);

    // FIXME  Searching for transform op in vector is awkward, as we've likely
    // already done this to create the UsdTransform3dMatrixOp object itself.
    // PPT, 10-Aug-2020.
    auto i = std::find(ops.begin(), ops.end(), op);

    if (i == ops.end()) {
        TF_FATAL_ERROR("Matrix op %s not found in transform ops.", op.GetOpName().GetText());
    }
    // If we want the op to be included, increment i.
    if (INCLUSIVE) {
        ++i;
    }
    std::vector<UsdGeomXformOp> cfOps(std::distance(ops.begin(), i));
    cfOps.assign(ops.begin(), i);

    GfMatrix4d m(1);
    if (!UsdGeomXformable::GetLocalTransformation(&m, cfOps, time)) {
        TF_FATAL_ERROR(
            "Local transformation computation for prim %s failed.", prim.GetPath().GetText());
    }

    return m;
}

auto computeLocalInclusiveTransform = computeLocalTransform<true>;
auto computeLocalExclusiveTransform = computeLocalTransform<false>;

std::vector<UsdGeomXformOp>::const_iterator
findMatrixOp(const std::vector<UsdGeomXformOp>& xformOps)
{
    auto opName = getMatrixOp();
    return std::find_if(xformOps.cbegin(), xformOps.cend(), [opName](const UsdGeomXformOp& op) {
        return (op.GetOpType() == UsdGeomXformOp::TypeTransform)
            && (!opName || std::string(opName) == op.GetOpName());
    });
}

// Given a starting point i (inclusive), is there a non-matrix transform op in
// the vector?
bool findNonMatrix(
    const std::vector<UsdGeomXformOp>::const_iterator& i,
    const std::vector<UsdGeomXformOp>&                 xformOps)
{
    return std::find_if(
               i,
               xformOps.cend(),
               [](const UsdGeomXformOp& op) {
                   return op.GetOpType() != UsdGeomXformOp::TypeTransform;
               })
        != xformOps.cend();
}

// Compute the inverse of the cumulative transform for the argument xform ops.
GfMatrix4d xformInv(
    const std::vector<UsdGeomXformOp>::const_iterator& begin,
    const std::vector<UsdGeomXformOp>::const_iterator& end,
    const Ufe::Path&                                   path)
{
    auto nbOps = std::distance(begin, end);
    if (nbOps == 0) {
        return GfMatrix4d { 1 };
    }
    std::vector<UsdGeomXformOp> ops(nbOps);
    ops.assign(begin, end);

    GfMatrix4d m { 1 };
    if (!UsdGeomXformable::GetLocalTransformation(&m, ops, getTime(path))) {
        TF_FATAL_ERROR(
            "Local transformation computation for item %s failed.", path.string().c_str());
    }

    return m.GetInverse();
}

// Class for setMatrixCmd() implementation.
class UsdSetMatrix4dUndoableCmd : public UsdValueUndoableCommandBase<Ufe::SetMatrix4dUndoableCommand>
{
public:
    UsdSetMatrix4dUndoableCmd(
        const Ufe::Matrix4d& newM,
        const Ufe::Path&     path,
        const UsdTimeCode&   writeTime)
        : UsdValueUndoableCommandBase<Ufe::SetMatrix4dUndoableCommand>(
            VtValue(newM),
            path,
            writeTime)
    {
    }

    ~UsdSetMatrix4dUndoableCmd() override { }

    bool set(const Ufe::Matrix4d&) override
    {
        // No-op: Maya does not set matrices through interactive manipulation.
        TF_WARN("Illegal call to UsdSetMatrix4dUndoableCmd::set()");
        return true;
    }

    void handleSet(State previousState, State newState, const VtValue& v) override
    {
        // Use editTransform3d() to set a single matrix transform op.
        // transform3d() returns a whole-object interface, which may include
        // other transform ops.
        auto t3d = Ufe::Transform3d::editTransform3d(sceneItem());
        const auto& matrix = v.Get<Ufe::Matrix4d>();
        t3d->setMatrix(matrix);
    }
};

// Command to set the translation on a scene item by setting a matrix transform
// op at an arbitrary position in the transform op stack.
class UsdTRSUndoableCmdBase : public UsdXformOpUndoableCommandBase<Ufe::SetVector3dUndoableCommand>
{
public:
#ifdef UFE_V2_FEATURES_AVAILABLE
    UsdTRSUndoableCmdBase(
        const Ufe::Path&      path,
        const UsdGeomXformOp& op,
        const UsdTimeCode&    writeTime)
        : UsdXformOpUndoableCommandBase<Ufe::SetVector3dUndoableCommand>(
            getValue(op.GetAttr(), getTime(path)),
            path,
            op,
            writeTime)
#else
    UsdTranslateUndoableCmd(
        const UsdSceneItem::Ptr& item,
        const UsdGeomXformOp&    op,
        const UsdTimeCode&       writeTime)
        : UsdXformOpUndoableCommandBase<Ufe::SetVector3dUndoableCommand>(
            getValue(op.GetAttr(), getTime(item->path())),
            item->path(),
            op,
            writeTime)
#endif
    {
        fOpTransform = op.GetOpTransform(readTime());
    }

protected:
    GfMatrix4d fOpTransform;
};

// Command to set the translation on a scene item by setting a matrix transform
// op at an arbitrary position in the transform op stack.
class UsdTranslateUndoableCmd : public UsdTRSUndoableCmdBase
{
public:
#ifdef UFE_V2_FEATURES_AVAILABLE
    UsdTranslateUndoableCmd(
        const Ufe::Path&      path,
        const UsdGeomXformOp& op,
        const UsdTimeCode&    writeTime)
        : UsdTRSUndoableCmdBase(path, op, writeTime)
#else
    UsdTranslateUndoableCmd(
        const UsdSceneItem::Ptr& item,
        const UsdGeomXformOp&    op,
        const UsdTimeCode&       writeTime)
        : UsdTRSUndoableCmdBase(item->path(), op, writeTime)
#endif
    {
    }

    // Executes the command by setting the translation onto the transform op.
    bool set(double x, double y, double z) override
    {
        fOpTransform.SetTranslateOnly(GfVec3d(x, y, z));
        setNewValue(VtValue(fOpTransform));
        execute();
        return true;
    }
};

class UsdRotateUndoableCmd : public UsdTRSUndoableCmdBase
{

public:
#ifdef UFE_V2_FEATURES_AVAILABLE
    UsdRotateUndoableCmd(
        const Ufe::Path&      path,
        const UsdGeomXformOp& op,
        const UsdTimeCode&    writeTime)
        : UsdTRSUndoableCmdBase(path, op, writeTime)
#else
    UsdRotateUndoableCmd(
        const UsdSceneItem::Ptr& item,
        const UsdGeomXformOp&    op,
        const UsdTimeCode&       writeTime)
        : UsdTRSUndoableCmdBase(item->path(), op, writeTime)
#endif
    {
        GfMatrix4d opTransform = op.GetOpTransform(readTime());

        // Other matrix decomposition code from AL:
        // from
        // https://github.com/AnimalLogic/maya-usd/blob/8852bdbb1fc904ac80543cd6103489097fa00154/lib/usd/utils/MayaTransformAPI.cpp#L979-L1055
        GfMatrix4d unusedR, unusedP;
        GfVec3d    s;
        if (!opTransform.Factor(&unusedR, &s, &fU, &fT, &unusedP)) {
            TF_FATAL_ERROR("Cannot decompose transform for op %s", op.GetOpName().GetText());
        }

        fS = GfMatrix4d(GfVec4d(s[0], s[1], s[2], 1.0));
    }

    // Executes the command by setting the rotation onto the transform op.
    bool set(double x, double y, double z) override
    {
        // Expect XYZ Euler angles in degrees.
        GfMatrix3d r(
            GfRotation(GfVec3d::XAxis(), x) * GfRotation(GfVec3d::YAxis(), y)
            * GfRotation(GfVec3d::ZAxis(), z));

        fU.SetRotate(r);

        GfMatrix4d opTransform = (fS * fU).SetTranslateOnly(fT);
        setNewValue(VtValue(opTransform));
        execute();
        return true;
    }

private:
    GfVec3d    fT;
    GfMatrix4d fS, fU;
};

class UsdScaleUndoableCmd : public UsdTRSUndoableCmdBase
{

public:
#ifdef UFE_V2_FEATURES_AVAILABLE
    UsdScaleUndoableCmd(
        const Ufe::Path&      path,
        const UsdGeomXformOp& op,
        const UsdTimeCode&    writeTime)
        : UsdTRSUndoableCmdBase(path, op, writeTime)
#else
    UsdScaleUndoableCmd(
        const UsdSceneItem::Ptr& item,
        const UsdGeomXformOp&    op,
        const UsdTimeCode&       writeTime)
        : UsdTRSUndoableCmdBase(item->path(), op, writeTime)
#endif
    {
        GfMatrix4d opTransform = op.GetOpTransform(readTime());

        // Other matrix decomposition code from AL:
        // from
        // https://github.com/AnimalLogic/maya-usd/blob/8852bdbb1fc904ac80543cd6103489097fa00154/lib/usd/utils/MayaTransformAPI.cpp#L979-L1055
        GfMatrix4d unusedR, unusedP;
        GfVec3d    unusedS;
        if (!opTransform.Factor(&unusedR, &unusedS, &fU, &fT, &unusedP)) {
            TF_FATAL_ERROR("Cannot decompose transform for op %s", op.GetOpName().GetText());
        }
    }

    // Executes the command by setting the rotation onto the transform op.
    bool set(double x, double y, double z) override
    {
        GfMatrix4d opTransform = (GfMatrix4d(GfVec4d(x, y, z, 1.0)) * fU).SetTranslateOnly(fT);
        setNewValue(VtValue(opTransform));
        execute();
        return true;
    }

private:
    GfVec3d    fT;
    GfMatrix4d fU;
};

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdTransform3dMatrixOp::UsdTransform3dMatrixOp(
    const UsdSceneItem::Ptr& item,
    const UsdGeomXformOp&    op)
    : UsdTransform3dBase(item)
    , _op(op)
{
}

/* static */
UsdTransform3dMatrixOp::Ptr
UsdTransform3dMatrixOp::create(const UsdSceneItem::Ptr& item, const UsdGeomXformOp& op)
{
    return std::make_shared<UsdTransform3dMatrixOp>(item, op);
}

Ufe::Vector3d UsdTransform3dMatrixOp::translation() const
{
    auto local = computeLocalInclusiveTransform(prim(), _op, getTime(path()));
    return toUfe(local.ExtractTranslation());
}

Ufe::Vector3d UsdTransform3dMatrixOp::rotation() const
{
    auto local = computeLocalInclusiveTransform(prim(), _op, getTime(path()));
    return toUfe(local.DecomposeRotation(GfVec3d::XAxis(), GfVec3d::YAxis(), GfVec3d::ZAxis()));
}

Ufe::Vector3d UsdTransform3dMatrixOp::scale() const
{
    auto       local = computeLocalInclusiveTransform(prim(), _op, getTime(path()));
    GfMatrix4d unusedR, unusedP, unusedU;
    GfVec3d    s, unusedT;
    if (!local.Factor(&unusedR, &s, &unusedU, &unusedT, &unusedP)) {
        TF_WARN("Cannot decompose local transform for %s", path().string().c_str());
        return Ufe::Vector3d(1, 1, 1);
    }

    return toUfe(s);
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMatrixOp::translateCmd(double x, double y, double z)
{
    return std::make_shared<UsdTranslateUndoableCmd>(
#ifdef UFE_V2_FEATURES_AVAILABLE
        path(),
#else
        usdSceneItem(),
#endif
        _op,
        UsdTimeCode::Default());
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3dMatrixOp::rotateCmd(double x, double y, double z)
{
    return std::make_shared<UsdRotateUndoableCmd>(
#ifdef UFE_V2_FEATURES_AVAILABLE
        path(),
#else
        usdSceneItem(),
#endif
        _op,
        UsdTimeCode::Default());
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3dMatrixOp::scaleCmd(double x, double y, double z)
{
    return std::make_shared<UsdScaleUndoableCmd>(
#ifdef UFE_V2_FEATURES_AVAILABLE
        path(),
#else
        usdSceneItem(),
#endif
        _op,
        UsdTimeCode::Default());
}

Ufe::SetMatrix4dUndoableCommand::Ptr UsdTransform3dMatrixOp::setMatrixCmd(const Ufe::Matrix4d& m)
{
    return std::make_shared<UsdSetMatrix4dUndoableCmd>(m, path(), UsdTimeCode::Default());
}

void UsdTransform3dMatrixOp::setMatrix(const Ufe::Matrix4d& m) { _op.Set(toUsd(m)); }

Ufe::Matrix4d UsdTransform3dMatrixOp::matrix() const
{
    return toUfe(_op.GetOpTransform(getTime(path())));
}

Ufe::Matrix4d UsdTransform3dMatrixOp::segmentInclusiveMatrix() const
{
    // Get the parent transform plus all ops including the requested one.
    auto              time = getTime(path());
    UsdGeomXformCache xformCache(time);
    auto              parent = xformCache.GetParentToWorldTransform(prim());
    auto              local = computeLocalInclusiveTransform(prim(), _op, time);
    return toUfe(local * parent);
}

Ufe::Matrix4d UsdTransform3dMatrixOp::segmentExclusiveMatrix() const
{
    // Get the parent transform plus all ops excluding the requested one.
    auto              time = getTime(path());
    UsdGeomXformCache xformCache(time);
    auto              parent = xformCache.GetParentToWorldTransform(prim());
    auto              local = computeLocalExclusiveTransform(prim(), _op, time);
    return toUfe(local * parent);
}

//------------------------------------------------------------------------------
// UsdTransform3dMatrixOpHandler
//------------------------------------------------------------------------------

UsdTransform3dMatrixOpHandler::UsdTransform3dMatrixOpHandler(
    const Ufe::Transform3dHandler::Ptr& nextHandler)
    : Ufe::Transform3dHandler()
    , _nextHandler(nextHandler)
{
}

/*static*/
UsdTransform3dMatrixOpHandler::Ptr
UsdTransform3dMatrixOpHandler::create(const Ufe::Transform3dHandler::Ptr& nextHandler)
{
    return std::make_shared<UsdTransform3dMatrixOpHandler>(nextHandler);
}

Ufe::Transform3d::Ptr
UsdTransform3dMatrixOpHandler::transform3d(const Ufe::SceneItem::Ptr& item) const
{
    // We must create a Transform3d interface to edit the whole object,
    // e.g. setting the local transformation matrix for the complete object.
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
    TF_AXIOM(usdItem);

    UsdGeomXformable xformable(usdItem->prim());
    bool             unused;
    auto             xformOps = xformable.GetOrderedXformOps(&unused);

    // If there is a single matrix transform op in the transform stack, then
    // transform3d() and editTransform3d() are equivalent: use that matrix op.
    if (xformOps.size() == 1 && xformOps.front().GetOpType() == UsdGeomXformOp::TypeTransform) {
        return UsdTransform3dMatrixOp::create(usdItem, xformOps.front());
    }

    // Find the matrix op to be transformed.
    auto i = findMatrixOp(xformOps);

    // If no matrix was found, pass on to the next handler.
    if (i == xformOps.cend()) {
        return _nextHandler->transform3d(item);
    }

    // If we've found a matrix op, but there is a more local non-matrix op in
    // the stack, the more local op should be used.  This will happen e.g. if a
    // pivot edit was done on a matrix op stack.  Since matrix ops don't
    // support pivot edits, a fallback Maya stack will be added, and from that
    // point on the fallback Maya stack must be used.
    if (findNonMatrix(i, xformOps)) {
        return _nextHandler->transform3d(item);
    }

    // At this point we know we have a matrix op to transform, and that it is
    // not alone on the transform op stack.  Wrap a matrix op Transform3d
    // interface for that matrix into a UsdTransform3dSetObjectMatrix object.
    // Ml is the transformation before the matrix op, Mr is the transformation
    // after the matrix op.
    auto mlInv = xformInv(xformOps.cbegin(), i, item->path());
    auto mrInv = xformInv(i + 1, xformOps.cend(), item->path());

    return UsdTransform3dSetObjectMatrix::create(
        UsdTransform3dMatrixOp::create(usdItem, *i), mlInv, mrInv);
}

Ufe::Transform3d::Ptr UsdTransform3dMatrixOpHandler::editTransform3d(
    const Ufe::SceneItem::Ptr& item UFE_V2(, const Ufe::EditTransform3dHint& hint)) const
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
    TF_AXIOM(usdItem);

    // Beware: the default UsdGeomXformOp constructor
    // https://github.com/PixarAnimationStudios/USD/blob/71b4baace2044ea4400ba802e91667f9ebe342f0/pxr/usd/usdGeom/xformOp.h#L148
    // leaves the _opType enum data member uninitialized, which as per
    // https://stackoverflow.com/questions/6842799/enum-variable-default-value/6842821
    // is undefined behavior, so a default constructed UsdGeomXformOp cannot be
    // used as a UsdGeomXformOp::TypeInvalid sentinel value.  PPT, 10-Aug-20.

    // We try to edit a matrix op in the prim's transform op stack.  If a
    // matrix op has been specified, it will be used if found.  If a matrix op
    // has not been specified, we edit the first matrix op in the stack.  If
    // the matrix op is not found, or there is no matrix op in the stack, let
    // the next Transform3d handler in the chain handle the request.
    UsdGeomXformable xformable(usdItem->prim());
    bool             unused;
    auto             xformOps = xformable.GetOrderedXformOps(&unused);

    // Find the matrix op to be transformed.
    auto i = findMatrixOp(xformOps);

    // If no matrix was found, pass on to the next handler.
    if (i == xformOps.cend()) {
        return _nextHandler->editTransform3d(item UFE_V2(, hint));
    }

    // If we've found a matrix op, but there is a more local non-matrix op in
    // the stack, the more local op should be used.  This will happen e.g. if a
    // pivot edit was done on a matrix op stack.  Since matrix ops don't
    // support pivot edits, a fallback Maya stack will be added, and from that
    // point on the fallback Maya stack must be used.  Also, pass pivot edits
    // on to the next handler, since we can't handle them.
    return (findNonMatrix(i, xformOps)
#ifdef UFE_V2_FEATURES_AVAILABLE
            || (hint.type() == Ufe::EditTransform3dHint::RotatePivot)
            || (hint.type() == Ufe::EditTransform3dHint::ScalePivot)
#endif
                )
        ? _nextHandler->editTransform3d(item UFE_V2(, hint))
        : UsdTransform3dMatrixOp::create(usdItem, *i);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
