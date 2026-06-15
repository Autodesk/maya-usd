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

#include "batchSaveLayersUIDelegate.h"

#include "../mayaLayerEditorDCCFunctions.h"
#include "mayaQtUtils.h"

#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
#include <batchSaveLayersUIDelegate.h>
#include <saveLayersDialog.h>
#include <utilFileSystem.h>
#include <utilQT.h>
#include <utilSerialization.h>
#else
#include "saveLayersDialog.h"
#endif

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/nodes/layerManager.h>
#include <mayaUsd/utils/stageCache.h>
#include <mayaUsd/utils/util.h> // UsdMayaUtil::ConvertMDistanceUnitToUsdGeomLinearUnit
#include <mayaUsd/utils/utilComponentCreator.h>
#include <mayaUsd/utils/utilFileSystem.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/base/tf/stringUtils.h> // TfStringPrintf
#include <pxr/usd/usdGeom/tokens.h>

#include <maya/MDistance.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <ghc/fs_std.hpp>

void UsdLayerEditor::initialize()
{
#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
    UsdLayerEditor::registerLayerEditorDCCFunctions();

    if (nullptr == UsdLayerEditor::getQtUtils()) {
        UsdLayerEditor::setQtUtils(new MayaQtUtils());
    }

    UsdLayerEditor::FileSystem::setDCCSceneLocationFunc(
        []() { return UsdMayaUtilFileSystem::getMayaSceneFileDir(); });

    UsdLayerEditor::FileSystem::setDCCWorkspaceSceneLocationFunc(
        []() { return std::string(UsdMayaUtil::GetCurrentMayaWorkspacePath().asChar()); });

    UsdLayerEditor::Serialization::setUpdateDCCObjectRootLayerFunction(
        [](const std::string&            proxyPath,
           const std::string&            layerPath,
           const PXR_NS::SdfLayerRefPtr& layer,
           bool                          wasTargetLayer) {
            MayaUsd::utils::setNewProxyPath(
                MString(proxyPath.c_str()),
                MString(layerPath.c_str()),
                MayaUsd::utils::kProxyPathFollowProxyShape,
                layer,
                wasTargetLayer);
        });

    UsdLayerEditor::Serialization::setGetStageCachesFunction([]() {
        std::vector<PXR_NS::UsdStageCache*> caches;
        for (PXR_NS::UsdStageCache& cache : UsdMayaStageCache::GetAllCaches())
            caches.push_back(&cache);
        return caches;
    });

    UsdLayerEditor::Serialization::setLayerUpAxisAndUnitsFn(
        [](const PXR_NS::SdfLayerRefPtr& layer) {
            const PXR_NS::TfToken upAxis
                = MGlobal::isZAxisUp() ? PXR_NS::UsdGeomTokens->z : PXR_NS::UsdGeomTokens->y;
            const double metersPerUnit
                = UsdMayaUtil::ConvertMDistanceUnitToUsdGeomLinearUnit(MDistance::internalUnit());
            layer->SetField(
                PXR_NS::SdfPath::AbsoluteRootPath(),
                PXR_NS::UsdGeomTokens->metersPerUnit,
                metersPerUnit);
            layer->SetField(
                PXR_NS::SdfPath::AbsoluteRootPath(), PXR_NS::UsdGeomTokens->upAxis, upAxis);
        });

    UsdLayerEditor::FileSystem::setPrepareLayerSaveUILayerFn(
        [](const std::string& relativeAnchor) -> bool {
            const char* script = "import mayaUsd_USDRootFileRelative as murel\n"
                                 "murel.usdFileRelative.setRelativeFilePathRoot(r'''%s''')";
            const std::string commandString = PXR_NS::TfStringPrintf(script, relativeAnchor.c_str());
            return MGlobal::executePythonCommand(commandString.c_str());
        });

    UsdLayerEditor::FileSystem::setFileWriteAccessFunction(
        [](const std::string& filePath) -> bool {
            const fs::filesystem::path p(filePath);
            if (!fs::filesystem::exists(p))
                return true;
            const auto perms = fs::filesystem::status(p).permissions();
            return (perms & fs::filesystem::perms::owner_write) != fs::filesystem::perms::none;
        });
#else
    if (nullptr == UsdLayerEditor::utils) {
        UsdLayerEditor::utils = new MayaQtUtils();
    }
#endif
}

#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
namespace {

// Adapt Maya's StageSavingInfo (carrying an MDagPath) to the shared
// StageSavingInfo (carrying a dccObjectPath string + stageName).
std::vector<UsdLayerEditor::StageSavingInfo>
toSharedInfos(const std::vector<MayaUsd::StageSavingInfo>& mayaInfos)
{
    std::vector<UsdLayerEditor::StageSavingInfo> sharedInfos;
    sharedInfos.reserve(mayaInfos.size());
    for (const auto& mi : mayaInfos) {
        UsdLayerEditor::StageSavingInfo si;
        si.stage = mi.stage;
        si.dccObjectPath = mi.dagPath.fullPathName().asChar();
        // Use the leaf name of the dag path as a friendly stage name.
        si.stageName = mi.dagPath.partialPathName().asChar();
        si.shareable = mi.shareable;
        si.isIncoming = mi.isIncoming;
        sharedInfos.push_back(si);
    }
    return sharedInfos;
}

} // namespace
#endif

MayaUsd::BatchSaveResult UsdLayerEditor::batchSaveLayersUIDelegate(
    const std::vector<MayaUsd::StageSavingInfo>& infos,
    bool                                         isExporting)
{
    if (MGlobal::kInteractive == MGlobal::mayaState()) {
        auto opt = MayaUsd::utils::serializeUsdEditsLocationOption();
        if (MayaUsd::utils::kSaveToUSDFiles == opt) {

            static const MString kConfirmExistingFileSave
                = MayaUsdOptionVars->ConfirmExistingFileSave.GetText();
            bool showConfirmDglOption = MGlobal::optionVarExists(kConfirmExistingFileSave)
                && MGlobal::optionVarIntValue(kConfirmExistingFileSave) != 0;

            bool atLeastOneLayerToSave = false;
            bool atLeastOneAnonToSave = false;

            for (const auto& info : infos) {
                MayaUsd::utils::StageLayersToSave StageLayersToSave;
                MayaUsd::utils::getLayersToSaveFromProxy(
                    info.dagPath.fullPathName().asChar(), StageLayersToSave);
                if (!StageLayersToSave._anonLayers.empty()) {
                    atLeastOneAnonToSave = true;
                    atLeastOneLayerToSave = true;
                    break;
                }
                if (!StageLayersToSave._dirtyFileBackedLayers.empty()) {
                    atLeastOneLayerToSave = true;
                    // If the option is set to show the confirmation dialog,
                    // we can stop here, we already know we will have to show it
                    // below, no need to complete the search for atLeastOneAnonToSave.
                    if (showConfirmDglOption) {
                        break;
                    }
                }
            }

            // if at least one stage contains anonymous layers, you need to show the comfirm dialog
            // so the user can choose where to save the anonymous layers.
            bool showConfirmDgl
                = (showConfirmDglOption || atLeastOneAnonToSave) && atLeastOneLayerToSave;

            if (showConfirmDgl) {

#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
                const auto                       sharedInfos = toSharedInfos(infos);
                UsdLayerEditor::SaveLayersDialog dlg(nullptr, sharedInfos, isExporting);
#else
                UsdLayerEditor::SaveLayersDialog dlg(nullptr, infos, isExporting);
#endif

                // The SaveLayers dialog only handles choosing new names for anonymous layers and
                // making sure that they are remapped correctly in either their parent layer or by
                // the owning proxy shape. The SaveLayers dialog itself does not currently handle
                // the saving of file-backed layers, so for now we will return that we only partiall
                // completed saving. This will trigger the LayerManager to double check what needs
                // to be saved and to complete the saving of all file-backed layers.
                //
                return (QDialog::Rejected == dlg.exec()) ? MayaUsd::kAbort
                                                         : MayaUsd::kPartiallyCompleted;
            }
        } else if (MayaUsd::utils::kSaveToMayaSceneFile == opt) {
            // When saving to Maya scene file, only show dialog for component stages
            // that need initial save location selection.
            bool hasComponentStages = false;
            for (const auto& info : infos) {
                if (MayaUsd::ComponentUtils::isAdskUsdComponent(
                        info.dagPath.fullPathName().asChar())) {
                    hasComponentStages = true;
                    break;
                }
            }

            if (hasComponentStages) {
                const bool componentsOnly = true;
#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
                const auto                       sharedInfos = toSharedInfos(infos);
                UsdLayerEditor::SaveLayersDialog dlg(
                    nullptr, sharedInfos, isExporting, componentsOnly);
#else
                UsdLayerEditor::SaveLayersDialog dlg(nullptr, infos, isExporting, componentsOnly);
#endif

                // Execute the dialog and return partially completed even if the dialog is closed.
                dlg.exec();

                return MayaUsd::kPartiallyCompleted;
            }
        }
    }

    return MayaUsd::kNotHandled;
}
