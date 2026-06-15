//
// Copyright 2026 Autodesk
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
#ifndef MAYA_LAYER_EDITOR_DCC_FUNCTIONS_H
#define MAYA_LAYER_EDITOR_DCC_FUNCTIONS_H

#include <mayaUsdUI/ui/api.h>

namespace UsdLayerEditor {

// Populates the shared layer-editor DCC-functions registry with the Maya
// implementations (Component Creator, Edit Forwarding, DCC object/stage
// queries). Call once at Maya plugin initialization.
MAYAUSD_UI_PUBLIC void registerLayerEditorDCCFunctions();

// Clears the registry back to defaults. Call at plugin unload.
MAYAUSD_UI_PUBLIC void deregisterLayerEditorDCCFunctions();

} // namespace UsdLayerEditor

#endif // MAYA_LAYER_EDITOR_DCC_FUNCTIONS_H
