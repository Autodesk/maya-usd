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
#include "translatorCurves.h"

#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/undo/OpUndoItems.h>

#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/nurbsCurves.h>

#include <maya/MDoubleArray.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MIntArray.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MTime.h>
#include <maya/MTimeArray.h>

PXR_NAMESPACE_OPEN_SCOPE

/* static */
bool convertToBezier(MFnNurbsCurve& nurbsCurveFn, MObject& mayaNodeTransformObj, MStatus& status)
{
    MFnNurbsCurve curveFn;
    MFnDagNode    dagFn;
    MObject curveObj = dagFn.create("bezierCurve", "bezierShape1", mayaNodeTransformObj, &status);
    if (status != MS::kSuccess) {
        return false;
    }
    curveFn.setObject(curveObj);
    // Create a nurbs to bezier converter
    MFnDependencyNode convFn;
    convFn.create("nurbsCurveToBezier");
    // Connect the converter between the nurbs and the bezier
    MPlug        convIn = convFn.findPlug("inputCurve", false);
    MPlug        convOut = convFn.findPlug("outputCurve", false);
    MPlug        nurbsOut = nurbsCurveFn.findPlug("local", false);
    MPlug        bezIn = dagFn.findPlug("create", false);
    MDGModifier& dgm = MayaUsd::MDGModifierUndoItem::create("Nurbs curve connections");
    dgm.connect(nurbsOut, convIn);
    dgm.connect(convOut, bezIn);
    dgm.doIt();
    // Pull on the bezier output to force computing the values :
    MPlug   bezOut = dagFn.findPlug("local", false);
    MObject val = bezOut.asMObject();
    // Remove the nurbs and converter:
    MDGModifier& dagm = MayaUsd::MDGModifierUndoItem::create("Nurbs curve deletion");
    dagm.deleteNode(convFn.object());
    dagm.deleteNode(nurbsCurveFn.object(), false);
    dagm.doIt();
    // replace deleted nurbs node with bezier node
    nurbsCurveFn.setObject(curveObj);
    return true;
}

/* static */
bool UsdMayaTranslatorCurves::Create(
    const UsdGeomCurves&         curves,
    MObject                      parentNode,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext*    context)
{
    if (!curves) {
        return false;
    }

    const UsdPrim& prim = curves.GetPrim();

    MStatus status;

    // Create node (transform)
    MObject mayaNodeTransformObj;
    if (!UsdMayaTranslatorUtil::CreateTransformNode(
            prim, parentNode, args, context, &status, &mayaNodeTransformObj)) {
        return false;
    }

    VtArray<GfVec3f> points;
    VtArray<int>     curveOrders;
    VtArray<int>     curveVertexCounts;
    VtArray<float>   curveWidths;
    VtArray<GfVec2d> curveRanges;
    VtArray<double>  curveKnots;
    VtArray<double>  _curveKnots; // subset of curveKnots

    // LIMITATION:  xxx REVISIT xxx
    //   Non-animated Attrs
    //   Assuming that a number of these USD attributes are assumed to not be animated
    //   Some we may want to expose as animatable later.
    //
    curves.GetCurveVertexCountsAttr().Get(&curveVertexCounts); // not animatable

    // Sanity Check
    if (curveVertexCounts.empty()) {
        TF_RUNTIME_ERROR(
            "vertexCount array is empty on NurbsCurves <%s>. Skipping...",
            prim.GetPath().GetText());
        return false; // No verts for the curve, so exit
    }

    // Gather points. If timeInterval is non-empty, pick the first available
    // sample in the timeInterval or default.
    UsdTimeCode         pointsTimeSample = UsdTimeCode::EarliestTime();
    std::vector<double> pointsTimeSamples;
    size_t              numTimeSamples = 0;
    if (!args.GetTimeInterval().IsEmpty()) {
        curves.GetPointsAttr().GetTimeSamplesInInterval(args.GetTimeInterval(), &pointsTimeSamples);
        numTimeSamples = pointsTimeSamples.size();
        if (numTimeSamples > 0) {
            pointsTimeSample = pointsTimeSamples[0];
        }
    }

    curves.GetPointsAttr().Get(&points, pointsTimeSample);

    if (points.empty()) {
        TF_RUNTIME_ERROR(
            "points array is empty on NurbsCurves <%s>. Skipping...", prim.GetPath().GetText());
        return false; // invalid nurbscurves, so exit
    }

    curves.GetWidthsAttr().Get(&curveWidths); // not animatable

    int     indexOffset = 0;
    int     coffset = 0;
    int     mayaDegree = 0;
    auto    curveType = MFn::kNurbsCurve;
    TfToken typeToken = UsdGeomTokens->linear;

    UsdGeomBasisCurves basisSchema;

    for (size_t curveIndex = 0; curveIndex < curveVertexCounts.size(); ++curveIndex) {

        if (UsdGeomNurbsCurves nurbsSchema = UsdGeomNurbsCurves(prim)) {
            if (curveKnots.empty()) {
                nurbsSchema.GetOrderAttr().Get(&curveOrders); // not animatable
                nurbsSchema.GetKnotsAttr().Get(&curveKnots);  // not animatable
                nurbsSchema.GetRangesAttr().Get(
                    &curveRanges); // not animatable or actually used....
            }

            if (curveIndex >= curveOrders.size()) {
                TF_RUNTIME_ERROR(
                    "Curve index goes beyond the curve orders array end (%d >= %d) in <%s>. "
                    "Skipping...",
                    (int)curveIndex,
                    (int)curveOrders.size(),
                    prim.GetPath().GetText());
                return false;
            }

            const int curveDegree = curveOrders[curveIndex] - 1;
            if (curveDegree < 1) {
                TF_RUNTIME_ERROR(
                    "Curve curve degree is invalid (%d) in <%s>. Skipping...",
                    curveDegree,
                    prim.GetPath().GetText());
                return false;
            }

            // Note: curveIndex is already limited by curveVertexCounts.size() from the loop.
            const int pointCount = curveVertexCounts[curveIndex];
            if (pointCount <= 0) {
                TF_RUNTIME_ERROR(
                    "Invalid point count (%d) in <%s>. Skipping...",
                    pointCount,
                    prim.GetPath().GetText());
                return false;
            }

            // Fill in missing knots.
            //
            // The USD NURBS curve schema (UsdGeomNurbsCurves) defines the number
            // of knots as: # spans + 2 * degree + 1.
            //
            // But the array of points is already equal to # spans + degree. So the
            // number of knots is # point + degree + 1
            //
            // The fist few knots (equal in number to the degree) must be equal
            // and the same applies to the last knots.
            const size_t requiredKnotCount = pointCount + curveDegree + 1;
            if (curveKnots.size() < coffset + requiredKnotCount) {
                // The knots array might already contain knots, so we're going to
                // keep the existing ones and only fill the necessary missing ones.
                // So calculate from which index we must start to fill.
                const size_t toBeFilledStartIndex = (curveKnots.size() > size_t(coffset))
                    ? size_t(curveKnots.size() - coffset)
                    : size_t(0);

                curveKnots.resize(coffset + requiredKnotCount);

                // Knot index start value, initialize to last knots if available,
                // otehrwise start at zero.
                size_t knotIdx = (toBeFilledStartIndex > 0)
                    ? size_t(curveKnots[coffset + toBeFilledStartIndex - 1])
                    : size_t(0);

                for (size_t i = toBeFilledStartIndex; i < curveKnots.size(); ++i) {
                    if (i < (size_t)curveDegree)
                        curveKnots[coffset + i] = knotIdx;
                    else if (i > curveKnots.size() - curveDegree)
                        curveKnots[coffset + i] = knotIdx;
                    else {
                        ++knotIdx;
                        curveKnots[coffset + i] = double(knotIdx);
                    }
                }
            }

            auto usdKnotStart = curveKnots.begin() + coffset;
            auto usdKnotEnd = usdKnotStart + pointCount + curveDegree + 1;

            // Remove front and back knots to match Maya representation. See
            // "Managing different knot representations in external applications"
            // section in MFnNurbsCurve documentation. There (and in USD docs)
            // we learns that there are two fewer knots in Maya.

            // make knot subset consisting of the current curve, trim ends
            _curveKnots = { usdKnotStart + 1, usdKnotEnd - 1 };

            // set offset to the beginning of next curve
            coffset += pointCount + curveDegree + 1;
            mayaDegree = curveDegree;

        } else {
            // Handle basis curves originally modeled in Maya as nurbs.
            curveType = MFn::kBezierCurve;

            curveOrders.resize(1);
            basisSchema = UsdGeomBasisCurves(prim);
            basisSchema.GetTypeAttr().Get(&typeToken);

            if (typeToken == UsdGeomTokens->linear) {
                curveOrders[0] = 2;
                _curveKnots.resize(curveVertexCounts[curveIndex]);
                for (size_t i = 0; i < _curveKnots.size(); ++i) {
                    _curveKnots[i] = i;
                }

            } else {
                curveOrders[0] = 4;

                _curveKnots.resize(curveVertexCounts[curveIndex] - 3 + 5);
                int knotIdx = 0;
                for (size_t i = 0; i < _curveKnots.size(); ++i) {
                    if (i < 3) {
                        _curveKnots[i] = 0.0;
                    } else {
                        if (i % 3 == 0) {
                            ++knotIdx;
                        } else if (i == _curveKnots.size() - 3) {
                            ++knotIdx;
                        }
                        _curveKnots[i] = double(knotIdx);
                    }
                }
            }
            mayaDegree = curveOrders[0] - 1;
        }

        // == Convert data
        size_t      mayaNumVertices = curveVertexCounts[curveIndex];
        MPointArray mayaPoints(mayaNumVertices);
        for (size_t i = 0; i < mayaNumVertices; i++) {
            size_t ipos = i + indexOffset;
            mayaPoints.set(i, points[ipos][0], points[ipos][1], points[ipos][2]);
        }

        double*      knots = _curveKnots.data();
        MDoubleArray mayaKnots(knots, _curveKnots.size());

        MFnNurbsCurve::Form mayaCurveForm = MFnNurbsCurve::kOpen; // HARDCODED
        bool                mayaCurveCreate2D = false;
        bool                mayaCurveCreateRational = true;

        // == Create NurbsCurve Shape Node
        MFnNurbsCurve curveFn;
        MObject       curveObj = curveFn.create(
            mayaPoints,
            mayaKnots,
            mayaDegree,
            mayaCurveForm,
            mayaCurveCreate2D,
            mayaCurveCreateRational,
            mayaNodeTransformObj,
            &status);
        if (status != MS::kSuccess) {
            return false;
        }
        if (curveType != MFn::kNurbsCurve && typeToken != UsdGeomTokens->linear) {
            // delete curve object and replace with bezier curve object
            convertToBezier(curveFn, mayaNodeTransformObj, status);
            if (status != MS::kSuccess) {
                return false;
            }
        }
        MString nodeName(prim.GetName().GetText());
        nodeName += "Shape";
        curveFn.setName(nodeName, false, &status);

        std::string nodePath(prim.GetPath().GetText());
        nodePath += "/";
        nodePath += nodeName.asChar();
        if (context) {
            context->RegisterNewMayaNode(nodePath, curveObj); // used for undo/redo
        }

        // == Animate points ==
        //   Use blendShapeDeformer so that all the points for a frame are contained in a single
        //   node Almost identical code as used with MayaMeshReader.cpp
        //
        if (numTimeSamples > 0) {
            MPointArray mayaPoints(mayaNumVertices);
            MObject     curveAnimObj;

            MFnBlendShapeDeformer blendFn;
            MObject               blendObj = blendFn.create(curveObj);
            if (context) {
                context->RegisterNewMayaNode(
                    blendFn.name().asChar(), blendObj); // used for undo/redo
            }

            for (unsigned int ti = 0; ti < numTimeSamples; ++ti) {
                curves.GetPointsAttr().Get(&points, pointsTimeSamples[ti]);

                for (unsigned int i = 0; i < mayaNumVertices; i++) {
                    unsigned int ipos = i + indexOffset;
                    mayaPoints.set(i, points[ipos][0], points[ipos][1], points[ipos][2]);
                }

                // == Create NurbsCurve Shape Node
                MFnNurbsCurve curveFn;
                if (curveAnimObj.isNull()) {
                    curveAnimObj = curveFn.create(
                        mayaPoints,
                        mayaKnots,
                        mayaDegree,
                        mayaCurveForm,
                        mayaCurveCreate2D,
                        mayaCurveCreateRational,
                        mayaNodeTransformObj,
                        &status);
                    if (status != MS::kSuccess) {
                        continue;
                    }
                    if (curveType == MFn::kBezierCurve) {
                        convertToBezier(curveFn, mayaNodeTransformObj, status);
                        if (status != MS::kSuccess) {
                            continue;
                        }
                    }

                } else {
                    // Reuse the already created curve by copying it and then setting the points
                    curveAnimObj = curveFn.copy(curveAnimObj, mayaNodeTransformObj, &status);
                    curveFn.setCVs(mayaPoints);
                }
                blendFn.addTarget(curveObj, ti, curveAnimObj, 1.0);
                curveFn.setIntermediateObject(true);
                if (context) {
                    context->RegisterNewMayaNode(
                        curveFn.fullPathName().asChar(), curveAnimObj); // used for undo/redo
                }
            }

            // Animate the weights so that curve0 has a weight of 1 at frame 0, etc.
            MFnAnimCurve animFn;

            // Construct the time array to be used for all the keys
            MTime::Unit timeUnit = MTime::uiUnit();
            double      timeSampleMultiplier
                = (context != nullptr) ? context->GetTimeSampleMultiplier() : 1.0;
            MTimeArray timeArray(numTimeSamples, MTime());
            for (unsigned int ti = 0; ti < numTimeSamples; ++ti) {
                timeArray.set(MTime(pointsTimeSamples[ti] * timeSampleMultiplier, timeUnit), ti);
            }

            // Key/Animate the weights
            MPlug plgAry = blendFn.findPlug("weight");
            if (!plgAry.isNull() && plgAry.isArray()) {
                for (unsigned int ti = 0; ti < numTimeSamples; ++ti) {
                    MPlug        plg = plgAry.elementByLogicalIndex(ti, &status);
                    MDoubleArray valueArray(numTimeSamples, 0.0);
                    valueArray[ti]
                        = 1.0; // Set the time value where this curve's weight should be 1.0
                    MObject animObj = animFn.create(plg, nullptr, &status);
                    animFn.addKeys(
                        &timeArray,
                        &valueArray,
                        MFnAnimCurve::kTangentLinear,
                        MFnAnimCurve::kTangentLinear);
                    if (context) {
                        context->RegisterNewMayaNode(
                            animFn.name().asChar(), animObj); // used for undo/redo
                    }
                }
            }
        }
        indexOffset += curveVertexCounts[curveIndex];
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
