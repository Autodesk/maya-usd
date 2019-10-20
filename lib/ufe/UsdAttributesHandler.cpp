// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "UsdAttributesHandler.h"
#include "UsdSceneItem.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdAttributesHandler::UsdAttributesHandler()
	: Ufe::AttributesHandler()
{
}

UsdAttributesHandler::~UsdAttributesHandler()
{
}

/*static*/
UsdAttributesHandler::Ptr UsdAttributesHandler::create()
{
	return std::make_shared<UsdAttributesHandler>();
}

//------------------------------------------------------------------------------
// Ufe::AttributesHandler overrides
//------------------------------------------------------------------------------

Ufe::Attributes::Ptr UsdAttributesHandler::attributes(const Ufe::SceneItem::Ptr& item) const
{
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
	assert(usdItem);
#endif
	auto usdAttributes = UsdAttributes::create(usdItem);
	return usdAttributes;
}

} // namespace ufe
} // namespace MayaUsd
