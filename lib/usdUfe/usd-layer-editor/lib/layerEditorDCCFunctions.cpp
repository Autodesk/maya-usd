//
// Copyright 2026 Autodesk
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
#include "layerEditorDCCFunctions.h"

namespace UsdLayerEditor {

namespace {
LayerEditorDCCFunctions& registry()
{
    static LayerEditorDCCFunctions sFunctions;
    return sFunctions;
}
} // namespace

void setComponentFns(const ComponentFns& fns) { registry().component = fns; }
void setEditForwardingFns(const EditForwardingFns& fns) { registry().editForwarding = fns; }
void setDccObjectFns(const DccObjectFns& fns) { registry().dccObject = fns; }
void setLayerEditorDCCFunctions(const LayerEditorDCCFunctions& fns) { registry() = fns; }
const LayerEditorDCCFunctions& layerEditorDCCFunctions() { return registry(); }

// ---- Component ----
void saveComponent(const PXR_NS::UsdStageRefPtr& stage, const std::string& dccObjectPath)
{
    if (registry().component.saveComponent)
        registry().component.saveComponent(stage, dccObjectPath);
}
void reloadComponent(const std::string& dccObjectPath)
{
    if (registry().component.reloadComponent)
        registry().component.reloadComponent(dccObjectPath);
}
void renameProxyShape(const std::string& oldDccObjectPath, const std::string& newName)
{
    if (registry().component.renameProxyShape)
        registry().component.renameProxyShape(oldDccObjectPath, newName);
}
bool isStageAComponent(const std::string& dccObjectPath)
{
    return registry().component.isStageAComponent
        ? registry().component.isStageAComponent(dccObjectPath)
        : false;
}
bool isUnsavedComponent(const PXR_NS::UsdStageRefPtr& stage)
{
    return registry().component.isUnsavedComponent
        ? registry().component.isUnsavedComponent(stage)
        : false;
}
bool shouldDisplayComponentInitialSaveDialog(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            dccObjectPath)
{
    return registry().component.shouldDisplayComponentInitialSaveDialog
        ? registry().component.shouldDisplayComponentInitialSaveDialog(stage, dccObjectPath)
        : false;
}
std::string sceneFolder()
{
    return registry().component.sceneFolder ? registry().component.sceneFolder() : std::string {};
}
std::string moveComponent(
    const std::string& saveLocation,
    const std::string& componentName,
    const std::string& dccObjectPath)
{
    return registry().component.moveComponent
        ? registry().component.moveComponent(saveLocation, componentName, dccObjectPath)
        : std::string {};
}
std::string previewComponentSave(
    const std::string& saveLocation,
    const std::string& componentName,
    const std::string& dccObjectPath)
{
    return registry().component.previewComponentSave
        ? registry().component.previewComponentSave(saveLocation, componentName, dccObjectPath)
        : std::string {};
}
std::vector<std::string> getComponentLayersToSave(const std::string& dccObjectPath)
{
    return registry().component.getComponentLayersToSave
        ? registry().component.getComponentLayersToSave(dccObjectPath)
        : std::vector<std::string> {};
}

// ---- Edit Forwarding ----
bool supportsEditForwarding()
{
    return registry().editForwarding.supportsEditForwarding
        ? registry().editForwarding.supportsEditForwarding()
        : false;
}
bool echoEditForwarding()
{
    return registry().editForwarding.echoEditForwarding
        ? registry().editForwarding.echoEditForwarding()
        : false;
}
void setEchoEditForwarding(bool echo)
{
    if (registry().editForwarding.setEchoEditForwarding)
        registry().editForwarding.setEchoEditForwarding(echo);
}

// ---- DCC object/stage queries ----
bool isDccObjectStageIncoming(const std::string& dccObjectPath)
{
    return registry().dccObject.isDccObjectStageIncoming
        ? registry().dccObject.isDccObjectStageIncoming(dccObjectPath)
        : false;
}
bool isDccObjectSharedStage(const std::string& dccObjectPath)
{
    return registry().dccObject.isDccObjectSharedStage
        ? registry().dccObject.isDccObjectSharedStage(dccObjectPath)
        : true; // matches the former AbstractCommandHook default
}

} // namespace UsdLayerEditor
