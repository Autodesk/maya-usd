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

#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdSkel/animQuery.h>
#include <pxr/usd/usdSkel/animation.h>
#include <pxr/usd/usdSkel/binding.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/blendShape.h>
#include <pxr/usd/usdSkel/root.h>

#include <maya/MFloatVectorArray.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnGeometryFilter.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MPointArray.h>
#include <maya/MStatus.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

struct _BlendShapeAttributesNames
{
    const char* it { "inputTarget" };
    const char* itg { "inputTargetGroup" };
    const char* iti { "inputTargetItem" };
    const char* ipt { "inputPointsTarget" };
    const char* ict { "inputComponentsTarget" };
    const char* sn { "supportNegativeWeights" };
    const char* w { "weight" };
    const char* ibig { "inbetweenInfoGroup" };
    const char* ibi { "inbetweenInfo" };
    const char* ibtn { "inbetweenTargetName" };
};

TfStaticData<_BlendShapeAttributesNames> _BlendShapeAttributes;

void _AddBlendShape(
    const MFnBlendShapeDeformer& fnBlendShape,
    const std::string&           blendShapeName,
    unsigned int                 targetIdx,
    float                        weight,
    const MPointArray&           deltas,
    MIntArray&                   indices,
    bool                         isInBetween = false)
{
    const auto blendShapeObj = fnBlendShape.object();

    auto inputTargetAttr = fnBlendShape.attribute(_BlendShapeAttributes->it);
    auto inputTargetGroupAttr = fnBlendShape.attribute(_BlendShapeAttributes->itg);
    auto inputTargetItemAttr = fnBlendShape.attribute(_BlendShapeAttributes->iti);
    auto inputPointsTargetAttr = fnBlendShape.attribute(_BlendShapeAttributes->ipt);
    auto inputComponentTargetAttr = fnBlendShape.attribute(_BlendShapeAttributes->ict);
    auto inputWeightAttr = fnBlendShape.attribute(_BlendShapeAttributes->w);

    if (weight < 0.f) {
        auto supportNegativeWeightAttr = fnBlendShape.attribute(_BlendShapeAttributes->sn);

        MPlug plgSupportNegativeWeight(blendShapeObj, supportNegativeWeightAttr);
        plgSupportNegativeWeight.setBool(true);
    }

    // The weight index is an integer that maps weight values to Maya's internal representation
    // Maya supports negative weights, so we need to handle the full range
    // Positive weights: 5000-6000 (weight 0.001 to 1.0)
    // Negative weights: 0-4999 (weight -5.0 to -0.001)
    // Zero weight: 5000
    // In theory, there's no upper bound for the max weight value.
    static const auto convertWeightToIndex = [](float weight) -> int {
        int result = 5000 + static_cast<int>(weight * 1000.f);
        return result >= 0 ? result : 0;
    };

    // Maya's blendShape api requires a copy of the original shape or a copy of the target mesh
    // for it to work.
    // To avoid that, use plugs to manually create the blendShape targets using only the deltas
    // from USD.

    // Create the plug for the desired attribute: .it[-1].itg[-1].iti[-1].ipt
    MPlug plgInPoints(blendShapeObj, inputPointsTargetAttr);
    // Set the first element position in the plug: .it[0].itg[-1].iti[-1].ipt
    plgInPoints.selectAncestorLogicalIndex(0, inputTargetAttr);
    // Set the second plug element: .it[0].itg[targetIdx].iti[-1].ipt
    plgInPoints.selectAncestorLogicalIndex(targetIdx, inputTargetGroupAttr);
    // Third element: .it[0].itg[targetIdx].iti[convertWeightToIndex(weight)].ipt
    plgInPoints.selectAncestorLogicalIndex(convertWeightToIndex(weight), inputTargetItemAttr);

    {
        MFnPointArrayData data;
        MObject           dataObj = data.create();
        data.set(deltas);
        plgInPoints.setValue(dataObj);
    }

    // Now, similarly, set the inputComponentsTarget attribute (which are the indices being changed)
    MPlug plgInCompTarget(blendShapeObj, inputComponentTargetAttr);
    plgInCompTarget.selectAncestorLogicalIndex(0, inputTargetAttr);
    plgInCompTarget.selectAncestorLogicalIndex(targetIdx, inputTargetGroupAttr);
    plgInCompTarget.selectAncestorLogicalIndex(convertWeightToIndex(weight), inputTargetItemAttr);
    {
        MFnSingleIndexedComponent indexList;
        auto                      indexListObj = indexList.create(MFn::kMeshVertComponent);
        indexList.addElements(indices);
        MFnComponentListData fnComp;
        MObject              compObj = fnComp.create();
        fnComp.add(indexListObj);
        plgInCompTarget.setValue(compObj);
    }

    if (isInBetween) {
        auto  inBetweenTargetNameAttr = fnBlendShape.attribute(_BlendShapeAttributes->ibtn);
        auto  inBetweenInfoGroupAttr = fnBlendShape.attribute(_BlendShapeAttributes->ibig);
        auto  inBetweenInfoAttr = fnBlendShape.attribute(_BlendShapeAttributes->ibi);
        MPlug plgInBetweenTargetName(blendShapeObj, inBetweenTargetNameAttr);
        plgInBetweenTargetName.selectAncestorLogicalIndex(targetIdx, inBetweenInfoGroupAttr);
        plgInBetweenTargetName.selectAncestorLogicalIndex(
            convertWeightToIndex(weight), inBetweenInfoAttr);

        // USD inbetweens are named: "inbetween:<name>"
        // Remove the inbetween prefix
        auto inBetweenName = blendShapeName.substr(blendShapeName.find_first_of(":") + 1);

        plgInBetweenTargetName.setString(MString(inBetweenName.c_str()));
    } else {
        MPlug plgWeight(blendShapeObj, inputWeightAttr);
        plgWeight.selectAncestorLogicalIndex(targetIdx, inputWeightAttr);

        MString cmd;
        cmd.format(
            "import maya.cmds as cmds; cmds.aliasAttr(\"^1s\",\"^2s\");",
            blendShapeName.c_str(),
            plgWeight.name().asChar());

        MGlobal::executePythonCommand(cmd);
    }
}

bool UsdMayaTranslatorBlendShape::Read(const UsdPrim& meshPrim, UsdMayaPrimReaderContext* context)
{
    if (!context) {
        return false;
    }

    // It's required that the meshPrim with the blendShapes already had its equivalent Maya node
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

    MFnBlendShapeDeformer blendFn;
    const auto            blendShapeObj
        = blendFn.create(objToBlendShape, MFnBlendShapeDeformer::kLocalOrigin, &status);
    CHECK_MSTATUS_AND_RETURN(status, false)
    MFnDependencyNode blendShapeDepNodeFn;
    blendShapeDepNodeFn.setObject(blendShapeObj);
    blendShapeDepNodeFn.setName(MString(
        TfStringPrintf("%s_Deformer", meshPrim.GetPath().GetElementString().c_str()).c_str()));

    for (size_t targetIdx = 0; targetIdx < blendShapeTargets.size(); ++targetIdx) {
        const SdfPath&          blendShapePath = blendShapeTargets[targetIdx];
        const UsdSkelBlendShape blendShape(stage->GetPrimAtPath(blendShapePath));
        const std::string       blendShapeName = blendShape.GetPath().GetElementString();

        VtVec3fArray deltaPoints;
        blendShape.GetOffsetsAttr().Get(&deltaPoints);
        if (deltaPoints.empty()) {
            TF_RUNTIME_ERROR(
                "BlendShape <%s> for mesh <%s> has no delta points.",
                blendShapePath.GetText(),
                meshPrim.GetPath().GetText());
            continue;
        }

        VtIntArray pointIndices;
        blendShape.GetPointIndicesAttr().Get(&pointIndices);

        // If indices aren't authored, use mesh default pointIndices
        if (pointIndices.empty()) {
            pointIndices.reserve(deltaPoints.size());
            for (size_t i = 0; i < deltaPoints.size(); ++i) {
                pointIndices.emplace_back(i);
            }
        }

        if (deltaPoints.size() != pointIndices.size()) {
            TF_RUNTIME_ERROR(
                "BlendShape <%s> for mesh <%s> has mismatched number of delta points and indices.",
                blendShapePath.GetText(),
                meshPrim.GetPath().GetText());
            continue;
        }

        // Convert Usd Arrays to Maya Arrays
        MIntArray indicesIntArray(
            pointIndices.cdata(), static_cast<unsigned int>(pointIndices.size()));
        MPointArray deltasPointsArray(static_cast<unsigned int>(pointIndices.size()));
        for (unsigned int pidx = 0; pidx < pointIndices.size(); ++pidx) {
            deltasPointsArray.set(
                pidx, deltaPoints[pidx][0], deltaPoints[pidx][1], deltaPoints[pidx][2]);
        }

        // MFnSingleIndexedComponent sorts the indices automatically, so we need to sort deltas
        // accordingly to maintain correspondence between indices and deltas.
        std::vector<std::pair<int, MPoint>> indexDeltaPairs;
        indexDeltaPairs.reserve(indicesIntArray.length());
        for (unsigned int i = 0; i < indicesIntArray.length(); ++i) {
            indexDeltaPairs.emplace_back(indicesIntArray[i], deltasPointsArray[i]);
        }
        std::sort(indexDeltaPairs.begin(), indexDeltaPairs.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });

        // Extract sorted indices and deltas
        MIntArray   sortedIndices;
        MPointArray sortedDeltas;
        sortedIndices.setLength(static_cast<unsigned int>(indexDeltaPairs.size()));
        sortedDeltas.setLength(static_cast<unsigned int>(indexDeltaPairs.size()));
        for (unsigned int i = 0; i < indexDeltaPairs.size(); ++i) {
            sortedIndices[i] = indexDeltaPairs[i].first;
            const MPoint& pt = indexDeltaPairs[i].second;
            sortedDeltas.set(i, pt.x, pt.y, pt.z);
        }

        _AddBlendShape(
            blendFn,
            blendShapeName,
            static_cast<unsigned int>(targetIdx),
            1.f,
            sortedDeltas,
            sortedIndices);

        const std::vector<UsdSkelInbetweenShape> inBetweens = blendShape.GetInbetweens();
        if (!inBetweens.empty()) {
            // Create a mapping from index value to original position
            std::map<int, unsigned int> indexToOriginalPos;
            for (unsigned int i = 0; i < indicesIntArray.length(); ++i) {
                indexToOriginalPos[indicesIntArray[i]] = i;
            }

            // Create a mapping from sorted position to original position
            std::vector<unsigned int> sortedToOriginalPos(indexDeltaPairs.size());
            for (unsigned int i = 0; i < indexDeltaPairs.size(); ++i) {
                sortedToOriginalPos[i] = indexToOriginalPos[sortedIndices[i]];
            }

            for (const auto& inBetween : inBetweens) {
                const std::string inBetweenName = inBetween.GetAttr().GetName().GetString();
                float             ibWeight = 0.f;
                inBetween.GetWeight(&ibWeight);

                VtVec3fArray inBetweenDeltaPoints;
                inBetween.GetOffsets(&inBetweenDeltaPoints);
                if (inBetweenDeltaPoints.empty()) {
                    TF_RUNTIME_ERROR(
                        "InBetween BlendShape <%s> for mesh <%s> has no delta points.",
                        inBetweenName.c_str(),
                        meshPrim.GetPath().GetText());
                    continue;
                }

                if (inBetweenDeltaPoints.size() != pointIndices.size()) {
                    TF_RUNTIME_ERROR(
                        "InBetween BlendShape <%s> for mesh <%s> has a different number of delta "
                        "points and indices.",
                        inBetweenName.c_str(),
                        meshPrim.GetPath().GetText());
                    continue;
                }

                // Sort the inbetween deltas using the same index order
                MPointArray sortedInBetweenDeltas(
                    static_cast<unsigned int>(indexDeltaPairs.size()));
                for (unsigned int i = 0; i < indexDeltaPairs.size(); ++i) {
                    const GfVec3f& offset = inBetweenDeltaPoints[sortedToOriginalPos[i]];
                    sortedInBetweenDeltas.set(i, offset[0], offset[1], offset[2]);
                }

                _AddBlendShape(
                    blendFn,
                    inBetweenName,
                    static_cast<unsigned int>(targetIdx),
                    ibWeight,
                    sortedInBetweenDeltas,
                    sortedIndices,
                    true);
            }
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
