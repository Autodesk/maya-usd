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

#ifndef ABSTRACTCOMMANDHOOK_H
#define ABSTRACTCOMMANDHOOK_H

// Needs to come first when used with VS2017 and Qt5.
#include "pxr/usd/sdf/layer.h"

#include <mayaUsd/utils/layerLocking.h>

#include <pxr/usd/usd/stage.h>

#include <QtCore/QString>

#include <string>

namespace UsdLayerEditor {

typedef PXR_NS::SdfLayerRefPtr UsdLayer;
typedef PXR_NS::UsdStageRefPtr UsdStage;

class SessionState;

/**
 * @brief The Abstract Command Hook contains all the methods which are used to modify USD layers and
 * stages. These methods will be "hooked" by the MayaCommandHook derived class to call maya mel
 * commands to do the work.
 *
 */

class AbstractCommandHook
{
public:
    typedef std::string const& Path;

    AbstractCommandHook(SessionState* in_sessionState)
        : _sessionState(in_sessionState)
    {
    }
    virtual ~AbstractCommandHook() { }

    // set stage edit target
    virtual void setEditTarget(UsdLayer usdLayer) = 0;

    // insert a sub layer path at a given index
    virtual void insertSubLayerPath(UsdLayer usdLayer, Path path, int index) = 0;

    // remove a sub layer by path
    virtual void removeSubLayerPath(UsdLayer usdLayer, Path path) = 0;

    // replaces a path in the layer stack
    virtual void replaceSubLayerPath(UsdLayer usdLayer, Path oldPath, Path newPath) = 0;

    // move a path at a given index inside the same layer or another layer.
    virtual void
    moveSubLayerPath(Path path, UsdLayer oldParentUsdLayer, UsdLayer newParentUsdLayer, int index)
        = 0;

    // discard edit on a layer
    virtual void discardEdits(UsdLayer usdLayer) = 0;

    // erases everything on a layer
    virtual void clearLayer(UsdLayer usdLayer) = 0;

    // add an anon layer at the top of the stack, returns it
    virtual UsdLayer addAnonymousSubLayer(UsdLayer usdLayer, std::string newName) = 0;

    // mute or unmute the given layer
    virtual void muteSubLayer(UsdLayer usdLayer, bool muteIt) = 0;

    // Sets the lock state on a layer
    virtual void
    lockLayer(UsdLayer usdLayer, MayaUsd::LayerLockType lockState, bool includeSubLayers)
        = 0;

    // Checks if the file layer or its sublayers are accessible on disk, and updates the system-lock
    // status.
    virtual void refreshLayerSystemLock(UsdLayer usdLayer, bool refreshSubLayers = false) = 0;

    // merge multiple layers into the strongest layer, removing them from their parents
    virtual void stitchLayers(
        const std::vector<PXR_NS::SdfLayerRefPtr>& layers,
        const std::vector<PXR_NS::SdfLayerRefPtr>& parents)
        = 0;

    // starts a complex undo operation in the host app. Please use UndoContext class to safely
    // open/close
    virtual void openUndoBracket(const QString& name) = 0;
    // closes a complex undo operation in the host app. Please use UndoContext class to safely
    // open/close
    virtual void closeUndoBracket() = 0;

    // Help menu callback
    virtual void showLayerEditorHelp() = 0;

    // this method is used to select the prims with spec in a layer
    virtual void selectPrimsWithSpec(UsdLayer usdLayer) = 0;

    // this method is used to check if the stage in the proxy shape is from
    // an incoming connection (using instage data or cache id for example)
    virtual bool isProxyShapeStageIncoming(const std::string& proxyShapePath) = 0;

    // this method is used to check if the proxy shape is sharing the composition
    // or has an owned root
    virtual bool isProxyShapeSharedStage(const std::string& proxyShapePath) = 0;

    // Increase the count tracking if command executions are delayed.
    void increaseDelayedCommands() { _delayCount += 1; }

    // Decrease the count tracking if command executions are delayed.
    void decreaseDelayedCommands()
    {
        _delayCount -= 1;
        if (!areCommandsDelayed())
            executeDelayedCommands();
    }

    // Verify if commands are currently delayed.
    bool areCommandsDelayed() const { return _delayCount > 0; }

protected:
    virtual void executeDelayedCommands() = 0;

    SessionState* _sessionState;
    int           _delayCount { 0 };
};

/**
 * @brief When executing multiple commands, it may sometimes be necessary to delay.
 *        the execution until all commands are issued. For example, when processing
 *        multiple elements in the selection, but the command itself might change the
 *        selection.
 */
class DelayAbstractCommandHook
{
public:
    DelayAbstractCommandHook(AbstractCommandHook& hook)
        : _hook(hook)
    {
        _hook.increaseDelayedCommands();
    }

    ~DelayAbstractCommandHook()
    {
        try {
            _hook.decreaseDelayedCommands();
        } catch (const std::exception&) {
            // Ignore exceptions in destructor.
        }
    }

private:
    AbstractCommandHook& _hook;
};

class UndoContext
{
public:
    UndoContext(AbstractCommandHook* in_parent, const QString& in_name)
        : _parent(in_parent)
    {
        in_parent->openUndoBracket(in_name);
    }
    ~UndoContext() { _parent->closeUndoBracket(); }
    AbstractCommandHook* hook() { return _parent; }

protected:
    AbstractCommandHook* _parent;
};

}; // namespace UsdLayerEditor

#endif // ABSTRACTCOMMANDHOOK_H
