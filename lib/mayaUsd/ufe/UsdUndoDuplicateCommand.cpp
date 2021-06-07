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
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/undo/UsdUndoBlock.h>
#endif
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

#ifdef UFE_V2_FEATURES_AVAILABLE
void UsdUndoDuplicateCommand::execute()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    UsdUndoBlock undoBlock(&_undoableItem);

    auto prim = ufePathToPrim(_ufeSrcPath);
    auto layer = prim.GetStage()->GetEditTarget().GetLayer();
    bool retVal = PXR_NS::SdfCopySpec(layer, prim.GetPath(), layer, _usdDstPath);
    TF_VERIFY(retVal, "Failed to copy spec data at '%s' to '%s'", prim.GetPath().GetText(), _usdDstPath.GetText());
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
#else
bool UsdUndoDuplicateCommand::duplicateUndo()
{
    // USD sends a ResyncedPaths notification after the prim is removed, but
    // at that point the prim is no longer valid, and thus a UFE post delete
    // notification is no longer possible.  To respect UFE object delete
    // notification semantics, which require the object to be alive when
    // the notification is sent, we send a pre delete notification here.
    auto                 ufeDstItem = duplicatedItem();
    Ufe::ObjectPreDelete notification(ufeDstItem);

#ifdef UFE_V2_FEATURES_AVAILABLE
    Ufe::Scene::instance().notify(notification);
#else
    Ufe::Scene::notifyObjectDelete(notification);
#endif
    auto prim = ufePathToPrim(_ufeSrcPath);
    prim.GetStage()->RemovePrim(_usdDstPath);

    return true;
}

bool UsdUndoDuplicateCommand::duplicateRedo()
{
    auto prim = ufePathToPrim(_ufeSrcPath);
    auto layer = prim.GetStage()->GetEditTarget().GetLayer();

    bool retVal = SdfCopySpec(layer, prim.GetPath(), layer, _usdDstPath);

    return retVal;
}

void UsdUndoDuplicateCommand::undo()
{
    try {
        MayaUsd::ufe::InAddOrDeleteOperation ad;
        if (!duplicateUndo()) {
            UFE_LOG("duplicate undo failed");
        }
    } catch (const std::exception& e) {
        UFE_LOG(e.what());
        throw; // re-throw the same exception
    }
}

void UsdUndoDuplicateCommand::redo()
{
    try {
        MayaUsd::ufe::InAddOrDeleteOperation ad;
        if (!duplicateRedo()) {
            UFE_LOG("duplicate redo failed");
        }
    } catch (const std::exception& e) {
        UFE_LOG(e.what());
        throw; // re-throw the same exception
    }
}
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
