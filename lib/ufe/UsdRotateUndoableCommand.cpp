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

#include "UsdRotateUndoableCommand.h"
#include "private/Utils.h"
#include "AL/usd/utils/MayaTransformAPI.h"
#include "../base/debugCodes.h"

MAYAUSD_NS_DEF {
namespace ufe {

UsdRotateUndoableCommand::UsdRotateUndoableCommand(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item, const UsdTimeCode& timeCode)
	: Ufe::RotateUndoableCommand(item)
	, fPrim(prim)
	, fPath(ufePath)
	, fTimeCode(timeCode)
	, fNoRotateOp(false)
{
	AL::usd::utils::MayaTransformAPI api(fPrim);
	fPrevRotateValue = api.rotate(fTimeCode);
}

UsdRotateUndoableCommand::~UsdRotateUndoableCommand()
{
}

/*static*/
UsdRotateUndoableCommand::Ptr UsdRotateUndoableCommand::create(const UsdPrim& prim, const Ufe::Path& ufePath, const Ufe::SceneItem::Ptr& item, const UsdTimeCode& timeCode)
{
	return std::make_shared<UsdRotateUndoableCommand>(prim, ufePath, item, timeCode);
}

void UsdRotateUndoableCommand::undo()
{
	AL::usd::utils::MayaTransformAPI api(fPrim);
	const auto order = api.rotateOrder();
	TF_DEBUG(MAYAUSD_UFE_MANIPULATORS).Msg("UsdRotateUndoableCommand::undo %s (%lf, %lf, %lf) [%d] @%lf\n", 
		fPath.string().c_str(), fPrevRotateValue[0], fPrevRotateValue[1], fPrevRotateValue[2], int(order), fTimeCode.GetValue());

	api.rotate(M_PI * fPrevRotateValue / 180.0f, order, fTimeCode);

	// Todo : We would want to remove the xformOp
	// (SD-06/07/2018) Haven't found a clean way to do it - would need to investigate
}

void UsdRotateUndoableCommand::redo()
{
	perform();
}

void UsdRotateUndoableCommand::perform()
{
	// No-op, use rotate to move the object
	// The Maya rotate command directly invokes our rotate() method in its
	// redoIt(), which is invoked both for the inital rotate and the redo.
}

//------------------------------------------------------------------------------
// Ufe::RotateUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdRotateUndoableCommand::rotate(double x, double y, double z)
{
	AL::usd::utils::MayaTransformAPI api(fPrim);
	const auto order = api.rotateOrder();
	TF_DEBUG(MAYAUSD_UFE_MANIPULATORS).Msg("UsdRotateUndoableCommand::rotate %s (%lf, %lf, %lf) [%d] @%lf\n", 
		fPath.string().c_str(), x, y, z, int(order), fTimeCode.GetValue());
	api.rotate(M_PI * GfVec3f(x, y, z) / 180.0f, order, fTimeCode);
	return true;
}

} // namespace ufe
} // namespace MayaUsd
