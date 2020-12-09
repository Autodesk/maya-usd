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

// Same keys definied in MEL file will have priority over C++ keys/values.
// Those with higher priority will end up showing in the UI.

// clang-format off

const auto kAddNewLayer              { create("kAddNewLayer", "Add a New Layer") };
const auto kAddSublayer              { create("kAddSublayer", "Add sublayer") };
const auto kAutoHideSessionLayer     { create("kAutoHideSessionLayer", "Auto-Hide Session Layer") };
const auto kConvertToRelativePath    { create("kConvertToRelativePath", "Convert to Relative Path") };
const auto kCancel                   { create("kCancel", "Cancel") };
const auto kClearLayerTitle          { create("kclearLayerTitle", "Clear \"^1s\"") };
const auto kClearLayerConfirmMessage { create("kClearLayerConfirmMessage",
                                              "Are you sure you want to clear this layer?"
                                              " \"^1s\"  will remain in the Layer Editor"
                                              " but all contents will be cleared, including sublayer paths.") };
const auto kCreate                   { create("kCreate", "Create") };
const auto kDiscardEditsTitle        { create("kDiscardEditsTitle", "Discard Edits \"^1s\"") };
const auto kDiscardEditsMsg          { create("kDiscardEditsMsg", "Are you sure you want to revert these edits?"
                                                                  " \"^1s\" will revert to its state on disk") };
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
const auto kSaveStage                { create("kSaveStage", "Save Stage") };
const auto kToSaveTheStageSaveAnonym { create("kToSaveTheStageSaveAnonym", "To save the stage, you must save your anonymous layer")};
const auto kToSaveTheStageSaveAnonyms{ create("kToSaveTheStageSaveAnonyms", "To save the stage, you must save your ^1s anonymous layers")};
const auto kToSaveTheStageFileWillBeSave  { create("kToSaveTheStageFileWillBeSave",
                                                   "To save the stage, edits to the following file will be saved.") };
const auto kToSaveTheStageFilesWillBeSave { create("kToSaveTheStageFilesWillBeSave",
                                                   "To save the stage, edits to the following ^1s files will be saved.") };
const auto kSetLayerAsTargetLayerTooltip  { create("kSetLayerAsTargetLayerTooltip", "Set layer as target layer. Edits are added to the target layer.") };
const auto kUsdLayerIdentifier       { create("kUsdLayerIdentifier", "USD Layer identifier: ^1s") };
const auto kUsdStage                 { create("kUsdStage", "USD Stage:") };

const auto kToSaveTheStageAnonFileWillBeSaved  { create("kToSaveTheStageAnonFileWillBeSaved",
                                                   "To save the stage, you must first save your anonymous layer.  All file-backed layers will also be saved.\n") };
const auto kToSaveTheStageAnonFilesWillBeSaved { create("kToSaveTheStageAnonFilesWillBeSaved",
                                                   "To save the stage, you must first save your ^1s anonymous layers.  All file-backed layers will also be saved.\n") };
const auto kSaveAnonymousLayersErrorsTitle { create("kSaveAnonymousLayersErrorsTitle", "Save All Layers Error")};
const auto kSaveAnonymousLayersErrorsMsg { create("kSaveAnonymousLayersErrorsMsg", "Errors were encountered while saving layers.  Check Script Editor for details.")};
const auto kSaveAnonymousLayersErrors { create("kSaveAnonymousLayersErrors", "Layer ^1s could not be saved to: ^2s")};


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
