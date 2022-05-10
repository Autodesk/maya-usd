//
// Copyright 2017 Animal Logic
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

#include "AL/usdmaya/utils/NurbsCurveUtils.h"

#include "AL/maya/utils/NodeHelper.h"
#include "AL/usdmaya/utils/DgNodeHelper.h"
#include "AL/usdmaya/utils/Utils.h"

#include <mayaUsdUtils/DiffCore.h>

#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MDoubleArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnFloatArrayData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>

#include <iostream>

namespace AL {
namespace usdmaya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
void convert3DFloatArrayTo4DDoubleArray(
    const float* const input,
    double* const      output,
    size_t             count)
{
    for (size_t i = 0, j = 0, n = count * 4; i != n; i += 4, j += 3) {
        output[i] = input[j];
        output[i + 1] = input[j + 1];
        output[i + 2] = input[j + 2];
        output[i + 3] = 1.0;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void copyPoints(const MFnNurbsCurve& fnCurve, const UsdAttribute& pointsAttr, UsdTimeCode time)
{
    MPointArray controlVertices;
    fnCurve.getCVs(controlVertices);
    const unsigned int cvCount = controlVertices.length();
    VtArray<GfVec3f>   dataPoints(cvCount);

    float* const        usdPoints = (float* const)dataPoints.cdata();
    const double* const mayaCVs = (const double* const) & controlVertices[0];

    convertDoubleVec4ArrayToFloatVec3Array(mayaCVs, usdPoints, cvCount);
    pointsAttr.Set(dataPoints, time);
}

//----------------------------------------------------------------------------------------------------------------------
void copyExtent(const MFnNurbsCurve& fnCurve, const UsdGeomNurbsCurves& nurbs, UsdTimeCode time)
{
    MPointArray controlVertices;
    fnCurve.getCVs(controlVertices);
    const unsigned int cvCount = controlVertices.length();
    VtArray<GfVec3f>   dataPoints(cvCount);

    float* const        usdPoints = (float* const)dataPoints.cdata();
    const double* const mayaCVs = (const double* const) & controlVertices[0];

    convertDoubleVec4ArrayToFloatVec3Array(mayaCVs, usdPoints, cvCount);

    // Extents calculation requires widths - set default width if empty or unfound for prim
    VtFloatArray curveWidths;
    nurbs.GetWidthsAttr().Get<VtFloatArray>(&curveWidths);
    if (curveWidths.empty()) {
        curveWidths.push_back(1.0f);
    }

    VtArray<GfVec3f> mayaExtent(2);
    UsdGeomCurves::ComputeExtent(dataPoints, curveWidths, &mayaExtent);
    nurbs.GetExtentAttr().Set(mayaExtent, time);
}

//----------------------------------------------------------------------------------------------------------------------
void copyCurveVertexCounts(
    const MFnNurbsCurve& fnCurve,
    const UsdAttribute&  countsAttr,
    UsdTimeCode          time)
{
    VtArray<int32_t> dataCurveVertexCounts(1, fnCurve.numCVs());
    countsAttr.Set(dataCurveVertexCounts, time);
}

//----------------------------------------------------------------------------------------------------------------------
void copyKnots(const MFnNurbsCurve& fnCurve, const UsdAttribute& knotsAttr, UsdTimeCode time)
{
    MDoubleArray knots;
    fnCurve.getKnots(knots);
    const uint32_t  count = knots.length();
    VtArray<double> dataKnots(count);
    memcpy((double*)dataKnots.cdata(), (const double*)&knots[0], sizeof(double) * count);
    knotsAttr.Set(dataKnots, time);
}

//----------------------------------------------------------------------------------------------------------------------
void copyRanges(const MFnNurbsCurve& fnCurve, const UsdAttribute& rangesAttr, UsdTimeCode time)
{
    double start, end;
    fnCurve.getKnotDomain(start, end);
    VtArray<GfVec2d> dataRanges(1, GfVec2d(start, end));
    rangesAttr.Set(dataRanges);
}

//----------------------------------------------------------------------------------------------------------------------
void copyOrder(const MFnNurbsCurve& fnCurve, const UsdAttribute& orderAttr, UsdTimeCode time)
{
    VtArray<int32_t> dataOrders(1, fnCurve.degree() + 1);
    orderAttr.Set(dataOrders);
}

//----------------------------------------------------------------------------------------------------------------------
void copyWidths(
    const MObject&            widthObj,
    const MPlug&              widthPlug,
    const MFnDoubleArrayData& widthArray,
    const UsdAttribute&       widthsAttr,
    UsdTimeCode               time)
{
    VtArray<float> dataWidths;
    if (widthObj.apiType() == MFn::kDoubleArrayData) {
        const uint32_t numElements = widthArray.length();
        dataWidths.resize(numElements);
        for (uint32_t i = 0; i < numElements; ++i) {
            dataWidths[i] = widthArray[i];
        }
        widthsAttr.Set(dataWidths);
    }
}
//----------------------------------------------------------------------------------------------------------------------
void copyWidths(
    const MObject&           widthObj,
    const MPlug&             widthPlug,
    const MFnFloatArrayData& widthArray,
    const UsdAttribute&      widthsAttr,
    UsdTimeCode              time)
{
    VtArray<float> dataWidths;
    if (widthObj.apiType() == MFn::kFloatArrayData) {
        const uint32_t numElements = widthArray.length();
        dataWidths.resize(numElements);
        for (uint32_t i = 0; i < numElements; ++i) {
            dataWidths[i] = widthArray[i];
        }
        widthsAttr.Set(dataWidths);
    }
}

//----------------------------------------------------------------------------------------------------------------------
bool getMayaCurveWidth(
    const MFnNurbsCurve& fnCurve,
    MObject&             object,
    MPlug&               plug,
    MFnDoubleArrayData&  array)
{
    // Width of the NURB curve
    MStatus status;
    plug = fnCurve.findPlug("widths", true, &status);
    if (status == MStatus::kSuccess) {
        plug.getValue(object);
        array.setObject(object);
        return true;
    }
    // "widths" not founded, try "width"
    plug = fnCurve.findPlug("width", true, &status);
    if (status == MStatus::kSuccess) {
        plug.getValue(object);
        array.setObject(object);
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool getMayaCurveWidth(const MFnNurbsCurve& fnCurve, MObject& object, MPlug& plug)
{
    // Width of the NURB curve
    MStatus status;
    plug = fnCurve.findPlug("widths", true, &status);
    if (status == MStatus::kSuccess) {
        plug.getValue(object);
        return true;
    }
    // "widths" not founded, try "width"
    plug = fnCurve.findPlug("width", true, &status);
    if (status == MStatus::kSuccess) {
        plug.getValue(object);
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool createMayaCurves(
    MFnNurbsCurve&            fnCurve,
    MObject&                  parent,
    const UsdGeomNurbsCurves& usdCurves,
    bool                      parentUnmerged)
{
    VtArray<int32_t> dataOrder;
    VtArray<int32_t> dataCurveVertexCounts;
    VtArray<GfVec3f> dataPoints;
    VtArray<double>  dataKnots;

    usdCurves.GetOrderAttr().Get(&dataOrder);
    if (dataOrder.empty()) {
        return false;
    }
    usdCurves.GetCurveVertexCountsAttr().Get(&dataCurveVertexCounts);
    if (dataCurveVertexCounts.empty()) {
        return false;
    }
    usdCurves.GetPointsAttr().Get(&dataPoints);
    if (dataPoints.empty()) {
        return false;
    }
    usdCurves.GetKnotsAttr().Get(&dataKnots);
    if (dataKnots.empty()) {
        return false;
    }

    MPointArray  controlVertices;
    MDoubleArray knotSequences;
    MDoubleArray curveWidths;

    size_t currentPointIndex = 0;
    size_t currentKnotIndex = 0;
    for (size_t i = 0, ncurves = dataCurveVertexCounts.size(); i < ncurves; ++i) {
        const int32_t numPoints = dataCurveVertexCounts[i];
        controlVertices.setLength(numPoints);

        const int32_t numKnots = numPoints + dataOrder[i] - 2;
        knotSequences.setLength(numKnots);

        const float*  pstart = (const float*)&dataPoints[currentPointIndex];
        const double* kstart = &dataKnots[currentKnotIndex];
        memcpy(&knotSequences[0], kstart, sizeof(double) * numKnots);

        currentPointIndex += numPoints;
        currentKnotIndex += numKnots;

        AL::usdmaya::utils::convert3DFloatArrayTo4DDoubleArray(
            pstart, (double*)&controlVertices[0], numPoints);
        fnCurve.create(
            controlVertices,
            knotSequences,
            dataOrder[i] - 1,
            MFnNurbsCurve::kOpen,
            false,
            false,
            parent);
    }

    if (UsdAttribute widthsAttr = usdCurves.GetWidthsAttr()) {
        const uint32_t flags = AL::maya::utils::NodeHelper::kReadable
            | AL::maya::utils::NodeHelper::kWritable | AL::maya::utils::NodeHelper::kStorable
            | AL::maya::utils::NodeHelper::kDynamic;

        VtArray<float> dataWidths;
        widthsAttr.Get(&dataWidths);

        if (dataWidths.size() == 1) {
            float   value = dataWidths[0];
            MObject objAttr;
            AL::maya::utils::NodeHelper::addFloatAttr(
                fnCurve.object(), "width", "width", 0.f, flags, &objAttr);
            if (!objAttr.isNull()) {
                AL::usdmaya::utils::DgNodeHelper::setFloat(fnCurve.object(), objAttr, value);
            } else {
                std::cerr << "createNode:addFloatAttr returned an invalid object" << std::endl;
            }
            MGlobal::executeCommand(MString("aliasAttr widths ") + fnCurve.name() + ".width");
        } else if (dataWidths.size() > 1) {
            MObject objAttr = AL::maya::utils::NodeHelper::addFloatArrayAttr(
                fnCurve.object(), "width", "width", flags);
            if (!objAttr.isNull()) {
                AL::usdmaya::utils::DgNodeHelper::setUsdFloatArray(
                    fnCurve.object(), objAttr, dataWidths);
            } else {
                std::cerr << "createNode:addFloatArrayAttr returned an invalid object" << std::endl;
            }
            MGlobal::executeCommand(MString("aliasAttr widths ") + fnCurve.name() + ".width");
        }
    }

    MString dagName = AL::usdmaya::utils::convert(usdCurves.GetPrim().GetName());
    if (!parentUnmerged) {
        dagName += "Shape";
    }
    fnCurve.setName(dagName);

    return true;
}

//----------------------------------------------------------------------------------------------------------------------
void copyNurbsCurveBindPoseData(
    MFnNurbsCurve&      fnCurve,
    UsdGeomNurbsCurves& usdCurves,
    UsdTimeCode         time)
{
    UsdGeomPrimvar pRefPrimVarAttr = usdCurves.CreatePrimvar(
        UsdUtilsGetPrefName(), SdfValueTypeNames->Point3fArray, UsdGeomTokens->vertex);

    if (pRefPrimVarAttr) {
        MStatus          status;
        const uint32_t   numVertices = fnCurve.numCVs();
        VtArray<GfVec3f> pref(numVertices);
        MPointArray      points;
        status = fnCurve.getCVs(points, MSpace::kObject);
        if (status) {
            for (uint32_t i = 0; i < numVertices; ++i) {
                pref[i][0] = float(points[i].x);
                pref[i][1] = float(points[i].y);
                pref[i][2] = float(points[i].z);
            }
            pRefPrimVarAttr.Set(pref, time);
        } else {
            MGlobal::displayError(
                MString("Unable to access mesh vertices on nurbs curve: ")
                + fnCurve.fullPathName());
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
uint32_t diffNurbsCurve(
    UsdGeomNurbsCurves& usdCurves,
    MFnNurbsCurve&      fnCurve,
    UsdTimeCode         timeCode,
    uint32_t            exportMask)
{
    uint32_t result = 0;
    if (exportMask & kCurvePoints) {
        MPointArray controlVertices;
        fnCurve.getCVs(controlVertices);

        VtArray<GfVec3f> dataPoints;

        const size_t        numControlVertices = controlVertices.length();
        const size_t        numPoints = dataPoints.size();
        const float* const  usdPoints = (const float* const)dataPoints.cdata();
        const double* const mayaCVs = (const double* const) & controlVertices[0];
        if (!MayaUsdUtils::compareArray3Dto4D(usdPoints, mayaCVs, numPoints, numControlVertices)) {
            result |= kCurvePoints;
        }
    }
    if (exportMask & kCurveExtent) {
        MPointArray controlVertices;
        fnCurve.getCVs(controlVertices);

        const unsigned int  cvCount = controlVertices.length();
        VtArray<GfVec3f>    points(cvCount);
        float* const        dataPoints = (float* const)points.cdata();
        const double* const mayaCVs = (const double* const) & controlVertices[0];
        convertDoubleVec4ArrayToFloatVec3Array(mayaCVs, dataPoints, cvCount);

        VtArray<GfVec3f> mayaExtent(2);
        UsdGeomPointBased::ComputeExtent(points, &mayaExtent);

        VtArray<GfVec3f> usdExtent(2);
        usdCurves.GetExtentAttr().Get(&usdExtent, timeCode);

        if (usdExtent != mayaExtent)
            result |= kCurveExtent;
    }

    if (exportMask & kCurveVertexCounts) {
        int numCVs = fnCurve.numCVs();

        VtArray<int32_t> dataCurveVertexCounts;
        usdCurves.GetCurveVertexCountsAttr().Get(&dataCurveVertexCounts);

        if (dataCurveVertexCounts.size() != 1 || dataCurveVertexCounts[0] != numCVs) {
            result |= kCurveVertexCounts;
        }
    }
    if (exportMask & kKnots) {
        MDoubleArray knots;
        fnCurve.getKnots(knots);

        VtArray<double> dataKnots;
        usdCurves.GetKnotsAttr().Get(&dataKnots);

        const size_t        numKnots = dataKnots.size();
        const size_t        numMayaKnots = knots.length();
        const double* const usdKnots = (const double* const)dataKnots.cdata();
        const double* const mayaKnots = (const double* const) & knots[0];
        if (!MayaUsdUtils::compareArray(usdKnots, mayaKnots, numKnots, numMayaKnots)) {
            result |= kKnots;
        }
    }
    if (exportMask & kRanges) {
        double knotDomain[2];
        fnCurve.getKnotDomain(knotDomain[0], knotDomain[1]);

        VtArray<GfVec2d> dataRanges;
        usdCurves.GetRangesAttr().Get(&dataRanges);
        const float* const usdRanges = (const float* const)dataRanges.cdata();
        if (dataRanges.size() != 1 || !MayaUsdUtils::compareArray(usdRanges, knotDomain, 2, 2)) {
            result |= kRanges;
        }
    }
    if (exportMask & kOrder) {
        int              degree = fnCurve.degree();
        VtArray<int32_t> dataOrders;
        usdCurves.GetOrderAttr().Get(&dataOrders);
        if (dataOrders.size() != 1 || dataOrders[0] != degree + 1) {
            result |= kOrder;
        }
    }
    if (exportMask & kWidths) {
        MPlug              widthPlug;
        MObject            widthObj;
        MFnDoubleArrayData widthArrayData;
        getMayaCurveWidth(fnCurve, widthObj, widthPlug, widthArrayData);

        if (!widthObj.isNull() && !widthPlug.isNull()) {
            VtArray<float> dataWidths;
            usdCurves.GetWidthsAttr().Get(&dataWidths);

            if (widthObj.apiType() == MFn::kDoubleArrayData) {
                MDoubleArray        widthArray = widthArrayData.array();
                const size_t        numUsdWidths = dataWidths.size();
                const size_t        numMayaWidth = widthArray.length();
                const float* const  usdWidths = dataWidths.cdata();
                const double* const mayaWidth = &widthArray[0];
                if (!MayaUsdUtils::compareArray(usdWidths, mayaWidth, numUsdWidths, numMayaWidth)) {
                    result |= kWidths;
                }
            } else if (
                MFnNumericAttribute(widthObj).unitType()
                == MFnNumericData::kDouble) // data can come in as a single value
            {
                if (dataWidths.size() != 1
                    || std::abs(dataWidths[0] - widthPlug.asFloat()) < 1e-5f) {
                    result |= kWidths;
                }
            }
        }
    }
    return result;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
