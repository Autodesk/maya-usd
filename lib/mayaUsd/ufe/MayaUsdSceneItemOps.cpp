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
#include "MayaUsdSceneItemOps.h"

#include <mayaUsd/ufe/MayaUsdUndoRenameCommand.h>
#include <mayaUsd/ufe/UsdUndoDuplicateCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdSceneItemOps, MayaUsdSceneItemOps);

MayaUsdSceneItemOps::MayaUsdSceneItemOps(const UsdUfe::UsdSceneItem::Ptr& item)
    : UsdUfe::UsdSceneItemOps(item)
{
}

/*static*/
MayaUsdSceneItemOps::Ptr MayaUsdSceneItemOps::create(const UsdUfe::UsdSceneItem::Ptr& item)
{
    return std::make_shared<MayaUsdSceneItemOps>(item);
}

//------------------------------------------------------------------------------
// Ufe::SceneItemOps overrides
//------------------------------------------------------------------------------

#ifdef UFE_V4_FEATURES_AVAILABLE
Ufe::SceneItemResultUndoableCommand::Ptr MayaUsdSceneItemOps::duplicateItemCmdNoExecute()
{
    return UsdUndoDuplicateCommand::create(usdSceneItem());
}
#endif

Ufe::Duplicate MayaUsdSceneItemOps::duplicateItemCmd()
{
    auto duplicateCmd = UsdUndoDuplicateCommand::create(usdSceneItem());
    duplicateCmd->execute();
    return Ufe::Duplicate(duplicateCmd->duplicatedItem(), duplicateCmd);
}

#ifdef UFE_V4_FEATURES_AVAILABLE
Ufe::SceneItemResultUndoableCommand::Ptr
MayaUsdSceneItemOps::renameItemCmdNoExecute(const Ufe::PathComponent& newName)
{
    return MayaUsdUndoRenameCommand::create(usdSceneItem(), newName);
}
#endif

Ufe::Rename MayaUsdSceneItemOps::renameItemCmd(const Ufe::PathComponent& newName)
{
    auto renameCmd = MayaUsdUndoRenameCommand::create(usdSceneItem(), newName);
    renameCmd->execute();
    return Ufe::Rename(renameCmd->renamedItem(), renameCmd);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
