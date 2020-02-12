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

#include "UsdScalePivotTranslateUndoableCommand.h"
#include "private/Utils.h"
#include "AL/usd/utils/MayaTransformAPI.h"
#include "../base/debugCodes.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdScalePivotTranslateUndoableCommand::UsdScalePivotTranslateUndoableCommand(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item, UsdTimeCode timeCode)
	: Ufe::TranslateUndoableCommand(item)
	, fPrim(prim)
	, fPath(ufePath)
	, fTimeCode(timeCode)
	, fNoPivotOp(false)
{
	AL::usd::utils::MayaTransformAPI api(prim);
	fPrevPivotValue = api.scalePivot(fTimeCode);
}

UsdScalePivotTranslateUndoableCommand::~UsdScalePivotTranslateUndoableCommand()
{
}

/*static*/
UsdScalePivotTranslateUndoableCommand::Ptr UsdScalePivotTranslateUndoableCommand::create(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item, UsdTimeCode timeCode)
{
	return std::make_shared<UsdScalePivotTranslateUndoableCommand>(prim, ufePath, item, timeCode);
}

void UsdScalePivotTranslateUndoableCommand::undo()
{
	TF_DEBUG(MAYAUSD_UFE_MANIPULATORS).Msg("UsdScalePivotTranslateUndoableCommand::undo %s (%lf, %lf, %lf) @%lf\n", 
		fPath.string().c_str(), fPrevPivotValue[0], fPrevPivotValue[1], fPrevPivotValue[2], fTimeCode.GetValue());
	AL::usd::utils::MayaTransformAPI api(fPrim);
	api.scalePivot(GfVec3f(fPrevPivotValue), fTimeCode);

	// Todo : We would want to remove the xformOp
	// (SD-06/07/2018) Haven't found a clean way to do it - would need to investigate
}

void UsdScalePivotTranslateUndoableCommand::redo()
{
	// No-op, use move to translate the rotate pivot of the object.
	// The Maya move command directly invokes our translate() method in its
	// redoIt(), which is invoked both for the inital move and the redo.
}

//------------------------------------------------------------------------------
// Ufe::TranslateUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdScalePivotTranslateUndoableCommand::translate(double x, double y, double z)
{
	TF_DEBUG(MAYAUSD_UFE_MANIPULATORS).Msg("UsdRotatePivotTranslateUndoableCommand::translate %s (%lf, %lf, %lf) @%lf\n", 
		fPath.string().c_str(), x, y, z, fTimeCode.GetValue());
	AL::usd::utils::MayaTransformAPI api(fPrim);
	api.scalePivot(GfVec3f(x, y, z), fTimeCode);
	return true;
}

} // namespace ufe
} // namespace MayaUsd