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

#include <ufe/hierarchyHandler.h>
#include <ufe/rtid.h>

#include "UsdHierarchy.h"

namespace LookdevXUsd
{
class UsdHierarchyHandler : public Ufe::HierarchyHandler
{
public:
    using Ptr = std::shared_ptr<UsdHierarchyHandler>;

    LOOKDEVX_USD_EXPORT UsdHierarchyHandler() = default;
    LOOKDEVX_USD_EXPORT ~UsdHierarchyHandler() override;

    UsdHierarchyHandler(const UsdHierarchyHandler&) = delete;
    UsdHierarchyHandler& operator=(const UsdHierarchyHandler&) = delete;
    UsdHierarchyHandler(UsdHierarchyHandler&&) = delete;
    UsdHierarchyHandler& operator=(UsdHierarchyHandler&&) = delete;

    LOOKDEVX_USD_EXPORT static void registerHandler(const Ufe::Rtid& rtId);
    LOOKDEVX_USD_EXPORT static void unregisterHandler();

    // Override to return custom hierarchy.
    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::Hierarchy::Ptr hierarchy(const Ufe::SceneItem::Ptr& item) const override;

    // Forward all the rest.
    //-----------------------------------------------------------------------------------

    // LCOV_EXCL_START

    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::SceneItem::Ptr createItem(const Ufe::Path& path) const override
    {
        return m_wrappedHierarchyHandler->createItem(path);
    }

    [[nodiscard]] LOOKDEVX_USD_EXPORT Ufe::Hierarchy::ChildFilter childFilter() const override
    {
        return m_wrappedHierarchyHandler->childFilter();
    }

    // LCOV_EXCL_STOP

    //-----------------------------------------------------------------------------------

private:
    static Ufe::HierarchyHandler::Ptr m_wrappedHierarchyHandler;
    static Ufe::Rtid m_rtid;

}; // UsdHierarchyHandler

} // namespace LookdevXUsd
