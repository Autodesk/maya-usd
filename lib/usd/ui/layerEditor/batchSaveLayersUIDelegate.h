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
#ifndef BATCH_SAVE_LAYERS_UI_H
#define BATCH_SAVE_LAYERS_UI_H

#include <mayaUsd/mayaUsd.h>
#include <mayaUsd/nodes/layerManager.h>
#include <mayaUsdUI/ui/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/common.h>

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

MAYAUSD_UI_PUBLIC
void initialize();

MAYAUSD_UI_PUBLIC
MayaUsd::BatchSaveResult batchSaveLayersUIDelegate(const std::vector<UsdStageRefPtr>&);

} // namespace UsdLayerEditor

#endif // BATCH_SAVE_LAYERS_UI_H
