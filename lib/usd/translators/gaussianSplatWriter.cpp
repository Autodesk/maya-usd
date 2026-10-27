//
// Copyright 2026 Autodesk
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
#include "gaussianSplatWriter.h"

#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/utils/gaussianSplatPly.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/base/gf/quatf.h>
#include <pxr/base/gf/range3f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/xformOp.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdVol/particleField3DGaussianSplat.h>

#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MString.h>

#include <ghc/fs_std.hpp>

#include <algorithm>
#include <cmath>
namespace MAYAUSD_NS_DEF {

namespace registration {
PXR_NAMESPACE_USING_DIRECTIVE
PXRUSDMAYA_REGISTER_CUSTOM_WRITER(aiGaussianSplat, GaussianSplatWriter);
} // namespace registration
namespace {

// mtoa's up-axis enum on aiGaussianSplat: 0=Y, 1=Z, 2=-Y, 3=-Z. The raw .ply is not
// up-axis-corrected (unlike mtoa's decoded out* plugs), so this correction is what makes the
// USD result match the Maya viewport.
enum class GSUpAxis
{
    kYUp = 0,
    kZUp = 1,
    kNegYUp = 2,
    kNegZUp = 3
};

bool resolvePlyPath(
    const MFnDependencyNode& depFn,
    const MObject&           mayaObj,
    const MDagPath&          dagPath,
    std::string*             out)
{
    MStatus status;
    MString filename = depFn.findPlug("filename", true, &status).asString();
    if (status != MS::kSuccess || filename.length() == 0) {
        TF_WARN(
            "%s: aiGaussianSplat has no filename specified; skipping.",
            dagPath.fullPathName().asChar());
        return false;
    }

    filename = filename.expandEnvironmentVariablesAndTilde();

    fs::filesystem::path plyPath(filename.asChar());
    if (!plyPath.is_absolute()) {
        const std::string resolved
            = PXR_NS::UsdMayaUtilFileSystem::resolveRelativePathWithinMayaContext(
                mayaObj, filename.asChar());
        if (!resolved.empty()) {
            plyPath = resolved;
        }
    }

    const MPlug useFilePlug = depFn.findPlug("useFile", true, &status);
    if (status == MS::kSuccess && !useFilePlug.isNull() && !useFilePlug.asBool()) {
        TF_WARN(
            "%s: useFile is off; maya-usd always exports the full, unfiltered .ply file.",
            dagPath.fullPathName().asChar());
    }

    const MPlug maskMeshPlug = depFn.findPlug("maskMesh", true, &status);
    if (status == MS::kSuccess && !maskMeshPlug.isNull() && maskMeshPlug.isConnected()) {
        TF_WARN(
            "%s: a maskMesh is connected; maya-usd ignores it and exports the full, "
            "unfiltered .ply file.",
            dagPath.fullPathName().asChar());
    }

    std::error_code existsError;
    if (!fs::filesystem::exists(plyPath, existsError) || existsError) {
        TF_WARN(
            "%s: .ply file not found at '%s'.",
            dagPath.fullPathName().asChar(),
            plyPath.generic_string().c_str());
        return false;
    }

    *out = plyPath.generic_string();
    return true;
}

PXR_NS::VtVec3fArray toVec3fArray(const std::vector<float>& src)
{
    static_assert(sizeof(PXR_NS::GfVec3f) == 3 * sizeof(float), "GfVec3f must be tightly packed");
    PXR_NS::VtVec3fArray array(src.size() / 3);
    std::copy_n(src.data(), array.size() * 3, reinterpret_cast<float*>(array.data()));
    return array;
}

// The PLY rotations are stored as (w, x, y, z), but GfQuatf's memory layout is (x, y, z, w), so
// the components cannot be copied as a block.
PXR_NS::VtQuatfArray toQuatfArray(const std::vector<float>& src)
{
    PXR_NS::VtQuatfArray array(src.size() / 4);
    for (size_t i = 0; i < array.size(); ++i) {
        const float* q = &src[i * 4];
        array[i] = PXR_NS::GfQuatf(q[0], q[1], q[2], q[3]);
    }
    return array;
}

PXR_NS::VtFloatArray toFloatArray(const std::vector<float>& src)
{
    return PXR_NS::VtFloatArray(src.begin(), src.end());
}

// Scales are half-axis lengths of each splat's ellipsoid. Padding by the largest one is a
// conservative, rotation-independent bound; it is not a tight (or 3-sigma) extent.
PXR_NS::GfRange3f
computeExtent(const PXR_NS::VtVec3fArray& positions, const PXR_NS::VtVec3fArray& scales)
{
    PXR_NS::GfRange3f extent;
    for (size_t i = 0; i < positions.size(); ++i) {
        const PXR_NS::GfVec3f& p = positions[i];
        const PXR_NS::GfVec3f& s = scales[i];
        const float pad = std::max({ std::fabs(s[0]), std::fabs(s[1]), std::fabs(s[2]) });
        extent.UnionWith(PXR_NS::GfRange3f(p - PXR_NS::GfVec3f(pad), p + PXR_NS::GfVec3f(pad)));
    }
    return extent;
}

bool hasRotateAxis(const PXR_NS::UsdGeomXformable& xformable)
{
    bool resetsXformStack = false;
    for (const PXR_NS::UsdGeomXformOp& op : xformable.GetOrderedXformOps(&resetsXformStack)) {
        if (!op.IsInverseOp()
            && PXR_NS::TfStringEndsWith(op.GetName().GetString(), ":rotateAxis")) {
            return true;
        }
    }
    return false;
}

bool addUpAxisOp(PXR_NS::UsdGeomXformable& xformable, double xRot)
{
    bool       resetsXformStack = false;
    const auto originalOps = xformable.GetOrderedXformOps(&resetsXformStack);
    auto       ops = originalOps;

    PXR_NS::UsdGeomXformOp upAxisOp = xformable.AddRotateXOp(
        PXR_NS::UsdGeomXformOp::PrecisionDouble, PXR_NS::TfToken("rotateAxis"));
    if (!upAxisOp || !upAxisOp.Set(xRot)) {
        xformable.SetXformOpOrder(originalOps, resetsXformStack);
        return false;
    }

    // Ops that Maya applies after rotateAxis. Shear is matched by name since it is a
    // TypeTransform op, like offsetParentMatrix, which comes before everything.
    const auto isAfterRotate = [](const PXR_NS::UsdGeomXformOp& op) {
        const std::string& name = op.GetName().GetString();
        return op.IsInverseOp() || op.GetOpType() == PXR_NS::UsdGeomXformOp::TypeScale
            || PXR_NS::TfStringEndsWith(name, ":shear")
            || name.find("scalePivot") != std::string::npos;
    };
    // Scan from the end: with worldspace export, the parent's ops come first in the stack.
    ops.insert(std::find_if_not(ops.rbegin(), ops.rend(), isAfterRotate).base(), upAxisOp);
    if (!xformable.SetXformOpOrder(ops, resetsXformStack)) {
        xformable.SetXformOpOrder(originalOps, resetsXformStack);
        return false;
    }
    return true;
}

} // namespace

/* static */
PXR_NS::UsdMayaPrimWriter::ContextSupport GaussianSplatWriter::CanExport(
    const PXR_NS::UsdMayaJobExportArgs& /*exportArgs*/,
    const MObject& /*exportObj*/)
{
    return ContextSupport::Supported;
}

GaussianSplatWriter::GaussianSplatWriter(
    const MFnDependencyNode&        depNodeFn,
    const PXR_NS::SdfPath&          usdPath,
    PXR_NS::UsdMayaWriteJobContext& jobCtx)
    : PXR_NS::UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    if (!TF_VERIFY(GetDagPath().isValid())) {
        return;
    }

    PXR_NS::UsdVolParticleField3DGaussianSplat primSchema
        = PXR_NS::UsdVolParticleField3DGaussianSplat::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            primSchema,
            "Could not define UsdVolParticleField3DGaussianSplat at path <%s>\n",
            GetUsdPath().GetText())) {
        return;
    }

    _usdPrim = primSchema.GetPrim();
    TF_VERIFY(
        _usdPrim,
        "Could not get UsdPrim for UsdVolParticleField3DGaussianSplat at path <%s>\n",
        primSchema.GetPath().GetText());
}

/* virtual */
void GaussianSplatWriter::Write(const PXR_NS::UsdTimeCode& usdTime)
{
    PXR_NS::UsdMayaPrimWriter::Write(usdTime);

    // The cloud parsed from the .ply file is static, so it is only written at the default
    // time, which the write job always visits first.
    if (!usdTime.IsDefault()) {
        return;
    }

    MStatus                 status;
    const MFnDependencyNode depFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    std::string plyPath;
    if (!resolvePlyPath(depFn, GetMayaObject(), GetDagPath(), &plyPath)) {
        return;
    }

    utils::SplatCloud cloud;
    std::string       errorMsg;
    if (!utils::loadSplatPly(plyPath, &cloud, &errorMsg)) {
        TF_RUNTIME_ERROR("%s: %s", GetDagPath().fullPathName().asChar(), errorMsg.c_str());
        return;
    }

    if (cloud.count() == 0 || !cloud.isValid()) {
        TF_WARN(
            "%s: '%s' contains no valid splat data; skipping.",
            GetDagPath().fullPathName().asChar(),
            plyPath.c_str());
        return;
    }

    PXR_NS::VtVec3fArray positions = toVec3fArray(cloud._positions);
    PXR_NS::VtVec3fArray scales = toVec3fArray(cloud._scales);
    PXR_NS::VtQuatfArray orientations = toQuatfArray(cloud._rotations);
    PXR_NS::VtFloatArray opacities = toFloatArray(cloud._opacities);
    PXR_NS::VtVec3fArray shCoeffs = toVec3fArray(cloud._shCoeffs);

    // Must run before positions/scales are handed to the pointer-based SetScaledAttribute
    // calls below, which VtValue::Take them and leave them default-constructed.
    const PXR_NS::GfRange3f extent = computeExtent(positions, scales);

    PXR_NS::UsdVolParticleField3DGaussianSplat primSchema(_usdPrim);
    PXR_NS::FlexibleSparseValueWriter*         valueWriter = _GetSparseValueWriter();

    PXR_NS::UsdMayaWriteUtil::SetScaledAttribute(
        primSchema.CreatePositionsAttr(),
        &positions,
        _metersPerUnitScalingFactor,
        usdTime,
        valueWriter);
    PXR_NS::UsdMayaWriteUtil::SetScaledAttribute(
        primSchema.CreateScalesAttr(), &scales, _metersPerUnitScalingFactor, usdTime, valueWriter);
    PXR_NS::UsdMayaWriteUtil::SetAttribute(
        primSchema.CreateOrientationsAttr(), &orientations, usdTime, valueWriter);
    PXR_NS::UsdMayaWriteUtil::SetAttribute(
        primSchema.CreateOpacitiesAttr(), &opacities, usdTime, valueWriter);

    PXR_NS::UsdAttribute shAttr = primSchema.CreateRadianceSphericalHarmonicsCoefficientsAttr();
    PXR_NS::UsdMayaWriteUtil::SetAttribute(shAttr, &shCoeffs, usdTime, valueWriter);
    // The degree must always be authored, even when it equals the schema fallback (3):
    // Hydra only sees authored values, and without it ignores the coefficients, which
    // renders every splat grey. The sparse value writer would skip a fallback value.
    primSchema.CreateRadianceSphericalHarmonicsDegreeAttr(PXR_NS::VtValue(cloud._shDegree));

    const MPlug inputColorSpacePlug = depFn.findPlug("inputColorSpace", true, &status);
    if (status == MS::kSuccess && !inputColorSpacePlug.isNull()) {
        const MString colorSpace = inputColorSpacePlug.asString();
        if (colorSpace.length() > 0) {
            shAttr.SetColorSpace(PXR_NS::TfToken(colorSpace.asChar()));
        }
    }

    PXR_NS::VtVec3fArray extentArray(2);
    extentArray[0] = extent.GetMin();
    extentArray[1] = extent.GetMax();
    PXR_NS::UsdMayaWriteUtil::SetScaledAttribute(
        primSchema.CreateExtentAttr(),
        &extentArray,
        _metersPerUnitScalingFactor,
        usdTime,
        valueWriter);

    const MPlug upAxisPlug = depFn.findPlug("upAxis", true, &status);
    if (status != MS::kSuccess || upAxisPlug.isNull()) {
        return;
    }

    const short    upAxisValue = upAxisPlug.asShort();
    const GSUpAxis upAxis = static_cast<GSUpAxis>(upAxisValue);
    double         xRot { 0.0 };
    switch (upAxis) {
    case GSUpAxis::kYUp: xRot = 0.0; break;
    case GSUpAxis::kZUp: xRot = -90.0; break;
    case GSUpAxis::kNegYUp: xRot = 180.0; break;
    case GSUpAxis::kNegZUp: xRot = 90.0; break;
    default:
        TF_WARN(
            "%s: unknown upAxis value %d; the up axis is not exported.",
            GetDagPath().fullPathName().asChar(),
            upAxisValue);
        return;
    }
    if (xRot == 0.0) {
        return;
    }

    PXR_NS::UsdGeomXformable xformable(_usdPrim);
    if (hasRotateAxis(xformable)) {
        TF_WARN(
            "%s: the transform already has a rotateAxis; the up axis is not exported.",
            GetDagPath().fullPathName().asChar());
    } else {
        if (upAxis == GSUpAxis::kNegYUp || upAxis == GSUpAxis::kNegZUp) {
            TF_WARN(
                "%s: node's upAxis is a negative axis, which has no USD stage "
                "up-axis equivalent; baking the corresponding rotateX correction "
                "anyway.",
                GetDagPath().fullPathName().asChar());
        }
        if (!addUpAxisOp(xformable, xRot)) {
            TF_WARN("%s: failed to export the up axis.", GetDagPath().fullPathName().asChar());
        }
    }
}

} // namespace MAYAUSD_NS_DEF
