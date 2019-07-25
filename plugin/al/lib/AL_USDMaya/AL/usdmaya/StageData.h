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

#include "AL/usdmaya/Api.h"

#include "AL/event/EventHandler.h"

#include <mayaUsd/nodes/stageData.h>

#include "pxr/usd/usd/stage.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This code is effectively copied from the pixar plugin. It's just used to pass the usd stage through the DG
/// \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------

// At time of writing, we keep this class for AL backward compatibility only,
// because it's registered with a different type ID than its base class, and
// existing Maya files with "requires" statements depend on it.  It should be
// unused otherwise.  To investigate: mark StageData attributes as
// non-storable.  PPT, 8-Apr-2019.

class StageData
  : public MayaUsdStageData
{
public:

  typedef MayaUsdStageData ParentClass;

  /// \brief  ctor
  StageData();

  /// \brief  dtor
  ~StageData();

  /// \brief  creates an instance of this data object
  AL_USDMAYA_PUBLIC
  static void* creator();

  /// the type id of the stage data
  AL_USDMAYA_PUBLIC
  static const MTypeId kTypeId;

  /// the type name of the stage data
  AL_USDMAYA_PUBLIC
  static const MString kName;

private:
  MTypeId typeId() const override;
  MString name() const override;
};

//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
