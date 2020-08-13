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
#include "private/UfeNotifGuard.h"
#include "private/Utils.h"
#include "Utils.h"

#include <ufe/log.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/stage.h>

#include <mayaUsdUtils/util.h>

#ifdef UFE_V2_FEATURES_AVAILABLE
#define UFE_ENABLE_ASSERTS
#include <ufe/ufeAssert.h>
#else
#include <cassert>
#endif

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

UsdUndoInsertChildCommand::UsdUndoInsertChildCommand(const UsdSceneItem::Ptr& parent,
                                                     const UsdSceneItem::Ptr& child,
                                                     const UsdSceneItem::Ptr& /* pos */) 
    : Ufe::UndoableCommand()
    , _ufeSrcItem(child)
    , _ufeDstItem(nullptr)
    , _ufeSrcPath(child->path())
    , _usdSrcPath(child->prim().GetPath())
{
    const auto& childPrim = child->prim();
    const auto& parentPrim = parent->prim();

    // Apply restriction rules
    ufe::applyCommandRestriction(childPrim, "reparent");
    ufe::applyCommandRestriction(parentPrim, "reparent");

    // First, check if we need to rename the child.
    const auto& childName = uniqueChildName(parent, child->path());

    // Create a new segment if parent and child are in different run-times.
    // parenting a USD node to the proxy shape node implies two different run-times
    auto cRtId = child->path().runTimeId();
    if (parent->path().runTimeId() == cRtId) {
        _ufeDstPath = parent->path() + childName;
    }
    else {
        auto cSep = child->path().getSegments().back().separator();
        _ufeDstPath = parent->path() + Ufe::PathSegment(
            Ufe::PathComponent(childName), cRtId, cSep);
    }
    _usdDstPath = parent->prim().GetPath().AppendChild(TfToken(childName));

    _childLayer = child->prim().GetStage()->GetEditTarget().GetLayer();

    // If parent prim is the pseudo-root, no def primSpec will be found, so
    // just use the edit target layer.
    _parentLayer = parentPrim.IsPseudoRoot() 
        ? parent->prim().GetStage()->GetEditTarget().GetLayer() 
        : MayaUsdUtils::defPrimSpecLayer(parentPrim);
}

UsdUndoInsertChildCommand::~UsdUndoInsertChildCommand()
{
}

/*static*/
UsdUndoInsertChildCommand::Ptr 
UsdUndoInsertChildCommand::create(const UsdSceneItem::Ptr& parent,
                                  const UsdSceneItem::Ptr& child,
                                  const UsdSceneItem::Ptr& pos)
{
    if (!parent || !child) {
        return nullptr;
    }

    // Error if requested parent is currently a child of requested child.
    if (parent->path().startsWith(child->path())) {
        return nullptr;
    }
    return std::make_shared<MakeSharedEnabler<UsdUndoInsertChildCommand>>(
        parent, child, pos);
}

bool UsdUndoInsertChildCommand::insertChildRedo()
{
    bool status = SdfCopySpec(_childLayer, _usdSrcPath, _parentLayer, _usdDstPath);
    if (status)
    {
        // remove all scene description for the given path and 
        // its subtree in the current UsdEditTarget 
        {
            auto stage = _ufeSrcItem->prim().GetStage();
            UsdEditContext ctx(stage, _childLayer);
            status = stage->RemovePrim(_usdSrcPath);
        }

        if (status) {
            _ufeDstItem = UsdSceneItem::create(_ufeDstPath, ufePathToPrim(_ufeDstPath));
            sendNotification<Ufe::ObjectReparent>(_ufeDstItem, _ufeSrcPath);
        }
    }
    else {
        UFE_LOG(std::string("Warning: SdfCopySpec(") +
                _usdSrcPath.GetString() + std::string(") failed."));
    }

    return status;
}

bool UsdUndoInsertChildCommand::insertChildUndo()
{
    bool status = SdfCopySpec(_parentLayer, _usdDstPath, _childLayer, _usdSrcPath);
    if (status)
    {
        // remove all scene description for the given path and 
        // its subtree in the current UsdEditTarget
        {
            auto stage = _ufeDstItem->prim().GetStage();
            UsdEditContext ctx(stage, _parentLayer);
            status = stage->RemovePrim(_usdDstPath);
        }

        if (status) {
            _ufeSrcItem = UsdSceneItem::create(_ufeSrcPath, ufePathToPrim(_ufeSrcPath));
            sendNotification<Ufe::ObjectReparent>(_ufeSrcItem, _ufeDstPath);
        }
    }
    else {
        UFE_LOG(std::string("Warning: RemovePrim(") +
                _usdDstPath.GetString() + std::string(") failed."));
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
