// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdTransform3dHandler.h"
#include "UsdSceneItem.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdTransform3dHandler::UsdTransform3dHandler()
	: Ufe::Transform3dHandler()
{
	fUsdTransform3d = UsdTransform3d::create();
}

UsdTransform3dHandler::~UsdTransform3dHandler()
{
}

/*static*/
UsdTransform3dHandler::Ptr UsdTransform3dHandler::create()
{
	return std::make_shared<UsdTransform3dHandler>();
}

//------------------------------------------------------------------------------
// Ufe::Transform3dHandler overrides
//------------------------------------------------------------------------------

Ufe::Transform3d::Ptr UsdTransform3dHandler::transform3d(const Ufe::SceneItem::Ptr& item) const
{
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
	assert(usdItem);
#endif
	fUsdTransform3d->setItem(usdItem);
	return fUsdTransform3d;
}

} // namespace ufe
} // namespace MayaUsd
