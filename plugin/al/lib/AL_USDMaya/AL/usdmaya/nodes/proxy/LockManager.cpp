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
#include "AL/usdmaya/nodes/proxy/LockManager.h"

#include <maya/MProfiler.h>
namespace {
const int _lockManagerProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "LockManager",
    "LockManager"
#else
    "LockManager"
#endif
);
} // namespace

namespace AL {
namespace usdmaya {
namespace nodes {
namespace proxy {

//----------------------------------------------------------------------------------------------------------------------
void LockManager::removeEntries(const SdfPathVector& entries)
{
    MProfilingScope profilerScope(
        _lockManagerProfilerCategory, MProfiler::kColorE_L3, "Remove entries");

    auto lockIt = m_lockedPrims.begin();
    auto lockEnd = m_lockedPrims.end();

    auto unlockIt = m_unlockedPrims.begin();
    auto unlockEnd = m_unlockedPrims.end();

    for (auto it = entries.begin(), end = entries.end(); lockIt != lockEnd && it != end; ++it) {
        const SdfPath path = *it;
        lockIt = std::lower_bound(lockIt, lockEnd, path);
        if (lockIt != lockEnd && *lockIt == path) {
            lockIt = m_lockedPrims.erase(lockIt);
        }
    }

    for (auto it = entries.begin(), end = entries.end(); unlockIt != unlockEnd && it != end; ++it) {
        const SdfPath path = *it;
        unlockIt = std::lower_bound(unlockIt, unlockEnd, path);
        if (unlockIt != unlockEnd && *unlockIt == path) {
            unlockIt = m_unlockedPrims.erase(unlockIt);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void LockManager::removeFromRootPath(const SdfPath& path)
{
    MProfilingScope profilerScope(
        _lockManagerProfilerCategory, MProfiler::kColorE_L3, "Remove from root path");

    {
        // find the start of the entries for this path
        auto lb = std::lower_bound(m_lockedPrims.begin(), m_lockedPrims.end(), path);
        auto ub = lb;

        // keep walking forwards until we hit a new branch in the tree
        while (ub != m_lockedPrims.end()) {
            if (ub->HasPrefix(path)) {
                ++ub;
            } else {
                break;
            }
        }

        // if we find a valid range, erase the previous entries
        if (lb != ub) {
            m_lockedPrims.erase(lb, ub);
        }
    }

    // remove all previous prims from m_lockInheritedPrims that are children of the current prim.
    {
        // find the start of the entries for this path
        auto lb = std::lower_bound(m_unlockedPrims.begin(), m_unlockedPrims.end(), path);
        auto ub = lb;

        // keep walking forwards until we hit a new branch in the tree
        while (ub != m_unlockedPrims.end()) {
            if (ub->HasPrefix(path)) {
                ++ub;
            } else {
                break;
            }
        }

        // if we find a valid range, erase the previous entries
        if (lb != ub) {
            m_unlockedPrims.erase(lb, ub);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void LockManager::setLocked(const SdfPath& path)
{
    auto lockIter = std::lower_bound(m_lockedPrims.begin(), m_lockedPrims.end(), path);
    if (lockIter != m_lockedPrims.end()) {
        // lock already in the locked array, so return
        if (*lockIter == path)
            return;
    }
    m_lockedPrims.insert(lockIter, path);

    auto unlockIter = std::lower_bound(m_unlockedPrims.begin(), m_unlockedPrims.end(), path);
    if (unlockIter != m_unlockedPrims.end()) {
        if (*unlockIter == path) {
            m_unlockedPrims.erase(unlockIter);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void LockManager::setUnlocked(const SdfPath& path)
{
    MProfilingScope profilerScope(
        _lockManagerProfilerCategory, MProfiler::kColorE_L3, "Set unlocked");

    auto unlockIter = std::lower_bound(m_unlockedPrims.begin(), m_unlockedPrims.end(), path);
    if (unlockIter != m_unlockedPrims.end()) {
        // lock already in the locked array, so return
        if (*unlockIter == path)
            return;
    }
    m_unlockedPrims.insert(unlockIter, path);

    auto lockIter = std::lower_bound(m_lockedPrims.begin(), m_lockedPrims.end(), path);
    if (lockIter != m_lockedPrims.end()) {
        if (*lockIter == path) {
            m_lockedPrims.erase(lockIter);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void LockManager::setInherited(const SdfPath& path)
{
    MProfilingScope profilerScope(
        _lockManagerProfilerCategory, MProfiler::kColorE_L3, "Set inherited");

    if (!m_unlockedPrims.empty()) {
        auto unlockIter = std::lower_bound(m_unlockedPrims.begin(), m_unlockedPrims.end(), path);
        if (unlockIter != m_unlockedPrims.end()) {
            // lock already in the locked array, so return
            if (*unlockIter == path) {
                m_unlockedPrims.erase(unlockIter);
            }
        }
    }
    if (!m_lockedPrims.empty()) {
        auto lockIter = std::lower_bound(m_lockedPrims.begin(), m_lockedPrims.end(), path);
        if (lockIter != m_lockedPrims.end()) {
            // lock already in the locked array, so return
            if (*lockIter == path) {
                m_lockedPrims.erase(lockIter);
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
bool LockManager::isLocked(const SdfPath& path) const
{
    MProfilingScope profilerScope(
        _lockManagerProfilerCategory, MProfiler::kColorE_L3, "Check locked state");

    if (m_lockedPrims.empty())
        return false;

    auto lockIter = std::lower_bound(m_lockedPrims.begin(), m_lockedPrims.end(), path);

    // first check the edge case where this path is in the locked set
    if (lockIter != m_lockedPrims.end() && *lockIter == path) {
        return true;
    }

    // now attempt to see if a parent of this path is in the locked set
    bool hasLockedParent = false;
    if (lockIter != m_lockedPrims.begin()) {
        --lockIter;
        {
            if (path.HasPrefix(*lockIter)) {
                hasLockedParent = true;
            }
        }
    }

    // now attempt to see if a parent of this path is in the unlocked set
    bool hasUnlockedParent = false;
    auto unlockIter = std::lower_bound(m_unlockedPrims.begin(), m_unlockedPrims.end(), path);

    // first check the edge case where this path is in the locked set
    if (unlockIter != m_unlockedPrims.end() && *unlockIter == path) {
        return false;
    }

    if (unlockIter != m_unlockedPrims.begin()) {
        --unlockIter;
        {
            if (path.HasPrefix(*unlockIter)) {
                hasUnlockedParent = true;
            }
        }
    }

    // if we have entries in both the locked and unlocked set.....
    if (hasLockedParent && hasUnlockedParent) {
        // if the locked path is longer than the unlocked path, the closest parent to path is
        // locked.
        return (lockIter->GetString().size() >= unlockIter->GetString().size());
    } else if (hasLockedParent) {
        return true;
    }

    // either the parent is unlocked, or no entry exists at all (treat as unlocked).
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace proxy
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
