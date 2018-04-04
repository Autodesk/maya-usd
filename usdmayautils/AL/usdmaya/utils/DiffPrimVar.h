//
// Copyright 2018 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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

#include "./Api.h"

#include "AL/usd/utils/DiffCore.h"
#include "maya/MFnMesh.h"
#include "maya/MDoubleArray.h"
#include "maya/MFloatArray.h"
#include "maya/MIntArray.h"
#include "maya/MUintArray.h"
#include "maya/MItMeshFaceVertex.h"
#include "maya/MGlobal.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usd/attribute.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a set of bit flags that identify which mesh/geometry components have changed
//----------------------------------------------------------------------------------------------------------------------
enum DiffComponents
{
  kPoints = 1 << 0, ///< the point position values have changed
  kNormals = 1 << 1, ///< the surface normals have changed
  kFaceVertexIndices = 1 << 2, ///< the face vertex indices have been modified
  kFaceVertexCounts = 1 << 3, ///< the number of vertices in the polygons have changed
  kNormalIndices = 1 << 4, ///< the normal indices have been modified
  kHoleIndices = 1 << 5, ///< the indices of the holes have changed
  kCreaseIndices = 1 << 6, ///< the edge crease indices have changed
  kCreaseWeights = 1 << 7, ///< the edge crease weights have changed
  kCreaseLengths = 1 << 8, ///< the edge crease lengths
  kCornerIndices = 1 << 9, ///< the vertex creases have changed
  kCornerSharpness = 1 << 10, ///< the vertex crease weights have changed
  kAllComponents = 0xFFFFFFFF
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  performs a diff between a point based usdgeom, and a maya mesh. This only checks the points
///         and normals of the mesh, and if the components differ, a bitmask is constructed and returned
///         indicating which components have changed
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
uint32_t diffGeom(UsdGeomPointBased& geom, MFnMesh& mesh, UsdTimeCode timeCode, uint32_t exportMask = kAllComponents);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
uint32_t diffFaceVertices(UsdGeomMesh& geom, MFnMesh& mesh, UsdTimeCode timeCode, uint32_t exportMask = kAllComponents);

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
  AL_USDMAYA_UTILS_PUBLIC
  PrimVarDiffEntry(const UsdGeomPrimvar& pv, const MString& setName, bool colourSet, bool indicesChanged, bool valuesChanged)
  : m_primVar(pv),
    m_setName(setName),
    m_flags((colourSet ? kIsColourSet : 0) |
            (indicesChanged ? kIndicesChanged : 0) |
            (valuesChanged ? kValuesChanged : 0)) {}

  /// \brief  returns the prim var we care about
  AL_USDMAYA_UTILS_PUBLIC
  const UsdGeomPrimvar& primVar()
    { return m_primVar; }

  /// \brief  returns the name of the UV (or colour) set in maya
  AL_USDMAYA_UTILS_PUBLIC
  const MString& setName() const
    { return m_setName; }

  /// \brief  returns true if this data should
  AL_USDMAYA_UTILS_PUBLIC
  bool isColourSet() const
    { return (m_flags & kIsColourSet) != 0; }

  /// \brief  returns true if this is a uv set
  AL_USDMAYA_UTILS_PUBLIC
  bool isUvSet() const
    { return !isColourSet(); }

  /// \brief  returns true if the set of indices has changed
  AL_USDMAYA_UTILS_PUBLIC
  bool indicesHaveChanged() const
    { return (m_flags & kIndicesChanged) != 0; }

  /// \brief  returns true if the UV or colour data has changed
  AL_USDMAYA_UTILS_PUBLIC
  bool dataHasChanged() const
    { return (m_flags & kValuesChanged) != 0; }

private:
  enum Flags
  {
    kIsColourSet = 1 << 0,
    kIndicesChanged = 1 << 1,
    kValuesChanged = 1 << 2
  };
  UsdGeomPrimvar m_primVar;
  MString m_setName;
  uint8_t m_flags;
};

typedef std::vector<PrimVarDiffEntry> PrimVarDiffReport;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares the colour sets on the usd prim v.s. the maya geometry. The function returns the array of colour
///         sets that have been added in maya, and a separate report that identifies any colour sets that have been
///         modified since being imported.
/// \param  geom the usd geometry
/// \param  mesh the maya geometry
/// \param  report the list of colour sets that have been modified
/// \return an array of the names of colour sets that have been added in maya
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
MStringArray hasNewColourSet(UsdGeomMesh& geom, MFnMesh& mesh, PrimVarDiffReport& report);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  compares the uv sets on the usd prim v.s. the maya geometry. The function returns the array of uv
///         sets that have been added in maya, and a separate report that identifies any uv sets that have been
///         modified since being imported.
/// \param  geom the usd geometry
/// \param  mesh the maya geometry
/// \param  report the list of uv sets that have been modified
/// \return an array of the names of uv sets that have been added in maya
//----------------------------------------------------------------------------------------------------------------------
AL_USDMAYA_UTILS_PUBLIC
MStringArray hasNewUvSet(UsdGeomMesh& geom, const MFnMesh& mesh, PrimVarDiffReport& report);

//----------------------------------------------------------------------------------------------------------------------
} // utils
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
