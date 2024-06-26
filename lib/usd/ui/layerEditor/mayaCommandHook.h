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

#ifndef MAYACOMMANDHOOK_H
#define MAYACOMMANDHOOK_H

#include "abstractCommandHook.h"

#include <vector>

namespace UsdLayerEditor {

/**
 * @brief "hook" all the commands of the layer editor to execute them with mel commands
 *
 */
class MayaCommandHook : public AbstractCommandHook
{
public:
    MayaCommandHook(SessionState* in_sessionState)
        : AbstractCommandHook(in_sessionState)
    {
    }

    void setEditTarget(UsdLayer usdLayer) override;

    // insert a sub layer path at a given index
    void insertSubLayerPath(UsdLayer usdLayer, Path path, int index) override;

    // remove a sub layer by path
    void removeSubLayerPath(UsdLayer usdLayer, Path path) override;

    // replaces a path in the layer stack
    void replaceSubLayerPath(UsdLayer usdLayer, Path oldPath, Path newPath) override;

    // move a path at a given index inside the same layer or another layer.
    void
    moveSubLayerPath(Path path, UsdLayer oldParentUsdLayer, UsdLayer newParentUsdLayer, int index)
        override;

    // discard edit on a layer
    void discardEdits(UsdLayer usdLayer) override;

    // erases everything on a layer
    void clearLayer(UsdLayer usdLayer) override;

    // add an anon layer at the top of the stack, returns it
    UsdLayer addAnonymousSubLayer(UsdLayer usdLayer, std::string newName) override;

    // mute or unmute the given layer
    void muteSubLayer(UsdLayer usdLayer, bool muteIt) override;

    // lock, system-lock or unlock the given layer
    void
    lockLayer(UsdLayer usdLayer, MayaUsd::LayerLockType lockState, bool includeSubLayers) override;

    // Checks if the file layer or its sublayers are accessible on disk, and updates the system-lock
    // status.
    void refreshLayerSystemLock(UsdLayer usdLayer, bool refreshSubLayers = false) override;

    // starts a complex undo operation in the host app. Please use UndoContext class to safely
    // open/close
    void openUndoBracket(const QString& name) override;
    // closes a complex undo operation in the host app. Please use UndoContext class to safely
    // open/close
    void closeUndoBracket() override;

    // Help menu callback
    void showLayerEditorHelp() override;

    // this method is used to select the prims with spec in a layer
    void selectPrimsWithSpec(UsdLayer usdLayer) override;

    // this method is used to check if the stage in the proxy shape is from
    // an incoming connection (using instage data or cache id for example)
    bool isProxyShapeStageIncoming(const std::string& proxyShapePath) override;

    // this method is used to check if the proxy shape is sharing the composition
    // or has an owned root
    bool isProxyShapeSharedStage(const std::string& proxyShapePath) override;

protected:
    std::string proxyShapePath();

    std::string executeMel(const std::string& commandString);
    void        executePython(const std::string& commandString);

    void executeDelayedCommands() override;

    struct DelayedCommand
    {
        DelayedCommand(const std::string& cmd, bool isP)
            : command(cmd)
            , isPython(isP)
        {
        }

        std::string command;
        bool        isPython { false };
    };

    std::vector<DelayedCommand> _delayedCommands;
};

} // namespace UsdLayerEditor

#endif // MAYACOMMANDHOOK_H
