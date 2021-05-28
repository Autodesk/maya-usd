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
    void moveSubLayerPath(Path path, UsdLayer oldParentUsdLayer, UsdLayer newParentUsdLayer, int index) override;

    // discard edit on a layer
    void discardEdits(UsdLayer usdLayer) override;

    // erases everything on a layer
    void clearLayer(UsdLayer usdLayer) override;

    // add an anon layer at the top of the stack, returns it
    UsdLayer addAnonymousSubLayer(UsdLayer usdLayer, std::string newName) override;

    // mute or unmute the given layer
    void muteSubLayer(UsdLayer usdLayer, bool muteIt) override;

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

protected:
    std::string proxyShapePath();
};

} // namespace UsdLayerEditor

#endif // MAYACOMMANDHOOK_H
