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

#ifndef MAYAUSDUI_USD_RENDERSETUP_CHILDPROCESSES_H
#define MAYAUSDUI_USD_RENDERSETUP_CHILDPROCESSES_H

#include <mayaUsdUI/ui/api.h>

#include <cstdint>
#include <vector>

#ifndef _WIN32
#include <sys/types.h>
#endif

// Child processes of this process, watched without owning them: Maya spawns the batch render
// process itself, so it cannot be started through an API that would report its exit.
namespace MayaUsdRenderSetup {

#ifdef _WIN32
//! Process ID; matches DWORD, so that windows.h stays out of the header.
using ProcessId = unsigned long;
#else
using ProcessId = pid_t;
#endif

//! Opaque handle from watchProcess(), to release with releaseProcess().
enum class WatchedProcess : std::intptr_t
{
};

//! IDs of the processes this process spawned that are still listed by the system.
MAYAUSD_UI_PUBLIC std::vector<ProcessId> getChildProcessIds();

//! Starts watching child `id` for isProcessRunning() and terminateProcess(). On Windows the
//! handle keeps the ID from being reused, and may be null when the process already exited.
MAYAUSD_UI_PUBLIC WatchedProcess watchProcess(ProcessId id);

//! Whether the watched process is still running. Elsewhere than Windows, a zombie has exited
//! but stays listed until it is reaped, and a PID that is no longer a child of this process
//! was reused by another process.
MAYAUSD_UI_PUBLIC bool isProcessRunning(WatchedProcess process);

//! Ends the watched process if it is still running.
MAYAUSD_UI_PUBLIC void terminateProcess(WatchedProcess process);

//! Releases what watchProcess() acquired.
MAYAUSD_UI_PUBLIC void releaseProcess(WatchedProcess process);

} // namespace MayaUsdRenderSetup

#endif // MAYAUSDUI_USD_RENDERSETUP_CHILDPROCESSES_H
