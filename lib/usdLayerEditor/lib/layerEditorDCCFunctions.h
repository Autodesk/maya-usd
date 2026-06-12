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

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class QWidget;

namespace UsdLayerEditor {

// std::function typedefs use the EXACT signatures of the former base-class overrides.
using SaveComponentFn    = std::function<void(const PXR_NS::UsdStageRefPtr&, const std::string&)>;
using ReloadComponentFn  = std::function<void(const std::string&)>;
// Returns the new DCC object path of the renamed proxy (empty if no rename happened).
using RenameProxyShapeFn = std::function<std::string(const std::string&, const std::string&)>;
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
using SetProxyRootLayerPathFn
    = std::function<void(const std::string&, const std::string&, const PXR_NS::SdfLayerRefPtr&)>;

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

struct ComponentFns
{
    SaveComponentFn                           saveComponent;
    ReloadComponentFn                         reloadComponent;
    RenameProxyShapeFn                        renameProxyShape;
    IsStageAComponentFn                       isStageAComponent;
    IsUnsavedComponentFn                      isUnsavedComponent;
    ShouldDisplayComponentInitialSaveDialogFn shouldDisplayComponentInitialSaveDialog;
    SceneFolderFn                             sceneFolder;
    MoveComponentFn                           moveComponent;
    PreviewComponentSaveFn                    previewComponentSave;
    GetComponentLayersToSaveFn                getComponentLayersToSave;
    std::function<void(const std::string&)>   displayError; // no-op when unset
    CaptureSessionLayerFn                     captureSessionLayer;   // returns null when unset
    TransferSessionLayerFn                    transferSessionLayer;  // no-op when unset
    SetProxyRootLayerPathFn                   setProxyRootLayerPath; // no-op when unset
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
};

struct SaveOptionFns
{
    std::function<bool()>        requireUsdPathsRelativeToSceneFile;        // default true
    std::function<bool()>        requireUsdPathsRelativeToParentLayer;      // default true
    std::function<bool()>        requireUsdPathsRelativeToEditTargetLayer;  // default true
    std::function<bool()>        wantReferenceCompositionArc;               // default false
    std::function<bool()>        wantPrependCompositionArc;                 // default true
    std::function<bool()>        wantPayloadLoaded;                         // default true
    std::function<std::string()> getReferencedPrimPath;                     // default ""
    std::function<void(bool)>    setRequireUsdPathsRelativeToSceneFile;
    std::function<void(bool)>    setRequireUsdPathsRelativeToParentLayer;
    std::function<bool()>        confirmExistingFileSave;                   // default true
    std::function<bool()>        getSaveLayerFormatBinary;                  // default true
    std::function<void(bool)>    setSaveLayerFormatBinary;
    std::function<int()>         getSerializedUsdEditsLocation;             // default 1 (kSaveToUSDFiles)
    std::function<void(int)>     setSerializedUsdEditsLocation;
};

struct EnvironmentFns
{
    std::function<bool()>     getPinLayerEditorStage;     // default false
    std::function<void(bool)> setPinLayerEditorStage;
    std::function<bool()>     isInteractiveDCCSession;    // default true
    std::function<bool()>     shouldExpandOrCollapseAll;  // default false
    std::function<QWidget*()> mainWindowParent;           // default nullptr
    std::function<int64_t()>  layerContentsArraySizeLimit;       // default 8
    std::function<int64_t()>  layerContentsTimeSamplesSizeLimit; // default 8
};

struct LayerEditorDCCFunctions
{
    ComponentFns      component;
    EditForwardingFns editForwarding;
    DccObjectFns      dccObject;
    SaveOptionFns     saveOption;
    EnvironmentFns    environment;
};

// Registration API — per-group setters (play cleanly with #ifdef guards), plus a
// full-struct setter and a getter used by the test RAII helper.
LayerEditorAPI void setComponentFns(const ComponentFns&);
LayerEditorAPI void setEditForwardingFns(const EditForwardingFns&);
LayerEditorAPI void setDccObjectFns(const DccObjectFns&);
LayerEditorAPI void setSaveOptionFns(const SaveOptionFns&);
LayerEditorAPI void setEnvironmentFns(const EnvironmentFns&);
LayerEditorAPI void setLayerEditorDCCFunctions(const LayerEditorDCCFunctions&);
LayerEditorAPI const LayerEditorDCCFunctions& layerEditorDCCFunctions();

// Accessor free functions — callers never null-check; an unset std::function
// yields the documented default (false / empty / no-op, except
// isDccObjectSharedStage which defaults to true).
LayerEditorAPI void        saveComponent(const PXR_NS::UsdStageRefPtr&, const std::string&);
LayerEditorAPI void        reloadComponent(const std::string&);
LayerEditorAPI std::string renameProxyShape(const std::string&, const std::string&);
LayerEditorAPI bool        isStageAComponent(const std::string&);
LayerEditorAPI bool        isUnsavedComponent(const PXR_NS::UsdStageRefPtr&);
LayerEditorAPI bool        shouldDisplayComponentInitialSaveDialog(
           const PXR_NS::UsdStageRefPtr&,
           const std::string&);
LayerEditorAPI std::string sceneFolder();
LayerEditorAPI std::string
moveComponent(const std::string&, const std::string&, const std::string&);
LayerEditorAPI std::string
previewComponentSave(const std::string&, const std::string&, const std::string&);
LayerEditorAPI std::vector<std::string> getComponentLayersToSave(const std::string&);
LayerEditorAPI void                     displayError(const std::string&);
LayerEditorAPI PXR_NS::SdfLayerRefPtr captureSessionLayer(const std::string& dccObjectPath);
LayerEditorAPI void transferSessionLayer(
    const PXR_NS::SdfLayerRefPtr& sourceSessionLayer,
    const std::string&            dstDccObjectPath);
LayerEditorAPI void setProxyRootLayerPath(
    const std::string&            dccObjectPath,
    const std::string&            rootLayerPath,
    const PXR_NS::SdfLayerRefPtr& rootLayer);

LayerEditorAPI bool supportsEditForwarding();
LayerEditorAPI bool echoEditForwarding();
LayerEditorAPI void setEchoEditForwarding(bool);
LayerEditorAPI void openEditForwardDialog(const PXR_NS::UsdStageRefPtr&);
LayerEditorAPI bool handleEFEditTargetUpdate(const PXR_NS::UsdStageRefPtr&);
LayerEditorAPI bool isEditForwardDialogOpen();

LayerEditorAPI bool isDccObjectStageIncoming(const std::string&);
LayerEditorAPI bool isDccObjectSharedStage(const std::string&);

// SaveOptionFns
LayerEditorAPI bool        requireUsdPathsRelativeToSceneFile();
LayerEditorAPI bool        requireUsdPathsRelativeToParentLayer();
LayerEditorAPI bool        requireUsdPathsRelativeToEditTargetLayer();
LayerEditorAPI bool        wantReferenceCompositionArc();
LayerEditorAPI bool        wantPrependCompositionArc();
LayerEditorAPI bool        wantPayloadLoaded();
LayerEditorAPI std::string getReferencedPrimPath();
LayerEditorAPI void        setRequireUsdPathsRelativeToSceneFile(bool);
LayerEditorAPI void        setRequireUsdPathsRelativeToParentLayer(bool);
LayerEditorAPI bool        confirmExistingFileSave();
LayerEditorAPI bool        getSaveLayerFormatBinary();
LayerEditorAPI void        setSaveLayerFormatBinary(bool);
LayerEditorAPI int         getSerializedUsdEditsLocation();
LayerEditorAPI void        setSerializedUsdEditsLocation(int);
// EnvironmentFns
LayerEditorAPI bool        getPinLayerEditorStage();
LayerEditorAPI void        setPinLayerEditorStage(bool);
LayerEditorAPI bool        isInteractiveDCCSession();
LayerEditorAPI bool        shouldExpandOrCollapseAll();
LayerEditorAPI QWidget*    mainWindowParent();
LayerEditorAPI int64_t     layerContentsArraySizeLimit();
LayerEditorAPI int64_t     layerContentsTimeSamplesSizeLimit();

} // namespace UsdLayerEditor

#endif // LAYER_EDITOR_DCC_FUNCTIONS_H
