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

#include <mayaUsd/ufe/UsdUndoDeleteCommand.h>
#include <mayaUsd/ufe/UsdUndoDuplicateCommand.h>
#include <mayaUsd/ufe/UsdUndoRenameCommand.h>
#include <mayaUsd/ufe/Utils.h>

#include <maya/MGlobal.h>

namespace {
// Warning: If a user tries to delete a prim that is deactivated (can be localized).
static constexpr char kWarningCannotDeactivePrim[]
    = "Cannot deactivate \"^1s\" because it is already inactive.";

void displayWarning(const UsdPrim& prim, const MString& fmt)
{
    MString msg, primArg(prim.GetName().GetText());
    msg.format(fmt, primArg);
    MGlobal::displayWarning(msg);
}

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdSceneItemOps::UsdSceneItemOps(const UsdSceneItem::Ptr& item)
    : Ufe::SceneItemOps()
    , fItem(item)
{
}

UsdSceneItemOps::~UsdSceneItemOps() { }

/*static*/
UsdSceneItemOps::Ptr UsdSceneItemOps::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdSceneItemOps>(item);
}

void UsdSceneItemOps::setItem(const UsdSceneItem::Ptr& item) { fItem = item; }

const Ufe::Path& UsdSceneItemOps::path() const { return fItem->path(); }

//------------------------------------------------------------------------------
// Ufe::SceneItemOps overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdSceneItemOps::sceneItem() const { return fItem; }

Ufe::UndoableCommand::Ptr UsdSceneItemOps::deleteItemCmd()
{
    if (prim().IsActive()) {
        auto deleteCmd = UsdUndoDeleteCommand::create(prim());
        deleteCmd->execute();
        return deleteCmd;
    }

    displayWarning(prim(), kWarningCannotDeactivePrim);
    return nullptr;
}

bool UsdSceneItemOps::deleteItem()
{
    if (prim().IsActive()) {
        return prim().SetActive(false);
    }

    displayWarning(prim(), kWarningCannotDeactivePrim);
    return false;
}

Ufe::Duplicate UsdSceneItemOps::duplicateItemCmd()
{
    auto duplicateCmd = UsdUndoDuplicateCommand::create(prim(), fItem->path());
    duplicateCmd->execute();
    auto item = createSiblingSceneItem(path(), duplicateCmd->usdDstPath().GetElementString());
    return Ufe::Duplicate(item, duplicateCmd);
}

Ufe::SceneItem::Ptr UsdSceneItemOps::duplicateItem()
{
    SdfPath        usdDstPath;
    SdfLayerHandle layer;
    UsdUndoDuplicateCommand::primInfo(prim(), usdDstPath, layer);
    bool status = UsdUndoDuplicateCommand::duplicate(layer, prim().GetPath(), usdDstPath);

    // The duplicate is a sibling of the source.
    if (status)
        return createSiblingSceneItem(path(), usdDstPath.GetElementString());

    return nullptr;
}

Ufe::SceneItem::Ptr UsdSceneItemOps::renameItem(const Ufe::PathComponent& newName)
{
    auto renameCmd = UsdUndoRenameCommand::create(fItem, newName);
    renameCmd->execute();
    return renameCmd->renamedItem();
}

Ufe::Rename UsdSceneItemOps::renameItemCmd(const Ufe::PathComponent& newName)
{
    auto renameCmd = UsdUndoRenameCommand::create(fItem, newName);
    renameCmd->execute();
    return Ufe::Rename(renameCmd->renamedItem(), renameCmd);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
