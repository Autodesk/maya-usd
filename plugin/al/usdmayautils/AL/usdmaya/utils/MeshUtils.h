//
// Copyright 2017 Animal Logic
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

#pragma once

#include "AL/maya/utils/MayaHelperMacros.h"
#include "AL/usdmaya/utils/Api.h"

#include <pxr/usd/usdGeom/mesh.h>

#include <maya/MFloatPointArray.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
#include <maya/MItMeshVertex.h>
#include <maya/MPlug.h>
#include <maya/MUintArray.h>

PXR_NAMESPACE_USING_DIRECTIVE

constexpr auto _alusd_colour = "alusd_colour_";

namespace AL {
namespace usdmaya {
namespace utils {

/// \brief  a conversion utility that takes an array of floating point data, and converts it into
/// double precision data \param  output the double precision output \param  input the input
/// floating point data \param  count the number of elements in the array \note   the sizes of the
/// output and input arrays must match the count
AL_USDMAYA_UTILS_PUBLIC
void floatToDouble(double* output, const float* const input, size_t count);

/// \brief  a conversion utility that takes an array of double precision point data, and converts it
/// into floating point data \param  output the input floating point otuput array \param  input the
/// double precision input \param  count the number of elements in the array \note   the sizes of
/// the output and input arrays must match the count
AL_USDMAYA_UTILS_PUBLIC
void doubleToFloat(float* output, const double* const input, size_t count);

/// \brief  converts a an input array of 3D floating point values, and converts them to 4D
/// (inserting 1.0 as the 4th component of
///         each array element)
/// \param  input the input array of 3D values
/// \param  output the output array of 4D values
/// \param  count the number of vector values in each array
/// \note   the sizes of the output and input arrays must match the count
AL_USDMAYA_UTILS_PUBLIC
void convert3DArrayTo4DArray(const float* const input, float* const output, size_t count);

/// \brief  converts a an input array of 3D floating point values, and converts them to 4D
/// (inserting 1.0 as the 4th component of
///         each array element)
/// \param  input the input array of 3D values
/// \param  output the output array of 4D values (double precision)
/// \param  count the number of vector values in each array
/// \note   the sizes of the output and input arrays must match the count
AL_USDMAYA_UTILS_PUBLIC
void convertFloatVec3ArrayToDoubleVec3Array(
    const float* const input,
    double* const      output,
    size_t             count);

/// \brief  This method generates a set of incrementing integer values from 0 to (count-1). So for
/// example, if the count is 7,
///         the indices output will contain the values (0, 1, 2, 3, 4, 5, 6)
/// \param  indices the array of integer values to fill
/// \param  count the number of indices to generate
AL_USDMAYA_UTILS_PUBLIC
void generateIncrementingIndices(MIntArray& indices, const size_t count);

/// \brief  This method takes an array of packed UV values, and seperates them into two arrays of U
/// and V values. \param  uv the input UV array \param  u the output array of u values \param  v the
/// output array of v values \param  count the number of elements in each of the three arrays \note
/// the u, v, and uv arrays must match the size specified by the count parameter
AL_USDMAYA_UTILS_PUBLIC
void unzipUVs(const float* const uv, float* const u, float* const v, const size_t count);

/// \brief  This method takes two arrays of u an v values, and interleaves them into a single array
/// of packed uv values \param  u the input array of u values \param  v the input array of v values
/// \param  uv the output UV array
/// \param  count the number of elements in each of the three arrays
/// \note   the u, v, and uv arrays must match the size specified by the count parameter
AL_USDMAYA_UTILS_PUBLIC
void zipUVs(const float* u, const float* v, float* uv, const size_t count);

/// \brief  this method checks the indices of a uv set, looking for any indices of -1 (which is
/// assumed to mean the uv set is sparse) \param  indices the array of uv indices \param  count the
/// number of indices in the indices array \return true if the uv set is sparse, false if a uv value
/// exists for each vertex-face
AL_USDMAYA_UTILS_PUBLIC
bool isUvSetDataSparse(const int32_t* indices, const uint32_t count);

/// \brief  given a set of UV indices, this method will extract all of the uv values from the u and
/// v arrays, and will interleave
///         them into an array of flattened uv values into the output array.
/// \param  output the array of output uv values. This array should contain (numIndices * 2)
/// floating point values. \param  u the input u values \param  u the input v values \param  indices
/// the uv indices (which index into the u and v array) \param  numIndices the number of indices in
/// the indices array
AL_USDMAYA_UTILS_PUBLIC
void interleaveIndexedUvData(
    float*         output,
    const float*   u,
    const float*   v,
    const int32_t* indices,
    const uint32_t numIndices);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class used to import mesh data from Usd into Maya
//----------------------------------------------------------------------------------------------------------------------
struct MeshImportContext
{
private:
    MFnMesh            fnMesh;     ///< the maya function set to use when setting the data
    MFloatPointArray   points;     ///< the array of vertices for the mesh being imported
    MVectorArray       normals;    ///< the array of normal vectors for the mesh being imported
    MIntArray          counts;     ///< the number of vertices in each face within the mesh
    MIntArray          connects;   ///< the vertex indices for each face-vertex in the mesh
    const UsdGeomMesh& mesh;       ///< the USD geometry being imported
    MObject            polyShape;  ///< the handle to the created mesh shape
    UsdTimeCode        m_timeCode; ///< the time at which to import the mesh
    AL_USDMAYA_UTILS_PUBLIC
    void gatherFaceConnectsAndVertices();

public:
    /// \brief  constructs the import context for the specified mesh
    /// \param  mesh the usd geometry to import
    /// \param  parentOrOwner the maya transform that will be the parent transform of the geometry
    /// being imported,
    ///         or a mesh data objected created via MFnMeshData.
    /// \param  dagName the name for the new mesh node
    /// \param  timeCode the time code at which to gather the data from USD
    MeshImportContext(
        const UsdGeomMesh& mesh,
        MObject            parentOrOwner,
        MString            dagName,
        UsdTimeCode        timeCode = UsdTimeCode::EarliestTime())
        : mesh(mesh)
        , m_timeCode(timeCode)
    {
        gatherFaceConnectsAndVertices();
        polyShape = fnMesh.create(
            points.length(), counts.length(), points, counts, connects, parentOrOwner);
        TfToken orientation;
        bool    leftHanded
            = (mesh.GetOrientationAttr().Get(&orientation, timeCode)
               && orientation == UsdGeomTokens->leftHanded);
        fnMesh.findPlug("op", true).setBool(leftHanded);
        //
        if (parentOrOwner.hasFn(MFn::kTransform)) {
            fnMesh.setName(dagName);
        }
    }

    /// \brief  reads the HoleIndices attribute from the usd geometry, and assigns those values as
    /// invisible faces on
    ///         the Maya mesh
    AL_USDMAYA_UTILS_PUBLIC
    void applyHoleFaces();

    /// \brief  assigns the vertex normals on the mesh (if they exist).
    AL_USDMAYA_UTILS_PUBLIC
    bool applyVertexNormals();

    /// \brief  assigns the edge creases on the maya geometry
    AL_USDMAYA_UTILS_PUBLIC
    bool applyEdgeCreases();

    /// \brief  assigns the vertex creates on the maya geometry
    AL_USDMAYA_UTILS_PUBLIC
    bool applyVertexCreases();

    /// \brief  creates all of the UV sets on the Maya geometry
    AL_USDMAYA_UTILS_PUBLIC
    void applyUVs();

    /// \brief  creates all of the colour sets on the Maya geometry
    AL_USDMAYA_UTILS_PUBLIC
    void applyColourSetData();

    /// \brief  returns the poly shape being imported.
    MObject getPolyShape() const { return polyShape; }

    /// \brief  returns the mesh function set
    MFnMesh& getFn() { return fnMesh; }
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class used to export mesh data from Maya into a USD prim
//----------------------------------------------------------------------------------------------------------------------
struct MeshExportContext
{
public:
    enum CompactionLevel
    {
        kNone,
        kBasic,
        kMedium,
        kFull
    };

    /// \brief  constructor
    /// \param  path the DAG path of the Maya mesh being exported
    /// \param  mesh an intialised USD prim into which the data should be copied
    /// \param  timeCode the time where the mesh data should be written
    /// \param  performDiff if true, perform a diff check to ensure only data that has changed gets
    /// written into USD \param  compactionLevel the amount of processing we want to perform when
    /// computing interpolation modes
    AL_USDMAYA_UTILS_PUBLIC
    MeshExportContext(
        MDagPath        path,
        UsdGeomMesh&    mesh,
        UsdTimeCode     timeCode,
        bool            performDiff = false,
        CompactionLevel compactionLevel = kFull,
        bool            reverseNormals = false);

    /// \brief  returns true if it's ok to continue exporting the data
    operator bool() const { return valid; }

    /// \brief  copies the edge crease information from maya onto the UsdPrim
    AL_USDMAYA_UTILS_PUBLIC
    void copyCreaseEdges();

    /// \brief  copies the vertex data from maya into the usd prim.
    /// \param  timeCode the time code at which to extract the samples
    AL_USDMAYA_UTILS_PUBLIC
    void copyVertexData(UsdTimeCode timeCode);

    /// \brief  computes the maya geometry extent and writes to usd prim.
    /// \param  timeCode the time code at which to extract the samples
    AL_USDMAYA_UTILS_PUBLIC
    void copyExtentData(UsdTimeCode timeCode);

    /// \brief  copies the normal data from maya into the usd prim.
    /// \param  timeCode the time code at which to extract the samples
    /// \param  writeAsPrimvars if true the normals will be written as a primvar, if false it will
    /// be written into
    ////        the normals atttribute. primvars support indexed normals, the normals attr does not.
    AL_USDMAYA_UTILS_PUBLIC
    void copyNormalData(UsdTimeCode timeCode, bool writeAsPrimvars = false);

    /// \brief  copies the vertex crease information from maya into the usd prim.
    AL_USDMAYA_UTILS_PUBLIC
    void copyCreaseVertices();

    /// \brief  copies the face connects and counts information from maya into the usd prim.
    AL_USDMAYA_UTILS_PUBLIC
    void copyFaceConnectsAndPolyCounts();

    /// \brief  copies the UV set data from maya into the usd prim
    AL_USDMAYA_UTILS_PUBLIC
    void copyUvSetData();

    /// \brief  copies the Points set data from maya into the usd prim as "pref"
    AL_USDMAYA_UTILS_PUBLIC
    void copyBindPoseData(UsdTimeCode time);

    /// \brief  copies the colour set data from maya into the usd prim.
    AL_USDMAYA_UTILS_PUBLIC
    void copyColourSetData();

    /// \brief  copies invisible face information into the usd file from maya
    AL_USDMAYA_UTILS_PUBLIC
    void copyInvisibleHoles();

    /// \brief  deprecated, will be removed in a later release
    AL_USDMAYA_UTILS_PUBLIC
    void copyAnimalFaceColours();

    /// \brief  returns the mesh function set
    MFnMesh& getFn() { return fnMesh; }

    /// \brief  returns the time code
    UsdTimeCode timeCode() const { return m_timeCode; }

private:
    MFnMesh         fnMesh;       ///< the maya function set
    MIntArray       faceCounts;   ///< the number of verts in each face
    MIntArray       faceConnects; ///< the face-vertex indices
    UsdTimeCode     m_timeCode;   ///< the time at which to extract the mesh data
    UsdGeomMesh&    mesh;         ///< the usd geometry
    uint32_t        diffGeom;     ///< the bit flags for standard geom params
    uint32_t        diffMesh;     ///< the bit flags for mesh params
    CompactionLevel compaction;
    bool            valid;          ///< true if the function set is ok
    bool            performDiff;    ///< true if performing a diff on export
    bool            reverseNormals; ///< true if reversing normals on 'opposite' meshes
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
