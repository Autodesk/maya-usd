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
#ifndef USDLAYEREDITOR_BATCH_SAVE_LAYERS_UI_H
#define USDLAYEREDITOR_BATCH_SAVE_LAYERS_UI_H

#include "layerEditorAPI.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/common.h>

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

enum BatchSaveResult
{
    kAbort,             // User has chosen to abort the file operation.
    kNotHandled,        // Callback did not handle any of the stages passed to it.
    kCompleted,         // Callback handled all stages.  Layer Manager should not continue
                        // to process anything.
    kPartiallyCompleted // Callback has handled the saving of some stages, but not all. Layer
                        // Manager should continue to look for unsaved stages.
};

/*! \brief Information about the stages that need to be saved.
 */
struct StageSavingInfo
{
    UsdStageRefPtr stage;
    std::string    stageName;
    // DCC-side object path (e.g. proxy shape path on the Maya side) that
    // owns the stage. May be empty if no DCC object is associated.
    std::string    dccObjectPath;
    bool           shareable = true;
    bool           isIncoming = false;
};

LayerEditorAPI  BatchSaveResult
batchSaveLayersUIDelegate(const std::vector<StageSavingInfo>& infos, bool isExporting);

} // namespace UsdLayerEditor

#endif // BATCH_SAVE_LAYERS_UI_H
