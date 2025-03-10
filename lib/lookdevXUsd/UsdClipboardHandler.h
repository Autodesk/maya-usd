//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc. and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//*****************************************************************************
#pragma once

#include <LookdevXUfe/ClipboardHandler.h>

namespace LookdevXUsd
{
/**
 * @brief Clipboard handler. It handles the Clipboard.
 */
class UsdClipboardHandler : public LookdevXUfe::ClipboardHandler
{
public:
    UsdClipboardHandler() = default;

    static void registerHandler(const Ufe::Rtid& rtId);
    static void unregisterHandler();

    // UsdClipboardHandler overrides
    [[nodiscard]] Ufe::UndoableCommand::Ptr cutCmd_(const Ufe::Selection& selection) override;
    [[nodiscard]] Ufe::UndoableCommand::Ptr copyCmd_(const Ufe::Selection& selection) override;
    [[nodiscard]] Ufe::PasteClipboardCommand::Ptr pasteCmd_(const Ufe::SceneItem::Ptr& parentItem) override;
    [[nodiscard]] Ufe::UndoableCommand::Ptr pasteCmd_(const Ufe::Selection& parentItems) override;
    [[nodiscard]] bool hasMaterialToPasteImpl() override;
    [[nodiscard]] bool hasNonMaterialToPasteImpl() override;
    [[nodiscard]] bool hasItemsToPaste_() override;
    [[nodiscard]] bool hasNodeGraphsToPasteImpl() override;
    void setClipboardPath(const std::string& clipboardPath) override;
    [[nodiscard]] bool canBeCut_(const Ufe::SceneItem::Ptr& item) override;
    void preCopy_() override;
    void preCut_() override;

private:
    static Ufe::ClipboardHandler::Ptr m_wrappedClipboardHandler;
    static Ufe::Rtid m_rtid;
};

} // namespace LookdevXUsd
