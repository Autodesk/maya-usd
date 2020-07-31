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
#if UFE_PREVIEW_VERSION_NUM >= 2013
#include <mayaUsd/ufe/UsdUndoInsertChildCommand.h>
#endif
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

#if UFE_PREVIEW_VERSION_NUM < 2018
Ufe::AppendedChild UsdHierarchy::appendChild(const Ufe::SceneItem::Ptr& child)
{
	auto usdChild = std::dynamic_pointer_cast<UsdSceneItem>(child);
#if !defined(NDEBUG)
	assert(usdChild);
#endif

	// First, check if we need to rename the child.
	std::string childName = uniqueChildName(fItem, child->path());

	// Set up all paths to perform the reparent.
	auto prim = usdChild->prim();
	auto stage = prim.GetStage();
	auto ufeSrcPath = usdChild->path();
	auto usdSrcPath = prim.GetPath();
	auto ufeDstPath = fItem->path() + childName;
	auto usdDstPath = fPrim.GetPath().AppendChild(TfToken(childName));
	SdfLayerHandle layer = MayaUsdUtils::defPrimSpecLayer(prim);
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
#if UFE_PREVIEW_VERSION_NUM >= 2013
Ufe::UndoableCommand::Ptr UsdHierarchy::insertChildCmd(
    const Ufe::SceneItem::Ptr& child,
    const Ufe::SceneItem::Ptr& pos
)
{
    return UsdUndoInsertChildCommand::create(
        fItem, downcast(child), downcast(pos));
}
#endif

#if UFE_PREVIEW_VERSION_NUM >= 2018

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

#endif // UFE_PREVIEW_VERSION_NUM

#if UFE_PREVIEW_VERSION_NUM < 2017
// Create a transform.
Ufe::SceneItem::Ptr UsdHierarchy::createGroup(const Ufe::PathComponent& name) const
{
	// According to Pixar, the following is more efficient when creating
	// multiple transforms, because of the use of ChangeBlock():
	// with Sdf.ChangeBlock():
	//     primSpec = Sdf.CreatePrimInLayer(layer, usdPath)
	//     primSpec.specifier = Sdf.SpecifierDef
	//     primSpec.typeName = 'Xform'

	// Rename the new group for uniqueness, if needed.
	Ufe::Path newPath = fItem->path() + name;
	auto childName = uniqueChildName(fItem, newPath);

	// Next, get the stage corresponding to the new path.
	auto segments = newPath.getSegments();
	TEST_USD_PATH(segments, newPath);
	auto dagSegment = segments[0];
	auto stage = getStage(Ufe::Path(dagSegment));

	// Build the corresponding USD path and create the USD group prim.
	auto usdPath = fItem->prim().GetPath().AppendChild(TfToken(childName));
	auto prim = UsdGeomXform::Define(stage, usdPath).GetPrim();

	// Create a UFE scene item from the prim.
	auto ufeChildPath = fItem->path() + childName;
	return UsdSceneItem::create(ufeChildPath, prim);
}

Ufe::Group UsdHierarchy::createGroupCmd(const Ufe::PathComponent& name) const
{
	auto createGroupCmd = UsdUndoCreateGroupCommand::create(fItem, name);
	createGroupCmd->execute();
	return Ufe::Group(createGroupCmd->group(), createGroupCmd);
}

#else // UFE_PREVIEW_VERSION_NUM


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

#endif // UFE_PREVIEW_VERSION_NUM

#if UFE_PREVIEW_VERSION_NUM >= 2018

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

#endif // UFE_PREVIEW_VERSION_NUM

#endif // UFE_V2_FEATURES_AVAILABLE

} // namespace ufe
} // namespace MayaUsd
