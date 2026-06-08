//
// Copyright 2024 Autodesk
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

#ifndef USDLAYEREDITOR_UIUTILS_H
#define USDLAYEREDITOR_UIUTILS_H

#include "layerEditorAPI.h"

#include <functional>
#include <string>

namespace UsdLayerEditor {
namespace UIUtils {

LayerEditorAPI void setErrorDisplayCallbackFunction(std::function<void(std::string)> errorFunction);

void displayError(const std::string& error);

int dpiScale(int pixel);

} // namespace UIUtils
} // namespace UsdLayerEditor

#endif