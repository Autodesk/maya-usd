//
// Copyright 2018 Animal Logic
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

#include "AL/usdmaya/utils/Api.h"

#include <mayaUsdUtils/ForwardDeclares.h>

#include <pxr/usd/usdGeom/mesh.h>

#include <maya/MFnMesh.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a set of bit flags that identify which mesh/geometry components have changed
//----------------------------------------------------------------------------------------------------------------------
enum DiffComponents
{
    kPoints = 1 << 0,            ///< the point position values have changed
    kNormals = 1 << 1,           ///< the surface normals have changed
    kFaceVertexIndices = 1 << 2, ///< the face vertex indices have been modified
    kFaceVertexCounts = 1 << 3,  ///< the number of vertices in the polygons have changed
    kNormalIndices = 1 << 4,     ///< the normal indices have been modified
    kHoleIndices = 1 << 5,       ///< the indices of the holes have changed
    kCreaseIndices = 1 << 6,     ///< the edge crease indices have changed
    kCreaseWeights = 1 << 7,     ///< the edge crease weights have changed
    kCreaseLengths = 1 << 8,     ///< the edge crease lengths
    kCornerIndices = 1 << 9,     ///< the vertex creases have changed
    kCornerSharpness = 1 << 10,  ///< the vertex crease weights have changed
    kExtent = 1 << 11,           ///< the point extents have changed
    kAllComponents = 0xFFFFFFFF
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  performs a diff between a point based usdgeom, and a maya mesh. This only checks the
/// points
///         and normals of the mesh, and if the components differ, a bitmask is constructed and
///         returned indicating which components have changed
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
uint32_t diffGeom(
    UsdGeomPointBased& geom,
    MFnMesh&           mesh,
    UsdTimeCode        timeCode,
    uint32_t           exportMask = kAllComponents);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
uint32_t diffFaceVertices(
    UsdGeomMesh& geom,
    MFnMesh&     mesh,
    UsdTimeCode  timeCode,
    uint32_t     exportMask = kAllComponents);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
class PrimVarDiffEntry
{
public:
    /// \brief  ctor
    /// \param  pv the prim var to store a reference to
    /// \param  setName the name of the uv (or colour) set to extract from maya
    /// \param  colourSet true if we should be extracting a colour set
    /// \param  indicesChanged true if the indices on the colour set have changed
    /// \param  valuesChanged true if the values on the colour set have changed
    /// \param  interpolation the interpolation mode
    PrimVarDiffEntry(
        const UsdGeomPrimvar& pv,
        const MString&        setName,
        bool                  colourSet,
        bool                  indicesChanged,
        bool                  valuesChanged,
        TfToken               interpolation)
        : m_primVar(pv)
        , m_setName(setName)
        , m_indicesToExtract()
        , m_flags(
              (colourSet ? kIsColourSet : 0) | (indicesChanged ? kIndicesChanged : 0)
              | (valuesChanged ? kValuesChanged : 0))
    {
        if (interpolation == UsdGeomTokens->constant)
            m_flags |= kConstant;
        else if (interpolation == UsdGeomTokens->vertex)
            m_flags |= kVertex;
        else if (interpolation == UsdGeomTokens->uniform)
            m_flags |= kUniform;
        else {
            m_flags |= kFaceVarying;
        }
    }

    /// \brief  ctor
    /// \param  pv the prim var to store a reference to
    /// \param  setName the name of the uv (or colour) set to extract from maya
    /// \param  colourSet true if we should be extracting a colour set
    /// \param  indicesChanged true if the indices on the colour set have changed
    /// \param  valuesChanged true if the values on the colour set have changed
    /// \param  interpolation the interpolation mode
    /// \param  elements a returned array of indices used to construct a new output array
    PrimVarDiffEntry(
        const UsdGeomPrimvar&   pv,
        const MString&          setName,
        bool                    colourSet,
        bool                    indicesChanged,
        bool                    valuesChanged,
        TfToken                 interpolation,
        std::vector<uint32_t>&& elements)
        : m_primVar(pv)
        , m_setName(setName)
        , m_indicesToExtract(elements)
        , m_flags(
              (colourSet ? kIsColourSet : 0) | (indicesChanged ? kIndicesChanged : 0)
              | (valuesChanged ? kValuesChanged : 0))
    {
        if (interpolation == UsdGeomTokens->constant)
            m_flags |= kConstant;
        else if (interpolation == UsdGeomTokens->vertex)
            m_flags |= kVertex;
        else if (interpolation == UsdGeomTokens->uniform)
            m_flags |= kUniform;
        else {
            m_flags |= kFaceVarying;
        }
    }

    /// \brief  ctor
    /// \param  pv the prim var to store a reference to
    /// \param  setName the name of the uv (or colour) set to extract from maya
    /// \param  colourSet true if we should be extracting a colour set
    /// \param  indicesChanged true if the indices on the colour set have changed
    /// \param  valuesChanged true if the values on the colour set have changed
    PrimVarDiffEntry(
        const UsdGeomPrimvar& pv,
        const MString&        setName,
        bool                  colourSet,
        bool                  indicesChanged,
        bool                  valuesChanged)
        : m_primVar(pv)
        , m_setName(setName)
        , m_indicesToExtract()
        , m_flags(
              (colourSet ? kIsColourSet : 0) | (indicesChanged ? kIndicesChanged : 0)
              | (valuesChanged ? kValuesChanged : 0))
    {
        m_flags |= kFaceVarying;
    }

    /// \brief  returns the prim var we care about
    UsdGeomPrimvar& primVar() { return m_primVar; }

    /// \brief  returns the prim var we care about
    const UsdGeomPrimvar& primVar() const { return m_primVar; }

    /// \brief  returns the name of the UV (or colour) set in maya
    const MString& setName() const { return m_setName; }

    /// \brief  returns true if this data should
    bool isColourSet() const { return (m_flags & kIsColourSet) != 0; }

    /// \brief  returns true if this is a uv set
    bool isUvSet() const { return !isColourSet(); }

    /// \brief  returns true if the set of indices has changed
    bool indicesHaveChanged() const { return (m_flags & kIndicesChanged) != 0; }

    /// \brief  returns true if the UV or colour data has changed
    bool dataHasChanged() const { return (m_flags & kValuesChanged) != 0; }

    /// \brief  returns true if the interpolation mode is constant
    bool constantInterpolation() const { return (m_flags & kConstant) != 0; }

    /// \brief  returns true if the interpolation mode is uniform
    bool uniformInterpolation() const { return (m_flags & kUniform) != 0; }

    /// \brief  returns true if the interpolation mode is per vertex
    bool vertexInterpolation() const { return (m_flags & kVertex) != 0; }

    /// \brief  returns true if the interpolation mode is face varying
    bool faceVaryingInterpolation() const { return (m_flags & kFaceVarying) != 0; }

    /// \brief  returns the indices of the elements to extract to construct the final exported array
    std::vector<uint32_t>& indicesToExtract() { return m_indicesToExtract; }

    /// \brief  returns the indices of the elements to extract to construct the final exported array
    const std::vector<uint32_t>& indicesToExtract() const { return m_indicesToExtract; }

private:
    enum Flags
    {
        kIsColourSet = 1 << 0,
        kIndicesChanged = 1 << 1,
        kValuesChanged = 1 << 2,

        kConstant = 1 << 28,
        kUniform = 1 << 29,
        kVertex = 1 << 30,
        kFaceVarying = 1 << 31
    };
    UsdGeomPrimvar        m_primVar;
    MString               m_setName;
    std::vector<uint32_t> m_indicesToExtract;
    uint32_t              m_flags;
};

typedef std::vector<PrimVarDiffEntry> PrimVarDiffReport;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares the colour sets on the usd prim v.s. the maya geometry. The function returns
/// the array of colour
///         sets that have been added in maya, and a separate report that identifies any colour sets
///         that have been modified since being imported.
/// \param  geom the usd geometry
/// \param  mesh the maya geometry
/// \param  report the list of colour sets that have been modified
/// \return an array of the names of colour sets that have been added in maya
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
MStringArray hasNewColourSet(UsdGeomMesh& geom, MFnMesh& mesh, PrimVarDiffReport& report);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares the uv sets on the usd prim v.s. the maya geometry. The function returns the
/// array of uv
///         sets that have been added in maya, and a separate report that identifies any uv sets
///         that have been modified since being imported.
/// \param  geom the usd geometry
/// \param  mesh the maya geometry
/// \param  report the list of uv sets that have been modified
/// \return an array of the names of uv sets that have been added in maya
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
MStringArray hasNewUvSet(UsdGeomMesh& geom, const MFnMesh& mesh, PrimVarDiffReport& report);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A fast method for quickly determining the Interpolation type. Determines if the
/// interpolation type is
///         constant, vertex, or faceVarying. This is mostly done by testing index values (instead
///         of accounting for duplicates).
/// \param  u the U components
/// \param  v the V components
/// \param  indices the UV indices
/// \param  pointIndices the vertex indices
/// \return UsdGeomTokens->constant, UsdGeomTokens->vertex, or UsdGeomTokens->faceVarying
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
TfToken guessUVInterpolationType(
    MFloatArray& u,
    MFloatArray& v,
    MIntArray&   indices,
    MIntArray&   pointIndices);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  In addition to the interpolation checks performed by guessUVInterpolationTypeUV, this
/// method also looks for
///         UV per-face assignments (uniform).
/// \param  u the U components
/// \param  v the V components
/// \param  indices the UV indices
/// \param  pointIndices the vertex indices
/// \param  faceCounts the face counts array for the mesh
/// \return UsdGeomTokens->constant, UsdGeomTokens->vertex, UsdGeomTokens->uniform, or
/// UsdGeomTokens->faceVarying
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
TfToken guessUVInterpolationTypeExtended(
    MFloatArray& u,
    MFloatArray& v,
    MIntArray&   indices,
    MIntArray&   pointIndices,
    MIntArray&   faceCounts);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This test performs the same function as guessUVInterpolationTypeExtendedUV, however the
/// checks it performs
///         are the against the actual UV data (so it accounts for duplicated UV values which may
///         not have the same index)
/// \param  u the U components
/// \param  v the V components
/// \param  indices the UV indices
/// \param  pointIndices the vertex indices
/// \param  faceCounts the face counts array for the mesh
/// \return UsdGeomTokens->constant, UsdGeomTokens->vertex, UsdGeomTokens->uniform, or
/// UsdGeomTokens->faceVarying
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
TfToken guessUVInterpolationTypeExtensive(
    MFloatArray&           u,
    MFloatArray&           v,
    MIntArray&             indices,
    MIntArray&             pointIndices,
    MIntArray&             faceCounts,
    std::vector<uint32_t>& indicesToExtract);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  performs a basic set of tests to determine the interpolation mode of the rgba prim var
/// data. \param  rgba the face varying colour array \param  numElements the number of RGBA colours
/// in the rgba array \return UsdGeomTokens->constant, or UsdGeomTokens->faceVarying
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
TfToken guessColourSetInterpolationType(const float* rgba, const size_t numElements);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  performs a basic set of tests to determine the interpolation mode of the rgba prim var
/// data. \param  rgba the face varying colour array \param  numElements the number of RGBA colours
/// in the rgba array \param  threshold the threshold value for RGBA colours \return
/// UsdGeomTokens->constant, UsdGeomTokens->uniform, or UsdGeomTokens->faceVarying
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
TfToken
guessColourSetInterpolationType(const float* rgba, const size_t numElements, float threshold);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  performs a more comprehensive set of tests to determine the interpolation mode for the
/// rgba prim var data. \param  rgba the face varying colour array \param  numElements the number of
/// RGBA colours in the rgba array \param  numPoints the number of points \param  pointIndices the
/// point indices in the mesh \param  faceCounts the face counts for the mesh \param
/// indicesToExtract the resulting set of indices which will be later used to extract the correct
/// data \return UsdGeomTokens->constant, UsdGeomTokens->vertex, UsdGeomTokens->uniform, or
/// UsdGeomTokens->faceVarying
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
TfToken guessColourSetInterpolationTypeExtensive(
    const float*           rgba,
    const size_t           numElements,
    const size_t           numPoints,
    MIntArray&             pointIndices,
    MIntArray&             faceCounts,
    std::vector<uint32_t>& indicesToExtract);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  performs a more comprehensive set of tests to determine the interpolation mode for the
/// rgba prim var data. \param  rgba the face varying colour array \param  numElements the number of
/// RGBA colours in the rgba array \param  threshold the threshold value for RGBA colours \param
/// numPoints the number of points \param  pointIndices the point indices in the mesh \param
/// faceCounts the face counts for the mesh \param  indicesToExtract the resulting set of indices
/// which will be later used to extract the correct data \return UsdGeomTokens->constant,
/// UsdGeomTokens->vertex, UsdGeomTokens->uniform, or UsdGeomTokens->faceVarying
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
TfToken guessColourSetInterpolationTypeExtensive(
    const float*           rgba,
    const size_t           numElements,
    float                  threshold,
    const size_t           numPoints,
    MIntArray&             pointIndices,
    MIntArray&             faceCounts,
    std::vector<uint32_t>& indicesToExtract);

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
