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

#include "ufe/path.h"
#include "ufe/transform3d.h"

#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to transform objects in 3D.
class MAYAUSD_CORE_PUBLIC UsdTransform3d : public Ufe::Transform3d
{
public:
	typedef std::shared_ptr<UsdTransform3d> Ptr;

	UsdTransform3d();
	~UsdTransform3d() override;

	// Delete the copy/move constructors assignment operators.
	UsdTransform3d(const UsdTransform3d&) = delete;
	UsdTransform3d& operator=(const UsdTransform3d&) = delete;
	UsdTransform3d(UsdTransform3d&&) = delete;
	UsdTransform3d& operator=(UsdTransform3d&&) = delete;

	//! Create a UsdTransform3d.
	static UsdTransform3d::Ptr create();

	void setItem(const UsdSceneItem::Ptr& item);

	// Ufe::Transform3d overrides
	const Ufe::Path& path() const override;
	Ufe::SceneItem::Ptr sceneItem() const override;
	Ufe::TranslateUndoableCommand::Ptr translateCmd() override;
	void translate(double x, double y, double z) override;
	Ufe::Vector3d translation() const override;
	Ufe::RotateUndoableCommand::Ptr rotateCmd() override;
	void rotate(double x, double y, double z) override;
	Ufe::ScaleUndoableCommand::Ptr scaleCmd() override;
	void scale(double x, double y, double z) override;
	Ufe::TranslateUndoableCommand::Ptr rotatePivotTranslateCmd() override;
	void rotatePivotTranslate(double x, double y, double z) override;
	Ufe::Vector3d rotatePivot() const override;
	Ufe::TranslateUndoableCommand::Ptr scalePivotTranslateCmd() override;
	void scalePivotTranslate(double x, double y, double z) override;
	Ufe::Vector3d scalePivot() const override;
	Ufe::Matrix4d segmentInclusiveMatrix() const override;
	Ufe::Matrix4d segmentExclusiveMatrix() const override;

private:
	UsdSceneItem::Ptr fItem;
	UsdPrim fPrim;

}; // UsdTransform3d

} // namespace ufe
} // namespace MayaUsd
