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
#include "private/Utils.h"
#include "Utils.h"

#include <pxr/usd/usdGeom/xformCache.h>

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

	Ufe::Matrix4d primToUfeXform(const UsdPrim& prim, const UsdTimeCode& time)
	{
		UsdGeomXformCache xformCache(time);
		GfMatrix4d usdMatrix = xformCache.GetLocalToWorldTransform(prim);
		Ufe::Matrix4d xform = convertFromUsd(usdMatrix);
		return xform;
	}

	Ufe::Matrix4d primToUfeExclusiveXform(const UsdPrim& prim, const UsdTimeCode& time)
	{
		UsdGeomXformCache xformCache(time);
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
	auto translateCmd = UsdTranslateUndoableCommand::create(fPrim, fItem->path(), fItem);
	return translateCmd;
}

void UsdTransform3d::translate(double x, double y, double z)
{
	translateOp(fPrim, fItem->path(), x, y, z);
}

Ufe::Vector3d UsdTransform3d::translation() const
{
	double x{0}, y{0}, z{0};
	const TfToken xlate("xformOp:translate");
	if (fPrim.HasAttribute(xlate))
	{
		// Initially, attribute can be created, but have no value.
		GfVec3d v;
		if (fPrim.GetAttribute(xlate).Get<GfVec3d>(&v,getTime(path())))
		{
			x = v[0]; y = v[1]; z = v[2];
		}
	}
	return Ufe::Vector3d(x, y, z);
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3d::rotateCmd()
{
	auto rotateCmd = UsdRotateUndoableCommand::create(fPrim, fItem->path(), fItem);
	return rotateCmd;
}

void UsdTransform3d::rotate(double x, double y, double z)
{
	rotateOp(fPrim, fItem->path(), x, y, z);
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3d::scaleCmd()
{
	auto scaleCmd = UsdScaleUndoableCommand::create(fPrim, fItem->path(), fItem);
	return scaleCmd;
}

void UsdTransform3d::scale(double x, double y, double z)
{
	scaleOp(fPrim, fItem->path(), x, y, z);
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::rotatePivotTranslateCmd()
{
	auto translateCmd = UsdRotatePivotTranslateUndoableCommand::create(fPrim, fItem->path(), fItem);
	return translateCmd;
}

void UsdTransform3d::rotatePivotTranslate(double x, double y, double z)
{
	rotatePivotTranslateOp(fPrim, fItem->path(), x, y, z);
}

Ufe::Vector3d UsdTransform3d::rotatePivot() const
{
	double x{0}, y{0}, z{0};
	const TfToken xpivot("xformOp:translate:pivot");
	if (fPrim.HasAttribute(xpivot))
	{
		// Initially, attribute can be created, but have no value.
		GfVec3f v;
		if (fPrim.GetAttribute(xpivot).Get<GfVec3f>(&v,getTime(path())))
		{
			x = v[0]; y = v[1]; z = v[2];
		}
	}
	return Ufe::Vector3d(x, y, z);
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::scalePivotTranslateCmd()
{
	throw std::runtime_error("UsdTransform3d::scalePivotTranslateCmd() not implemented");
}

void UsdTransform3d::scalePivotTranslate(double x, double y, double z)
{
	return rotatePivotTranslate(x, y, z);
}

Ufe::Vector3d UsdTransform3d::scalePivot() const
{
	return rotatePivot();
}

Ufe::Matrix4d UsdTransform3d::segmentInclusiveMatrix() const
{
	return primToUfeXform(fPrim,getTime(path()));
}
 
Ufe::Matrix4d UsdTransform3d::segmentExclusiveMatrix() const
{
	return primToUfeExclusiveXform(fPrim,getTime(path()));
}

} // namespace ufe
} // namespace MayaUsd
