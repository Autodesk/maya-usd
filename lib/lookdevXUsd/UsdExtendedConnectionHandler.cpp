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

#include "UsdExtendedConnectionHandler.h"
#include "UsdComponentConnections.h"
#include "UsdConnectionCommands.h"

#include <stdexcept>

namespace LookdevXUsd
{

UsdExtendedConnectionHandler::Ptr UsdExtendedConnectionHandler::create()
{
    return std::make_shared<UsdExtendedConnectionHandler>();
}

LookdevXUfe::ComponentConnections::Ptr UsdExtendedConnectionHandler::sourceComponentConnections(
    const Ufe::SceneItem::Ptr& item) const
{
    return UsdComponentConnections::create(item);
}

std::shared_ptr<LookdevXUfe::CreateConnectionResultCommand> UsdExtendedConnectionHandler::createConnectionCmd(
    const Ufe::Attribute::Ptr& srcAttr,
    const std::string& srcComponent,
    const Ufe::Attribute::Ptr& dstAttr,
    const std::string& dstComponent) const
{
    return UsdCreateConnectionCommand::create(srcAttr, srcComponent, dstAttr, dstComponent);
}

std::shared_ptr<LookdevXUfe::DeleteConnectionCommand> UsdExtendedConnectionHandler::deleteConnectionCmd(
    const Ufe::Attribute::Ptr& srcAttr,
    const std::string& srcComponent,
    const Ufe::Attribute::Ptr& dstAttr,
    const std::string& dstComponent) const
{
    return UsdDeleteConnectionCommand::create(srcAttr, srcComponent, dstAttr, dstComponent);
}

} // namespace LookdevXUsd
