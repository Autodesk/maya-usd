// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "UsdTransform3d.h"

#include "ufe/transform3dHandler.h"

//PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdTransform3d interface object.
class MAYAUSD_CORE_PUBLIC UsdTransform3dHandler : public Ufe::Transform3dHandler
{
public:
	typedef std::shared_ptr<UsdTransform3dHandler> Ptr;

	UsdTransform3dHandler();
	~UsdTransform3dHandler();

	// Delete the copy/move constructors assignment operators.
	UsdTransform3dHandler(const UsdTransform3dHandler&) = delete;
	UsdTransform3dHandler& operator=(const UsdTransform3dHandler&) = delete;
	UsdTransform3dHandler(UsdTransform3dHandler&&) = delete;
	UsdTransform3dHandler& operator=(UsdTransform3dHandler&&) = delete;

	//! Create a UsdTransform3dHandler.
	static UsdTransform3dHandler::Ptr create();

	// Ufe::Transform3dHandler overrides
	Ufe::Transform3d::Ptr transform3d(const Ufe::SceneItem::Ptr& item) const override;

private:
	UsdTransform3d::Ptr fUsdTransform3d;

}; // UsdTransform3dHandler

} // namespace ufe
} // namespace MayaUsd
