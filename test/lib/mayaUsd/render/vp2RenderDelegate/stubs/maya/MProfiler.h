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
#ifndef MAYAUSD_TEST_STUB_MPROFILER_H
#define MAYAUSD_TEST_STUB_MPROFILER_H

// Stand-in for <maya/MProfiler.h>, for unit tests that compile instrumented
// sources without Maya (testSubtreeScopingSceneIndex). Every call is a no-op,
// so the scopes cost nothing in the test.
//
// It declares only what those sources use. A source that uses more does not
// compile against it: add the missing piece here. Do not put it on the include
// path of a target that also has the Maya include directories.

class MProfiler
{
public:
    enum ProfilingColor
    {
        kColorC_L2
    };

    static int  addCategory(const char*, const char*) { return 0; }
    static bool recordingActive() { return false; }
};

class MProfilingScope
{
public:
    MProfilingScope(int, MProfiler::ProfilingColor, const char*, const char* = nullptr) { }
};

#endif
