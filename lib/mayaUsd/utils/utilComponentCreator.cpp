//
// Copyright 2025 Autodesk
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

#include <mayaUsd/utils/utilComponentCreator.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/pxr.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ComponentUtils {

std::vector<std::string> getAdskUsdComponentLayersToSave(const std::string& proxyPath)
{
    // Ask via python what layers need to be saved for the component.
    // With the maya api we can only return a string, so we concat the ids.
    MString getLayersFromComponent;
    getLayersFromComponent.format(
        "def usd_component_creator_get_layers_to_save():\n"
        "    import mayaUsd\n"
        "    import mayaUsd.ufe\n"
        "    from usd_component_creator_plugin import MayaComponentManager\n"
        "    stage = mayaUsd.ufe.getStage('^1s')\n"
        "    if stage is None:\n"
        "        return ''\n"
        "    ids = MayaComponentManager.GetInstance().GetSaveInfo(stage)\n"
        "    result = ''\n"
        "    first = True\n"
        "    for id in ids:\n"
        "        if not first:\n"
        "            result += '\n'\n"
        "        result += id\n"
        "        first = False\n"
        "    return result\n",
        proxyPath.c_str());

    if (MGlobal::executePythonCommand(getLayersFromComponent)) {
        auto resultString = MGlobal::executePythonCommandStringResult(
            "usd_component_creator_get_layers_to_save()");
        MStringArray layerIds;
        resultString.split('\n', layerIds);
        std::vector<std::string> toSave;
        for (const auto& id : layerIds) {
            toSave.push_back(id.asUTF8());
        }
        return toSave;
    }
    return {};
}

bool isAdskUsdComponent(const std::string& proxyShapePath)
{
    MString defineIsComponentCmd;
    defineIsComponentCmd.format(
        "def usd_component_creator_is_proxy_shape_a_component():\n"
        "    from pxr import Sdf, Usd, UsdUtils\n"
        "    import mayaUsd\n"
        "    import mayaUsd.ufe\n"
        "    try:\n"
        "        from AdskUsdComponentCreator import ComponentDescription\n"
        "    except ImportError:\n"
        "        return -1\n"
        "    proxyStage = mayaUsd.ufe.getStage('^1s')\n"
        "    component_description = ComponentDescription.CreateFromStageMetadata(proxyStage)\n"
        "    if component_description:\n"
        "        return 1\n"
        "    else:\n"
        "        return 0",
        proxyShapePath.c_str());

    int     isStageAComponent = 0;
    MStatus success;
    if (MS::kSuccess
        == (success = MGlobal::executePythonCommand(defineIsComponentCmd, false, false))) {
        MString runIsComponentCmd = "usd_component_creator_is_proxy_shape_a_component()";
        success = MGlobal::executePythonCommand(runIsComponentCmd, isStageAComponent);
    }

    if (success != MS::kSuccess) {
        TF_RUNTIME_ERROR(
            "Error occurred when testing stage '%s' for component.", proxyShapePath.c_str());
    }

    return isStageAComponent == 1;
}

void saveAdskUsdComponent(const std::string& proxyPath)
{
    MString saveComponent;
    saveComponent.format(
        "from pxr import Sdf, Usd, UsdUtils\n"
        "import mayaUsd\n"
        "import mayaUsd.ufe\n"
        "from usd_component_creator_plugin import MayaComponentManager\n"
        "proxyStage = mayaUsd.ufe.getStage('^1s')\n"
        "MayaComponentManager.GetInstance().SaveComponent(proxyStage)",
        proxyPath.c_str());

    if (!MGlobal::executePythonCommand(saveComponent)) {
        TF_RUNTIME_ERROR("Error while saving USD component '%s'", proxyPath.c_str());
    }
}

} // namespace ComponentUtils
} // namespace MAYAUSD_NS_DEF