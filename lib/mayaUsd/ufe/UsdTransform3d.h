//
// Copyright 2019 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#pragma once

#include <ufe/path.h>
#include <ufe/transform3d.h>

#include <pxr/usd/usd/prim.h>

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdSceneItem.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to transform objects in 3D.
class MAYAUSD_CORE_PUBLIC UsdTransform3d : public Ufe::Transform3d
{
public:
	typedef std::shared_ptr<UsdTransform3d> Ptr;

	UsdTransform3d();
	UsdTransform3d(const UsdSceneItem::Ptr& item);
	~UsdTransform3d() override = default;

	// Delete the copy/move constructors assignment operators.
	UsdTransform3d(const UsdTransform3d&) = delete;
	UsdTransform3d& operator=(const UsdTransform3d&) = delete;
	UsdTransform3d(UsdTransform3d&&) = delete;
	UsdTransform3d& operator=(UsdTransform3d&&) = delete;

	//! Create a UsdTransform3d.
	static UsdTransform3d::Ptr create();
	static UsdTransform3d::Ptr create(const UsdSceneItem::Ptr& item);

	void setItem(const UsdSceneItem::Ptr& item);

	// Ufe::Transform3d overrides
	const Ufe::Path& path() const override;
	Ufe::SceneItem::Ptr sceneItem() const override;

    // When the current Maya preview release has UFE 0.2.13, this conditional
    // compilation can be converted to:
    // #ifdef UFE_V2_FEATURES_AVAILABLE
#if UFE_PREVIEW_VERSION_NUM >= 2013
	Ufe::TranslateUndoableCommand::Ptr translateCmd(double x, double y, double z) override;
	Ufe::RotateUndoableCommand::Ptr rotateCmd(double x, double y, double z) override;
	Ufe::ScaleUndoableCommand::Ptr scaleCmd(double x, double y, double z) override;
    Ufe::Vector3d rotation() const override;
    Ufe::Vector3d scale() const override;
#else
	Ufe::TranslateUndoableCommand::Ptr translateCmd() override;
	Ufe::RotateUndoableCommand::Ptr rotateCmd() override;
	Ufe::ScaleUndoableCommand::Ptr scaleCmd() override;
#endif

	void translate(double x, double y, double z) override;
	Ufe::Vector3d translation() const override;
	void rotate(double x, double y, double z) override;
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
