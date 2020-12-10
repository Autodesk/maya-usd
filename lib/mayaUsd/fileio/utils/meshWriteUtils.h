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
#ifndef PXRUSDMAYA_MESH_WRITE_UTILS_H
#define PXRUSDMAYA_MESH_WRITE_UTILS_H

#include <mayaUsd/base/api.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdUtils/sparseValueWriter.h>

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MObject.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomMesh;

// Utilities for dealing with writing USD from Maya mesh/subdiv tags.
namespace UsdMayaMeshWriteUtils {
/**
 * Finds a skinCluster directly connected upstream in the DG to the given mesh.
 *
 * @param mesh              The mesh to search from.
 * @param skinCluster       Storage for the result. If no skinCluster can be found,
 *                          this will be a null `MObject`.
 *
 * @return                  `MStatus::kSuccess` if the operation completed successfully.
 */
MAYAUSD_CORE_PUBLIC
MStatus getSkinClusterConnectedToMesh(const MObject& mesh, MObject& skinCluster);

/**
 * Similar to `getSkinClusterConnectedToMesh`, except that instead of finding a
 * directly-connected skinCluster, it searches the DG upstream of the mesh for any
 * other skinClusters as well.
 *
 * @param mesh              The mesh to start the search from.
 * @param skinClusters      Storage for the result.
 *
 * @return                  `MStatus::kSuccess` if the operation completed successfully.
 */
MAYAUSD_CORE_PUBLIC
MStatus getSkinClustersUpstreamOfMesh(const MObject& mesh, MObjectArray& skinClusters);

/**
 * Calculates the union bounding box of a given array of meshes.
 *
 * @param meshes    The meshes to calculate the union bounding box of.
 *
 * @return          The union bounding box.
 */
MAYAUSD_CORE_PUBLIC
MBoundingBox calcBBoxOfMeshes(const MObjectArray& meshes);

/// Helper method for getting Maya mesh normals as a VtVec3fArray.
MAYAUSD_CORE_PUBLIC
bool getMeshNormals(const MFnMesh& mesh, VtVec3fArray* normalsArray, TfToken* interpolation);

/// Gets the subdivision scheme tagged for the Maya mesh by consulting the
/// adaptor for \c UsdGeomMesh.subdivisionSurface, and then falling back to
/// the RenderMan for Maya attribute.
MAYAUSD_CORE_PUBLIC
TfToken getSubdivScheme(const MFnMesh& mesh);

/// Gets the subdivision interpolate boundary tagged for the Maya mesh by
/// consulting the adaptor for \c UsdGeomMesh.interpolateBoundary, and then
/// falling back to the RenderMan for Maya attribute.
MAYAUSD_CORE_PUBLIC
TfToken getSubdivInterpBoundary(const MFnMesh& mesh);

/// Gets the subdivision face-varying linear interpolation tagged for the
/// Maya mesh by consulting the adaptor for
/// \c UsdGeomMesh.faceVaryingLinearInterpolation, and then falling back to
/// the OpenSubdiv2-style tagging.
MAYAUSD_CORE_PUBLIC
TfToken getSubdivFVLinearInterpolation(const MFnMesh& mesh);

MAYAUSD_CORE_PUBLIC
bool isMeshValid(const MDagPath& dagPath);

MAYAUSD_CORE_PUBLIC
void exportReferenceMesh(UsdGeomMesh& primSchema, MObject obj);

MAYAUSD_CORE_PUBLIC
void assignSubDivTagsToUSDPrim(
    MFnMesh&                   meshFn,
    UsdGeomMesh&               primSchema,
    UsdUtilsSparseValueWriter* valueWriter);

void assignSubDivTagsToUSDPrim(
    MFnMesh&                   meshFn,
    UsdGeomMesh&               primSchema,
    UsdUtilsSparseValueWriter* valueWriter);

MAYAUSD_CORE_PUBLIC
void writePointsData(
    const MFnMesh&             meshFn,
    UsdGeomMesh&               primSchema,
    const UsdTimeCode&         usdTime,
    UsdUtilsSparseValueWriter* valueWriter);

MAYAUSD_CORE_PUBLIC
void writeFaceVertexIndicesData(
    const MFnMesh&             meshFn,
    UsdGeomMesh&               primSchema,
    const UsdTimeCode&         usdTime,
    UsdUtilsSparseValueWriter* valueWriter);

MAYAUSD_CORE_PUBLIC
void writeInvisibleFacesData(
    const MFnMesh&             meshFn,
    UsdGeomMesh&               primSchema,
    UsdUtilsSparseValueWriter* valueWriter);

MAYAUSD_CORE_PUBLIC
bool getMeshUVSetData(
    const MFnMesh& mesh,
    const MString& uvSetName,
    VtVec2fArray*  uvArray,
    TfToken*       interpolation,
    VtIntArray*    assignmentIndices);

MAYAUSD_CORE_PUBLIC
bool writeUVSetsAsVec2fPrimvars(
    const MFnMesh&             meshFn,
    UsdGeomMesh&               primSchema,
    const UsdTimeCode&         usdTime,
    UsdUtilsSparseValueWriter* valueWriter);

MAYAUSD_CORE_PUBLIC
void writeSubdivInterpBound(
    MFnMesh&                   mesh,
    UsdGeomMesh&               primSchema,
    UsdUtilsSparseValueWriter* valueWriter);

MAYAUSD_CORE_PUBLIC
void writeSubdivFVLinearInterpolation(
    MFnMesh&                   meshFn,
    UsdGeomMesh&               primSchema,
    UsdUtilsSparseValueWriter* valueWriter);

MAYAUSD_CORE_PUBLIC
void writeNormalsData(
    const MFnMesh&             meshFn,
    UsdGeomMesh&               primSchema,
    const UsdTimeCode&         usdTime,
    UsdUtilsSparseValueWriter* valueWriter);

MAYAUSD_CORE_PUBLIC
bool addDisplayPrimvars(
    UsdGeomGprim&                       primSchema,
    const UsdTimeCode&                  usdTime,
    const MFnMesh::MColorRepresentation colorRep,
    const VtVec3fArray&                 RGBData,
    const VtFloatArray&                 AlphaData,
    const TfToken&                      interpolation,
    const VtIntArray&                   assignmentIndices,
    const bool                          clamped,
    const bool                          authored,
    UsdUtilsSparseValueWriter*          valueWriter);

MAYAUSD_CORE_PUBLIC
bool createRGBPrimVar(
    UsdGeomGprim&              primSchema,
    const TfToken&             name,
    const UsdTimeCode&         usdTime,
    const VtVec3fArray&        data,
    const TfToken&             interpolation,
    const VtIntArray&          assignmentIndices,
    bool                       clamped,
    UsdUtilsSparseValueWriter* valueWriter);

MAYAUSD_CORE_PUBLIC
bool createRGBAPrimVar(
    UsdGeomGprim&              primSchema,
    const TfToken&             name,
    const UsdTimeCode&         usdTime,
    const VtVec3fArray&        rgbData,
    const VtFloatArray&        alphaData,
    const TfToken&             interpolation,
    const VtIntArray&          assignmentIndices,
    bool                       clamped,
    UsdUtilsSparseValueWriter* valueWriter);

MAYAUSD_CORE_PUBLIC
bool createAlphaPrimVar(
    UsdGeomGprim&              primSchema,
    const TfToken&             name,
    const UsdTimeCode&         usdTime,
    const VtFloatArray&        data,
    const TfToken&             interpolation,
    const VtIntArray&          assignmentIndices,
    bool                       clamped,
    UsdUtilsSparseValueWriter* valueWriter);

/// Collect values from the color set named \p colorSet.
/// If \p isDisplayColor is true and this color set represents displayColor,
/// the unauthored/unpainted values in the color set will be filled in using
/// the shader values in \p shadersRGBData and \p shadersAlphaData if available.
/// Values are gathered per face vertex, but then the data is compressed to
/// vertex, uniform, or constant interpolation if possible.
/// Unauthored/unpainted values will be given the index -1.
MAYAUSD_CORE_PUBLIC
bool getMeshColorSetData(
    MFnMesh&                       mesh,
    const MString&                 colorSet,
    bool                           isDisplayColor,
    const VtVec3fArray&            shadersRGBData,
    const VtFloatArray&            shadersAlphaData,
    const VtIntArray&              shadersAssignmentIndices,
    VtVec3fArray*                  colorSetRGBData,
    VtFloatArray*                  colorSetAlphaData,
    TfToken*                       interpolation,
    VtIntArray*                    colorSetAssignmentIndices,
    MFnMesh::MColorRepresentation* colorSetRep,
    bool*                          clamped);

} // namespace UsdMayaMeshWriteUtils

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_MESH_WRITE_UTILS_H
