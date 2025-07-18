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
#include "UsdSceneItemOps.h"

#include <mayaUsd/ufe/MayaUsdUndoRenameCommand.h>
#include <mayaUsd/ufe/UsdUndoDuplicateCommand.h>
#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/UsdUndoDeleteCommand.h>

#include <maya/MGlobal.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::SceneItemOps, UsdSceneItemOps);

UsdSceneItemOps::UsdSceneItemOps(const UsdUfe::UsdSceneItem::Ptr& item)
    : Ufe::SceneItemOps()
    , _item(item)
{
}

/*static*/
UsdSceneItemOps::Ptr UsdSceneItemOps::create(const UsdUfe::UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdSceneItemOps>(item);
}

void UsdSceneItemOps::setItem(const UsdUfe::UsdSceneItem::Ptr& item) { _item = item; }

const Ufe::Path& UsdSceneItemOps::path() const { return _item->path(); }

//------------------------------------------------------------------------------
// Ufe::SceneItemOps overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdSceneItemOps::sceneItem() const { return _item; }

#ifdef UFE_V4_FEATURES_AVAILABLE
Ufe::UndoableCommand::Ptr UsdSceneItemOps::deleteItemCmdNoExecute()
{
    return UsdUfe::UsdUndoDeleteCommand::create(prim());
}
#endif

Ufe::UndoableCommand::Ptr UsdSceneItemOps::deleteItemCmd()
{
    auto deleteCmd = UsdUfe::UsdUndoDeleteCommand::create(prim());
    deleteCmd->execute();
    return deleteCmd;
}

bool UsdSceneItemOps::deleteItem()
{
    if (prim()) {
        auto deleteCmd = UsdUfe::UsdUndoDeleteCommand::create(prim());
        deleteCmd->execute();
        return true;
    }

    return false;
}

#ifdef UFE_V4_FEATURES_AVAILABLE
Ufe::SceneItemResultUndoableCommand::Ptr UsdSceneItemOps::duplicateItemCmdNoExecute()
{
    return UsdUndoDuplicateCommand::create(_item);
}
#endif

Ufe::Duplicate UsdSceneItemOps::duplicateItemCmd()
{
    auto duplicateCmd = UsdUndoDuplicateCommand::create(_item);
    duplicateCmd->execute();
    return Ufe::Duplicate(duplicateCmd->duplicatedItem(), duplicateCmd);
}

Ufe::SceneItem::Ptr UsdSceneItemOps::duplicateItem()
{
    auto duplicate = duplicateItemCmd();
    return duplicate.item;
}

#ifdef UFE_V4_FEATURES_AVAILABLE
Ufe::SceneItemResultUndoableCommand::Ptr
UsdSceneItemOps::renameItemCmdNoExecute(const Ufe::PathComponent& newName)
{
    return MayaUsdUndoRenameCommand::create(_item, newName);
}
#endif

Ufe::Rename UsdSceneItemOps::renameItemCmd(const Ufe::PathComponent& newName)
{
    auto renameCmd = MayaUsdUndoRenameCommand::create(_item, newName);
    renameCmd->execute();
    return Ufe::Rename(renameCmd->renamedItem(), renameCmd);
}

Ufe::SceneItem::Ptr UsdSceneItemOps::renameItem(const Ufe::PathComponent& newName)
{
    auto renameCmd = MayaUsdUndoRenameCommand::create(_item, newName);
    renameCmd->execute();
    return renameCmd->renamedItem();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
