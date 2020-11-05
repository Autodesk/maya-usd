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
#pragma once
#include <cassert>
#include <cstdint>
#include <ctime>
#include <ostream>
#include <string>
#include <unordered_map>

namespace AL {
namespace usdmaya {

//----------------------------------------------------------------------------------------------------------------------
/// \ingroup  profiler
/// a constant that determines the maximum possible depth of the timing
//----------------------------------------------------------------------------------------------------------------------
const uint32_t MAX_TIMESTAMP_STACK_SIZE = 16;

//----------------------------------------------------------------------------------------------------------------------
/// \ingroup  profilerprofiler
/// \brief  This class provides a static hash that should be unique for a line within a specific
/// function.
//----------------------------------------------------------------------------------------------------------------------
class ProfilerSectionTag
{
    friend class Profiler;

public:
    /// \brief  ctor
    /// \param  sectionName a human readable name for the profiling section
    /// \param  filePath the file that contains this code section
    /// \param  lineNumber the line number in the file where this section starts
    inline ProfilerSectionTag(
        const std::string sectionName,
        const std::string filePath,
        const size_t      lineNumber)
        : m_sectionName(sectionName)
        , m_filePath(filePath)
        , m_lineNumber(lineNumber)
        , m_hash(
              ((std::hash<std::string>()(filePath) << 1) ^ (std::hash<size_t>()(lineNumber) << 1))
              ^ std::hash<std::string>()(sectionName))
    {
    }

    /// \brief  equality operator
    /// \param  rhs right hand side to test for equality
    /// \return true if equal, false otherwise
    inline bool operator==(const ProfilerSectionTag& rhs) const
    {
        return m_hash == rhs.m_hash && m_sectionName == rhs.m_sectionName
            && m_filePath == rhs.m_filePath && m_lineNumber == rhs.m_lineNumber;
    }

    /// \brief  return the hash of this class
    /// \return the hash of this class
    inline size_t hash() const { return m_hash; }

private:
    const std::string m_sectionName; ///< the human readable identifier for this section
    const std::string m_filePath;    ///< the file that contains this code section
    const size_t      m_lineNumber;  ///< the line number within the file
    const size_t      m_hash;        ///< unique hash to identify this section
};

//----------------------------------------------------------------------------------------------------------------------
/// \ingroup  profiler
/// \brief  This class represents a path made up of AL::usdmaya::ProfilerSectionTag's. It is used so
/// that we can distinguish between
///         identical code sections, accessed from alternative paths, e.g.
/// \code
/// void func1() {
///   AL_BEGIN_PROFILE_SECTION(func1);
///   AL_END_PROFILE_SECTION();
/// }
/// void func2() {
///   AL_BEGIN_PROFILE_SECTION(func2);
///   func1();
///   AL_END_PROFILE_SECTION();
/// }
/// void func3() {
///   AL_BEGIN_PROFILE_SECTION(func3);
///   func1();
///   AL_END_PROFILE_SECTION();
/// }
/// \endcode
/// In this case, we can access func1 via two paths: |func2|func1, and |func3|func1
//----------------------------------------------------------------------------------------------------------------------
class ProfilerSectionPath
{
    friend class Profiler;

public:
    /// \brief  ctor
    /// \param  top the top node on the profiling stack
    /// \param  parent the parent path
    inline ProfilerSectionPath(
        const AL::usdmaya::ProfilerSectionTag* const top,
        const ProfilerSectionPath* const             parent = 0)
        : m_top(top)
        , m_parent(parent)
        , m_hash(m_top->hash() ^ (m_parent ? m_parent->hash() : 0))
    {
    }

    /// \brief  equality operator
    /// \param  rhs the right hand side of the comparison
    /// \return true if the paths are the same
    inline bool operator==(const ProfilerSectionPath& rhs) const
    {
        return (m_hash == rhs.m_hash && m_top == rhs.m_top && m_parent == rhs.m_parent);
    }

    /// \brief  return the hash of this class
    /// \return the hash of this class
    inline size_t hash() const { return m_hash; }

private:
    const ProfilerSectionTag* const  m_top;
    const ProfilerSectionPath* const m_parent;
    const size_t                     m_hash;
};
} // namespace usdmaya
} // namespace AL

//----------------------------------------------------------------------------------------------------------------------
#ifndef AL_GENERATING_DOCS
namespace std {
template <> struct hash<AL::usdmaya::ProfilerSectionTag>
{
    inline size_t operator()(const AL::usdmaya::ProfilerSectionTag& k) const { return k.hash(); }
};
template <> struct hash<AL::usdmaya::ProfilerSectionPath>
{
    inline size_t operator()(const AL::usdmaya::ProfilerSectionPath& k) const { return k.hash(); }
};
} // namespace std
#endif

//----------------------------------------------------------------------------------------------------------------------
namespace AL {
namespace usdmaya {
//----------------------------------------------------------------------------------------------------------------------
/// \ingroup  profiler
/// \brief  This class implements a very simple incode profiler. This profiler is NOT thread safe.
/// It is mainly used to
///         get some basic stats on the where the bottlenecks are during a file import/export
///         operation. A simple example of usage:
/// \code
/// void func1() {
///   AL_BEGIN_PROFILE_SECTION(func1);
///   Sleep(1);
///   AL_END_PROFILE_SECTION();
/// }
/// void func2() {
///   AL_BEGIN_PROFILE_SECTION(func2);
///   func1();
///   Sleep(1);
///   AL_END_PROFILE_SECTION();
/// }
/// void func3() {
///   AL_BEGIN_PROFILE_SECTION(func3);
///   func1();
///   Sleep(1);
///   AL_END_PROFILE_SECTION();
/// }
///
/// void myBigFunction()
/// {
///   AL_BEGIN_PROFILE_SECTION(myBigFunction);
///   func2();
///   func3();
///   AL_BEGIN_END_SECTION();
///   AL::usdmaya::Profiler::printReport(std::cout);
/// }
/// \endcode
//----------------------------------------------------------------------------------------------------------------------
class Profiler
{
public:
    /// \brief  call to output the report
    /// \param  os the stream to write the report to
    static void printReport(std::ostream& os);

    /// \brief  call to clear internal timers
    static inline void clearAll()
    {
        assert(m_stackPos == 0);
        m_map.clear();
    }

    /// \brief  do not call directly. Use the AL_BEGIN_PROFILE_SECTION macro
    /// \param  entry a unique tag for this code section.
    static void pushTime(const ProfilerSectionTag* entry);

    /// \brief  do not call directly. Use the AL_END_PROFILE_SECTION macro
    static void popTime();

private:
    typedef std::unordered_map<ProfilerSectionPath, timespec> ProfilerSectionPathLUT;
    typedef ProfilerSectionPathLUT::const_iterator            iter_t;
    static inline bool compareTimeStamps(const iter_t, const iter_t);
    static void print(std::ostream& os, iter_t, const ProfilerSectionPathLUT&, uint32_t, double);

    struct ProfilerSectionStackNode
    {
        timespec                         m_time;
        const ProfilerSectionTag*        m_entry;
        ProfilerSectionPathLUT::iterator m_path;
    };

    static inline timespec timeDiff(const timespec startTime, const timespec endTime)
    {
        timespec ts;
        if (endTime.tv_nsec < startTime.tv_nsec) {
            ts.tv_sec = endTime.tv_sec - 1 - startTime.tv_sec;
            ts.tv_nsec = 1000000000 + endTime.tv_nsec - startTime.tv_nsec;
        } else {
            ts.tv_sec = endTime.tv_sec - startTime.tv_sec;
            ts.tv_nsec = endTime.tv_nsec - startTime.tv_nsec;
        }
        return ts;
    }
    static inline timespec timeAdd(timespec t1, timespec t2)
    {
        timespec ts;
        int32_t  sec = t2.tv_sec + t1.tv_sec;
        int32_t  nsec = t2.tv_nsec + t1.tv_nsec;
        if (nsec >= 1000000000) {
            nsec -= 1000000000;
            sec++;
        }
        ts.tv_sec = sec;
        ts.tv_nsec = nsec;
        return ts;
    }

    static ProfilerSectionStackNode m_timeStack[MAX_TIMESTAMP_STACK_SIZE];
    static uint32_t                 m_stackPos;
    static ProfilerSectionPathLUT   m_map;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------

/// \ingroup  profiler
/// Put this macro at the start of a timed section of code
#define AL_BEGIN_PROFILE_SECTION(TimedSection)                                                   \
    {                                                                                            \
        static const AL::usdmaya::ProfilerSectionTag __entry(#TimedSection, __FILE__, __LINE__); \
        AL::usdmaya::Profiler::pushTime(&__entry);                                               \
    }

/// \ingroup  profiler
/// Put this macro after a timed section of code.
#define AL_END_PROFILE_SECTION()          \
    {                                     \
        AL::usdmaya::Profiler::popTime(); \
    }
