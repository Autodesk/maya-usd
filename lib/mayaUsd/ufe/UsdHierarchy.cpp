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

#include <cassert>
#include <stdexcept>

#include <ufe/scene.h>
#include <ufe/sceneNotification.h>
#include <ufe/log.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/base/tf/stringUtils.h>

#include <mayaUsd/ufe/Utils.h>

#include <mayaUsdUtils/util.h>

#include "private/UfeNotifGuard.h"
#include "private/Utils.h"

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdUndoCreateGroupCommand.h>
#include <mayaUsd/ufe/UsdUndoInsertChildCommand.h>
#endif

namespace {
	UsdPrimSiblingRange getUSDFilteredChildren(const UsdPrim& prim, const Usd_PrimFlagsPredicate pred = UsdPrimDefaultPredicate)
	{
		// We need to be able to traverse down to instance proxies, so turn
		// on that part of the predicate, since by default, it is off. Since
		// the equivalent of GetChildren is
		// GetFilteredChildren( UsdPrimDefaultPredicate ),
		// we will use that as the initial value.
		//
		return prim.GetFilteredChildren(UsdTraverseInstanceProxies(pred));
	}
}

MAYAUSD_NS_DEF {
namespace ufe {

UsdHierarchy::UsdHierarchy(const UsdSceneItem::Ptr& item)
	: Ufe::Hierarchy()
    , fItem(item)
{
}

UsdHierarchy::~UsdHierarchy()
{
}

/*static*/
UsdHierarchy::Ptr UsdHierarchy::create(const UsdSceneItem::Ptr& item)
{
	return std::make_shared<UsdHierarchy>(item);
}

void UsdHierarchy::setItem(const UsdSceneItem::Ptr& item)
{
	fItem = item;
}

const Ufe::Path& UsdHierarchy::path() const
{
	return fItem->path();
}

UsdSceneItem::Ptr UsdHierarchy::usdSceneItem() const
{
	return fItem;
}

//------------------------------------------------------------------------------
// Ufe::Hierarchy overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdHierarchy::sceneItem() const
{
	return fItem;
}

bool UsdHierarchy::hasChildren() const
{
	return !getUSDFilteredChildren(prim()).empty();
}

Ufe::SceneItemList UsdHierarchy::children() const
{
    return createUFEChildList(getUSDFilteredChildren(prim()));
}

#ifdef UFE_V2_FEATURES_AVAILABLE
#if UFE_PREVIEW_VERSION_NUM >= 2022
Ufe::SceneItemList UsdHierarchy::filteredChildren(const ChildFilter& childFilter) const
{
    // Note: for now the only child filter flag we support is "Inactive Prims".
    //       See UsdHierarchyHandler::childFilter()
    if ((childFilter.size() == 1) && (childFilter.front().name == "InactivePrims"))
    {
        // See uniqueChildName() for explanation of USD filter predicate.
        Usd_PrimFlagsPredicate flags = childFilter.front().value ? UsdPrimIsDefined && !UsdPrimIsAbstract
                                                                 : UsdPrimDefaultPredicate;
        return createUFEChildList(getUSDFilteredChildren(prim(), flags));
    }

    UFE_LOG("Unknown child filter");
    return Ufe::SceneItemList();
}
#endif
#endif

Ufe::SceneItemList UsdHierarchy::createUFEChildList(const UsdPrimSiblingRange& range) const
{
    // Return UFE child list from input USD child list.
    Ufe::SceneItemList children;
    for (const auto& child : range)
    {
        children.emplace_back(UsdSceneItem::create(fItem->path() + child.GetName(), child));
    }
    return children;
}

Ufe::SceneItem::Ptr UsdHierarchy::parent() const
{
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
	auto childPrim = usdChild->prim();
	auto stage = childPrim.GetStage();
	auto ufeSrcPath = usdChild->path();
	auto usdSrcPath = childPrim.GetPath();
	auto ufeDstPath = fItem->path() + childName;
	auto usdDstPath = prim().GetPath().AppendChild(TfToken(childName));
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
		std::string err = TfStringPrintf("Appending child %s to parent %s failed.",
						ufeSrcPath.string().c_str(), fItem->path().string().c_str());
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
Ufe::InsertChildCommand::Ptr UsdHierarchy::insertChildCmd(
    const Ufe::SceneItem::Ptr& child,
    const Ufe::SceneItem::Ptr& pos
)
{
    return UsdUndoInsertChildCommand::create(
        fItem, downcast(child), downcast(pos));
}

Ufe::SceneItem::Ptr UsdHierarchy::insertChild(
        const Ufe::SceneItem::Ptr& ,
        const Ufe::SceneItem::Ptr& 
)
{
    // Should be possible to implement trivially when support for returning the
    // result of the parent command (MAYA-105278) is implemented.  For now,
    // Ufe::Hierarchy::insertChildCmd() returns a base class
    // Ufe::UndoableCommand::Ptr object, from which we can't retrieve the added
    // child.  PPT, 13-Jul-2020.
    return nullptr;
}

// Create a transform.
Ufe::SceneItem::Ptr UsdHierarchy::createGroup(const Ufe::Selection& selection, const Ufe::PathComponent& name) const
{
	Ufe::SceneItem::Ptr createdItem = nullptr;

	UsdUndoCreateGroupCommand::Ptr cmd = UsdUndoCreateGroupCommand::create(fItem, selection, name.string());
	if (cmd) {
		cmd->execute();
		createdItem = cmd->group();
	}

	return createdItem;
}

Ufe::UndoableCommand::Ptr UsdHierarchy::createGroupCmd(const Ufe::Selection& selection, const Ufe::PathComponent& name) const
{
	return UsdUndoCreateGroupCommand::create(fItem, selection, name.string());
}

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

#endif // UFE_V2_FEATURES_AVAILABLE

} // namespace ufe
} // namespace MayaUsd
