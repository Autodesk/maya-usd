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
#ifndef LAYER_EDITOR_DCC_FUNCTIONS_H
#define LAYER_EDITOR_DCC_FUNCTIONS_H

#include "layerEditorAPI.h"

#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/stageCache.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace UsdLayerEditor {

// std::function typedefs use the EXACT signatures of the former base-class overrides.
using SaveComponentFn    = std::function<void(const PXR_NS::UsdStageRefPtr&, const std::string&)>;
using ReloadComponentFn  = std::function<void(const std::string&)>;
using IsStageAComponentFn  = std::function<bool(const std::string&)>;
using IsUnsavedComponentFn = std::function<bool(const PXR_NS::UsdStageRefPtr&)>;
using ShouldDisplayComponentInitialSaveDialogFn
    = std::function<bool(const PXR_NS::UsdStageRefPtr&, const std::string&)>;
using SceneFolderFn = std::function<std::string()>;
using MoveComponentFn
    = std::function<std::string(const std::string&, const std::string&, const std::string&)>;
using PreviewComponentSaveFn
    = std::function<std::string(const std::string&, const std::string&, const std::string&)>;
using GetComponentLayersToSaveFn = std::function<std::vector<std::string>(const std::string&)>;
using CaptureSessionLayerFn  = std::function<PXR_NS::SdfLayerRefPtr(const std::string&)>;
using TransferSessionLayerFn = std::function<void(const PXR_NS::SdfLayerRefPtr&, const std::string&)>;

// How updateDCCObjectRootLayer should resolve the root-layer path written back to the DCC object.
enum class DccObjectRootLayerPathMode
{
    FollowPreference, // honor the proxy / option-var relative-vs-absolute path-mode preference
    ForceAbsolute,    // ignore the preference and write an absolute root-layer path
};

using SupportsEditForwardingFn = std::function<bool()>;
using EchoEditForwardingFn     = std::function<bool()>;
using SetEchoEditForwardingFn  = std::function<void(bool)>;
using OpenEditForwardDialogFn  = std::function<void(const PXR_NS::UsdStageRefPtr&)>;
// Returns true if edit forwarding is active and has handled the stage's edit
// target (the caller then skips the normal auto-targeting); false otherwise.
using HandleEFEditTargetUpdateFn = std::function<bool(const PXR_NS::UsdStageRefPtr&)>;
using IsEditForwardDialogOpenFn = std::function<bool()>;

using IsDccObjectStageIncomingFn = std::function<bool(const std::string&)>;
using IsDccObjectSharedStageFn   = std::function<bool(const std::string&)>;
// Returns the new DCC object path of the renamed object (empty if no rename happened).
using RenameObjectFn = std::function<std::string(const std::string&, const std::string&)>;

struct ComponentFns
{
    SaveComponentFn                           saveComponent;
    ReloadComponentFn                         reloadComponent;
    IsStageAComponentFn                       isStageAComponent;
    IsUnsavedComponentFn                      isUnsavedComponent;
    ShouldDisplayComponentInitialSaveDialogFn shouldDisplayComponentInitialSaveDialog;
    MoveComponentFn                           moveComponent;
    PreviewComponentSaveFn                    previewComponentSave;
    GetComponentLayersToSaveFn                getComponentLayersToSave;
};

struct EditForwardingFns
{
    SupportsEditForwardingFn         supportsEditForwarding;
    EchoEditForwardingFn             echoEditForwarding;
    SetEchoEditForwardingFn          setEchoEditForwarding;
    OpenEditForwardDialogFn          openEditForwardDialog;
    HandleEFEditTargetUpdateFn handleEFEditTargetUpdate;
    IsEditForwardDialogOpenFn isEditForwardDialogOpen; // default false when unset
};

struct DccObjectFns
{
    IsDccObjectStageIncomingFn isDccObjectStageIncoming;
    IsDccObjectSharedStageFn   isDccObjectSharedStage;
    RenameObjectFn             renameObject;
};

struct SaveOptionFns
{
    std::function<bool()>        requireUsdPathsRelativeToSceneFile;
    std::function<bool()>        requireUsdPathsRelativeToParentLayer;
    std::function<bool()>        requireUsdPathsRelativeToEditTargetLayer;
    std::function<bool()>        wantReferenceCompositionArc;
    std::function<bool()>        wantPrependCompositionArc;
    std::function<bool()>        wantPayloadLoaded;
    std::function<std::string()> getReferencedPrimPath;
    std::function<void(bool)>    setRequireUsdPathsRelativeToSceneFile;
    std::function<void(bool)>    setRequireUsdPathsRelativeToParentLayer;
    std::function<bool()>        confirmExistingFileSave;
    std::function<bool()>        getSaveLayerFormatBinary;
    std::function<void(bool)>    setSaveLayerFormatBinary;
    std::function<int()>         getSerializedUsdEditsLocation;
    std::function<void(int)>     setSerializedUsdEditsLocation;
};

struct EnvironmentFns
{
    std::function<bool()>                    getPinLayerEditorStage;
    std::function<void(bool)>                setPinLayerEditorStage;
    std::function<bool()>                    isInteractiveDCCSession;
    std::function<bool()>                    shouldExpandOrCollapseAll;
    std::function<int64_t()>                 getLayerContentsArraySizeLimit;
    std::function<int64_t()>                 getLayerContentsTimeSamplesSizeLimit;
    std::function<void(const std::string&)>  displayError;
};

struct FileSystemFns
{
    std::function<std::string()>            getDCCSceneDir;
    std::function<std::string()>            getDCCWorkspaceScenesDir;
    SceneFolderFn                           sceneFolder;
    std::function<bool(const std::string&)> prepareLayerSaveUILayer;
    std::function<bool(const std::string&)> checkWriteAccess;
};

struct SerializationFns
{
    std::function<std::vector<PXR_NS::UsdStageCache*>()> getStageCaches;
    std::function<std::vector<PXR_NS::UsdStageRefPtr>()> getAllStages;
    std::function<void(const PXR_NS::SdfLayerRefPtr&)>   setLayerUpAxisAndUnits;
    std::function<void(
        const std::string&,
        const std::string&,
        const PXR_NS::SdfLayerRefPtr&,
        bool,
        DccObjectRootLayerPathMode)>
                           updateDCCObjectRootLayer;
    CaptureSessionLayerFn  captureSessionLayer;
    TransferSessionLayerFn transferSessionLayer;
};

struct LayerEditorDCCFunctions
{
    ComponentFns      component;
    EditForwardingFns editForwarding;
    DccObjectFns      dccObject;
    SaveOptionFns     saveOption;
    EnvironmentFns    environment;
    FileSystemFns     fileSystem;
    SerializationFns  serialization;
};

// Registration API — per-group setters (play cleanly with #ifdef guards), plus a
// full-struct setter and a getter used by the test RAII helper.
LAYEREDITOR_PUBLIC void setComponentFns(const ComponentFns&);
LAYEREDITOR_PUBLIC void setEditForwardingFns(const EditForwardingFns&);
LAYEREDITOR_PUBLIC void setDccObjectFns(const DccObjectFns&);
LAYEREDITOR_PUBLIC void setSaveOptionFns(const SaveOptionFns&);
LAYEREDITOR_PUBLIC void setEnvironmentFns(const EnvironmentFns&);
LAYEREDITOR_PUBLIC void setFileSystemFns(const FileSystemFns&);
LAYEREDITOR_PUBLIC void setSerializationFns(const SerializationFns&);
LAYEREDITOR_PUBLIC void setLayerEditorDCCFunctions(const LayerEditorDCCFunctions&);
LAYEREDITOR_PUBLIC const LayerEditorDCCFunctions& layerEditorDCCFunctions();

// Accessor free functions — callers never null-check; an unset std::function
// yields the documented default (false / empty / no-op, except
// isDccObjectSharedStage which defaults to true).
LAYEREDITOR_PUBLIC void        saveComponent(const PXR_NS::UsdStageRefPtr&, const std::string&);
LAYEREDITOR_PUBLIC void        reloadComponent(const std::string&);
LAYEREDITOR_PUBLIC bool        isStageAComponent(const std::string&);
LAYEREDITOR_PUBLIC bool        isUnsavedComponent(const PXR_NS::UsdStageRefPtr&);
LAYEREDITOR_PUBLIC bool        shouldDisplayComponentInitialSaveDialog(
           const PXR_NS::UsdStageRefPtr&,
           const std::string&);
LAYEREDITOR_PUBLIC std::string sceneFolder();
LAYEREDITOR_PUBLIC std::string
moveComponent(const std::string&, const std::string&, const std::string&);
LAYEREDITOR_PUBLIC std::string
previewComponentSave(const std::string&, const std::string&, const std::string&);
LAYEREDITOR_PUBLIC std::vector<std::string> getComponentLayersToSave(const std::string&);

LAYEREDITOR_PUBLIC bool supportsEditForwarding();
LAYEREDITOR_PUBLIC bool echoEditForwarding();
LAYEREDITOR_PUBLIC void setEchoEditForwarding(bool);
LAYEREDITOR_PUBLIC void openEditForwardDialog(const PXR_NS::UsdStageRefPtr&);
LAYEREDITOR_PUBLIC bool handleEFEditTargetUpdate(const PXR_NS::UsdStageRefPtr&);
LAYEREDITOR_PUBLIC bool isEditForwardDialogOpen();

LAYEREDITOR_PUBLIC bool isDccObjectStageIncoming(const std::string&);
LAYEREDITOR_PUBLIC bool isDccObjectSharedStage(const std::string&);
LAYEREDITOR_PUBLIC std::string renameObject(const std::string&, const std::string&);

// SaveOptionFns
LAYEREDITOR_PUBLIC bool        requireUsdPathsRelativeToSceneFile();
LAYEREDITOR_PUBLIC bool        requireUsdPathsRelativeToParentLayer();
LAYEREDITOR_PUBLIC bool        requireUsdPathsRelativeToEditTargetLayer();
LAYEREDITOR_PUBLIC bool        wantReferenceCompositionArc();
LAYEREDITOR_PUBLIC bool        wantPrependCompositionArc();
LAYEREDITOR_PUBLIC bool        wantPayloadLoaded();
LAYEREDITOR_PUBLIC std::string getReferencedPrimPath();
LAYEREDITOR_PUBLIC void        setRequireUsdPathsRelativeToSceneFile(bool);
LAYEREDITOR_PUBLIC void        setRequireUsdPathsRelativeToParentLayer(bool);
LAYEREDITOR_PUBLIC bool        confirmExistingFileSave();
LAYEREDITOR_PUBLIC bool        getSaveLayerFormatBinary();
LAYEREDITOR_PUBLIC void        setSaveLayerFormatBinary(bool);
LAYEREDITOR_PUBLIC int         getSerializedUsdEditsLocation();
LAYEREDITOR_PUBLIC void        setSerializedUsdEditsLocation(int);
// EnvironmentFns
LAYEREDITOR_PUBLIC bool        getPinLayerEditorStage();
LAYEREDITOR_PUBLIC void        setPinLayerEditorStage(bool);
LAYEREDITOR_PUBLIC bool        isInteractiveDCCSession();
LAYEREDITOR_PUBLIC bool        shouldExpandOrCollapseAll();
LAYEREDITOR_PUBLIC int64_t     getLayerContentsArraySizeLimit();
LAYEREDITOR_PUBLIC int64_t     getLayerContentsTimeSamplesSizeLimit();
LAYEREDITOR_PUBLIC void        displayError(const std::string&);

// FileSystemFns
LAYEREDITOR_PUBLIC std::string getDCCSceneDir();
LAYEREDITOR_PUBLIC std::string getDCCWorkspaceScenesDir();
LAYEREDITOR_PUBLIC bool        prepareLayerSaveUILayer(const std::string& relativeAnchor);
LAYEREDITOR_PUBLIC bool        checkWriteAccess(const std::string& filePath);

// SerializationFns
LAYEREDITOR_PUBLIC std::vector<PXR_NS::UsdStageCache*> getStageCaches();
LAYEREDITOR_PUBLIC std::vector<PXR_NS::UsdStageRefPtr> getAllStages();
LAYEREDITOR_PUBLIC void setLayerUpAxisAndUnits(const PXR_NS::SdfLayerRefPtr& layer);
// Writes the saved root-layer path back onto the DCC object. pathMode selects whether to honor the
// proxy/option-var relative-vs-absolute preference (FollowPreference, the normal save flow) or to
// force an absolute path (ForceAbsolute, used after a component move repaths the root).
LAYEREDITOR_PUBLIC void updateDCCObjectRootLayer(
    const std::string&            dccObjectPath,
    const std::string&            layerPath,
    const PXR_NS::SdfLayerRefPtr& layer,
    bool                          wasTargetLayer,
    DccObjectRootLayerPathMode    pathMode = DccObjectRootLayerPathMode::FollowPreference);
LAYEREDITOR_PUBLIC PXR_NS::SdfLayerRefPtr captureSessionLayer(const std::string& dccObjectPath);
LAYEREDITOR_PUBLIC void transferSessionLayer(
    const PXR_NS::SdfLayerRefPtr& sourceSessionLayer,
    const std::string&            dstDccObjectPath);

} // namespace UsdLayerEditor

#endif // LAYER_EDITOR_DCC_FUNCTIONS_H
