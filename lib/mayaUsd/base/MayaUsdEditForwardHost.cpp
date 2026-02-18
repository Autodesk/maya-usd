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

#include <maya/MGlobal.h>

namespace {
bool idleTaskQueued = false;
}

void MayaUsdEditForwardHost::ExecuteInCmd(std::function<void()> callback, bool immediate)
{
    if (immediate) {
        if (callback) {
            callback();
        }
        return;
    }

    static std::vector<std::function<void()>> callbacks;
    callbacks.push_back(callback);

    if (idleTaskQueued) {
        // If we already have a task queue, it will run all callbacks.
        return;
    }

    MGlobal::executeTaskOnIdle([](void* data) {
        // Get a local copy before we iterate, in case callbacks themselves
        // append new callbacks.
        auto callbacksCopy = callbacks;
        callbacks.clear();
        idleTaskQueued = false;

        for (auto cb : callbacksCopy) {
            if (cb) {
                cb();
            }
        }
    });
    idleTaskQueued = true;
}

bool MayaUsdEditForwardHost::IsEditForwardingPaused() const { return _paused; }

void MayaUsdEditForwardHost::PauseEditForwarding(bool pause) { _paused = pause; }

void MayaUsdEditForwardHost::TrackLayerStates(const pxr::SdfLayerHandle& layer)
{
    // TODO : need to hook up maya-usd layer tracking for undo.
}
