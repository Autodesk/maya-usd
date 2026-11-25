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

#include <maya/MCommandMessage.h>
#include <maya/MGlobal.h>
#include <maya/MMessage.h>
#include <maya/MSceneMessage.h>
#include <maya/MTimerMessage.h>

#include <cstring>
#include <regex>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/types.h>

#include <cerrno>

#include <signal.h>
#endif

namespace {

constexpr float kPollSeconds = 0.5f;
// Timer ticks to let pass after the watched process exits. Maya notices the exit on its own
// schedule, and offers to cancel a render it still believes is running.
constexpr int kTicksAfterExit = 2;

bool isProcessRunning(int64_t processId)
{
#ifdef _WIN32
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(processId));
    if (!process) {
        // Access denied still means the process exists, just owned by someone else.
        return GetLastError() == ERROR_ACCESS_DENIED;
    }
    const bool running = WaitForSingleObject(process, 0) == WAIT_TIMEOUT;
    CloseHandle(process);
    return running;
#else
    // EPERM still means the process exists, just owned by someone else.
    return kill(static_cast<pid_t>(processId), 0) == 0 || errno == EPERM;
#endif
}

bool isStatusReport(const std::string& command)
{
    // Command callbacks see the flags quoted: batchRender "-status" "...".
    static const std::regex kStatus(R"(^\s*batchRender\s+"?-st(?:atus)?\b)");
    return std::regex_search(command, kStatus);
}

} // namespace

namespace MayaUsdRenderSetup {

MayaBatchRenderQueue& MayaBatchRenderQueue::instance()
{
    static MayaBatchRenderQueue queue;
    return queue;
}

MayaBatchRenderQueue::~MayaBatchRenderQueue() { clear(); }

void MayaBatchRenderQueue::start(std::vector<MString> commands)
{
    clear();
    _pending.assign(commands.begin(), commands.end());
    if (_pending.empty()) {
        return;
    }
    _callbackIds.append(MCommandMessage::addCommandCallback(_commandCallback, this));
    _callbackIds.append(MTimerMessage::addTimerCallback(kPollSeconds, _timerCallback, this));
    _callbackIds.append(
        MSceneMessage::addCallback(MSceneMessage::kBeforeNew, _sceneCallback, this));
    _callbackIds.append(
        MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, _sceneCallback, this));
    _startNext();
}

void MayaBatchRenderQueue::clear()
{
    _pending.clear();
    if (_callbackIds.length() > 0) {
        MMessage::removeCallbacks(_callbackIds);
        _callbackIds.clear();
    }
}

/* static */
std::optional<int64_t> MayaBatchRenderQueue::statusProcessId(const std::string& command)
{
    // The batch process sends "<percent> <pid> <file>", which Maya's command port prefixes with
    // "batchRender -status", quoted or not. The digit count keeps the ID within int64_t.
    static const std::regex kStatus(
        R"(^\s*batchRender\s+"?-st(?:atus)?"?\s+"\s*-?\d+\s+(\d{1,18})\b)");
    std::smatch match;
    if (!std::regex_search(command, match, kStatus)) {
        return {};
    }
    return std::stoll(match[1].str());
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

    // The first progress report can arrive while the launch command still runs.
    _watchedProcessId.reset();
    _ticksAfterExit = 0;
    _launching = true;
    const MStatus status = MGlobal::executeCommand(command);
    _launching = false;

    // Nothing left to start once the last command runs; a failed one stops the queue.
    if (!status || _pending.empty()) {
        clear();
    }
}

void MayaBatchRenderQueue::_onCommand(const std::string& command)
{
    // Only the first report of the render started last names the process to watch.
    if (isStatusReport(command)) {
        if (!_watchedProcessId) {
            _watchedProcessId = statusProcessId(command);
        }
        return;
    }

    if (!_launching && stopsQueue(command)) {
        clear();
    }
}

void MayaBatchRenderQueue::_onTimer()
{
    if (!_watchedProcessId || isProcessRunning(*_watchedProcessId)) {
        return;
    }
    if (++_ticksAfterExit < kTicksAfterExit) {
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
    static_cast<MayaBatchRenderQueue*>(clientData)->clear();
}

} // namespace MayaUsdRenderSetup
