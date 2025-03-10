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

#include "UsdConnectionCommands.h"

#include <LookdevXUfe/UfeUtils.h>

namespace LookdevXUsd
{

UsdCreateConnectionCommand::~UsdCreateConnectionCommand() = default;

// Public for std::make_shared() access, use create() instead.
UsdCreateConnectionCommand::UsdCreateConnectionCommand(const Ufe::Attribute::Ptr& srcAttr,
                                                       const std::string& srcComponent,
                                                       const Ufe::Attribute::Ptr& dstAttr,
                                                       const std::string& dstComponent)
    : m_srcInfo(std::make_unique<LookdevXUfe::AttributeComponentInfo>(srcAttr, srcComponent)),
      m_dstInfo(std::make_unique<LookdevXUfe::AttributeComponentInfo>(dstAttr, dstComponent))
{
}

UsdCreateConnectionCommand::Ptr UsdCreateConnectionCommand::create(const Ufe::Attribute::Ptr& srcAttr,
                                                                   const std::string& srcComponent,
                                                                   const Ufe::Attribute::Ptr& dstAttr,
                                                                   const std::string& dstComponent)
{
    if (!srcComponent.empty() || !dstComponent.empty())
    {
        throwIfSceneItemsNotComponentConnectable(srcAttr->sceneItem(), dstAttr->sceneItem());
    }

    const auto srcComponentNames = LookdevXUfe::UfeUtils::attributeComponentsAsStrings(srcAttr);
    if (!srcComponent.empty() &&
        std::find(srcComponentNames.begin(), srcComponentNames.end(), srcComponent) == srcComponentNames.end())
    {
        throw std::runtime_error("Connecting source attribute: '" + srcAttr->name() + "' component: '" + srcComponent +
                                 "' is currently unsupported.");
    }
    const auto dstComponentNames = LookdevXUfe::UfeUtils::attributeComponentsAsStrings(dstAttr);
    if (!dstComponent.empty() &&
        std::find(dstComponentNames.begin(), dstComponentNames.end(), dstComponent) == dstComponentNames.end())
    {
        throw std::runtime_error("Connecting destination attribute: '" + dstAttr->name() + "' component: '" +
                                 dstComponent + "' is currently unsupported.");
    }

    return std::make_shared<UsdCreateConnectionCommand>(srcAttr, srcComponent, dstAttr, dstComponent);
}

void UsdCreateConnectionCommand::execute()
{
    const MayaUsdAPI::UsdUndoBlock undoBlock(&m_undoableItem);

    createConnection(*m_srcInfo, *m_dstInfo);
}

void UsdCreateConnectionCommand::undo()
{
    m_undoableItem.undo();
}

void UsdCreateConnectionCommand::redo()
{
    m_undoableItem.redo();
}

std::shared_ptr<LookdevXUfe::ExtendedConnection> UsdCreateConnectionCommand::extendedConnection() const
{
    return std::make_shared<LookdevXUfe::ExtendedConnection>(*m_srcInfo, *m_dstInfo);
}

std::vector<std::string> UsdCreateConnectionCommand::componentNames(const Ufe::Attribute::Ptr& attr) const
{
    return LookdevXUfe::UfeUtils::attributeComponentsAsStrings(attr);
}

UsdDeleteConnectionCommand::~UsdDeleteConnectionCommand() = default;

UsdDeleteConnectionCommand::UsdDeleteConnectionCommand(const Ufe::Attribute::Ptr& srcAttr,
                                                       const std::string& srcComponent,
                                                       const Ufe::Attribute::Ptr& dstAttr,
                                                       const std::string& dstComponent)
    : m_srcInfo(std::make_unique<LookdevXUfe::AttributeComponentInfo>(srcAttr, srcComponent)),
      m_dstInfo(std::make_unique<LookdevXUfe::AttributeComponentInfo>(dstAttr, dstComponent))
{
}

UsdDeleteConnectionCommand::Ptr UsdDeleteConnectionCommand::create(const Ufe::Attribute::Ptr& srcAttr,
                                                                   const std::string& srcComponent,
                                                                   const Ufe::Attribute::Ptr& dstAttr,
                                                                   const std::string& dstComponent)
{
    return std::make_shared<UsdDeleteConnectionCommand>(srcAttr, srcComponent, dstAttr, dstComponent);
}

void UsdDeleteConnectionCommand::execute()
{
    const MayaUsdAPI::UsdUndoBlock undoBlock(&m_undoableItem);

    deleteConnection(*m_srcInfo, *m_dstInfo);
}

void UsdDeleteConnectionCommand::undo()
{
    m_undoableItem.undo();
}

void UsdDeleteConnectionCommand::redo()
{
    m_undoableItem.redo();
}

std::vector<std::string> UsdDeleteConnectionCommand::componentNames(const Ufe::Attribute::Ptr& attr) const
{
    return LookdevXUfe::UfeUtils::attributeComponentsAsStrings(attr);
}

} // namespace LookdevXUsd