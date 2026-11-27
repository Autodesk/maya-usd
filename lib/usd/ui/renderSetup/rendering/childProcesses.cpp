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

#include "childProcesses.h"

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

#include <type_traits>

static_assert(std::is_same<MayaUsdRenderSetup::ProcessId, DWORD>::value, "ProcessId must be DWORD");
#elif defined(__APPLE__)
#include <sys/proc.h>
#include <sys/proc_info.h>

#include <libproc.h>
#include <signal.h>
#include <unistd.h>
#else
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace {

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

} // namespace

namespace MayaUsdRenderSetup {

std::vector<ProcessId> getChildProcessIds()
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

#ifdef _WIN32
namespace {
HANDLE toHandle(WatchedProcess process)
{
    return reinterpret_cast<HANDLE>(static_cast<std::intptr_t>(process));
}
} // namespace
#endif

WatchedProcess watchProcess(ProcessId id)
{
#ifdef _WIN32
    return static_cast<WatchedProcess>(
        reinterpret_cast<std::intptr_t>(OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, id)));
#else
    return static_cast<WatchedProcess>(id);
#endif
}

bool isProcessRunning(WatchedProcess process)
{
#ifdef _WIN32
    const HANDLE handle = toHandle(process);
    return handle && WaitForSingleObject(handle, 0) == WAIT_TIMEOUT;
#elif defined(__APPLE__)
    struct proc_bsdinfo info
    {
    };
    return proc_pidinfo(static_cast<pid_t>(process), PROC_PIDTBSDINFO, 0, &info, sizeof(info))
        == static_cast<int>(sizeof(info))
        && info.pbi_ppid == static_cast<uint32_t>(getpid()) && info.pbi_status != SZOMB;
#else
    char  state = 0;
    pid_t parentId = 0;
    return readProcStat(std::to_string(static_cast<pid_t>(process)), state, parentId)
        && state != 'Z' && parentId == getpid();
#endif
}

void terminateProcess(WatchedProcess process)
{
#ifdef _WIN32
    if (const HANDLE handle = toHandle(process)) {
        TerminateProcess(handle, 1);
    }
#else
    // A PID that is no longer a running child of this process may belong to another process now.
    if (isProcessRunning(process)) {
        kill(static_cast<pid_t>(process), SIGTERM);
    }
#endif
}

void releaseProcess(WatchedProcess process)
{
#ifdef _WIN32
    if (const HANDLE handle = toHandle(process)) {
        CloseHandle(handle);
    }
#else
    (void)process;
#endif
}

} // namespace MayaUsdRenderSetup
