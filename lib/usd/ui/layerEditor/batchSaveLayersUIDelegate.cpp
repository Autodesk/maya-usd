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

#include <mayaUsd/nodes/layerManager.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <maya/MGlobal.h>

bool UsdLayerEditor::batchSaveLayersUIDelegate(const std::vector<UsdStageRefPtr>& stages)
{
    if (MGlobal::kInteractive == MGlobal::mayaState()) {
        auto opt = UsdMayaSerialization::serializeUsdEditsLocationOption();
        if (UsdMayaSerialization::kSaveToUSDFiles == opt) {
            UsdLayerEditor::SaveLayersDialog dlg(nullptr, stages);
            if (QDialog::Accepted != dlg.exec()) {
                return false;
            }
        }
    }

    return true;
}
