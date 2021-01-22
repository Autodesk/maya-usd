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
#include "UsdUndoAddNewPrimCommand.h"

#include "private/UfeNotifGuard.h"

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/UsdUndoBlock.h>

namespace {

Ufe::Path appendToPath(const Ufe::Path& path, const std::string& name)
{
    Ufe::Path newUfePath;
    if (1 == path.getSegments().size()) {
        newUfePath = path
            + Ufe::PathSegment(Ufe::PathComponent(name), MayaUsd::ufe::getUsdRunTimeId(), '/');
    } else {
        newUfePath = path + name;
    }
    return newUfePath;
}

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoAddNewPrimCommand::UsdUndoAddNewPrimCommand(
    const UsdSceneItem::Ptr& usdSceneItem,
    const std::string&       name,
    const std::string&       type)
    : Ufe::UndoableCommand()
{
    // First get the stage from the proxy shape.
    auto ufePath = usdSceneItem->path();
    auto segments = ufePath.getSegments();
    auto dagSegment = segments[0];
    _stage = usdSceneItem->prim().GetStage();
    if (_stage) {
        // Append the parent path and the requested name into a full ufe path.
        _newUfePath = appendToPath(ufePath, name + '1');

        // Ensure the requested name is unique.
        auto newPrimName = uniqueChildName(usdSceneItem->prim(), _newUfePath.back().string());

        // If the name had to change then we need to update the full ufe path.
        if (name != newPrimName) {
            _newUfePath = appendToPath(ufePath, newPrimName);
        }

        // Build (and store) the usd path for the new prim with the unique name.
        PXR_NS::SdfPath usdItemPath = usdSceneItem->prim().GetPath();
        _primPath = usdItemPath.AppendChild(PXR_NS::TfToken(newPrimName));

        // The type of prim we were asked to create.
        // Note: "Def" means create typeless prim.
        _primToken = (type.empty() || type == "Def") ? TfToken() : TfToken(type);
    }
}

void UsdUndoAddNewPrimCommand::execute()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    UsdUndoBlock undoBlock(&_undoableItem);

    if (_stage) {
        MayaUsd::ufe::InAddOrDeleteOperation ad;
        auto                                 prim = _stage->DefinePrim(_primPath, _primToken);
        if (!prim.IsValid())
            TF_RUNTIME_ERROR("Failed to create new prim type: %s", _primToken.GetText());
    }
}

void UsdUndoAddNewPrimCommand::undo()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.undo();
}

void UsdUndoAddNewPrimCommand::redo()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.redo();
}

const Ufe::Path& UsdUndoAddNewPrimCommand::newUfePath() const { return _newUfePath; }

PXR_NS::UsdPrim UsdUndoAddNewPrimCommand::newPrim() const
{
    if (!_stage) {
        return UsdPrim();
    }

    return _stage->GetPrimAtPath(_primPath);
}

UsdUndoAddNewPrimCommand::Ptr UsdUndoAddNewPrimCommand::create(
    const UsdSceneItem::Ptr& usdSceneItem,
    const std::string&       name,
    const std::string&       type)
{
    return std::make_shared<UsdUndoAddNewPrimCommand>(usdSceneItem, name, type);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
