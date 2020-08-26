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
	UsdPrimSiblingRange filteredChildren( const UsdPrim& prim )
	{
		// We need to be able to traverse down to instance proxies, so turn
		// on that part of the predicate, since by default, it is off. Since
		// the equivalent of GetChildren is
		// GetFilteredChildren( UsdPrimDefaultPredicate ),
		// we will use that as the initial value.
		//
		Usd_PrimFlagsPredicate predicate = UsdPrimDefaultPredicate;
		predicate = predicate.TraverseInstanceProxies(true);
		return prim.GetFilteredChildren(predicate);
	}

}

MAYAUSD_NS_DEF {
namespace ufe {

UsdHierarchy::UsdHierarchy(const UsdSceneItem::Ptr& item)
	: Ufe::Hierarchy(), fItem(item), fPrim(item->prim())
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
	fPrim = item->prim();
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
	return !filteredChildren(fPrim).empty();
}

Ufe::SceneItemList UsdHierarchy::children() const
{
	// Return USD children only, i.e. children within this run-time.
	Ufe::SceneItemList children;
	for (auto child : filteredChildren(fPrim))
	{
		children.emplace_back(UsdSceneItem::create(fItem->path() + child.GetName(), child));
	}
	return children;
}

Ufe::SceneItem::Ptr UsdHierarchy::parent() const
{
	return UsdSceneItem::create(fItem->path().pop(), fPrim.GetParent());
}

#ifdef UFE_V2_FEATURES_AVAILABLE

Ufe::UndoableCommand::Ptr UsdHierarchy::insertChildCmd(
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
