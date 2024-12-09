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

#include "Export.h"

#include "LookdevXUfe/SoloingHandler.h"

#include <ufe/hierarchy.h>

namespace LookdevXUsd
{

class UsdHierarchy : public Ufe::Hierarchy
{
public:
    using Ptr = std::shared_ptr<UsdHierarchy>;

    LOOKDEVX_USD_EXPORT explicit UsdHierarchy(Ufe::Hierarchy::Ptr wrappedUsdHierarchy)
        : m_wrappedUsdHierarchy(std::move(wrappedUsdHierarchy))
    {
    }

    LOOKDEVX_USD_EXPORT ~UsdHierarchy() override;

    UsdHierarchy(const UsdHierarchy&) = delete;
    UsdHierarchy& operator=(const UsdHierarchy&) = delete;
    UsdHierarchy(UsdHierarchy&&) = delete;
    UsdHierarchy& operator=(UsdHierarchy&&) = delete;

    LOOKDEVX_USD_EXPORT static UsdHierarchy::Ptr create(const Ufe::Hierarchy::Ptr& wrappedMayaUsdHierarchy)
    {
        return std::make_shared<UsdHierarchy>(wrappedMayaUsdHierarchy);
    }

    // Override to perform custom filtering.
    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::SceneItemList filteredChildren(const ChildFilter& filter) const override;

    // Forward all the rest.
    //-----------------------------------------------------------------------------------

    // LCOV_EXCL_START

    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::SceneItem::Ptr sceneItem() const override
    {
        return m_wrappedUsdHierarchy->sceneItem();
    }

    [[nodiscard]] LOOKDEVX_USD_EXPORT bool hasChildren() const override
    {
        return m_wrappedUsdHierarchy->hasChildren();
    }

    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::SceneItemList children() const override
    {
        return m_wrappedUsdHierarchy->children();
    }

    [[nodiscard]] LOOKDEVX_USD_EXPORT bool hasFilteredChildren(const ChildFilter& filter) const override
    {
        return m_wrappedUsdHierarchy->hasFilteredChildren(filter);
    }

    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::SceneItem::Ptr parent() const override
    {
        return m_wrappedUsdHierarchy->parent();
    }

    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::SceneItem::Ptr defaultParent() const override
    {
        return m_wrappedUsdHierarchy->defaultParent();
    }

    LOOKDEVX_USD_EXPORT Ufe::SceneItem::Ptr insertChild(const Ufe::SceneItem::Ptr& child,
                                                        const Ufe::SceneItem::Ptr& pos) override
    {
        return m_wrappedUsdHierarchy->insertChild(child, pos);
    }

    LOOKDEVX_USD_EXPORT Ufe::InsertChildCommand::Ptr insertChildCmd(const Ufe::SceneItem::Ptr& child,
                                                                    const Ufe::SceneItem::Ptr& pos) override
    {
        return m_wrappedUsdHierarchy->insertChildCmd(child, pos);
    }

    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::SceneItem::Ptr createGroup(const Ufe::PathComponent& name) const override
    {
        return m_wrappedUsdHierarchy->createGroup(name);
    }

    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::InsertChildCommand::Ptr createGroupCmd(
        const Ufe::PathComponent& name) const override
    {
        return m_wrappedUsdHierarchy->createGroupCmd(name);
    }

    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::UndoableCommand::Ptr reorderCmd(
        const Ufe::SceneItemList& orderedList) const override
    {
        return m_wrappedUsdHierarchy->reorderCmd(orderedList);
    }

    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::UndoableCommand::Ptr ungroupCmd() const override
    {
        return m_wrappedUsdHierarchy->ungroupCmd();
    }
    
    // LCOV_EXCL_STOP

    //-----------------------------------------------------------------------------------

private:
    const Ufe::Hierarchy::Ptr m_wrappedUsdHierarchy;

}; // UsdHierarchy

} // namespace LookdevXUsd
