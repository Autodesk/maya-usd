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
#ifndef USDLAYEREDITOR_UFECOMMANDHOOK_H
#define USDLAYEREDITOR_UFECOMMANDHOOK_H

#include "abstractCommandHook.h"
#include "ufeCommandHook.h"

#include <ufe/batchCompositeCommand.h>
#include <ufe/notification.h>
#include <ufe/subject.h>

namespace UsdLayerEditor {

/**
 * @brief "hook" all the commands of the layer editor through UFE undoable commands.
 *
 */
class LAYEREDITOR_PUBLIC UfeCommandHook
    : public AbstractCommandHook
    , public Ufe::Subject
{
public:
    UfeCommandHook(SessionState* in_sessionState)
        : AbstractCommandHook(in_sessionState)
    {
    }

    // AbstractCommandHook overrides.
    void setEditTarget(UsdLayer usdLayer) override;
    void insertSubLayerPath(UsdLayer usdLayer, Path path, int index) override;
    void removeSubLayerPath(UsdLayer usdLayer, Path path) override;
    void replaceSubLayerPath(UsdLayer usdLayer, Path oldPath, Path newPath) override;
    void
    moveSubLayerPath(Path path, UsdLayer oldParentUsdLayer, UsdLayer newParentUsdLayer, int index)
        override;
    void     discardEdits(UsdLayer usdLayer) override;
    void     clearLayer(UsdLayer usdLayer) override;
    void     flattenLayer(UsdLayer usdLayer) override;
    UsdLayer addAnonymousSubLayer(UsdLayer usdLayer, std::string newName) override;
    void     muteSubLayer(UsdLayer usdLayer, bool muteIt) override;
    void     openUndoBracket(const QString& name) override;
    void     closeUndoBracket() override;
    void     showLayerEditorHelp() override;
    void     selectPrimsWithSpec(UsdLayer usdLayer) override;
    void     lockLayer(UsdLayer usdLayer, LayerLockType lockState, bool includeSubLayers) override;
    void     refreshLayerSystemLock(UsdLayer usdLayer, bool refreshSubLayers = false) override;
    void     stitchLayers(const std::vector<PXR_NS::SdfLayerRefPtr>& layers) override;

    // If we are within an undo "bracket", append the command. Otherwise, execute it immediately.
    void AppendOrExecuteCommand(const Ufe::UndoableCommand::Ptr& cmd);

    class CommandExecuted : public Ufe::Notification
    {
    };

protected:
    void executeDelayedCommands() override;

    class LayedEditorCommand : public Ufe::CompositeUndoableCommand
    {
    public:
        typedef std::shared_ptr<LayedEditorCommand> Ptr;

        LayedEditorCommand(const std::string commandString) { _commandString = commandString; }
        std::string commandString() const override { return _commandString; }

    private:
        std::string _commandString;
    };

    LayedEditorCommand::Ptr compositeCommand = nullptr;
};

} // namespace UsdLayerEditor

#endif
