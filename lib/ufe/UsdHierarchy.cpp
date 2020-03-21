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
#include "UsdUndoCreateGroupCommand.h"
#include "private/Utils.h"
#include "Utils.h"
#include "private/InPathChange.h"

#include <ufe/sceneNotification.h>
#include <ufe/scene.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/base/tf/stringUtils.h>

#include <cassert>
#include <stdexcept>

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

UsdHierarchy::UsdHierarchy()
	: Ufe::Hierarchy()
{
}

UsdHierarchy::~UsdHierarchy()
{
}

/*static*/
UsdHierarchy::Ptr UsdHierarchy::create()
{
	return std::make_shared<UsdHierarchy>();
}

void UsdHierarchy::setItem(UsdSceneItem::Ptr item)
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

Ufe::AppendedChild UsdHierarchy::appendChild(const Ufe::SceneItem::Ptr& child)
{
	auto usdChild = std::dynamic_pointer_cast<UsdSceneItem>(child);
#if !defined(NDEBUG)
	assert(usdChild);
#endif

	// First, check if we need to rename the child.
	std::string childName = uniqueChildName(sceneItem(), child->path());

	// Set up all paths to perform the reparent.
	auto prim = usdChild->prim();
	auto stage = prim.GetStage();
	auto ufeSrcPath = usdChild->path();
	auto usdSrcPath = prim.GetPath();
	auto ufeDstPath = fItem->path() + childName;
	auto usdDstPath = fPrim.GetPath().AppendChild(TfToken(childName));
	SdfLayerHandle layer = defPrimSpecLayer(prim);
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
	auto notification = Ufe::ObjectReparent(ufeDstItem, ufeSrcPath);
	Ufe::Scene::notifyObjectPathChange(notification);

	// FIXME  No idea how to get the child prim index yet.  PPT, 16-Aug-2018.
	return Ufe::AppendedChild(ufeDstItem, ufeSrcPath, 0);
}

#ifdef UFE_V2_FEATURES_AVAILABLE
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
	auto childName = uniqueChildName(sceneItem(), newPath);

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
#endif

} // namespace ufe
} // namespace MayaUsd
