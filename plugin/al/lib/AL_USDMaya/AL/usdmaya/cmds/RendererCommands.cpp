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
#include "AL/usdmaya/cmds/RendererCommands.h"

#include "AL/maya/utils/CommandGuiHelper.h"
#include "AL/usdmaya/nodes/RendererManager.h"

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MSyntax.h>

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Get / Set renderer plugin settings
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ManageRenderer, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax ManageRenderer::createSyntax()
{
    MSyntax syn;
    syn.addFlag("-h", "-help", MSyntax::kNoArg);
    syn.addFlag("-sp", "-setPlugin", MSyntax::kString);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool ManageRenderer::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MStatus ManageRenderer::doIt(const MArgList& argList)
{
    try {
        MStatus      status;
        MArgDatabase args(syntax(), argList, &status);
        if (!status)
            return status;

        AL_MAYA_COMMAND_HELP(args, g_helpText);

        if (args.isFlagSet("-sp")) {
            MString name;
            args.getFlagArgument("-sp", 0, name);
            bool result = nodes::RendererManager::findOrCreateManager()->setRendererPlugin(name);
            setResult(result);
        }
    } catch (const MStatus& status) {
        return status;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStringArray buildRendererPluginsList(const MString&)
{
    nodes::RendererManager* RendererManager = nodes::RendererManager::findManager();
    if (RendererManager) {
        int index = RendererManager->getRendererPluginIndex();
        if (index > 0) {
            // swap items so current plugin is first and active in the list
            MStringArray result = AL::usdmaya::nodes::RendererManager::getRendererPluginList();
            MString      temp = result[0];
            result[0] = result[index];
            result[index] = temp;
            return result;
        }
    }
    return AL::usdmaya::nodes::RendererManager::getRendererPluginList();
}

//----------------------------------------------------------------------------------------------------------------------
void constructRendererCommandGuis()
{
    /// It makes little sense to add this menu when there's just one option
    if (AL::usdmaya::nodes::RendererManager::getRendererPluginList().length() > 1) {
        {
            AL::maya::utils::CommandGuiHelper manageRenderer(
                "AL_usdmaya_ManageRenderer", "Hydra Renderer Plugin", "Set", "USD/Renderer", false);
            manageRenderer.addListOption(
                "sp", "Plugin Name", (AL::maya::utils::GenerateListFn)buildRendererPluginsList);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Documentation strings.
//----------------------------------------------------------------------------------------------------------------------

const char* const ManageRenderer::g_helpText = R"(
ManageRenderer Overview:

  This command allows you to manage global renderer settings:

     ManageRenderer -sp "Embree";  //< sets current renderer to Intel Embree
     ManageRenderer -sp "Glimpse";  //< sets current renderer to glimpse
     ManageRenderer -sp "GL";  //< sets current renderer to Hydra GL
)";

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
