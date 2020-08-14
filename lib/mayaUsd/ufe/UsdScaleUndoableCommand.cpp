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
#include "UsdScaleUndoableCommand.h"

#include "private/Utils.h"
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/base/debugCodes.h>

#include <mayaUsdUtils/TransformOpTools.h>

MAYAUSD_NS_DEF {
namespace ufe {

static bool ExistingOpHasSamples(const UsdGeomXformOp& op)
{
	return op.GetNumTimeSamples() != 0;
}

UsdScaleUndoableCommand::UsdScaleUndoableCommand(
    const UsdSceneItem::Ptr& item, double x, double y, double z, const UsdTimeCode& timeCode
) : Ufe::ScaleUndoableCommand(item),
	fPrim(ufePathToPrim(item->path())),
	fPrevValue(0,0,0),
	fNewValue(x, y, z),
	fPath(item->path()),
	fTimeCode(timeCode)
{
    try 
    {
        MayaUsdUtils::TransformOpProcessor proc(fPrim, TfToken(""), MayaUsdUtils::TransformOpProcessor::kScale, timeCode);
        fOp = proc.op();
		// only write time samples if op already has samples
		if(!ExistingOpHasSamples(fOp))
		{
			fTimeCode = UsdTimeCode::Default();
		}
        fPrevValue = proc.Scale();
    }
    catch(const std::exception& e)
    {
		// use default time code if using a new op?
		fTimeCode = UsdTimeCode::Default();

        //
        // So I'm going to make the assumption here that you *probably* want to manipulate the very 
        // last scale in the xform op stack?
        // 
        // uniform token[] xformOpOrder = ["xformOp:translate", "xformOp:translate:rotatePivotTranslate", 
        //                                 "xformOp:translate:rotatePivot", "xformOp:rotateXYZ", 
        //                                 "!invert!xformOp:translate:rotatePivot", "xformOp:translate:scalePivotTranslate", 
        //                                 "xformOp:translate:scalePivot", "xformOp:scale", "!invert!xformOp:translate:scalePivot"]
        //                                                                 ^^ This one ^^
        // 
        UsdGeomXformable xform(fPrim);
        bool reset;
        std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&reset);
        fOp = xform.AddScaleOp(UsdGeomXformOp::PrecisionFloat);
		if(ops.empty())
		{
			// do nothing, scale will have just been added, so will be the only op in the stack
		}
		else
		{
			auto& back = ops.back();
			auto isInverseOp = back.IsInverseOp();
			auto isTranslateOp = (back.GetOpType() == UsdGeomXformOp::TypeTranslate);
			if(isInverseOp && isTranslateOp)
			{
				// if we have a scale pivot at the end of the stack, insert before last item
				if(back.HasSuffix(TfToken("scalePivot")) || back.HasSuffix(TfToken("pivot")))
				{
		        	ops.insert(ops.end() - 1, fOp);
				}
				else
				{
					// default - add to end of transform stack
					ops.push_back(fOp);
				}
			}
		    else
			{
				// default - add to end of transform stack
				ops.push_back(fOp);
			}
			// update the xform op order
	        xform.SetXformOpOrder(ops, reset);
		}
    }
}

UsdScaleUndoableCommand::~UsdScaleUndoableCommand()
{}

/*static*/
UsdScaleUndoableCommand::Ptr UsdScaleUndoableCommand::create(
    const UsdSceneItem::Ptr& item, double x, double y, double z, const UsdTimeCode& timeCode
)
{
	auto cmd = std::make_shared<MakeSharedEnabler<UsdScaleUndoableCommand>>(
        item, x, y, z, timeCode);
    return cmd;

}

void UsdScaleUndoableCommand::undo()
{
    // do nothing
    if(GfIsClose(fNewValue, fPrevValue, 1e-5f))
    {
        return;
    }
    switch(fOp.GetOpType())
    {
    case UsdGeomXformOp::TypeScale:
        {
            switch(fOp.GetPrecision())
            {
            case UsdGeomXformOp::PrecisionHalf:
                {
                    fOp.Set(GfVec3h(fPrevValue[0], fPrevValue[1], fPrevValue[2]), fTimeCode);
                }
                break;
            case UsdGeomXformOp::PrecisionFloat:
                {
                    fOp.Set(GfVec3f(fPrevValue[0], fPrevValue[1], fPrevValue[2]), fTimeCode);
                }
                break;
            case UsdGeomXformOp::PrecisionDouble:
                {
                    fOp.Set(fPrevValue, fTimeCode);
                }
                break;
            }
        }
        break;

    case UsdGeomXformOp::TypeTransform:
        {
            GfMatrix4d M;
            fOp.Get(&M, fTimeCode);
			GfVec3d relativeScalar(fNewValue[0] / fPrevValue[0], fNewValue[1] / fPrevValue[1], fNewValue[2] / fPrevValue[2]);
            M[0][0] *= relativeScalar[0]; M[0][1] *= relativeScalar[0]; M[0][2] *= relativeScalar[0];
            M[1][0] *= relativeScalar[1]; M[1][1] *= relativeScalar[1]; M[1][2] *= relativeScalar[1];
            M[2][0] *= relativeScalar[2]; M[2][1] *= relativeScalar[2]; M[2][2] *= relativeScalar[2];
            fOp.Set(M, fTimeCode);
        }
        break;

    default:
        break;
    }
}

void UsdScaleUndoableCommand::redo()
{
}

//------------------------------------------------------------------------------
// Ufe::ScaleUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdScaleUndoableCommand::scale(double x, double y, double z)
{
    fNewValue = GfVec3d(x, y, z);
    try
    {
        MayaUsdUtils::TransformOpProcessor proc(fPrim, TfToken(""), MayaUsdUtils::TransformOpProcessor::kScale, fTimeCode);
		auto s = proc.Scale();
		
		// do nothing 
		if(GfIsClose(fNewValue, s, 1e-5f))
		{
			return true;
		}

        GfVec3d diff(fNewValue[0] / s[0], fNewValue[1] / s[1], fNewValue[2] / s[2]);
        proc.Scale(diff);
    }
    catch(std::exception e)
    {
        return false;
    }
    return true;
}

} // namespace ufe
} // namespace MayaUsd
