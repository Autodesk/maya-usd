// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdObject3dHandler.h"
#include "UsdSceneItem.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdObject3dHandler::UsdObject3dHandler() : Ufe::Object3dHandler() {}

UsdObject3dHandler::~UsdObject3dHandler() {}

/*static*/
UsdObject3dHandler::Ptr UsdObject3dHandler::create()
{
	return std::make_shared<UsdObject3dHandler>();
}

//------------------------------------------------------------------------------
// UsdObject3dHandler overrides
//------------------------------------------------------------------------------

Ufe::Object3d::Ptr UsdObject3dHandler::object3d(const Ufe::SceneItem::Ptr& item) const
{
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
	assert(usdItem);
#endif
	return UsdObject3d::create(usdItem);
}

} // namespace ufe
} // namespace MayaUsd
