//
// Copyright 2020 Autodesk
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

#include "UsdUndoInsertChildCommand.h"
#include "Utils.h"
#include "private/InPathChange.h"

#include <ufe/scene.h>
#include <ufe/sceneNotification.h>
#include <ufe/log.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#define UFE_ENABLE_ASSERTS
#include <ufe/ufeAssert.h>
#else
#include <cassert>
#endif

#include <mayaUsdUtils/util.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/base/tf/token.h>

namespace {
// shared_ptr requires public ctor, dtor, so derive a class for it.
template<class T>
struct MakeSharedEnabler : public T {
    MakeSharedEnabler(
        const MayaUsd::ufe::UsdSceneItem::Ptr& parent,
        const MayaUsd::ufe::UsdSceneItem::Ptr& child,
        const MayaUsd::ufe::UsdSceneItem::Ptr& pos
    ) : T(parent, child, pos) {}
};
}

MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoInsertChildCommand::UsdUndoInsertChildCommand(
    const UsdSceneItem::Ptr& parent,
    const UsdSceneItem::Ptr& child,
    const UsdSceneItem::Ptr& /* pos */
) : Ufe::UndoableCommand()
{
    // First, check if we need to rename the child.
    auto childName = uniqueChildName(parent, child->path());

    // Set up all paths to perform the reparent.
    auto childPrim  = child->prim();
    fStage      = childPrim.GetStage();
    fUfeSrcItem = child;
    fUsdSrcPath = childPrim.GetPath();
    // Create a new segment if parent and child are in different run-times.
    auto cRtId = child->path().runTimeId();
    if (parent->path().runTimeId() == cRtId) {
        fUfeDstPath = parent->path() + childName;
    }
    else {
        auto cSep = child->path().getSegments().back().separator();
        fUfeDstPath = parent->path() + Ufe::PathSegment(
            Ufe::PathComponent(childName), cRtId, cSep);
    }
    fUsdDstPath = parent->prim().GetPath().AppendChild(TfToken(childName));

    fChildLayer = MayaUsdUtils::defPrimSpecLayer(childPrim);
    if (!fChildLayer) {
        std::string err = TfStringPrintf("No child prim found at %s", childPrim.GetPath().GetString().c_str());
        throw std::runtime_error(err.c_str());
    }

    auto parentPrim = parent->prim();
    // If parent prim is the pseudo-root, no def primSpec will be found, so
    // just use the edit target layer.
    fParentLayer = parentPrim.IsPseudoRoot() ?
        fStage->GetEditTarget().GetLayer() :
        MayaUsdUtils::defPrimSpecLayer(parentPrim);

    if (!fParentLayer) {
        std::string err = TfStringPrintf("No parent prim found at %s", parentPrim.GetPath().GetString().c_str());
        throw std::runtime_error(err.c_str());
    }
}

UsdUndoInsertChildCommand::~UsdUndoInsertChildCommand()
{
}

/*static*/
UsdUndoInsertChildCommand::Ptr UsdUndoInsertChildCommand::create(
    const UsdSceneItem::Ptr& parent,
    const UsdSceneItem::Ptr& child,
    const UsdSceneItem::Ptr& pos
)
{
    // Error if requested parent is currently a child of requested child.
    if (parent->path().startsWith(child->path())) {
        return nullptr;
    }
    return std::make_shared<MakeSharedEnabler<UsdUndoInsertChildCommand>>(
        parent, child, pos);
}

bool UsdUndoInsertChildCommand::insertChildRedo()
{
    // See comments in UsdUndoRenameCommand.cpp.
    bool status = SdfCopySpec(fChildLayer, fUsdSrcPath, fParentLayer, fUsdDstPath);
    if (status)
    {
        auto srcPrim = fStage->GetPrimAtPath(fUsdSrcPath);
#ifdef UFE_V2_FEATURES_AVAILABLE
        UFE_ASSERT_MSG(srcPrim, "Invalid prim cannot be inactivated.");
#else
        assert(srcPrim);
#endif
        status = srcPrim.SetActive(false);

        if (status) {
            fUfeDstItem = UsdSceneItem::create(fUfeDstPath, ufePathToPrim(fUfeDstPath));
            Ufe::ObjectReparent notification(fUfeDstItem, fUfeSrcItem->path());
            Ufe::Scene::notifyObjectPathChange(notification);
        }
    }
    else {
        UFE_LOG(std::string("Warning: SdfCopySpec(") +
                fUsdSrcPath.GetString() + std::string(") failed."));
    }

    return status;
}

bool UsdUndoInsertChildCommand::insertChildUndo()
{
    bool status{false};
    {
        // Regardless of where the edit target is currently set, switch to the
        // layer where we copied the source prim into the destination, then
        // restore the edit target.
        UsdEditContext ctx(fStage, fParentLayer);
        status = fStage->RemovePrim(fUsdDstPath);
    }
    if (status) {
        auto srcPrim = fStage->GetPrimAtPath(fUsdSrcPath);
#ifdef UFE_V2_FEATURES_AVAILABLE
        UFE_ASSERT_MSG(srcPrim, "Invalid prim cannot be activated.");
#else
        assert(srcPrim);
#endif
        status = srcPrim.SetActive(true);

        if (status) {
            Ufe::ObjectReparent notification(fUfeSrcItem, fUfeDstItem->path());
            Ufe::Scene::notifyObjectPathChange(notification);
            fUfeDstItem = nullptr;
        }
    }
    else {
        UFE_LOG(std::string("Warning: RemovePrim(") +
                fUsdDstPath.GetString() + std::string(") failed."));
    }

    return status;
}

void UsdUndoInsertChildCommand::undo()
{
    try {
        InPathChange pc;
        if (!insertChildUndo()) {
            UFE_LOG("insert child undo failed");
        }
    }
    catch (const std::exception& e) {
        UFE_LOG(e.what());
        throw;  // re-throw the same exception
    }
}

void UsdUndoInsertChildCommand::redo()
{
    InPathChange pc;
    if (!insertChildRedo()) {
        UFE_LOG("insert child redo failed");
    }
}

} // namespace ufe
} // namespace MayaUsd
