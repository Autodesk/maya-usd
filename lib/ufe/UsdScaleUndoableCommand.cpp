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
#include "Utils.h"
#include "AL/usd/utils/MayaTransformAPI.h"
#include "../base/debugCodes.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdScaleUndoableCommand::UsdScaleUndoableCommand(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item, const UsdTimeCode& timeCode)
	: Ufe::ScaleUndoableCommand(item)
	, fPrim(prim)
	, fPath(ufePath)
	, fTimeCode(timeCode)
	, fNoScaleOp(false)
{
	AL::usd::utils::MayaTransformAPI api(prim);
	fPrevScaleValue = api.scale(fTimeCode);
}

UsdScaleUndoableCommand::~UsdScaleUndoableCommand()
{
}

/*static*/
UsdScaleUndoableCommand::Ptr UsdScaleUndoableCommand::create(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item, const UsdTimeCode& timeCode)
{
	return std::make_shared<UsdScaleUndoableCommand>(prim, ufePath, item, timeCode);
}

void UsdScaleUndoableCommand::undo()
{
	TF_DEBUG(MAYAUSD_UFE_MANIPULATORS).Msg("UsdScaleUndoableCommand::undo %s (%lf, %lf, %lf) @%lf\n",
		fPath.string().c_str(), fPrevScaleValue[0], fPrevScaleValue[1], fPrevScaleValue[2], fTimeCode.GetValue());
	AL::usd::utils::MayaTransformAPI api(fPrim);
	api.scale(fPrevScaleValue, fTimeCode);
	// Todo : We would want to remove the xformOp
	// (SD-06/07/2018) Haven't found a clean way to do it - would need to investigate
}

void UsdScaleUndoableCommand::redo()
{
	perform();
}

void UsdScaleUndoableCommand::perform()
{
	// No-op, use scale to scale the object.
	// The Maya scale command directly invokes our scale() method in its
	// redoIt(), which is invoked both for the inital scale and the redo.
}

//------------------------------------------------------------------------------
// Ufe::ScaleUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdScaleUndoableCommand::scale(double x, double y, double z)
{
	TF_DEBUG(MAYAUSD_UFE_MANIPULATORS).Msg("UsdScaleUndoableCommand::scale %s (%lf, %lf, %lf) @%lf\n", 
		fPath.string().c_str(), x, y, z, fTimeCode.GetValue());
	AL::usd::utils::MayaTransformAPI api(fPrim);
	api.scale(GfVec3f(x, y, z), fTimeCode);
	return true;
}

} // namespace ufe
} // namespace MayaUsd
