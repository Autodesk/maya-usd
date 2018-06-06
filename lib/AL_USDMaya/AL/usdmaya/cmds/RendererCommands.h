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

#include "../Api.h"

#include "AL/usdmaya/ForwardDeclares.h"
#include "AL/maya/utils/Api.h"
#include "AL/maya/utils/MayaHelperMacros.h"
#include "maya/MDGModifier.h"
#include "maya/MObject.h"
#include "maya/MPxCommand.h"

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include <functional>

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Get / Set renderer plugin settings
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ManageRenderer
  : public MPxCommand
{
  MArgDatabase makeDatabase(const MArgList& args);
public:
  AL_MAYA_DECLARE_COMMAND();
private:
  bool isUndoable() const override;
  MStatus doIt(const MArgList& args) override;
};


/// \brief  function called on startup to generate the menu & option boxes for the layer commands
/// \ingroup commands
AL_USDMAYA_PUBLIC
void constructRendererCommandGuis();

//----------------------------------------------------------------------------------------------------------------------
} // cmds
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
