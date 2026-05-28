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

#include "saveLayersDialog.h"
#include "tokens.h"
#include "utilOptions.h"
#include "utilQT.h"
#include "utilSerialization.h"

#include <usdUfe/ufe/Utils.h>

UsdLayerEditor::BatchSaveResult UsdLayerEditor::batchSaveLayersUIDelegate(
    const std::vector<StageSavingInfo>& infos,
    bool                                isExporting)
{
    // TODO LE-EXTRACT Maya non-interactive mode.
    // if (MGlobal::kInteractive == MGlobal::mayaState()) {

    auto opt = Serialization::serializeUsdEditsLocationOption();
    if (Serialization::kSaveToUSDFiles == opt) {

        static const std::string kConfirmExistingFileSave
            = UsdLayerEditorOptionVars->ConfirmExistingFileSave.GetText();
        bool showConfirmDgl = Options::optionVarExists(kConfirmExistingFileSave)
            && Options::optionVarIntValue(kConfirmExistingFileSave) != 0;

        // if at least one stage contains anonymous layers, you need to show the comfirm dialog
        // so the user can choose where to save the anonymous layers.
         if (!showConfirmDgl) {
            for (const auto& info : infos) {
                Serialization::StageLayersToSave StageLayersToSave;
                const auto dccObjectPath = UsdUfe::stagePath(info.stage).string();
                Serialization::getLayersToSaveFromDCCObject(dccObjectPath, StageLayersToSave);
                if (!StageLayersToSave._anonLayers.empty()) {
                    showConfirmDgl = true;
                    break;
                }
            }
        }

        if (showConfirmDgl) {
            UsdLayerEditor::SaveLayersDialog dlg(nullptr, infos, isExporting);

            // The SaveLayers dialog only handles choosing new names for anonymous layers and
            // making sure that they are remapped correctly in either their parent layer or by
            // the owning proxy shape. The SaveLayers dialog itself does not currently handle
            // the saving of file-backed layers, so for now we will return that we only partially
            // completed saving. This will trigger the LayerManager to double check what needs
            // to be saved and to complete the saving of all file-backed layers.
            //
            return (QDialog::Rejected == dlg.exec()) ? kAbort : kPartiallyCompleted;
        }
    }
    //}

    return BatchSaveResult::kNotHandled;
}
