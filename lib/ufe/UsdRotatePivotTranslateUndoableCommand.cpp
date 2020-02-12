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

#include "UsdRotatePivotTranslateUndoableCommand.h"
#include "private/Utils.h"
#include "AL/usd/utils/MayaTransformAPI.h"
#include "../base/debugCodes.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdRotatePivotTranslateUndoableCommand::UsdRotatePivotTranslateUndoableCommand(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item, const UsdTimeCode& timeCode)
	: Ufe::TranslateUndoableCommand(item)
	, fPrim(prim)
	, fPath(ufePath)
	, fTimeCode(timeCode)
	, fNoPivotOp(false)
{
	AL::usd::utils::MayaTransformAPI api(prim);
	fPrevPivotValue = api.rotatePivot(fTimeCode);
}

UsdRotatePivotTranslateUndoableCommand::~UsdRotatePivotTranslateUndoableCommand()
{
}

/*static*/
UsdRotatePivotTranslateUndoableCommand::Ptr UsdRotatePivotTranslateUndoableCommand::create(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item, const UsdTimeCode& timeCode)
{
	return std::make_shared<UsdRotatePivotTranslateUndoableCommand>(prim, ufePath, item, timeCode);
}

void UsdRotatePivotTranslateUndoableCommand::undo()
{
	TF_DEBUG(MAYAUSD_UFE_MANIPULATORS).Msg("UsdRotatePivotTranslateUndoableCommand::undo %s (%lf, %lf, %lf) @%lf\n",
	  fPath.string().c_str(), fPrevPivotValue[0], fPrevPivotValue[1], fPrevPivotValue[2], fTimeCode.GetValue());
	AL::usd::utils::MayaTransformAPI api(fPrim);
	api.rotatePivot(GfVec3f(fPrevPivotValue), fTimeCode);

	// Todo : We would want to remove the xformOp
	// (SD-06/07/2018) Haven't found a clean way to do it - would need to investigate
}

void UsdRotatePivotTranslateUndoableCommand::redo()
{
	// No-op, use move to translate the rotate pivot of the object.
	// The Maya move command directly invokes our translate() method in its
	// redoIt(), which is invoked both for the inital move and the redo.
}

//------------------------------------------------------------------------------
// Ufe::TranslateUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdRotatePivotTranslateUndoableCommand::translate(double x, double y, double z)
{
	TF_DEBUG(MAYAUSD_UFE_MANIPULATORS).Msg("UsdRotatePivotTranslateUndoableCommand::translate %s (%lf, %lf, %lf) @%lf\n", 
		fPath.string().c_str(), x, y, z, fTimeCode.GetValue());
	AL::usd::utils::MayaTransformAPI api(fPrim);
	api.rotatePivot(GfVec3f(x, y, z), fTimeCode);
	return true;
}

} // namespace ufe
} // namespace MayaUsd
