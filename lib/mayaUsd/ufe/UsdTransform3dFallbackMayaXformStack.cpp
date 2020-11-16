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

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/fileio/utils/xformStack.h>

#include "private/UfeNotifGuard.h"

#include <maya/MEulerRotation.h>

#include <pxr/usd/usdGeom/xformCache.h>

#include <map>

#include <iostream>

namespace {

using namespace MayaUsd::ufe::hack;

// Fallback Transform3d handler transform ops namespace components are:
// "xformOp:<opType>:maya_fallback:<suffix>"
// The "maya_fallback" namespace component is not optional, but the rest of the
// suffix remains optional.
const TfToken fallbackComponent("maya_fallback");

// Type traits for GfVec precision.
template<class V>
struct OpPrecision {
    static UsdGeomXformOp::Precision precision;
};

template<>
UsdGeomXformOp::Precision OpPrecision<GfVec3f>::precision = 
    UsdGeomXformOp::PrecisionFloat;

template<>
UsdGeomXformOp::Precision OpPrecision<GfVec3d>::precision = 
    UsdGeomXformOp::PrecisionDouble;

inline double TO_DEG(double a) { return a * 180.0 / 3.141592654; }
inline double TO_RAD(double a) { return a * 3.141592654 / 180.0; }

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
const std::unordered_map<std::string, UsdTransform3dMayaXformStack::OpNdx> opNameToNdx{
    {"xformOp:translate:maya_fallback", UsdTransform3dMayaXformStack::NdxTranslate},
    {"xformOp:translate:maya_fallback:rotatePivotTranslate", UsdTransform3dMayaXformStack::NdxRotatePivotTranslate},
    {"xformOp:translate:maya_fallback:rotatePivot", UsdTransform3dMayaXformStack::NdxRotatePivot},
    {"xformOp:rotateX:maya_fallback",   UsdTransform3dMayaXformStack::NdxRotate},
    {"xformOp:rotateY:maya_fallback",   UsdTransform3dMayaXformStack::NdxRotate},
    {"xformOp:rotateZ:maya_fallback",   UsdTransform3dMayaXformStack::NdxRotate},
    {"xformOp:rotateXYZ:maya_fallback", UsdTransform3dMayaXformStack::NdxRotate},
    {"xformOp:rotateXZY:maya_fallback", UsdTransform3dMayaXformStack::NdxRotate},
    {"xformOp:rotateYXZ:maya_fallback", UsdTransform3dMayaXformStack::NdxRotate},
    {"xformOp:rotateYZX:maya_fallback", UsdTransform3dMayaXformStack::NdxRotate},
    {"xformOp:rotateZXY:maya_fallback", UsdTransform3dMayaXformStack::NdxRotate},
    {"xformOp:rotateZYX:maya_fallback", UsdTransform3dMayaXformStack::NdxRotate},
    {"xformOp:orient:maya_fallback",    UsdTransform3dMayaXformStack::NdxRotate},
    {"xformOp:rotateXYZ:maya_fallback:rotateAxis", UsdTransform3dMayaXformStack::NdxRotateAxis},
    {"!invert!xformOp:translate:maya_fallback:rotatePivot", UsdTransform3dMayaXformStack::NdxRotatePivotInverse},
    {"xformOp:translate:maya_fallback:scalePivotTranslate", UsdTransform3dMayaXformStack::NdxScalePivotTranslate},
    {"xformOp:translate:maya_fallback:scalePivot", UsdTransform3dMayaXformStack::NdxScalePivot},
    {"xformOp:maya_fallback:shear",     UsdTransform3dMayaXformStack::NdxShear},
    {"xformOp:scale:maya_fallback",     UsdTransform3dMayaXformStack::NdxScale},
    {"!invert!xformOp:translate:maya_fallback:scalePivot", UsdTransform3dMayaXformStack::NdxScalePivotInverse}};

//----------------------------------------------------------------------
// Conversion functions from RotXYZ to all supported rotation attributes.
//----------------------------------------------------------------------

typedef VtValue (*CvtRotXYZToAttrFn)(double x, double y, double z);

VtValue toXYZ(double x, double y, double z) {
    // No rotation order conversion
    VtValue v;
    v = GfVec3f(x, y, z);
    return v;
}

// Reorder argument RotXYZ rotation.
template<MEulerRotation::RotationOrder DST_ROT_ORDER>
VtValue to(double x, double y, double z) {
    MEulerRotation eulerRot(TO_RAD(x), TO_RAD(y), TO_RAD(z), MEulerRotation::kXYZ);
    eulerRot.reorderIt(DST_ROT_ORDER);
    VtValue v;
    v = GfVec3f(TO_DEG(eulerRot.x), TO_DEG(eulerRot.y), TO_DEG(eulerRot.z));
    return v;
}

auto toXZY = to<MEulerRotation::kXZY>;
auto toYXZ = to<MEulerRotation::kYXZ>;
auto toYZX = to<MEulerRotation::kYZX>;
auto toZXY = to<MEulerRotation::kZXY>;
auto toZYX = to<MEulerRotation::kZYX>;

// Scalar float is the proper type for single-axis rotations.
VtValue toX(double x, double, double) {
    VtValue v;
    v = float(x);
    return v;
}

VtValue toY(double, double y, double) {
    VtValue v;
    v = float(y);
    return v;
}

VtValue toZ(double, double, double z) {
    VtValue v;
    v = float(z);
    return v;
}

CvtRotXYZToAttrFn getCvtRotXYZToAttrFn(const TfToken& opName)
{
    // Can't get std::unordered_map<TfToken, CvtRotXYZToAttrFn> to instantiate.
    static std::map<TfToken, CvtRotXYZToAttrFn> cvt = {
        {TfToken("xformOp:rotateX:maya_fallback"),   toX},
        {TfToken("xformOp:rotateY:maya_fallback"),   toY},
        {TfToken("xformOp:rotateZ:maya_fallback"),   toZ},
        {TfToken("xformOp:rotateXYZ:maya_fallback"), toXYZ},
        {TfToken("xformOp:rotateXZY:maya_fallback"), toXZY},
        {TfToken("xformOp:rotateYXZ:maya_fallback"), toYXZ},
        {TfToken("xformOp:rotateYZX:maya_fallback"), toYZX},
        {TfToken("xformOp:rotateZXY:maya_fallback"), toZXY},
        {TfToken("xformOp:rotateZYX:maya_fallback"), toZYX},
        {TfToken("xformOp:orient:maya_fallback"),    nullptr}}; // FIXME, unsupported.

    return cvt.at(opName);
}

//----------------------------------------------------------------------
// Conversion functions from all supported rotation attributes to RotXYZ.
//----------------------------------------------------------------------

typedef Ufe::Vector3d (*CvtRotXYZFromAttrFn)(const VtValue& value);

Ufe::Vector3d fromXYZ(const VtValue& value) {
    // No rotation order conversion
    auto v = value.Get<GfVec3f>();
    return Ufe::Vector3d(v[0], v[1], v[2]);
}

template<MEulerRotation::RotationOrder SRC_ROT_ORDER>
Ufe::Vector3d from(const VtValue& value) {
    auto v = value.Get<GfVec3f>();

    MEulerRotation eulerRot(TO_RAD(v[0]), TO_RAD(v[1]), TO_RAD(v[2]), SRC_ROT_ORDER);
    eulerRot.reorderIt(MEulerRotation::kXYZ);
    return Ufe::Vector3d(TO_DEG(eulerRot.x), TO_DEG(eulerRot.y), TO_DEG(eulerRot.z));
}

auto fromXZY = from<MEulerRotation::kXZY>;
auto fromYXZ = from<MEulerRotation::kYXZ>;
auto fromYZX = from<MEulerRotation::kYZX>;
auto fromZXY = from<MEulerRotation::kZXY>;
auto fromZYX = from<MEulerRotation::kZYX>;

Ufe::Vector3d fromX(const VtValue& value) {
    return Ufe::Vector3d(value.Get<float>(), 0, 0);
}

Ufe::Vector3d fromY(const VtValue& value) {
    return Ufe::Vector3d(0, value.Get<float>(), 0);
}

Ufe::Vector3d fromZ(const VtValue& value) {
    return Ufe::Vector3d(0, 0, value.Get<float>());
}

CvtRotXYZFromAttrFn getCvtRotXYZFromAttrFn(const TfToken& opName)
{
    static std::map<TfToken, CvtRotXYZFromAttrFn> cvt = {
        {TfToken("xformOp:rotateX:maya_fallback"),   fromX},
        {TfToken("xformOp:rotateY:maya_fallback"),   fromY},
        {TfToken("xformOp:rotateZ:maya_fallback"),   fromZ},
        {TfToken("xformOp:rotateXYZ:maya_fallback"), fromXYZ},
        {TfToken("xformOp:rotateXZY:maya_fallback"), fromXZY},
        {TfToken("xformOp:rotateYXZ:maya_fallback"), fromYXZ},
        {TfToken("xformOp:rotateYZX:maya_fallback"), fromYZX},
        {TfToken("xformOp:rotateZXY:maya_fallback"), fromZXY},
        {TfToken("xformOp:rotateZYX:maya_fallback"), fromZYX},
        {TfToken("xformOp:orient:maya_fallback"),    nullptr}}; // FIXME, unsupported.

    return cvt.at(opName);
}

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
	auto opNdx = opNameToNdx.at(op.GetOpName());
	if (opNdx < orderedNdx) {
	    return false;
	}
	orderedNdx = opNdx+1;
    }
    return true;
}

}

namespace MAYAUSD_NS_DEF {
namespace ufe {
namespace hack {

namespace {

// Adapted from UsdTransform3dMatrixOp.cpp.
template<bool INCLUSIVE>
GfMatrix4d computeLocalTransform(
    const std::vector<UsdGeomXformOp>&          ops,
    std::vector<UsdGeomXformOp>::const_iterator endOp,
    const UsdTimeCode&                          time
)
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

std::vector<UsdGeomXformOp>::const_iterator findFirstFallbackOp(const std::vector<UsdGeomXformOp>& ops)
{
    return std::find_if(ops.begin(), ops.end(),
        [](const UsdGeomXformOp& op) {
            return op.GetOpName().GetString().find(fallbackComponent.GetText())
		!= std::string::npos;
        });
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
        return UsdTransform3dMayaXformStack::create(
            usdItem, std::vector<UsdGeomXformOp>());
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
    return MatchingSubstack(candidateOps) ? 
        UsdTransform3dMayaXformStack::create(usdItem, candidateOps) : nullptr;
}

// Helper class to factor out common code for translate, rotate, scale
// undoable commands.
class UsdTRSUndoableCmdBase : public Ufe::SetVector3dUndoableCommand {
private:
    UsdGeomXformOp    fOp;
    const UsdTimeCode fReadTime;
    const UsdTimeCode fWriteTime;
    const VtValue     fPrevOpValue;
    const TfToken     fAttrName;

public:
    UsdTRSUndoableCmdBase(
        const VtValue&        newOpValue,
        const Ufe::Path&      path,
        const UsdGeomXformOp& op,
        const UsdTimeCode&    writeTime_
    ) : Ufe::SetVector3dUndoableCommand(path), fOp(op),
        // Always read from proxy shape time.
        fReadTime(getTime(path)),
        fWriteTime(writeTime_),
        fPrevOpValue(getValue(op.GetAttr(), readTime())),
        fAttrName(op.GetOpName()),
        fNewOpValue(newOpValue)
    {}

    // Have separate execute override which is value-only, as default execute()
    // calls redo, and undo / redo will eventually not be value only and will
    // support propertySpec / primSpec management.
    void execute() override { setValue(fNewOpValue); }

    void recreateOp() {
        auto sceneItem = std::dynamic_pointer_cast<UsdSceneItem>(
            Ufe::Hierarchy::createItem(path())); TF_AXIOM(sceneItem);
        auto prim = sceneItem->prim(); TF_AXIOM(prim);
        auto attr = prim.GetAttribute(fAttrName); TF_AXIOM(attr);
        fOp       = UsdGeomXformOp(attr);
    }

    void undo() override {
        // Re-create the XformOp, as the underlying prim may have been
        // invalidated by later commands.
        recreateOp();
        setValue(fPrevOpValue);
    }
    void redo() override {
        // Re-create the XformOp, as the underlying prim may have been
        // invalidated by earlier commands.
        recreateOp();
        setValue(fNewOpValue);
    }

    void setValue(const VtValue& v) { fOp.GetAttr().Set(v, fWriteTime); }

    UsdTimeCode readTime() const { return fReadTime; }
    UsdTimeCode writeTime() const { return fWriteTime; }

protected:
    VtValue fNewOpValue;
};

// UsdRotatePivotTranslateUndoableCmd uses hard-coded USD common transform API
// single pivot attribute name, not reusable.
template<class V>
class UsdVecOpUndoableCmd : public UsdTRSUndoableCmdBase {
public:
    UsdVecOpUndoableCmd(
        const V&              v,
        const Ufe::Path&      path,
        const UsdGeomXformOp& op,
        const UsdTimeCode&    writeTime
    ) : UsdTRSUndoableCmdBase(VtValue(v), path, op, writeTime)
    {}

    // Executes the command by setting the translation onto the transform op.
    bool set(double x, double y, double z) override
    {
        fNewOpValue = V(x, y, z);

        setValue(fNewOpValue);
        return true;
    }
};

class UsdRotateOpUndoableCmd : public UsdTRSUndoableCmdBase {
public:
    UsdRotateOpUndoableCmd(
        const GfVec3f&        r,
        const Ufe::Path&      path,
        const UsdGeomXformOp& op,
        CvtRotXYZToAttrFn     cvt,
        const UsdTimeCode&    writeTime
    ) : UsdTRSUndoableCmdBase(VtValue(r), path, op, writeTime), 
        _cvtRotXYZToAttr(cvt)
    {}

    // Executes the command by setting the rotation onto the transform op.
    bool set(double x, double y, double z) override
    {
        fNewOpValue = _cvtRotXYZToAttr(x, y, z);

        setValue(fNewOpValue);
        return true;
    }

private:

    // Convert from UFE RotXYZ rotation to a value for the transform op.
    CvtRotXYZToAttrFn _cvtRotXYZToAttr;
};

}

UsdTransform3dMayaXformStack::UsdTransform3dMayaXformStack(
    const UsdSceneItem::Ptr& item, const std::vector<UsdGeomXformOp>& ops
)
    : UsdTransform3dBase(item), _xformable(prim())
{
    TF_AXIOM(_xformable);

    // *** FIXME ***  We ask for ordered transform ops, but the prim may have
    // other transform ops that are not in the ordered list.  However, those
    // transform ops are not contributing to the final local transform.
    for (const auto& op : ops) {
        std::string opName = op.GetOpName();
        auto ndx = opNameToNdx.at(opName);
        _orderedOps[ndx] = op;
    }
}

/* static */
UsdTransform3dMayaXformStack::Ptr UsdTransform3dMayaXformStack::create(
    const UsdSceneItem::Ptr& item, const std::vector<UsdGeomXformOp>& ops
)
{
    return std::make_shared<UsdTransform3dMayaXformStack>(item, ops);
}

Ufe::Vector3d UsdTransform3dMayaXformStack::translation() const
{
    return getVector3d<GfVec3d>(UsdGeomXformOp::GetOpName(
        UsdGeomXformOp::TypeTranslate, fallbackComponent));
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

Ufe::TranslateUndoableCommand::Ptr UsdTransform3dMayaXformStack::translateCmd(double x, double y, double z)
{
    return setVector3dCmd(GfVec3d(x, y, z), UsdGeomXformOp::GetOpName(
        UsdGeomXformOp::TypeTranslate, fallbackComponent), fallbackComponent);
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3dMayaXformStack::rotateCmd(double x, double y, double z)
{
    // If there is no rotate transform op yet, create it.
    UsdGeomXformOp r;
    GfVec3f v(x, y, z);
    CvtRotXYZToAttrFn cvt = toXYZ; // No conversion is default.
    if (!hasOp(NdxRotate)) {
        // Use notification guard, otherwise will generate one notification for
        // the xform op add, and another for the reorder.
        InTransform3dChange guard(path());
        r = _xformable.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, fallbackComponent);
        if (!TF_VERIFY(r)) {
            return nullptr;
        }
        r.Set(v);
        setXformOpOrder();
    }
    else {
        r = getOp(NdxRotate);
        cvt = getCvtRotXYZToAttrFn(r.GetOpName());
    }

    return std::make_shared<UsdRotateOpUndoableCmd>(
        v, path(), r, cvt, UsdTimeCode::Default());
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3dMayaXformStack::scaleCmd(double x, double y, double z)
{
    // If there is no scale transform op yet, create it.
    UsdGeomXformOp s;
    GfVec3f v(x, y, z);
    if (!hasOp(NdxScale)) {
        InTransform3dChange guard(path());
        s = _xformable.AddScaleOp(UsdGeomXformOp::PrecisionFloat, fallbackComponent);
        if (!TF_VERIFY(s)) {
            return nullptr;
        }
        s.Set(v);
        setXformOpOrder();
    }
    else {
        s = getOp(NdxScale);
    }

    return std::make_shared<UsdVecOpUndoableCmd<GfVec3f>>(
        v, path(), s, UsdTimeCode::Default());
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::rotatePivotCmd(double x, double y, double z)
{
    return pivotCmd(TfToken("maya_fallback:rotatePivot"), x, y, z);
}

Ufe::Vector3d UsdTransform3dMayaXformStack::rotatePivot() const
{
    return getVector3d<GfVec3f>(UsdGeomXformOp::GetOpName(
        UsdGeomXformOp::TypeTranslate, TfToken("maya_fallback:rotatePivot")));
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::scalePivotCmd(double x, double y, double z)
{
    return pivotCmd(TfToken("maya_fallback:scalePivot"), x, y, z);
}

Ufe::Vector3d UsdTransform3dMayaXformStack::scalePivot() const
{
    return getVector3d<GfVec3f>(UsdGeomXformOp::GetOpName(
        UsdGeomXformOp::TypeTranslate, TfToken("maya_fallback:scalePivot")));
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::translateRotatePivotCmd(
    double x, double y, double z)
{
    auto opSuffix = TfToken("maya_fallback:rotatePivotTranslate");
    auto attrName = UsdGeomXformOp::GetOpName(
        UsdGeomXformOp::TypeTranslate, opSuffix);
    return setVector3dCmd(GfVec3f(x, y, z), attrName, opSuffix);
}

Ufe::Vector3d UsdTransform3dMayaXformStack::rotatePivotTranslation() const
{
    return getVector3d<GfVec3f>(UsdGeomXformOp::GetOpName(
        UsdGeomXformOp::TypeTranslate,
        TfToken("maya_fallback:rotatePivotTranslate")));
}

Ufe::TranslateUndoableCommand::Ptr
UsdTransform3dMayaXformStack::translateScalePivotCmd(
    double x, double y, double z)
{
    auto opSuffix = TfToken("maya_fallback:scalePivotTranslate");
    auto attrName = UsdGeomXformOp::GetOpName(
        UsdGeomXformOp::TypeTranslate, opSuffix);
    return setVector3dCmd(GfVec3f(x, y, z), attrName, opSuffix);
}

Ufe::Vector3d UsdTransform3dMayaXformStack::scalePivotTranslation() const
{
    return getVector3d<GfVec3f>(UsdGeomXformOp::GetOpName(
        UsdGeomXformOp::TypeTranslate,
	TfToken("maya_fallback:scalePivotTranslate")));
}

template<class V>
Ufe::Vector3d UsdTransform3dMayaXformStack::getVector3d(
    const TfToken& attrName
) const
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

template<class V>
Ufe::SetVector3dUndoableCommand::Ptr
UsdTransform3dMayaXformStack::setVector3dCmd(
    const V& v, const TfToken& attrName, const TfToken& opSuffix
)
{
    UsdGeomXformOp op;

    // If the attribute doesn't exist yet, create it.
    auto attr = prim().GetAttribute(attrName);
    if (!attr) {
        InTransform3dChange guard(path());
        op = _xformable.AddTranslateOp(OpPrecision<V>::precision, opSuffix);
        if (!TF_VERIFY(op)) {
            return nullptr;
        }
        op.Set(v);
        setXformOpOrder();
    }
    else {
        op = UsdGeomXformOp(attr);
        TF_AXIOM(op);
    }

    return std::make_shared<UsdVecOpUndoableCmd<V>>(
        v, path(), op, UsdTimeCode::Default());
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3dMayaXformStack::pivotCmd(
    const TfToken& pvtOpSuffix,
    double x, double y, double z
)
{
    auto pvtAttrName = UsdGeomXformOp::GetOpName(
        UsdGeomXformOp::TypeTranslate, pvtOpSuffix);
    UsdGeomXformOp p;

    // If the pivot attribute pair doesn't exist yet, create it.
    auto attr = prim().GetAttribute(pvtAttrName);
    GfVec3f v(x, y, z);
    if (!attr) {
        // Without a notification guard each operation (each transform op
        // addition, setting the attribute value, and setting the transform op
        // order) will notify.  Observers would see an object in an inconsistent
        // state, especially after pivot is added but before its inverse is
        // added --- this does not match the Maya transform stack.  Use of
        // SdfChangeBlock is discouraged when calling USD APIs above Sdf, so
        // use our own guard.
        InTransform3dChange guard(path());
        p = _xformable.AddTranslateOp(UsdGeomXformOp::PrecisionFloat,
                                     pvtOpSuffix);
        auto pInv = _xformable.AddTranslateOp(UsdGeomXformOp::PrecisionFloat,
            pvtOpSuffix, /* isInverseOp */ true);
        if (!TF_VERIFY(p && pInv)) {
            return nullptr;
        }
        p.Set(v);
        setXformOpOrder();
    }
    else {
        p = UsdGeomXformOp(attr);
        TF_AXIOM(p);
    }

    return std::make_shared<UsdVecOpUndoableCmd<GfVec3f>>(
        v, path(), p, UsdTimeCode::Default());
}

void UsdTransform3dMayaXformStack::setXformOpOrder()
{
    // As this method is called after appending a transform op to the fallback
    // transform op sub-stack, we copy transform ops up to but excluding the
    // first op in the fallback transform op sub-stack.
    bool resetsXformStack = false;
    auto oldOrder = _xformable.GetOrderedXformOps(&resetsXformStack);
    auto i = findFirstFallbackOp(oldOrder); TF_AXIOM(i != oldOrder.end());

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
        std::string opName = op.GetOpName();
	auto ndx = opNameToNdx.at(opName);
	orderedOps[ndx] = op;
    }

    // Set the transform op order attribute, and rebuild our indexed cache.
    _orderedOps.clear();
    for (const auto& orderedOp : orderedOps) {
        const auto& op = orderedOp.second;
        newOrder.emplace_back(op);
        std::string opName = op.GetOpName();
        auto ndx = opNameToNdx.at(opName);
        _orderedOps[ndx] = op;
    }

    _xformable.SetXformOpOrder(newOrder, resetsXformStack);
}

bool UsdTransform3dMayaXformStack::hasOp(OpNdx ndx) const
{
    return _orderedOps.find(ndx) != _orderedOps.end();
}

UsdGeomXformOp UsdTransform3dMayaXformStack::getOp(OpNdx ndx) const
{
    return _orderedOps.at(ndx);
}

// segmentInclusiveMatrix() from UsdTransform3dBase is fine.

Ufe::Matrix4d UsdTransform3dMayaXformStack::segmentExclusiveMatrix() const
{
    // Get the parent transform plus all ops up to and excluding the first
    // fallback op.
    auto time = getTime(path());
    UsdGeomXformCache xformCache(time);
    auto parent = xformCache.GetParentToWorldTransform(prim());
    bool unused;
    auto ops = _xformable.GetOrderedXformOps(&unused);
    auto local = computeLocalExclusiveTransform(
        ops, findFirstFallbackOp(ops), time);
    return toUfe(local * parent);
}

//------------------------------------------------------------------------------
// UsdTransform3dMayaXformStackHandler
//------------------------------------------------------------------------------

UsdTransform3dMayaXformStackHandler::UsdTransform3dMayaXformStackHandler()
    : Ufe::Transform3dHandler()
{}

/*static*/
UsdTransform3dMayaXformStackHandler::Ptr UsdTransform3dMayaXformStackHandler::create()
{
    return std::make_shared<UsdTransform3dMayaXformStackHandler>();
}

Ufe::Transform3d::Ptr UsdTransform3dMayaXformStackHandler::transform3d(
    const Ufe::SceneItem::Ptr& item
) const
{
    return createTransform3d(item);
}

Ufe::Transform3d::Ptr UsdTransform3dMayaXformStackHandler::editTransform3d(
    const Ufe::SceneItem::Ptr& item
) const
{
    return createTransform3d(item);
}

} // namespace hack
} // namespace ufe
} // namespace MayaUsd

