#ifndef USDCLIPBOARDHANDLER_H
#define USDCLIPBOARDHANDLER_H

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

#include <mayaUsd/base/api.h>

#include <ufe/clipboardHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdClipboardHandler interface object.
class MAYAUSD_CORE_PUBLIC UsdClipboardHandler : public Ufe::ClipboardHandler
{
public:
    typedef std::shared_ptr<UsdClipboardHandler> Ptr;

    UsdClipboardHandler();
    ~UsdClipboardHandler() override;

    // Delete the copy/move constructors assignment operators.
    UsdClipboardHandler(const UsdClipboardHandler&) = delete;
    UsdClipboardHandler& operator=(const UsdClipboardHandler&) = delete;
    UsdClipboardHandler(UsdClipboardHandler&&) = delete;
    UsdClipboardHandler& operator=(UsdClipboardHandler&&) = delete;

    //! Create a UsdClipboardHandler.
    static UsdClipboardHandler::Ptr create();

    // Ufe::ClipboardHandler overrides.
    Ufe::UndoableCommand::Ptr          cutCmd_(const Ufe::Selection& selection) override;
    Ufe::UndoableCommand::Ptr          copyCmd_(const Ufe::Selection& selection) override;
    Ufe::SelectionUndoableCommand::Ptr pasteCmd_(const Ufe::SceneItem::Ptr& parentItem) override;

}; // UsdClipboardHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // USDCLIPBOARDHANDLER_H