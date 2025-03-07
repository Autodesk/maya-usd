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

#ifndef USD_UNDO_CONNECTION_COMMAND_H
#define USD_UNDO_CONNECTION_COMMAND_H

#include "Export.h"

#include <LookdevXUfe/AttributeComponentInfo.h>
#include <LookdevXUfe/ExtendedConnection.h>
#include <LookdevXUfe/UndoableCommand.h>

#include <mayaUsdAPI/undo.h>

namespace LookdevXUsd
{

//! \brief UsdUndoConnectionCommand
class UsdCreateConnectionCommand : public LookdevXUfe::CreateConnectionResultCommand
{
public:
    using Ptr = std::shared_ptr<UsdCreateConnectionCommand>;

    // Public for std::make_shared() access, use create() instead.
    LOOKDEVX_USD_EXPORT UsdCreateConnectionCommand(const Ufe::Attribute::Ptr& srcAttr,
                                                   const std::string& srcComponent,
                                                   const Ufe::Attribute::Ptr& dstAttr,
                                                   const std::string& dstComponent);
    LOOKDEVX_USD_EXPORT ~UsdCreateConnectionCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdCreateConnectionCommand(const UsdCreateConnectionCommand&) = delete;
    UsdCreateConnectionCommand& operator=(const UsdCreateConnectionCommand&) = delete;
    UsdCreateConnectionCommand(UsdCreateConnectionCommand&&) = delete;
    UsdCreateConnectionCommand& operator=(UsdCreateConnectionCommand&&) = delete;

    //! Create a UsdCreateConnectionCommand object
    LOOKDEVX_USD_EXPORT static UsdCreateConnectionCommand::Ptr create(const Ufe::Attribute::Ptr& srcAttr,
                                                                      const std::string& srcComponent,
                                                                      const Ufe::Attribute::Ptr& dstAttr,
                                                                      const std::string& dstComponent);

    LOOKDEVX_USD_EXPORT void execute() override;
    LOOKDEVX_USD_EXPORT void undo() override;
    LOOKDEVX_USD_EXPORT void redo() override;

    [[nodiscard]] LOOKDEVX_USD_EXPORT std::shared_ptr<LookdevXUfe::ExtendedConnection> extendedConnection()
        const override;

    [[nodiscard]] LOOKDEVX_USD_EXPORT const std::unique_ptr<LookdevXUfe::AttributeComponentInfo>& srcInfo() const
    {
        return m_srcInfo;
    }

    [[nodiscard]] LOOKDEVX_USD_EXPORT const std::unique_ptr<LookdevXUfe::AttributeComponentInfo>& dstInfo() const
    {
        return m_dstInfo;
    }

protected:
    [[nodiscard]] LOOKDEVX_USD_EXPORT std::vector<std::string> componentNames(
        const Ufe::Attribute::Ptr& attr) const override;

private:
    MayaUsdAPI::UsdUndoableItem m_undoableItem;

    std::unique_ptr<LookdevXUfe::AttributeComponentInfo> m_srcInfo;
    std::unique_ptr<LookdevXUfe::AttributeComponentInfo> m_dstInfo;
};

//! \brief UsdUndoComponentConnectionCommand
class UsdDeleteConnectionCommand : public LookdevXUfe::DeleteConnectionCommand
{
public:
    using Ptr = std::shared_ptr<UsdDeleteConnectionCommand>;

    // Public for std::make_shared() access, use create() instead.
    LOOKDEVX_USD_EXPORT UsdDeleteConnectionCommand(const Ufe::Attribute::Ptr& srcAttr,
                                                   const std::string& srcComponent,
                                                   const Ufe::Attribute::Ptr& dstAttr,
                                                   const std::string& dstComponent);
    LOOKDEVX_USD_EXPORT ~UsdDeleteConnectionCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdDeleteConnectionCommand(const UsdDeleteConnectionCommand&) = delete;
    UsdDeleteConnectionCommand& operator=(const UsdDeleteConnectionCommand&) = delete;
    UsdDeleteConnectionCommand(UsdDeleteConnectionCommand&&) = delete;
    UsdDeleteConnectionCommand& operator=(UsdDeleteConnectionCommand&&) = delete;

    //! Create a UsdHiddenCommand object
    LOOKDEVX_USD_EXPORT static UsdDeleteConnectionCommand::Ptr create(const Ufe::Attribute::Ptr& srcAttr,
                                                                      const std::string& srcComponent,
                                                                      const Ufe::Attribute::Ptr& dstAttr,
                                                                      const std::string& dstComponent);

    LOOKDEVX_USD_EXPORT void execute() override;
    LOOKDEVX_USD_EXPORT void undo() override;
    LOOKDEVX_USD_EXPORT void redo() override;

protected:
    [[nodiscard]] LOOKDEVX_USD_EXPORT std::vector<std::string> componentNames(
        const Ufe::Attribute::Ptr& attr) const override;

private:
    MayaUsdAPI::UsdUndoableItem m_undoableItem;

    std::unique_ptr<LookdevXUfe::AttributeComponentInfo> m_srcInfo;
    std::unique_ptr<LookdevXUfe::AttributeComponentInfo> m_dstInfo;
};

} // namespace LookdevXUsd

#endif
