//
// Copyright 2017 Animal Logic
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

#include "../Api.h"

#include "maya/MPxCommand.h"

#if MAYA_API_VERSION < 201800
#include "maya/MArgDatabase.h"
#endif

#include "AL/maya/utils/Api.h"
#include "AL/maya/utils/MayaHelperMacros.h"


namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command that allows you to query and modify the current status of the TfDebug symbols.
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class UsdDebugCommand
  : public MPxCommand
{
public:
  AL_MAYA_DECLARE_COMMAND();
private:
  bool isUndoable() const override;
  MStatus doIt(const MArgList& args) override;
};

/// builds the GUI for the TfDebug notices
AL_USDMAYA_PUBLIC
void constructDebugCommandGuis();

} // cmds
} // usdmaya
} // AL



