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
#include "initStringResources.h"

#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
#include <stringResources.h>

#include <maya/MString.h>
#include <maya/MStringResource.h>
#include <maya/MStringResourceId.h>
#else
#include "layerEditor/stringResources.h"
#endif

namespace MAYAUSD_NS_DEF {

#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)

namespace {

// Bridge a shared StringResources::Resource into Maya's MStringResource registry
// so that the .pres files installed with the plugin can override the default
// text for translations. The shared widgets themselves read directly from the
// Resource::value, so when no Maya override is registered the default English
// strings still appear.
MStatus registerShared(const UsdLayerEditor::StringResources::Resource& res)
{
    MStringResourceId id(res.module.c_str(), res.key.c_str(), MString(res.value.c_str()));
    return MStringResource::registerString(id);
}

} // namespace

MStatus initStringResources()
{
    using namespace UsdLayerEditor::StringResources;
    MStatus status { MStatus::MStatusCode::kSuccess };

    // Forward each shared resource into Maya's MStringResource registry. The
    // list mirrors the const auto declarations in <stringResources.h>. The
    // shared widgets fetch values via getAsQString(Resource), which reads the
    // value embedded in the Resource directly — Maya's registry is consulted
    // only by Maya-side code that still goes through MStringResource APIs.
    status = registerShared(kAddNewLayer);
    status = registerShared(kAddSublayer);
    status = registerShared(kAutoHideSessionLayer);
    status = registerShared(kDisplayLayerContents);
    status = registerShared(kDisplayLayerContentsEmpty);
    status = registerShared(kEditForwardBanner);
    status = registerShared(kEchoEditForwarding);
    status = registerShared(kDisplayLayerExpandAllValues);
    status = registerShared(kDisplayLayerExpandAllValuesTooltip);
    status = registerShared(kConvertToRelativePath);
    status = registerShared(kCancel);
    status = registerShared(kCreate);
    status = registerShared(kReloadTitle);
    status = registerShared(kReloadMsg);
    status = registerShared(kReloadButtonText);
    status = registerShared(kHelp);
    status = registerShared(kHelpOnUSDLayerEditor);
    status = registerShared(kLoadExistingLayer);
    status = registerShared(kLoadSublayersError);
    status = registerShared(kLoadSublayersTo);
    status = registerShared(kLoadSublayers);
    status = registerShared(kLayerPath);
    status = registerShared(kMuteUnmuteLayer);
    status = registerShared(kLockUnlockLayer);
    status = registerShared(kLayerIsSystemLocked);
    status = registerShared(kNoLayers);
    status = registerShared(kNotUndoable);
    status = registerShared(kOption);
    status = registerShared(kPathNotFound);
    status = registerShared(kRealPath);
    status = registerShared(kRemoveSublayer);
    status = registerShared(kMenuStitchLayers);
    status = registerShared(kSave);
    status = registerShared(kSaveAll);
    status = registerShared(kSaveAllEditsInLayerStack);
    status = registerShared(kSaveLayer);
    status = registerShared(kSaveName);
    status = registerShared(kSaveLayerSaveNestedAnonymLayer);
    status = registerShared(kSaveLayerWarnTitle);
    status = registerShared(kSaveLayerWarnMsg);
    status = registerShared(kSaveStage);
    status = registerShared(kSaveStages);
    status = registerShared(kSaveStagesAndExport);
    status = registerShared(kSaveXStages);
    status = registerShared(kToSaveTheStageSaveAnonym);
    status = registerShared(kToSaveStageFilesConfirm);
    status = registerShared(kToSaveTheStageSaveFiles);
    status = registerShared(kToExportTheStageSaveAnonym);
    status = registerShared(kToExportTheStageSaveFiles);
    status = registerShared(kToSaveTheStageSaveComponents);
    status = registerShared(kToExportTheStageSaveComponents);
    status = registerShared(kUsedInStagesTooltip);
    status = registerShared(kSetLayerAsTargetLayerTooltip);
    status = registerShared(kUsdLayerIdentifier);
    status = registerShared(kUsdStage);
    status = registerShared(kPinUsdStageTooltip);
    status = registerShared(kSaveAnonymousLayersErrorsTitle);
    status = registerShared(kSaveAnonymousLayersErrorsMsg);
    status = registerShared(kSaveAnonymousLayersErrors);
    status = registerShared(kSaveAnonymousConfirmOverwriteTitle);
    status = registerShared(kSaveAnonymousConfirmOverwrite);
    status = registerShared(kSaveAnonymousIdenticalFilesTitle);
    status = registerShared(kSaveAnonymousIdenticalFiles);
    status = registerShared(kBatchSaveAllRelative);
    status = registerShared(kBatchSaveRelativeToScene);
    status = registerShared(kBatchSaveRelativeToParent);
    status = registerShared(kBatchSaveRelativeToLayerTooltip);
    status = registerShared(kBatchSaveRelativeToSceneTooltip);
    status = registerShared(kErrorCannotAddPathInHierarchy);
    status = registerShared(kErrorCannotAddPathInHierarchyThrough);
    status = registerShared(kErrorCannotAddPathTwice);
    status = registerShared(kErrorFailedToSaveFile);
    status = registerShared(kErrorRecursionDetected);
    status = registerShared(kErrorDidNotFind);
    status = registerShared(kErrorFailedToReloadLayer);

    return status;
}

#else

MStatus initStringResources() { return UsdLayerEditor::StringResources::registerAll(); }

#endif

} // namespace MAYAUSD_NS_DEF
