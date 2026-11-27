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

#ifndef MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERQUEUE_H
#define MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERQUEUE_H

#include <mayaUsdUI/ui/api.h>

#include <maya/MCallbackIdArray.h>
#include <maya/MString.h>

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace MayaUsdRenderSetup {

//! Runs batchRender commands one after another.
//!
//! Maya runs a single batch render at a time and tells plugins nothing when it ends. Maya
//! spawns the batch process while the `batchRender` command runs, well before the process
//! reports any progress, so the queue watches the processes Maya gained during the command and
//! starts the next one once they have all exited. If the command spawns none, the remaining
//! renders are dropped with a warning.
class MAYAUSD_UI_PUBLIC MayaBatchRenderQueue
{
public:
    static MayaBatchRenderQueue& instance();

    ~MayaBatchRenderQueue();

    MayaBatchRenderQueue(const MayaBatchRenderQueue&) = delete;
    MayaBatchRenderQueue& operator=(const MayaBatchRenderQueue&) = delete;

    //! Drops any renders not started yet, then runs `commands` in order. Opening or creating a
    //! scene drops the renders not started yet, as they would render the other scene.
    //! `sceneCopy`, the file the commands render when given, is deleted once the queue is done.
    //! After cancel(), the first command waits for the canceled processes to exit.
    void start(std::vector<MString> commands, const std::string& sceneCopy = {});

    //! Drops the renders not started yet and deletes the scene copy; a running one carries on.
    void clear();

    //! Kills the running render and drops the renders not started yet. The scene copy is
    //! deleted once the killed processes exit, unless start() reuses it.
    void cancel();

    //! Whether a render is running or waiting to start.
    bool isRunning() const { return !_pending.empty() || !_watchedProcesses.empty(); }

    //! Number of renders not started yet.
    size_t pendingCount() const { return _pending.size(); }

    //! Whether `command` cancels the batch render or starts another one, either of which drops
    //! the renders not started yet. Showing the image or reporting progress does not.
    static bool stopsQueue(const std::string& command);

private:
    MayaBatchRenderQueue() = default;

    void _startNext();
    void _dropPending();
    void _stopWatching();
    void _onCommand(const std::string& command);
    void _onTimer();

    static void _commandCallback(const MString& command, void* clientData);
    static void _timerCallback(float elapsedTime, float lastTime, void* clientData);
    static void _sceneCallback(void* clientData);

    std::deque<MString> _pending;
    std::string         _sceneCopy;
    MCallbackIdArray    _callbackIds;
    // Set while one of our own commands runs, so it is not taken for a user cancel.
    bool _launching { false };
    // Batch processes Maya spawned for the render started last: process HANDLEs on Windows,
    // process IDs elsewhere.
    std::vector<std::intptr_t> _watchedProcesses;
    int                        _ticksAfterExit { 0 };
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERQUEUE_H
