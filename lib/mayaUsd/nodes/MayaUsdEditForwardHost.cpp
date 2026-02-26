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
#include "MayaUsdEditForwardHost.h"

#include "MayaUsdEditForwardCommand.h"

#include <usdUfe/ufe/UsdUndoableCommand.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/undo/UsdUndoManager.h>

#include <maya/MGlobal.h>
#include <ufe/undoableCommandMgr.h>

namespace {
bool idleTaskQueued = false;

bool IsInUsdUndoBlock() { return UsdUfe::UsdUndoBlock::depth() > 0; }

} // namespace

void MayaUsdEditForwardHost::ExecuteInCmd(std::function<void()> callback, bool immediate)
{
    // If we are not inside a USD undoable command, do not forward. We woudln't know how to.
    // This could be a script editing USD data, or an interactive edit (slider drag from
    // attribute editor).
    static std::vector<std::function<void()>> callbacks;
    if (!IsInUsdUndoBlock()) {
        return;
    }

    if (!idleTaskQueued) {
        // Open an undo chunk so that the original edit command and the
        // forward command executed on next idle are treated as one undo unit.
        // The chunk is opened here, while the current command is still executing
        // before it gets added to the stack.
        MGlobal::executeCommand("undoInfo -openChunk");
        idleTaskQueued = true;

        MGlobal::executeTaskOnIdle([](void* data) {
            // Get a local copy before we execute, in case callbacks themselves
            // append new callbacks.
            auto callbacksCopy = callbacks;
            callbacks.clear();
            idleTaskQueued = false;

            auto cmd = MAYAUSD_NS_DEF::MayaUsdEditForwardCommand::create(std::move(callbacksCopy));
            Ufe::UndoableCommandMgr::instance().executeCmd(cmd);

            MGlobal::executeCommand("undoInfo -closeChunk");
        });
    }

    if (immediate) {
        if (callback) {
            auto cmd = MAYAUSD_NS_DEF::MayaUsdEditForwardCommand::create(callback);
            Ufe::UndoableCommandMgr::instance().executeCmd(cmd);
        }
    } else {
        if (callback) {
            callbacks.push_back(callback);
        }
    }
}

bool MayaUsdEditForwardHost::IsEditForwardingPaused() const
{
    return _paused || MGlobal::isUndoing() || MGlobal::isRedoing();
}

void MayaUsdEditForwardHost::PauseEditForwarding(bool pause) { _paused = pause; }

void MayaUsdEditForwardHost::TrackLayerStates(const pxr::SdfLayerHandle& layer)
{
    UsdUfe::UsdUndoManager::instance().trackLayerStates(layer);
}
