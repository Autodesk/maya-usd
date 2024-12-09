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
#ifndef USD_EXTENDED_CONNECTION_HANDLER_H
#define USD_EXTENDED_CONNECTION_HANDLER_H

#include "Export.h"

#include <LookdevXUfe/ExtendedConnectionHandler.h>

namespace LookdevXUsd
{

class LOOKDEVX_USD_EXPORT UsdExtendedConnectionHandler : public LookdevXUfe::ExtendedConnectionHandler
{
public:
    using Ptr = std::shared_ptr<UsdExtendedConnectionHandler>;

    //! Constructor.
    UsdExtendedConnectionHandler() = default;
    //! Destructor.
    ~UsdExtendedConnectionHandler() override = default;

    //@{
    // Delete the copy/move constructors assignment operators.
    UsdExtendedConnectionHandler(const UsdExtendedConnectionHandler&) = delete;
    UsdExtendedConnectionHandler& operator=(const UsdExtendedConnectionHandler&) = delete;
    UsdExtendedConnectionHandler(UsdExtendedConnectionHandler&&) = delete;
    UsdExtendedConnectionHandler& operator=(UsdExtendedConnectionHandler&&) = delete;
    //@}

    static UsdExtendedConnectionHandler::Ptr create();

    [[nodiscard]] LookdevXUfe::ComponentConnections::Ptr sourceComponentConnections(
        const Ufe::SceneItem::Ptr& item) const override;

    [[nodiscard]] std::shared_ptr<LookdevXUfe::CreateConnectionResultCommand> createConnectionCmd(
        const Ufe::Attribute::Ptr& srcAttr,
        const std::string& srcComponent,
        const Ufe::Attribute::Ptr& dstAttr,
        const std::string& dstComponent) const override;

    [[nodiscard]] std::shared_ptr<LookdevXUfe::DeleteConnectionCommand> deleteConnectionCmd(
        const Ufe::Attribute::Ptr& srcAttr,
        const std::string& srcComponent,
        const Ufe::Attribute::Ptr& dstAttr,
        const std::string& dstComponent) const override;
}; // UsdExtendedConnectionHandler

} // namespace LookdevXUsd

#endif
