//
// Copyright 2019 Autodesk
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
#include "ProxyShapeHierarchy.h"

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usd/stage.h>

#include <ufe/log.h>
#include <ufe/pathComponent.h>
#include <ufe/pathSegment.h>
#include <ufe/rtid.h>

#include <cassert>
#include <stdexcept>

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdUndoCreateGroupCommand.h>
#include <mayaUsd/ufe/UsdUndoInsertChildCommand.h>
#include <mayaUsd/ufe/UsdUndoReorderCommand.h>
#endif

#ifdef UFE_V3_FEATURES_AVAILABLE
#include <mayaUsd/fileio/primUpdaterManager.h>

#include <ufe/pathString.h> // In UFE v2 but only needed for primUpdater.
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// We want to display the unloaded prims, so removed UsdPrimIsLoaded from
// the default UsdPrimDefaultPredicate.
// Note: UsdPrimIsActive is handled differently because pulled objects
//       are set inactive (to hide them from Rendering), so we handle
//       them differently.
const Usd_PrimFlagsConjunction MayaUsdPrimDefaultPredicate = UsdPrimIsDefined && !UsdPrimIsAbstract;

UsdPrimSiblingRange getUSDFilteredChildren(
    const UsdPrim&               prim,
    const Usd_PrimFlagsPredicate pred = MayaUsdPrimDefaultPredicate)
{
    // Since the equivalent of GetChildren is
    // GetFilteredChildren( UsdPrimDefaultPredicate ),
    // we will use that as the initial value.
    //
    return prim.GetFilteredChildren(pred);
}
} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
extern Ufe::Rtid g_USDRtid;

//------------------------------------------------------------------------------
// ProxyShapeHierarchy
//------------------------------------------------------------------------------

ProxyShapeHierarchy::ProxyShapeHierarchy(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler)
    : Ufe::Hierarchy()
    , fMayaHierarchyHandler(mayaHierarchyHandler)
{
}

ProxyShapeHierarchy::~ProxyShapeHierarchy() { }

/*static*/
ProxyShapeHierarchy::Ptr
ProxyShapeHierarchy::create(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler)
{
    return std::make_shared<ProxyShapeHierarchy>(mayaHierarchyHandler);
}

/*static*/
ProxyShapeHierarchy::Ptr ProxyShapeHierarchy::create(
    const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler,
    const Ufe::SceneItem::Ptr&        item)
{
    auto hierarchy = create(mayaHierarchyHandler);
    hierarchy->setItem(item);
    return hierarchy;
}

void ProxyShapeHierarchy::setItem(const Ufe::SceneItem::Ptr& item)
{
    // Our USD root prim is from the stage, which is from the item. So if we are
    // changing the item, it's possible that we won't have the same stage (and
    // thus the same root prim). To be safe, clear our stored root prim.
    if (fItem != item) {
        fUsdRootPrim = UsdPrim();
    }
    fItem = item;
    fMayaHierarchy = fMayaHierarchyHandler->hierarchy(item);
}

const UsdPrim& ProxyShapeHierarchy::getUsdRootPrim() const
{
    if (!fUsdRootPrim.IsValid()) {
        // FIXME During AL_usdmaya_ProxyShapeImport, nodes (both Maya
        // and USD) are being added (e.g. the proxy shape itself), but
        // there is no stage yet, and there is no way to detect that a
        // proxy shape import command is under way.  PPT, 28-Sep-2018.
        UsdStageWeakPtr stage = getStage(fItem->path());
        if (stage) {
            fUsdRootPrim = stage->GetPseudoRoot();
        }
    }
    return fUsdRootPrim;
}

//------------------------------------------------------------------------------
// Ufe::Hierarchy overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr ProxyShapeHierarchy::sceneItem() const { return fItem; }

#if (UFE_PREVIEW_VERSION_NUM >= 4004)

bool ProxyShapeHierarchy::hasChildren() const
{
    // We have an extra logic in createUFEChildList to remap and filter
    // prims. Going this direction is more costly, but easier to maintain.
    //
    // I don't have data that proves we need to worry about performance in here,
    // so going after maintainability.
    return !children().empty();
}

bool ProxyShapeHierarchy::hasFilteredChildren(const ChildFilter& childFilter) const
{
    // We have an extra logic in createUFEChildList to remap and filter
    // prims. Going this direction is more costly, but easier to maintain.
    //
    // I don't have data that proves we need to worry about performance in here,
    // so going after maintainability.
    return !filteredChildren(childFilter).empty();
}

#else

bool ProxyShapeHierarchy::hasChildren() const
{
    // Return children of the USD root.
    const UsdPrim& rootPrim = getUsdRootPrim();
    if (!rootPrim.IsValid())
        return false;

    // We have an extra logic in createUFEChildList to remap and filter
    // prims. Going this direction is more costly, but easier to maintain.
    //
    // I don't have data that proves we need to worry about performance in here,
    // so going after maintainability.
    return !createUFEChildList(getUSDFilteredChildren(rootPrim), false /*filterInactive*/).empty();
}

#endif

Ufe::SceneItemList ProxyShapeHierarchy::children() const
{
    // Return children of the USD root.
    const UsdPrim& rootPrim = getUsdRootPrim();
    if (!rootPrim.IsValid())
        return Ufe::SceneItemList();

    return createUFEChildList(getUSDFilteredChildren(rootPrim), true /*filterInactive*/);
}

#ifdef UFE_V2_FEATURES_AVAILABLE
Ufe::SceneItemList ProxyShapeHierarchy::filteredChildren(const ChildFilter& childFilter) const
{
    // Return filtered children of the USD root.
    const UsdPrim& rootPrim = getUsdRootPrim();
    if (!rootPrim.IsValid())
        return Ufe::SceneItemList();

    // Note: for now the only child filter flag we support is "Inactive Prims".
    //       See UsdHierarchyHandler::childFilter()
    if ((childFilter.size() == 1) && (childFilter.front().name == "InactivePrims")) {
        // See uniqueChildName() for explanation of USD filter predicate.
        const bool             showInactive = childFilter.front().value;
        Usd_PrimFlagsPredicate flags
            = showInactive ? UsdPrimIsDefined && !UsdPrimIsAbstract : MayaUsdPrimDefaultPredicate;
        return createUFEChildList(getUSDFilteredChildren(rootPrim, flags), !showInactive);
    }

    UFE_LOG("Unknown child filter");
    return Ufe::SceneItemList();
}
#endif

// Return UFE child list from input USD child list.
Ufe::SceneItemList
ProxyShapeHierarchy::createUFEChildList(const UsdPrimSiblingRange& range, bool filterInactive) const
{
    // We must create selection items for our children.  These will have as
    // path the path of the proxy shape, with a single path segment of a
    // single component appended to it.
    auto               parentPath = fItem->path();
    Ufe::SceneItemList children;
    UFE_V3(std::string dagPathStr;)
    for (const auto& child : range) {
#ifdef UFE_V3_FEATURES_AVAILABLE
        if (MAYAUSD_NS_DEF::readPullInformation(child, dagPathStr)) {
            auto item = Ufe::Hierarchy::createItem(Ufe::PathString::path(dagPathStr));
            // if we mapped to a valid object, insert it. it's possible that we got stale object
            // so in this case simply fallback to the usual processing of items
            if (item) {
                children.emplace_back(item);
                continue;
            }
        }
#endif
        if (!filterInactive || child.IsActive()) {
            children.emplace_back(UsdSceneItem::create(
                parentPath
                    + Ufe::PathSegment(
                        Ufe::PathComponent(child.GetName().GetString()), g_USDRtid, '/'),
                child));
        }
    }
    return children;
}

Ufe::SceneItem::Ptr ProxyShapeHierarchy::parent() const { return fMayaHierarchy->parent(); }

#ifndef UFE_V2_FEATURES_AVAILABLE
// UFE v1 specific method
Ufe::AppendedChild ProxyShapeHierarchy::appendChild(const Ufe::SceneItem::Ptr& child)
{
    throw std::runtime_error("ProxyShapeHierarchy::appendChild() not implemented");
}
#endif

#ifdef UFE_V2_FEATURES_AVAILABLE

Ufe::InsertChildCommand::Ptr ProxyShapeHierarchy::insertChildCmd(
    const Ufe::SceneItem::Ptr& child,
    const Ufe::SceneItem::Ptr& pos)
{
    // UsdUndoInsertChildCommand expects a UsdSceneItem which wraps a prim, so
    // create one using the pseudo-root and our own path.
    auto usdItem = UsdSceneItem::create(sceneItem()->path(), getUsdRootPrim());

    return UsdUndoInsertChildCommand::create(usdItem, downcast(child), downcast(pos));
}

Ufe::SceneItem::Ptr
ProxyShapeHierarchy::insertChild(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos)
{
    auto insertChildCommand = insertChildCmd(child, pos);
    return insertChildCommand->insertedChild();
}

#ifdef UFE_V3_FEATURES_AVAILABLE
Ufe::SceneItem::Ptr ProxyShapeHierarchy::createGroup(const Ufe::PathComponent& name) const
{
    Ufe::SceneItem::Ptr createdItem;

    auto usdItem = UsdSceneItem::create(sceneItem()->path(), getUsdRootPrim());
    UsdUndoCreateGroupCommand::Ptr cmd = UsdUndoCreateGroupCommand::create(usdItem, name.string());
    if (cmd) {
        cmd->execute();
        createdItem = cmd->insertedChild();
    }

    return createdItem;
}
#else
Ufe::SceneItem::Ptr ProxyShapeHierarchy::createGroup(
    const Ufe::Selection&     selection,
    const Ufe::PathComponent& name) const
{
    Ufe::SceneItem::Ptr createdItem;

    auto usdItem = UsdSceneItem::create(sceneItem()->path(), getUsdRootPrim());
    UsdUndoCreateGroupCommand::Ptr cmd
        = UsdUndoCreateGroupCommand::create(usdItem, selection, name.string());
    if (cmd) {
        cmd->execute();
        createdItem = cmd->insertedChild();
    }

    return createdItem;
}
#endif

#ifdef UFE_V3_FEATURES_AVAILABLE
Ufe::InsertChildCommand::Ptr
ProxyShapeHierarchy::createGroupCmd(const Ufe::PathComponent& name) const
{
    auto usdItem = UsdSceneItem::create(sceneItem()->path(), getUsdRootPrim());

    return UsdUndoCreateGroupCommand::create(usdItem, name.string());
}
#else
Ufe::UndoableCommand::Ptr ProxyShapeHierarchy::createGroupCmd(
    const Ufe::Selection&     selection,
    const Ufe::PathComponent& name) const
{
    auto usdItem = UsdSceneItem::create(sceneItem()->path(), getUsdRootPrim());

    return UsdUndoCreateGroupCommand::create(usdItem, selection, name.string());
}
#endif

Ufe::UndoableCommand::Ptr
ProxyShapeHierarchy::reorderCmd(const Ufe::SceneItemList& orderedList) const
{
    std::vector<TfToken> orderedTokens;

    for (const auto& item : orderedList) {
        orderedTokens.emplace_back(downcast(item)->prim().GetPath().GetNameToken());
    }

    // create a reorder command and pass in the parent and its ordered children list
    return UsdUndoReorderCommand::create(getUsdRootPrim(), orderedTokens);
}

Ufe::SceneItem::Ptr ProxyShapeHierarchy::defaultParent() const
{
    // The default parent is the root of the USD scene.
    // Since the proxy shape corresponds to the USD root prim,
    // therefore we implement the default parent as the proxy shape itself.
    return UsdSceneItem::create(sceneItem()->path(), getUsdRootPrim());
}

#endif // UFE_V2_FEATURES_AVAILABLE

#ifdef UFE_V3_FEATURES_AVAILABLE
Ufe::UndoableCommand::Ptr ProxyShapeHierarchy::ungroupCmd() const
{
    // pseudo root can not be ungrouped.
    return nullptr;
}
#endif // UFE_V3_FEATURES_AVAILABLE

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
