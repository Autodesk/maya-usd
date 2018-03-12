//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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

#include "maya/MObject.h"
#include "maya/MString.h"
#include "maya/MUuid.h"
#include "maya/MFnDependencyNode.h"

#include <map>
#include <string>

#include "AL/maya/utils/ForwardDeclares.h"
#include "AL/maya/utils/ForwardDeclares.h"

namespace AL {
namespace maya {
namespace utils {


//----------------------------------------------------------------------------------------------------------------------
// code to speed up comparisons of MObject guids
//----------------------------------------------------------------------------------------------------------------------

/// \brief  A type to store a UUID from a maya node
/// \ingroup usdmaya
struct guid
{
  uint8_t uuid[16]; ///< the UUID for a Maya node
};

#if AL_UTILS_ENABLE_SIMD

/// \brief  Less than comparison utility for sorting via 128bit guid.
/// \ingroup usdmaya
struct guid_compare
{
  inline bool operator () (const i128 a, const i128 b) const
  {
    const uint32_t lt_mask = movemask16i8(cmplt16i8(a, b));
    const uint32_t eq_mask = 0xFFFF & (~movemask16i8(cmpeq16i8(a, b)));
    if(!eq_mask) return false;

    // find first bit value that is not equal
    const uint32_t index = __builtin_ctz(eq_mask);
    // now see whether that bit has been set
    return (lt_mask & (1 << index)) != 0;
  }
};

#else

/// \brief  Less than comparison utility for sorting via 128bit guid.
/// \ingroup usdmaya
struct guid_compare
{
  /// \brief  performs a less than comparison between two UUIDs. Used to sort the entries in an MObjectMap
  /// \param  a first UUID to compare
  /// \param  b second UUID to compare
  /// \return true if a < b
  inline bool operator () (const guid& a, const guid& b) const
  {
    for(int i = 0; i < 16; ++i)
    {
      if(a.uuid[i] < b.uuid[i]) return true;
      if(a.uuid[i] > b.uuid[i]) return false;
    }
    return false;
  }
};
#endif

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class that acts as a lookup table for dependency nodes. It works by storing a sorted map based on the
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
      guid uuid;
    };
    fn.uuid().get(uuid.uuid);
    bool contains = m_nodeMap.find(sse) != m_nodeMap.end();
    if(!contains)
      m_nodeMap.insert(std::make_pair(sse, fn.object()));
    #else
    guid uuid;
    fn.uuid().get(uuid.uuid);
    bool contains = m_nodeMap.find(uuid) != m_nodeMap.end();
    if(!contains)
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
      guid uuid;
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
  std::map<i128, MObject, maya::guid_compare> m_nodeMap;
  #else
  std::map<guid, MObject, guid_compare> m_nodeMap;
  #endif
};
//----------------------------------------------------------------------------------------------------------------------
} // utils
} // maya
} // AL
//----------------------------------------------------------------------------------------------------------------------
