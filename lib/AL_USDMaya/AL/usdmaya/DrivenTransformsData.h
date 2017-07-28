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

#include "AL/usdmaya/Common.h"
#include "maya/MPxData.h"
#include "maya/MVector.h"
#include "maya/MMatrix.h"

#include <vector>
#include <string>
#include <memory>

namespace AL {
namespace usdmaya {


struct DrivenTransforms
{
  inline uint32_t transformCount() const { return m_drivenPrimPaths.size(); }
  void initTransform(uint32_t index);

  std::vector<std::string> m_drivenPrimPaths;
  std::vector<MMatrix> m_drivenMatrix;
  std::vector<bool> m_drivenVisibility;
  std::vector<int32_t> m_dirtyMatrices;
  std::vector<int32_t> m_dirtyVisibilities;
};

class DrivenTransformsData
  : public MPxData
{
public:

  /// \brief ctor
  DrivenTransformsData();

  /// \brief dtor
  ~DrivenTransformsData();

  /// \brief creates an instance of this data object
  static void* creator();

  /// the type id of the driven transform data
  static const MTypeId kTypeId;

  /// the type name of the driven transform data
  static const MString kName;

  /// the structure of driven transform
  DrivenTransforms m_drivenTransforms;

private:
  void copy(const MPxData& data) override;

private:
  MTypeId typeId() const override;
  MString name() const override;
};

//----------------------------------------------------------------------------------------------------------------------
}// usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
