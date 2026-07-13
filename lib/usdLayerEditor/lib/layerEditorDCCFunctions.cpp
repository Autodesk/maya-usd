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

#include <pxr/usd/usdUtils/stageCache.h>

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
void setFileSystemFns(const FileSystemFns& fns) { registry().fileSystem = fns; }
void setSerializationFns(const SerializationFns& fns) { registry().serialization = fns; }
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
bool isEditForwardDialogOpen()
{
    return registry().editForwarding.isEditForwardDialogOpen
        ? registry().editForwarding.isEditForwardDialogOpen()
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
std::string renameObject(const std::string& oldDccObjectPath, const std::string& newName)
{
    return registry().dccObject.renameObject
        ? registry().dccObject.renameObject(oldDccObjectPath, newName)
        : std::string {};
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
QWidget* getMainWindowParent()
{
    return registry().environment.getMainWindowParent ? registry().environment.getMainWindowParent()
                                                    : nullptr;
}
int64_t getLayerContentsArraySizeLimit()
{
    return registry().environment.getLayerContentsArraySizeLimit
        ? registry().environment.getLayerContentsArraySizeLimit()
        : 8;
}
int64_t getLayerContentsTimeSamplesSizeLimit()
{
    return registry().environment.getLayerContentsTimeSamplesSizeLimit
        ? registry().environment.getLayerContentsTimeSamplesSizeLimit()
        : 8;
}
void displayError(const std::string& error)
{
    if (registry().environment.displayError)
        registry().environment.displayError(error);
}

// ---- FileSystem ----
std::string getDCCSceneDir()
{
    return registry().fileSystem.getDCCSceneDir
        ? registry().fileSystem.getDCCSceneDir()
        : std::string {};
}
std::string getDCCWorkspaceScenesDir()
{
    return registry().fileSystem.getDCCWorkspaceScenesDir
        ? registry().fileSystem.getDCCWorkspaceScenesDir()
        : std::string {};
}
std::string sceneFolder()
{
    return registry().fileSystem.sceneFolder ? registry().fileSystem.sceneFolder()
                                             : std::string {};
}
bool prepareLayerSaveUILayer(const std::string& relativeAnchor)
{
    return registry().fileSystem.prepareLayerSaveUILayer
        ? registry().fileSystem.prepareLayerSaveUILayer(relativeAnchor)
        : true;
}
bool checkWriteAccess(const std::string& filePath)
{
    return registry().fileSystem.checkWriteAccess
        ? registry().fileSystem.checkWriteAccess(filePath)
        : false;
}

// ---- Serialization ----
std::vector<PXR_NS::UsdStageCache*> getStageCaches()
{
    return registry().serialization.getStageCaches
        ? registry().serialization.getStageCaches()
        : std::vector<PXR_NS::UsdStageCache*> { &PXR_NS::UsdUtilsStageCache::Get() };
}
std::vector<PXR_NS::UsdStageRefPtr> getAllStages()
{
    return registry().serialization.getAllStages
        ? registry().serialization.getAllStages()
        : PXR_NS::UsdUtilsStageCache::Get().GetAllStages();
}
void setLayerUpAxisAndUnits(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (registry().serialization.setLayerUpAxisAndUnits)
        registry().serialization.setLayerUpAxisAndUnits(layer);
}
void updateDCCObjectRootLayer(
    const std::string&            dccObjectPath,
    const std::string&            layerPath,
    const PXR_NS::SdfLayerRefPtr& layer,
    bool                          wasTargetLayer,
    DccObjectRootLayerPathMode    pathMode)
{
    if (registry().serialization.updateDCCObjectRootLayer)
        registry().serialization.updateDCCObjectRootLayer(
            dccObjectPath, layerPath, layer, wasTargetLayer, pathMode);
}
PXR_NS::SdfLayerRefPtr captureSessionLayer(const std::string& dccObjectPath)
{
    return registry().serialization.captureSessionLayer
        ? registry().serialization.captureSessionLayer(dccObjectPath)
        : PXR_NS::SdfLayerRefPtr {};
}
void transferSessionLayer(
    const PXR_NS::SdfLayerRefPtr& sourceSessionLayer,
    const std::string&            dstDccObjectPath)
{
    if (registry().serialization.transferSessionLayer)
        registry().serialization.transferSessionLayer(sourceSessionLayer, dstDccObjectPath);
}

} // namespace UsdLayerEditor
