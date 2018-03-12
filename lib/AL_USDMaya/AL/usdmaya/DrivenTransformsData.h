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

#include "AL/usdmaya/nodes/proxy/DrivenTransforms.h"
#include "maya/MPxData.h"
#include "maya/MVector.h"
#include "maya/MMatrix.h"

#include <vector>
#include <string>
#include "AL/usd/utils/ForwardDeclares.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The driven transform data passed through the DG
//----------------------------------------------------------------------------------------------------------------------
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
  nodes::proxy::DrivenTransforms m_drivenTransforms;

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
