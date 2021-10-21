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

#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/nurbsCurves.h>

#include <maya/MDoubleArray.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MGlobal.h>
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
    status = curveFn.setObject(curveObj);
    // Create a nurbs to bezier converter
    MFnDependencyNode convFn;
    convFn.create("nurbsCurveToBezier");
    // Connect the converter between the nurbs and the bezier
    MPlug       convIn = convFn.findPlug("inputCurve", false);
    MPlug       convOut = convFn.findPlug("outputCurve", false);
    MPlug       nurbsOut = nurbsCurveFn.findPlug("local", false);
    MPlug       bezIn = dagFn.findPlug("create", false);
    MDGModifier dgm;
    dgm.connect(nurbsOut, convIn);
    dgm.connect(convOut, bezIn);
    dgm.doIt();
    // Pull on the bezier output to force computing the values :
    MPlug   bezOut = dagFn.findPlug("local", false);
    MObject val = bezOut.asMObject();
    // Remove the nurbs and converter:
    MDGModifier dagm;
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
    VtArray<int>     curveOrder;
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

    int  indexOffset = 0;
    int  coffset = 0;
    int  mayaDegree = 0;
    auto curveType = MFn::kNurbsCurve;

    for (size_t curveIndex = 0; curveIndex < curveVertexCounts.size(); ++curveIndex) {

        if (UsdGeomNurbsCurves nurbsSchema = UsdGeomNurbsCurves(prim)) {
            if (curveKnots.empty()) {
                nurbsSchema.GetOrderAttr().Get(&curveOrder); // not animatable
                nurbsSchema.GetKnotsAttr().Get(&curveKnots); // not animatable
                nurbsSchema.GetRangesAttr().Get(
                    &curveRanges); // not animatable or actually used....
            }

            // Remove front and back knots to match Maya representation. See
            // "Managing different knot representations in external applications"
            // section in MFnNurbsCurve documentation.
            // make knot subset consisting of the current curve, trim ends
            _curveKnots = { curveKnots.begin() + coffset + 1,
                            curveKnots.begin() + coffset + curveVertexCounts[curveIndex] + 3 };
            // set offset to the beginning of next curve
            coffset += curveVertexCounts[curveIndex] + 4;
            mayaDegree = curveOrder[curveIndex] - 1;

        } else {
            // Handle basis curves originally modeled in Maya as nurbs.
            curveType = MFn::kBezierCurve;

            curveOrder.resize(1);
            UsdGeomBasisCurves basisSchema = UsdGeomBasisCurves(prim);
            TfToken            typeToken;
            basisSchema.GetTypeAttr().Get(&typeToken);
            TfToken basisToken;

            basisSchema.GetBasisAttr().Get(&basisToken);

            if (typeToken == UsdGeomTokens->linear) {
                curveOrder[0] = 2;
                _curveKnots.resize(curveVertexCounts[curveIndex]);
                for (size_t i = 0; i < _curveKnots.size(); ++i) {
                    _curveKnots[i] = i;
                }

            } else { // basisToken == UsdGeomTokens->bezier || bspline)
                curveOrder[0] = 4;

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
            mayaDegree = curveOrder[0] - 1;
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
        if (curveType != MFn::kNurbsCurve) {
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
                    animFn.addKeys(&timeArray, &valueArray);
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
