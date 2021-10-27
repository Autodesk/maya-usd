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

#include "private/UfeNotifGuard.h"
#include "private/Utils.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsdUtils/util.h>

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

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdUndoCreateGroupCommand.h>
#include <mayaUsd/ufe/UsdUndoInsertChildCommand.h>
#include <mayaUsd/ufe/UsdUndoReorderCommand.h>
#endif

#ifdef UFE_V3_FEATURES_AVAILABLE
#include <mayaUsd/fileio/primUpdater.h>
#include <mayaUsd/ufe/UsdUndoUngroupCommand.h>
#include <ufe/pathString.h>     // In UFE v2 but only needed for primUpdater.
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
UsdPrimSiblingRange getUSDFilteredChildren(
    const MayaUsd::ufe::UsdSceneItem::Ptr usdSceneItem,
    const Usd_PrimFlagsPredicate          pred = UsdPrimDefaultPredicate)
{
    // If the scene item represents a point instance of a PointInstancer prim,
    // we consider it child-less. The namespace children of a PointInstancer
    // can only be accessed directly through the PointInstancer prim and not
    // through one of its point instances. Any authoring that would affect the
    // point instance should be done either to the PointInstancer or to the
    // prototype that is being instanced.
    if (usdSceneItem->isPointInstance()) {
        return UsdPrimSiblingRange();
    }

    const UsdPrim& prim = usdSceneItem->prim();

    // We need to be able to traverse down to instance proxies, so turn
    // on that part of the predicate, since by default, it is off. Since
    // the equivalent of GetChildren is
    // GetFilteredChildren( UsdPrimDefaultPredicate ),
    // we will use that as the initial value.
    //
    return prim.GetFilteredChildren(UsdTraverseInstanceProxies(pred));
}
} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdHierarchy::UsdHierarchy(const UsdSceneItem::Ptr& item)
    : Ufe::Hierarchy()
    , fItem(item)
{
}

UsdHierarchy::~UsdHierarchy() { }

/*static*/
UsdHierarchy::Ptr UsdHierarchy::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdHierarchy>(item);
}

void UsdHierarchy::setItem(const UsdSceneItem::Ptr& item) { fItem = item; }

const Ufe::Path& UsdHierarchy::path() const { return fItem->path(); }

UsdSceneItem::Ptr UsdHierarchy::usdSceneItem() const { return fItem; }

//------------------------------------------------------------------------------
// Ufe::Hierarchy overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdHierarchy::sceneItem() const { return fItem; }

bool UsdHierarchy::hasChildren() const { return !getUSDFilteredChildren(fItem).empty(); }

Ufe::SceneItemList UsdHierarchy::children() const
{
    return createUFEChildList(getUSDFilteredChildren(fItem));
}

#ifdef UFE_V2_FEATURES_AVAILABLE
Ufe::SceneItemList UsdHierarchy::filteredChildren(const ChildFilter& childFilter) const
{
    // Note: for now the only child filter flag we support is "Inactive Prims".
    //       See UsdHierarchyHandler::childFilter()
    if ((childFilter.size() == 1) && (childFilter.front().name == "InactivePrims")) {
        // See uniqueChildName() for explanation of USD filter predicate.
        Usd_PrimFlagsPredicate flags = childFilter.front().value
            ? UsdPrimIsDefined && !UsdPrimIsAbstract
            : UsdPrimDefaultPredicate;
        return createUFEChildList(getUSDFilteredChildren(fItem, flags));
    }

    UFE_LOG("Unknown child filter");
    return Ufe::SceneItemList();
}
#endif

// Return UFE child list from input USD child list.
Ufe::SceneItemList UsdHierarchy::createUFEChildList(const UsdPrimSiblingRange& range) const
{
    // Note that the calls to this function are given a range from
    // getUSDFilteredChildren() above, which ensures that when fItem is a
    // point instance of a PointInstancer, it will be child-less. As a result,
    // we expect to receive an empty range in that case, and will return an
    // empty scene item list as a result.
    Ufe::SceneItemList children;
    UFE_V3(std::string dagPathStr;)
    for (const auto& child : range) {
#ifdef UFE_V3_FEATURES_AVAILABLE
        if (PXR_NS::UsdMayaPrimUpdater::readPullInformation(child, dagPathStr)) {
            auto item = Ufe::Hierarchy::createItem(Ufe::PathString::path(dagPathStr));
            if (TF_VERIFY(item)) {
                children.emplace_back(item);
            }
        } else {
#endif
            children.emplace_back(UsdSceneItem::create(fItem->path() + child.GetName(), child));
#ifdef UFE_V3_FEATURES_AVAILABLE
        }
#endif
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
    return UsdSceneItem::create(fItem->path().pop(), prim().GetParent());
}

#ifndef UFE_V2_FEATURES_AVAILABLE
// UFE v1 specific method
Ufe::AppendedChild UsdHierarchy::appendChild(const Ufe::SceneItem::Ptr& child)
{
    auto usdChild = std::dynamic_pointer_cast<UsdSceneItem>(child);
#if !defined(NDEBUG)
    assert(usdChild);
#endif

    // First, check if we need to rename the child.
    std::string childName = uniqueChildName(fItem->prim(), child->path().back().string());

    // Set up all paths to perform the reparent.
    auto           childPrim = usdChild->prim();
    auto           stage = childPrim.GetStage();
    auto           ufeSrcPath = usdChild->path();
    auto           usdSrcPath = childPrim.GetPath();
    auto           ufeDstPath = fItem->path() + childName;
    auto           usdDstPath = prim().GetPath().AppendChild(TfToken(childName));
    SdfLayerHandle layer = MayaUsdUtils::defPrimSpecLayer(childPrim);
    if (!layer) {
        std::string err = TfStringPrintf("No prim found at %s", usdSrcPath.GetString().c_str());
        throw std::runtime_error(err.c_str());
    }

    // In USD, reparent is implemented like rename, using copy to
    // destination, then remove from source.
    // See UsdUndoRenameCommand._rename comments for details.
    InPathChange pc;

    auto status = SdfCopySpec(layer, usdSrcPath, layer, usdDstPath);
    if (!status) {
        std::string err = TfStringPrintf(
            "Appending child %s to parent %s failed.",
            ufeSrcPath.string().c_str(),
            fItem->path().string().c_str());
        throw std::runtime_error(err.c_str());
    }

    stage->RemovePrim(usdSrcPath);
    auto ufeDstItem = UsdSceneItem::create(ufeDstPath, ufePathToPrim(ufeDstPath));

    sendNotification<Ufe::ObjectReparent>(ufeDstItem, ufeSrcPath);

    // FIXME  No idea how to get the child prim index yet.  PPT, 16-Aug-2018.
    return Ufe::AppendedChild(ufeDstItem, ufeSrcPath, 0);
}
#endif

#ifdef UFE_V2_FEATURES_AVAILABLE
Ufe::InsertChildCommand::Ptr
UsdHierarchy::insertChildCmd(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos)
{
    return UsdUndoInsertChildCommand::create(fItem, downcast(child), downcast(pos));
}

Ufe::SceneItem::Ptr
UsdHierarchy::insertChild(const Ufe::SceneItem::Ptr& child, const Ufe::SceneItem::Ptr& pos)
{
    auto insertChildCommand = insertChildCmd(child, pos);
    return insertChildCommand->insertedChild();
}

// Create a transform.
#if (UFE_PREVIEW_VERSION_NUM >= 3005)
Ufe::SceneItem::Ptr UsdHierarchy::createGroup(const Ufe::PathComponent& name) const
{
    Ufe::SceneItem::Ptr createdItem = nullptr;

    UsdUndoCreateGroupCommand::Ptr cmd = UsdUndoCreateGroupCommand::create(fItem, name.string());
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
        = UsdUndoCreateGroupCommand::create(fItem, selection, name.string());
    if (cmd) {
        cmd->execute();
        createdItem = cmd->insertedChild();
    }

    return createdItem;
}
#endif

#if (UFE_PREVIEW_VERSION_NUM >= 3001)
Ufe::InsertChildCommand::Ptr
#else
Ufe::UndoableCommand::Ptr
#endif

#if (UFE_PREVIEW_VERSION_NUM >= 3005)
UsdHierarchy::createGroupCmd(const Ufe::PathComponent& name) const
{
    return UsdUndoCreateGroupCommand::create(fItem, name.string());
}
#else
UsdHierarchy::createGroupCmd(const Ufe::Selection& selection, const Ufe::PathComponent& name) const
{
    return UsdUndoCreateGroupCommand::create(fItem, selection, name.string());
}
#endif

Ufe::SceneItem::Ptr UsdHierarchy::defaultParent() const
{
    // Default parent for USD nodes is the pseudo-root of their stage, which is
    // represented by the proxy shape.
    auto path = fItem->path();
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
    return UsdUndoReorderCommand::create(downcast(sceneItem())->prim(), orderedTokens);
}
#endif // UFE_V2_FEATURES_AVAILABLE

#ifdef UFE_V3_FEATURES_AVAILABLE
Ufe::UndoableCommand::Ptr UsdHierarchy::ungroupCmd() const
{
    return UsdUndoUngroupCommand::create(fItem);
}
#endif // UFE_V3_FEATURES_AVAILABLE

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
