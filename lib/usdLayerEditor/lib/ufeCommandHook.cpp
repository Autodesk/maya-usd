//
// Copyright 2024 Autodesk
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

#include "ufeCommandHook.h"

#include "abstractCommandHook.h"
#include "LayerEditorCommands.h"
#include "sessionState.h"

#include <pxr/base/tf/diagnosticHelper.h>

#include <ufe/undoableCommand.h>
#include <ufe/undoableCommandMgr.h>

#include <cassert>
#include <string>

namespace UsdLayerEditor {

void UfeCommandHook::executeDelayedCommands()
{
    // TODO LE-EXTRACT Some commands require to be delays, for example adding a parent layer.
}

void UfeCommandHook::setEditTarget(UsdLayer usdLayer)
{
    auto cmd = ::std::make_shared<SetEditTargetCmd>(_sessionState->stage(), usdLayer);
    AppendOrExecuteCommand(cmd);
}

void UfeCommandHook::openUndoBracket(const QString& name)
{
    compositeCommand = std::make_shared<LayedEditorCommand>(name.toStdString());
}

void UfeCommandHook::closeUndoBracket()
{
    if (!compositeCommand) {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_CODING_ERROR("Not ongoing undoable bracket to be closed.");
        return;
    }

    if (!compositeCommand->cmdsList().empty()) {
        Ufe::UndoableCommandMgr::instance().executeCmd(compositeCommand);
        notify(CommandExecuted {});
    }

    compositeCommand.reset();
}

void UfeCommandHook::insertSubLayerPath(UsdLayer usdLayer, Path path, int index)
{
    auto cmd = ::std::make_shared<InsertSubPathCmd>(_sessionState->stage() , usdLayer, path, index);
    AppendOrExecuteCommand(cmd);
}

void UfeCommandHook::removeSubLayerPath(UsdLayer usdLayer, Path path)
{
    auto   cmd = ::std::make_shared<RemoveSubPathCmd>(_sessionState->stage(), usdLayer, path);
    AppendOrExecuteCommand(cmd);
}

void UfeCommandHook::moveSubLayerPath(
    Path     path,
    UsdLayer oldParentUsdLayer,
    UsdLayer newParentUsdLayer,
    int      index)
{
    auto removeCmd = ::std::make_shared<RemoveSubPathCmd>(
        _sessionState->stage(), oldParentUsdLayer, path);
    AppendOrExecuteCommand(removeCmd);

    auto insertCmd = ::std::make_shared<InsertSubPathCmd>(
        _sessionState->stage(), newParentUsdLayer, path, index);
    AppendOrExecuteCommand(insertCmd);
}

void UfeCommandHook::replaceSubLayerPath(UsdLayer usdLayer, Path oldPath, Path newPath)
{
    auto cmd = ::std::make_shared<ReplaceSubPathCmd>(usdLayer, oldPath, newPath);
    AppendOrExecuteCommand(cmd);
}

void UfeCommandHook::discardEdits(UsdLayer usdLayer)
{
    auto cmd = ::std::make_shared<DiscardEditCmd>(usdLayer);
    AppendOrExecuteCommand(cmd);

    refreshLayerSystemLock(usdLayer);
}

void UfeCommandHook::clearLayer(UsdLayer usdLayer)
{
    auto cmd = ::std::make_shared<ClearLayerCmd>(usdLayer);
    AppendOrExecuteCommand(cmd);
}

void UfeCommandHook::flattenLayer(UsdLayer usdLayer)
{
    auto cmd = ::std::make_shared<FlattenLayerCmd>(usdLayer);
    AppendOrExecuteCommand(cmd);
}

UsdLayer UfeCommandHook::addAnonymousSubLayer(UsdLayer usdLayer, std::string newName)
{
    auto cmd = ::std::make_shared<AddAnonSubLayerCmd>(_sessionState->stage(), usdLayer);
    cmd->_anonName = newName;
    Ufe::UndoableCommandMgr::instance().executeCmd(cmd);
    notify(CommandExecuted{});

    auto layerId = cmd->addedLayer();
    if (!layerId.empty()) {
        return pxr::SdfLayer::FindOrOpen(layerId);
    }

    return {};
}

void UfeCommandHook::muteSubLayer(UsdLayer usdLayer, bool muteIt)
{
    auto cmd = ::std::make_shared<MuteLayerCmd>(_sessionState->stage(), usdLayer, muteIt);
    AppendOrExecuteCommand(cmd);
}

void UfeCommandHook::showLayerEditorHelp()
{
    // TODO LE-EXTRACT Show layer editor help.
}

void UfeCommandHook::selectPrimsWithSpec(UsdLayer usdLayer)
{
    // TODO LE-EXTRACT Select prims with spec.
}

void UfeCommandHook::lockLayer(UsdLayer usdLayer, LayerLockType lockState, bool includeSubLayers)
{
    auto cmd = ::std::make_shared<LockLayerCmd>(
        _sessionState->stage(),
        usdLayer,
        lockState,
        includeSubLayers);
    AppendOrExecuteCommand(cmd);
}

void UfeCommandHook::stitchLayers(const std::vector<PXR_NS::SdfLayerRefPtr>& layers)
{
    if (layers.size() < 2)
        return;

    std::vector<std::string> identifiers;
    identifiers.reserve(layers.size());
    for (const auto& layer : layers) {
        if (layer)
            identifiers.push_back(layer->GetIdentifier());
    }

    if (identifiers.size() < 2)
        return;

    auto cmd = ::std::make_shared<StitchLayersCmd>(_sessionState->stage(), identifiers);
    AppendOrExecuteCommand(cmd);
}

void UfeCommandHook::refreshLayerSystemLock(UsdLayer usdLayer, bool refreshSubLayers)
{
    auto cmd = ::std::make_shared<RefreshSystemLockLayerCmd>(
        _sessionState->stage(), usdLayer, refreshSubLayers);
    
    // We do not want to populate the undo stack in the DCC with only the refresh command.
    // If we are in a composite command, append it so that the refresh is run after the command,
    // otherwise execute it directly but not via the manager.
    if (compositeCommand) {
        compositeCommand->append(cmd);
    } else {
        cmd->execute();
    }
}

void UfeCommandHook::AppendOrExecuteCommand(const Ufe::UndoableCommand::Ptr& cmd)
{
    if (compositeCommand) {
        compositeCommand->append(cmd);
    } else {
        Ufe::UndoableCommandMgr::instance().executeCmd(cmd);
        notify(CommandExecuted {});
    }
}

} // namespace UsdLayerEditor
