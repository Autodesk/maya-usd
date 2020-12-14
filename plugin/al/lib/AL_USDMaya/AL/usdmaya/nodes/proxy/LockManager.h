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

#include <AL/usdmaya/Api.h>

#include <pxr/usd/sdf/path.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {
namespace proxy {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class that maintains a list of locked and unlocked prims.
//----------------------------------------------------------------------------------------------------------------------
struct LockManager
{
    /// \brief  removes the specified paths from the locked and unlocked sets
    /// \param  entries the array of entires to remove.
    AL_USDMAYA_PUBLIC
    void removeEntries(const SdfPathVector& entries);

    /// \brief  removes all entries from the locked and unlocked sets that are a child of the
    /// specified path \param  path the root path to remove
    AL_USDMAYA_PUBLIC
    void removeFromRootPath(const SdfPath& path);

    /// \brief  sets the specified path as locked (and any paths that inherit their state from their
    /// parent).
    ///         If the path exists within the unlocked set, it will be removed.
    /// \param  path the path to insert into the locked set
    AL_USDMAYA_PUBLIC
    void setLocked(const SdfPath& path);

    /// \brief  sets the specified path as unlocked (and any paths that inherit their state from
    /// their parent).
    ///         If the path exists within the locked set, it will be removed.
    /// \param  path the path to insert into the unlocked set
    AL_USDMAYA_PUBLIC
    void setUnlocked(const SdfPath& path);

    /// \brief  adds the specified path to the locked prims list. No checking is done by this method
    /// to see whether
    ///         the path is part of the locked or unlocked sets. The intention for this method is to
    ///         quickly build up changes in the set of lock prims, and having done that, later sort
    ///         them by calling sort()
    /// \param  path the path to insert into the locked set
    inline void addLocked(const SdfPath& path) { m_lockedPrims.emplace_back(path); }

    /// \brief  adds the specified path to the unlocked prims list. No checking is done by this
    /// method to see whether
    ///         the path is part of the locked or unlocked sets. The intention for this method is to
    ///         quickly build up changes in the set of lock prims, and having done that, later sort
    ///         them by calling sort()
    /// \param  path the path to insert into the unlocked set
    inline void addUnlocked(const SdfPath& path) { m_unlockedPrims.emplace_back(path); }

    /// \brief  sorts the two sets of locked and unlocked prims for fast lookup
    inline void sort()
    {
        std::sort(m_lockedPrims.begin(), m_lockedPrims.end());
        std::sort(m_unlockedPrims.begin(), m_unlockedPrims.end());
    }

    /// \brief  Will remove the path from both the locked and unlocked sets. The lock status will
    /// now be inherited. \param  path the path to set as inherited
    AL_USDMAYA_PUBLIC
    void setInherited(const SdfPath& path);

    /// \brief  a query function to determine whether the specified path is locked or not.
    /// \param  path the path to query the lock status for
    /// \return true if the prim is locked (either by explicitly being set as locked, or by
    /// inheriting a lock status)
    AL_USDMAYA_PUBLIC
    bool isLocked(const SdfPath& path) const;

private:
    SdfPathVector m_lockedPrims;
    SdfPathVector m_unlockedPrims;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace proxy
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
