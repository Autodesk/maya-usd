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

#include "tokens.h"

namespace UsdLayerEditor {
namespace Options {

inline bool optionVarExists(const std::string& name)
{
    // TODO LE-EXTRACT Options save - check existing.

    // Temporary exception for showing confirmation dialogs on saving layers.
    if (name == pxr::UsdLayerEditorOptionVars->ConfirmExistingFileSave.GetText()) {
        return true;
    }

    return false; 
}

inline int optionVarIntValue(const std::string& name)
{
    // TODO LE-EXTRACT Options save - get int value.

    // Temporary exception for showing confirmation dialogs on saving layers.
    if (name == pxr::UsdLayerEditorOptionVars->ConfirmExistingFileSave.GetText()) {
        return 1;
    }
    return 0;
}

inline int optionVarIntValue(const std::string& name, bool& exists)
{
    exists = optionVarExists(name);
    if (exists) {
        return optionVarIntValue(name);
    }
    return 0;
}

inline void setOptionVarValue(const std::string& name, int value)
{
    // TODO LE-EXTRACT Options save - set int value.
}

} // namespace Options
} // namespace UsdLayerEditor