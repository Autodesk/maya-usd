// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"
#include "UsdSceneItem.h"

#include "ufe/object3d.h"

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time 3D object interface
/*!
    This class implements the Object3d interface for USD prims.
*/
class MAYAUSD_CORE_PUBLIC UsdObject3d : public Ufe::Object3d
{
public:
	using Ptr = std::shared_ptr<UsdObject3d>;

	UsdObject3d(const UsdSceneItem::Ptr& item);
	~UsdObject3d() override;

	// Delete the copy/move constructors assignment operators.
	UsdObject3d(const UsdObject3d&) = delete;
	UsdObject3d& operator=(const UsdObject3d&) = delete;
	UsdObject3d(UsdObject3d&&) = delete;
	UsdObject3d& operator=(UsdObject3d&&) = delete;

	//! Create a UsdObject3d.
	static UsdObject3d::Ptr create(const UsdSceneItem::Ptr& item);

	// Ufe::Object3d overrides
	Ufe::SceneItem::Ptr sceneItem() const override;
    Ufe::BBox3d boundingBox() const override;
#if UFE_PREVIEW_VERSION_NUM >= 2010
    bool visibility() const override;
    void setVisibility(bool vis) override;
#endif

private:
	UsdSceneItem::Ptr fItem;
	UsdPrim fPrim;

}; // UsdObject3d

} // namespace ufe
} // namespace MayaUsd
