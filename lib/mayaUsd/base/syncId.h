//
// Copyright 2020 Autodesk
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

#ifndef MAYAUSD_SYNCID_H
#define MAYAUSD_SYNCID_H

#include <mayaUsd/base/id.h>

#include <string>

namespace MAYAUSD_NS_DEF {

/*! \class SyncId
    Identifier that is used to synchronize with an Id. It's used to keep track of when local
    changes are in sync with remote changes so that you don't have to use callbacks.
*/
class SyncId
{
    // Really important that the sync invalid value is different from the
    // Id invalid value since being in sync with an invalid version
    // is in fact a valid state.
    enum
    {
        kInvalidId = Id::kInvalidId - 1
    };

public:
    inline SyncId();
    inline bool        inSync(const Id& masterId) const;
    inline bool        inSync(const SyncId& syncId) const;
    inline void        sync(const Id& masterId);
    inline void        sync(const SyncId& syncId);
    inline bool        valid() const;
    inline void        invalidate();
    inline Id          toId() const;
    inline             operator Id() const;
    inline std::string toString() const;

private:
    short _syncId { kInvalidId }; //!< ID after the last sync, kInvalidID if no sync yet
};

//----------------------------------------------------------------------
/*! \brief Constructor, initialize to the invalid state
 */
inline SyncId::SyncId()
    : _syncId(SyncId::kInvalidId)
{
    // Safety check to ensure that Id and SyncId have the same resolution. Since
    // they rely on wraparound having different types (e.g. short and int) would
    // cause synchronization problems.
    static_assert(sizeof(Id) == sizeof(SyncId), "sizeof(Id) == sizeof(SyncId)");
}

//----------------------------------------------------------------------
/*! \brief Check to see if the ID is in sync with the given master ID
    \param[in] masterId Master ID against which to check synchronization
    \return True if the IDs are in sync and don't need updating.
*/
inline bool SyncId::inSync(const Id& masterId) const
{
    if (_syncId == SyncId::kInvalidId)
        return false;
    return (_syncId == masterId._id);
}

//----------------------------------------------------------------------
/*! \brief Check to see if the ID is in sync with another sync ID
        Unlike comparison against a master ID the sync IDs are considered
        to be equal if both of them contain the invalid ID value.
    \param[in] syncId Sync ID against which to check synchronization
    \return True if the IDs are in sync and don't need updating.
*/
inline bool SyncId::inSync(const SyncId& syncId) const { return (_syncId == syncId._syncId); }

//----------------------------------------------------------------------
/*! \brief Mark this ID as being in sync with the given master ID
    \note Care must be taken to avoid synchronizing with different master
        ID objects. You may need to synchronize with a master ID
        object that is destroyed and recreated so it was easier to pass it
        in and take that risk rather than to try and keep track of the
        master ID's existence.

    \param[in] masterId Master ID with which to synchronize
*/
inline void SyncId::sync(const Id& masterId) { _syncId = masterId._id; }

//----------------------------------------------------------------------
/*! \brief Mark this ID as being in sync with the given sync ID
    What this really means is that this sync ID is synchronized to the
    same master as the given one. This is just a convenience method.

    \param[in] syncId Sync ID with which to synchronize
*/
inline void SyncId::sync(const SyncId& syncId) { _syncId = syncId._syncId; }

//----------------------------------------------------------------------
/*! \brief Check to see if this ID is valid
    \return True if this ID has been synced to a master ID
*/
inline bool SyncId::valid() const { return _syncId != SyncId::kInvalidId; }

//----------------------------------------------------------------------
/*! \brief Mark this ID as invalid
 */
inline void SyncId::invalidate() { _syncId = SyncId::kInvalidId; }

//----------------------------------------------------------------------
/*! \brief Cast operator. Promotes a SyncId to a Id
    \return Id equivalent to the current SyncId or invalid Id if SyncId
        is invalid.
*/
inline Id SyncId::toId() const
{
    Id tempId;
    if (_syncId != SyncId::kInvalidId)
        tempId._id = _syncId;

    return tempId;
}

//----------------------------------------------------------------------
/*! \brief Cast operator. Promotes a SyncId to a Id
    \return Id equivalent to the current SyncId. It is an assertable
                error to call this method on an invalid SyncId
*/
inline SyncId::operator Id() const
{
    Id tempId;
    tempId._id = _syncId;
    return tempId;
}

//----------------------------------------------------------------------
/*! \brief Cast to string for debugging purpose
 */
inline std::string SyncId::toString() const
{
    std::string idStr;
    if (_syncId != SyncId::kInvalidId) {
        idStr = std::to_string(_syncId);
    } else {
        idStr = "INV";
    }

    return idStr;
}

} // namespace MAYAUSD_NS_DEF

#endif
