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
void setSaveOptionFns(const SaveOptionFns& fns) { registry().saveOption = fns; }
void setEnvironmentFns(const EnvironmentFns& fns) { registry().environment = fns; }
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
std::string renameProxyShape(const std::string& oldDccObjectPath, const std::string& newName)
{
    return registry().component.renameProxyShape
        ? registry().component.renameProxyShape(oldDccObjectPath, newName)
        : std::string {};
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
void displayError(const std::string& error)
{
    if (registry().component.displayError)
        registry().component.displayError(error);
}
void transferSessionLayer(const std::string& oldDccObjectPath, const std::string& newDccObjectPath)
{
    if (registry().component.transferSessionLayer)
        registry().component.transferSessionLayer(oldDccObjectPath, newDccObjectPath);
}
void setProxyRootLayerPath(
    const std::string&            dccObjectPath,
    const std::string&            rootLayerPath,
    const PXR_NS::SdfLayerRefPtr& rootLayer)
{
    if (registry().component.setProxyRootLayerPath)
        registry().component.setProxyRootLayerPath(dccObjectPath, rootLayerPath, rootLayer);
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
void openEditForwardDialog(const PXR_NS::UsdStageRefPtr& currentStage)
{
    if (registry().editForwarding.openEditForwardDialog)
        registry().editForwarding.openEditForwardDialog(currentStage);
}
bool handleEFEditTargetUpdate(const PXR_NS::UsdStageRefPtr& stage)
{
    return registry().editForwarding.handleEFEditTargetUpdate
        ? registry().editForwarding.handleEFEditTargetUpdate(stage)
        : false;
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

// ---- SaveOption ----
bool requireUsdPathsRelativeToSceneFile()
{
    return registry().saveOption.requireUsdPathsRelativeToSceneFile
        ? registry().saveOption.requireUsdPathsRelativeToSceneFile()
        : true;
}
bool requireUsdPathsRelativeToParentLayer()
{
    return registry().saveOption.requireUsdPathsRelativeToParentLayer
        ? registry().saveOption.requireUsdPathsRelativeToParentLayer()
        : true;
}
bool requireUsdPathsRelativeToEditTargetLayer()
{
    return registry().saveOption.requireUsdPathsRelativeToEditTargetLayer
        ? registry().saveOption.requireUsdPathsRelativeToEditTargetLayer()
        : true;
}
bool wantReferenceCompositionArc()
{
    return registry().saveOption.wantReferenceCompositionArc
        ? registry().saveOption.wantReferenceCompositionArc()
        : false;
}
bool wantPrependCompositionArc()
{
    return registry().saveOption.wantPrependCompositionArc
        ? registry().saveOption.wantPrependCompositionArc()
        : true;
}
bool wantPayloadLoaded()
{
    return registry().saveOption.wantPayloadLoaded
        ? registry().saveOption.wantPayloadLoaded()
        : true;
}
std::string getReferencedPrimPath()
{
    return registry().saveOption.getReferencedPrimPath
        ? registry().saveOption.getReferencedPrimPath()
        : std::string();
}
void setRequireUsdPathsRelativeToSceneFile(bool value)
{
    if (registry().saveOption.setRequireUsdPathsRelativeToSceneFile)
        registry().saveOption.setRequireUsdPathsRelativeToSceneFile(value);
}
void setRequireUsdPathsRelativeToParentLayer(bool value)
{
    if (registry().saveOption.setRequireUsdPathsRelativeToParentLayer)
        registry().saveOption.setRequireUsdPathsRelativeToParentLayer(value);
}
bool confirmExistingFileSave()
{
    return registry().saveOption.confirmExistingFileSave
        ? registry().saveOption.confirmExistingFileSave()
        : true;
}
bool getSaveLayerFormatBinary()
{
    return registry().saveOption.getSaveLayerFormatBinary
        ? registry().saveOption.getSaveLayerFormatBinary()
        : true;
}
void setSaveLayerFormatBinary(bool value)
{
    if (registry().saveOption.setSaveLayerFormatBinary)
        registry().saveOption.setSaveLayerFormatBinary(value);
}
int getSerializedUsdEditsLocation()
{
    return registry().saveOption.getSerializedUsdEditsLocation
        ? registry().saveOption.getSerializedUsdEditsLocation()
        : 1; // kSaveToUSDFiles
}
void setSerializedUsdEditsLocation(int value)
{
    if (registry().saveOption.setSerializedUsdEditsLocation)
        registry().saveOption.setSerializedUsdEditsLocation(value);
}
// ---- Environment ----
bool getPinLayerEditorStage()
{
    return registry().environment.getPinLayerEditorStage
        ? registry().environment.getPinLayerEditorStage()
        : false;
}
void setPinLayerEditorStage(bool value)
{
    if (registry().environment.setPinLayerEditorStage)
        registry().environment.setPinLayerEditorStage(value);
}
bool isInteractiveDCCSession()
{
    return registry().environment.isInteractiveDCCSession
        ? registry().environment.isInteractiveDCCSession()
        : true;
}
bool shouldExpandOrCollapseAll()
{
    return registry().environment.shouldExpandOrCollapseAll
        ? registry().environment.shouldExpandOrCollapseAll()
        : false;
}
QWidget* mainWindowParent()
{
    return registry().environment.mainWindowParent ? registry().environment.mainWindowParent()
                                                    : nullptr;
}
int64_t layerContentsArraySizeLimit()
{
    return registry().environment.layerContentsArraySizeLimit
        ? registry().environment.layerContentsArraySizeLimit()
        : 8;
}
int64_t layerContentsTimeSamplesSizeLimit()
{
    return registry().environment.layerContentsTimeSamplesSizeLimit
        ? registry().environment.layerContentsTimeSamplesSizeLimit()
        : 8;
}

} // namespace UsdLayerEditor
