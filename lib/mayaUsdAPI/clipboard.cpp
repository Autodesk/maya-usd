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

#include "clipboard.h"

#include <usdUfe/ufe/UsdClipboardHandler.h>

namespace MAYAUSDAPI_NS_DEF {

bool hasItemToPaste(const Ufe::ClipboardHandler::Ptr& ptr, HasItemToPasteTestFn testFn)
{
    if (auto usdClipboardHandler = std::dynamic_pointer_cast<UsdUfe::UsdClipboardHandler>(ptr)) {
        return usdClipboardHandler->hasItemToPaste(testFn);
    }
    return false;
}

void setClipboardFilePath(const Ufe::ClipboardHandler::Ptr& ptr, const std::string& clipboardPath)
{
    if (auto usdClipboardHandler = std::dynamic_pointer_cast<UsdUfe::UsdClipboardHandler>(ptr)) {
        usdClipboardHandler->setClipboardFilePath(clipboardPath);
    }
}

void setClipboardFileFormat(const Ufe::ClipboardHandler::Ptr& ptr, const std::string& formatTag)
{
    if (auto usdClipboardHandler = std::dynamic_pointer_cast<UsdUfe::UsdClipboardHandler>(ptr)) {
        usdClipboardHandler->setClipboardFileFormat(formatTag);
    }
}

} // namespace MAYAUSDAPI_NS_DEF