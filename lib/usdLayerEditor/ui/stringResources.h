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

// Shorthands taking the bare resource name, e.g. GET_LE_QSTR(kCancel).
#define GET_LE_QSTR(key) \
    UsdLayerEditor::StringResources::getAsQString(UsdLayerEditor::StringResources::key)
#define GET_LE_STR(key) \
    UsdLayerEditor::StringResources::getAsString(UsdLayerEditor::StringResources::key)

// Creates a resource keyed by the stringified name; only valid inside namespace StringResources.
#define CREATE_LE_STR(key, value) \
    create("UsdLayerEditor", #key, QT_TRANSLATE_NOOP("UsdLayerEditor", value))

// -------------------------------------------------------------

// QT_TRANSLATE_NOOP marks values for lupdate and expands to the bare literal, leaving the lookup
// to getAsQString().
//
// These are defaults; a DCC that registers the key in its own catalog overrides what is shown.

// clang-format off

const auto kAddNewLayer                  { CREATE_LE_STR(kAddNewLayer, "Add a New Layer") };
const auto kAddSublayer                  { CREATE_LE_STR(kAddSublayer, "Add sublayer") };
const auto kAutoHideSessionLayer         { CREATE_LE_STR(kAutoHideSessionLayer, "Auto-Hide Session Layer") };
const auto kDisplayLayerContents         { CREATE_LE_STR(kDisplayLayerContents, "Display Layer Content") };
const auto kDisplayLayerContentsEmpty    { CREATE_LE_STR(kDisplayLayerContentsEmpty, "Select a single layer to display the contents.\n\nLarge layers may take longer to load.") };
const auto kEditForwardingTooltipEnabled { CREATE_LE_STR(kEditForwardingTooltipEnabled, "Edit Forwarding (enabled)\n"
                                                  "When enabled, forwards scene edits to layers by rule. Unmatched edits go to the current target layer.\n"
                                                  "When disabled, all edits go to the current target layer.\n\n"
                                                  "Click to open configuration.") };
const auto kEditForwardingTooltipDisabled { CREATE_LE_STR(kEditForwardingTooltipDisabled, "Edit Forwarding (disabled)\n"
                                                   "When enabled, forwards scene edits to layers by rule. Unmatched edits go to the current target layer.\n"
                                                   "When disabled, all edits go to the current target layer.\n\n"
                                                   "Click to open configuration.") };
const auto kEchoEditForwarding           { CREATE_LE_STR(kEchoEditForwarding, "Echo Edit Forwarding in Script Editor") };
const auto kConfigureEditForwarding      { CREATE_LE_STR(kConfigureEditForwarding, "Edit Forwarding Configuration") };
const auto kConfigureEditForwardingTitle { CREATE_LE_STR(kConfigureEditForwardingTitle, "USD Edit Forwarding Configuration") };
const auto kRenderSetupTooltipActive     { CREATE_LE_STR(kRenderSetupTooltipActive, "USD Render Setup (active render layer)\n"
                                                  "When a render layer is active, it appears under the session layer as the strongest layer. All scene edits go to this layer. Other layers can't be targeted.\n\n"
                                                  "Click to open USD Render Setup") };
const auto kRenderSetupTooltipInactive   { CREATE_LE_STR(kRenderSetupTooltipInactive, "USD Render Setup (no active render layer)\n"
                                                  "When a render layer is active, it appears under the session layer as the strongest layer. All scene edits go to this layer. Other layers can't be targeted.\n\n"
                                                  "Click to open USD Render Setup") };
const auto kTargetLockedByRenderLayerTooltip { CREATE_LE_STR(kTargetLockedByRenderLayerTooltip, "A render layer is active. Only the render layer can be targeted.") };
const auto kDisplayLayerExpandAllValues  { CREATE_LE_STR(kDisplayLayerExpandAllValues, "Expand All Values") };
const auto kDisplayLayerExpandAllValuesTooltip { CREATE_LE_STR(kDisplayLayerExpandAllValuesTooltip, "Enable to display all array values and timeSamples in the layer content") };
const auto kConvertToRelativePath        { CREATE_LE_STR(kConvertToRelativePath, "Convert to Relative Path") };
const auto kCancel                       { CREATE_LE_STR(kCancel, "Cancel") };
const auto kCreate                       { CREATE_LE_STR(kCreate, "Create") };
const auto kReloadTitle                  { CREATE_LE_STR(kReloadTitle, "Reload \"^1s\"") };
const auto kReloadMsg                    { CREATE_LE_STR(kReloadMsg, "Reloading \"^1s\" will discard all current edits and revert it to its state on disk. Are you sure you want to proceed?") };
const auto kReloadButtonText             { CREATE_LE_STR(kReloadButtonText, "Reload") };
const auto kHelp                         { CREATE_LE_STR(kHelp, "Help") };
const auto kHelpOnUSDLayerEditor         { CREATE_LE_STR(kHelpOnUSDLayerEditor, "Help on USD Layer Editor") };
const auto kLoadExistingLayer            { CREATE_LE_STR(kLoadExistingLayer, "Load an Existing Layer") };
const auto kLoadSublayersError           { CREATE_LE_STR(kLoadSublayersError, "Load Sublayers Error") };
const auto kLoadSublayersTo              { CREATE_LE_STR(kLoadSublayersTo, "Load Sublayers to ^1s") };
const auto kLoadSublayers                { CREATE_LE_STR(kLoadSublayers, "Load Sublayers") };
const auto kLayerPath                    { CREATE_LE_STR(kLayerPath, "Layer Path:") };
const auto kMuteUnmuteLayer              { CREATE_LE_STR(kMuteUnmuteLayer, "Mute/unmute the layer. Muted layers are ignored by the stage.") };
const auto kLockUnlockLayer              { CREATE_LE_STR(kLockUnlockLayer, "Lock/unlock the layer. A locked layer cannot be edited.") };
const auto kLayerIsSystemLocked          { CREATE_LE_STR(kLayerIsSystemLocked, "This layer has been locked by the system or administrator and is not editable.\nContact your TD for access.") };
const auto kNoLayers                     { CREATE_LE_STR(kNoLayers, "No Layers") };
const auto kNotUndoable                  { CREATE_LE_STR(kNotUndoable, "You can not undo this action.") };
const auto kOption                       { CREATE_LE_STR(kOption, "Option") };
const auto kPathNotFound                 { CREATE_LE_STR(kPathNotFound, "Path not found: ") };
const auto kRealPath                     { CREATE_LE_STR(kRealPath, "Real Path: ^1s") };
const auto kRemoveSublayer               { CREATE_LE_STR(kRemoveSublayer, "Remove sublayer") };
const auto kMenuStitchLayers             { CREATE_LE_STR(kMenuStitchLayers, "Merge Layers") };
const auto kSave                         { CREATE_LE_STR(kSave, "Save") };
const auto kSaveAll                      { CREATE_LE_STR(kSaveAll, "Save All") };
const auto kSaveAllEditsInLayerStack     { CREATE_LE_STR(kSaveAllEditsInLayerStack, "Save all edits in the Layer Stack") };
const auto kSaveLayer                    { CREATE_LE_STR(kSaveLayer, "Save Layer") };
const auto kSaveName                     { CREATE_LE_STR(kSaveName, "Save ^1s") };
const auto kSaveLayerSaveNestedAnonymLayer { CREATE_LE_STR(kSaveLayerSaveNestedAnonymLayer, "To save ^1s, you must save your ^2s anonymous layer(s) that are nested under it.") };
const auto kSaveLayerWarnTitle           { CREATE_LE_STR(kSaveLayerWarnTitle, "Save ^1s") };
const auto kSaveLayerWarnMsg             { CREATE_LE_STR(kSaveLayerWarnMsg, "Saving edits to ^1s will overwrite your file.") };
const auto kSaveStage                    { CREATE_LE_STR(kSaveStage, "Save Stage") };
const auto kSaveStages                   { CREATE_LE_STR(kSaveStages, "Save Stage(s)") };
const auto kSaveStagesAndExport          { CREATE_LE_STR(kSaveStagesAndExport, "Save Stage(s) and Export") };
const auto kSaveXStages                  { CREATE_LE_STR(kSaveXStages, "Save ^1s Stage(s)") };
const auto kToSaveTheStageSaveAnonym     { CREATE_LE_STR(kToSaveTheStageSaveAnonym, "To save the ^1s stage(s), save the following ^2s anonymous layer(s).") };
const auto kToSaveTheStageSaveFiles      { CREATE_LE_STR(kToSaveTheStageSaveFiles, "To save the ^1s stage(s), the following existing file(s) will be overwritten.") };
const auto kToExportTheStageSaveAnonym   { CREATE_LE_STR(kToExportTheStageSaveAnonym, "To export the ^1s stage(s), save the following ^2s anonymous layer(s).") };
const auto kToExportTheStageSaveFiles    { CREATE_LE_STR(kToExportTheStageSaveFiles, "To export the ^1s stage(s), the following existing file(s) will be overwritten.") };
const auto kToSaveTheStageSaveComponents { CREATE_LE_STR(kToSaveTheStageSaveComponents, "To save the ^1s stage(s), save the following component(s).") };
const auto kToExportTheStageSaveComponents { CREATE_LE_STR(kToExportTheStageSaveComponents, "To export the ^1s stage(s), save the following component(s).") };
const auto kUsedInStagesTooltip          { CREATE_LE_STR(kUsedInStagesTooltip, "<b>Used in Stages</b>: ") };

const auto kSetLayerAsTargetLayerTooltip { CREATE_LE_STR(kSetLayerAsTargetLayerTooltip, "Set layer as target layer. Edits are added to the target layer.") };
const auto kUsdLayerIdentifier           { CREATE_LE_STR(kUsdLayerIdentifier, "USD Layer identifier: ^1s") };
const auto kUsdStage                     { CREATE_LE_STR(kUsdStage, "USD Stage:") };
const auto kPinUsdStageTooltip           { CREATE_LE_STR(kPinUsdStage, "Pin the current stage to keep your Layer Editor view fixed while you select different stages.") };

const auto kSaveAnonymousLayersErrorsTitle { CREATE_LE_STR(kSaveAnonymousLayersErrorsTitle, "Save All Layers Error") };
const auto kSaveAnonymousLayersErrorsMsg { CREATE_LE_STR(kSaveAnonymousLayersErrorsMsg, "Errors were encountered while saving layers.  Check Script Editor for details.") };
const auto kSaveAnonymousLayersErrors    { CREATE_LE_STR(kSaveAnonymousLayersErrors, "Layer ^1s could not be saved to: ^2s") };
const auto kSaveAnonymousConfirmOverwriteTitle { CREATE_LE_STR(kSaveAnonymousConfirmOverwriteTitle, "Confirm Overwrite") };
const auto kSaveAnonymousConfirmOverwrite { CREATE_LE_STR(kSaveAnonymousConfirmOverwrite, "^1s file(s) already exist and will be overwritten.  Do you want to continue?") };
const auto kSaveAnonymousIdenticalFilesTitle { CREATE_LE_STR(kSaveAnonymousIdenticalFilesTitle, "Identical Names Error") };
const auto kSaveAnonymousIdenticalFiles { CREATE_LE_STR(kSaveAnonymousIdenticalFiles, "^1s files have identical file names and prevent correct saving.  Please select different file names for each layer. Here is the list of identical file names:<br>") };

const auto kBatchSaveAllRelative { CREATE_LE_STR(kBatchSaveAllRelative, "Convert All to Relative Paths") };
const auto kBatchSaveRelativeToScene { CREATE_LE_STR(kBatchSaveRelativeToScene, "Relative to Scene File") };
const auto kBatchSaveRelativeToParent { CREATE_LE_STR(kBatchSaveRelativeToParent, "Relative to Parent Layer") };
const auto kBatchSaveRelativeToLayerTooltip { CREATE_LE_STR(kBatchSaveRelativeToLayerTooltip, "This path will be relative to ^1s") };
const auto kBatchSaveRelativeToSceneTooltip { CREATE_LE_STR(kBatchSaveRelativeToSceneTooltip, "This path will be relative to the saved scene file") };

// -------------------------------------------------------------
// Layer context menu
//
// Reuses the keys the old editor's MEL menu registered, so Maya's translations still apply.
// kMenuPrintToScriptEditor keeps the neutral "Listener" wording; Maya overrides it.
// -------------------------------------------------------------

const auto kMenuRemove                   { CREATE_LE_STR(kMenuRemove, "Remove") };
const auto kMenuSaveAs                   { CREATE_LE_STR(kMenuSaveAs, "Save As...") };
const auto kMenuSaveEdits                { CREATE_LE_STR(kMenuSaveEdits, "Save Edits") };
const auto kMenuReload                   { CREATE_LE_STR(kMenuReload, "Reload") };
const auto kMenuAddSublayer              { CREATE_LE_STR(kMenuAddSublayer, "Add Sublayer") };
const auto kMenuAddParentLayer           { CREATE_LE_STR(kMenuAddParentLayer, "Add Parent Layer") };
const auto kMenuLoadSublayers            { CREATE_LE_STR(kMenuLoadSublayers, "Load Sublayers...") };
const auto kMenuMergeWithSublayers       { CREATE_LE_STR(kMenuMergeWithSublayers, "Merge with Sublayers") };
const auto kMenuMute                     { CREATE_LE_STR(kMenuMute, "Mute") };
const auto kMenuUnmute                   { CREATE_LE_STR(kMenuUnmute, "Unmute") };
const auto kMenuLock                     { CREATE_LE_STR(kMenuLock, "Lock") };
const auto kMenuUnlock                   { CREATE_LE_STR(kMenuUnlock, "Unlock") };
const auto kMenuLockLayerAndSublayers    { CREATE_LE_STR(kMenuLockLayerAndSublayers, "Lock Layer and Sublayers") };
const auto kMenuUnlockLayerAndSublayers  { CREATE_LE_STR(kMenuUnlockLayerAndSublayers, "Unlock Layer and Sublayers") };
const auto kMenuPrintToScriptEditor      { CREATE_LE_STR(kMenuPrintToScriptEditor, "Print to Listener") };
const auto kMenuSelectPrimsWithSpec      { CREATE_LE_STR(kMenuSelectPrimsWithSpec, "Select Prims With Spec") };
const auto kMenuClear                    { CREATE_LE_STR(kMenuClear, "Clear") };

// -------------------------------------------------------------
// Component save widget
// -------------------------------------------------------------

const auto kComponentName                { CREATE_LE_STR(kComponentName, "Name") };
const auto kComponentLocation            { CREATE_LE_STR(kComponentLocation, "Location") };
const auto kComponentBrowseForFolder     { CREATE_LE_STR(kComponentBrowseForFolder, "Browse for folder") };
const auto kComponentFileStructure       { CREATE_LE_STR(kComponentFileStructure, "The following file structure is created on save.") };
const auto kComponentNothingToPreview    { CREATE_LE_STR(kComponentNothingToPreview, "Nothing to preview since no information about the template is available.") };

// -------------------------------------------------------------
// Save-layer file dialog
//
// Used only by the Qt fallback; Maya replaces the whole dialog via browseForLayerSavePath.
// -------------------------------------------------------------

const auto kSaveUsdFileDialogTitle       { CREATE_LE_STR(kSaveUsdFileDialogTitle, "Save Universal Scene Description (USD) File") };
const auto kSaveUsdFileDialogFilter      { CREATE_LE_STR(kSaveUsdFileDialogFilter, "USD (*.usd;*.usda;*.usdc)") };

// -------------------------------------------------------------
// Errors
// -------------------------------------------------------------

const auto kErrorCannotAddPathInHierarchy        { CREATE_LE_STR(kErrorCannotAddPathInHierarchy, "Cannot add path \"^1s\" again in the layer hierarchy") };
const auto kErrorCannotAddPathInHierarchyThrough { CREATE_LE_STR(kErrorCannotAddPathInHierarchyThrough, "Cannot add path \"^1s\" again in the layer hierarchy through \"^2s\"") };
const auto kErrorCannotAddPathTwice              { CREATE_LE_STR(kErrorCannotAddPathTwice, "Cannot add path \"^1s\" twice to the layer stack") };
const auto kErrorFailedToSaveFile                { CREATE_LE_STR(kErrorFailedToSaveFile, "Failed to save file to \"^1s\"") };
const auto kErrorRecursionDetected               { CREATE_LE_STR(kErrorRecursionDetected, "Recursion detected. Found \"^1s\" multiple times.\n"
                                                           "Only added the first instance to the tree view.") };
const auto kErrorDidNotFind                      { CREATE_LE_STR(kErrorDidNotFind, "USD Layer Editor: did not find \"^1s\"\n") };
const auto kErrorFailedToReloadLayer             { CREATE_LE_STR(kErrorFailedToReloadLayer, "Failed to Reload Layer") };

// clang-format on

} // namespace StringResources

} // namespace UsdLayerEditor

#endif // STRING_RESOURCES_H
