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

#include <usdUfe/base/debugCodes.h>
#include <usdUfe/ufe/UsdUndoableCommand.h>
#include <usdUfe/ufe/trf/Utils.h>
#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/undo/UsdUndoManager.h>

#include <maya/MGlobal.h>
#include <ufe/undoableCommandMgr.h>

namespace {

bool idleTaskQueued = false;

bool IsInUsdUndoBlock() { return UsdUfe::UsdUndoBlock::depth() > 0; }

bool forwardingOpenedUndoChunk = false;

} // namespace

void MayaUsdEditForwardHost::ExecuteInCmd(std::function<void()> callback, bool immediate)
{
    // If requested to be run immediately, likely an explicit request to forward edits, 
    // we can just create an undoable command and run it (no need to consider the UndoBlock 
    // context etc. below, that is only relevant when reacting to changes in the scene).
    if (immediate) {
        if (callback) {
            auto cmd = MayaUsd::MayaUsdEditForwardCommand::create(callback);
            Ufe::UndoableCommandMgr::instance().executeCmd(cmd);
        }
        return;
    }

    // If we are not inside a USD undoable command, do not forward. We would not know how to.
    // This could be a script editing USD data, or an interactive edit (slider drag from
    // attribute editor).
    static MayaUsd::MayaUsdEditForwardCommand::Callbacks callbacks;
    const bool forwardEditsWithoutUsdUndoBlock = UsdUfe::NoUsdUndoBlockGuard::wantsForwarding();
    if (!forwardEditsWithoutUsdUndoBlock && !IsInUsdUndoBlock()) {
        TF_DEBUG_MSG(USDUFE_UNDOCMD, "No forwarding: not in set command and not in undo block\n");
        return;
    }

    // if we are in a undo block then we need an undo chunk to group the
    // command with the original edit command. Make sure to only open this chunk
    // once in case multiple commands are executed before the next on-idle.
    //
    // This undo chunk ensures that the original edit command and the
    // forward command executed on next idle are treated as one undo unit.
    // The chunk is opened here, while the current command is still executing
    // before it gets added to the stack.
    if (IsInUsdUndoBlock()) {
        if (!forwardingOpenedUndoChunk) {
            TF_DEBUG_MSG(USDUFE_UNDOCMD, "In undo block, opening undo chunk for forwarding\n");
            forwardingOpenedUndoChunk = true;
            MGlobal::executeCommand("undoInfo -openChunk");
        }
    }

    if (!idleTaskQueued) {
        idleTaskQueued = true;
        MGlobal::executeTaskOnIdle([](void* data) {
            // Get a local copy before we execute, in case callbacks themselves
            // append new callbacks.
            MayaUsd::MayaUsdEditForwardCommand::Callbacks callbacksCopy;
            callbacksCopy.swap(callbacks);
            idleTaskQueued = false;

            if (forwardingOpenedUndoChunk) {
                TF_DEBUG_MSG(USDUFE_UNDOCMD, "In undo block, forwarding using command\n");
                auto cmd = MayaUsd::MayaUsdEditForwardCommand::create(std::move(callbacksCopy));
                Ufe::UndoableCommandMgr::instance().executeCmd(cmd);
                forwardingOpenedUndoChunk = false;
                MGlobal::executeCommand("undoInfo -closeChunk");
            } else {
                TF_DEBUG_MSG(USDUFE_UNDOCMD, "No undo block, forwarding directly\n");
                for (const auto& cb : callbacksCopy) {
                    if (cb) {
                        cb();
                    }
                }
            }
        });
    }

    if (callback) {
        callbacks.push_back(callback);
    }
}

bool MayaUsdEditForwardHost::IsEditForwardingPaused() const
{
    // We always respect the pause flag.
    if (_paused)
        return true;

    // We never pause forwarding when in a transform set operation, as that
    // does not use a UsdUndoBlock in its execute, set, undo and redo functions.
    // IOW, even undo and redo need to forwarded.
    if (UsdUfe::NoUsdUndoBlockGuard::wantsForwarding()) {
        TF_DEBUG_MSG(USDUFE_UNDOCMD, "Forwarding is not paused: in transform set()\n");
        return false;
    }

    // When in undo/redo that *do* use UsdUndoBlocks, we pause forwarding.
    // This is because the UsdUndoBlock will correctly restore the data
    // and thus will not need to be forwarded.
    if (MGlobal::isUndoing() || MGlobal::isRedoing()) {
        TF_DEBUG_MSG(USDUFE_UNDOCMD, "Forwarding is paused: in undo/redo, \n");
        return true;
    }

    TF_DEBUG_MSG(USDUFE_UNDOCMD, "Forwarding is not paused\n");
    return false;
}

void MayaUsdEditForwardHost::PauseEditForwarding(bool pause) { _paused = pause; }

void MayaUsdEditForwardHost::TrackLayerStates(const pxr::SdfLayerHandle& layer)
{
    UsdUfe::UsdUndoManager::instance().trackLayerStates(layer);
}
