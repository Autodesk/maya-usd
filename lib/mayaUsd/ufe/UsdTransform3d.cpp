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

#include <mayaUsd/ufe/UsdRotatePivotTranslateUndoableCommand.h>
#include <mayaUsd/ufe/UsdRotateUndoableCommand.h>
#include <mayaUsd/ufe/UsdScaleUndoableCommand.h>
#include <mayaUsd/ufe/UsdTranslateUndoableCommand.h>
#include <mayaUsd/ufe/Utils.h>

#include "private/Utils.h"

#include <pxr/usd/usdGeom/xformCache.h>

#include <maya/MSelectionList.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>


MAYAUSD_NS_DEF {
namespace ufe {
namespace {
const TfToken kEmptyToken("");
}

#define USE_TRANSFORM_OPS 1

	MAYAUSD_CORE_PUBLIC
	ActiveTool GetActiveTool()
	{
		MString result;
		MGlobal::executeCommand("currentCtx", result, false, false);
		if(result == "RotateSuperContext")
			return ActiveTool::kRotate;
		if(result == "moveSuperContext")
			return ActiveTool::kTranslate;
		if(result == "scaleSuperContext")
			return ActiveTool::kScale;
		return ActiveTool::kSelect;
	}

	MayaUsdUtils::TransformOpProcessor::Space CurrentTranslateManipulatorSpace()
	{
		int result = 0;
		MGlobal::executeCommand("manipMoveContext -q -mode \"Move\"", result, false, false);
		switch(result)
		{
		case 0: 
			{
				// So for some reason, the xy/xz/yz planes, and x/y/z manips all return local space.
				// However, the central box works in world space. 
				int cah;
				MGlobal::executeCommand("manipMoveContext -q -cah \"Move\"", cah, false, false);
				return cah != 3 ? MayaUsdUtils::TransformOpProcessor::kPostTransform : MayaUsdUtils::TransformOpProcessor::kWorld;
			}
			break;

		case 1: return MayaUsdUtils::TransformOpProcessor::kPreTransform;
		case 2: return MayaUsdUtils::TransformOpProcessor::kWorld;
		default: break;
		}
		return MayaUsdUtils::TransformOpProcessor::kTransform;
	}

	MAYAUSD_CORE_PUBLIC
	MayaUsdUtils::TransformOpProcessor::Space CurrentManipulatorSpace()
	{
		int result = 0;
		switch(GetActiveTool())
		{
		case ActiveTool::kRotate:
			MGlobal::executeCommand("manipRotateContext -q -mode \"Rotate\"", result, false, false);
			break;
		case ActiveTool::kTranslate:
			MGlobal::executeCommand("manipMoveContext -q -mode \"Move\"", result, false, false);
			break;
		case ActiveTool::kScale:
			MGlobal::executeCommand("manipScaleContext -q -mode \"Scale\"", result, false, false);
			break;
		default:
			break;
		}

		switch(result)
		{
		case 0: return MayaUsdUtils::TransformOpProcessor::kPostTransform;
		case 1: return MayaUsdUtils::TransformOpProcessor::kPreTransform;
		case 2: return MayaUsdUtils::TransformOpProcessor::kWorld;
		// Given the lack of a coordinate frame in UFE, we can only handle local/world frames :(
		/* 
		0 - Object Space
		1 - Local Space
		2 - World Space (default)
		3 - Move Along Vertex Normal
		4 - Move Along Rotation Axis
		5 - Move Along Live Object Axis
		6 - Custom Axis Orientation
		9 - Component Space
		*/
		default: break;
		}
		return MayaUsdUtils::TransformOpProcessor::kTransform;
	}

namespace {

	Ufe::Matrix4d convertFromUsd(const GfMatrix4d& matrix)
	{
		return *(const Ufe::Matrix4d*)&matrix;
	}

	Ufe::Matrix4d primToUfeXform(const UsdPrim& prim, const UsdTimeCode& time)
	{
		#if USE_TRANSFORM_OPS
		try
		{		
			auto space = CurrentManipulatorSpace();
			switch(GetActiveTool())
			{
			case ActiveTool::kRotate:
				{
					if(MayaUsdUtils::TransformOpProcessor::kWorld == space || 
					   MayaUsdUtils::TransformOpProcessor::kTransform == space)
					{
						MayaUsdUtils::TransformOpProcessor proc(prim, kEmptyToken, MayaUsdUtils::TransformOpProcessor::kRotate, time);
						return convertFromUsd(proc.ManipulatorMatrix() * proc.ParentFrame());
					}
				}
				break;

			case ActiveTool::kTranslate:
				{
					if(MayaUsdUtils::TransformOpProcessor::kWorld == space || 
					   MayaUsdUtils::TransformOpProcessor::kTransform == space)
					{
						MayaUsdUtils::TransformOpProcessor proc(prim, kEmptyToken, MayaUsdUtils::TransformOpProcessor::kTranslate, time);
						return convertFromUsd(proc.ManipulatorMatrix() * proc.ParentFrame());
					}
					if(MayaUsdUtils::TransformOpProcessor::kPreTransform == space || 
					   MayaUsdUtils::TransformOpProcessor::kPostTransform == space)
					{
						// use defaults
					}
				}
				break;

			case ActiveTool::kScale:
				{
					if(MayaUsdUtils::TransformOpProcessor::kWorld == space || 
					   MayaUsdUtils::TransformOpProcessor::kTransform == space)
					{
						MayaUsdUtils::TransformOpProcessor proc(prim, kEmptyToken, MayaUsdUtils::TransformOpProcessor::kScale, time);
						return convertFromUsd(proc.ManipulatorMatrix() * proc.ParentFrame());
					}
				}
				break;

			case ActiveTool::kSelect:
				/* do nothing, just revert to handing back parent world + parent matrices */
				break;
			}
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		#endif

		UsdGeomXformCache xformCache(time);
		GfMatrix4d usdMatrix = xformCache.GetLocalToWorldTransform(prim);
		Ufe::Matrix4d xform = convertFromUsd(usdMatrix);
		return xform;
	}

	Ufe::Matrix4d primToUfeExclusiveXform(const UsdPrim& prim, const UsdTimeCode& time)
	{
		#if USE_TRANSFORM_OPS
		try
		{
			switch(GetActiveTool())
			{
			case ActiveTool::kRotate:
				{
					MayaUsdUtils::TransformOpProcessor proc(prim, kEmptyToken, MayaUsdUtils::TransformOpProcessor::kRotate, time);
					return convertFromUsd(proc.CoordinateFrame() * proc.ParentFrame());
				}
				break;

			case ActiveTool::kTranslate:
				{
					auto space = CurrentTranslateManipulatorSpace();
					MayaUsdUtils::TransformOpProcessor proc(prim, kEmptyToken, MayaUsdUtils::TransformOpProcessor::kTranslate, time);
					if(space != MayaUsdUtils::TransformOpProcessor::kWorld && 
					   space != MayaUsdUtils::TransformOpProcessor::kPreTransform)
					{
						return convertFromUsd(proc.CoordinateFrame() * proc.ParentFrame());
					}
					return convertFromUsd(proc.ParentFrame());
				}
				break;

			case ActiveTool::kScale:
				{
					MayaUsdUtils::TransformOpProcessor proc(prim, kEmptyToken, MayaUsdUtils::TransformOpProcessor::kScale, time);
					return convertFromUsd(proc.CoordinateFrame() * proc.ParentFrame());
				}
				break;

			case ActiveTool::kSelect:
				/* do nothing, just revert to handing back parent world + parent matrices */
				break;
			}
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		#endif
		UsdGeomXformCache xformCache(time);
		GfMatrix4d usdMatrix = xformCache.GetParentToWorldTransform(prim);
		Ufe::Matrix4d xform = convertFromUsd(usdMatrix);
		return xform;
	}
}

UsdTransform3d::UsdTransform3d() : Transform3d()
{
}

UsdTransform3d::UsdTransform3d(const UsdSceneItem::Ptr& item)
    : Transform3d(), fItem(item), fPrim(item->prim())
{}

/*static*/
UsdTransform3d::Ptr UsdTransform3d::create()
{
	return std::make_shared<UsdTransform3d>();
}

/* static */
UsdTransform3d::Ptr UsdTransform3d::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdTransform3d>(item);
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

#if UFE_PREVIEW_VERSION_NUM >= 2013
Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::translateCmd(double x, double y, double z)
{
	return UsdTranslateUndoableCommand::create(fItem, x, y, z, timeCode());
}
#endif

void UsdTransform3d::translate(double x, double y, double z)
{
	MayaUsdUtils::TransformOpProcessorEx proc(fPrim, TfToken(""), MayaUsdUtils::TransformOpProcessor::kTranslate, timeCode());
	switch(CurrentManipulatorSpace())
	{
	case MayaUsdUtils::TransformOpProcessor::kWorld:
		proc.SetTranslate(GfVec3d(x, y, z), CurrentManipulatorSpace());
		break;
	case MayaUsdUtils::TransformOpProcessor::kPreTransform:
		proc.SetTranslate(GfVec3d(x, y, z), CurrentManipulatorSpace());
		break;
	case MayaUsdUtils::TransformOpProcessor::kTransform:
		proc.SetTranslate(GfVec3d(x, y, z), CurrentManipulatorSpace());
		break;
	case MayaUsdUtils::TransformOpProcessor::kPostTransform:
		proc.SetTranslate(GfVec3d(x, y, z), CurrentManipulatorSpace());
		break;
	}
}

Ufe::Vector3d UsdTransform3d::translation() const
{
	switch(CurrentManipulatorSpace())
	{
	case MayaUsdUtils::TransformOpProcessor::kWorld:
	case MayaUsdUtils::TransformOpProcessor::kTransform:
		{
			MayaUsdUtils::TransformOpProcessorEx proc(fPrim, TfToken(""), MayaUsdUtils::TransformOpProcessor::kTranslate, timeCode());
			auto translate = proc.Translation();
			return Ufe::Vector3d(translate[0], translate[1], translate[2]);
		}
		break;
	default:
		break;
	}
	bool reset;
	std::vector<UsdGeomXformOp> ops = UsdGeomXformable(fPrim).GetOrderedXformOps(&reset);
	GfMatrix4d m = MayaUsdUtils::TransformOpProcessor::EvaluateCoordinateFrameForIndex(ops, ops.size(), timeCode());
	return Ufe::Vector3d(m[3][0], m[3][1], m[3][2]);
}

#if UFE_PREVIEW_VERSION_NUM >= 2013
Ufe::Vector3d UsdTransform3d::rotation() const
{
	MayaUsdUtils::TransformOpProcessorEx proc(fPrim, TfToken(""), MayaUsdUtils::TransformOpProcessor::kRotate, timeCode());
	auto quat = proc.Rotation();

	// UFE only supports XYZ rotation orders, so convert from quat to euler 
	auto r = MayaUsdUtils::QuatToEulerXYZ(quat);
	return Ufe::Vector3d(r[0], r[1], r[2]) * 180.0/M_PI;
}

Ufe::Vector3d UsdTransform3d::scale() const
{
	MayaUsdUtils::TransformOpProcessorEx proc(fPrim, TfToken(""), MayaUsdUtils::TransformOpProcessor::kScale, timeCode());
	auto scale = proc.Scale();
	return Ufe::Vector3d(scale[0], scale[1], scale[2]);
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3d::rotateCmd(double x, double y, double z)
{
	return UsdRotateUndoableCommand::create(fItem, x, y, z, timeCode());
}
#endif

void UsdTransform3d::rotate(double x, double y, double z)
{
	MayaUsdUtils::TransformOpProcessorEx proc(fPrim, TfToken(""), MayaUsdUtils::TransformOpProcessor::kRotate, timeCode());
	GfQuatd quat = MayaUsdUtils::QuatFromEulerXYZ(GfVec3d(x, y, z));
	proc.SetRotate(quat, CurrentManipulatorSpace());
}

#if UFE_PREVIEW_VERSION_NUM >= 2013
Ufe::ScaleUndoableCommand::Ptr UsdTransform3d::scaleCmd(double x, double y, double z)
{
	return UsdScaleUndoableCommand::create(fItem, x, y, z, timeCode());
}

#else

Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::translateCmd()
{
	return UsdTranslateUndoableCommand::create(fItem, 0, 0, 0, timeCode());
}

Ufe::RotateUndoableCommand::Ptr UsdTransform3d::rotateCmd()
{
	return UsdRotateUndoableCommand::create(fItem, 0, 0, 0, timeCode());
}

Ufe::ScaleUndoableCommand::Ptr UsdTransform3d::scaleCmd()
{
	return UsdScaleUndoableCommand::create(fItem, 1, 1, 1, timeCode());
}
#endif

void UsdTransform3d::scale(double x, double y, double z)
{
	MayaUsdUtils::TransformOpProcessorEx proc(fPrim, TfToken(""), MayaUsdUtils::TransformOpProcessor::kScale, timeCode());
	proc.SetScale(GfVec3d(x, y, z), CurrentManipulatorSpace());
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::rotatePivotTranslateCmd()
{
	auto translateCmd = UsdRotatePivotTranslateUndoableCommand::create(fPrim, fItem->path(), fItem, timeCode());
	return translateCmd;
}

void UsdTransform3d::rotatePivotTranslate(double x, double y, double z)
{
}

Ufe::Vector3d UsdTransform3d::rotatePivot() const
{
	return Ufe::Vector3d(0, 0, 0);
}

Ufe::TranslateUndoableCommand::Ptr UsdTransform3d::scalePivotTranslateCmd()
{
	return Ufe::TranslateUndoableCommand::Ptr();
}

void UsdTransform3d::scalePivotTranslate(double x, double y, double z)
{
}

Ufe::Vector3d UsdTransform3d::scalePivot() const
{
	return Ufe::Vector3d(0, 0, 0);
}

Ufe::Matrix4d UsdTransform3d::segmentInclusiveMatrix() const
{
	auto m = primToUfeXform(fPrim, timeCode());
	return m;
}
 
Ufe::Matrix4d UsdTransform3d::segmentExclusiveMatrix() const
{
	auto m = primToUfeExclusiveXform(fPrim, timeCode());
	return m;
}

} // namespace ufe
} // namespace MayaUsd
