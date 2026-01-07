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
#include "UsdHierarchy.h"

#include <usdUfe/ufe/UsdUndoCreateGroupCommand.h>
#include <usdUfe/ufe/UsdUndoInsertChildCommand.h>
#include <usdUfe/ufe/UsdUndoReorderCommand.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/xform.h>

#include <ufe/log.h>
#include <ufe/path.h>
#include <ufe/pathComponent.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

#include <cassert>
#include <stdexcept>
#include <string>

#ifdef UFE_V3_FEATURES_AVAILABLE
#include <usdUfe/ufe/UsdUndoUngroupCommand.h>

#include <ufe/pathString.h> // In UFE v2 but only needed for primUpdater.
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// We want to display the unloaded prims, so removed UsdPrimIsLoaded from
// the default UsdPrimDefaultPredicate.
// Note: UsdPrimIsActive is handled differently because pulled objects
//       are set inactive (to hide them from Rendering), so we handle
//       them differently.
const Usd_PrimFlagsConjunction kUsdUfePrimDefaultPredicate = UsdPrimIsDefined && !UsdPrimIsAbstract;

UsdPrimSiblingRange invalidSiblingRange()
{
    // Note: we explicitly create a range using the *same* underlying iterator instance
    //       to ensure the range can be detected as being empty.
    //
    //       Normally, we would simply return a default-constructed range, but there
    //       is a bug in USD that default-constructed ranges create invalid object
    //       containing uninitialized pointers that are very likely to crash, because
    //       they will *not* be interpreted as an empty range because the uninitialized
    //       pointers are *very unlikely* to have the same value and "look" like identical
    //       iterators. Most of the times they will have different value and thus look like
    //       a large range pointing at random locations and crash on use.
    //
    //       The low-level cause is that the range class in based on boost adaptors, which
    //       take as template argument the type of the underlying iterator. Unfortunately,
    //       USD uses raw pointers as the underlying iterator type. Also unfortunately,
    //       boost adaptor default constructor does nothing, which mean its member variable
    //       use the default constructor, which is a no-op for raw pointers. Boost should
    //       have used the "member_var {}" trick to initialize its members to proper
    //       default values even in the presence of pointers.
    //
    //       So we instead explicitly build a range using the same underlying iterator instance.
    //       This iterator will also contain an uninitialized pointer (for the same reason as
    //       above, a boost iterator adaptor problem), but since it will be the same pointer
    //       in the begin and end iterator, they will look like an empty range and prevent any
    //       algorithm from deferencing the invalid pointer.
    //
    //       And to be even safer, we use a static variable, which ensures that the uninitialized
    //       raw pointer will be null.
    static const UsdPrimSiblingIterator empty;
    return UsdPrimSiblingRange(empty, empty);
}

UsdPrimSiblingRange getUSDFilteredChildren(
    const UsdUfe::UsdSceneItem::Ptr usdSceneItem,
    const Usd_PrimFlagsPredicate    pred = kUsdUfePrimDefaultPredicate)
{
    // If the scene item represents a point instance of a PointInstancer prim,
    // we consider it child-less. The namespace children of a PointInstancer
    // can only be accessed directly through the PointInstancer prim and not
    // through one of its point instances. Any authoring that would affect the
    // point instance should be done either to the PointInstancer or to the
    // prototype that is being instanced.
    if (usdSceneItem->isPointInstance()) {
        return invalidSiblingRange();
    }

    const UsdPrim& prim = usdSceneItem->prim();
    if (!prim.IsValid())
        return invalidSiblingRange();

    // We need to be able to traverse down to instance proxies, so turn
    // on that part of the predicate, since by default, it is off. Since
    // the equivalent of GetChildren is
    // GetFilteredChildren( UsdPrimDefaultPredicate ),
    // we will use that as the initial value.
    //
    return prim.GetFilteredChildren(UsdTraverseInstanceProxies(pred));
}
} // namespace

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::Hierarchy, UsdHierarchy);

UsdHierarchy::UsdHierarchy(const UsdSceneItem::Ptr& item)
    : Ufe::Hierarchy()
    , _item(item)
{
}

/*static*/
UsdHierarchy::Ptr UsdHierarchy::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdHierarchy>(item);
}

void UsdHierarchy::setItem(const UsdSceneItem::Ptr& item) { _item = item; }

const Ufe::Path& UsdHierarchy::path() const { return _item->path(); }

UsdSceneItem::Ptr UsdHierarchy::usdSceneItem() const { return _item; }

/*static*/
Usd_PrimFlagsConjunction UsdHierarchy::usdUfePrimDefaultPredicate()
{
    return kUsdUfePrimDefaultPredicate;
}

//------------------------------------------------------------------------------
// Ufe::Hierarchy overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdHierarchy::sceneItem() const { return _item; }

#ifdef UFE_V4_FEATURES_AVAILABLE

bool UsdHierarchy::hasChildren() const
{
    // We have an extra logic in createUFEChildList to remap and filter
    // prims. Going this direction is more costly, but easier to maintain.
    //
    // I don't have data that proves we need to worry about performance in here,
    // so going after maintainability.
    return !children().empty();
}

bool UsdHierarchy::hasFilteredChildren(const ChildFilter& childFilter) const
{
    // We have an extra logic in createUFEChildList to remap and filter
    // prims. Going this direction is more costly, but easier to maintain.
    //
    // I don't have data that proves we need to worry about performance in here,
    // so going after maintainability.
    return !filteredChildren(childFilter).empty();
}

#else

bool UsdHierarchy::hasChildren() const
{
    // We have an extra logic in createUFEChildList to remap and filter
    // prims. Going this direction is more costly, but easier to maintain.
    //
    // I don't have data that proves we need to worry about performance in here,
    // so going after maintainability.
    const bool isFilteringInactive = false;
    return !createUFEChildList(getUSDFilteredChildren(_item), isFilteringInactive).empty();
}

#endif

Ufe::SceneItemList UsdHierarchy::children() const
{
    return createUFEChildList(getUSDFilteredChildren(_item), true /*filterInactive*/);
}

Ufe::SceneItemList UsdHierarchy::filteredChildren(const ChildFilter& childFilter) const
{
    Usd_PrimFlagsPredicate flags = UsdUfe::getUsdPredicate(childFilter);
    return createUFEChildList(getUSDFilteredChildren(_item, flags), false);
}

bool UsdHierarchy::childrenHook(
    const PXR_NS::UsdPrim& child,
    Ufe::SceneItemList&    children,
    bool                   filterInactive) const
{
    return false;
}

// Return UFE child list from input USD child list.
Ufe::SceneItemList
UsdHierarchy::createUFEChildList(const UsdPrimSiblingRange& range, bool filterInactive) const
{
    // Note that the calls to this function are given a range from
    // getUSDFilteredChildren() above, which ensures that when fItem is a
    // point instance of a PointInstancer, it will be child-less. As a result,
    // we expect to receive an empty range in that case, and will return an
    // empty scene item list as a result.
    Ufe::SceneItemList children;
    for (const auto& child : range) {
        // Give derived classes a chance to process this child.
        if (childrenHook(child, children, filterInactive))
            continue;

        if (!filterInactive || child.IsActive()) {
            children.emplace_back(UsdSceneItem::create(_item->path() + child.GetName(), child));
        }
    }
    return children;
}

Ufe::SceneItem::Ptr UsdHierarchy::parent() const
{
    // We do not have a special case for point instances here. If fItem
    // represents a point instance of a PointInstancer, we consider the
    // PointInstancer prim to be the "parent" of the point instance, even
    // though this isn't really true in the USD sense. This allows pick-walking
    // from point instances up to their PointInstancer.
    UsdPrim p = prim();
    if (p.IsValid())
        return UsdSceneItem::create(_item->path().pop(), p.GetParent());
    else
        return Hierarchy::createItem(_item->path().pop());
}

Ufe::InsertChildCommand::Ptr
UsdHierarchy::insertChildCmd(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos)
{
    // Changing the hierarchy of inactive items is not allowed.
    if (!_item->prim().IsActive())
        return nullptr;

    return UsdUndoInsertChildCommand::create(_item, downcast(child), downcast(pos));
}

Ufe::SceneItem::Ptr
UsdHierarchy::insertChild(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos)
{
    auto insertChildCommand = insertChildCmd(child, pos);
    if (!insertChildCommand)
        return nullptr;

    return insertChildCommand->insertedChild();
}

// Create a transform.
#ifdef UFE_V3_FEATURES_AVAILABLE
Ufe::SceneItem::Ptr UsdHierarchy::createGroup(const Ufe::PathComponent& name) const
{
    Ufe::SceneItem::Ptr createdItem = nullptr;

    UsdUndoCreateGroupCommand::Ptr cmd = UsdUndoCreateGroupCommand::create(_item, name.string());
    if (cmd) {
        cmd->execute();
        createdItem = cmd->insertedChild();
    }

    return createdItem;
}
#else
Ufe::SceneItem::Ptr
UsdHierarchy::createGroup(const Ufe::Selection& selection, const Ufe::PathComponent& name) const
{
    Ufe::SceneItem::Ptr createdItem = nullptr;

    UsdUndoCreateGroupCommand::Ptr cmd
        = UsdUndoCreateGroupCommand::create(_item, selection, name.string());
    if (cmd) {
        cmd->execute();
        createdItem = cmd->insertedChild();
    }

    return createdItem;
}
#endif

#ifdef UFE_V3_FEATURES_AVAILABLE
Ufe::InsertChildCommand::Ptr UsdHierarchy::createGroupCmd(const Ufe::PathComponent& name) const
{
    return UsdUndoCreateGroupCommand::create(_item, name.string());
}
#else
Ufe::UndoableCommand::Ptr
UsdHierarchy::createGroupCmd(const Ufe::Selection& selection, const Ufe::PathComponent& name) const
{
    return UsdUndoCreateGroupCommand::create(_item, selection, name.string());
}
#endif

Ufe::SceneItem::Ptr UsdHierarchy::defaultParent() const
{
    // Default parent for USD nodes is the pseudo-root of their stage, which is
    // represented by the proxy shape.
    auto path = _item->path();
#if !defined(NDEBUG)
    assert(path.nbSegments() == 2);
#endif
    auto proxyShapePath = path.popSegment();
    return createItem(proxyShapePath);
}

Ufe::UndoableCommand::Ptr UsdHierarchy::reorderCmd(const Ufe::SceneItemList& orderedList) const
{
    std::vector<TfToken> orderedTokens;

    for (const auto& item : orderedList) {
        orderedTokens.emplace_back(downcast(item)->prim().GetPath().GetNameToken());
    }

    // create a reorder command and pass in the parent and its reordered children list
    return UsdUndoReorderCommand::create(_item->prim(), orderedTokens);
}

#ifdef UFE_V3_FEATURES_AVAILABLE
Ufe::UndoableCommand::Ptr UsdHierarchy::ungroupCmd() const
{
    return UsdUndoUngroupCommand::create(_item);
}
#endif // UFE_V3_FEATURES_AVAILABLE

} // namespace USDUFE_NS_DEF
