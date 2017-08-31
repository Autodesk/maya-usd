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

#include "AL/usdmaya/Common.h"
#include "maya/MPxData.h"
#include "maya/MVector.h"
#include "maya/MMatrix.h"

#include <vector>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {
namespace proxy {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class that encapsulates the driven transforms data within a proxy shape
//----------------------------------------------------------------------------------------------------------------------
class DrivenTransforms
{
public:

  /// \brief  ctor
  DrivenTransforms()
    : m_drivenPrimPaths(), m_drivenMatrix(), m_drivenVisibility(), m_dirtyMatrices(), m_dirtyVisibilities() {}

  /// \brief  returns the number of transforms
  inline size_t transformCount() const
    { return m_drivenPrimPaths.size(); }

  /// \brief  initialise the transform at the specified index
  /// \param  primPathCount the number of transforms to be driven
  void initTransforms(const size_t primPathCount);

  /// \brief  update the driven prim paths
  /// \param  drivenPaths the returned array of driven paths that were updated
  /// \param  drivenPrims the returned array of driven prims that were updated
  /// \param  stage the usd stage that contains the prims
  bool constructDrivenPrimsArray(SdfPathVector& drivenPaths, std::vector<UsdPrim>& drivenPrims, UsdStageRefPtr stage);

  /// \brief  update the driven prim visibility
  /// \param  drivenPrims the driven prims to update
  /// \param  currentTime the current time
  void updateDrivenVisibility(std::vector<UsdPrim>& drivenPrims, const MTime& currentTime);

  /// \brief  update the driven prim transforms
  /// \param  drivenPrims the driven prims to update
  /// \param  currentTime the current time
  void updateDrivenTransforms(std::vector<UsdPrim>& drivenPrims, const MTime& currentTime);

  /// \brief  dirties the visibility for the specified prim index
  /// \param  primIndex the index of the prim
  /// \param  newValue the new visibility value
  void dirtyVisibility(int32_t primIndex, bool newValue)
    {
      m_drivenVisibility[primIndex] = newValue;
      m_dirtyVisibilities.push_back(primIndex);
    }

  /// \brief  dirties the matrix for the specified prim index
  /// \param  primIndex the index of the prim
  void dirtyMatrix(int32_t primIndex, const MMatrix& newValue)
    {
      m_drivenMatrix[primIndex] = newValue;
      m_dirtyMatrices.push_back(primIndex);
    }

  /// \brief  set the driven prim paths on the host driven transforms
  /// \param  primPaths the prim paths to set on the proxy
  void setDrivenPrimPaths(const SdfPathVector& primPaths)
    { m_drivenPrimPaths = primPaths; }

  /// \brief  reserve the dirtied visibility array
  /// \param  visibilityCount the number of items in the dirty visibility array to reserve
  void visibilityReserve(uint32_t visibilityCount)
    {
      m_dirtyVisibilities.clear();
      m_dirtyVisibilities.resize(visibilityCount);
    }

  /// \brief  reserve the dirtied matrix array
  /// \param  matrixCount the number of items in the dirty matrix array to reserve
  void matricesReserve(uint32_t matrixCount)
    {
      m_dirtyMatrices.clear();
      m_dirtyMatrices.resize(matrixCount);
    }

  /// \brief  returns the paths of the driven transforms
  /// \return the driven transforms
  const SdfPathVector& drivenPrimPaths() const
    { return m_drivenPrimPaths; }

  /// \brief  returns the matrices that have been dirtied
  /// \return returns the indices of the prims that have dirtied matrix params
  const std::vector<int32_t>& dirtyMatrices() const
    { return m_dirtyMatrices; }

  /// \brief  returns the visibilities that have been dirtied
  /// \return returns the indices of the prims that have dirtied visibility params
  const std::vector<int32_t>& dirtyVisibilities() const
    { return m_dirtyVisibilities; }

  /// \brief  returns the visibilities that have been dirtied
  /// \return returns the indices of the prims that have dirtied visibility params
  const std::vector<MMatrix>& drivenMatrices() const
    { return m_drivenMatrix; }

  /// \brief  returns the visibilities that have been dirtied
  /// \return returns the indices of the prims that have dirtied visibility params
  const std::vector<bool>& drivenVisibilities() const
    { return m_drivenVisibility; }


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

