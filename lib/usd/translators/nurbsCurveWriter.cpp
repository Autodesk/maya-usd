//
// Copyright 2016 Pixar
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
#include "nurbsCurveWriter.h"

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/base/gf/vec2d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/curves.h>
#include <pxr/usd/usdGeom/nurbsCurves.h>

#include <maya/MDoubleArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnFloatArrayData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MPointArray.h>

#include <numeric>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_WRITER(nurbsCurve, PxrUsdTranslators_NurbsCurveWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(nurbsCurve, UsdGeomNurbsCurves);

PxrUsdTranslators_NurbsCurveWriter::PxrUsdTranslators_NurbsCurveWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    if (!TF_VERIFY(GetDagPath().isValid())) {
        return;
    }
    MStatus       status = MS::kSuccess;
    MFnNurbsCurve curveFn(GetDagPath(), &status);
    MString       type = curveFn.typeName();

    isLinear = curveFn.degree() == 1;

    if (type == "bezierCurve" || isLinear) {
        UsdGeomBasisCurves primSchema = UsdGeomBasisCurves::Define(GetUsdStage(), GetUsdPath());
        if (!TF_VERIFY(
                primSchema,
                "Could not define UsdGeomBasisCurves at path '%s'\n",
                GetUsdPath().GetText())) {
            return;
        }
        _usdPrim = primSchema.GetPrim();
        if (!TF_VERIFY(
                _usdPrim,
                "Could not get UsdPrim for UsdGeomBasisCurves at path '%s'\n",
                primSchema.GetPath().GetText())) {
            return;
        }
    } else {
        UsdGeomNurbsCurves primSchema = UsdGeomNurbsCurves::Define(GetUsdStage(), GetUsdPath());
        if (!TF_VERIFY(
                primSchema,
                "Could not define UsdGeomNurbsCurves at path '%s'\n",
                GetUsdPath().GetText())) {
            return;
        }
        _usdPrim = primSchema.GetPrim();
        if (!TF_VERIFY(
                _usdPrim,
                "Could not get UsdPrim for UsdGeomNurbsCurves at path '%s'\n",
                primSchema.GetPath().GetText())) {
            return;
        }
    }
}

/* virtual */
void PxrUsdTranslators_NurbsCurveWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaPrimWriter::Write(usdTime);

    UsdGeomNurbsCurves primSchema(_usdPrim);
    writeNurbsCurveAttrs(usdTime, primSchema);
}

bool PxrUsdTranslators_NurbsCurveWriter::writeNurbsCurveAttrs(
    const UsdTimeCode&  usdTime,
    UsdGeomNurbsCurves& primSchema)
{
    MStatus status = MS::kSuccess;

    // Return if usdTime does not match if shape is animated
    if (usdTime.IsDefault() == _HasAnimCurves()) {
        // skip shape as the usdTime does not match if shape isAnimated value
        return true;
    }

    MFnDependencyNode fnDepNode(GetDagPath().node(), &status);
    MFnNurbsCurve     curveFn(GetDagPath(), &status);
    if (!status) {
        TF_RUNTIME_ERROR(
            "MFnNurbsCurve() failed for curve at DAG path: %s",
            GetDagPath().fullPathName().asChar());
        return false;
    }

    // How to repeat the end knots.
    bool                wrap = false;
    MFnNurbsCurve::Form form(curveFn.form());
    if (form == MFnNurbsCurve::kClosed || form == MFnNurbsCurve::kPeriodic) {
        wrap = true;
    }

    // Get curve attrs ======
    unsigned int numCurves = 1; // Assuming only 1 curve for now
    VtIntArray   curveOrder(numCurves);
    VtIntArray   curveVertexCounts(numCurves);
    VtFloatArray curveWidths(numCurves);
    VtVec2dArray ranges(numCurves);

    curveOrder[0] = curveFn.degree() + 1;
    curveVertexCounts[0] = curveFn.numCVs();
    if (!TF_VERIFY(curveOrder[0] <= curveVertexCounts[0])) {
        return false;
    }

    // Find the curve width attribute
    MObject widthObj;
    MPlug   widthPlug = curveFn.findPlug("widths", true, &status);
    if (!status) {
        TF_WARN(
            "No NURBS curves width(s) attribute found for path: %s",
            GetDagPath().fullPathName().asChar());
    } else {
        widthPlug.getValue(widthObj);
    }
    if (!widthObj.isNull() && widthObj.apiType() != MFn::kInvalid) {
        // Copy the widths from the found data
        if (widthObj.apiType() == MFn::kDoubleArrayData) {
            MFnDoubleArrayData widthArray;
            widthArray.setObject(widthObj);
            const uint32_t numElements = widthArray.length();
            curveWidths.resize(numElements);
            for (uint32_t i = 0; i < numElements; ++i) {
                curveWidths[i] = widthArray[i];
            }
        } else if (widthObj.apiType() == MFn::kFloatArrayData) {
            MFnFloatArrayData widthArray;
            widthArray.setObject(widthObj);
            const uint32_t numElements = widthArray.length();
            curveWidths.resize(numElements);
            for (uint32_t i = 0; i < numElements; ++i) {
                curveWidths[i] = widthArray[i];
            }
        }
    } else if (
        MFnNumericAttribute(widthPlug.attribute()).unitType() == MFnNumericData::kDouble
        || MFnNumericAttribute(widthPlug.attribute()).unitType() == MFnNumericData::kFloat) {
        // Copy the widths from the plug value
        curveWidths.push_back(widthPlug.asFloat());
    } else {
        // Default to a contant width of 1.0f
        curveWidths[0] = 1;
    }

    double mayaKnotDomainMin;
    double mayaKnotDomainMax;
    status = curveFn.getKnotDomain(mayaKnotDomainMin, mayaKnotDomainMax);
    CHECK_MSTATUS_AND_RETURN(status, false);
    ranges[0][0] = mayaKnotDomainMin;
    ranges[0][1] = mayaKnotDomainMax;

    MPointArray mayaCurveCVs;
    status = curveFn.getCVs(mayaCurveCVs, MSpace::kObject);
    CHECK_MSTATUS_AND_RETURN(status, false);
    VtVec3fArray points(mayaCurveCVs.length()); // all CVs batched together
    for (unsigned int i = 0; i < mayaCurveCVs.length(); i++) {
        points[i].Set(mayaCurveCVs[i].x, mayaCurveCVs[i].y, mayaCurveCVs[i].z);
    }

    MDoubleArray mayaCurveKnots;
    status = curveFn.getKnots(mayaCurveKnots);
    CHECK_MSTATUS_AND_RETURN(status, false);
    const uint32_t mayaKnotsCount = mayaCurveKnots.length();

    // USD requires 2 additional knots
    VtDoubleArray curveKnots(mayaKnotsCount + 2);
    for (uint32_t i = 0; i < mayaKnotsCount; i++) {
        curveKnots[i + 1] = mayaCurveKnots[i];
    }
    if (wrap) {
        // Set end knots according to the USD spec for periodic curves
        curveKnots[0] = curveKnots[1]
            - (curveKnots[curveKnots.size() - 2] - curveKnots[curveKnots.size() - 3]);
        curveKnots[curveKnots.size() - 1]
            = curveKnots[curveKnots.size() - 2] + (curveKnots[2] - curveKnots[1]);
    } else {
        // Set end knots according to the USD spec for non-periodic curves
        curveKnots[0] = curveKnots[1];
        curveKnots[curveKnots.size() - 1] = curveKnots[curveKnots.size() - 2];
    }

    // Gprim
    VtVec3fArray extent(2);
    UsdGeomCurves::ComputeExtent(points, curveWidths, &extent);
    UsdMayaWriteUtil::SetAttribute(
        primSchema.CreateExtentAttr(), &extent, usdTime, _GetSparseValueWriter());

    // find the number of segments: (vertexCount - order + 1) per curve
    // varying interpolation is number of segments + number of curves
    size_t accumulatedVertexCount
        = std::accumulate(curveVertexCounts.begin(), curveVertexCounts.end(), 0);
    size_t accumulatedOrder = std::accumulate(curveOrder.begin(), curveOrder.end(), 0);
    size_t expectedSegmentCount
        = accumulatedVertexCount - accumulatedOrder + curveVertexCounts.size();
    size_t expectedVaryingSize = expectedSegmentCount + curveVertexCounts.size();

    if (curveWidths.size() == 1)
        primSchema.SetWidthsInterpolation(UsdGeomTokens->constant);
    else if (curveWidths.size() == points.size())
        primSchema.SetWidthsInterpolation(UsdGeomTokens->vertex);
    else if (curveWidths.size() == curveVertexCounts.size())
        primSchema.SetWidthsInterpolation(UsdGeomTokens->uniform);
    else if (curveWidths.size() == expectedVaryingSize)
        primSchema.SetWidthsInterpolation(UsdGeomTokens->varying);
    else {
        TF_WARN(
            "MFnNurbsCurve has unsupported width size "
            "for standard interpolation metadata: %s",
            GetDagPath().fullPathName().asChar());
    }

    if (curveFn.typeName() == "bezierCurve" || isLinear) {
        UsdGeomBasisCurves primSchemaBasis(_usdPrim);
        size_t             pntCnt = points.size();
        VtVec3fArray       linearArray;

        if (!isLinear) {
            for (size_t i = 0; i < pntCnt - 3; i += 3) {
                // check if out and in handles are coincident
                GfVec3f h1 = points[i + 1] - points[i];
                GfVec3f h2 = points[i + 3] - points[i + 2];

                if (GfIsClose(h1, h2, 1e-5)) {
                    if (linearArray.empty())
                        linearArray.push_back(points[i]);
                    linearArray.push_back(points[i + 3]);
                }
            }

            if (!wrap && linearArray.size() == ((pntCnt - 4) / 3) + 2) {
                points = linearArray;
                curveVertexCounts[0] = linearArray.size();
                isLinear = true;
            }
        }

        if (isLinear) {
            primSchemaBasis.CreateTypeAttr(VtValue(TfToken("linear")));
        } else {
            primSchemaBasis.CreateTypeAttr(VtValue(TfToken("cubic")));
            primSchemaBasis.CreateBasisAttr(VtValue(TfToken("bezier"))); // HARDCODED
        }
    } else {
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetOrderAttr(), curveOrder, UsdTimeCode::Default(), _GetSparseValueWriter());
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetKnotsAttr(),
            &curveKnots,
            UsdTimeCode::Default(),
            _GetSparseValueWriter()); // not animatable
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetRangesAttr(),
            &ranges,
            UsdTimeCode::Default(),
            _GetSparseValueWriter()); // not animatable
    }
    // Curve
    // not animatable
    UsdMayaWriteUtil::SetAttribute(
        primSchema.GetCurveVertexCountsAttr(),
        &curveVertexCounts,
        UsdTimeCode::Default(),
        _GetSparseValueWriter());
    UsdMayaWriteUtil::SetAttribute(
        primSchema.GetWidthsAttr(), &curveWidths, UsdTimeCode::Default(), _GetSparseValueWriter());
    UsdMayaWriteUtil::SetAttribute(
        primSchema.GetPointsAttr(), &points, usdTime, _GetSparseValueWriter()); // CVs

    // TODO: Handle periodic and non-periodic cases

    return true;
}

/* virtual */
bool PxrUsdTranslators_NurbsCurveWriter::ExportsGprims() const { return true; }

PXR_NAMESPACE_CLOSE_SCOPE
