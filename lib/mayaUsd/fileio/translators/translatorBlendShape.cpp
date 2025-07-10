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

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usdSkel/animQuery.h>
#include <pxr/usd/usdSkel/animation.h>
#include <pxr/usd/usdSkel/binding.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/blendShape.h>
#include <pxr/usd/usdSkel/root.h>

#include <maya/MFloatVectorArray.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnGeometryFilter.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MPointArray.h>
#include <maya/MStatus.h>

#include <iterator>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
void _AddBlendShape(
    MFnBlendShapeDeformer& blendShapeFn,
    const MObject&         originalShape,
    const MObject&         parentTransform,
    const MPointArray&     originalPoints,
    const GfVec3f*         originalNormals,
    const VtIntArray&      pointIndices,
    const std::string      name,
    const VtVec3fArray     offsetArray,
    const VtVec3fArray&    normalsOffsetArray,
    int                    blendShapeTargetIndex,
    float                  weight)
{
    if (offsetArray.size() != pointIndices.size()) {
        TF_RUNTIME_ERROR(
            "BlendShape <%s> doesn't match the number of offset points.", name.c_str());
        return;
    }

    MFnMesh blendShapeMeshFn;
    MStatus status;
    auto    newShape = blendShapeMeshFn.copy(originalShape, parentTransform, &status);

    MPointArray deltaPoints(originalPoints);
    for (size_t pidx = 0; pidx < pointIndices.size(); ++pidx) {
        // USD BlendShapes doesn't require to all points to have a delta.
        // Thus, given the indicesArray, find the actual index (index) in the original shape that
        // will be affected that the offset on that particular position.
        int     index = pointIndices[pidx];
        MPoint  original = originalPoints[index];
        GfVec3f offset = offsetArray[pidx];

        deltaPoints.set(
            MPoint(original.x + offset[0], original.y + offset[1], original.z + offset[2]), index);
    }
    blendShapeMeshFn.setPoints(deltaPoints);

    // Applying blendShape normals
    if (!normalsOffsetArray.empty()) {
        MFloatVectorArray deltaNormals(normalsOffsetArray.size());
        for (size_t i = 0; i < normalsOffsetArray.size(); ++i) {
            MFloatVector deltaNormal(
                originalNormals[i][0], originalNormals[i][1], originalNormals[i][2]);

            deltaNormal.x += normalsOffsetArray[i][0];
            deltaNormal.y += normalsOffsetArray[i][1];
            deltaNormal.z += normalsOffsetArray[i][2];
            deltaNormals.set(deltaNormal, i);
        }
        blendShapeMeshFn.setNormals(deltaNormals);
    }

    MFnDependencyNode ibDepNodeFn;
    ibDepNodeFn.setObject(newShape);
    ibDepNodeFn.setName(MString(name.c_str()));

    blendShapeFn.addTarget(originalShape, blendShapeTargetIndex, newShape, weight);
    blendShapeMeshFn.setIntermediateObject(true);
}

bool UsdMayaTranslatorBlendShape::Read(const UsdPrim& meshPrim, UsdMayaPrimReaderContext* context)
{
    if (!context) {
        return false;
    }

    // It's required that the meshPrim with the blendShapes already had it's equivalent Maya node
    // created and properly registered by the writer
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

    // There's no easy way to access the mesh's original normals.
    // This is how it's done in other places when those are needed.
    const float* normals = originalMeshFn.getRawNormals(&status);
    CHECK_MSTATUS_AND_RETURN(status, false)
    const GfVec3f* originalNormals = reinterpret_cast<const GfVec3f*>(normals);

    MFnBlendShapeDeformer blendFn;
    const auto            blendShapeObj
        = blendFn.create(objToBlendShape, MFnBlendShapeDeformer::kLocalOrigin, &status);
    CHECK_MSTATUS_AND_RETURN(status, false)
    MFnDependencyNode blendShapeDepNodeFn;
    blendShapeDepNodeFn.setObject(blendShapeObj);
    blendShapeDepNodeFn.setName(MString(
        TfStringPrintf("%s_Deformer", meshPrim.GetPath().GetElementString().c_str()).c_str()));

    MObject      deformedMeshObject;
    VtVec3fArray deltaPoints, deltaNormals;
    VtIntArray   pointIndices;
    for (size_t targetIdx = 0; targetIdx < blendShapeTargets.size(); ++targetIdx) {
        const SdfPath     blendShapePath = blendShapeTargets[targetIdx];
        UsdSkelBlendShape blendShape(stage->GetPrimAtPath(blendShapePath));
        std::string       blendShapeName = blendShape.GetPath().GetElementString();

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

        _AddBlendShape(
            blendFn,
            originalShape,
            parentTransform,
            originalPoints,
            originalNormals,
            pointIndices,
            blendShapeName,
            deltaPoints,
            deltaNormals,
            targetIdx,
            1.0f); // The original blendShape always has full weight of 1.0f

        // After adding the main blendShape - now add all the in-betweens

        const std::vector<UsdSkelInbetweenShape> inBetweens = blendShape.GetInbetweens();
        // no in-betweens, can continue to the next blendShape
        if (inBetweens.empty()) {
            continue;
        }

        for (size_t ib = 0; ib < inBetweens.size(); ++ib) {
            const auto&       currentInBetween = inBetweens[ib];
            const std::string inBetweenName = currentInBetween.GetAttr().GetName().GetString();
            float             ibWeight = 0.f;
            currentInBetween.GetWeight(&ibWeight);

            VtVec3fArray inBetweenDeltaPoints, inBetweenNormals;
            currentInBetween.GetOffsets(&inBetweenDeltaPoints);
            if (inBetweenDeltaPoints.empty()) {
                TF_RUNTIME_ERROR(
                    "InBetween BlendShape <%s> for mesh <%s> has no delta points.",
                    inBetweenName.c_str(),
                    meshPrim.GetPath().GetText());
                continue;
            }

            currentInBetween.GetNormalOffsets(&inBetweenNormals);

            // InBetween are new blendShapes added to the same target index
            _AddBlendShape(
                blendFn,
                originalShape,
                parentTransform,
                originalPoints,
                originalNormals,
                pointIndices,
                inBetweenName,
                inBetweenDeltaPoints,
                inBetweenNormals,
                targetIdx,
                ibWeight);
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
        // Couldn't find the blendShape token in the animation prim. Probably not animated
        if (iter == animBlendShapes.end()) {
            continue;
        }
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
                    animFn.addKeys(
                        &timeArray,
                        &valueArray,
                        MFnAnimCurve::kTangentLinear,
                        MFnAnimCurve::kTangentLinear);
                }
            }
        }
        depGraph.next();
    }

    return true;
}

} // namespace MAYAUSD_NS_DEF
