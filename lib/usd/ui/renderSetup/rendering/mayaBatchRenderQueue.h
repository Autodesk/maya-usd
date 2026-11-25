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
#include <optional>
#include <string>
#include <vector>

namespace MayaUsdRenderSetup {

//! Runs batchRender commands one after another.
//!
//! Maya runs a single batch render at a time and tells plugins nothing when it ends. The batch
//! process reports its progress back to Maya as `batchRender -status` commands carrying its
//! process ID, so the queue reads that ID from the first report and starts the next command once
//! that process has exited.
class MAYAUSD_UI_PUBLIC MayaBatchRenderQueue
{
public:
    static MayaBatchRenderQueue& instance();

    ~MayaBatchRenderQueue();

    MayaBatchRenderQueue(const MayaBatchRenderQueue&) = delete;
    MayaBatchRenderQueue& operator=(const MayaBatchRenderQueue&) = delete;

    //! Drops any renders not started yet, then runs `commands` in order. Opening or creating a
    //! scene drops the renders not started yet, as they would render the other scene.
    void start(std::vector<MString> commands);

    //! Drops the renders not started yet; a running one carries on.
    void clear();

    //! Number of renders not started yet.
    size_t pendingCount() const { return _pending.size(); }

    //! Process ID in a `batchRender -status "<percent> <pid> <file>"` progress report; empty for
    //! any other command, or a report it cannot be read from.
    static std::optional<int64_t> statusProcessId(const std::string& command);

    //! Whether `command` cancels the batch render or starts another one, either of which drops
    //! the renders not started yet. Showing the image or reporting progress does not.
    static bool stopsQueue(const std::string& command);

private:
    MayaBatchRenderQueue() = default;

    void _startNext();
    void _onCommand(const std::string& command);
    void _onTimer();

    static void _commandCallback(const MString& command, void* clientData);
    static void _timerCallback(float elapsedTime, float lastTime, void* clientData);
    static void _sceneCallback(void* clientData);

    std::deque<MString> _pending;
    MCallbackIdArray    _callbackIds;
    // Set while one of our own commands runs, so it is not taken for a user cancel.
    bool _launching { false };
    // Empty until the batch process started last reports its process ID.
    std::optional<int64_t> _watchedProcessId;
    int                    _ticksAfterExit { 0 };
};

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_MAYABATCHRENDERQUEUE_H
