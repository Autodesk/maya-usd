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

#include "AL/maya/utils/Utils.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>

#include <map>

namespace AL {
namespace maya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class that acts as a lookup table for dependency nodes. It works by storing a sorted
/// map based on the
///         uuid of each node.
/// \ingroup usdmaya
//----------------------------------------------------------------------------------------------------------------------
struct MObjectMap
{
    /// \brief  insert a node into the map.
    /// \param  fn the function set attached to the node to insert
    /// \return true if the node had already been added, false if the node was added.
    inline bool insert(const MFnDependencyNode& fn)
    {
#if AL_UTILS_ENABLE_SIMD
        union
        {
            __m128i sse;
            guid    uuid;
        };
        fn.uuid().get(uuid.uuid);
        bool contains = m_nodeMap.find(sse) != m_nodeMap.end();
        if (!contains)
            m_nodeMap.insert(std::make_pair(sse, fn.object()));
#else
        guid uuid;
        fn.uuid().get(uuid.uuid);
        bool contains = m_nodeMap.find(uuid) != m_nodeMap.end();
        if (!contains)
            m_nodeMap.insert(std::make_pair(uuid, fn.object()));
#endif
        return contains;
    };

    /// \brief  returns true if the dependency node is in the map
    /// \param  fn the function set attached to the node to find in the map
    /// \return true if the node exists in the map
    inline bool contains(const MFnDependencyNode& fn)
    {
#if AL_UTILS_ENABLE_SIMD
        union
        {
            __m128i sse;
            guid    uuid;
        };
        fn.uuid().get(uuid.uuid);
        bool contains = m_nodeMap.find(sse) != m_nodeMap.end();
#else
        guid uuid;
        fn.uuid().get(uuid.uuid);
        bool contains = m_nodeMap.find(uuid) != m_nodeMap.end();
#endif
        return contains;
    };

private:
#if AL_UTILS_ENABLE_SIMD
    std::map<i128, MObject, guid_compare> m_nodeMap;
#else
    std::map<guid, MObject, guid_compare> m_nodeMap;
#endif
};
//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
