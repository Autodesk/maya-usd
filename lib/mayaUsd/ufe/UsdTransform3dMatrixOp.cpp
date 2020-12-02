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
#include <mayaUsd/ufe/Utils.h>

#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/transform.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/usdGeom/xformOp.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <ufe/transform3dUndoableCommands.h>

#include <algorithm>

namespace {

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

// Helper class to factor out common code for translate, rotate, scale
// undoable commands.
class UsdTRSUndoableCmdBase
{
private:
    UsdGeomXformOp    fOp;
    const UsdTimeCode fReadTime;
    const UsdTimeCode fWriteTime;
    const VtValue     fPrevOpValue;

public:
    UsdTRSUndoableCmdBase(
        const Ufe::Path&      path,
        const UsdGeomXformOp& op,
        const UsdTimeCode&    writeTime_)
        : fOp(op)
        ,
        // Always read from proxy shape time.
        fReadTime(getTime(path))
        , fWriteTime(writeTime_)
        , fPrevOpValue(getValue(op.GetAttr(), readTime()))
        , fNewOpValue(fPrevOpValue)
    {
    }

    void undo() { fOp.GetAttr().Set(fPrevOpValue, fWriteTime); }
    void redo() { fOp.GetAttr().Set(fNewOpValue, fWriteTime); }

    UsdTimeCode readTime() const { return fReadTime; }
    UsdTimeCode writeTime() const { return fWriteTime; }

protected:
    VtValue fNewOpValue;
};

// Command to set the translation on a scene item by setting a matrix transform
// op at an arbitrary position in the transform op stack.
class UsdTranslateUndoableCmd
    : public Ufe::TranslateUndoableCommand
    , public UsdTRSUndoableCmdBase
{
public:
    UsdTranslateUndoableCmd(
#if UFE_PREVIEW_VERSION_NUM >= 2021
        const Ufe::Path& path,
#else
        const UsdSceneItem::Ptr& item,
#endif
        const UsdGeomXformOp& op,
        const UsdTimeCode&    writeTime
#if UFE_PREVIEW_VERSION_NUM >= 2021
        )
        : Ufe::TranslateUndoableCommand(path)
        , UsdTRSUndoableCmdBase(path, op, writeTime)
#else
        )
        : Ufe::TranslateUndoableCommand(item)
        , UsdTRSUndoableCmdBase(item->path(), op, writeTime)
#endif
    {
        fOpTransform = op.GetOpTransform(readTime());
    }

    void undo() override { UsdTRSUndoableCmdBase::undo(); }
    void redo() override { UsdTRSUndoableCmdBase::redo(); }

    // Executes the command by setting the translation onto the transform op.
    bool set(double x, double y, double z) override
    {
        fOpTransform.SetTranslateOnly(GfVec3d(x, y, z));
        fNewOpValue = fOpTransform;

        redo();
        return true;
    }

private:
    GfMatrix4d fOpTransform;
};

class UsdRotateUndoableCmd
    : public Ufe::RotateUndoableCommand
    , public UsdTRSUndoableCmdBase
{

public:
    UsdRotateUndoableCmd(
#if UFE_PREVIEW_VERSION_NUM >= 2021
        const Ufe::Path& path,
#else
        const UsdSceneItem::Ptr& item,
#endif
        const UsdGeomXformOp& op,
        const UsdTimeCode&    writeTime
#if UFE_PREVIEW_VERSION_NUM >= 2021
        )
        : Ufe::RotateUndoableCommand(path)
        , UsdTRSUndoableCmdBase(path, op, writeTime)
#else
        )
        : Ufe::RotateUndoableCommand(item)
        , UsdTRSUndoableCmdBase(item->path(), op, writeTime)
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

    void undo() override { UsdTRSUndoableCmdBase::undo(); }
    void redo() override { UsdTRSUndoableCmdBase::redo(); }

    // Executes the command by setting the rotation onto the transform op.
    bool set(double x, double y, double z) override
    {
        // Expect XYZ Euler angles in degrees.
        GfMatrix3d r(
            GfRotation(GfVec3d::XAxis(), x) * GfRotation(GfVec3d::YAxis(), y)
            * GfRotation(GfVec3d::ZAxis(), z));

        fU.SetRotate(r);

        GfMatrix4d opTransform = (fS * fU).SetTranslateOnly(fT);
        fNewOpValue = opTransform;

        redo();
        return true;
    }

private:
    GfVec3d    fT;
    GfMatrix4d fS, fU;
};

class UsdScaleUndoableCmd
    : public Ufe::ScaleUndoableCommand
    , public UsdTRSUndoableCmdBase
{

public:
    UsdScaleUndoableCmd(
#if UFE_PREVIEW_VERSION_NUM >= 2021
        const Ufe::Path& path,
#else
        const UsdSceneItem::Ptr& item,
#endif
        const UsdGeomXformOp& op,
        const UsdTimeCode&    writeTime
#if UFE_PREVIEW_VERSION_NUM >= 2021
        )
        : Ufe::ScaleUndoableCommand(path)
        , UsdTRSUndoableCmdBase(path, op, writeTime)
#else
        )
        : Ufe::ScaleUndoableCommand(item)
        , UsdTRSUndoableCmdBase(item->path(), op, writeTime)
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

    void undo() override { UsdTRSUndoableCmdBase::undo(); }
    void redo() override { UsdTRSUndoableCmdBase::redo(); }

    // Executes the command by setting the rotation onto the transform op.
    bool set(double x, double y, double z) override
    {
        GfMatrix4d opTransform = (GfMatrix4d(GfVec4d(x, y, z, 1.0)) * fU).SetTranslateOnly(fT);
        fNewOpValue = opTransform;

        redo();
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
        TF_WARN("Cannot decompose local transform for %s", pathCStr());
        return Ufe::Vector3d(1, 1, 1);
    }

    return toUfe(s);
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMatrixOp::translateCmd(double x, double y, double z)
{
    return std::make_shared<UsdTranslateUndoableCmd>(
#if UFE_PREVIEW_VERSION_NUM >= 2021
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
#if UFE_PREVIEW_VERSION_NUM >= 2021
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
#if UFE_PREVIEW_VERSION_NUM >= 2021
        path(),
#else
        usdSceneItem(),
#endif
        _op,
        UsdTimeCode::Default());
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
    // This method can be used to edit the 3D transform of the argument, but at
    // time of writing this is not implemented in UsdTransform3dMatrixOp, and
    // our UsdTransform3dBaseHandler base class does not know how to edit the
    // argument either.  Simply delegate to the next handler in the list.
    return _nextHandler->transform3d(item);
}

Ufe::Transform3d::Ptr UsdTransform3dMatrixOpHandler::editTransform3d(
    const Ufe::SceneItem::Ptr& item
#if UFE_PREVIEW_VERSION_NUM >= 2030
    ,
    const Ufe::EditTransform3dHint& hint
#endif
) const
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif

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
    auto             opName = getMatrixOp();
    UsdGeomXformable xformable(usdItem->prim());
    bool             unused;
    auto             xformOps = xformable.GetOrderedXformOps(&unused);
    auto i = std::find_if(xformOps.begin(), xformOps.end(), [opName](const UsdGeomXformOp& op) {
        return (op.GetOpType() == UsdGeomXformOp::TypeTransform)
            && (!opName || std::string(opName) == op.GetOpName());
    });
    bool foundMatrix = (i != xformOps.end());

    // If we've found a matrix op, but there is a more local non-matrix op in
    // the stack, the more local op should be used to handle the edit.
    bool moreLocalNonMatrix = foundMatrix
        ? (std::find_if(
               i,
               xformOps.end(),
               [](const UsdGeomXformOp& op) {
                   return op.GetOpType() != UsdGeomXformOp::TypeTransform;
               })
           != xformOps.end())
        : false;

    // We can't handle pivot edits, so in that case pass on to the next handler.
    return (foundMatrix && !moreLocalNonMatrix
#if UFE_PREVIEW_VERSION_NUM >= 2030
            && (hint.type() != Ufe::EditTransform3dHint::RotatePivot)
            && (hint.type() != Ufe::EditTransform3dHint::ScalePivot)
#endif
                )
        ? UsdTransform3dMatrixOp::create(usdItem, *i)
        : _nextHandler->editTransform3d(
            item
#if UFE_PREVIEW_VERSION_NUM >= 2030
            ,
            hint
#endif
        );
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
