//
// Copyright 2024 Autodesk
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
//
#include "translatorBlendShape.h"

#include "mayaUsd/undo/OpUndoItems.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usdSkel/animQuery.h>
#include <pxr/usd/usdSkel/animation.h>
#include <pxr/usd/usdSkel/binding.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/blendShape.h>
#include <pxr/usd/usdSkel/root.h>

#include <maya/MFnAnimCurve.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnGeometryFilter.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MPointArray.h>
#include <maya/MStatus.h>

#include <iterator>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

bool UsdMayaTranslatorBlendShape::Read(const UsdPrim& meshPrim, UsdMayaPrimReaderContext* context)
{
    if (!context) {
        return false;
    }

    MObject objToBlendShape = context->GetMayaNode(meshPrim.GetPath(), false);
    if (objToBlendShape.isNull()) {
        return true;
    }

    const UsdStageWeakPtr stage = meshPrim.GetStage();
    UsdSkelBindingAPI     skelBinding = UsdSkelBindingAPI::Get(stage, meshPrim.GetPath());

    SdfPathVector blendShapeTargets;
    skelBinding.GetBlendShapeTargetsRel().GetTargets(&blendShapeTargets);
    if (blendShapeTargets.empty()) {
        // no blendshapes, nothing to do
        return true;
    }

    MStatus status;

    MDagPath shapeDagPath;
    status = MDagPath::getAPathTo(objToBlendShape, shapeDagPath);
    CHECK_MSTATUS_AND_RETURN(status, false)
    status = shapeDagPath.extendToShape();
    CHECK_MSTATUS_AND_RETURN(status, false)
    MObject originalShape = shapeDagPath.node(&status);
    CHECK_MSTATUS_AND_RETURN(status, false)
    MObject parentTransform = shapeDagPath.transform(&status);
    CHECK_MSTATUS_AND_RETURN(status, false)

    MFnMesh     originalMeshFn(originalShape);
    MPointArray originalPoints;
    originalMeshFn.getPoints(originalPoints);
    const size_t originalNumVertices = originalPoints.length();

    MFnBlendShapeDeformer blendFn;
    const auto            blendShapeObj
        = blendFn.create(objToBlendShape, MFnBlendShapeDeformer::kLocalOrigin, &status);
    CHECK_MSTATUS_AND_RETURN(status, false)

    MObject      deformedMeshObject;
    VtVec3fArray deltaPoints, deltaNormals;
    VtIntArray   pointIndices;
    for (size_t targetIdx = 0; targetIdx < blendShapeTargets.size(); ++targetIdx) {
        const SdfPath     blendShapePath = blendShapeTargets[targetIdx];
        UsdSkelBlendShape blendShape(stage->GetPrimAtPath(blendShapePath));

        blendShape.GetOffsetsAttr().Get(&deltaPoints);
        if (deltaPoints.empty()) {
            TF_RUNTIME_ERROR(
                "BlendShape <%s> for mesh <%s> has no delta points.",
                blendShapePath.GetText(),
                meshPrim.GetPath().GetText());
            continue;
        }

        blendShape.GetNormalOffsetsAttr().Get(&deltaNormals);
        blendShape.GetPointIndicesAttr().Get(&pointIndices);

        // Indices aren't authored. Use mesh default pointIndices
        if (pointIndices.empty()) {
            pointIndices.resize(originalNumVertices);
            for (size_t i = 0; i < originalNumVertices; ++i) {
                pointIndices[i] = i;
            }
        }

        // blendshape points initially identical to the original shape
        MPointArray deformedPoints(originalPoints);

        // Blendshapes don't necessarily offset all mesh points. Thus, first get the index
        // of the point that it was changed and apply the offset to that point.
        for (int pidx = 0; pidx < pointIndices.size(); ++pidx) {
            int     index = pointIndices[pidx];
            MPoint  original = originalPoints[index];
            GfVec3f offset = deltaPoints[pidx];

            deformedPoints.set(
                MPoint(original.x + offset[0], original.y + offset[1], original.z + offset[2]),
                index);
        }

        // create the new blendShape mesh with the applied offset
        MFnMesh meshFn;
        deformedMeshObject = meshFn.copy(originalShape, parentTransform, &status);
        meshFn.setPoints(deformedPoints);

        // rename the blendShape to match what is coming from the usd blendshape
        MFnDependencyNode dpNodeFn;
        dpNodeFn.setObject(deformedMeshObject);
        dpNodeFn.setName(MString(blendShape.GetPath().GetElementString().c_str()));

        // Include the main blendShape as default target weight of
        blendFn.addTarget(originalShape, targetIdx, deformedMeshObject, 1.0f);
        meshFn.setIntermediateObject(true);

        const std::vector<UsdSkelInbetweenShape> inBetweens = blendShape.GetInbetweens();
        // no in-betweens, can continue to the next blendShape
        if (inBetweens.empty()) {
            continue;
        }

        // After adding the main blendShape - now add all the in-betweens
        MObject ibObject;
        for (size_t ib = 0; ib < inBetweens.size(); ++ib) {
            const auto&       currentInBetween = inBetweens[ib];
            const std::string inBetweenName = currentInBetween.GetAttr().GetName().GetString();
            float             ibWeight = 0.f;
            currentInBetween.GetWeight(&ibWeight);

            VtVec3fArray inBetweenDeltaPoints, inBetweenDeltaNormals;
            currentInBetween.GetOffsets(&inBetweenDeltaPoints);

            MFnMesh ibMeshFn;
            ibObject = ibMeshFn.copy(originalShape, parentTransform, &status);

            MPointArray ibPoints(originalPoints);
            for (int pidx = 0; pidx < pointIndices.size(); ++pidx) {
                int     index = pointIndices[pidx];
                MPoint  original = originalPoints[index];
                GfVec3f offset = inBetweenDeltaPoints[pidx];

                ibPoints.set(
                    MPoint(original.x + offset[0], original.y + offset[1], original.z + offset[2]),
                    index);
            }
            ibMeshFn.setPoints(ibPoints);

            MFnDependencyNode ibDepNodeFn;
            ibDepNodeFn.setObject(ibObject);
            ibDepNodeFn.setName(MString(inBetweenName.c_str()));

            blendFn.addTarget(originalShape, targetIdx, ibObject, ibWeight);
            ibMeshFn.setIntermediateObject(true);
        }
    }
    return true;
}

bool UsdMayaTranslatorBlendShape::ReadAnimations(
    const UsdSkelSkinningQuery&     skinningQuery,
    const UsdSkelAnimQuery&         animQuery,
    const UsdMayaPrimReaderContext& context)
{
    MStatus status;

    const UsdPrim& skinnedPrim = skinningQuery.GetPrim();
    VtTokenArray   skinnedBlendShapes;
    skinningQuery.GetBlendShapeOrder(&skinnedBlendShapes);
    // mesh doesn't have any blendshapes
    if (skinnedBlendShapes.empty()) {
        return true;
    }

    const VtTokenArray animBlendShapes = animQuery.GetBlendShapeOrder();
    if (animBlendShapes.empty()) {
        // there are no animated blendShapes. Nothing to do
        return true;
    }

    std::vector<unsigned int> correctBlendShapeIndices;
    correctBlendShapeIndices.reserve(skinnedBlendShapes.size());
    // The animation prim can hold blendShapes from several mesh prims.
    // So it's necessary to find the the indices only for those on the current mesh prim.
    for (const TfToken& token : skinnedBlendShapes) {
        const auto iter = std::find(animBlendShapes.begin(), animBlendShapes.end(), token);
        const auto idx = static_cast<int>(std::distance(animBlendShapes.begin(), iter));
        if (idx >= 0) {
            correctBlendShapeIndices.emplace_back(idx);
        }
    }

    const UsdStageWeakPtr  stage = skinnedPrim.GetStage();
    const UsdSkelAnimation skelAnimPrim(animQuery.GetPrim());
    const UsdAttribute     animWeightsAttribute = skelAnimPrim.GetBlendShapeWeightsAttr();
    const GfInterval       timeCodeInterval(stage->GetStartTimeCode(), stage->GetEndTimeCode());
    std::vector<double>    animatedTimeSamples;
    animWeightsAttribute.GetTimeSamplesInInterval(timeCodeInterval, &animatedTimeSamples);

    if (animatedTimeSamples.empty()) {
        // no animation in time range. Nothing to do.
        return true;
    }

    MTimeArray  timeArray;
    MTime::Unit timeUnit = MTime::uiUnit();
    double      timeSampleMultiplier = context.GetTimeSampleMultiplier();
    timeArray.setLength(animatedTimeSamples.size());
    for (unsigned int ti = 0u; ti < animatedTimeSamples.size(); ++ti) {
        timeArray.set(MTime(animatedTimeSamples[ti] * timeSampleMultiplier, timeUnit), ti);
    }

    MObject  transformNode = context.GetMayaNode(skinnedPrim.GetPath(), false);
    MDagPath shapeDagPath;
    status = MDagPath::getAPathTo(transformNode, shapeDagPath);
    shapeDagPath.extendToShape();
    MObject shapeObject = shapeDagPath.node(&status);

    if (shapeObject.isNull() || !shapeObject.hasFn(MFn::kMesh)) {
        return false;
    }

    MItDependencyGraph depGraph(
        shapeObject,
        MFn::kBlendShape,
        MItDependencyGraph::kUpstream,
        MItDependencyGraph::kBreadthFirst,
        MItDependencyGraph::kPlugLevel,
        &status);

    MFnBlendShapeDeformer fnBlendShape;
    while (!depGraph.isDone()) {
        MObject dagNode = depGraph.currentItem();
        if (dagNode.hasFn(MFn::kBlendShape)) {
            fnBlendShape.setObject(dagNode);

            MPlug plgBsArray = fnBlendShape.findPlug("weight", false, &status);
            if (!plgBsArray.isNull() && plgBsArray.isArray()) {
                MFnAnimCurve animFn;
                for (unsigned int idx = 0; idx < correctBlendShapeIndices.size(); ++idx) {
                    const auto   blendShapeIndex = correctBlendShapeIndices[idx];
                    MPlug        plg = plgBsArray.elementByLogicalIndex(idx, &status);
                    MObject      animObj = animFn.create(plg, nullptr, &status);
                    MDoubleArray valueArray(animatedTimeSamples.size(), 0.0);
                    for (size_t timeSample = 0; timeSample < animatedTimeSamples.size();
                         ++timeSample) {
                        VtFloatArray weights;
                        animWeightsAttribute.Get(&weights, animatedTimeSamples[timeSample]);
                        valueArray[timeSample] = weights[blendShapeIndex];
                    }
                    animFn.addKeys(&timeArray, &valueArray);
                }
            }
        }
        depGraph.next();
    }

    return true;
}

} // namespace MAYAUSD_NS_DEF
