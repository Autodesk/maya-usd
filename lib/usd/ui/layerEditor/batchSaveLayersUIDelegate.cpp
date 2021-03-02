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

#include <mayaUsd/nodes/layerManager.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <maya/MGlobal.h>

void UsdLayerEditor::initialize()
{
    if (nullptr == UsdLayerEditor::utils) {
        UsdLayerEditor::utils = new MayaQtUtils();
    }
}

MayaUsd::BatchSaveResult
UsdLayerEditor::batchSaveLayersUIDelegate(const std::vector<UsdStageRefPtr>& stages)
{
    if (MGlobal::kInteractive == MGlobal::mayaState()) {
        auto opt = MayaUsd::utils::serializeUsdEditsLocationOption();
        if (MayaUsd::utils::kSaveToUSDFiles == opt) {
            UsdLayerEditor::SaveLayersDialog dlg(nullptr, stages);

            if (QDialog::Rejected == dlg.exec()) {
                return MayaUsd::kAbort;
            }

            if (!dlg.layersNotSaved().isEmpty() || !dlg.layersWithErrorPairs().isEmpty()) {
                return MayaUsd::kPartiallyCompleted;
            } else {
                return MayaUsd::kCompleted;
            }
        }
    }

    return MayaUsd::kNotHandled;
}
