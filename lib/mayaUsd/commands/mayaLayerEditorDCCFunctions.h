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
#ifndef MAYAUSD_MAYA_LAYER_EDITOR_DCC_FUNCTIONS_H
#define MAYAUSD_MAYA_LAYER_EDITOR_DCC_FUNCTIONS_H

#include <mayaUsd/base/api.h>

namespace UsdLayerEditor {

// Populates the shared layer-editor DCC-functions registry with the Maya
// implementations that do not depend on Qt (Component Creator, DCC object/stage
// queries, save options, file system, serialization, and the non-UI parts of
// Edit Forwarding). Safe to call in headless/batch sessions. Call once at Maya
// plugin initialization, before any layer-editor command runs. In Qt builds,
// registerLayerEditorQtFunctions() adds the remaining UI-dependent functions on
// top of these.
MAYAUSD_CORE_PUBLIC void registerLayerEditorDCCFunctions();

// Clears the registry back to defaults. Call at plugin unload.
MAYAUSD_CORE_PUBLIC void deregisterLayerEditorDCCFunctions();

} // namespace UsdLayerEditor

#endif // MAYAUSD_MAYA_LAYER_EDITOR_DCC_FUNCTIONS_H
