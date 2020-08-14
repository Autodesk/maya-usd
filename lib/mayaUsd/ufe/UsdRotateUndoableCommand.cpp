//
// Copyright 2020 Autodesk
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
#include "UsdRotateUndoableCommand.h"

#include "Utils.h"
#include "private/Utils.h"
#include "../base/debugCodes.h"

#include <mayaUsdUtils/TransformOpTools.h>
#include <iostream>

MAYAUSD_NS_DEF {
namespace ufe {

static bool ExistingOpHasSamples(const UsdGeomXformOp& op)
{
	return op.GetNumTimeSamples() != 0;
}

// there doesn't seem to be a GF close for quats. 
// Simply checks the dot product for roughly being 1 (or -1 if comparing the negate with itself). 
bool GfIsClose(GfQuatd a, GfQuatd b, double eps)
{
	const double* pa = (const double*)&a;
	const double* pb = (const double*)&b;
	double dp = (pa[0] * pb[0]) + (pa[1] * pb[1]) + (pa[2] * pb[2]) + (pa[3] * pb[3]);
	return (std::abs(dp) > (1.0 - eps));
}

UsdRotateUndoableCommand::UsdRotateUndoableCommand(
    const UsdSceneItem::Ptr& item, double x, double y, double z, const UsdTimeCode& timeCode)
	: Ufe::RotateUndoableCommand(item),
	fPrim(ufePathToPrim(item->path())),
	fNewValue(MayaUsdUtils::QuatFromEulerXYZ(x, y, z)),
	fPath(item->path()),
	fTimeCode(timeCode)
{
    try 
    {
        MayaUsdUtils::TransformOpProcessor proc(fPrim, TfToken(""), MayaUsdUtils::TransformOpProcessor::kRotate, timeCode);
        fOp = proc.op();
		// only write time samples if op already has samples
		if(!ExistingOpHasSamples(fOp))
		{
			fTimeCode = UsdTimeCode::Default();
		}
        fPrevValue = proc.Rotation();
    }
    catch(const std::exception& e)
    {
		// use default time code if using a new op?
		fTimeCode = UsdTimeCode::Default();
		
        //
        // For rotation, I'm going to attempt a reasonably sensible guess. 
        // 
        // uniform token[] xformOpOrder = ["xformOp:translate", "xformOp:translate:rotatePivotTranslate", 
        //                                 "xformOp:translate:rotatePivot", "xformOp:rotateXYZ", 
        //                                                                     ^^ This one ^^
        //                                 "!invert!xformOp:translate:rotatePivot", "xformOp:translate:scalePivotTranslate", 
        //                                 "xformOp:translate:scalePivot", "xformOp:scale", "!invert!xformOp:translate:scalePivot"]
        // 
        UsdGeomXformable xform(fPrim);
        bool reset;
        std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&reset);
        fOp = xform.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat);
		if(ops.empty())
		{
			// do nothing, rotate will have just been added, so will be the only op in the stack
		}
		else
		{
			// step through the non-inverted translations in the stack
			auto it = ops.begin();
			while(it != ops.end())
			{
				auto isTranslateOp = (it->GetOpType() == UsdGeomXformOp::TypeTranslate);
				if(!isTranslateOp)
					break;

				auto isInverseOp = it->IsInverseOp();
				if(isInverseOp)
					break;
				++it;
			}

		    ops.insert(it, fOp);

			// update the xform op order
	        xform.SetXformOpOrder(ops, reset);
		}
    }
}

UsdRotateUndoableCommand::~UsdRotateUndoableCommand()
{}

/*static*/
UsdRotateUndoableCommand::Ptr UsdRotateUndoableCommand::create(
    const UsdSceneItem::Ptr& item, double x, double y, double z, const UsdTimeCode& timeCode)
{
	auto cmd = std::make_shared<MakeSharedEnabler<UsdRotateUndoableCommand>>(
        item, x, y, z, timeCode);
    return cmd;
}

void UsdRotateUndoableCommand::undo()
{
    if(GfIsClose(fNewValue, fPrevValue, 1e-5f))
    {
       return;
    }
    switch(fOp.GetOpType())
    {
	// invalid types here
    case UsdGeomXformOp::TypeScale:
    case UsdGeomXformOp::TypeTranslate:
		return;
	
    default:
        break;
    }

    MayaUsdUtils::TransformOpProcessorEx proc(fPrim, TfToken(""), MayaUsdUtils::TransformOpProcessor::kRotate, fTimeCode);
	proc.SetRotate(fPrevValue);
}

void UsdRotateUndoableCommand::redo()
{
}


//------------------------------------------------------------------------------
// Ufe::RotateUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdRotateUndoableCommand::rotate(double x, double y, double z)
{
	auto is_zero = [](const double q) {
		return std::fabs(q*q) < 1e-5f;
	};
	fNewValue = MayaUsdUtils::QuatFromEulerXYZ(x, y, z);
	
	try
	{
		MayaUsdUtils::TransformOpProcessorEx proc(fPrim, TfToken(""), MayaUsdUtils::TransformOpProcessor::kRotate, fTimeCode);
		auto currentRotation = proc.Rotation();

		// compute offset between new and current value
		GfQuatd diff = currentRotation.GetInverse() * fNewValue;

		// if the local space offset implies that we have a rotation only in x, y, or z, 
		// then apply that as a single angle offset to a single axis. In some cases this will 
		// result in a faster 
		if(is_zero(diff.GetImaginary()[1]) && is_zero(diff.GetImaginary()[2]))
		{
			// turn this back into an x rotation value
			double ang = 2.0 * std::atan2(diff.GetImaginary()[0], diff.GetReal());
			proc.RotateX(ang);
		}
		else
		if(is_zero(diff.GetImaginary()[0]) && is_zero(diff.GetImaginary()[2]))
		{	
			// turn this back into an y rotation value
			double ang = 2.0 * std::atan2(diff.GetImaginary()[1], diff.GetReal());
			proc.RotateY(ang);
		}
		else
		if(is_zero(diff.GetImaginary()[0]) && is_zero(diff.GetImaginary()[1]))
		{	
			// turn this back into an y rotation value
			double ang = 2.0 * std::atan2(diff.GetImaginary()[2], diff.GetReal());
			proc.RotateZ(ang);
		}
		else
		{
			proc.Rotate(diff, MayaUsdUtils::TransformOpProcessor::kTransform);
		}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return false;
	}
    return true;
}

} // namespace ufe
} // namespace MayaUsd
