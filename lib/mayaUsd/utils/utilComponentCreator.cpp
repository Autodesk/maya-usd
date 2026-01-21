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

#include "util.h"
#include "utilFileSystem.h"

#include <mayaUsd/utils/utilComponentCreator.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

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
        "            result += '\\n'\n"
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

bool isUnsavedAdskUsdComponent(const PXR_NS::UsdStageRefPtr stage)
{
    // If the component is still only really in memory, there is nothing to refresh.
    // Detect this case by check if the root layer is empty on disk.
    if (!stage) {
        return false;
    }

    auto rootLayer = stage->GetRootLayer();
    if (!rootLayer) {
        return false;
    }

    // If the root layer is not dirty, then we know for sure the on disk version is non-empty.
    if (!rootLayer->IsDirty()) {
        return false;
    }

    auto diskVersion = SdfLayer::OpenAsAnonymous(rootLayer->GetRealPath());
    if (!diskVersion) {
        return true;
    }
    return diskVersion->IsEmpty();
}

void reloadAdskUsdComponent(const std::string& proxyPath)
{
    MString saveComponent;
    saveComponent.format(
        "from pxr import Sdf, Usd, UsdUtils\n"
        "import mayaUsd\n"
        "import mayaUsd.ufe\n"
        "from usd_component_creator_plugin import MayaComponentManager\n"
        "proxyStage = mayaUsd.ufe.getStage('^1s')\n"
        "MayaComponentManager.GetInstance().ReloadComponent(proxyStage)",
        proxyPath.c_str());

    if (!MGlobal::executePythonCommand(saveComponent)) {
        TF_RUNTIME_ERROR("Error while reloading USD component '%s'", proxyPath.c_str());
    }
}

std::string previewSaveAdskUsdComponent(
    const std::string& saveLocation,
    const std::string& componentName,
    const std::string& proxyPath)
{
    MString defMoveComponentPreviewCmd;
    defMoveComponentPreviewCmd.format(
        "def usd_component_creator_move_component_preview():\n"
        "    import json\n"
        "    from pxr import Sdf, Usd, UsdUtils\n"
        "    import mayaUsd\n"
        "    import mayaUsd.ufe\n"
        "    try:\n"
        "        from AdskUsdComponentCreator import ComponentDescription, "
        "PreviewMoveComponentHierarchy\n"
        "    except ImportError:\n"
        "        return None\n"
        "    proxyStage = mayaUsd.ufe.getStage('^1s')\n"
        "    component_description = "
        "    ComponentDescription.CreateFromStageMetadata(proxyStage)\n"
        "    if component_description:\n"
        "        move_comp_preview = PreviewMoveComponentHierarchy(component_description, '^2s', "
        "'^3s')\n"
        "        return json.dumps(move_comp_preview)\n"
        "    else:\n"
        "        return \"\"",
        proxyPath.c_str(),
        saveLocation.c_str(),
        componentName.c_str());

    if (MS::kSuccess == MGlobal::executePythonCommand(defMoveComponentPreviewCmd)) {
        MString result;
        MString runComponentMovePreviewCmd = "usd_component_creator_move_component_preview()";
        if (MS::kSuccess == MGlobal::executePythonCommand(runComponentMovePreviewCmd, result)) {
            return result.asChar();
        }
    }
    return {};
}

std::string moveAdskUsdComponent(
    const std::string& saveLocation,
    const std::string& componentName,
    const std::string& proxyPath)
{
    MString defMoveComponentCmd;
    defMoveComponentCmd.format(
        "def usd_component_creator_move_component():\n"
        "    from pxr import Sdf, Usd, UsdUtils\n"
        "    import mayaUsd\n"
        "    import mayaUsd.ufe\n"
        "    try:\n"
        "        from AdskUsdComponentCreator import ComponentDescription, MoveComponent\n"
        "        from usd_component_creator_plugin import MayaComponentManager\n"
        "    except ImportError:\n"
        "        return ''\n"
        "    proxyStage = mayaUsd.ufe.getStage('^1s')\n"
        "    MayaComponentManager.GetInstance().SaveComponent(proxyStage)\n"
        "    component_description = ComponentDescription.CreateFromStageMetadata(proxyStage)\n"
        "    if not component_description:\n"
        "        return ''\n"
        "    moved_comp = MoveComponent(component_description, '^2s', '^3s', True, False)\n"
        "    return moved_comp[0].root_layer_filename\n",
        proxyPath.c_str(),
        saveLocation.c_str(),
        componentName.c_str());

    if (MS::kSuccess == MGlobal::executePythonCommand(defMoveComponentCmd)) {
        MString result;
        MString runMoveComponentCmd = "usd_component_creator_move_component()";
        if (MS::kSuccess == MGlobal::executePythonCommand(runMoveComponentCmd, result)) {
            return result.asChar();
        }
    }

    TF_RUNTIME_ERROR(
        "Error while moving USD component '%s' to '%s/%s'",
        proxyPath.c_str(),
        saveLocation.c_str(),
        componentName.c_str());
    return {};
}

bool shouldDisplayComponentInitialSaveDialog(
    const PXR_NS::UsdStageRefPtr stage,
    const std::string&           proxyShapePath)
{
    if (!MayaUsd::ComponentUtils::isAdskUsdComponent(proxyShapePath)) {
        return false;
    }

    MString tempDir;
    MGlobal::executeCommand("internalVar -userTmpDir", tempDir);
    return UsdMayaUtilFileSystem::isPathInside(
        UsdMayaUtil::convert(tempDir), stage->GetRootLayer()->GetRealPath());
}

} // namespace ComponentUtils
} // namespace MAYAUSD_NS_DEF
