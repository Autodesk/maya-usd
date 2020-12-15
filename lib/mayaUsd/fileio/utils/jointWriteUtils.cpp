//
// Copyright 2018 Pixar
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
// Modifications copyright (C) 2020 Autodesk
//
#include "jointWriteUtils.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/fileio/translators/translatorSkel.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/utils/colorSpace.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdSkel/animation.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>
#include <maya/MUintArray.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (Animation)
    (Skeleton)
);
// clang-format on

SdfPath UsdMayaJointUtil::getAnimationPath(const SdfPath& skelPath)
{
    return skelPath.AppendChild(_tokens->Animation);
}

void UsdMayaJointUtil::getJointHierarchyComponents(
    const MDagPath&        dagPath,
    MDagPath*              skelXformPath,
    MDagPath*              jointHierarchyRootPath,
    std::vector<MDagPath>* joints)
{
    if (joints)
        joints->clear();
    *skelXformPath = MDagPath();

    MItDag dagIter(MItDag::kDepthFirst, MFn::kJoint);
    dagIter.reset(dagPath, MItDag::kDepthFirst, MFn::kJoint);

    // The first joint may be the root of a Skeleton.
    if (!dagIter.isDone()) {
        MDagPath path;
        dagIter.getPath(path);
        if (UsdMayaTranslatorSkel::IsUsdSkeleton(path)) {
            *skelXformPath = path;
            dagIter.next();
        }
    }

    // All remaining joints are treated as normal joints.
    if (joints) {
        while (!dagIter.isDone()) {
            MDagPath path;
            dagIter.getPath(path);
            joints->push_back(path);
            dagIter.next();
        }
    }

    if (skelXformPath->isValid()) {
        *jointHierarchyRootPath = *skelXformPath;
    } else {
        *jointHierarchyRootPath = dagPath;
        jointHierarchyRootPath->pop();
    }
}

VtTokenArray UsdMayaJointUtil::getJointNames(
    const std::vector<MDagPath>& joints,
    const MDagPath&              rootDagPath,
    bool                         stripNamespaces)
{
    MDagPath skelXformPath, jointHierarchyRootPath;
    getJointHierarchyComponents(rootDagPath, &skelXformPath, &jointHierarchyRootPath);

    // Get paths relative to the root of the joint hierarchy or the scene root.
    // Joints have to be transforms, so mergeTransformAndShape
    // shouldn't matter here. (Besides, we're not actually using these
    // to point to prims.)
    SdfPath rootPath;
    if (jointHierarchyRootPath.length() == 0) {
        // Joint name relative to the scene root.
        // Note that, in this case, the export will eventually error when trying
        // to obtain the SkelRoot. But it's better that we not error here and
        // only error inside the UsdMaya_SkelBindingsProcessor so that we
        // consolidate the SkelRoot-related errors in one place.
        rootPath = SdfPath::AbsoluteRootPath();
    } else {
        // Joint name relative to joint root.
        rootPath = UsdMayaUtil::MDagPathToUsdPath(
            jointHierarchyRootPath,
            /*mergeTransformAndShape*/ false,
            stripNamespaces);
    }

    VtTokenArray result;
    for (const MDagPath& joint : joints) {

        SdfPath path = UsdMayaUtil::MDagPathToUsdPath(
            joint, /*mergeTransformAndShape*/ false, stripNamespaces);
        result.push_back(path.MakeRelativePath(rootPath).GetToken());
    }
    return result;
}

/// Gets the expected path where a skeleton will be exported for
/// the given root joint. The skeleton both binds a skeleton and
/// holds root transformations of the joint hierarchy.
SdfPath UsdMayaJointUtil::getSkeletonPath(const MDagPath& rootJoint, bool stripNamespaces)
{
    return UsdMayaUtil::MDagPathToUsdPath(
        rootJoint, /*mergeTransformAndShape*/ false, stripNamespaces);
}

MObject UsdMayaJointUtil::getSkinCluster(const MDagPath& dagPath)
{
    MObject currentDagObject = dagPath.node();

    MItDependencyGraph itDG(
        currentDagObject, MFn::kSkinClusterFilter, MItDependencyGraph::kUpstream);
    if (itDG.isDone()) {
        // No skin clusters.
        return MObject::kNullObj;
    }

    MObject skinClusterObj = itDG.currentItem();
    // If there's another skin cluster, then we have multiple skin clusters.
    if (itDG.next() && !itDG.isDone()) {
        TF_WARN(
            "Multiple skinClusters upstream of '%s'; using closest "
            "skinCluster '%s'",
            dagPath.fullPathName().asChar(),
            MFnDependencyNode(skinClusterObj).name().asChar());
    }

    return skinClusterObj;
}

MObject UsdMayaJointUtil::getInputMesh(const MFnSkinCluster& skinCluster)
{
    MStatus status;
    MPlug   inputPlug = skinCluster.findPlug("input", true, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    MPlug inputPlug0 = inputPlug.elementByLogicalIndex(0, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    MPlug inputGeometry = inputPlug0.child(0, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    MObject inputGeometryObj = inputGeometry.asMObject(MDGContext::fsNormal, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    if (!inputGeometryObj.hasFn(MFn::kMesh)) {
        TF_WARN(
            "%s is not a mesh; unable to obtain input mesh for %s",
            inputGeometry.name().asChar(),
            skinCluster.name().asChar());
        return MObject();
    }

    return inputGeometryObj;
}

int UsdMayaJointUtil::getCompressedSkinWeights(
    const MFnMesh&        mesh,
    const MFnSkinCluster& skinCluster,
    VtIntArray*           usdJointIndices,
    VtFloatArray*         usdJointWeights)
{
    // Get the single output dag path from the skin cluster.
    // Note that we can't get the dag path from the mesh because it's the input
    // mesh (and also may not have a dag path).
    MDagPath outputDagPath;
    MStatus  status = skinCluster.getPathAtIndex(0, outputDagPath);
    if (!status) {
        TF_CODING_ERROR(
            "Calling code should have guaranteed that skinCluster "
            "'%s' has at least one output",
            skinCluster.name().asChar());
        return 0;
    }

    // Get all of the weights from the skinCluster in one batch.
    unsigned int              numVertices = mesh.numVertices();
    MFnSingleIndexedComponent components;
    components.create(MFn::kMeshVertComponent);
    components.setCompleteData(numVertices);
    MDoubleArray weights;
    unsigned int numInfluences;
    skinCluster.getWeights(outputDagPath, components.object(), weights, numInfluences);

    // Determine how many influence/weight "slots" we actually need per point.
    // For example, if there are the joints /a, /a/b, and /a/c, but each point
    // only has non-zero weighting for a single joint, then we only need one
    // slot instead of three.
    int maxInfluenceCount = 0;
    for (unsigned int vert = 0; vert < numVertices; ++vert) {
        // Looping through each vertex.
        const unsigned int offset = vert * numInfluences;
        int                influenceCount = 0;
        for (unsigned int i = 0; i < numInfluences; ++i) {
            // Looping through each weight for vertex.
            if (weights[offset + i] != 0.0) {
                influenceCount++;
            }
        }
        maxInfluenceCount = std::max(maxInfluenceCount, influenceCount);
    }

    usdJointIndices->assign(maxInfluenceCount * numVertices, 0);
    usdJointWeights->assign(maxInfluenceCount * numVertices, 0.0);
    for (unsigned int vert = 0; vert < numVertices; ++vert) {
        // Looping through each vertex.
        const unsigned int inputOffset = vert * numInfluences;
        int                outputOffset = vert * maxInfluenceCount;
        for (unsigned int i = 0; i < numInfluences; ++i) {
            // Looping through each weight for vertex.
            float weight = weights[inputOffset + i];
            if (!GfIsClose(weight, 0.0, 1e-8)) {
                (*usdJointIndices)[outputOffset] = i;
                (*usdJointWeights)[outputOffset] = weight;
                outputOffset++;
            }
        }
    }
    return maxInfluenceCount;
}

void UsdMayaJointUtil::warnForPostDeformationTransform(
    const SdfPath&        path,
    const MDagPath&       deformedMeshDag,
    const MFnSkinCluster& skinCluster)
{
    MStatus status;

    MMatrix deformedMeshWorldXf = deformedMeshDag.inclusiveMatrix(&status);
    if (!status)
        return;

    MMatrix bindPreMatrix;
    if (UsdMayaUtil::getPlugMatrix(skinCluster, "bindPreMatrix", &bindPreMatrix)) {

        if (!GfIsClose(
                GfMatrix4d(deformedMeshWorldXf.matrix), GfMatrix4d(bindPreMatrix.matrix), 1e-5)) {
            TF_WARN(
                "Mesh <%s> appears to have a non-identity post-deformation "
                "transform (the 'bindPreMatrix' property of the skinCluster "
                "does not match the inclusive matrix of the deformed mesh). "
                "The resulting skinning in USD may be incorrect.",
                path.GetText());
        }
    }
}

bool UsdMayaJointUtil::getGeomBindTransform(
    const MFnSkinCluster& skinCluster,
    GfMatrix4d*           geomBindXf)
{
    MMatrix geomWorldRestXf;
    if (!UsdMayaUtil::getPlugMatrix(skinCluster, "geomMatrix", &geomWorldRestXf)) {
        // All skinClusters should have geomMatrix, but if not...
        TF_RUNTIME_ERROR(
            "Couldn't read geomMatrix from skinCluster '%s'", skinCluster.name().asChar());
        return false;
    }

    *geomBindXf = GfMatrix4d(geomWorldRestXf.matrix);
    return true;
}

bool UsdMayaJointUtil::writeJointInfluences(
    const MFnSkinCluster&    skinCluster,
    const MFnMesh&           inMesh,
    const UsdSkelBindingAPI& binding)
{
    // The data in the skinCluster is essentially already in the same format
    // as UsdSkel expects, but we're going to compress it by only outputting
    // the nonzero weights.
    VtIntArray   jointIndices;
    VtFloatArray jointWeights;
    int          maxInfluenceCount
        = getCompressedSkinWeights(inMesh, skinCluster, &jointIndices, &jointWeights);

    if (maxInfluenceCount <= 0)
        return false;

    UsdSkelSortInfluences(&jointIndices, &jointWeights, maxInfluenceCount);

    UsdGeomPrimvar indicesPrimvar = binding.CreateJointIndicesPrimvar(false, maxInfluenceCount);
    indicesPrimvar.Set(jointIndices);

    UsdGeomPrimvar weightsPrimvar = binding.CreateJointWeightsPrimvar(false, maxInfluenceCount);
    weightsPrimvar.Set(jointWeights);

    return true;
}

bool UsdMayaJointUtil::writeJointOrder(
    const MDagPath&              rootJoint,
    const std::vector<MDagPath>& jointDagPaths,
    const UsdSkelBindingAPI&     binding,
    const bool                   stripNamespaces)
{
    // Get joint name tokens how PxrUsdTranslators_JointWriter would generate
    // them. We don't need to check that they actually exist.
    VtTokenArray jointNames
        = UsdMayaJointUtil::getJointNames(jointDagPaths, rootJoint, stripNamespaces);

    binding.CreateJointsAttr().Set(jointNames);
    return true;
}

MDagPath UsdMayaJointUtil::getRootJoint(const std::vector<MDagPath>& jointDagPaths)
{
    MDagPath uniqueRoot;

    for (const MDagPath& dagPath : jointDagPaths) {
        // Find the roostmost joint in my ancestor chain.
        // (It's OK if there are intermediary non-joints; just skip them.)
        MDagPath curPath = dagPath;
        MDagPath rootmostJoint = dagPath;
        while (curPath.length() > 0) {
            curPath.pop();
            if (curPath.hasFn(MFn::kJoint)) {
                rootmostJoint = curPath;
            }
        }

        // All root joints must match.
        if (uniqueRoot.isValid()) {
            if (!(uniqueRoot == rootmostJoint)) {
                return MDagPath();
            }
        } else {
            uniqueRoot = rootmostJoint;
        }
    }

    return uniqueRoot;
}

MObject UsdMayaJointUtil::writeSkinningData(
    UsdGeomMesh&               primSchema,
    const SdfPath&             usdPath,
    const MDagPath&            dagPath,
    SdfPath&                   skelPath,
    const bool                 stripNamespaces,
    UsdUtilsSparseValueWriter* valueWriter)
{
    // Figure out if we even have a skin cluster in the first place.
    MObject skinClusterObj = UsdMayaJointUtil::getSkinCluster(dagPath);
    if (skinClusterObj.isNull()) {
        return MObject();
    }
    MFnSkinCluster skinCluster(skinClusterObj);

    MObject inMeshObj = UsdMayaJointUtil::getInputMesh(skinCluster);
    if (inMeshObj.isNull()) {
        return MObject();
    }
    MFnMesh inMesh(inMeshObj);

    // Get all influences and find the rootmost joint.
    MDagPathArray jointDagPathArr;
    if (!skinCluster.influenceObjects(jointDagPathArr)) {
        return MObject();
    }

    std::vector<MDagPath> jointDagPaths(jointDagPathArr.length());
    for (unsigned int i = 0; i < jointDagPathArr.length(); ++i) {
        jointDagPaths[i] = jointDagPathArr[i];
    }

    MDagPath rootJoint = UsdMayaJointUtil::getRootJoint(jointDagPaths);
    if (!rootJoint.isValid()) {
        // No roots or multiple roots!
        // XXX: This is a somewhat arbitrary restriction due to the way that
        // we currently export skeletons in PxrUsdTranslators_JointWriter. We treat an
        // entire joint hierarchy rooted at a single joint as a single skeleton,
        // so when binding the mesh to a skeleton, we have to make sure that
        // we're only binding to a single skeleton.
        //
        // This restrction is largely a consequence of UsdSkel encoding joint
        // transforms in 'skeleton space': We need something that defines a rest
        // (or bind) transform, since otherwise transforming into skeleton space
        // is undefined for the rest pose.
        return MObject();
    }

    // Write everything to USD once we know that we have OK data.
    const UsdSkelBindingAPI bindingAPI
        = UsdMayaTranslatorUtil::GetAPISchemaForAuthoring<UsdSkelBindingAPI>(primSchema.GetPrim());

    if (UsdMayaJointUtil::writeJointInfluences(skinCluster, inMesh, bindingAPI)) {
        UsdMayaJointUtil::writeJointOrder(rootJoint, jointDagPaths, bindingAPI, stripNamespaces);
    }

    GfMatrix4d geomBindTransform;
    if (UsdMayaJointUtil::getGeomBindTransform(skinCluster, &geomBindTransform)) {
        UsdMayaWriteUtil::SetAttribute(
            bindingAPI.CreateGeomBindTransformAttr(),
            &geomBindTransform,
            UsdTimeCode::Default(),
            valueWriter);
    }

    UsdMayaJointUtil::warnForPostDeformationTransform(usdPath, dagPath, skinCluster);

    skelPath = UsdMayaJointUtil::getSkeletonPath(rootJoint, stripNamespaces);

    // Export will create a Skeleton at the location corresponding to
    // the root joint. Configure this mesh to be bound to the same skel.
    bindingAPI.CreateSkeletonRel().SetTargets({ skelPath });

    return inMeshObj;
}

PXR_NAMESPACE_CLOSE_SCOPE