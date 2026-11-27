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
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <regex>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// tlhelp32.h needs the types windows.h defines.
// clang-format off
#include <windows.h>
#include <tlhelp32.h>
// clang-format on
#elif defined(__APPLE__)
#include <sys/proc.h>
#include <sys/proc_info.h>
#include <sys/types.h>

#include <libproc.h>
#include <signal.h>
#include <unistd.h>
#else
#include <sys/types.h>

#include <cstdlib>
#include <fstream>
#include <sstream>

#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace {

constexpr float kPollSeconds = 0.5f;
// Timer ticks to let pass after the watched processes exit. Maya notices the exit on its own
// schedule, and offers to cancel a render it still believes is running.
constexpr int kTicksAfterExit = 2;

#ifdef _WIN32
using ProcessId = DWORD;
#else
using ProcessId = pid_t;
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
// Reads the state and parent ID fields of /proc/<pid>/stat. The command name before them is in
// parentheses and can contain spaces.
bool readProcStat(const std::string& pid, char& state, pid_t& parentId)
{
    std::ifstream stat("/proc/" + pid + "/stat");
    std::string   content;
    if (!std::getline(stat, content)) {
        return false;
    }
    const auto nameEnd = content.rfind(')');
    if (nameEnd == std::string::npos) {
        return false;
    }
    std::istringstream fields(content.substr(nameEnd + 1));
    return static_cast<bool>(fields >> state >> parentId);
}
#endif

std::vector<ProcessId> childProcessIds()
{
    std::vector<ProcessId> ids;
#ifdef _WIN32
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return ids;
    }
    const DWORD     self = GetCurrentProcessId();
    PROCESSENTRY32W entry {};
    entry.dwSize = sizeof(entry);
    for (BOOL found = Process32FirstW(snapshot, &entry); found;
         found = Process32NextW(snapshot, &entry)) {
        if (entry.th32ParentProcessID == self) {
            ids.push_back(entry.th32ProcessID);
        }
    }
    CloseHandle(snapshot);
#elif defined(__APPLE__)
    const pid_t self = getpid();
    // proc_listchildpids returns a PID count; the buffer size is in bytes. The extra room covers
    // children spawned between the two calls.
    const int count = proc_listchildpids(self, nullptr, 0);
    if (count > 0) {
        ids.resize(static_cast<size_t>(count) + 16);
        const int listed
            = proc_listchildpids(self, ids.data(), static_cast<int>(ids.size() * sizeof(pid_t)));
        ids.resize(listed > 0 ? static_cast<size_t>(listed) : 0);
    }
#else
    const pid_t self = getpid();
    if (DIR* proc = opendir("/proc")) {
        while (const dirent* entry = readdir(proc)) {
            char*      end = nullptr;
            const long pid = std::strtol(entry->d_name, &end, 10);
            char       state = 0;
            pid_t      parentId = 0;
            if (pid > 0 && *end == '\0' && readProcStat(entry->d_name, state, parentId)
                && parentId == self) {
                ids.push_back(static_cast<pid_t>(pid));
            }
        }
        closedir(proc);
    }
#endif
    return ids;
}

// What isProcessRunning() needs to watch `id`. On Windows the handle keeps the ID from being
// reused, and is null when the process already exited.
std::intptr_t watchProcess(ProcessId id)
{
#ifdef _WIN32
    return reinterpret_cast<std::intptr_t>(OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, id));
#else
    return static_cast<std::intptr_t>(id);
#endif
}

bool isProcessRunning(std::intptr_t process)
{
    // Elsewhere than Windows, a zombie has exited but stays listed until Maya reaps it, and a
    // PID that is no longer a child of Maya was reused by another process.
#ifdef _WIN32
    return process && WaitForSingleObject(reinterpret_cast<HANDLE>(process), 0) == WAIT_TIMEOUT;
#elif defined(__APPLE__)
    struct proc_bsdinfo info {};
    return proc_pidinfo(static_cast<pid_t>(process), PROC_PIDTBSDINFO, 0, &info, sizeof(info))
        == static_cast<int>(sizeof(info))
        && info.pbi_ppid == static_cast<uint32_t>(getpid()) && info.pbi_status != SZOMB;
#else
    char  state = 0;
    pid_t parentId = 0;
    return readProcStat(std::to_string(process), state, parentId) && state != 'Z'
        && parentId == getpid();
#endif
}

// Maya notices the exit and resets its batch render state, as when the process ends on its own.
void terminateProcess(std::intptr_t process)
{
#ifdef _WIN32
    if (process) {
        TerminateProcess(reinterpret_cast<HANDLE>(process), 1);
    }
#else
    // A PID that is no longer a running child of Maya may belong to another process now.
    if (isProcessRunning(process)) {
        kill(static_cast<pid_t>(process), SIGTERM);
    }
#endif
}

void releaseProcess(std::intptr_t process)
{
#ifdef _WIN32
    if (process) {
        CloseHandle(reinterpret_cast<HANDLE>(process));
    }
#else
    (void)process;
#endif
}

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
    // The caller reuses the path, so the file already holds the scene the new commands render.
    if (_sceneCopy == sceneCopy) {
        _sceneCopy.clear();
    }
    // Processes of a canceled render: until Maya notices their exit, it offers to cancel the
    // render it believes is running, so the first command waits for them on the timer.
    std::vector<std::intptr_t> canceledProcesses;
    canceledProcesses.swap(_watchedProcesses);
    clear();
    _watchedProcesses = std::move(canceledProcesses);
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
    if (_watchedProcesses.empty()) {
        _startNext();
    }
}

void MayaBatchRenderQueue::cancel()
{
    for (const std::intptr_t process : _watchedProcesses) {
        terminateProcess(process);
    }
    _dropPending();
}

void MayaBatchRenderQueue::clear()
{
    _pending.clear();
    _stopWatching();
    if (_callbackIds.length() > 0) {
        MMessage::removeCallbacks(_callbackIds);
        _callbackIds.clear();
    }
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
    for (const std::intptr_t process : _watchedProcesses) {
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

    const std::vector<ProcessId> before = childProcessIds();
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
        for (const ProcessId id : childProcessIds()) {
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
