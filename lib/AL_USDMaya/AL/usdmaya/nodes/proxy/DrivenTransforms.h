//
// Copyright 2017 Animal Logic
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

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"

#include "maya/MPxData.h"
#include "maya/MVector.h"
#include "maya/MMatrix.h"

#include <vector>
#include <string>
#include "AL/maya/utils/ForwardDeclares.h"
#include "AL/usd/utils/ForwardDeclares.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {
namespace proxy {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This class maintains a set of prim paths to transform prims, and a cache of their matrix and visibility
///         states. It also maintains a set of indices that describe the dirty states of those attributes that have been
///         dirtied. If the number of transforms has changed, initTransforms should be called to initialise the internal
///         memory storage. setDrivenPrimPaths should be called to specify the prim paths. Whenever you need to specify
///         a change to the matrix or visibility values, call either dirtyVisibility or dirtyMatrix, and specify the
///         index of the prim to modify.
///         Within the compute method of the node, constructDrivenPrimsArray should be called to construct the array
///         of prims to update, which can then be passed to the update method to set the dirty values on the prim
///         attributes.
//----------------------------------------------------------------------------------------------------------------------
class DrivenTransforms
{
public:

  /// \brief  ctor
  inline DrivenTransforms()
    : m_drivenPrimPaths(), m_drivenMatrix(), m_drivenVisibility(), m_dirtyMatrices(), m_dirtyVisibilities() {}

  /// \brief  returns the number of transforms
  inline size_t transformCount() const
    { return m_drivenPrimPaths.size(); }

  /// \brief  resizes the driven transform internals to hold the specified number of prims
  /// \param  primPathCount the number of transforms to be driven
  void resizeDrivenTransforms(const size_t primPathCount);

  /// \brief  set the driven prim paths on the host driven transforms
  /// \param  primPaths the prim paths to set on the proxy
  inline void setDrivenPrimPaths(const SdfPathVector& primPaths)
    { m_drivenPrimPaths = primPaths; }

  /// \brief  update the driven transforms
  /// \param  stage the stage to extract the prims from
  /// \param  currentTime the current time
  ///
  bool update(UsdStageRefPtr stage, const MTime& currentTime);

  /// \brief  dirties the visibility for the specified prim index
  /// \param  primIndex the index of the prim
  /// \param  newValue the new visibility value
  inline void dirtyVisibility(const int32_t primIndex, bool newValue)
    {
      m_drivenVisibility[primIndex] = newValue;
      m_dirtyVisibilities.push_back(primIndex);
    }

  /// \brief  dirties the matrix for the specified prim index
  /// \param  primIndex the index of the prim
  /// \param  newValue the new matrix value
  inline void dirtyMatrix(const int32_t primIndex, const MMatrix& newValue)
    {
      m_drivenMatrix[primIndex] = newValue;
      m_dirtyMatrices.push_back(primIndex);
    }

  /// \brief  returns the paths of the driven transforms
  /// \return the driven transforms
  inline const SdfPathVector& drivenPrimPaths() const
    { return m_drivenPrimPaths; }

  /// \brief  returns the matrices that have been dirtied
  /// \return returns the indices of the prims that have dirtied matrix params
  inline const std::vector<int32_t>& dirtyMatrices() const
    { return m_dirtyMatrices; }

  /// \brief  returns the visibilities that have been dirtied
  /// \return returns the indices of the prims that have dirtied visibility params
  inline const std::vector<int32_t>& dirtyVisibilities() const
    { return m_dirtyVisibilities; }

  /// \brief  returns the visibilities that have been dirtied
  /// \return returns the current matrix values of the driven transforms
  inline const std::vector<MMatrix>& drivenMatrices() const
    { return m_drivenMatrix; }

  /// \brief  returns the visibilities that have been dirtied
  /// \return returns the current visibility statuses of the driven transforms
  inline const std::vector<bool>& drivenVisibilities() const
    { return m_drivenVisibility; }

private:
  void updateDrivenVisibility(std::vector<UsdPrim>& drivenPrims, const MTime& currentTime);
  void updateDrivenTransforms(std::vector<UsdPrim>& drivenPrims, const MTime& currentTime);
private:
  SdfPathVector m_drivenPrimPaths;
  std::vector<MMatrix> m_drivenMatrix;
  std::vector<bool> m_drivenVisibility;
  std::vector<int32_t> m_dirtyMatrices;
  std::vector<int32_t> m_dirtyVisibilities;
};

//----------------------------------------------------------------------------------------------------------------------
} // proxy
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

