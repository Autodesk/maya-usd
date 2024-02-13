#ifndef USDCLIPBOARDHANDLER_H
#define USDCLIPBOARDHANDLER_H

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

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdClipboard.h>

#include <ufe/clipboardHandler.h>

namespace USDUFE_NS_DEF {

//! \brief Implementation of Ufe::ClipboardHandler interface for USD objects.
class USDUFE_PUBLIC UsdClipboardHandler : public Ufe::ClipboardHandler
{
public:
    using Ptr = std::shared_ptr<UsdClipboardHandler>;

    UsdClipboardHandler();

    // Delete the copy/move constructors assignment operators.
    UsdClipboardHandler(const UsdClipboardHandler&) = delete;
    UsdClipboardHandler& operator=(const UsdClipboardHandler&) = delete;
    UsdClipboardHandler(UsdClipboardHandler&&) = delete;
    UsdClipboardHandler& operator=(UsdClipboardHandler&&) = delete;

    //! Create a UsdClipboardHandler.
    static UsdClipboardHandler::Ptr create();

    // Ufe::ClipboardHandler overrides
    Ufe::UndoableCommand::Ptr       cutCmd_(const Ufe::Selection& selection) override;
    Ufe::UndoableCommand::Ptr       copyCmd_(const Ufe::Selection& selection) override;
    Ufe::PasteClipboardCommand::Ptr pasteCmd_(const Ufe::SceneItem::Ptr& parentItem) override;
    Ufe::UndoableCommand::Ptr       pasteCmd_(const Ufe::Selection& parentItems) override;
    bool                            hasItemsToPaste_() override;
    bool                            canBeCut_(const Ufe::SceneItem::Ptr& item) override;
    void                            preCopy_() override;
    void                            preCut_() override;

    // Helper functions
    typedef bool (*HasItemToPasteTestFn)(const PXR_NS::UsdPrim& prim);
    bool hasItemToPaste(HasItemToPasteTestFn testFn);

    //! Sets the absolute path (with filename) for saving clipboard data to.
    void setClipboardFilePath(const std::string& clipboardPath);

    //! Sets the USD file format for the clipboard file.
    //! \param[in] formatTag USD file format to save. Must be either "usda" or "usdc".
    void setClipboardFileFormat(const std::string& formatTag);

private:
    UsdClipboard::Ptr _clipboard;

}; // UsdClipboardHandler

} // namespace USDUFE_NS_DEF

#endif // USDCLIPBOARDHANDLER_H
