// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdSceneItemOpsHandler.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdSceneItemOpsHandler::UsdSceneItemOpsHandler()
	: Ufe::SceneItemOpsHandler()
{
	fUsdSceneItemOps = UsdSceneItemOps::create();
}

UsdSceneItemOpsHandler::~UsdSceneItemOpsHandler()
{
}

/*static*/
UsdSceneItemOpsHandler::Ptr UsdSceneItemOpsHandler::create()
{
	return std::make_shared<UsdSceneItemOpsHandler>();
}

//------------------------------------------------------------------------------
// Ufe::SceneItemOpsHandler overrides
//------------------------------------------------------------------------------

Ufe::SceneItemOps::Ptr UsdSceneItemOpsHandler::sceneItemOps(const Ufe::SceneItem::Ptr& item) const
{
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
	assert(usdItem);
#endif
	fUsdSceneItemOps->setItem(usdItem);
	return fUsdSceneItemOps;
}

} // namespace ufe
} // namespace MayaUsd
