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
#include "UsdTransform3dFallbackMayaXformStack.h"

#include <mayaUsd/ufe/RotationUtils.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usdGeom/xformCache.h>

namespace {

using namespace MayaUsd::ufe;

// Fallback Transform3d handler transform ops namespace components are:
// "xformOp:<opType>:maya_fallback:<suffix>"
// The "maya_fallback" namespace component is not optional, but the rest of the
// suffix remains optional.
#define FB_CMPT "maya_fallback"
const TfToken fallbackComponent(FB_CMPT);

// UsdMayaXformStack::FindOpIndex() requires an inconvenient isInvertedTwin
// argument, various rotate transform op equivalences in a separate
// UsdMayaXformStack::IsCompatibleType().  Just roll our own op name to
// Maya transform stack index position.
const std::unordered_map<TfToken, UsdTransform3dMayaXformStack::OpNdx, TfToken::HashFunctor>
    gOpNameToNdx {
        { TfToken("xformOp:translate:" FB_CMPT), UsdTransform3dMayaXformStack::NdxTranslate },
        { TfToken("xformOp:translate:" FB_CMPT ":rotatePivotTranslate"),
          UsdTransform3dMayaXformStack::NdxRotatePivotTranslate },
        { TfToken("xformOp:translate:" FB_CMPT ":rotatePivot"),
          UsdTransform3dMayaXformStack::NdxRotatePivot },
        { TfToken("xformOp:rotateX:" FB_CMPT), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateY:" FB_CMPT), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateZ:" FB_CMPT), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateXYZ:" FB_CMPT), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateXZY:" FB_CMPT), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateYXZ:" FB_CMPT), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateYZX:" FB_CMPT), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateZXY:" FB_CMPT), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateZYX:" FB_CMPT), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:orient:" FB_CMPT), UsdTransform3dMayaXformStack::NdxRotate },
        { TfToken("xformOp:rotateXYZ:" FB_CMPT ":rotateAxis"),
          UsdTransform3dMayaXformStack::NdxRotateAxis },
        { TfToken("!invert!xformOp:translate:" FB_CMPT ":rotatePivot"),
          UsdTransform3dMayaXformStack::NdxRotatePivotInverse },
        { TfToken("xformOp:translate:" FB_CMPT ":scalePivotTranslate"),
          UsdTransform3dMayaXformStack::NdxScalePivotTranslate },
        { TfToken("xformOp:translate:" FB_CMPT ":scalePivot"),
          UsdTransform3dMayaXformStack::NdxScalePivot },
        { TfToken("xformOp:transform:" FB_CMPT ":shear"), UsdTransform3dMayaXformStack::NdxShear },
        { TfToken("xformOp:scale:" FB_CMPT), UsdTransform3dMayaXformStack::NdxScale },
        { TfToken("!invert!xformOp:translate:" FB_CMPT ":scalePivot"),
          UsdTransform3dMayaXformStack::NdxScalePivotInverse }
    };

bool MatchingSubstack(const std::vector<UsdGeomXformOp>& ops)
{
    // An empty sub-stack matches.
    if (ops.empty()) {
        return true;
    }

    // Check op ordering.  Contrary to the fully-general USD transform stack
    // capability, which allows for multiple (different) rotation transform
    // ops, the Maya stack only allows for a single rotation transform op.
    // Therefore, at each step we increment the ordered index one beyond the
    // current op's index, since each index can appear only once.
    int orderedNdx = UsdTransform3dMayaXformStack::NdxTranslate;
    for (const auto& op : ops) {
        auto opNdx = gOpNameToNdx.at(op.GetOpName());
        if (opNdx < orderedNdx) {
            return false;
        }
        orderedNdx = opNdx + 1;
    }
    return true;
}

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {
namespace {

// Adapted from UsdTransform3dMatrixOp.cpp.
template <bool INCLUSIVE>
GfMatrix4d computeLocalTransform(
    const std::vector<UsdGeomXformOp>&          ops,
    std::vector<UsdGeomXformOp>::const_iterator endOp,
    const UsdTimeCode&                          time)
{
    // If we want the op to be included, increment the end op iterator.
    if (INCLUSIVE) {
        TF_AXIOM(endOp != ops.end());
        ++endOp;
    }

    // GetLocalTransformation() interface does not allow passing a begin and
    // end iterator, so copy into an argument vector.
    std::vector<UsdGeomXformOp> argOps(std::distance(ops.begin(), endOp));
    argOps.assign(ops.begin(), endOp);

    GfMatrix4d m(1);
    if (!UsdGeomXformable::GetLocalTransformation(&m, argOps, time)) {
        TF_FATAL_ERROR("Local transformation computation failed.");
    }

    return m;
}

auto computeLocalInclusiveTransform = computeLocalTransform<true>;
auto computeLocalExclusiveTransform = computeLocalTransform<false>;

std::vector<UsdGeomXformOp>::const_iterator
findFirstFallbackOp(const std::vector<UsdGeomXformOp>& ops)
{
    return std::find_if(ops.begin(), ops.end(), [](const UsdGeomXformOp& op) {
        return op.GetOpName().GetString().find(fallbackComponent.GetText()) != std::string::npos;
    });
}

void setXformOpOrder(const UsdGeomXformable& xformable)
{
    // As this method is called after appending a transform op to the fallback
    // transform op sub-stack, we copy transform ops up to but excluding the
    // first op in the fallback transform op sub-stack.
    bool resetsXformStack = false;
    auto oldOrder = xformable.GetOrderedXformOps(&resetsXformStack);
    auto i = findFirstFallbackOp(oldOrder);
    TF_AXIOM(i != oldOrder.end());

    // Copy ops before the Maya sub-stack unchanged.
    std::vector<UsdGeomXformOp> newOrder;
    newOrder.reserve(oldOrder.size());
    std::copy(oldOrder.cbegin(), i, std::back_inserter(newOrder));

    // Sort from the start of the Maya sub-stack.  Use the Maya transform stack
    // indices to add to a map, then simply traverse the map to obtain the
    // transform ops in Maya sub-stack order.
    std::map<int, UsdGeomXformOp> orderedOps;
    for (; i != oldOrder.end(); ++i) {
        auto op = *i;
        auto ndx = gOpNameToNdx.at(op.GetOpName());
        orderedOps[ndx] = op;
    }

    // Set the transform op order attribute.
    for (const auto& orderedOp : orderedOps) {
        const auto& op = orderedOp.second;
        newOrder.emplace_back(op);
    }

    xformable.SetXformOpOrder(newOrder, resetsXformStack);
}

Ufe::Transform3d::Ptr createTransform3d(const Ufe::SceneItem::Ptr& item)
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif

    // If the prim isn't transformable, can't create a Transform3d interface
    // for it.
    UsdGeomXformable xformSchema(usdItem->prim());
    if (!xformSchema) {
        return nullptr;
    }
    bool resetsXformStack = false;
    auto xformOps = xformSchema.GetOrderedXformOps(&resetsXformStack);

    // We are the fallback Transform3d handler: there must be transform ops to
    // match.
    TF_AXIOM(!xformOps.empty());

    // We find the first transform op in the vector that has our fallback
    // component token in its attribute name.  From that point on, all
    // remaining transform ops must match a Maya transform stack with the
    // fallback component token.  If no transform op matches the fallback
    // component token, we start a new Maya transform stack at the end of the
    // existing stack.
    auto i = findFirstFallbackOp(xformOps);

    // No transform op matched: start a new Maya transform stack at the end.
    if (i == xformOps.end()) {
        return UsdTransform3dFallbackMayaXformStack::create(usdItem);
    }

    // Otherwise, copy ops starting at the first fallback op we found.  If all
    // is well, from the first fallback op onwards, we have a sub-stack that
    // matches the fallback Maya transform stack.
    std::vector<UsdGeomXformOp> candidateOps;
    candidateOps.reserve(std::distance(i, xformOps.cend()));
    std::copy(i, xformOps.cend(), std::back_inserter(candidateOps));

    // We're the last handler in the chain of responsibility: if the candidate
    // ops support the Maya transform stack, create a Maya transform stack
    // interface for it, otherwise no further handlers to delegate to, so fail.
    return MatchingSubstack(candidateOps) ? UsdTransform3dFallbackMayaXformStack::create(usdItem)
                                          : nullptr;
}

} // namespace

UsdTransform3dFallbackMayaXformStack::UsdTransform3dFallbackMayaXformStack(
    const UsdSceneItem::Ptr& item)
    : UsdTransform3dMayaXformStack(item)
{
}

/* static */
UsdTransform3dFallbackMayaXformStack::Ptr
UsdTransform3dFallbackMayaXformStack::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdTransform3dFallbackMayaXformStack>(item);
}

UsdTransform3dFallbackMayaXformStack::SetXformOpOrderFn
UsdTransform3dFallbackMayaXformStack::getXformOpOrderFn() const
{
    return setXformOpOrder;
}

TfToken UsdTransform3dFallbackMayaXformStack::getOpSuffix(OpNdx ndx) const
{
    static std::unordered_map<OpNdx, TfToken> opSuffix
        = { { NdxRotatePivotTranslate, TfToken(FB_CMPT ":rotatePivotTranslate") },
            { NdxRotatePivot, TfToken(FB_CMPT ":rotatePivot") },
            { NdxRotateAxis, TfToken(FB_CMPT ":rotateAxis") },
            { NdxScalePivotTranslate, TfToken(FB_CMPT ":scalePivotTranslate") },
            { NdxScalePivot, TfToken(FB_CMPT ":scalePivot") },
            { NdxShear, TfToken(FB_CMPT ":shear") } };
    return opSuffix.at(ndx);
}

TfToken UsdTransform3dFallbackMayaXformStack::getTRSOpSuffix() const { return fallbackComponent; }

UsdTransform3dMayaXformStack::CvtRotXYZFromAttrFn
UsdTransform3dFallbackMayaXformStack::getCvtRotXYZFromAttrFn(const TfToken& opName) const
{
    static std::unordered_map<TfToken, CvtRotXYZFromAttrFn, TfToken::HashFunctor> cvt
        = { { TfToken("xformOp:rotateX:" FB_CMPT), fromX },
            { TfToken("xformOp:rotateY:" FB_CMPT), fromY },
            { TfToken("xformOp:rotateZ:" FB_CMPT), fromZ },
            { TfToken("xformOp:rotateXYZ:" FB_CMPT), fromXYZ },
            { TfToken("xformOp:rotateXZY:" FB_CMPT), fromXZY },
            { TfToken("xformOp:rotateYXZ:" FB_CMPT), fromYXZ },
            { TfToken("xformOp:rotateYZX:" FB_CMPT), fromYZX },
            { TfToken("xformOp:rotateZXY:" FB_CMPT), fromZXY },
            { TfToken("xformOp:rotateZYX:" FB_CMPT), fromZYX },
            { TfToken("xformOp:orient:" FB_CMPT), nullptr } }; // FIXME, unsupported.

    return cvt.at(opName);
}

UsdTransform3dMayaXformStack::CvtRotXYZToAttrFn
UsdTransform3dFallbackMayaXformStack::getCvtRotXYZToAttrFn(const TfToken& opName) const
{
    static std::unordered_map<TfToken, CvtRotXYZToAttrFn, TfToken::HashFunctor> cvt
        = { { TfToken("xformOp:rotateX:" FB_CMPT), toX },
            { TfToken("xformOp:rotateY:" FB_CMPT), toY },
            { TfToken("xformOp:rotateZ:" FB_CMPT), toZ },
            { TfToken("xformOp:rotateXYZ:" FB_CMPT), toXYZ },
            { TfToken("xformOp:rotateXZY:" FB_CMPT), toXZY },
            { TfToken("xformOp:rotateYXZ:" FB_CMPT), toYXZ },
            { TfToken("xformOp:rotateYZX:" FB_CMPT), toYZX },
            { TfToken("xformOp:rotateZXY:" FB_CMPT), toZXY },
            { TfToken("xformOp:rotateZYX:" FB_CMPT), toZYX },
            { TfToken("xformOp:orient:" FB_CMPT), nullptr } }; // FIXME, unsupported.

    return cvt.at(opName);
}

std::map<UsdTransform3dMayaXformStack::OpNdx, UsdGeomXformOp>
UsdTransform3dFallbackMayaXformStack::getOrderedOps() const
{
    bool resetsXformStack = false;
    auto ops = _xformable.GetOrderedXformOps(&resetsXformStack);
    auto i = findFirstFallbackOp(ops);
    TF_AXIOM(i != ops.end());

    // Sort from the start of the Maya sub-stack.  Use the Maya transform stack
    // indices to add to a map, then simply traverse the map to obtain the
    // transform ops in Maya sub-stack order.
    std::map<OpNdx, UsdGeomXformOp> orderedOps;
    for (; i != ops.end(); ++i) {
        auto op = *i;
        auto ndx = gOpNameToNdx.at(op.GetOpName());
        orderedOps[ndx] = op;
    }
    return orderedOps;
}

// segmentInclusiveMatrix() from UsdTransform3dBase is fine.

Ufe::Matrix4d UsdTransform3dFallbackMayaXformStack::segmentExclusiveMatrix() const
{
    // Get the parent transform plus all ops up to and excluding the first
    // fallback op.
    auto              time = getTime(path());
    UsdGeomXformCache xformCache(time);
    auto              parent = xformCache.GetParentToWorldTransform(prim());
    bool              unused;
    auto              ops = _xformable.GetOrderedXformOps(&unused);
    auto              local = computeLocalExclusiveTransform(ops, findFirstFallbackOp(ops), time);
    return toUfe(local * parent);
}

//------------------------------------------------------------------------------
// UsdTransform3dFallbackMayaXformStackHandler
//------------------------------------------------------------------------------

UsdTransform3dFallbackMayaXformStackHandler::UsdTransform3dFallbackMayaXformStackHandler()
    : Ufe::Transform3dHandler()
{
}

/*static*/
UsdTransform3dFallbackMayaXformStackHandler::Ptr
UsdTransform3dFallbackMayaXformStackHandler::create()
{
    return std::make_shared<UsdTransform3dFallbackMayaXformStackHandler>();
}

Ufe::Transform3d::Ptr
UsdTransform3dFallbackMayaXformStackHandler::transform3d(const Ufe::SceneItem::Ptr& item) const
{
    return createTransform3d(item);
}

Ufe::Transform3d::Ptr
UsdTransform3dFallbackMayaXformStackHandler::editTransform3d(const Ufe::SceneItem::Ptr& item) const
{
    return createTransform3d(item);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
