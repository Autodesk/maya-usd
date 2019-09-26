// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "UsdAttributes.h"

#include "ufe/AttributesHandler.h"

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create the USD Attributes interface objects.
class UsdAttributesHandler : public Ufe::AttributesHandler
{
public:
	typedef std::shared_ptr<UsdAttributesHandler> Ptr;

	UsdAttributesHandler();
	~UsdAttributesHandler();

	// Delete the copy/move constructors assignment operators.
	UsdAttributesHandler(const UsdAttributesHandler&) = delete;
	UsdAttributesHandler& operator=(const UsdAttributesHandler&) = delete;
	UsdAttributesHandler(UsdAttributesHandler&&) = delete;
	UsdAttributesHandler& operator=(UsdAttributesHandler&&) = delete;

	//! Create a UsdAttributesHandler.
	static UsdAttributesHandler::Ptr create();

	// Ufe::AttributesHandler overrides
	Ufe::Attributes::Ptr attributes(const Ufe::SceneItem::Ptr& item) const override;

private:

}; // UsdAttributesHandler

} // namespace ufe
} // namespace MayaUsd
