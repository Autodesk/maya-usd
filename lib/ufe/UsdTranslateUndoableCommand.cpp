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

#include "UsdTranslateUndoableCommand.h"
#include "private/Utils.h"
#include "Utils.h"
#include "mayaUsdUtils/MayaTransformAPI.h"
#include "../base/debugCodes.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdTranslateUndoableCommand::UsdTranslateUndoableCommand(
	const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item, const UsdTimeCode& timeCode)
	: Ufe::TranslateUndoableCommand(item)
	, fPrim(prim)
	, fPath(ufePath)
	, fTimeCode(timeCode)
	, fNoTranslateOp(false)
{
	MayaUsdUtils::MayaTransformAPI api(prim);
	fPrevTranslateValue = api.translate(fTimeCode);
}

UsdTranslateUndoableCommand::~UsdTranslateUndoableCommand()
{
}

/*static*/
UsdTranslateUndoableCommand::Ptr UsdTranslateUndoableCommand::create(
	const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item, const UsdTimeCode& timeCode)
{
	return std::make_shared<UsdTranslateUndoableCommand>(prim, ufePath, item, timeCode);
}

void UsdTranslateUndoableCommand::undo()
{
	TF_DEBUG(MAYAUSD_UFE_MANIPULATORS).Msg("UsdTranslateUndoableCommand::undo %s (%lf, %lf, %lf) @%lf\n", 
		fPath.string().c_str(), fPrevTranslateValue[0], fPrevTranslateValue[1], fPrevTranslateValue[2], fTimeCode.GetValue());
	
	MayaUsdUtils::MayaTransformAPI api(fPrim);
	api.translate(fPrevTranslateValue, fTimeCode);

	// Todo : We would want to remove the xformOp
	// (SD-06/07/2018) Haven't found a clean way to do it - would need to investigate
}

void UsdTranslateUndoableCommand::redo()
{
	perform();
}

void UsdTranslateUndoableCommand::perform()
{
	// No-op, use translate to move the object.
	// The Maya move command directly invokes our translate() method in its
	// redoIt(), which is invoked both for the inital move and the redo.
}

//------------------------------------------------------------------------------
// Ufe::TranslateUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdTranslateUndoableCommand::translate(double x, double y, double z)
{
	TF_DEBUG(MAYAUSD_UFE_MANIPULATORS).Msg("UsdTranslateUndoableCommand::translate %s (%lf, %lf, %lf) @%lf\n",
		fPath.string().c_str(), x, y, z, fTimeCode.GetValue());
	MayaUsdUtils::MayaTransformAPI api(fPrim);
	api.translate(GfVec3d(x, y, z), fTimeCode);
	return true;
}

} // namespace ufe
} // namespace MayaUsd
