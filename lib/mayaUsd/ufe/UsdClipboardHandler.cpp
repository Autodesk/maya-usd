//
// Copyright 2023 Autodesk
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

#include "UsdClipboardHandler.h"

#include <mayaUsd/ufe/UsdUndoClipboardCommand.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdClipboardHandler::UsdClipboardHandler() { }

UsdClipboardHandler::~UsdClipboardHandler() { }

/*static*/
UsdClipboardHandler::Ptr UsdClipboardHandler::create()
{
    return std::make_shared<UsdClipboardHandler>();
}

//------------------------------------------------------------------------------
// Ufe::ClipboardHandler overrides
//------------------------------------------------------------------------------

Ufe::UndoableCommand::Ptr UsdClipboardHandler::cutCmd_(const Ufe::Selection& selection)
{
    return UsdUndoCutClipboardCommand::create(selection);
}

Ufe::UndoableCommand::Ptr UsdClipboardHandler::copyCmd_(const Ufe::Selection& selection)
{
    return UsdUndoCopyClipboardCommand::create(selection);
}

Ufe::SelectionUndoableCommand::Ptr
UsdClipboardHandler::pasteCmd_(const Ufe::SceneItem::Ptr& parentItem)
{
    return UsdUndoPasteClipboardCommand::create(parentItem);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
