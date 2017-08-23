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

  /// \brief  returns the number of transforms
  inline size_t transformCount() const
    { return m_drivenPrimPaths.size(); }

  /// \brief  initialise the transform at the specified index
  /// \param  index the transform index
  void initTransform(uint32_t index);

  /// \brief  update the driven prim paths
  /// \param  drivenIndex the index of the transform to update
  /// \param  drivenPaths the driven paths to update
  /// \param  drivenPrims the driven prims to update
  void updateDrivenPrimPaths(uint32_t drivenIndex, SdfPathVector& drivenPaths, std::vector<UsdPrim>& drivenPrims, UsdStageRefPtr stage);

  /// \brief  update the driven prim visibility
  /// \param  drivenPrims the driven prims to update
  /// \param  currentTime the current time
  void updateDrivenVisibility(std::vector<UsdPrim>& drivenPrims, const MTime& currentTime);

  /// \brief  update the driven prim transforms
  /// \param  drivenPrims the driven prims to update
  /// \param  currentTime the current time
  void updateDrivenTransforms(std::vector<UsdPrim>& drivenPrims, const MTime& currentTime);

  void dirtyVisibility(int32_t primIndex, bool newValue)
    {
      m_drivenVisibility[primIndex] = newValue;
      m_dirtyVisibilities.push_back(primIndex);
    }

  void dirtyMatrix(int32_t primIndex)
    { m_dirtyMatrices.push_back(primIndex); }

  void clearDirtyMatrices()
    { m_dirtyMatrices.clear(); }

  void clearDirtyVisibilities()
    { m_dirtyVisibilities.clear(); }

  ///
  void setDrivenPrimPaths(const SdfPathVector& primPaths)
    { m_drivenPrimPaths = primPaths; }

  /// \brief  reserve the visibility
  void visibilityReserve(uint32_t visibilityCnt)
    {
      m_dirtyVisibilities.clear();
      m_dirtyVisibilities.resize(visibilityCnt);
    }

  /// \brief  reserve the visibility
  void matricesReserve(uint32_t visibilityCnt)
    {
      m_dirtyMatrices.clear();
      m_dirtyMatrices.resize(visibilityCnt);
    }

  void setMatrix(const MMatrix& m, uint32_t index)
    {
      m_drivenMatrix[index] = m;
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

