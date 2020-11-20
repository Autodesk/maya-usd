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

#ifndef MAYAUSD_ID_H
#define MAYAUSD_ID_H

#include <climits>
#include <string>

namespace MAYAUSD_NS_DEF {

/*! \class Id

        Trivial class to manage ids

        \sa SyncId

        IDs have meaning in relation to each other but not really on their own.
        They are used to see if state information is up to date in relation to
        the current version of that state.

        For example you can have a main database with a version in it which
        gets updated every time any change to the database is made. Then you
        can store cached information from the database with its own ID and
        compare that against the ID of the database to decide if the cache
        needs to be updated.

        There are two flavours of ID:

        Main (Id)
                This is attached to the object being versioned. Every time a
                significant(*) change happens to that object the ID number
                is bumped. If a complete rebuild is needed the ID number
                is invalidated and restarts.

                (*) The definition of "significant" is left up to the object
                        being versioned. For example if it's a graph then any
                        changes to the graph topology might be significant but a
                        renaming operation might not.

        Syncronized (SyncId)
                This is attached to the object trying to keep in sync with
                an object having a Id. The comparison operators let you check
                to see if the versions are in sync or not. This would be a good
                mechanism to keep a cache up to date without using notification.

                void TmyCache::synchronize()
                {
                        if( myId.inSync( masterId ) )
                        {
                                if( rebuildMyCache() )
                                {
                                        myId.sync( masterId );
                                }
                        }
                }
*/
class Id
{
    friend class SyncId; // For access to the invalid ID
    enum
    {
        kInvalidId = -1
    };

public:
    // ID modification and verification
    inline void next();
    inline void invalidate();
    inline bool valid() const;

    // Equality and inequality operators
    inline bool operator==(const Id& rhs) const;
    inline bool operator!=(const Id& rhs) const;

    inline std::string toString() const;

private:
    short _id { kInvalidId }; //!< The actual ID number
};

//----------------------------------------------------------------------
/*! \brief Equality operator; compare against another ID.
    \param[in] rhs ID to compare against
    \return True if this ID is the same as the given one
*/
inline bool Id::operator==(const Id& rhs) const { return (_id == rhs._id); }

//----------------------------------------------------------------------
/*! \brief Inequality operator; compare against another ID.
    \param[in] rhs ID to compare against
    \return False if this ID is the same as the given one
*/
inline bool Id::operator!=(const Id& rhs) const { return (_id != rhs._id); }

//----------------------------------------------------------------------
/*! \brief Call this when the object being IDd has changed
 */
inline void Id::next() { _id = (_id == SHRT_MAX) ? 0 : _id + 1; }

//----------------------------------------------------------------------
/*! \brief  Mark this ID as invalid
            This would called when the object being IDd needs rebuilding
*/
inline void Id::invalidate() { _id = kInvalidId; }

//----------------------------------------------------------------------
/*! \brief Check to see if the ID is currently valid
    \return True if the ID is a legal one. Since IDs only have meaning
        relative to each other this just checks that the ID has
        been set at least once.
*/
inline bool Id::valid() const { return _id != Id::kInvalidId; }

//----------------------------------------------------------------------
/*! \brief Cast to string for debugging purpose
 */
inline std::string Id::toString() const
{
    std::string idStr;
    if (_id != Id::kInvalidId) {
        idStr = std::to_string(_id);
    } else {
        idStr = "INV";
    }

    return idStr;
}

} // namespace MAYAUSD_NS_DEF

#endif
