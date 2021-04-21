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
// Modifications copyright (C) 2020 Autodesk
//

#ifndef PXRUSDMAYA_JOINT_WRITE_UTILS_H
#define PXRUSDMAYA_JOINT_WRITE_UTILS_H

#include <mayaUsd/base/api.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdSkel/skeleton.h>
#include <pxr/usd/usdSkel/utils.h>
#include <pxr/usd/usdUtils/sparseValueWriter.h>

#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MObject.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE

// Utilities for dealing with writing out joint and skin data.
namespace UsdMayaJointUtil {
/// Gets all of the components of the joint hierarchy rooted at \p dagPath.
/// The \p skelXformPath will hold the path to a joint that defines
/// the transform of a UsdSkelSkeleton. It may be invalid if no
/// joint explicitly defines that transform.
/// The \p joints array, if provided, will be filled with the ordered set of
/// joint paths, excluding the set of joints described above.
/// The \p jointHierarchyRootPath will hold the common parent path of
/// all of the returned joints.
MAYAUSD_CORE_PUBLIC
void getJointHierarchyComponents(
    const MDagPath&        dagPath,
    MDagPath*              skelXformPath,
    MDagPath*              jointHierarchyRootPath,
    std::vector<MDagPath>* joints = nullptr);

MAYAUSD_CORE_PUBLIC
SdfPath getAnimationPath(const SdfPath& skelPath);

/// Gets the joint name tokens for the given dag paths, assuming a joint
/// hierarchy with the given root joint.
MAYAUSD_CORE_PUBLIC
VtTokenArray
getJointNames(const std::vector<MDagPath>& joints, const MDagPath& rootJoint, bool stripNamespaces);

/// Gets the expected path where a skeleton will be exported for
/// the given root joint. The skeleton both binds a skeleton and
/// holds root transformations of the joint hierarchy.
MAYAUSD_CORE_PUBLIC
SdfPath getSkeletonPath(const MDagPath& rootJoint, bool stripNamespaces, const SdfPath& parentScopePath);

/// Gets the closest upstream skin cluster for the mesh at the given dag path.
/// Warns if there is more than one skin cluster.
MAYAUSD_CORE_PUBLIC
MObject getSkinCluster(const MDagPath& dagPath);

/// Finds the input (pre-skin) mesh for the given skin cluster.
/// Warning, do not use MFnSkinCluster::getInputGeometry; it will give you
/// the wrong results (or rather, not the ones we want here).
/// Given the following (simplified) DG:
///     pCubeShape1Orig.worldMesh[0] -> tweak1.inputGeometry
///     tweak1.outputGeometry[0] -> skinCluster1.input[0].inputGeometry
///     skinCluster1.outputGeometry[0] -> pCubeShape1.inMesh
/// Requesting the input geometry for skinCluster1 will give you the mesh
///     pCubeShape1Orig
/// and not
///     tweak1.outputGeometry
/// as desired for this use case.
/// For best results, read skinCluster1.input[0].inputGeometry directly.
/// Note that the Maya documentation states "a skinCluster node can deform
/// only a single geometry" [1] so we are free to ignore any input geometries
/// after the first one.
/// [1]: http://help.autodesk.com/view/MAYAUL/2018/ENU/?guid=__cpp_ref_class_m_fn_skin_cluster_html
MAYAUSD_CORE_PUBLIC
MObject getInputMesh(const MFnSkinCluster& skinCluster);

/// Gets skin weights, and compresses them into the form expected by
/// UsdSkelBindingAPI, which allows us to omit zero-weight influences from the
/// joint weights list.
MAYAUSD_CORE_PUBLIC
int getCompressedSkinWeights(
    const MFnMesh&        mesh,
    const MFnSkinCluster& skinCluster,
    VtIntArray*           usdJointIndices,
    VtFloatArray*         usdJointWeights);

/// Check if a skinned primitive has an unsupported post-deformation
/// transformation. These transformations aren't represented in UsdSkel.
///
/// When a SkinCluster deforms meshes, the results are transformed back into the
/// space of the mesh. The output is then plugged up to the final mesh, which
/// has its own transform. Usually this change in transformation -- from putting
/// the deformation results back into the space of the source mesh, and then
/// transforming the result by the output mesh -- share the same transformation,
/// such that there's no overall change in transformation. This is not always
/// the case. In particular, 'broken' rigs may have the transformations out of
/// sync (the result of which being that the deformed meshes drift away from the
/// skeleton that drives them).
///
/// We have no nice way of encoding a mesh-specific post-deformation transform
/// in UsdSkel, and so can only try and warn the user.
MAYAUSD_CORE_PUBLIC
void warnForPostDeformationTransform(
    const SdfPath&        path,
    const MDagPath&       deformedMeshDag,
    const MFnSkinCluster& skinCluster);

/// Compute the geomBindTransform for a mesh using \p skinCluster.
MAYAUSD_CORE_PUBLIC
bool getGeomBindTransform(const MFnSkinCluster& skinCluster, GfMatrix4d* geomBindXf);

/// Gets the unique root joint of the given joint dag paths, or an invalid
/// MDagPath if there is no such unique joint (i.e. the joints form two
/// separate joint hierarchies). Currently, we don't support skin bound to
/// multiple joint hierarchies.
MAYAUSD_CORE_PUBLIC
MDagPath getRootJoint(const std::vector<MDagPath>& jointDagPaths);

/// Compute and write joint influences.
MAYAUSD_CORE_PUBLIC
bool writeJointInfluences(
    const MFnSkinCluster&    skinCluster,
    const MFnMesh&           inMesh,
    const UsdSkelBindingAPI& binding);

MAYAUSD_CORE_PUBLIC
bool writeJointOrder(
    const MDagPath&              rootJoint,
    const std::vector<MDagPath>& jointDagPaths,
    const UsdSkelBindingAPI&     binding,
    const bool                   stripNamespaces);

/// Writes skeleton skinning data for the mesh if it has skin clusters.
/// This method will internally determine, based on the job export args,
/// whether the prim has skinning data and whether it is eligible for
/// skinning data export.
/// If skinning data is successfully exported, then returns the pre-skin
/// mesh object. Otherwise, if no skeleton data was exported (whether there
/// was an error, or this mesh had no skinning, or this mesh was skipped),
/// returns a null MObject.
/// This should only be called once at the default time.
MAYAUSD_CORE_PUBLIC
MObject writeSkinningData(
    UsdGeomMesh&               primSchema,
    const SdfPath&             usdPath,
    const MDagPath&            dagPath,
    SdfPath&                   skelPath,
    const SdfPath&             parentScopePath,
    const bool                 stripNamespaces,
    UsdUtilsSparseValueWriter* valueWriter);
} // namespace UsdMayaJointUtil

PXR_NAMESPACE_CLOSE_SCOPE

#endif