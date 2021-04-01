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

#include "mayaQtUtils.h"
#include "saveLayersDialog.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/nodes/layerManager.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <maya/MGlobal.h>

void UsdLayerEditor::initialize()
{
    if (nullptr == UsdLayerEditor::utils) {
        UsdLayerEditor::utils = new MayaQtUtils();
    }
}

MayaUsd::BatchSaveResult UsdLayerEditor::batchSaveLayersUIDelegate(const MDagPathArray& proxyShapes)
{
    if (MGlobal::kInteractive == MGlobal::mayaState()) {
        auto opt = MayaUsd::utils::serializeUsdEditsLocationOption();
        if (MayaUsd::utils::kSaveToUSDFiles == opt) {

            static const MString kConfirmExistingFileSave
                = MayaUsdOptionVars->ConfirmExistingFileSave.GetText();
            bool showConfirmDgl = MGlobal::optionVarExists(kConfirmExistingFileSave)
                && MGlobal::optionVarIntValue(kConfirmExistingFileSave) != 0;

            // if at least one stage contains anonymous layers, you need to show the comfirm dialog
            // so the user can choose where to save the anonymous layers.
            if (!showConfirmDgl) {
                for (auto& shape : proxyShapes) {
                    MayaUsd::utils::stageLayersToSave stageLayersToSave;
                    MayaUsd::utils::getLayersToSaveFromProxy(
                        shape.fullPathName().asChar(), stageLayersToSave);
                    if (!stageLayersToSave._anonLayers.empty()) {
                        showConfirmDgl = true;
                        break;
                    }
                }
            }

            if (showConfirmDgl) {
                UsdLayerEditor::SaveLayersDialog dlg(nullptr, proxyShapes);

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
        }
    }

    return MayaUsd::kNotHandled;
}
