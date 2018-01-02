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
#include <functional>
#include <map>
#include "maya/MGlobal.h"


namespace AL {
namespace usdmaya {
namespace events {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Event types which are supported by the Event Manager.
//----------------------------------------------------------------------------------------------------------------------
enum class MayaEventType
{
  kBeforeNew, ///< called prior to file new
  kAfterNew, ///< called after to file new
  kBeforeOpen, ///< called prior to file open
  kAfterOpen, ///< called after to file open
  kBeforeSave, ///< called prior to file save
  kAfterSave, ///< called after to file save
  kBeforeReference, ///< called prior to file reference
  kAfterReference, ///< called after to file reference
  kBeforeUnloadReference, ///< called prior to file reference unloaded
  kAfterUnloadReference, ///< called after to file reference unloaded
  kBeforeLoadReference, ///< called prior to file reference loaded
  kAfterLoadReference, ///< called after to file reference loaded
  kBeforeCreateReference, ///< called prior to file reference created
  kAfterCreateReference, ///< called after to file reference created
  kMayaInitialized, ///< called after maya has been initialised
  kMayaExiting, ///< called prior to maya exiting
  kSceneMessageLast
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief Converts supported event type into Maya's event type
/// \return The corresponding Maya event type, else MSceneMessage::kLast
//----------------------------------------------------------------------------------------------------------------------
inline MSceneMessage::Message eventToMayaEvent(MayaEventType internalEvent)
{
  static std::array<MSceneMessage::Message, size_t(MayaEventType::kSceneMessageLast)> eventToMayaEventTable =
    {
      MSceneMessage::kBeforeNew,
      MSceneMessage::kAfterNew,
      MSceneMessage::kBeforeOpen,
      MSceneMessage::kAfterOpen,
      MSceneMessage::kBeforeSave,
      MSceneMessage::kAfterSave,
      MSceneMessage::kBeforeReference,
      MSceneMessage::kAfterReference,
      MSceneMessage::kBeforeUnloadReference,
      MSceneMessage::kAfterUnloadReference,
      MSceneMessage::kBeforeLoadReference,
      MSceneMessage::kAfterLoadReference,
      MSceneMessage::kBeforeCreateReference,
      MSceneMessage::kAfterCreateReference,
      MSceneMessage::kMayaInitialized,
      MSceneMessage::kMayaExiting
    };

  if(size_t(internalEvent) < eventToMayaEventTable.size())
  {
    return eventToMayaEventTable[size_t(internalEvent)];
  }

  return MSceneMessage::kLast;
}

typedef std::pair<MSceneMessage::Message, MayaEventType> Entry;
inline bool operator<(const Entry &ar, const MSceneMessage::Message br)
{
  return ar.first < br;
}


//----------------------------------------------------------------------------------------------------------------------
/// \brief Converts Maya's event type into supported event type
/// \return The corresponding supported Event type, else MayaEventType::kSceneMessageLast
//----------------------------------------------------------------------------------------------------------------------
inline MayaEventType eventToMayaEvent(MSceneMessage::Message mayaEvent)
{
  // Insert new Maya events into the correct order.
  static std::array<Entry, 16> eventToMayaEventTable =
    {
      Entry(MSceneMessage::kBeforeNew, MayaEventType::kBeforeNew),
      Entry(MSceneMessage::kAfterNew, MayaEventType::kAfterNew),
      Entry(MSceneMessage::kBeforeOpen, MayaEventType::kBeforeOpen),
      Entry(MSceneMessage::kAfterOpen, MayaEventType::kAfterOpen),
      Entry(MSceneMessage::kBeforeSave, MayaEventType::kBeforeSave),
      Entry(MSceneMessage::kAfterSave, MayaEventType::kAfterSave),
      Entry(MSceneMessage::kBeforeReference, MayaEventType::kBeforeReference),
      Entry(MSceneMessage::kAfterReference, MayaEventType::kAfterReference),
      Entry(MSceneMessage::kBeforeUnloadReference, MayaEventType::kBeforeUnloadReference),
      Entry(MSceneMessage::kAfterUnloadReference, MayaEventType::kAfterUnloadReference),
      Entry(MSceneMessage::kMayaInitialized, MayaEventType::kMayaInitialized),
      Entry(MSceneMessage::kMayaExiting, MayaEventType::kMayaExiting),
      Entry(MSceneMessage::kBeforeLoadReference, MayaEventType::kBeforeLoadReference),
      Entry(MSceneMessage::kAfterLoadReference, MayaEventType::kAfterLoadReference),
      Entry(MSceneMessage::kBeforeCreateReference, MayaEventType::kBeforeCreateReference),
      Entry(MSceneMessage::kAfterCreateReference, MayaEventType::kAfterCreateReference)
    };

  auto entry = std::lower_bound(eventToMayaEventTable.begin(),
                                eventToMayaEventTable.end(),
                                mayaEvent);

  if(entry != eventToMayaEventTable.end())
  {
    return entry->second;
  }

  return MayaEventType::kSceneMessageLast;
}

//----------------------------------------------------------------------------------------------------------------------
} // events
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
