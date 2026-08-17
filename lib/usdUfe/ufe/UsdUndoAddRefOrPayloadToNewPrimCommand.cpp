//
// Copyright 2026 Autodesk
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
#include "UsdUndoAddRefOrPayloadToNewPrimCommand.h"

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/UsdUndoAddNewPrimCommand.h>
#include <usdUfe/ufe/UsdUndoAddPayloadCommand.h>
#include <usdUfe/ufe/UsdUndoAddReferenceCommand.h>
#include <usdUfe/ufe/UsdUndoPayloadCommand.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/usd/usd/stage.h>

namespace USDUFE_NS_DEF {

PXR_NAMESPACE_USING_DIRECTIVE

USDUFE_VERIFY_CLASS_SETUP(Ufe::UndoableCommand, UsdUndoAddRefOrPayloadToNewPrimCommand);

UsdUndoAddRefOrPayloadToNewPrimCommand::UsdUndoAddRefOrPayloadToNewPrimCommand(
    const UsdPrim&     parentPrim,
    const std::string& newPrimName,
    const std::string& filePath,
    const std::string& primPath,
    bool               prepend,
    bool               isPayload,
    bool               preload)
    : _parentPrim(parentPrim)
    , _newPrimName(newPrimName)
    , _filePath(filePath)
    , _primPath(primPath)
    , _prepend(prepend)
    , _isPayload(isPayload)
    , _preload(preload)
    , _compositeCmd(std::make_shared<Ufe::CompositeUndoableCommand>())
{
}

void UsdUndoAddRefOrPayloadToNewPrimCommand::execute()
{
    if (!_parentPrim.IsValid())
        return;

    // Build a scene item for the parent prim so we can reuse the existing create-prim command.
    const Ufe::Path parentPath = _parentPrim.IsPseudoRoot()
        ? stagePath(_parentPrim.GetStage())
        : stagePath(_parentPrim.GetStage()) + usdPathToUfePathSegment(_parentPrim.GetPath());
    auto parentItem = UsdSceneItem::create(parentPath, _parentPrim);

    // Create a typeless "def" (its type composes through the arc added next).
    auto addPrimCmd = UsdUndoAddNewPrimCommand::create(parentItem, _newPrimName, "Def");
    if (!addPrimCmd)
        return;
    _compositeCmd->append(addPrimCmd);
    addPrimCmd->execute();

    const UsdPrim newPrim = addPrimCmd->newPrim();
    if (!newPrim.IsValid())
        return;

    // Add the reference or payload arc to the newly-created prim, reusing the existing commands.
    if (_isPayload) {
        Ufe::UndoableCommand::Ptr preloadCmd;
        if (_preload) {
            preloadCmd
                = std::make_shared<UsdUndoLoadPayloadCommand>(newPrim, UsdLoadWithDescendants);
        } else {
            preloadCmd = std::make_shared<UsdUndoUnloadPayloadCommand>(newPrim);
        }
        _compositeCmd->append(preloadCmd);
        preloadCmd->execute();

        auto payloadCmd
            = std::make_shared<UsdUndoAddPayloadCommand>(newPrim, _filePath, _primPath, _prepend);
        _compositeCmd->append(payloadCmd);
        payloadCmd->execute();
    } else {
        auto refCmd
            = std::make_shared<UsdUndoAddReferenceCommand>(newPrim, _filePath, _primPath, _prepend);
        _compositeCmd->append(refCmd);
        refCmd->execute();
    }
}

void UsdUndoAddRefOrPayloadToNewPrimCommand::undo() { _compositeCmd->undo(); }

void UsdUndoAddRefOrPayloadToNewPrimCommand::redo() { _compositeCmd->redo(); }

} // namespace USDUFE_NS_DEF
