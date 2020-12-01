//
// Copyright 2020 Apple
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

#include "jointWriter.h"
#include "meshWriter.h"

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/meshWriteUtils.h>
#include <mayaUsd/fileio/utils/writeUtil.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usdGeom/pointBased.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/blendShape.h>

#include <maya/MApiNamespace.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnGeometryFilter.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MObject.h>
#include <maya/MPointArray.h>
#include <maya/MStatus.h>

#include <algorithm>
#include <complex>
#include <string>
#include <unordered_map>
#include <vector>

#define MAYA_BLENDSHAPE_EVAL_HOTFIX 1

#define MAYA_ATTR_NAME_WEIGHT "weight"

struct MayaBlendShapeWeightDatum
{
    MObjectArray targetMeshes; // The target shape(s) to hit. (i.e. multiple shapes would be using
                               // Maya's "in-betweens" feature.)
    MIntArray
              inputTargetGroupIndices; // The input group indices at which each target is connected under.
    MIntArray inputTargetIndices; // The input indices at which each target is connected under.
    MIntArray targetItemIndices;  // The Maya blendshape weight indices for the resulting deformed
                                  // mesh shape.
    unsigned int weightIndex; // The logical index of the weight attribute on the blendshape node.
};

struct MayaBlendShapeDatum
{
    MObject deformedMesh;       // The resulting deformed mesh shape (i.e. deformation + weight +
                                // envelope) Maya calls this a base object.
    MObject baseMesh;           // The original base mesh shape (i.e. no deformation)
    MObject blendShapeDeformer; // The blendshape deformer node itself.
    std::vector<MayaBlendShapeWeightDatum> weightDatas;
    unsigned int                           numWeights;
    unsigned int outputGeomIndex; // The logical index at which the deformed mesh is ultimately
                                  // connected downstream from the blendshape deformer.
};

float mayaGetBlendShapeTargetWeightFromIndex(const unsigned int index)
{
    float targetWeight = (static_cast<float>(index) - 5000.0f) * 0.001f;
    return targetWeight;
}

/**
 * Gets information about available blend shapes for a given deformed mesh (i.e. final result)
 *
 * @param mesh        The deformed mesh to find the blendshape info(s) for.
 * @param outInfos    Storage for the result.
 *
 * @return            A status code.
 */
MStatus mayaGetBlendShapeInfosForMesh(
    const MObject&                    deformedMesh,
    std::vector<MayaBlendShapeDatum>& outInfos)
{
    // TODO: (yliangsiew) Eh, find a way to avoid incremental allocations like these and just
    // allocate upfront. But hard to do with the iterative search functions of the DG...
    MStatus stat;

    // NOTE: (yliangsiew) If there's a skinCluster, find that first since that
    // will be the intermediate to the blendShape node. If not, just search for any
    // blendshape deformers upstream of the mesh.
    MObject      searchObject;
    MObjectArray skinClusters;
    stat = mayaGetSkinClustersUpstreamOfMesh(deformedMesh, skinClusters);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    unsigned int numSkinClusters = skinClusters.length();
    switch (numSkinClusters) {
    case 0: searchObject = MObject(deformedMesh); break;
    case 1: searchObject = MObject(skinClusters[0]); break;
    default:
        MGlobal::displayWarning("More than one skinCluster was found; only the first one will be "
                                "considered during the search!");
        searchObject = MObject(skinClusters[0]);
        break;
    }

    // TODO: (yliangsiew) Problem: if there are _intermediate deformers between blendshapes,
    // then oh-no: what do we do? Like blendshape1 -> wrap -> blendshape2. We can't possibliy
    // export that into current USD file format and expect predictable behaviour.
    // Houston, we have a problem...
    MFnGeometryFilter     fnGeoFilter;
    MFnBlendShapeDeformer fnBlendShape;
    MItDependencyGraph    itDg(
        searchObject,
        MFn::kBlendShape,
        MItDependencyGraph::kUpstream,
        MItDependencyGraph::kDepthFirst,
        MItDependencyGraph::kPlugLevel,
        &stat);
    MFnDependencyNode fnNode;
    for (; !itDg.isDone(); itDg.next()) {
        MObject curBlendShape = itDg.currentItem();
        assert(curBlendShape.hasFn(MFn::kBlendShape));

        MPlug outputGeomPlug = itDg.thisPlug();
        assert(outputGeomPlug.isElement() == true);
        unsigned int outputGeomPlugIdx = outputGeomPlug.logicalIndex();

        // NOTE: (yliangsiew) Because we can have multiple output
        // deformed meshes from a single blendshape deformer, we have
        // to walk back up the graph using the connected index to find
        // out what the _actual_ base mesh was.
        MayaBlendShapeDatum info = {};
        info.blendShapeDeformer = curBlendShape;
        info.outputGeomIndex = outputGeomPlugIdx;
        fnGeoFilter.setObject(curBlendShape);
        MObject inputGeo = fnGeoFilter.inputShapeAtIndex(outputGeomPlugIdx, &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        info.baseMesh = inputGeo;
        info.deformedMesh = deformedMesh;

        fnBlendShape.setObject(curBlendShape);
        info.numWeights = fnBlendShape.numWeights();
        MIntArray weightIndices;
        stat = fnBlendShape.weightIndexList(weightIndices);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        for (unsigned int i = 0; i < weightIndices.length(); ++i) {
            MayaBlendShapeWeightDatum weightInfo = {};
            weightInfo.weightIndex = weightIndices[i];
            stat = fnBlendShape.getTargets(
                deformedMesh, weightInfo.weightIndex, weightInfo.targetMeshes);
            CHECK_MSTATUS_AND_RETURN_IT(stat);

            for (unsigned int j = 0; j < weightInfo.targetMeshes.length(); ++j) {
                MObject targetMesh = weightInfo.targetMeshes[j];
                stat = fnNode.setObject(targetMesh);
                CHECK_MSTATUS_AND_RETURN_IT(stat);
                MPlug plgWorldMeshes = fnNode.findPlug("worldMesh", false, &stat);
                CHECK_MSTATUS_AND_RETURN_IT(stat);
                unsigned int numWorldMeshesConnected = plgWorldMeshes.numElements();
                if (numWorldMeshesConnected != 1) {
                    return MStatus::kFailure;
                }
                MPlug plgWorldMesh = plgWorldMeshes.elementByPhysicalIndex(0, &stat);
                CHECK_MSTATUS_AND_RETURN_IT(stat);

                if (!plgWorldMesh.isSource()) {
                    return MStatus::kFailure;
                }
                MPlugArray destPlugs;
                plgWorldMesh.destinations(destPlugs);
                for (unsigned int k = 0; k < destPlugs.length(); ++k) {
                    MPlug   destPlug = destPlugs[k];
                    MObject blendShape = destPlug.node();
                    MPlug   inputTargetGroupPlug;
                    MPlug   inputTargetPlug;
                    if (blendShape == curBlendShape) {
                        MPlug inputTargetItemPlug = destPlug.parent();
                        MPlug inputTargetItemsPlug = inputTargetItemPlug.array();
                        inputTargetGroupPlug = inputTargetItemsPlug.parent();
                        weightInfo.inputTargetGroupIndices.append(
                            inputTargetGroupPlug.logicalIndex());

                        MPlug inputTargetGroupsPlug = inputTargetGroupPlug.array();
                        inputTargetPlug = inputTargetGroupsPlug.parent();
                        weightInfo.inputTargetIndices.append(inputTargetPlug.logicalIndex());
                    }
                }
            }

            // NOTE: (yliangsiew) If the target mesh has "in-between" weights, in
            // Maya they are stored as an array of sparse ints, where the formula is:
            // index = fullWeight * 1000 + 5000.
            // Thus fullWeight values of 0.5, 1.0 and 2.0, they will be connected to
            // inputTargetItem array indices 5500, 6000 and 7000, respectively.
            // Refer to the docs for MFnBlendShape::targetItemIndexList for more info.
            stat = fnBlendShape.targetItemIndexList(
                weightInfo.weightIndex, deformedMesh, weightInfo.targetItemIndices);
            CHECK_MSTATUS_AND_RETURN_IT(stat);

            info.weightDatas.push_back(weightInfo);
        }
        outInfos.push_back(info);
    }
    return stat;
}

PXR_NAMESPACE_OPEN_SCOPE

VtIntArray getUnionOfVtIntArrays(const VtIntArray* const arrays, const size_t count)
{
    std::unordered_map<int, int> visitedMap = {};
    for (size_t i = 0; i < count; ++i) {
        const VtIntArray array = arrays[i];
        for (size_t j = 0; j < array.size(); ++j) {
            ++(visitedMap[array[j]]);
        }
    }

    VtIntArray result;
    for (auto it = visitedMap.begin(); it != visitedMap.end(); ++it) {
        if (it->second != 0) {
            result.push_back(it->first);
        }
    }

    return result;
}

MStatus findNormalOffsetsBetweenMeshes(
    const MObject&   target,
    const MObject&   base,
    const MIntArray& indices,
    VtVec3fArray&    offsets)
{
    assert(target.hasFn(MFn::kMesh));
    assert(base.hasFn(MFn::kMesh));

    MStatus stat;
    MFnMesh fnMesh(target, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    unsigned int numNormalsTarget = fnMesh.numNormals(&stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    const float* targetNormals = fnMesh.getRawNormals(&stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    const GfVec3f* pVtTargetNormals = reinterpret_cast<const GfVec3f*>(targetNormals);
    VtVec3fArray   vtTargetNormals(pVtTargetNormals, pVtTargetNormals + numNormalsTarget);

    fnMesh.setObject(base);
    unsigned int numNormalsBase = fnMesh.numNormals(&stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    if (numNormalsTarget != numNormalsBase) {
        return MStatus::kFailure;
    }

    const float* baseNormals = fnMesh.getRawNormals(&stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    const GfVec3f* pVtBaseNormals = reinterpret_cast<const GfVec3f*>(baseNormals);
    VtVec3fArray   vtBaseNormals(pVtBaseNormals, pVtBaseNormals + numNormalsBase);

    unsigned int numIndices = indices.length();
    offsets.resize(numIndices);
    for (unsigned int i = 0; i < numIndices; ++i) {
        int componentIdx = indices[i];
        offsets[i] = vtTargetNormals[componentIdx] - vtBaseNormals[componentIdx];
    }

    return stat;
}

/**
 * Reads data for a target from a blendshape node.
 *
 * @param blendShapeNode        The blendshape deformer.
 * @param inputTargetIndex      The input target sparse index.
 * @param targetGroupIndex      The input target group sparse index.
 * @param targetWeightIndex     The input target item sparse index that also acts as the "weight" of
 * the target.
 * @param outputGeomIndex       The output geometry sparse index.
 * @param targetOffsetIndices   Storage for the indices of the components being affected.
 * @param targetOffsets         Storage for the vertex offsets of the components being affected.
 * @param targetNormalOffsets   Storage for the normal offsets of the components being affected.
 *
 * @return                      A status code.
 */
MStatus readBlendShapeTargetData(
    const MObject& blendShapeNode,
    const int      inputTargetIndex,
    const int      targetGroupIndex,
    const int      targetWeightIndex,
    const int      outputGeomIndex,
    VtIntArray&    targetOffsetIndices,
    VtVec3fArray&  targetOffsets,
    VtVec3fArray&  targetNormalOffsets)
{
    MStatus stat;
    assert(blendShapeNode.hasFn(MFn::kBlendShape));
    MFnDependencyNode fnNode(blendShapeNode, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    MPlug plgInputTargets = fnNode.findPlug("inputTarget", false, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    assert(plgInputTargets.isArray());
    MPlug plgInputTarget = plgInputTargets.elementByPhysicalIndex(inputTargetIndex, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    MPlug plgInputTargetGrps = mayaFindChildPlugWithName(plgInputTarget, "inputTargetGroup");
    assert(!plgInputTargetGrps.isNull());
    assert(plgInputTargetGrps.isArray());

    unsigned int numInputTargetGrps = plgInputTargetGrps.numElements();
    assert(static_cast<int>(numInputTargetGrps) > targetGroupIndex);
    MPlug plgInputTargetGrp = plgInputTargetGrps.elementByPhysicalIndex(targetGroupIndex, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    MPlug plgInputTargetItems = mayaFindChildPlugWithName(plgInputTargetGrp, "inputTargetItem");
    assert(!plgInputTargetItems.isNull());
    assert(plgInputTargetItems.isArray());

#if _DEBUG // TODO: (yliangsiew) Need to find a good macro that I can rely on for this.
    MIntArray plgInputTargetItemLogicalIndices;
    plgInputTargetItems.getExistingArrayAttributeIndices(plgInputTargetItemLogicalIndices, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    assert(mayaSearchMIntArray(weightIndex, plgInputTargetItemLogicalIndices) == true);
#endif

    MPlug plgInputTargetItem = plgInputTargetItems.elementByLogicalIndex(targetWeightIndex, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    MPlug plgInputComponentsTarget
        = mayaFindChildPlugWithName(plgInputTargetItem, "inputComponentsTarget");
    assert(!plgInputComponentsTarget.isNull());

    // NOTE: (yliangsiew) Problem: looks like there's a maya bug where you have to twiddle the
    // blendshape weight directly before these kComponentListData-type plugs get evaluated.
#if MAYA_BLENDSHAPE_EVAL_HOTFIX
    MPlug plgBlendShapeWeights = fnNode.findPlug("weight", false, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    assert(plgBlendShapeWeights.isArray());

    MFnBlendShapeDeformer fnBS(blendShapeNode, &stat);
    MFloatArray           origWeightVals; // Storage for the original weight values.
    MIntArray             indicesChanged;
    MIntArray             targetItemIndexList;
    MIntArray             bsWeightIndices;
    stat = fnBS.weightIndexList(bsWeightIndices);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    // NOTE: (yliangsiew) Need to force the other kComponentListData plugs to
    // be dirtied and recalculated.
    MObjectArray baseObjs;
    stat = fnBS.getBaseObjects(baseObjs);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    for (unsigned int i = 0; i < bsWeightIndices.length(); ++i) {
        int weightIndex = bsWeightIndices[i];
        for (unsigned int j = 0; j < baseObjs.length(); ++j) {
            MObject baseObj = baseObjs[j];
            targetItemIndexList.clear();
            stat = fnBS.targetItemIndexList(weightIndex, baseObj, targetItemIndexList);
            CHECK_MSTATUS_AND_RETURN_IT(stat);

            for (unsigned int k = 0; k < targetItemIndexList.length(); ++k) {
                float curWeight = fnBS.weight(weightIndex, &stat);
                CHECK_MSTATUS_AND_RETURN_IT(stat);
                origWeightVals.append(curWeight);
                // NOTE: (yliangsiew) For in-between shapes, need to trigger at _all_
                // full weight values of each target so as to populate the components
                // list for each of the targets. Yea, this is dumb.
                float targetWeight = mayaGetBlendShapeTargetWeightFromIndex(targetItemIndexList[k]);
                fnBS.setWeight(i, targetWeight);
                indicesChanged.append(i);

                // NOTE: (yliangsiew) We also just force an evaluation
                // of the mesh at each time we set the blendshape
                // weight value in the scene to force the components
                // list to update.
                MFnMesh fnMesh;
                for (unsigned int i = 0; i < baseObjs.length(); ++i) {
                    stat = fnMesh.setObject(baseObjs[i]);
                    CHECK_MSTATUS_AND_RETURN_IT(stat);
                    const float* meshPts = fnMesh.getRawPoints(&stat);
                    CHECK_MSTATUS_AND_RETURN_IT(stat);
                    (void)meshPts;
                }
            }
        }
    }

    // NOTE: (yliangsiew) Restore the weight changes that we were doing above.
    for (unsigned int i = 0; i < indicesChanged.length(); ++i) {
        stat = fnBS.setWeight(indicesChanged[i], origWeightVals[i]);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
    }
#endif

    MDataHandle dhInputComponentsTarget = plgInputComponentsTarget.asMDataHandle(&stat);

    CHECK_MSTATUS_AND_RETURN_IT(stat);
    MObject inputComponentsTargetIndices = dhInputComponentsTarget.data();
    if (inputComponentsTargetIndices.isNull()
        || !inputComponentsTargetIndices.hasFn(MFn::kComponentListData)) {
        return MStatus::kFailure;
    }
    MFnComponentListData fnComponentListData(inputComponentsTargetIndices, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    unsigned int numIndices = fnComponentListData.length();
    if (numIndices == 0) {
        return MStatus::kFailure;
    }

    targetOffsetIndices.clear();
    MIntArray indices;
    for (unsigned int i = 0; i < numIndices; ++i) {
        MObject                   curComponent = fnComponentListData[i];
        MFnSingleIndexedComponent fnSingleIndexedComponent(curComponent, &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        stat = fnSingleIndexedComponent.getElements(indices);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        for (unsigned int j = 0; j < indices.length(); ++j) {
            targetOffsetIndices.push_back(indices[j]);
        }
    }

    MPlug plgInputPointsTarget = mayaFindChildPlugWithName(plgInputTargetItem, "inputPointsTarget");
    MDataHandle dhInputPointsTarget = plgInputPointsTarget.asMDataHandle(&stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    MObject inputPointsTarget = dhInputPointsTarget.data();
    assert(inputPointsTarget.hasFn(MFn::kPointArrayData));

    MFnPointArrayData fnPtArrayData(inputPointsTarget, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    MPointArray  mptTargetOffsets = fnPtArrayData.array();
    unsigned int numOffsets = mptTargetOffsets.length();
    if (numOffsets == 0) { // NOTE: (yliangsiew) There cannot possibly be _no_ offsets in the
                           // blendshape, since we have indices for the offsets.
        return MStatus::kFailure;
    }
    targetOffsets.resize(numOffsets);
    for (unsigned int i = 0; i < numOffsets; ++i) {
        MPoint curPt = mptTargetOffsets[i];
        targetOffsets[i] = GfVec3f(curPt.x, curPt.y, curPt.z);
    }

    MPlug plgTargetGeom = mayaFindChildPlugWithName(plgInputTargetItem, "inputGeomTarget");
    if (plgTargetGeom.isNull()) {
        return MStatus::kFailure;
    }
    MObject targetGeom = plgTargetGeom.asMObject();
    assert(targetGeom.hasFn(MFn::kMesh));

    MPlug plgOutputGeoms = fnNode.findPlug("outputGeometry", false, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    assert(plgOutputGeoms.isArray());

    MPlug plgOutputGeom = plgOutputGeoms.elementByPhysicalIndex(outputGeomIndex, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    MObject outputGeom = plgOutputGeom.asMObject();
    assert(outputGeom.hasFn(MFn::kMesh));

    stat = findNormalOffsetsBetweenMeshes(targetGeom, outputGeom, indices, targetNormalOffsets);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    return stat;
}

void findUnionAndProcessArrays(
    const std::vector<VtIntArray>&   indicesArrays,
    const std::vector<VtVec3fArray>& offsetsArrays,
    const std::vector<VtVec3fArray>& normalsArrays,
    VtIntArray&                      unionIndices,
    std::vector<VtVec3fArray>&       unionOffsetsArrays,
    std::vector<VtVec3fArray>&       unionNormalsArrays)
{
    // NOTE: (yliangsiew) Because according to the USD blendshape schema, the pointIndices
    // mapping applies to all in-between shapes, we need to calculate the union of the indices here:
    std::unordered_map<int, int> visitedIndicesMap = {};
    size_t                       numArrays = indicesArrays.size();
    for (size_t i = 0; i < numArrays; ++i) {
        const VtIntArray& array = indicesArrays[i];
        for (size_t j = 0; j < array.size(); ++j) {
            ++(visitedIndicesMap[array[j]]);
        }
    }

    unionIndices.clear();
    for (const auto& it : visitedIndicesMap) {
        if (it.second != 0) {
            unionIndices.push_back(it.first);
        }
    }

    std::sort(unionIndices.begin(), unionIndices.end());

    size_t numUnionIndices = unionIndices.size();
    unionOffsetsArrays.clear();
    unionOffsetsArrays.resize(offsetsArrays.size());
    unionNormalsArrays.clear();
    unionNormalsArrays.resize(normalsArrays.size());
    for (size_t i = 0; i < numArrays; ++i) {
        const VtVec3fArray& origOffsetsArray = offsetsArrays[i];
        const size_t        numOrigOffsets = origOffsetsArray.size();
        assert(numOrigOffsets != 0);
        VtVec3fArray& newOffsetsArray = unionOffsetsArrays[i];
        newOffsetsArray.assign(numUnionIndices, GfVec3f(0.0f));

        const VtVec3fArray& origNormalsArray = normalsArrays[i];
        const size_t        numOrigNormals = origNormalsArray.size();
        assert(numOrigOffsets == numOrigNormals);
        VtVec3fArray& newNormalsArray = unionNormalsArrays[i];
        newNormalsArray.assign(numUnionIndices, GfVec3f(0.0f));

        const VtIntArray& origIndicesArray = indicesArrays[i];
        for (size_t j = 0, k = 0; j < numUnionIndices; ++j) {
            int index = unionIndices[j];
            int origIndex = origIndicesArray[k];
            if (index != origIndex || k > numOrigOffsets - 1) {
                GfVec3f sentinel(0.0f);
                newOffsetsArray[j] = sentinel;
                newNormalsArray[j] = sentinel;
            } else {
                newOffsetsArray[j] = origOffsetsArray[k];
                newNormalsArray[j] = origNormalsArray[k];
                ++k;
            }
        }
    }

    return;
}

// NOTE: (yliangsiew) This gets called once for each shape being exported
// under a single transform.
MObject PxrUsdTranslators_MeshWriter::writeBlendShapeData(UsdGeomMesh& primSchema)
{
    MStatus                    stat;
    const UsdMayaJobExportArgs exportArgs = this->_GetExportArgs();

    MDagPath deformedMeshDagPath = this->GetDagPath();
    MObject  deformedMesh = deformedMeshDagPath.node();
    assert(deformedMesh.hasFn(MFn::kMesh) == true);

    MObjectArray blendShapeDeformers;
    MIntArray    blendShapeCxnIndices;

    // TODO: (yliangsiew) Figure out if this can be isolated. It's kind of hard
    // because we want to avoid repeated walks through the DG.
    std::vector<MayaBlendShapeDatum> blendShapeDeformerInfos;
    stat = mayaGetBlendShapeInfosForMesh(deformedMesh, blendShapeDeformerInfos);
    if (stat != MStatus::kSuccess) {
        TF_WARN("Cannot find any blendshape deformers for the mesh.");
        return MObject::kNullObj;
    }

    size_t numOfBlendShapeDeformers = blendShapeDeformerInfos.size();
    switch (numOfBlendShapeDeformers) {
    case 0: TF_WARN("Cannot find any blendshape deformers for the mesh."); return MObject::kNullObj;
    case 1: break;
    default:
        // TODO: (yliangsiew) For multiple blend shape deformers, what do we do?
        // Do we collapse the shapes from multiple blend targets together, or just
        // write out only the "closest" blendshape deformer's targets? Or just write all
        // of them and print this warning to end-users?
        TF_WARN("Multiple blendshape deformers were found; while your shapes will still be saved, "
                "since USDSkelBlendShape does not support a deformation stack, results may be "
                "unpredictable on import.");
        break;
    }

    bool exportAnim = !(exportArgs.timeSamples.empty());

    SdfPathVector  usdBlendShapePaths;
    VtTokenArray   usdBlendShapeNames;
    const SdfPath& primSchemaPath = primSchema.GetPrim().GetPath();
    for (size_t i = 0; i < numOfBlendShapeDeformers; ++i) {
        MayaBlendShapeDatum blendShapeInfo = blendShapeDeformerInfos[i];
        // NOTE: (yliangsiew) Each of the weights here we iterate over is equivalent
        // to each individual weight that you are able to toggle on a blendshape node
        // in the Attribute Editor within Maya.
        for (unsigned int j = 0; j < blendShapeInfo.numWeights; ++j) {
            MayaBlendShapeWeightDatum weightInfo = blendShapeInfo.weightDatas[j];
            unsigned int              numTargetItemIndices = weightInfo.targetItemIndices.length();
            switch (numTargetItemIndices) {
            case 0:
                TF_RUNTIME_ERROR("No target indices for the blendshape target could be found. "
                                 "Check that the blendshape was set up correctly.");
                return MObject::kNullObj;
            case 1: { // NOTE: (yliangsiew) Means no inbetweens possible.
                unsigned int numOfTargets = weightInfo.targetMeshes.length();
                for (unsigned int k = 0; k < numOfTargets; ++k) {
                    MObject targetMesh = weightInfo.targetMeshes[k];
                    MString curTargetMeshNameMStr = mayaGetUniqueNameOfDAGNode(targetMesh);
                    assert(curTargetMeshNameMStr.length() != 0);
                    std::string curTargetMeshName
                        = TfMakeValidIdentifier(std::string(curTargetMeshNameMStr.asChar()));
                    SdfPath usdBlendShapePath
                        = primSchemaPath.AppendChild(TfToken(curTargetMeshName));
                    UsdSkelBlendShape usdBlendShape
                        = UsdSkelBlendShape::Define(this->GetUsdStage(), usdBlendShapePath);
                    if (!usdBlendShape) {
                        TF_RUNTIME_ERROR(
                            "Could not create blendshape primitive: <%s>",
                            usdBlendShapePath.GetText());
                        return MObject::kNullObj;
                    }

                    unsigned int inputTargetIndex = weightInfo.inputTargetIndices[k];
                    unsigned int inputTargetGroupIndex = weightInfo.inputTargetGroupIndices[k];
                    unsigned int targetWeightIndex = weightInfo.targetItemIndices[0];

                    VtIntArray   targetIndices;
                    VtVec3fArray targetOffsets;
                    VtVec3fArray targetNormalOffsets;
                    stat = readBlendShapeTargetData(
                        blendShapeInfo.blendShapeDeformer,
                        inputTargetIndex,
                        inputTargetGroupIndex,
                        targetWeightIndex,
                        blendShapeInfo.outputGeomIndex,
                        targetIndices,
                        targetOffsets,
                        targetNormalOffsets);
                    if (stat != MStatus::kSuccess) {
                        TF_RUNTIME_ERROR("Error occurred while attempting to read offset data for "
                                         "the blendshape.");
                        return MObject::kNullObj;
                    }

                    usdBlendShape.CreatePointIndicesAttr(VtValue(targetIndices));
                    usdBlendShape.CreateOffsetsAttr(VtValue(targetOffsets));
                    usdBlendShape.CreateNormalOffsetsAttr(VtValue(targetNormalOffsets));

                    usdBlendShapePaths.emplace_back(usdBlendShapePath);
                    usdBlendShapeNames.push_back(TfToken(curTargetMeshName));

                    // NOTE: (yliangsiew) Because animation export is deferred until subsequent
                    // calls in meshWriter.cpp, we just store the plugs to retrieve the samples from
                    // first, until the time comes to sample them.
                    if (exportAnim) {
                        unsigned int weightIndex = weightInfo.weightIndex;
                        MObject      blendShapeNode = blendShapeInfo.blendShapeDeformer;
                        assert(blendShapeNode.hasFn(MFn::kBlendShape));
                        MFnDependencyNode fnNode(blendShapeNode, &stat);
                        CHECK_MSTATUS_AND_RETURN(stat, MObject::kNullObj);
                        MPlug weightsPlug = fnNode.findPlug(MAYA_ATTR_NAME_WEIGHT, false, &stat);
                        CHECK_MSTATUS_AND_RETURN(stat, MObject::kNullObj);
                        assert(weightsPlug.isArray());
                        MPlug weightPlug = weightsPlug.elementByLogicalIndex(weightIndex);
                        this->_animBlendShapeWeightPlugs.append(weightPlug);
                    }
                }
                break;
            }
            default: {
                // NOTE: (yliangsiew) If there _are_ in-betweens, we just write out the additional
                // in-between shapes and format names for them ourselves based on the weight that
                // they're supposed to activate at.
                unsigned int numOfTargets = weightInfo.targetMeshes.length();
                // NOTE: (yliangsiew) Because of just how USD works; need to create
                // the base shape first before we create the inbetween shapes.
                // For this, we will use the name of the plug at the corresponding weight index.
                MFnDependencyNode fnNode(blendShapeInfo.blendShapeDeformer, &stat);
                if (stat != MStatus::kSuccess) {
                    TF_RUNTIME_ERROR(
                        "Error occurred while attempting to read name for the blendshape.");
                    return MObject::kNullObj;
                }
                MPlug plgBlendShapeWeights = fnNode.findPlug(MAYA_ATTR_NAME_WEIGHT, false, &stat);
                if (stat != MStatus::kSuccess) {
                    TF_RUNTIME_ERROR(
                        "Error occurred while attempting to read name for the blendshape.");
                    return MObject::kNullObj;
                }

                MPlug plgBlendShapeWeight
                    = plgBlendShapeWeights.elementByLogicalIndex(weightInfo.weightIndex);
                MString weightTargetName = plgBlendShapeWeight.partialName(
                    false, false, false, true, false, true, &stat);
                if (stat != MStatus::kSuccess) {
                    TF_RUNTIME_ERROR(
                        "Error occurred while attempting to read name for the blendshape.");
                    return MObject::kNullObj;
                }

                SdfPath usdBlendShapePath
                    = primSchemaPath.AppendChild(TfToken(weightTargetName.asChar()));
                UsdSkelBlendShape usdBlendShape
                    = UsdSkelBlendShape::Define(this->GetUsdStage(), usdBlendShapePath);
                if (!usdBlendShape) {
                    TF_RUNTIME_ERROR(
                        "Could not create blendshape primitive: <%s>", usdBlendShapePath.GetText());
                    return MObject::kNullObj;
                }

                // NOTE: (yliangsiew) Because according to the USD blendshape schema, the
                // pointIndices mapping applies to all in-between shapes, we need to calculate the
                // union of the indices here:
                std::vector<VtIntArray>   indicesArrays(numOfTargets);
                std::vector<VtVec3fArray> targetsOffsetsArrays(numOfTargets);
                std::vector<VtVec3fArray> targetsNormalOffsetsArrays(numOfTargets);

                for (unsigned int k = 0; k < numOfTargets; ++k) {
                    unsigned int inputTargetIndex = weightInfo.inputTargetIndices[k];
                    unsigned int inputTargetGroupIndex = weightInfo.inputTargetGroupIndices[k];
                    unsigned int targetWeightIndex = weightInfo.targetItemIndices[k];
                    VtIntArray   targetIndices;
                    VtVec3fArray targetOffsets;
                    VtVec3fArray targetNormalOffsets;
                    stat = readBlendShapeTargetData(
                        blendShapeInfo.blendShapeDeformer,
                        inputTargetIndex,
                        inputTargetGroupIndex,
                        targetWeightIndex,
                        blendShapeInfo.outputGeomIndex,
                        targetIndices,
                        targetOffsets,
                        targetNormalOffsets);
                    if (stat != MStatus::kSuccess) {
                        TF_RUNTIME_ERROR("Error occurred while attempting to read offset data for "
                                         "the blendshape.");
                        return MObject::kNullObj;
                    }
                    indicesArrays[k] = targetIndices;
                    targetsOffsetsArrays[k] = targetOffsets;
                    targetsNormalOffsetsArrays[k] = targetNormalOffsets;
                }

                VtIntArray                unionIndices;
                std::vector<VtVec3fArray> processedOffsetsArrays;
                std::vector<VtVec3fArray> processedNormalsOffsetsArrays;
                findUnionAndProcessArrays(
                    indicesArrays,
                    targetsOffsetsArrays,
                    targetsNormalOffsetsArrays,
                    unionIndices,
                    processedOffsetsArrays,
                    processedNormalsOffsetsArrays);

                for (unsigned int k = 0; k < numOfTargets; ++k) {
                    MObject     targetMesh = weightInfo.targetMeshes[k];
                    MString     curTargetMeshNameMStr = mayaGetUniqueNameOfDAGNode(targetMesh);
                    std::string curTargetMeshName
                        = TfMakeValidIdentifier(std::string(curTargetMeshNameMStr.asChar()));
                    unsigned int targetWeightIndex = weightInfo.targetItemIndices[k];
                    if (targetWeightIndex == 6000) { // NOTE: (yliangsiew) For default fullweight,
                                                     // we don't append the weight name.
                        usdBlendShape.CreatePointIndicesAttr(VtValue(unionIndices));
                        usdBlendShape.CreateOffsetsAttr(VtValue(processedOffsetsArrays[k]));
                        usdBlendShape.CreateNormalOffsetsAttr(
                            VtValue(processedNormalsOffsetsArrays[k]));
                        usdBlendShapePaths.emplace_back(usdBlendShapePath);
                        usdBlendShapeNames.push_back(TfToken(curTargetMeshName));

                        // NOTE: (yliangsiew) Because animation export is deferred until subsequent
                        // calls in meshWriter.cpp, we just store the plugs to retrieve the samples
                        // from first, until the time comes to sample them.
                        if (exportAnim) {
                            unsigned int weightIndex = weightInfo.weightIndex;
                            MObject      blendShapeNode = blendShapeInfo.blendShapeDeformer;
                            assert(blendShapeNode.hasFn(MFn::kBlendShape));
                            MFnDependencyNode fnNode(blendShapeNode, &stat);
                            CHECK_MSTATUS_AND_RETURN(stat, MObject::kNullObj);
                            MPlug weightsPlug
                                = fnNode.findPlug(MAYA_ATTR_NAME_WEIGHT, false, &stat);
                            CHECK_MSTATUS_AND_RETURN(stat, MObject::kNullObj);
                            assert(weightsPlug.isArray());
                            MPlug weightPlug = weightsPlug.elementByLogicalIndex(weightIndex);
                            this->_animBlendShapeWeightPlugs.append(weightPlug);
                        }
                    } else {
                        float weightValue
                            = mayaGetBlendShapeTargetWeightFromIndex(targetWeightIndex);
                        int     representedWeight = static_cast<int>(weightValue * 100.0f);
                        TfToken usdInbetweenName = TfToken(
                            std::string(curTargetMeshName) + std::string("_")
                            + std::to_string(representedWeight));
                        UsdSkelInbetweenShape usdInbetween
                            = usdBlendShape.CreateInbetween(usdInbetweenName);
                        if (!usdInbetween.IsDefined()) {
                            TF_RUNTIME_ERROR("Error occurred while attempting to define the "
                                             "in-between blendshape.");
                            return MObject::kNullObj;
                        }
                        usdInbetween.SetWeight(weightValue);
                        usdInbetween.SetOffsets(processedOffsetsArrays[k]);
                        usdInbetween.SetNormalOffsets(processedNormalsOffsetsArrays[k]);
                    }
                }
            } break;
            }
        }
    }

    const UsdSkelBindingAPI bindingAPI = UsdSkelBindingAPI::Apply(primSchema.GetPrim());
    UsdAttribute            blendShapesAttr = bindingAPI.CreateBlendShapesAttr();
    blendShapesAttr.Set(VtValue(usdBlendShapeNames));

    UsdRelationship targetsRel = bindingAPI.CreateBlendShapeTargetsRel();
    targetsRel.SetTargets(usdBlendShapePaths);

    SdfPath       blendShapeAnimPath;
    SdfPathVector skelTargets;
    bindingAPI.GetSkeletonRel().GetTargets(&skelTargets);
    size_t numSkelTargets = skelTargets.size();

    if (numSkelTargets > 0) {
        if (exportAnim) {
            blendShapeAnimPath = skelTargets[0].AppendPath(SdfPath("Animation"));
            UsdRelationship animSourceRel = bindingAPI.GetAnimationSourceRel();
            if (!animSourceRel) {
                animSourceRel = bindingAPI.CreateAnimationSourceRel();
            }
            animSourceRel.SetTargets({ blendShapeAnimPath });
        }
    } else {
        // NOTE: (yliangsiew) Do blendshapes _require_ that an empty skeleton be created?
        // Looks like the answer is "yes".
        SdfPath         skelPath;
        UsdSkelSkeleton skel
            = UsdSkelSkeleton::Get(this->GetUsdStage(), primSchemaPath.GetParentPath());
        if (skel) {
            skelPath = skel.GetPath();
        } else {
            skelPath = primSchemaPath.GetParentPath().AppendPath(
                SdfPath(primSchema.GetPrim().GetName().GetString() + "_Skeleton"));
            skel = UsdSkelSkeleton::Define(this->GetUsdStage(), skelPath);
        }
        if (!skel) {
            TF_RUNTIME_ERROR("Could not create skeleton primitive: <%s>", skelPath.GetText());
            return MObject::kNullObj;
        }

        _writeJobCtx.MarkSkelBindings(skelPath, skelPath, exportArgs.exportSkels);
        UsdRelationship skelRel = bindingAPI.CreateSkeletonRel();
        skelRel.SetTargets({ skelPath });

        const UsdSkelBindingAPI skelBindingAPI
            = UsdMayaTranslatorUtil::GetAPISchemaForAuthoring<UsdSkelBindingAPI>(skel.GetPrim());
        if (exportAnim) {
            blendShapeAnimPath = skelPath.AppendPath(SdfPath("Animation"));
            UsdRelationship animSourceRel = skelBindingAPI.GetAnimationSourceRel();
            if (!animSourceRel) {
                animSourceRel = skelBindingAPI.CreateAnimationSourceRel();
            }
            animSourceRel.SetTargets({ blendShapeAnimPath });
        }
    }

    if (!exportAnim) {
        return deformedMesh;
    }

    _skelAnim = UsdSkelAnimation::Get(this->GetUsdStage(), blendShapeAnimPath);
    if (!_skelAnim) {
        _skelAnim = UsdSkelAnimation::Define(this->GetUsdStage(), blendShapeAnimPath);
        if (!_skelAnim) {
            TF_RUNTIME_ERROR(
                "Could not create animation primitive: <%s>", blendShapeAnimPath.GetText());
            return MObject::kNullObj;
        }
    }

    VtTokenArray existingBlendShapeNames;
    UsdAttribute skelAnimBlendShapesAttr = _skelAnim.GetBlendShapesAttr();
    if (skelAnimBlendShapesAttr.HasAuthoredValue()) {
        skelAnimBlendShapesAttr.Get(&existingBlendShapeNames);
    } else {
        skelAnimBlendShapesAttr = _skelAnim.CreateBlendShapesAttr();
    }

    skelAnimBlendShapesAttr.Set(usdBlendShapeNames);

    return deformedMesh;
}

bool PxrUsdTranslators_MeshWriter::writeBlendShapeAnimation(const UsdTimeCode& usdTime)
{
    VtTokenArray existingBlendShapeNames;
    UsdAttribute blendShapesAttr = this->_skelAnim.GetBlendShapesAttr();
    if (!blendShapesAttr) {
        TF_RUNTIME_ERROR("No blendshapes attribute could be found.");
        return false;
    }
    blendShapesAttr.Get(&existingBlendShapeNames);
    size_t       numExistingBlendShapes = existingBlendShapeNames.size();
    VtFloatArray usdWeights(numExistingBlendShapes);
    UsdAttribute blendShapeWeightsAttr = this->_skelAnim.GetBlendShapeWeightsAttr();
    if (blendShapeWeightsAttr.HasAuthoredValue()) {
        blendShapeWeightsAttr.Get(&usdWeights, usdTime);
    } else {
        blendShapeWeightsAttr = this->_skelAnim.CreateBlendShapeWeightsAttr();
    }

    unsigned int numWeightPlugs = this->_animBlendShapeWeightPlugs.length();
    if (numExistingBlendShapes != numWeightPlugs) {
        return false;
    }

    for (size_t i = 0; i < usdWeights.size(); ++i) {
        MPlug weightPlug = this->_animBlendShapeWeightPlugs[i];
        usdWeights[i] = weightPlug.asFloat();
    }

    bool result = blendShapeWeightsAttr.Set(VtValue(usdWeights), usdTime);

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
