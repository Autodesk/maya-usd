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
enum MayaEventType
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
MSceneMessage::Message eventToMayaEvent(MayaEventType internalEvent)
{
  static std::array<MSceneMessage::Message, MayaEventType::kSceneMessageLast> eventToMayaEventTable =
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

  if(internalEvent < eventToMayaEventTable.size())
  {
    return eventToMayaEventTable[internalEvent];
  }

  return MSceneMessage::kLast;
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief Converts Maya's event type into supported event type
/// \return The corresponding supported Event type, else MayaEventType::kSceneMessageLast
//----------------------------------------------------------------------------------------------------------------------
MayaEventType eventToMayaEvent(MSceneMessage::Message mayaEvent)
{
  typedef std::pair<MSceneMessage::Message, MayaEventType> MapEntry;
  static std::map<MSceneMessage::Message, MayaEventType> mayaEventToEventTable =
    {
      MapEntry(MSceneMessage::kBeforeNew, kBeforeNew),
      MapEntry(MSceneMessage::kAfterNew, kAfterNew),
      MapEntry(MSceneMessage::kBeforeOpen, kBeforeOpen),
      MapEntry(MSceneMessage::kAfterOpen, kAfterOpen),
      MapEntry(MSceneMessage::kBeforeSave, kBeforeSave),
      MapEntry(MSceneMessage::kAfterSave, kAfterSave),
      MapEntry(MSceneMessage::kBeforeReference, kBeforeReference),
      MapEntry(MSceneMessage::kAfterReference, kAfterReference),
      MapEntry(MSceneMessage::kBeforeUnloadReference, kBeforeUnloadReference),
      MapEntry(MSceneMessage::kAfterUnloadReference, kAfterUnloadReference),
      MapEntry(MSceneMessage::kBeforeLoadReference, kBeforeLoadReference),
      MapEntry(MSceneMessage::kAfterLoadReference, kAfterLoadReference),
      MapEntry(MSceneMessage::kBeforeCreateReference, kBeforeCreateReference),
      MapEntry(MSceneMessage::kAfterCreateReference, kAfterCreateReference),
      MapEntry(MSceneMessage::kMayaInitialized, kMayaInitialized),
      MapEntry(MSceneMessage::kMayaExiting, kMayaExiting)
    };

  if(mayaEventToEventTable.find(mayaEvent) != mayaEventToEventTable.end())
  {
    return mayaEventToEventTable[mayaEvent];
  }

  return kSceneMessageLast;
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Flags to help in the ordering of the Listeners
//----------------------------------------------------------------------------------------------------------------------
enum ListenerOrder : uint32_t
{
  kPlaceLast = 1 << 30,
  kPlaceFirst = 1 << 29,
};

// C++ standard had an "oversight" for make_unique. This is copied from Herb Sutter's blog
// See: https://herbsutter.com/gotw/_102/
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args)
{
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

//----------------------------------------------------------------------------------------------------------------------
} // events
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
