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
#include "UsdUndoDuplicateCommand.h"

#include "private/UfeNotifGuard.h"
#include "private/Utils.h"

#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/UsdUndoBlock.h>
#include <mayaUsdUtils/util.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/log.h>
#include <ufe/path.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoDuplicateCommand::UsdUndoDuplicateCommand(const UsdSceneItem::Ptr& srcItem)
    : Ufe::UndoableCommand()
    , _ufeSrcPath(srcItem->path())
{
    auto srcPrim = srcItem->prim();
    auto parentPrim = srcPrim.GetParent();

    ufe::applyCommandRestriction(srcPrim, "duplicate");

    auto newName = uniqueChildName(parentPrim, srcPrim.GetName());
    _usdDstPath = parentPrim.GetPath().AppendChild(TfToken(newName));
}

UsdUndoDuplicateCommand::~UsdUndoDuplicateCommand() { }

UsdUndoDuplicateCommand::Ptr UsdUndoDuplicateCommand::create(const UsdSceneItem::Ptr& srcItem)
{
    return std::make_shared<UsdUndoDuplicateCommand>(srcItem);
}

UsdSceneItem::Ptr UsdUndoDuplicateCommand::duplicatedItem() const
{
    return createSiblingSceneItem(_ufeSrcPath, _usdDstPath.GetElementString());
}

void UsdUndoDuplicateCommand::execute()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    UsdUndoBlock undoBlock(&_undoableItem);

    auto prim = ufePathToPrim(_ufeSrcPath);
    auto layer = prim.GetStage()->GetEditTarget().GetLayer();
    SdfCopySpec(layer, prim.GetPath(), layer, _usdDstPath);
}

void UsdUndoDuplicateCommand::undo()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.undo();
}

void UsdUndoDuplicateCommand::redo()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.redo();
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
