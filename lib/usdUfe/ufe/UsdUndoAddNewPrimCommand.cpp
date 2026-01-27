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

#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/UfeNotifGuard.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>

#include <ufe/hierarchy.h>
#include <ufe/pathString.h>

namespace USDUFE_NS_DEF {

// Ensure that UsdUndoAddNewPrimCommand is properly setup.
#ifdef UFE_V4_FEATURES_AVAILABLE
USDUFE_VERIFY_CLASS_SETUP(Ufe::SceneItemResultUndoableCommand, UsdUndoAddNewPrimCommand);
#else
USDUFE_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, UsdUndoAddNewPrimCommand);
#endif

UsdUndoAddNewPrimCommand::UsdUndoAddNewPrimCommand(
    const UsdSceneItem::Ptr& usdSceneItem,
    const std::string&       name,
    const std::string&       type)
#ifdef UFE_V4_FEATURES_AVAILABLE
    : Ufe::SceneItemResultUndoableCommand()
#else
    : Ufe::UndoableCommand()
#endif
{
    // First get the stage from the proxy shape.
    auto ufePath = usdSceneItem->path();
    auto segments = ufePath.getSegments();
    auto dagSegment = segments[0];
    _stage = usdSceneItem->prim().GetStage();
    if (_stage) {

        std::string base, suffixStr;

        // Append the parent path and the requested name into a full ufe path.
        // Append a '1' to new primitives names if the name does not end with a digit.
        _newUfePath = splitNumericalSuffix(name, base, suffixStr)
            ? appendToUsdPath(ufePath, name)
            : appendToUsdPath(ufePath, name + '1');

        // Ensure the requested name is unique.
        auto newPrimName
            = UsdUfe::uniqueChildName(usdSceneItem->prim(), _newUfePath.back().string(), nullptr);

        // If the name had to change then we need to update the full ufe path.
        if (name != newPrimName) {
            _newUfePath = appendToUsdPath(ufePath, newPrimName);
        }

        // Build (and store) the usd path for the new prim with the unique name.
        PXR_NS::SdfPath usdItemPath = usdSceneItem->prim().GetPath();
        _primPath = usdItemPath.AppendChild(PXR_NS::TfToken(newPrimName));

        // The type of prim we were asked to create.
        // Note: "Def" means create typeless prim.
        _primToken = (type.empty() || type == "Def") ? PXR_NS::TfToken() : PXR_NS::TfToken(type);
    }
}

void UsdUndoAddNewPrimCommand::execute()
{
    UsdUfe::InAddOrDeleteOperation ad;

    UsdUndoBlock undoBlock(&_undoableItem);

    if (_stage) {
        std::string errMsg;
        if (!UsdUfe::isEditTargetLayerModifiable(_stage, &errMsg)) {
            TF_RUNTIME_ERROR("%s", errMsg.c_str());
        } else {
            UsdPrim                        prim;
            UsdUfe::InAddOrDeleteOperation ad;
            if (_primToken.GetText() == PXR_NS::TfToken("Class")) {
                prim = _stage->CreateClassPrim(_primPath);
            } else {
                prim = _stage->DefinePrim(_primPath, _primToken);
            }
            if (!prim.IsValid()) {
                TF_RUNTIME_ERROR("Failed to create new prim type: %s", _primToken.GetText());
                _stage->RemovePrim(_primPath);
            }
        }
    }
}

void UsdUndoAddNewPrimCommand::undo()
{
    UsdUfe::InAddOrDeleteOperation ad;

    _undoableItem.undo();
    removeSessionLeftOvers(_stage, _primPath, &_undoableItem);
}

void UsdUndoAddNewPrimCommand::redo()
{
    UsdUfe::InAddOrDeleteOperation ad;

    _undoableItem.redo();
}

#ifdef UFE_V4_FEATURES_AVAILABLE
std::string UsdUndoAddNewPrimCommand::commandString() const
{
    return std::string("CreatePrim ") + _primToken.GetText() + " "
        + Ufe::PathString::string(_newUfePath);
}

Ufe::SceneItem::Ptr UsdUndoAddNewPrimCommand::sceneItem() const
{
    return Ufe::Hierarchy::createItem(newUfePath());
}
#endif

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

Ufe::Selection getNewSelectionFromCommand(const UsdUndoAddNewPrimCommand& cmd)
{
    Ufe::Selection newSelection;
    const auto     newItem = Ufe::Hierarchy::createItem(cmd.newUfePath());
    // The add operation may have failed (for example, if attempting to edit instance proxies).
    // Appending a null item throws an exception, which we dont want in this case.
    if (newItem) {
        newSelection.append(Ufe::Hierarchy::createItem(cmd.newUfePath()));
    }
    return newSelection;
}

} // namespace USDUFE_NS_DEF
