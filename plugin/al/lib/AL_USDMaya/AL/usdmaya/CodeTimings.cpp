//
// Copyright 2017 Animal Logic
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
#include "AL/usdmaya/CodeTimings.h"

#include <algorithm>
#include <vector>

#ifdef OSMac_
// For clock_gettime()
#include <time.h>
#endif

namespace AL {
namespace usdmaya {

//----------------------------------------------------------------------------------------------------------------------
Profiler::ProfilerSectionStackNode Profiler::m_timeStack[MAX_TIMESTAMP_STACK_SIZE];
uint32_t                           Profiler::m_stackPos = 0;
Profiler::ProfilerSectionPathLUT   Profiler::m_map;

//----------------------------------------------------------------------------------------------------------------------
inline bool Profiler::compareTimeStamps(const iter_t a, const iter_t b)
{
    if (a->second.tv_sec > b->second.tv_sec)
        return true;
    if (a->second.tv_sec < b->second.tv_sec)
        return false;
    return a->second.tv_nsec > b->second.tv_nsec;
}

#define INDENT                            \
    for (uint32_t i = 0; i < indent; ++i) \
        os << "  ";

//----------------------------------------------------------------------------------------------------------------------
void Profiler::print(
    std::ostream&                 os,
    iter_t                        it,
    const ProfilerSectionPathLUT& lut,
    uint32_t                      indent,
    double                        total)
{
    const ProfilerSectionPath&             hp = it->first;
    const AL::usdmaya::ProfilerSectionTag& he = *hp.m_top;

    double timeTaken = (it->second.tv_sec * 1000.0f + it->second.tv_nsec * 0.000001f);
    double percentage = timeTaken / total;
    percentage = int(10000.0 * percentage) * 0.01;

    INDENT;
    if (timeTaken > 20000.0) {
        os << "[" << percentage << "%](" << (timeTaken * 0.001) << "S) " << he.m_sectionName
           << std::endl;
    } else {
        os << "[" << percentage << "%](" << timeTaken << "ms) " << he.m_sectionName << std::endl;
    }

    std::vector<iter_t> sorted;
    for (auto a = lut.begin(), e = lut.end(); a != e; ++a) {
        if (a->first.m_parent == &hp) {
            sorted.push_back(a);
        }
    }
    std::sort(sorted.begin(), sorted.end(), compareTimeStamps);

    for (auto a = sorted.begin(), e = sorted.end(); a != e; ++a) {
        print(os, *a, lut, indent + 1, total);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void Profiler::printReport(std::ostream& os)
{
    double              total = 0;
    std::vector<iter_t> sorted;
    for (auto a = m_map.begin(), e = m_map.end(); a != e; ++a) {
        if (!a->first.m_parent) {
            total += (a->second.tv_sec * 1000.0f + a->second.tv_nsec * 0.000001f);
            sorted.push_back(a);
        }
    }
    std::sort(sorted.begin(), sorted.end(), compareTimeStamps);

    for (auto a = sorted.begin(), e = sorted.end(); a != e; ++a) {
        print(os, *a, m_map, 0, total);
    }

    clearAll();
}

#ifdef _WIN32

#include <Windows.h>
int clock_gettime(int, struct timespec* spec) // C-file part
{
    __int64 wintime;
    GetSystemTimeAsFileTime((FILETIME*)&wintime);
    wintime -= 116444736000000000i64;            // 1jan1601 to 1jan1970
    spec->tv_sec = wintime / 10000000i64;        // seconds
    spec->tv_nsec = wintime % 10000000i64 * 100; // nano-seconds
    return 0;
}
#define CLOCK_REALTIME_COARSE 0

#endif

//----------------------------------------------------------------------------------------------------------------------
void Profiler::pushTime(const AL::usdmaya::ProfilerSectionTag* entry)
{
    assert(MAX_TIMESTAMP_STACK_SIZE > m_stackPos);
    m_timeStack[m_stackPos].m_entry = entry;
#ifdef _WIN32
    while (clock_gettime(CLOCK_REALTIME_COARSE, &m_timeStack[m_stackPos].m_time)
           != 0) /* deliberately empty */
        ;
#endif

    const timespec null_ts = { 0, 0 };
    if (m_stackPos) {
        auto                parent = m_timeStack[m_stackPos - 1].m_path;
        ProfilerSectionPath path(entry, &parent->first);
        auto                it = m_map.find(path);
        if (it != m_map.end()) {
            m_timeStack[m_stackPos].m_path = it;
        } else {
            m_map.insert(std::make_pair(path, null_ts));
            m_timeStack[m_stackPos].m_path = m_map.find(path);
        }
    } else {
        ProfilerSectionPath path(entry, 0);
        auto                it = m_map.find(path);
        if (it != m_map.end()) {
            m_timeStack[m_stackPos].m_path = it;
        } else {
            m_map.insert(std::make_pair(path, null_ts));
            m_timeStack[m_stackPos].m_path = m_map.find(path);
        }
    }

    ++m_stackPos;
}

//----------------------------------------------------------------------------------------------------------------------
void Profiler::popTime()
{
    assert(m_stackPos > 0);
    --m_stackPos;
    timespec endtime {};
#ifdef _WIN32
    while (clock_gettime(CLOCK_REALTIME_COARSE, &endtime) != 0) /* deliberately empty */
        ;
#elif OSMac_
    // clock_gettime() is only present on OSX 10.12 and above.
    clock_gettime(CLOCK_REALTIME, &endtime);
#else
    timespec_get(&endtime, TIME_UTC);
#endif

    // compute time difference
    timespec diff = timeDiff(m_timeStack[m_stackPos].m_time, endtime);
    m_timeStack[m_stackPos].m_path->second = timeAdd(diff, m_timeStack[m_stackPos].m_path->second);
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
