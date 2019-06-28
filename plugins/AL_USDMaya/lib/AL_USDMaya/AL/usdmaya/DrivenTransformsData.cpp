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
#include "AL/usdmaya/DrivenTransformsData.h"
#include "AL/usdmaya/TypeIDs.h"
#include "maya/MTypeId.h"
#include "maya/MString.h"

namespace AL {
namespace usdmaya {


//----------------------------------------------------------------------------------------------------------------------
const MTypeId DrivenTransformsData::kTypeId(AL_USDMAYA_DRIVENTRANSFORMS_DATA);
const MString DrivenTransformsData::kName("AL_usdmaya_DrivenTransformsData");

//----------------------------------------------------------------------------------------------------------------------
void* DrivenTransformsData::creator()
{
  return new DrivenTransformsData;
}

//----------------------------------------------------------------------------------------------------------------------
DrivenTransformsData::DrivenTransformsData()
{
}

//----------------------------------------------------------------------------------------------------------------------
DrivenTransformsData::~DrivenTransformsData()
{
}

//----------------------------------------------------------------------------------------------------------------------
void DrivenTransformsData::copy(const MPxData& data)
{
  const DrivenTransformsData* transformsData = static_cast<const DrivenTransformsData*>(&data);
  if (transformsData)
  {
    m_drivenTransforms = transformsData->m_drivenTransforms;
  }
}

//----------------------------------------------------------------------------------------------------------------------
MTypeId DrivenTransformsData::typeId() const
{
  return kTypeId;
}

//----------------------------------------------------------------------------------------------------------------------
MString DrivenTransformsData::name() const
{
  return kName;
}
//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
