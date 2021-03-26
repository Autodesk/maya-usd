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

#ifndef STRING_RESOURCES_H
#define STRING_RESOURCES_H

#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringResource.h>
#include <maya/MStringResourceId.h>

class QString;

namespace UsdLayerEditor {

namespace StringResources {

// register all strings
MStatus registerAll();

// Retrive a string resource from the given MStringResourceId.
MString getAsMString(const MStringResourceId& strResID);

// Retrive a string resource from the given MStringResourceId.
QString getAsQString(const MStringResourceId& strResID);

// create a MStringResourceId, must be called before registerAll()
MStringResourceId create(const char* key, const char* value);

// -------------------------------------------------------------

// Same keys defined in MEL file will have priority over C++ keys/values.
// Those with higher priority will end up showing in the UI.

// clang-format off

const auto kAddNewLayer              { create("kAddNewLayer", "Add a New Layer") };
const auto kAddSublayer              { create("kAddSublayer", "Add sublayer") };
const auto kAscii                    { create("kAscii", "ASCII") };
const auto kAutoHideSessionLayer     { create("kAutoHideSessionLayer", "Auto-Hide Session Layer") };
const auto kBinary                   { create("kBinary", "Binary") };
const auto kConfirmExistFileSave     { create("kConfirmExistFileSave", "Confirm Existing File Save") };
const auto kConvertToRelativePath    { create("kConvertToRelativePath", "Convert to Relative Path") };
const auto kCancel                   { create("kCancel", "Cancel") };
const auto kCreate                   { create("kCreate", "Create") };
const auto kRevertToFileTitle        { create("kRevertToFileTitle", "Revert to File \"^1s\"") };
const auto kRevertToFileMsg          { create("kRevertToFileMsg", "Are you sure you want to revert \"^1s\" to its "
                                               "state on disk? All edits will be discarded.") };
const auto kHelp                     { create("kHelp", "Help") };
const auto kHelpOnUSDLayerEditor     { create("kHelpOnUSDLayerEditor", "Help on USD Layer Editor") };
const auto kLoadExistingLayer        { create("kLoadExistingLayer", "Load an Existing Layer") };
const auto kLoadSublayersError       { create("kLoadSublayersError", "Load Sublayers Error") };
const auto kLoadSublayersTo          { create("kLoadSublayersTo", "Load Sublayers to ^1s") };
const auto kLoadSublayers            { create("kLoadSublayers", "Load Sublayers") };
const auto kLayerPath                { create("kLayerPath", "Layer Path:") };
const auto kMuteUnmuteLayer          { create("kMuteUnmuteLayer", "Mute/unmute the layer. Muted layers are ignored by the stage.")};
const auto kNoLayers                 { create("kNoLayers", "No Layers") };
const auto kNotUndoable              { create("kNotUndoable", "You can not undo this action.") };
const auto kOption                   { create("kOption", "Option") };
const auto kPathNotFound             { create("kPathNotFound", "Path not found: ") };
const auto kRealPath                 { create("kRealPath", "Real Path: ^1s") };
const auto kRemoveSublayer           { create("kRemoveSublayer", "Remove sublayer") };
const auto kSave                     { create("kSave", "Save") };
const auto kSaveAll                  { create("kSaveAll", "Save All") };
const auto kSaveAllEditsInLayerStack { create("kSaveAllEditsInLayerStack", "Save all edits in the Layer Stack")};
const auto kSaveLayer                { create("kSaveLayer", "Save Layer") };
const auto kSaveName                 { create("kSaveName", "Save ^1s") };
const auto kSaveLayerSaveNestedAnonymLayer { create("kSaveLayerSaveNestedAnonymLayer",
                                                    "To save ^1s, you must save your ^2s anonymous layer(s) that are nested under it.") };
const auto kSaveLayerWarnTitle       { create("kSaveLayerWarnTitle", "Save ^1s") };
const auto kSaveLayerWarnMsg         { create("kSaveLayerWarnMsg", "Saving edits to ^1s will overwrite your file.") };
const auto kSaveStage                { create("kSaveStage", "Save Stage") };
const auto kSaveStages               { create("kSaveStages", "Save Stage(s)") };
const auto kSaveXStages              { create("kSaveXStages", "Save ^1s Stage(s)") };
const auto kToSaveTheStageSaveAnonym { create("kToSaveTheStageSaveAnonym", "To save the ^1s stage(s), save the following ^2s anonymous layer(s).") };
const auto kToSaveTheStageSaveFiles  { create("kToSaveTheStageSaveFiles", "To save the ^1s stage(s), the following existing file(s) will be overwritten.") };
const auto kUsedInStagesTooltip      { create("kUsedInStagesTooltip", "<b>Used in Stages</b>: ") };

const auto kSetLayerAsTargetLayerTooltip  { create("kSetLayerAsTargetLayerTooltip", "Set layer as target layer. Edits are added to the target layer.") };
const auto kUsdSaveFileFormat        { create("kUsdSaveFileFormat", "Save .usd File Format") };
const auto kUsdLayerIdentifier       { create("kUsdLayerIdentifier", "USD Layer identifier: ^1s") };
const auto kUsdStage                 { create("kUsdStage", "USD Stage:") };

const auto kSaveAnonymousLayersErrorsTitle { create("kSaveAnonymousLayersErrorsTitle", "Save All Layers Error")};
const auto kSaveAnonymousLayersErrorsMsg { create("kSaveAnonymousLayersErrorsMsg", "Errors were encountered while saving layers.  Check Script Editor for details.")};
const auto kSaveAnonymousLayersErrors { create("kSaveAnonymousLayersErrors", "Layer ^1s could not be saved to: ^2s")};
const auto kSaveAnonymousConfirmOverwriteTitle { create("kSaveAnonymousConfirmOverwriteTitle", "Confirm Overwrite")};
const auto kSaveAnonymousConfirmOverwrite { create("kSaveAnonymousConfirmOverwrite", "^1s file(s) already exist and will be overwritten.  Do you want to continue?")};

const auto kSaveLayerUsdFileFormatAnn{ create("kSaveLayerUsdFileFormatAnn", "Select whether the .usd file is written out in binary or ASCII. You can save a file in .usdc (binary) or .usda (ASCII) format. Manually entering a file name with an extension overrides the selection in this drop-down menu.") };
const auto kSaveLayerUsdFileFormatSbm{ create("kSaveLayerUsdFileFormatSbm", "Select whether the .usd file is written out in binary or ASCII") };


// -------------------------------------------------------------
// Errors
// -------------------------------------------------------------

const auto kErrorCannotAddPathInHierarchy        { create("kErrorCannotAddPathInHierarchy",
                                                          "Cannot add path \"^1s\" again in the layer hierarchy") };
const auto kErrorCannotAddPathInHierarchyThrough { create("kErrorCannotAddPathInHierarchyThrough",
                                                           "Cannot add path \"^1s\" again in the layer hierarchy through \"^2s\"") };
const auto kErrorCannotAddPathTwice              { create("kErrorCannotAddPathTwice",
                                                          "Cannot add path \"^1s\" twice to the layer stack") };
const auto kErrorFailedToSaveFile                { create("kErrorFailedToSaveFile",
                                                           "Failed to save file to \"^1s\"") };
const auto kErrorRecursionDetected               { create("kErrorRecursionDetected",
                                                          "Recursion detected. Found \"^1s\" multiple times.\n"
                                                           "Only added the first instance to the tree view.") };
const auto kErrorDidNotFind                      { create("kErrorDidNotFind",
                                                          "USD Layer Editor: did not find \"^1s\"\n") };
const auto kErrorFailedToReloadLayer             { create("kErrorFailedToReloadLayer",
                                                           "Failed to Reload Layer") };

// clang-format on

} // namespace StringResources

} // namespace UsdLayerEditor

#endif // STRING_RESOURCES_H
