// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdSceneItem.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdSceneItem::UsdSceneItem(const Ufe::Path& path, const UsdPrim& prim)
	: Ufe::SceneItem(path)
	, fPrim(prim)
{
}

UsdSceneItem::~UsdSceneItem()
{
}

/*static*/
UsdSceneItem::Ptr UsdSceneItem::create(const Ufe::Path& path, const UsdPrim& prim)
{
	return std::make_shared<UsdSceneItem>(path, prim);
}

const UsdPrim& UsdSceneItem::prim() const
{
	return fPrim;
}

//------------------------------------------------------------------------------
// Ufe::SceneItem overrides
//------------------------------------------------------------------------------

std::string UsdSceneItem::nodeType() const
{
	return fPrim.GetTypeName();
}

} // namespace ufe
} // namespace MayaUsd
