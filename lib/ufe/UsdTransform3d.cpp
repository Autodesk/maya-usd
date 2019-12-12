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

#include "UsdTransform3d.h"
#include "UsdTranslateUndoableCommand.h"
#include "UsdRotateUndoableCommand.h"
#include "UsdScaleUndoableCommand.h"
#include "UsdRotatePivotTranslateUndoableCommand.h"
#include "UsdScalePivotTranslateUndoableCommand.h"
#include "AL/usd/utils/MayaTransformAPI.h"
#include "private/Utils.h"

#include <pxr/usd/usdGeom/xformCache.h>

#include <maya/MSelectionList.h>
#include <maya/MFnDagNode.h>

MAYAUSD_NS_DEF {
namespace ufe {

namespace {
	Ufe::Matrix4d convertFromUsd(const GfMatrix4d& matrix)
	{
		// Even though memory layout of Ufe::Matrix4d and double[4][4] are identical
		// we need to return a copy of the matrix so we cannot cast.
		double m[4][4];
		matrix.Get(m);
		Ufe::Matrix4d uMat;
		uMat.matrix = {{	{{m[0][0], m[0][1], m[0][2], m[0][3]}},
							{{m[1][0], m[1][1], m[1][2], m[1][3]}},
							{{m[2][0], m[2][1], m[2][2], m[2][3]}},
							{{m[3][0], m[3][1], m[3][2], m[3][3]}} }};
		return uMat;
	}

	Ufe::Matrix4d primToUfeXform(const UsdPrim& prim)
	{
		UsdGeomXformCache xformCache;
		GfMatrix4d usdMatrix = xformCache.GetLocalToWorldTransform(prim);
		Ufe::Matrix4d xform = convertFromUsd(usdMatrix);
		return xform;
	}

	Ufe::Matrix4d primToUfeExclusiveXform(const UsdPrim& prim)
	{
		UsdGeomXformCache xformCache;
		GfMatrix4d usdMatrix = xformCache.GetParentToWorldTransform(prim);
		Ufe::Matrix4d xform = convertFromUsd(usdMatrix);
		return xform;
	}
}

UsdTransform3d::UsdTransform3d()
	: Transform3d()
{
}

/*static*/
UsdTransform3d::Ptr UsdTransform3d::create()
{
	return std::make_shared<UsdTransform3d>();
}

void UsdTransform3d::setItem(const UsdSceneItem::Ptr& item)
{
	fPrim = item->prim();
	fItem = item;
}

//------------------------------------------------------------------------------
// Ufe::Transform3d overrides
//------------------------------------------------------------------------------

MayaUsdProxyShapeBase* UsdTransform3d::proxy() const
{
	auto ufePath = path();
	const auto& pathStr = ufePath.string();
	auto pos = pathStr.find_first_of('/');

	MString dagPathStr(pathStr.c_str() + 6, pos - 6);
	MSelectionList sl;
	sl.add(dagPathStr);
	MDagPath dagPath;
	sl.getDagPath(0, dagPath);
	MFnDagNode fn(dagPath);
  return (MayaUsdProxyShapeBase*)fn.userNode();
}

UsdTimeCode UsdTransform3d::timeCode() const
{
	MayaUsdProxyShapeBase* proxyShape = proxy();
	if(proxyShape)
	{
		auto value = proxyShape->getTime();
		return value;
	}
	return UsdTimeCode::Default();
}

const Ufe::Path& UsdTransform3d::path() const
{
	return fItem->path();
}

Ufe::SceneItem::Ptr UsdTransform3d::sceneItem() const
{
	return fItem;
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::translateCmd()
{
	auto translateCmd = UsdTranslateUndoableCommand::create(fPrim, fItem->path(), fItem, timeCode());
	return translateCmd;
}

void UsdTransform3d::translate(double x, double y, double z)
{
	translateOp(fPrim, fItem->path(), x, y, z);
}

Ufe::Vector3d UsdTransform3d::translation() const
{
	AL::usd::utils::MayaTransformAPI api(fPrim);
	auto translate = api.translate(timeCode());
	return Ufe::Vector3d(translate[0], translate[1], translate[2]);
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3d::rotateCmd()
{
	auto rotateCmd = UsdRotateUndoableCommand::create(fPrim, fItem->path(), fItem, timeCode());
	return rotateCmd;
}

void UsdTransform3d::rotate(double x, double y, double z)
{
	AL::usd::utils::MayaTransformAPI api(fPrim);
	auto order = api.rotateOrder();
	api.rotate(GfVec3f(x, y, z), order, timeCode());
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3d::scaleCmd()
{
	auto scaleCmd = UsdScaleUndoableCommand::create(fPrim, fItem->path(), fItem, timeCode());
	return scaleCmd;
}

void UsdTransform3d::scale(double x, double y, double z)
{
	scaleOp(fPrim, fItem->path(), x, y, z);
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::rotatePivotTranslateCmd()
{
	auto translateCmd = UsdRotatePivotTranslateUndoableCommand::create(fPrim, fItem->path(), fItem, timeCode());
	return translateCmd;
}

void UsdTransform3d::rotatePivotTranslate(double x, double y, double z)
{
	AL::usd::utils::MayaTransformAPI api(fPrim);
	api.rotatePivot(GfVec3f(x, y, z), timeCode());
}

Ufe::Vector3d UsdTransform3d::rotatePivot() const
{
	AL::usd::utils::MayaTransformAPI api(fPrim);
	auto value = api.rotatePivot(timeCode());
	return Ufe::Vector3d(value[0], value[1], value[2]);
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::scalePivotTranslateCmd()
{
	auto scalePivotCmd = UsdScalePivotTranslateUndoableCommand::create(fPrim, fItem->path(), fItem, timeCode());
	return scalePivotCmd;
}

void UsdTransform3d::scalePivotTranslate(double x, double y, double z)
{
	AL::usd::utils::MayaTransformAPI api(fPrim);
	api.scalePivot(GfVec3f(x, y, z), timeCode());
}

Ufe::Vector3d UsdTransform3d::scalePivot() const
{
	AL::usd::utils::MayaTransformAPI api(fPrim);
	auto value = api.scalePivot(timeCode());
	return Ufe::Vector3d(value[0], value[1], value[2]);
}

Ufe::Matrix4d UsdTransform3d::segmentInclusiveMatrix() const
{
	return primToUfeXform(fPrim);
}
 
Ufe::Matrix4d UsdTransform3d::segmentExclusiveMatrix() const
{
	return primToUfeExclusiveXform(fPrim);
}

} // namespace ufe
} // namespace MayaUsd
