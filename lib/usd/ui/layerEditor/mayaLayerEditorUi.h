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

#ifndef MAYALAYEREDITORUI_H
#define MAYALAYEREDITORUI_H

#include <mayaUsdUI/ui/api.h>

namespace UsdLayerEditor {

// Registers the Qt-dependent layer-editor DCC functions (main window parent,
// Edit Forwarding dialog) and installs MayaQtUtils as the Qt utils provider.
// Call once at Maya plugin initialization, after registerLayerEditorDCCFunctions().
MAYAUSD_UI_PUBLIC void initializeUi();

} // namespace UsdLayerEditor

#endif // MAYALAYEREDITORUI_H
