// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "UsdHierarchy.h"

//PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time hierarchy interface for children of the USD root prim.
/*!
    This class modifies its base class implementation to return the Maya USD
    gateway node as parent of USD prims that are children of the USD root prim.
 */
class MAYAUSD_CORE_PUBLIC UsdRootChildHierarchy : public UsdHierarchy
{
public:
	typedef std::shared_ptr<UsdRootChildHierarchy> Ptr;

	UsdRootChildHierarchy();
	~UsdRootChildHierarchy() override;

	// Delete the copy/move constructors assignment operators.
	UsdRootChildHierarchy(const UsdRootChildHierarchy&) = delete;
	UsdRootChildHierarchy& operator=(const UsdRootChildHierarchy&) = delete;
	UsdRootChildHierarchy(UsdRootChildHierarchy&&) = delete;
	UsdRootChildHierarchy& operator=(UsdRootChildHierarchy&&) = delete;

	//! Create a UsdRootChildHierarchy.
	static UsdRootChildHierarchy::Ptr create();

	// Ufe::Hierarchy overrides
	Ufe::SceneItem::Ptr parent() const override;

}; // UsdRootChildHierarchy

} // namespace ufe
} // namespace MayaUsd
