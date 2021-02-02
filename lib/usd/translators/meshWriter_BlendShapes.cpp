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

#include <maya/MAnimUtil.h>
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

constexpr char kMayaAttrNameWeight[] = "weight";
constexpr char kMayaAttrNameBlendShapeInTgt[] = "inputTarget";
constexpr char kMayaAttrNameBlendShapeInTgtGrp[] = "inputTargetGroup";
constexpr char kMayaAttrNameBlendShapeInTgtItem[] = "inputTargetItem";
constexpr char kMayaAttrNameBlendShapeInGeomTgt[] = "inputGeomTarget";
constexpr char kMayaAttrNameBlendShapeInCompsTgt[] = "inputComponentsTarget";
constexpr char kMayaAttrNameBlendShapeInPtsTgt[] = "inputPointsTarget";

PXR_NAMESPACE_OPEN_SCOPE

/// The information about a single blendshape target.
struct MayaBlendShapeTargetDatum
{
    MObject targetMesh;         // May be a null `MObject` if the target is already "baked" into the
                                // blendshape deformer.
    VtVec3fArray ptOffsets;     // The actual relative offsets of the vertices for the mesh.
    VtVec3fArray normalOffsets; // If `targetMesh` is null, this will be an array of 0.0 offsets
                                // equal in size to `ptOffsets` (because Maya itself does not
                                // inherently support blendshape normal offsets.)
    VtIntArray indices; // The indices of the components that are offset. This will be the equal to
                        // the size of `ptOffsets` and `normalOffsets`.
};

/// The information about the set of targets associated with a given `weightIndex` (i.e. one of the
/// weight element plugs on a blendshape node.)
struct MayaBlendShapeWeightDatum
{
    // The individual targets that are associated with the `weightIndex`. Could be individual
    // targets, or in-between targets.
    std::vector<MayaBlendShapeTargetDatum> targets;

    // The input group indices at which each target is connected under. (i.e. inputTargetGroup[0]
    // plug)
    MIntArray inputTargetGroupIndices;

    // The Maya blendshape weight indices for the resulting deformed mesh shape. These indices
    // directly affect the weight at which the target is triggered. (i.e. inputTargetItem[6000]
    // plug)
    MIntArray targetItemIndices;

    // The input index  at which the target(s) are connected under. (i.e. inputTarget[0] plug)
    unsigned int inputTargetIndex;

    // The logical index of the weight attribute on the blendshape node.
    unsigned int weightIndex;
};

/// The information about a single blendshape node.
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

MStatus mayaFindPtAndNormalOffsetsBetweenMeshes(
    const MObject&    a,
    const MObject&    b,
    VtVec3fArray&     ptOffsets,
    VtVec3fArray&     nrmOffsets,
    const VtIntArray& indices)
{
    MStatus status;
    TF_VERIFY(MObjectHandle(a).isAlive() && MObjectHandle(b).isAlive());
    if (!a.hasFn(MFn::kMesh) || !b.hasFn(MFn::kMesh)) {
        return MStatus::kInvalidParameter;
    }

    size_t numIndices = indices.size();
    if (numIndices == 0) {
        return MStatus::kInvalidParameter;
    }

    ptOffsets.resize(numIndices);
    nrmOffsets.resize(numIndices);

    MFnMesh fnMesh(a, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    const float* nrmsA = fnMesh.getRawNormals(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    const GfVec3f* pVtNrmsA = reinterpret_cast<const GfVec3f*>(nrmsA);

    // TODO: (yliangsiew) Need to account for float/double meshes.
    const float* ptsA = fnMesh.getRawPoints(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    const GfVec3f* pVtPtsA = reinterpret_cast<const GfVec3f*>(ptsA);

    status = fnMesh.setObject(b);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    const float* nrmsB = fnMesh.getRawNormals(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    const GfVec3f* pVtNrmsB = reinterpret_cast<const GfVec3f*>(nrmsB);

    // TODO: (yliangsiew) Need to account for float/double meshes.
    const float* ptsB = fnMesh.getRawPoints(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    const GfVec3f* pVtPtsB = reinterpret_cast<const GfVec3f*>(ptsB);

    for (unsigned int i = 0; i < numIndices; ++i) {
        const int     componentIdx = indices[i];
        const GfVec3f ptA = pVtPtsA[componentIdx];
        const GfVec3f ptB = pVtPtsB[componentIdx];
        ptOffsets[i] = ptB - ptA;

        const GfVec3f nrmA = pVtNrmsA[componentIdx];
        const GfVec3f nrmB = pVtNrmsB[componentIdx];
        nrmOffsets[i] = nrmB - nrmA;
    }

    return status;
}

#if MAYA_BLENDSHAPE_EVAL_HOTFIX
MStatus mayaBlendShapeTriggerAllTargets(MObject& blendShape)
{
    MStatus status;
    TF_VERIFY(blendShape.hasFn(MFn::kBlendShape));

    MFnBlendShapeDeformer fnBS(blendShape, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MIntArray weightIndices;
    status = fnBS.weightIndexList(weightIndices);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    unsigned int numWeights = fnBS.numWeights();
    MFloatArray  origWeights(numWeights);

    MObjectArray baseObjs;
    status = fnBS.getBaseObjects(baseObjs);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MIntArray targetItemIndices;
    MFnMesh   fnMesh;
    // NOTE: (yliangsiew) Save out the original weights first to restore them after.
    for (unsigned int i = 0; i < weightIndices.length(); ++i) {
        const int weightIndex = weightIndices[i];
        origWeights[i] = fnBS.weight(weightIndex, &status);

        for (unsigned int j = 0; j < baseObjs.length(); ++j) {
            const MObject baseObj = baseObjs[j];
            targetItemIndices.clear();
            status = fnBS.targetItemIndexList(weightIndex, baseObj, targetItemIndices);
            CHECK_MSTATUS_AND_RETURN_IT(status);

            for (unsigned int k = 0; k < targetItemIndices.length(); ++k) {
                // NOTE: (yliangsiew) For in-between shapes, need to trigger at _all_
                // full weight values of each target so as to populate the components
                // list for each of the targets. Yea, this is dumb.
                float targetWeight = mayaGetBlendShapeTargetWeightFromIndex(targetItemIndices[k]);
                fnBS.setWeight(weightIndex, targetWeight);

                // NOTE: (yliangsiew) We also just force an evaluation
                // of the mesh at each time we set the blendshape
                // weight value in the scene to force the components
                // list to update.
                for (unsigned int m = 0; m < baseObjs.length(); ++m) {
                    status = fnMesh.setObject(baseObjs[m]);
                    CHECK_MSTATUS_AND_RETURN_IT(status);
                    const float* meshPts = fnMesh.getRawPoints(&status);
                    CHECK_MSTATUS_AND_RETURN_IT(status);
                    (void)meshPts;
                }
            }
        }

        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    // NOTE: (yliangsiew) Restore the original weights after twiddling around with them.
    for (unsigned int i = 0; i < numWeights; ++i) {
        const int   weightIndex = weightIndices[i];
        const float origWeight = origWeights[i];
        status = fnBS.setWeight(weightIndex, origWeight);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return status;
}
#endif

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
    stat = UsdMayaMeshWriteUtils::getSkinClustersUpstreamOfMesh(deformedMesh, skinClusters);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    unsigned int numSkinClusters = skinClusters.length();
    switch (numSkinClusters) {
    case 0: searchObject = MObject(deformedMesh); break;
    case 1: searchObject = MObject(skinClusters[0]); break;
    default:
        TF_WARN("More than one skinCluster was found; only the first one will be "
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
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    MFnDependencyNode fnNode;
    for (; !itDg.isDone(); itDg.next()) {
        MObject curBlendShape = itDg.currentItem();
        TF_VERIFY(curBlendShape.hasFn(MFn::kBlendShape));

        MPlug outputGeomPlug = itDg.thisPlug();
        TF_VERIFY(outputGeomPlug.isElement() == true);
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
        MObject outputGeo = fnGeoFilter.outputShapeAtIndex(outputGeomPlugIdx, &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        info.baseMesh = inputGeo;
        info.deformedMesh = deformedMesh;

        fnBlendShape.setObject(curBlendShape);
        info.numWeights = fnBlendShape.numWeights();
        MIntArray weightIndices;
        stat = fnBlendShape.weightIndexList(weightIndices);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        // NOTE: (yliangsiew) Ok, so for each weight, need to go targetItemIndexList()
        // for each base object, then use the base object outputGeometry logicalIndex and
        // assume that it is the same logical index for the inputTarget logical index; this
        // is the way (that we will find the inputTargetItem plug to read component data from)
        MPlug plgInTgts = fnBlendShape.findPlug(kMayaAttrNameBlendShapeInTgt, false, &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);
        MPlug plgInTgt = plgInTgts.elementByLogicalIndex(outputGeomPlugIdx, &stat);
        CHECK_MSTATUS_AND_RETURN_IT(stat);

        // NOTE: (yliangsiew) So after we call targetItemIndexList which associates a given weight
        // index and a base object, we can infer that the inputTarget group logical index matches
        // that of the outputGeometry logical index (i.e. the base object).
        // The logical index of the weight plug should match that of the
        // inputTargetGroup that is being driven by said weight. (Confirmed by @williamkrick from
        // ADSK).
        MPlug plgInTgtGrps
            = UsdMayaUtil::FindChildPlugWithName(plgInTgt, kMayaAttrNameBlendShapeInTgtGrp);
        TF_VERIFY(!plgInTgtGrps.isNull());

        // NOTE: (yliangsiew) Problem: looks like there's a maya bug where you have to twiddle the
        // blendshape weight directly before these kComponentListData-type plugs get evaluated.
#if MAYA_BLENDSHAPE_EVAL_HOTFIX
        mayaBlendShapeTriggerAllTargets(curBlendShape);
#endif

        for (unsigned int i = 0; i < weightIndices.length(); ++i) {
            MayaBlendShapeWeightDatum weightInfo = {};
            weightInfo.weightIndex = weightIndices[i];
            weightInfo.inputTargetIndex = outputGeomPlugIdx;

            stat = fnBlendShape.targetItemIndexList(
                weightInfo.weightIndex, outputGeo, weightInfo.targetItemIndices);
            CHECK_MSTATUS_AND_RETURN_IT(stat);

            unsigned int numInTgtGrps = plgInTgtGrps.numElements();
            if (numInTgtGrps == 0) {
                continue;
            }

            // NOTE: (yliangsiew) The index of the inputTargetGroup should match the index into the
            // weight attribute.
            MPlug plgInTgtGrp = plgInTgtGrps.elementByLogicalIndex(weightInfo.weightIndex);

            unsigned int inTgtGrpIdx = plgInTgtGrp.logicalIndex();
            weightInfo.inputTargetGroupIndices.append(inTgtGrpIdx);

            MPlug plgInTgtItems
                = UsdMayaUtil::FindChildPlugWithName(plgInTgtGrp, kMayaAttrNameBlendShapeInTgtItem);
            for (unsigned int k = 0; k < weightInfo.targetItemIndices.length(); ++k) {
                MPlug plgInTgtItem
                    = plgInTgtItems.elementByLogicalIndex(weightInfo.targetItemIndices[k]);
                MPlug plgInGeomTgt = UsdMayaUtil::FindChildPlugWithName(
                    plgInTgtItem, kMayaAttrNameBlendShapeInGeomTgt);
                TF_VERIFY(!plgInGeomTgt.isNull());

                // NOTE: (yliangsiew) Get the indices first so that we know which
                // components to calculate the offsets for.
                MPlug plgInComponentsTgt = UsdMayaUtil::FindChildPlugWithName(
                    plgInTgtItem, kMayaAttrNameBlendShapeInCompsTgt);
                TF_VERIFY(!plgInComponentsTgt.isNull());

                MayaBlendShapeTargetDatum meshTargetDatum = {};
                MIntArray                 indices;
                stat = UsdMayaUtil::GetAllIndicesFromComponentListDataPlug(
                    plgInComponentsTgt, indices);
                CHECK_MSTATUS_AND_RETURN_IT(stat);
                for (unsigned int m = 0; m < indices.length(); ++m) {
                    meshTargetDatum.indices.push_back(indices[m]);
                }

                unsigned int numComponentIndices = meshTargetDatum.indices.size();
                if (numComponentIndices == 0) {
                    TF_WARN(
                        "Found zero-length component indices on a plug; cannot determine "
                        "blendshape target info from it: %s",
                        plgInComponentsTgt.name().asChar());
                    continue;
                }

                // NOTE: (yliangsiew) We check if the geometry target is actually connected.
                // If it is, we can use that to find normal offset information. If it's not, we
                // have to assume normals have no offsets since Maya doesn't support them in
                // blendshapes.
                if (plgInGeomTgt.isDestination()) {
                    // TODO: (yliangsiew) Maybe DG iterator to walk to the mesh? But for now, all
                    // testing seems to imply direct connections are the default...
                    MPlug plgInGeomTgtSrc = plgInGeomTgt.source(&stat);
                    CHECK_MSTATUS_AND_RETURN_IT(stat);

                    MObject meshInGeomTgt = plgInGeomTgtSrc.node();
                    TF_VERIFY(meshInGeomTgt.hasFn(MFn::kMesh));

                    meshTargetDatum.targetMesh = meshInGeomTgt;
                    mayaFindPtAndNormalOffsetsBetweenMeshes(
                        inputGeo,
                        meshInGeomTgt,
                        meshTargetDatum.ptOffsets,
                        meshTargetDatum.normalOffsets,
                        meshTargetDatum.indices);
                } else {
                    // NOTE: (yliangsiew) If there is no geometry target, then we have to assume
                    // the target has already been "baked" into the blendshape deformer. In this
                    // case we need to compute the deltas manually for the points.
                    meshTargetDatum.normalOffsets.resize(
                        numComponentIndices); // NOTE: (yliangsiew) Zeroed out normal offsets.
                    MPlug plgInPtsTgt = UsdMayaUtil::FindChildPlugWithName(
                        plgInTgtItem, kMayaAttrNameBlendShapeInPtsTgt);
                    TF_VERIFY(!plgInPtsTgt.isNull());
                    MObject inPtsTgtData = plgInPtsTgt.asMObject(&stat);
                    CHECK_MSTATUS_AND_RETURN_IT(stat);
                    MFnPointArrayData fnPtArrayData(inPtsTgtData, &stat);
                    CHECK_MSTATUS_AND_RETURN_IT(stat);

                    MPointArray ptDeltas = fnPtArrayData.array();
                    for (unsigned int m = 0; m < numComponentIndices; ++m) {
                        MPoint pt = ptDeltas[m];
                        meshTargetDatum.ptOffsets.push_back(GfVec3f(pt.x, pt.y, pt.z));
                    }
                }
                weightInfo.targets.push_back(meshTargetDatum);
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
        TF_VERIFY(numOrigOffsets != 0);
        VtVec3fArray& newOffsetsArray = unionOffsetsArrays[i];
        newOffsetsArray.assign(numUnionIndices, GfVec3f(0.0f));

        const VtVec3fArray& origNormalsArray = normalsArrays[i];
#ifdef _DEBUG
        const size_t numOrigNormals = origNormalsArray.size();
        TF_VERIFY(numOrigOffsets == numOrigNormals);
#endif
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
    TF_VERIFY(deformedMesh.hasFn(MFn::kMesh) == true);

    MObjectArray blendShapeDeformers;
    MIntArray    blendShapeCxnIndices;

    // TODO: (yliangsiew) Figure out if this can be isolated. It's kind of hard
    // because we want to avoid repeated walks through the DG.
    std::vector<MayaBlendShapeDatum> blendShapeDeformerInfos;
    stat = mayaGetBlendShapeInfosForMesh(deformedMesh, blendShapeDeformerInfos);
    if (stat != MStatus::kSuccess) {
        TF_WARN(
            "Could not read blendshape information for the mesh: %s.",
            deformedMeshDagPath.fullPathName().asChar());
        return MObject::kNullObj;
    }

    size_t numOfBlendShapeDeformers = blendShapeDeformerInfos.size();
    switch (numOfBlendShapeDeformers) {
    case 0:
        TF_WARN(
            "Cannot find any blendshape deformers for the mesh: %s",
            deformedMeshDagPath.fullPathName().asChar());
        return MObject::kNullObj;
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
            case 1: { // NOTE: (yliangsiew) Means no inbetweens possible. i.e. [6000] only.
                size_t numOfTargets = weightInfo.targets.size();
                for (size_t k = 0; k < numOfTargets; ++k) {
                    MayaBlendShapeTargetDatum targetDatum = weightInfo.targets[k];
                    MObject                   targetMesh = targetDatum.targetMesh;
                    MString                   curTargetNameMStr;
                    if (!targetMesh.isNull()) {
                        // NOTE: (yliangsiew) Because UsdSkelBlendShape does not
                        // support animated targets (the `normalOffsets` and
                        // `offsets` attributes are defined as uniforms), we cannot
                        // fully support it in the exporter either.
                        if (MObjectHandle(targetMesh).isAlive() && targetMesh.hasFn(MFn::kMesh)
                            && MAnimUtil::isAnimated(targetMesh)) {
                            TF_RUNTIME_ERROR(
                                "Animated blendshapes are not supported in USD. Please "
                                "bake down deformer history and remove existing "
                                "connections first before attempting to export.");
                            return MObject::kNullObj;
                        }
                        curTargetNameMStr = UsdMayaUtil::GetUniqueNameOfDAGNode(targetMesh);
                    } else {
                        MFnDependencyNode fnNode(blendShapeInfo.blendShapeDeformer, &stat);
                        CHECK_MSTATUS_AND_RETURN(stat, MObject::kNullObj);
                        MPlug plgBlendShapeWeights = fnNode.findPlug(kMayaAttrNameWeight);
                        MPlug plgBlendShapeWeight
                            = plgBlendShapeWeights.elementByLogicalIndex(weightInfo.weightIndex);
                        MString plgBlendShapeName = plgBlendShapeWeight.partialName(
                            0,
                            0,
                            0,
                            1,
                            0,
                            0,
                            &stat); // NOTE: (yliangsiew) The target name is set as an alias, so
                                    // we'll use that instead of calling our target "weight_".
                        CHECK_MSTATUS_AND_RETURN(stat, MObject::kNullObj);
                        // NOTE: (yliangsiew) Because a single weight can drive multiple targets, we
                        // have to put a numeric suffix in the target name.
                        if (k == 0) {
                            curTargetNameMStr = MString(TfStringPrintf("%s", plgBlendShapeName.asChar()).c_str());
                        } else {
                            curTargetNameMStr = MString(TfStringPrintf("%s%zu", plgBlendShapeName.asChar(), k).c_str());
                        }
                    }

                    TF_VERIFY(curTargetNameMStr.length() != 0);
                    std::string curTargetName
                        = TfMakeValidIdentifier(std::string(curTargetNameMStr.asChar()));
                    SdfPath usdBlendShapePath = primSchemaPath.AppendChild(TfToken(curTargetName));
                    UsdSkelBlendShape usdBlendShape
                        = UsdSkelBlendShape::Define(this->GetUsdStage(), usdBlendShapePath);
                    if (!usdBlendShape) {
                        TF_RUNTIME_ERROR(
                            "Could not create blendshape primitive: <%s>",
                            usdBlendShapePath.GetText());
                        return MObject::kNullObj;
                    }

                    usdBlendShape.CreatePointIndicesAttr(VtValue(targetDatum.indices));
                    usdBlendShape.CreateOffsetsAttr(VtValue(targetDatum.ptOffsets));
                    usdBlendShape.CreateNormalOffsetsAttr(VtValue(targetDatum.normalOffsets));

                    usdBlendShapePaths.push_back(usdBlendShapePath);
                    usdBlendShapeNames.push_back(TfToken(curTargetName));

                    // NOTE: (yliangsiew) Because animation export is deferred until subsequent
                    // calls in meshWriter.cpp, we just store the plugs to retrieve the samples from
                    // first, until the time comes to sample them.
                    if (exportAnim) {
                        unsigned int weightIndex = weightInfo.weightIndex;
                        MObject      blendShapeNode = blendShapeInfo.blendShapeDeformer;
                        TF_VERIFY(blendShapeNode.hasFn(MFn::kBlendShape));
                        MFnDependencyNode fnNode(blendShapeNode, &stat);
                        CHECK_MSTATUS_AND_RETURN(stat, MObject::kNullObj);
                        MPlug weightsPlug = fnNode.findPlug(kMayaAttrNameWeight, false, &stat);
                        CHECK_MSTATUS_AND_RETURN(stat, MObject::kNullObj);
                        TF_VERIFY(weightsPlug.isArray());
                        MPlug weightPlug = weightsPlug.elementByLogicalIndex(weightIndex);
                        this->_animBlendShapeWeightPlugs.append(weightPlug);
                    }
                }
                break;
            }
            default: { // NOTE: (yliangsiew) Multiple target item indices (i.e. [6000, 5500, 5000,
                       // etc.])
                // NOTE: (yliangsiew) If there _are_ in-betweens, we just write out the additional
                // in-between shapes and format names for them ourselves based on the weight that
                // they're supposed to activate at.
                size_t numOfTargets = weightInfo.targets.size();

                // NOTE: (yliangsiew) Because UsdSkelBlendShape does not support
                // animated targets (the `normalOffsets` and `offsets`
                // attributes are defined as uniforms), we cannot fully support
                // it in the exporter either.
                for (size_t k = 0; k < numOfTargets; ++k) {
                    MayaBlendShapeTargetDatum targetDatum = weightInfo.targets[k];
                    MString                   curTargetNameMStr;
                    MObject                   targetMesh = targetDatum.targetMesh;
                    if (!targetMesh.isNull() && MObjectHandle(targetMesh).isAlive()
                        && targetMesh.hasFn(MFn::kMesh) && MAnimUtil::isAnimated(targetMesh)) {
                        // NOTE: (yliangsiew) Because UsdSkelBlendShape does not
                        // support animated targets (the `normalOffsets` and
                        // `offsets` attributes are defined as uniforms), we cannot
                        // fully support it in the exporter either.
                        TF_RUNTIME_ERROR("Animated blendshapes are not supported in USD. Please "
                                         "bake down deformer history and remove existing "
                                         "connections first before attempting to export.");
                        return MObject::kNullObj;
                    }
                }

                // NOTE: (yliangsiew) Because of just how USD works; need to create
                // the base shape first before we create the inbetween shapes.
                // For this, we will use the name of the plug at the corresponding weight index.
                MFnDependencyNode fnNode(blendShapeInfo.blendShapeDeformer, &stat);
                if (stat != MStatus::kSuccess) {
                    TF_RUNTIME_ERROR(
                        "Error occurred while attempting to read name for the blendshape.");
                    return MObject::kNullObj;
                }
                MPlug plgBlendShapeWeights = fnNode.findPlug(kMayaAttrNameWeight, false, &stat);
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

                for (size_t k = 0; k < numOfTargets; ++k) {
                    MayaBlendShapeTargetDatum targetDatum = weightInfo.targets[k];
                    indicesArrays[k] = targetDatum.indices;
                    targetsOffsetsArrays[k] = targetDatum.ptOffsets;
                    targetsNormalOffsetsArrays[k] = targetDatum.normalOffsets;
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

                for (size_t k = 0; k < numOfTargets; ++k) {
                    MayaBlendShapeTargetDatum targetDatum = weightInfo.targets[k];
                    MObject                   targetMesh = targetDatum.targetMesh;
                    // NOTE: (yliangsiew) If mesh is already baked in, format name differently.
                    MString curTargetNameMStr;
                    if (!targetMesh.isNull()) {
                        // NOTE: (yliangsiew) Because UsdSkelBlendShape does not
                        // support animated targets (the `normalOffsets` and
                        // `offsets` attributes are defined as uniforms), we cannot
                        // fully support it in the exporter either.
                        if (MObjectHandle(targetMesh).isAlive() && targetMesh.hasFn(MFn::kMesh)
                            && MAnimUtil::isAnimated(targetMesh)) {
                            TF_RUNTIME_ERROR(
                                "Animated blendshapes are not supported in USD. Please "
                                "bake down deformer history and remove existing "
                                "connections first before attempting to export.");
                            return MObject::kNullObj;
                        }
                        curTargetNameMStr = UsdMayaUtil::GetUniqueNameOfDAGNode(targetMesh);
                    } else {
                        MFnDependencyNode fnNode(blendShapeInfo.blendShapeDeformer, &stat);
                        CHECK_MSTATUS_AND_RETURN(stat, MObject::kNullObj);
                        MPlug plgBlendShapeWeights = fnNode.findPlug(kMayaAttrNameWeight);
                        MPlug plgBlendShapeWeight
                            = plgBlendShapeWeights.elementByLogicalIndex(weightInfo.weightIndex);
                        MString plgBlendShapeName = plgBlendShapeWeight.partialName(
                            0,
                            0,
                            0,
                            1,
                            0,
                            0,
                            &stat); // NOTE: (yliangsiew) The target name is set as an alias, so
                                    // we'll use that instead of calling our target "weight_".
                        CHECK_MSTATUS_AND_RETURN(stat, MObject::kNullObj);
                        // NOTE: (yliangsiew) Because a single weight can drive multiple targets, we
                        // have to put a numeric suffix in the target name.
                        if (k == 0) {
                            curTargetNameMStr = MString(TfStringPrintf("%s", plgBlendShapeName.asChar()).c_str());
                        } else {
                            curTargetNameMStr = MString(TfStringPrintf("%s%zu", plgBlendShapeName.asChar(), k).c_str());
                        }
                    }
                    TF_VERIFY(curTargetNameMStr.length() != 0);
                    std::string curTargetName
                        = TfMakeValidIdentifier(std::string(curTargetNameMStr.asChar()));
                    unsigned int targetWeightIndex = weightInfo.targetItemIndices[k];
                    if (targetWeightIndex == 6000) { // NOTE: (yliangsiew) For default fullweight,
                                                     // we don't append the weight name.
                        usdBlendShape.CreatePointIndicesAttr(VtValue(unionIndices));
                        usdBlendShape.CreateOffsetsAttr(VtValue(processedOffsetsArrays[k]));
                        usdBlendShape.CreateNormalOffsetsAttr(
                            VtValue(processedNormalsOffsetsArrays[k]));
                        usdBlendShapePaths.push_back(usdBlendShapePath);
                        usdBlendShapeNames.push_back(TfToken(curTargetName));

                        // NOTE: (yliangsiew) Because animation export is deferred until subsequent
                        // calls in meshWriter.cpp, we just store the plugs to retrieve the samples
                        // from first, until the time comes to sample them.
                        if (exportAnim) {
                            unsigned int weightIndex = weightInfo.weightIndex;
                            MObject      blendShapeNode = blendShapeInfo.blendShapeDeformer;
                            TF_VERIFY(blendShapeNode.hasFn(MFn::kBlendShape));
                            MFnDependencyNode fnNode(blendShapeNode, &stat);
                            CHECK_MSTATUS_AND_RETURN(stat, MObject::kNullObj);
                            MPlug weightsPlug = fnNode.findPlug(kMayaAttrNameWeight, false, &stat);
                            CHECK_MSTATUS_AND_RETURN(stat, MObject::kNullObj);
                            TF_VERIFY(weightsPlug.isArray());
                            MPlug weightPlug = weightsPlug.elementByLogicalIndex(weightIndex);
                            this->_animBlendShapeWeightPlugs.append(weightPlug);
                        }
                    } else {
                        float weightValue
                            = mayaGetBlendShapeTargetWeightFromIndex(targetWeightIndex);
                        int     representedWeight = static_cast<int>(weightValue * 100.0f);
                        TfToken usdInbetweenName = TfToken(
                            std::string(curTargetName) + std::string("_")
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
