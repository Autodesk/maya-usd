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

#include "layerEditorAPI.h"

#include <QtCore/QCoreApplication>

#include <map>
#include <string>

#ifndef USDLAYEREDITOR_STRING_RESOURCES_H
#define USDLAYEREDITOR_STRING_RESOURCES_H

class QString;

namespace UsdLayerEditor {

namespace StringResources {

struct Resource
{
    std::string module;
    std::string key;
    std::string value;
};

// Retrieve a string resource, translated for the running DCC.
LAYEREDITOR_UI_PUBLIC QString getAsQString(const Resource& strResID);
LAYEREDITOR_UI_PUBLIC std::string getAsString(const Resource& strResID);

// Create a Resource and add it to the registry returned by allResources().
LAYEREDITOR_UI_PUBLIC Resource create(const char* module, const char* key, const char* value);

// Every resource below, keyed by Resource::key; a DCC walks this to populate its own catalog.
LAYEREDITOR_UI_PUBLIC const std::map<std::string, Resource>& allResources();

// -------------------------------------------------------------

// QT_TRANSLATE_NOOP marks values for lupdate and expands to the bare literal, leaving the lookup
// to getAsQString().
//
// These are defaults; a DCC that registers the key in its own catalog overrides what is shown.

// clang-format off

const auto kAddNewLayer                  { create("UsdLayerEditor", "kAddNewLayer", QT_TRANSLATE_NOOP("UsdLayerEditor", "Add a New Layer")) };
const auto kAddSublayer                  { create("UsdLayerEditor", "kAddSublayer", QT_TRANSLATE_NOOP("UsdLayerEditor", "Add sublayer")) };
const auto kAutoHideSessionLayer         { create("UsdLayerEditor", "kAutoHideSessionLayer", QT_TRANSLATE_NOOP("UsdLayerEditor", "Auto-Hide Session Layer")) };
const auto kDisplayLayerContents         { create("UsdLayerEditor", "kDisplayLayerContents", QT_TRANSLATE_NOOP("UsdLayerEditor", "Display Layer Content")) };
const auto kDisplayLayerContentsEmpty    { create("UsdLayerEditor", "kDisplayLayerContentsEmpty", QT_TRANSLATE_NOOP("UsdLayerEditor", "Select a single layer to display the contents.\n\nLarge layers may take longer to load.")) };
const auto kEditForwardingTooltipEnabled { create("UsdLayerEditor", "kEditForwardingTooltipEnabled", QT_TRANSLATE_NOOP("UsdLayerEditor", "Edit Forwarding (enabled)\n"
                                                  "When enabled, forwards scene edits to layers by rule. Unmatched edits go to the current target layer.\n"
                                                  "When disabled, all edits go to the current target layer.\n\n"
                                                  "Click to open configuration.")) };
const auto kEditForwardingTooltipDisabled { create("UsdLayerEditor", "kEditForwardingTooltipDisabled", QT_TRANSLATE_NOOP("UsdLayerEditor", "Edit Forwarding (disabled)\n"
                                                   "When enabled, forwards scene edits to layers by rule. Unmatched edits go to the current target layer.\n"
                                                   "When disabled, all edits go to the current target layer.\n\n"
                                                   "Click to open configuration.")) };
const auto kEchoEditForwarding           { create("UsdLayerEditor", "kEchoEditForwarding", QT_TRANSLATE_NOOP("UsdLayerEditor", "Echo Edit Forwarding in Script Editor")) };
const auto kConfigureEditForwarding      { create("UsdLayerEditor", "kConfigureEditForwarding", QT_TRANSLATE_NOOP("UsdLayerEditor", "Edit Forwarding Configuration")) };
const auto kConfigureEditForwardingTitle { create("UsdLayerEditor", "kConfigureEditForwardingTitle", QT_TRANSLATE_NOOP("UsdLayerEditor", "USD Edit Forwarding Configuration")) };
const auto kDisplayLayerExpandAllValues  { create("UsdLayerEditor", "kDisplayLayerExpandAllValues", QT_TRANSLATE_NOOP("UsdLayerEditor", "Expand All Values")) };
const auto kDisplayLayerExpandAllValuesTooltip { create("UsdLayerEditor", "kDisplayLayerExpandAllValuesTooltip", QT_TRANSLATE_NOOP("UsdLayerEditor", "Enable to display all array values and timeSamples in the layer content")) };
const auto kConvertToRelativePath        { create("UsdLayerEditor", "kConvertToRelativePath", QT_TRANSLATE_NOOP("UsdLayerEditor", "Convert to Relative Path")) };
const auto kCancel                       { create("UsdLayerEditor", "kCancel", QT_TRANSLATE_NOOP("UsdLayerEditor", "Cancel")) };
const auto kCreate                       { create("UsdLayerEditor", "kCreate", QT_TRANSLATE_NOOP("UsdLayerEditor", "Create")) };
const auto kReloadTitle                  { create("UsdLayerEditor", "kReloadTitle", QT_TRANSLATE_NOOP("UsdLayerEditor", "Reload \"^1s\"")) };
const auto kReloadMsg                    { create("UsdLayerEditor", "kReloadMsg", QT_TRANSLATE_NOOP("UsdLayerEditor", "Reloading \"^1s\" will discard all current edits and revert it to its state on disk. Are you sure you want to proceed?")) };
const auto kReloadButtonText             { create("UsdLayerEditor", "kReloadButtonText", QT_TRANSLATE_NOOP("UsdLayerEditor", "Reload")) };
const auto kHelp                         { create("UsdLayerEditor", "kHelp", QT_TRANSLATE_NOOP("UsdLayerEditor", "Help")) };
const auto kHelpOnUSDLayerEditor         { create("UsdLayerEditor", "kHelpOnUSDLayerEditor", QT_TRANSLATE_NOOP("UsdLayerEditor", "Help on USD Layer Editor")) };
const auto kLoadExistingLayer            { create("UsdLayerEditor", "kLoadExistingLayer", QT_TRANSLATE_NOOP("UsdLayerEditor", "Load an Existing Layer")) };
const auto kLoadSublayersError           { create("UsdLayerEditor", "kLoadSublayersError", QT_TRANSLATE_NOOP("UsdLayerEditor", "Load Sublayers Error")) };
const auto kLoadSublayersTo              { create("UsdLayerEditor", "kLoadSublayersTo", QT_TRANSLATE_NOOP("UsdLayerEditor", "Load Sublayers to ^1s")) };
const auto kLoadSublayers                { create("UsdLayerEditor", "kLoadSublayers", QT_TRANSLATE_NOOP("UsdLayerEditor", "Load Sublayers")) };
const auto kLayerPath                    { create("UsdLayerEditor", "kLayerPath", QT_TRANSLATE_NOOP("UsdLayerEditor", "Layer Path:")) };
const auto kMuteUnmuteLayer              { create("UsdLayerEditor", "kMuteUnmuteLayer", QT_TRANSLATE_NOOP("UsdLayerEditor", "Mute/unmute the layer. Muted layers are ignored by the stage."))};
const auto kLockUnlockLayer              { create("UsdLayerEditor", "kLockUnlockLayer", QT_TRANSLATE_NOOP("UsdLayerEditor", "Lock/unlock the layer. A locked layer cannot be edited."))};
const auto kLayerIsSystemLocked          { create("UsdLayerEditor", "kLayerIsSystemLocked", QT_TRANSLATE_NOOP("UsdLayerEditor", "This layer has been locked by the system or administrator and is not editable.\nContact your TD for access."))};
const auto kNoLayers                     { create("UsdLayerEditor", "kNoLayers", QT_TRANSLATE_NOOP("UsdLayerEditor", "No Layers")) };
const auto kNotUndoable                  { create("UsdLayerEditor", "kNotUndoable", QT_TRANSLATE_NOOP("UsdLayerEditor", "You can not undo this action.")) };
const auto kOption                       { create("UsdLayerEditor", "kOption", QT_TRANSLATE_NOOP("UsdLayerEditor", "Option")) };
const auto kPathNotFound                 { create("UsdLayerEditor", "kPathNotFound", QT_TRANSLATE_NOOP("UsdLayerEditor", "Path not found: ")) };
const auto kRealPath                     { create("UsdLayerEditor", "kRealPath", QT_TRANSLATE_NOOP("UsdLayerEditor", "Real Path: ^1s")) };
const auto kRemoveSublayer               { create("UsdLayerEditor", "kRemoveSublayer", QT_TRANSLATE_NOOP("UsdLayerEditor", "Remove sublayer")) };
const auto kMenuStitchLayers             { create("UsdLayerEditor", "kMenuStitchLayers", QT_TRANSLATE_NOOP("UsdLayerEditor", "Merge Layers")) };
const auto kSave                         { create("UsdLayerEditor", "kSave", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save")) };
const auto kSaveAll                      { create("UsdLayerEditor", "kSaveAll", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save All")) };
const auto kSaveAllEditsInLayerStack     { create("UsdLayerEditor", "kSaveAllEditsInLayerStack", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save all edits in the Layer Stack"))};
const auto kSaveLayer                    { create("UsdLayerEditor", "kSaveLayer", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save Layer")) };
const auto kSaveName                     { create("UsdLayerEditor", "kSaveName", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save ^1s")) };
const auto kSaveLayerSaveNestedAnonymLayer { create("UsdLayerEditor", "kSaveLayerSaveNestedAnonymLayer", QT_TRANSLATE_NOOP("UsdLayerEditor", "To save ^1s, you must save your ^2s anonymous layer(s) that are nested under it.")) };
const auto kSaveLayerWarnTitle           { create("UsdLayerEditor", "kSaveLayerWarnTitle", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save ^1s")) };
const auto kSaveLayerWarnMsg             { create("UsdLayerEditor", "kSaveLayerWarnMsg", QT_TRANSLATE_NOOP("UsdLayerEditor", "Saving edits to ^1s will overwrite your file.")) };
const auto kSaveStage                    { create("UsdLayerEditor", "kSaveStage", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save Stage")) };
const auto kSaveStages                   { create("UsdLayerEditor", "kSaveStages", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save Stage(s)")) };
const auto kSaveStagesAndExport          { create("UsdLayerEditor", "kSaveStagesAndExport", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save Stage(s) and Export")) };
const auto kSaveXStages                  { create("UsdLayerEditor", "kSaveXStages", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save ^1s Stage(s)")) };
const auto kToSaveTheStageSaveAnonym     { create("UsdLayerEditor", "kToSaveTheStageSaveAnonym", QT_TRANSLATE_NOOP("UsdLayerEditor", "To save the ^1s stage(s), save the following ^2s anonymous layer(s).")) };
const auto kToSaveTheStageSaveFiles      { create("UsdLayerEditor", "kToSaveTheStageSaveFiles", QT_TRANSLATE_NOOP("UsdLayerEditor", "To save the ^1s stage(s), the following existing file(s) will be overwritten.")) };
const auto kToExportTheStageSaveAnonym   { create("UsdLayerEditor", "kToExportTheStageSaveAnonym", QT_TRANSLATE_NOOP("UsdLayerEditor", "To export the ^1s stage(s), save the following ^2s anonymous layer(s).")) };
const auto kToExportTheStageSaveFiles    { create("UsdLayerEditor", "kToExportTheStageSaveFiles", QT_TRANSLATE_NOOP("UsdLayerEditor", "To export the ^1s stage(s), the following existing file(s) will be overwritten.")) };
const auto kToSaveTheStageSaveComponents { create("UsdLayerEditor", "kToSaveTheStageSaveComponents", QT_TRANSLATE_NOOP("UsdLayerEditor", "To save the ^1s stage(s), save the following component(s).")) };
const auto kToExportTheStageSaveComponents { create("UsdLayerEditor", "kToExportTheStageSaveComponents", QT_TRANSLATE_NOOP("UsdLayerEditor", "To export the ^1s stage(s), save the following component(s).")) };
const auto kUsedInStagesTooltip          { create("UsdLayerEditor", "kUsedInStagesTooltip", QT_TRANSLATE_NOOP("UsdLayerEditor", "<b>Used in Stages</b>: ")) };

const auto kSetLayerAsTargetLayerTooltip { create("UsdLayerEditor", "kSetLayerAsTargetLayerTooltip", QT_TRANSLATE_NOOP("UsdLayerEditor", "Set layer as target layer. Edits are added to the target layer.")) };
const auto kUsdLayerIdentifier           { create("UsdLayerEditor", "kUsdLayerIdentifier", QT_TRANSLATE_NOOP("UsdLayerEditor", "USD Layer identifier: ^1s")) };
const auto kUsdStage                     { create("UsdLayerEditor", "kUsdStage", QT_TRANSLATE_NOOP("UsdLayerEditor", "USD Stage:")) };
const auto kPinUsdStageTooltip           { create("UsdLayerEditor", "kPinUsdStage", QT_TRANSLATE_NOOP("UsdLayerEditor", "Pin the current stage to keep your Layer Editor view fixed while you select different stages.")) };

const auto kSaveAnonymousLayersErrorsTitle { create("UsdLayerEditor", "kSaveAnonymousLayersErrorsTitle", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save All Layers Error"))};
const auto kSaveAnonymousLayersErrorsMsg { create("UsdLayerEditor", "kSaveAnonymousLayersErrorsMsg", QT_TRANSLATE_NOOP("UsdLayerEditor", "Errors were encountered while saving layers.  Check Script Editor for details."))};
const auto kSaveAnonymousLayersErrors    { create("UsdLayerEditor", "kSaveAnonymousLayersErrors", QT_TRANSLATE_NOOP("UsdLayerEditor", "Layer ^1s could not be saved to: ^2s"))};
const auto kSaveAnonymousConfirmOverwriteTitle { create("UsdLayerEditor", "kSaveAnonymousConfirmOverwriteTitle", QT_TRANSLATE_NOOP("UsdLayerEditor", "Confirm Overwrite"))};
const auto kSaveAnonymousConfirmOverwrite { create("UsdLayerEditor", "kSaveAnonymousConfirmOverwrite", QT_TRANSLATE_NOOP("UsdLayerEditor", "^1s file(s) already exist and will be overwritten.  Do you want to continue?"))};
const auto kSaveAnonymousIdenticalFilesTitle { create("UsdLayerEditor", "kSaveAnonymousIdenticalFilesTitle", QT_TRANSLATE_NOOP("UsdLayerEditor", "Identical Names Error"))};
const auto kSaveAnonymousIdenticalFiles { create("UsdLayerEditor", "kSaveAnonymousIdenticalFiles", QT_TRANSLATE_NOOP("UsdLayerEditor", "^1s files have identical file names and prevent correct saving.  Please select different file names for each layer. Here is the list of identical file names:<br>"))};

const auto kBatchSaveAllRelative { create("UsdLayerEditor", "kBatchSaveAllRelative", QT_TRANSLATE_NOOP("UsdLayerEditor", "Convert All to Relative Paths"))};
const auto kBatchSaveRelativeToScene { create("UsdLayerEditor", "kBatchSaveRelativeToScene", QT_TRANSLATE_NOOP("UsdLayerEditor", "Relative to Scene File"))};
const auto kBatchSaveRelativeToParent { create("UsdLayerEditor", "kBatchSaveRelativeToParent", QT_TRANSLATE_NOOP("UsdLayerEditor", "Relative to Parent Layer"))};
const auto kBatchSaveRelativeToLayerTooltip { create("UsdLayerEditor", "kBatchSaveRelativeToLayerTooltip", QT_TRANSLATE_NOOP("UsdLayerEditor", "This path will be relative to ^1s"))};
const auto kBatchSaveRelativeToSceneTooltip { create("UsdLayerEditor", "kBatchSaveRelativeToSceneTooltip", QT_TRANSLATE_NOOP("UsdLayerEditor", "This path will be relative to the saved scene file"))};

// -------------------------------------------------------------
// Layer context menu
//
// Reuses the keys the old editor's MEL menu registered, so Maya's translations still apply.
// kMenuPrintToScriptEditor keeps the neutral "Listener" wording; Maya overrides it.
// -------------------------------------------------------------

const auto kMenuRemove                   { create("UsdLayerEditor", "kMenuRemove", QT_TRANSLATE_NOOP("UsdLayerEditor", "Remove")) };
const auto kMenuSaveAs                   { create("UsdLayerEditor", "kMenuSaveAs", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save As...")) };
const auto kMenuSaveEdits                { create("UsdLayerEditor", "kMenuSaveEdits", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save Edits")) };
const auto kMenuReload                   { create("UsdLayerEditor", "kMenuReload", QT_TRANSLATE_NOOP("UsdLayerEditor", "Reload")) };
const auto kMenuAddSublayer              { create("UsdLayerEditor", "kMenuAddSublayer", QT_TRANSLATE_NOOP("UsdLayerEditor", "Add Sublayer")) };
const auto kMenuAddParentLayer           { create("UsdLayerEditor", "kMenuAddParentLayer", QT_TRANSLATE_NOOP("UsdLayerEditor", "Add Parent Layer")) };
const auto kMenuLoadSublayers            { create("UsdLayerEditor", "kMenuLoadSublayers", QT_TRANSLATE_NOOP("UsdLayerEditor", "Load Sublayers...")) };
const auto kMenuMergeWithSublayers       { create("UsdLayerEditor", "kMenuMergeWithSublayers", QT_TRANSLATE_NOOP("UsdLayerEditor", "Merge with Sublayers")) };
const auto kMenuMute                     { create("UsdLayerEditor", "kMenuMute", QT_TRANSLATE_NOOP("UsdLayerEditor", "Mute")) };
const auto kMenuUnmute                   { create("UsdLayerEditor", "kMenuUnmute", QT_TRANSLATE_NOOP("UsdLayerEditor", "Unmute")) };
const auto kMenuLock                     { create("UsdLayerEditor", "kMenuLock", QT_TRANSLATE_NOOP("UsdLayerEditor", "Lock")) };
const auto kMenuUnlock                   { create("UsdLayerEditor", "kMenuUnlock", QT_TRANSLATE_NOOP("UsdLayerEditor", "Unlock")) };
const auto kMenuLockLayerAndSublayers    { create("UsdLayerEditor", "kMenuLockLayerAndSublayers", QT_TRANSLATE_NOOP("UsdLayerEditor", "Lock Layer and Sublayers")) };
const auto kMenuUnlockLayerAndSublayers  { create("UsdLayerEditor", "kMenuUnlockLayerAndSublayers", QT_TRANSLATE_NOOP("UsdLayerEditor", "Unlock Layer and Sublayers")) };
const auto kMenuPrintToScriptEditor      { create("UsdLayerEditor", "kMenuPrintToScriptEditor", QT_TRANSLATE_NOOP("UsdLayerEditor", "Print to Listener")) };
const auto kMenuSelectPrimsWithSpec      { create("UsdLayerEditor", "kMenuSelectPrimsWithSpec", QT_TRANSLATE_NOOP("UsdLayerEditor", "Select Prims With Spec")) };
const auto kMenuClear                    { create("UsdLayerEditor", "kMenuClear", QT_TRANSLATE_NOOP("UsdLayerEditor", "Clear")) };

// -------------------------------------------------------------
// Component save widget
// -------------------------------------------------------------

const auto kComponentName                { create("UsdLayerEditor", "kComponentName", QT_TRANSLATE_NOOP("UsdLayerEditor", "Name")) };
const auto kComponentLocation            { create("UsdLayerEditor", "kComponentLocation", QT_TRANSLATE_NOOP("UsdLayerEditor", "Location")) };
const auto kComponentBrowseForFolder     { create("UsdLayerEditor", "kComponentBrowseForFolder", QT_TRANSLATE_NOOP("UsdLayerEditor", "Browse for folder")) };
const auto kComponentFileStructure       { create("UsdLayerEditor", "kComponentFileStructure", QT_TRANSLATE_NOOP("UsdLayerEditor", "The following file structure is created on save.")) };
const auto kComponentNothingToPreview    { create("UsdLayerEditor", "kComponentNothingToPreview", QT_TRANSLATE_NOOP("UsdLayerEditor", "Nothing to preview since no information about the template is available.")) };

// -------------------------------------------------------------
// Save-layer file dialog
//
// Used only by the Qt fallback; Maya replaces the whole dialog via browseForLayerSavePath.
// -------------------------------------------------------------

const auto kSaveUsdFileDialogTitle       { create("UsdLayerEditor", "kSaveUsdFileDialogTitle", QT_TRANSLATE_NOOP("UsdLayerEditor", "Save Universal Scene Description (USD) File")) };
const auto kSaveUsdFileDialogFilter      { create("UsdLayerEditor", "kSaveUsdFileDialogFilter", QT_TRANSLATE_NOOP("UsdLayerEditor", "USD (*.usd;*.usda;*.usdc)")) };

// -------------------------------------------------------------
// Errors
// -------------------------------------------------------------

const auto kErrorCannotAddPathInHierarchy        { create("UsdLayerEditor", "kErrorCannotAddPathInHierarchy", QT_TRANSLATE_NOOP("UsdLayerEditor", "Cannot add path \"^1s\" again in the layer hierarchy")) };
const auto kErrorCannotAddPathInHierarchyThrough { create("UsdLayerEditor", "kErrorCannotAddPathInHierarchyThrough", QT_TRANSLATE_NOOP("UsdLayerEditor", "Cannot add path \"^1s\" again in the layer hierarchy through \"^2s\"")) };
const auto kErrorCannotAddPathTwice              { create("UsdLayerEditor", "kErrorCannotAddPathTwice", QT_TRANSLATE_NOOP("UsdLayerEditor", "Cannot add path \"^1s\" twice to the layer stack")) };
const auto kErrorFailedToSaveFile                { create("UsdLayerEditor", "kErrorFailedToSaveFile", QT_TRANSLATE_NOOP("UsdLayerEditor", "Failed to save file to \"^1s\"")) };
const auto kErrorRecursionDetected               { create("UsdLayerEditor", "kErrorRecursionDetected", QT_TRANSLATE_NOOP("UsdLayerEditor", "Recursion detected. Found \"^1s\" multiple times.\n"
                                                           "Only added the first instance to the tree view.")) };
const auto kErrorDidNotFind                      { create("UsdLayerEditor", "kErrorDidNotFind", QT_TRANSLATE_NOOP("UsdLayerEditor", "USD Layer Editor: did not find \"^1s\"\n")) };
const auto kErrorFailedToReloadLayer             { create("UsdLayerEditor", "kErrorFailedToReloadLayer", QT_TRANSLATE_NOOP("UsdLayerEditor", "Failed to Reload Layer")) };

// clang-format on

} // namespace StringResources

} // namespace UsdLayerEditor

#endif // STRING_RESOURCES_H
