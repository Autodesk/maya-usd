// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "ufe/sceneItem.h"

#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time scene item interface
class MAYAUSD_CORE_PUBLIC UsdSceneItem : public Ufe::SceneItem
{
public:
	typedef std::shared_ptr<UsdSceneItem> Ptr;

	UsdSceneItem(const Ufe::Path& path, const UsdPrim& prim);
	~UsdSceneItem() override;

	// Delete the copy/move constructors assignment operators.
	UsdSceneItem(const UsdSceneItem&) = delete;
	UsdSceneItem& operator=(const UsdSceneItem&) = delete;
	UsdSceneItem(UsdSceneItem&&) = delete;
	UsdSceneItem& operator=(UsdSceneItem&&) = delete;

	//! Create a UsdSceneItem from a UFE path and a USD prim.
	static UsdSceneItem::Ptr create(const Ufe::Path& path, const UsdPrim& prim);

	const UsdPrim& prim() const;

	// Ufe::SceneItem overrides
	std::string nodeType() const override;

private:
	UsdPrim fPrim;
}; // UsdSceneItem

} // namespace ufe
} // namespace MayaUsd
