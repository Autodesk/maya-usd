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

#include "UsdHierarchyHandler.h"
#include "UsdHierarchy.h"

#include <ufe/runTimeMgr.h>

#ifdef UFE_HIERARCHY_HAS_FIND
#include "LookdevXUfe/SceneItemUI.h"

#include <usdUfe/ufe/UsdSceneItem.h>

#include <unordered_map>
#include <vector>
#endif

namespace
{
#ifdef UFE_HIERARCHY_HAS_FIND

bool isHidden(const Ufe::SceneItem::Ptr& item)
{
    const auto sceneItemUI = LookdevXUfe::SceneItemUI::sceneItemUI(item);
    return sceneItemUI && sceneItemUI->hidden();
}

// Whether the item, or one of its ancestors of the same USD path segment below searchRoot, is hidden.
// hiddenCache memoizes that result for the paths visited along the way.
bool isHiddenFromSearch(const Ufe::SceneItem::Ptr& item,
                        const Ufe::Path& searchRoot,
                        std::unordered_map<Ufe::Path, bool>& hiddenCache)
{
    const auto usdItem = std::dynamic_pointer_cast<UsdUfe::UsdSceneItem>(item);
    if (!usdItem)
    {
        return isHidden(item);
    }

    const auto nbSegments = item->path().nbSegments();
    std::vector<Ufe::Path> visited;
    bool hidden = false;
    Ufe::SceneItem::Ptr current = item;
    Ufe::Path path = item->path();
    PXR_NS::UsdPrim prim = usdItem->prim();
    while (true)
    {
        const auto cached = hiddenCache.find(path);
        if (cached != hiddenCache.end())
        {
            hidden = cached->second;
            break;
        }
        visited.push_back(path);
        if (isHidden(current))
        {
            hidden = true;
            break;
        }

        path = path.pop();
        prim = prim.GetParent();
        if (path == searchRoot || path.nbSegments() != nbSegments || !prim || prim.IsPseudoRoot())
        {
            break;
        }
        current = UsdUfe::UsdSceneItem::create(path, prim);
    }

    for (const auto& visitedPath : visited)
    {
        hiddenCache.emplace(visitedPath, hidden);
    }
    return hidden;
}

#endif
} // namespace

namespace LookdevXUsd
{

Ufe::HierarchyHandler::Ptr UsdHierarchyHandler::m_wrappedHierarchyHandler;
Ufe::Rtid UsdHierarchyHandler::m_rtid;

UsdHierarchyHandler::~UsdHierarchyHandler() = default;

void UsdHierarchyHandler::registerHandler(const Ufe::Rtid& rtId)
{
    if (m_wrappedHierarchyHandler)
    {
        return;
    }

    m_rtid = rtId;
    auto& runTimeMgr = Ufe::RunTimeMgr::instance();
    m_wrappedHierarchyHandler = runTimeMgr.hierarchyHandler(m_rtid);
    runTimeMgr.setHierarchyHandler(m_rtid, std::make_shared<UsdHierarchyHandler>());
}

void UsdHierarchyHandler::unregisterHandler()
{
    if (m_wrappedHierarchyHandler)
    {
        auto& runTimeMgr = Ufe::RunTimeMgr::instance();
        if (runTimeMgr.hasId(m_rtid))
        {
            runTimeMgr.setHierarchyHandler(m_rtid, m_wrappedHierarchyHandler);
        }
        m_wrappedHierarchyHandler.reset();
    }
}

Ufe::Hierarchy::Ptr UsdHierarchyHandler::hierarchy(const Ufe::SceneItem::Ptr& item) const
{
    return UsdHierarchy::create(m_wrappedHierarchyHandler->hierarchy(item));
}

#ifdef UFE_HIERARCHY_HAS_FIND
Ufe::Selection UsdHierarchyHandler::find(Ufe::Rtid runTimeId,
                                         const Ufe::Path& searchRoot,
                                         const Ufe::Hierarchy::NameMatchFcn& matches,
                                         const Ufe::Hierarchy::ChildFilter& childFilter,
                                         const Ufe::Hierarchy::FindInterruptFcn& interrupt) const
{
    const Ufe::Selection found =
        m_wrappedHierarchyHandler->find(runTimeId, searchRoot, matches, childFilter, interrupt);

    // UsdHierarchy::filteredChildren() hides items, so neither they nor their descendants are reachable.
    std::unordered_map<Ufe::Path, bool> hiddenCache;
    Ufe::Selection visible;
    for (const auto& item : found)
    {
        if (!isHiddenFromSearch(item, searchRoot, hiddenCache))
        {
            visible.append(item);
        }
    }
    return visible;
}
#endif

} // namespace LookdevXUsd
