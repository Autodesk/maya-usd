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

#include "mayaBatchRenderQueue.h"

#include <mayaUsdUI/ui/stringResources.h>

#include <maya/MCommandMessage.h>
#include <maya/MGlobal.h>
#include <maya/MMessage.h>
#include <maya/MSceneMessage.h>
#include <maya/MTimerMessage.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <regex>

namespace {

constexpr float kPollSeconds = 0.5f;
// Timer ticks to let pass after the watched processes exit. Maya notices the exit on its own
// schedule, and offers to cancel a render it still believes is running.
constexpr int kTicksAfterExit = 2;

} // namespace

namespace MayaUsdRenderSetup {

MayaBatchRenderQueue& MayaBatchRenderQueue::instance()
{
    static MayaBatchRenderQueue queue;
    return queue;
}

MayaBatchRenderQueue::~MayaBatchRenderQueue() { clear(); }

void MayaBatchRenderQueue::start(std::vector<MString> commands, const std::string& sceneCopy)
{
    // The exit of the processes still watched is waited for again, from the new queue's timer.
    _ticksAfterExit = 0;
    _removeCallbacks();
    // A reused path already holds the scene the new commands render.
    if (_sceneCopy != sceneCopy) {
        _deleteSceneCopy();
    }
    _pending.assign(commands.begin(), commands.end());
    _sceneCopy = sceneCopy;
    if (_pending.empty()) {
        clear();
        return;
    }
    _callbackIds.append(MCommandMessage::addCommandCallback(_commandCallback, this));
    _callbackIds.append(MTimerMessage::addTimerCallback(kPollSeconds, _timerCallback, this));
    _callbackIds.append(
        MSceneMessage::addCallback(MSceneMessage::kBeforeNew, _sceneCallback, this));
    _callbackIds.append(
        MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, _sceneCallback, this));
    // Processes of a canceled render stay watched: until Maya notices their exit, it offers to
    // cancel the render it believes is running, so the first command waits for them on the timer.
    if (_watchedProcesses.empty()) {
        _startNext();
    }
}

void MayaBatchRenderQueue::cancel()
{
    // Maya notices the exit and resets its batch render state, as when the process ends on its own.
    for (const WatchedProcess process : _watchedProcesses) {
        terminateProcess(process);
    }
    _dropPending();
}

void MayaBatchRenderQueue::clear()
{
    _pending.clear();
    _stopWatching();
    _removeCallbacks();
    _deleteSceneCopy();
}

void MayaBatchRenderQueue::_removeCallbacks()
{
    if (_callbackIds.length() > 0) {
        MMessage::removeCallbacks(_callbackIds);
        _callbackIds.clear();
    }
}

void MayaBatchRenderQueue::_deleteSceneCopy()
{
    if (!_sceneCopy.empty()) {
        std::error_code ignored;
        std::filesystem::remove(_sceneCopy, ignored);
        _sceneCopy.clear();
    }
}

void MayaBatchRenderQueue::_dropPending()
{
    _pending.clear();
    // Otherwise the running render still reads the scene copy; _onTimer clears once it exits.
    if (_watchedProcesses.empty()) {
        clear();
    }
}

void MayaBatchRenderQueue::_stopWatching()
{
    for (const WatchedProcess process : _watchedProcesses) {
        releaseProcess(process);
    }
    _watchedProcesses.clear();
    _ticksAfterExit = 0;
}

/* static */
bool MayaBatchRenderQueue::stopsQueue(const std::string& command)
{
    // A bare batchRender is the renderer's cancel procedure; -renderCommandOptions or -filename
    // starts a render.
    static const std::regex kStop(
        R"re(^\s*batchRender\s*;?\s*$|)re"
        R"re(^\s*batchRender\b.*"?-(?:renderCommandOptions|rco|filename|f)"?\s)re");
    return std::regex_search(command, kStop);
}

void MayaBatchRenderQueue::_startNext()
{
    const MString command = _pending.front();
    _pending.pop_front();

    _stopWatching();

    const std::vector<ProcessId> before = getChildProcessIds();
    _launching = true;
    const MStatus status = MGlobal::executeCommand(command);
    _launching = false;
    // A scene change or plugin unload during the command cleared the queue.
    if (_callbackIds.length() == 0) {
        return;
    }

    // batchRender reports success even when it could not start the process, and starts none
    // when the user keeps a render already running.
    if (status) {
        for (const ProcessId id : getChildProcessIds()) {
            if (std::find(before.begin(), before.end(), id) == before.end()) {
                _watchedProcesses.push_back(watchProcess(id));
            }
        }
    }
    if (!_watchedProcesses.empty()) {
        return;
    }

    if (!_pending.empty()) {
        MString count;
        count += static_cast<int>(_pending.size());
        MString warning;
        warning.format(GET_MAYAUSDUI_MSTR(kBatchRenderQueueDropped), count);
        MGlobal::displayWarning(warning);
    }
    clear();
}

void MayaBatchRenderQueue::_onCommand(const std::string& command)
{
    if (!_launching && stopsQueue(command)) {
        _dropPending();
    }
}

void MayaBatchRenderQueue::_onTimer()
{
    if (_watchedProcesses.empty()
        || std::any_of(_watchedProcesses.begin(), _watchedProcesses.end(), isProcessRunning)) {
        return;
    }
    if (++_ticksAfterExit < kTicksAfterExit) {
        return;
    }
    // The last render is done with the scene copy.
    if (_pending.empty()) {
        clear();
        return;
    }
    _startNext();
}

/* static */
void MayaBatchRenderQueue::_commandCallback(const MString& command, void* clientData)
{
    // Called for every Maya command, so most are turned away before any copy or regex.
    const char* text = command.asChar();
    if (std::strstr(text, "batchRender")) {
        static_cast<MayaBatchRenderQueue*>(clientData)->_onCommand(text);
    }
}

/* static */
void MayaBatchRenderQueue::_timerCallback(float, float, void* clientData)
{
    static_cast<MayaBatchRenderQueue*>(clientData)->_onTimer();
}

/* static */
void MayaBatchRenderQueue::_sceneCallback(void* clientData)
{
    static_cast<MayaBatchRenderQueue*>(clientData)->_dropPending();
}

} // namespace MayaUsdRenderSetup
