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

#include <LookdevXUfe/FileHandler.h>

namespace LookdevXUsd
{
/**
 * @brief File handler. It handles the file specific operations.
 */
class UsdFileHandler : public LookdevXUfe::FileHandler
{
public:
    static LookdevXUfe::FileHandler::Ptr create()
    {
        return std::make_shared<UsdFileHandler>();
    }

    UsdFileHandler();
    ~UsdFileHandler() override;

    UsdFileHandler(const UsdFileHandler&) = delete;
    UsdFileHandler& operator=(const UsdFileHandler&) = delete;
    UsdFileHandler(UsdFileHandler&&) = delete;
    UsdFileHandler& operator=(UsdFileHandler&&) = delete;

    // UsdFileHandler overrides
    [[nodiscard]] std::string getResolvedPath(const Ufe::AttributeFilename::Ptr& fnAttr) const override;
    [[nodiscard]] Ufe::UndoableCommand::Ptr convertPathToAbsoluteCmd(
        const Ufe::AttributeFilename::Ptr& fnAttr) const override;
    [[nodiscard]] Ufe::UndoableCommand::Ptr convertPathToRelativeCmd(
        const Ufe::AttributeFilename::Ptr& fnAttr) const override;
    [[nodiscard]] Ufe::UndoableCommand::Ptr setPreferredPathCmd(const Ufe::AttributeFilename::Ptr& fnAttr,
                                                                const std::string& path) const override;
    [[nodiscard]] std::string openFileDialog(const Ufe::AttributeFilename::Ptr& fnAttr) const override;
};

} // namespace LookdevXUsd
