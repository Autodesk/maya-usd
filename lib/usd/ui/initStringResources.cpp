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
#include "initStringResources.h"

#include <mayaUsd/base/stringResources.h>
#include <mayaUsdUI/ui/stringResources.h>
#include <usdLayerEditor/ui/stringResources.h>

#include <usdUfe/base/stringResources.h>

namespace MayaUsdUI {

MStatus initStringResources()
{
    MStatus status;

    // Registers the UsdUfe plugin's strings with Maya.
    if (!UsdUfe::StringResources::registerStrings())
        status = MStatus::kFailure;

    // Registers the MayaUsd plugin's strings with Maya.
    if (!MayaUsd::StringResources::registerStrings())
        status = MStatus::kFailure;

    // Registers the MayaUsdUI plugin's strings with Maya.
    if (!StringResources::registerStrings())
        status = MStatus::kFailure;

    // Registers the usdLayerEditor plugin's strings with Maya.
    if (!UsdLayerEditor::StringResources::registerStrings())
        status = MStatus::kFailure;

    return status;
}

} // namespace MayaUsdUI
