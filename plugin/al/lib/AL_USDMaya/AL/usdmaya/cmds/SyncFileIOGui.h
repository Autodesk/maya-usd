//
// Copyright 2019 Animal Logic
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

#include <AL/maya/utils/MayaHelperMacros.h>
#include <AL/usdmaya/Api.h>

#include <maya/MPxCommand.h>

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command that is used to pre-sync the auto generated GUI for the plugin options to a
/// translator. \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class SyncFileIOGui : public MPxCommand
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
};

} // namespace cmds
} // namespace usdmaya
} // namespace AL
