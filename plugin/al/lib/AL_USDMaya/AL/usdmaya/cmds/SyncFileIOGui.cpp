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
#include "AL/usdmaya/cmds/SyncFileIOGui.h"

#include "AL/maya/utils/MenuBuilder.h"
#include "AL/maya/utils/PluginTranslatorOptions.h"
#include "AL/usdmaya/DebugCodes.h"

#include <maya/MArgDatabase.h>
#include <maya/MSyntax.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace cmds {

AL_MAYA_DEFINE_COMMAND(SyncFileIOGui, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax SyncFileIOGui::createSyntax()
{
    MSyntax syn;
    syn.addFlag("-h", "-help", MSyntax::kNoArg);
    syn.addArg(MSyntax::kString);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool SyncFileIOGui::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MStatus SyncFileIOGui::doIt(const MArgList& argList)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("AL_usdmaya_SyncFileIOGui::doIt\n");
    try {
        MStatus      status;
        MArgDatabase args(syntax(), argList, &status);
        if (!status)
            return status;

        AL_MAYA_COMMAND_HELP(args, g_helpText);

        MString translatorName;
        status = args.getCommandArgument(0, translatorName);
        if (!status)
            return status;

        maya::utils::PluginTranslatorOptionsContextManager::resyncGUI(translatorName.asChar());
    } catch (const MStatus&) {
    }
    return MS::kSuccess;
}

const char* const SyncFileIOGui::g_helpText = R"(
    AL_usdmaya_SyncFileIOGui Overview:

      This command is for internal use.

      This command resyncs the MEL code needed to create the GUI components for plug-in file
    translator options. Within the AL_USDMaya plug-in, there are two possible option GUI's that
    can be synced...

    For Import:    AL_usdmaya_SyncFileIOGui "ImportTranslator"
    For Export:    AL_usdmaya_SyncFileIOGui "ExportTranslator"

    You shouldn't have to call these methods manually - they should be called automatically.

)";

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
