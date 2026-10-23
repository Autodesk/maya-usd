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
//
// QT_TRANSLATE_NOOP marks values for lupdate and expands to the bare literal, leaving the lookup
// to getAsQString().
#define CREATE_LE_STR(key, value) \
    create("UsdLayerEditor", #key, QT_TRANSLATE_NOOP("UsdLayerEditor", value))

// Declares a const resource whose variable name matches its key, written only once.
// Expands to: const auto key { CREATE_LE_STR(key, value) };
#define DECLARE_LE_STR(key, value) \
    const auto key { CREATE_LE_STR(key, value) }

// -------------------------------------------------------------

// These are defaults; a DCC that registers the key in its own catalog overrides what is shown.

// clang-format off

DECLARE_LE_STR(kAddNewLayer, "Add a New Layer");
DECLARE_LE_STR(kAddSublayer, "Add sublayer");
DECLARE_LE_STR(kAutoHideSessionLayer, "Auto-Hide Session Layer");
DECLARE_LE_STR(kDisplayLayerContents, "Display Layer Content");
DECLARE_LE_STR(kDisplayLayerContentsEmpty, "Select a single layer to display the contents.\n\nLarge layers may take longer to load.");
DECLARE_LE_STR(kEditForwardingTooltipEnabled, "Edit Forwarding (enabled)\n"
                                              "When enabled, forwards scene edits to layers by rule. Unmatched edits go to the current target layer.\n"
                                              "When disabled, all edits go to the current target layer.\n\n"
                                              "Click to open configuration.");
DECLARE_LE_STR(kEditForwardingTooltipDisabled, "Edit Forwarding (disabled)\n"
                                               "When enabled, forwards scene edits to layers by rule. Unmatched edits go to the current target layer.\n"
                                               "When disabled, all edits go to the current target layer.\n\n"
                                               "Click to open configuration.");
DECLARE_LE_STR(kEchoEditForwarding, "Echo Edit Forwarding in Script Editor");
DECLARE_LE_STR(kConfigureEditForwarding, "Edit Forwarding Configuration");
DECLARE_LE_STR(kConfigureEditForwardingTitle, "USD Edit Forwarding Configuration");
DECLARE_LE_STR(kRenderSetupTooltipActive, "USD Render Setup (active render layer)\n"
                                          "When a render layer is active, it appears under the session layer as the strongest layer. All scene edits go to this layer. Other layers can't be targeted.\n\n"
                                          "Click to open USD Render Setup");
DECLARE_LE_STR(kRenderSetupTooltipInactive, "USD Render Setup (no active render layer)\n"
                                            "When a render layer is active, it appears under the session layer as the strongest layer. All scene edits go to this layer. Other layers can't be targeted.\n\n"
                                            "Click to open USD Render Setup");
DECLARE_LE_STR(kTargetLockedByRenderLayerTooltip, "A render layer is active. Layers cannot be targeted while it is.");
DECLARE_LE_STR(kDisplayLayerExpandAllValues, "Expand All Values");
DECLARE_LE_STR(kDisplayLayerExpandAllValuesTooltip, "Enable to display all array values and timeSamples in the layer content");
DECLARE_LE_STR(kConvertToRelativePath, "Convert to Relative Path");
DECLARE_LE_STR(kCancel, "Cancel");
DECLARE_LE_STR(kCreate, "Create");
DECLARE_LE_STR(kReloadTitle, "Reload \"^1s\"");
DECLARE_LE_STR(kReloadMsg, "Reloading \"^1s\" will discard all current edits and revert it to its state on disk. Are you sure you want to proceed?");
DECLARE_LE_STR(kReloadButtonText, "Reload");
DECLARE_LE_STR(kHelp, "Help");
DECLARE_LE_STR(kHelpOnUSDLayerEditor, "Help on USD Layer Editor");
DECLARE_LE_STR(kLoadExistingLayer, "Load an Existing Layer");
DECLARE_LE_STR(kLoadSublayersError, "Load Sublayers Error");
DECLARE_LE_STR(kLoadSublayersTo, "Load Sublayers to ^1s");
DECLARE_LE_STR(kLoadSublayers, "Load Sublayers");
DECLARE_LE_STR(kLayerPath, "Layer Path:");
DECLARE_LE_STR(kMuteUnmuteLayer, "Mute/unmute the layer. Muted layers are ignored by the stage.");
DECLARE_LE_STR(kLockUnlockLayer, "Lock/unlock the layer. A locked layer cannot be edited.");
DECLARE_LE_STR(kLayerIsSystemLocked, "This layer has been locked by the system or administrator and is not editable.\nContact your TD for access.");
DECLARE_LE_STR(kNoLayers, "No Layers");
DECLARE_LE_STR(kNotUndoable, "You can not undo this action.");
DECLARE_LE_STR(kOption, "Option");
DECLARE_LE_STR(kPathNotFound, "Path not found: ");
DECLARE_LE_STR(kRealPath, "Real Path: ^1s");
DECLARE_LE_STR(kRemoveSublayer, "Remove sublayer");
DECLARE_LE_STR(kMenuStitchLayers, "Merge Layers");
DECLARE_LE_STR(kSave, "Save");
DECLARE_LE_STR(kSaveAll, "Save All");
DECLARE_LE_STR(kSaveAllEditsInLayerStack, "Save all edits in the Layer Stack");
DECLARE_LE_STR(kSaveLayer, "Save Layer");
DECLARE_LE_STR(kSaveName, "Save ^1s");
DECLARE_LE_STR(kSaveLayerSaveNestedAnonymLayer, "To save ^1s, you must save your ^2s anonymous layer(s) that are nested under it.");
DECLARE_LE_STR(kSaveLayerWarnTitle, "Save ^1s");
DECLARE_LE_STR(kSaveLayerWarnMsg, "Saving edits to ^1s will overwrite your file.");
DECLARE_LE_STR(kSaveStage, "Save Stage");
DECLARE_LE_STR(kSaveStages, "Save Stage(s)");
DECLARE_LE_STR(kSaveStagesAndExport, "Save Stage(s) and Export");
DECLARE_LE_STR(kSaveXStages, "Save ^1s Stage(s)");
DECLARE_LE_STR(kToSaveTheStageSaveAnonym, "To save the ^1s stage(s), save the following ^2s anonymous layer(s).");
DECLARE_LE_STR(kToSaveTheStageSaveFiles, "To save the ^1s stage(s), the following existing file(s) will be overwritten.");
DECLARE_LE_STR(kToExportTheStageSaveAnonym, "To export the ^1s stage(s), save the following ^2s anonymous layer(s).");
DECLARE_LE_STR(kToExportTheStageSaveFiles, "To export the ^1s stage(s), the following existing file(s) will be overwritten.");
DECLARE_LE_STR(kToSaveTheStageSaveComponents, "To save the ^1s stage(s), save the following component(s).");
DECLARE_LE_STR(kToExportTheStageSaveComponents, "To export the ^1s stage(s), save the following component(s).");
DECLARE_LE_STR(kUsedInStagesTooltip, "<b>Used in Stages</b>: ");

DECLARE_LE_STR(kSetLayerAsTargetLayerTooltip, "Set layer as target layer. Edits are added to the target layer.");
DECLARE_LE_STR(kUsdLayerIdentifier, "USD Layer identifier: ^1s");
DECLARE_LE_STR(kUsdStage, "USD Stage:");
DECLARE_LE_STR(kPinUsdStageTooltip, "Pin the current stage to keep your Layer Editor view fixed while you select different stages.");

DECLARE_LE_STR(kSaveAnonymousLayersErrorsTitle, "Save All Layers Error");
DECLARE_LE_STR(kSaveAnonymousLayersErrorsMsg, "Errors were encountered while saving layers. Check Script Editor for details.");
DECLARE_LE_STR(kSaveAnonymousLayersErrors, "Layer ^1s could not be saved to: ^2s");
DECLARE_LE_STR(kSaveAnonymousConfirmOverwriteTitle, "Confirm Overwrite");
DECLARE_LE_STR(kSaveAnonymousConfirmOverwrite, "^1s file(s) already exist and will be overwritten. Do you want to continue?");
DECLARE_LE_STR(kSaveAnonymousIdenticalFilesTitle, "Identical Names Error");
DECLARE_LE_STR(kSaveAnonymousIdenticalFiles, "^1s files have identical file names and prevent correct saving. Please select different file names for each layer. Here is the list of identical file names:<br>");

DECLARE_LE_STR(kBatchSaveAllRelative, "Convert All to Relative Paths");
DECLARE_LE_STR(kBatchSaveRelativeToScene, "Relative to Scene File");
DECLARE_LE_STR(kBatchSaveRelativeToParent, "Relative to Parent Layer");
DECLARE_LE_STR(kBatchSaveRelativeToLayerTooltip, "This path will be relative to ^1s");
DECLARE_LE_STR(kBatchSaveRelativeToSceneTooltip, "This path will be relative to the saved scene file");

// -------------------------------------------------------------
// Layer context menu
//
// Reuses the keys the old editor's MEL menu registered, so Maya's translations still apply.
// kMenuPrintToScriptEditor keeps the neutral "Listener" wording; Maya overrides it.
// -------------------------------------------------------------

DECLARE_LE_STR(kMenuRemove, "Remove");
DECLARE_LE_STR(kMenuSaveAs, "Save As...");
DECLARE_LE_STR(kMenuSaveEdits, "Save Edits");
DECLARE_LE_STR(kMenuReload, "Reload");
DECLARE_LE_STR(kMenuAddSublayer, "Add Sublayer");
DECLARE_LE_STR(kMenuAddParentLayer, "Add Parent Layer");
DECLARE_LE_STR(kMenuLoadSublayers, "Load Sublayers...");
DECLARE_LE_STR(kMenuMergeWithSublayers, "Merge with Sublayers");
DECLARE_LE_STR(kMenuMute, "Mute");
DECLARE_LE_STR(kMenuUnmute, "Unmute");
DECLARE_LE_STR(kMenuLock, "Lock");
DECLARE_LE_STR(kMenuUnlock, "Unlock");
DECLARE_LE_STR(kMenuLockLayerAndSublayers, "Lock Layer and Sublayers");
DECLARE_LE_STR(kMenuUnlockLayerAndSublayers, "Unlock Layer and Sublayers");
DECLARE_LE_STR(kMenuPrintToScriptEditor, "Print to Listener");
DECLARE_LE_STR(kMenuSelectPrimsWithSpec, "Select Prims With Spec");
DECLARE_LE_STR(kMenuClear, "Clear");

// -------------------------------------------------------------
// Component save widget
// -------------------------------------------------------------

DECLARE_LE_STR(kComponentName, "Name");
DECLARE_LE_STR(kComponentLocation, "Location");
DECLARE_LE_STR(kComponentBrowseForFolder, "Browse for folder");
DECLARE_LE_STR(kComponentFileStructure, "The following file structure is created on save.");
DECLARE_LE_STR(kComponentNothingToPreview, "Nothing to preview since no information about the template is available.");

// -------------------------------------------------------------
// Save-layer file dialog
//
// Used only by the Qt fallback; Maya replaces the whole dialog via browseForLayerSavePath.
// -------------------------------------------------------------

DECLARE_LE_STR(kSaveUsdFileDialogTitle, "Save Universal Scene Description (USD) File");
DECLARE_LE_STR(kSaveUsdFileDialogFilter, "USD (*.usd;*.usda;*.usdc)");

// -------------------------------------------------------------
// Errors
// -------------------------------------------------------------

DECLARE_LE_STR(kErrorCannotAddPathInHierarchy, "Cannot add path \"^1s\" again in the layer hierarchy");
DECLARE_LE_STR(kErrorCannotAddPathInHierarchyThrough, "Cannot add path \"^1s\" again in the layer hierarchy through \"^2s\"");
DECLARE_LE_STR(kErrorCannotAddPathTwice, "Cannot add path \"^1s\" twice to the layer stack");
DECLARE_LE_STR(kErrorFailedToSaveFile, "Failed to save file to \"^1s\"");
DECLARE_LE_STR(kErrorRecursionDetected, "Recursion detected. Found \"^1s\" multiple times.\n"
                                        "Only added the first instance to the tree view.");
DECLARE_LE_STR(kErrorDidNotFind, "USD Layer Editor: did not find \"^1s\"\n");
DECLARE_LE_STR(kErrorFailedToReloadLayer, "Failed to Reload Layer");


// Only valid inside namespace StringResources; undef to avoid polluting the global namespace.
#undef CREATE_LE_STR
#undef DECLARE_LE_STR

// clang-format on

} // namespace StringResources

} // namespace UsdLayerEditor

#endif // STRING_RESOURCES_H
