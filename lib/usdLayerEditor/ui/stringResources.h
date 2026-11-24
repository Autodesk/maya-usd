//
// Copyright 2020 Autodesk
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
#ifndef USDLAYEREDITOR_STRING_RESOURCES_H
#define USDLAYEREDITOR_STRING_RESOURCES_H

#include <stringResourceBase.h>
#include <usdLayerEditor/lib/layerEditorAPI.h>

#include <string>

class QString;

namespace UsdLayerEditor {
namespace StringResources {

using StringResourceBase::Resource;
using StringResourceBase::ResourceMap;

// Register all UsdLayerEditor strings with the DCC.
LAYEREDITOR_UI_PUBLIC bool registerStrings();

// Retrieve a string resource, translated for the running DCC.
LAYEREDITOR_UI_PUBLIC QString getAsQString(const Resource& strResID);
LAYEREDITOR_UI_PUBLIC std::string getAsString(const Resource& strResID);

// Every resource below, keyed by Resource::key; a DCC walks this to populate its own catalog.
LAYEREDITOR_UI_PUBLIC const ResourceMap& allResources();

// Shorthands taking the bare resource name, e.g. GET_LE_QSTR(kCancel).
#define GET_LE_QSTR(key) \
    UsdLayerEditor::StringResources::getAsQString(UsdLayerEditor::StringResources::key)
#define GET_LE_STR(key) \
    UsdLayerEditor::StringResources::getAsString(UsdLayerEditor::StringResources::key)

// -------------------------------------------------------------

// clang-format off

// The list of all resources. Add new strings here only: the variable name matches its key.
// These are defaults; a DCC that registers the key in its own catalog overrides what is shown.
//
// Each is declared extern below and defined once in stringResources.cpp, so a Resource is built
// once per string rather than once per translation unit including this header (C++14: no inline
// variables).

#define USDLAYEREDITOR_STRING_LIST(X) \
    X(kAddNewLayer, "Add a New Layer") \
    X(kAddSublayer, "Add sublayer") \
    X(kAutoHideSessionLayer, "Auto-Hide Session Layer") \
    X(kDisplayLayerContents, "Display Layer Content") \
    X(kDisplayLayerContentsEmpty, "Select a single layer to display the contents.\n\nLarge layers may take longer to load.") \
    X(kEditForwardingTooltipEnabled, "Edit Forwarding (enabled)\n" \
                                     "When enabled, forwards scene edits to layers by rule. Unmatched edits go to the current target layer.\n" \
                                     "When disabled, all edits go to the current target layer.\n\n" \
                                     "Click to open configuration.") \
    X(kEditForwardingTooltipDisabled, "Edit Forwarding (disabled)\n" \
                                      "When enabled, forwards scene edits to layers by rule. Unmatched edits go to the current target layer.\n" \
                                      "When disabled, all edits go to the current target layer.\n\n" \
                                      "Click to open configuration.") \
    X(kEchoEditForwarding, "Echo Edit Forwarding in Script Editor") \
    X(kConfigureEditForwarding, "Edit Forwarding Configuration") \
    X(kRenderSetupTooltipActive, "USD Render Setup (active render layer)\n" \
                                 "When a render layer is active, it appears under the session layer as the strongest layer. All scene edits go to this layer. Other layers can't be targeted.\n\n" \
                                 "Click to open USD Render Setup") \
    X(kRenderSetupTooltipInactive, "USD Render Setup (no active render layer)\n" \
                                   "When a render layer is active, it appears under the session layer as the strongest layer. All scene edits go to this layer. Other layers can't be targeted.\n\n" \
                                   "Click to open USD Render Setup") \
    X(kTargetLockedByRenderLayerTooltip, "A render layer is active. Layers cannot be targeted while it is.") \
    X(kDisplayLayerExpandAllValues, "Expand All Values") \
    X(kDisplayLayerExpandAllValuesTooltip, "Enable to display all array values and timeSamples in the layer content") \
    X(kConvertToRelativePath, "Convert to Relative Path") \
    X(kCancel, "Cancel") \
    X(kCreate, "Create") \
    X(kReloadTitle, "Reload \"^1s\"") \
    X(kReloadMsg, "Reloading \"^1s\" will discard all current edits and revert it to its state on disk. Are you sure you want to proceed?") \
    X(kReloadButtonText, "Reload") \
    X(kHelp, "Help") \
    X(kHelpOnUSDLayerEditor, "Help on USD Layer Editor") \
    X(kLoadExistingLayer, "Load an Existing Layer") \
    X(kLoadSublayersError, "Load Sublayers Error") \
    X(kLoadSublayersTo, "Load Sublayers to ^1s") \
    X(kLoadSublayers, "Load Sublayers") \
    X(kLayerPath, "Layer Path:") \
    X(kMuteUnmuteLayer, "Mute/unmute the layer. Muted layers are ignored by the stage.") \
    X(kLockUnlockLayer, "Lock/unlock the layer. A locked layer cannot be edited.") \
    X(kLayerIsSystemLocked, "This layer has been locked by the system or administrator and is not editable.\nContact your TD for access.") \
    X(kNoLayers, "No Layers") \
    X(kNotUndoable, "You can not undo this action.") \
    X(kOption, "Option") \
    X(kPathNotFound, "Path not found: ") \
    X(kRemoveSublayer, "Remove sublayer") \
    X(kMenuStitchLayers, "Merge Layers") \
    X(kSave, "Save") \
    X(kSaveAll, "Save All") \
    X(kSaveAllEditsInLayerStack, "Save all edits in the Layer Stack") \
    X(kSaveLayer, "Save Layer") \
    X(kSaveName, "Save ^1s") \
    X(kSaveLayerSaveNestedAnonymLayer, "To save ^1s, you must save your ^2s anonymous layer(s) that are nested under it.") \
    X(kSaveLayerWarnTitle, "Save ^1s") \
    X(kSaveLayerWarnMsg, "Saving edits to ^1s will overwrite your file.") \
    X(kSaveStage, "Save Stage") \
    X(kSaveStages, "Save Stage(s)") \
    X(kSaveStagesAndExport, "Save Stage(s) and Export") \
    X(kSaveXStages, "Save ^1s Stage(s)") \
    X(kToSaveTheStageSaveAnonym, "To save the ^1s stage(s), save the following ^2s anonymous layer(s).") \
    X(kToSaveTheStageSaveFiles, "To save the ^1s stage(s), the following existing file(s) will be overwritten.") \
    X(kToExportTheStageSaveAnonym, "To export the ^1s stage(s), save the following ^2s anonymous layer(s).") \
    X(kToExportTheStageSaveFiles, "To export the ^1s stage(s), the following existing file(s) will be overwritten.") \
    X(kToSaveTheStageSaveComponents, "To save the ^1s stage(s), save the following component(s).") \
    X(kToExportTheStageSaveComponents, "To export the ^1s stage(s), save the following component(s).") \
    X(kUsedInStagesTooltip, "<b>Used in Stages</b>: ") \
    X(kSetLayerAsTargetLayerTooltip, "Set layer as target layer. Edits are added to the target layer.") \
    X(kUsdStage, "USD Stage:") \
    X(kPinUsdStageTooltip, "Pin the current stage to keep your Layer Editor view fixed while you select different stages.") \
    X(kSaveAnonymousLayersErrorsTitle, "Save All Layers Error") \
    X(kSaveAnonymousLayersErrorsMsg, "Errors were encountered while saving layers. Check Script Editor for details.") \
    X(kSaveAnonymousLayersErrors, "Layer ^1s could not be saved to: ^2s") \
    X(kSaveAnonymousConfirmOverwriteTitle, "Confirm Overwrite") \
    X(kSaveAnonymousConfirmOverwrite, "^1s file(s) already exist and will be overwritten. Do you want to continue?") \
    X(kSaveAnonymousIdenticalFilesTitle, "Identical Names Error") \
    X(kSaveAnonymousIdenticalFiles, "^1s files have identical file names and prevent correct saving. Please select different file names for each layer. Here is the list of identical file names:<br>") \
    X(kBatchSaveAllRelative, "Convert All to Relative Paths") \
    X(kBatchSaveRelativeToScene, "Relative to Scene File") \
    X(kBatchSaveRelativeToParent, "Relative to Parent Layer") \
    X(kBatchSaveRelativeToLayerTooltip, "This path will be relative to ^1s") \
    X(kBatchSaveRelativeToSceneTooltip, "This path will be relative to the saved scene file") \
    /* ------------------------------------------------------------- */ \
    /* Layer context menu */ \
    /* Reuses the keys the old editor's MEL menu registered, so Maya's translations still apply. */ \
    /* ------------------------------------------------------------- */ \
    X(kMenuRemove, "Remove") \
    X(kMenuSaveAs, "Save As...") \
    X(kMenuSaveEdits, "Save Edits") \
    X(kMenuReload, "Reload") \
    X(kMenuAddSublayer, "Add Sublayer") \
    X(kMenuAddParentLayer, "Add Parent Layer") \
    X(kMenuLoadSublayers, "Load Sublayers...") \
    X(kMenuMergeWithSublayers, "Merge with Sublayers") \
    X(kMenuMute, "Mute") \
    X(kMenuUnmute, "Unmute") \
    X(kMenuLock, "Lock") \
    X(kMenuUnlock, "Unlock") \
    X(kMenuLockLayerAndSublayers, "Lock Layer and Sublayers") \
    X(kMenuUnlockLayerAndSublayers, "Unlock Layer and Sublayers") \
    X(kMenuPrintToScriptEditor, "Print to Script Editor") \
    X(kMenuPrintToListener, "Print to Listener") \
    X(kMenuSelectPrimsWithSpec, "Select Prims With Spec") \
    X(kMenuClear, "Clear") \
    /* ------------------------------------------------------------- */ \
    /* Component save widget */ \
    /* ------------------------------------------------------------- */ \
    X(kComponentName, "Name") \
    X(kComponentLocation, "Location") \
    X(kComponentBrowseForFolder, "Browse for folder") \
    X(kSelectFolder, "Select Folder") \
    X(kComponentFileStructure, "The following file structure is created on save.") \
    X(kComponentNothingToPreview, "Nothing to preview since no information about the template is available.") \
    /* ------------------------------------------------------------- */ \
    /* Save-layer file dialog */ \
    /* Used only by the Qt fallback; Maya replaces the whole dialog via browseForLayerSavePath. */ \
    /* ------------------------------------------------------------- */ \
    X(kSaveUsdFileDialogTitle, "Save Universal Scene Description (USD) File") \
    X(kSaveUsdFileDialogFilter, "USD (*.usd;*.usda;*.usdc)") \
    /* ------------------------------------------------------------- */ \
    /* Errors */ \
    /* ------------------------------------------------------------- */ \
    X(kErrorCannotAddPathInHierarchy, "Cannot add path \"^1s\" again in the layer hierarchy") \
    X(kErrorCannotAddPathInHierarchyThrough, "Cannot add path \"^1s\" again in the layer hierarchy through \"^2s\"") \
    X(kErrorCannotAddPathTwice, "Cannot add path \"^1s\" twice to the layer stack") \
    X(kErrorFailedToSaveFile, "Failed to save file to \"^1s\"") \
    X(kErrorRecursionDetected, "Recursion detected. Found \"^1s\" multiple times.\n" \
                               "Only added the first instance to the tree view.") \
    X(kErrorDidNotFind, "USD Layer Editor: did not find \"^1s\"\n") \
    X(kErrorFailedToReloadLayer, "Failed to Reload Layer")

// Only valid inside namespace StringResources; undef to avoid polluting the global namespace.
#define DECLARE_LE_STR(key, value) LAYEREDITOR_UI_PUBLIC extern const Resource key;
USDLAYEREDITOR_STRING_LIST(DECLARE_LE_STR)
#undef DECLARE_LE_STR

// clang-format on

} // namespace StringResources
} // namespace UsdLayerEditor

#endif // USDLAYEREDITOR_STRING_RESOURCES_H
