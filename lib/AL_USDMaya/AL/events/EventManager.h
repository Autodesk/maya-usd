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

#include <pxr/pxr.h>
#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <array>
#include "maya/MMessage.h"
#include "maya/MSceneMessage.h"

#include "AL/events/Events.h"


PXR_NAMESPACE_USING_DIRECTIVE
namespace AL {
namespace usdmaya {
namespace events {
class Listener;

typedef void* UserData;
typedef MMessage::MBasicFunction Callback;
typedef uint64_t EventID;
typedef Listener ListenerEntry;
typedef std::vector<ListenerEntry> Listeners;
typedef std::array<Listeners, MayaEventType::kSceneMessageLast> ListenerContainer;
typedef std::array<MCallbackId, MayaEventType::kSceneMessageLast> MayaCallbackIDContainer;

/*
 * \brief Objects of this class contain all the data needed to allow callbacks to happen
 */
struct Listener
{
  UserData userData = nullptr;  ///< data which is returned back to the user which registered this even and data
  Callback callback = nullptr;  ///< called when the event is triggered

  MString command;              ///< Python or Mel command to call on callback
  MString tag;                  ///< tag or category of the event purpose

  EventID id;                   ///< the id generated for the event

  struct
  {
    uint32_t weight : 31;       ///< order weight of this event
    uint32_t isPython : 1;      ///< if true (and the C++ function pointer is NULL), the command string will be treated as python, otherwise MEL
  };

  inline bool operator < (const Listener& event) const
    { return weight < event.weight; }

  inline bool operator==(const EventID& rhs) const
    { return id == rhs; }
  inline bool operator!=(const EventID& rhs) const
    { return id != rhs; }
};

// For comparison between and EventID and a listener
inline bool operator==(const EventID& lhs2, const Listener& rhs)
  { return lhs2 == rhs.id; }
inline bool operator!=(const EventID& lhs2, const Listener& rhs)
  { return lhs2 != rhs.id; }

#define ID_TOTAL_BITS 64
#define ID_MAYAEVENTTYPE_BITS 16
#define ID_COUNT 48

//----------------------------------------------------------------------------------------------------------------------
/// \brief Stores and orders the registered Event objects and executes these Events when the wanted Maya callbacks are triggered.
//----------------------------------------------------------------------------------------------------------------------
class MayaEventManager
{

public:
  explicit MayaEventManager();

  /// \brief Creates an event which listens to the specified Maya event,
  ///   obeying the passed in order weight.
  ///   Internally this method creates a Listener type
  ///   and passes this object onto the register function.
  /// \param event corresponding internal maya event
  /// \param callback function which will be called
  /// \param userData data which is returned to the user when the callback is triggered
  /// \param isPython true if the specified command should be executed as python
  /// \param command the string that will be executed when the callback happens
  /// \param weight the priority order for when this event is run, the lower the number the higher the priority
  /// \param tag string to help classify the type of listener
  /// \return the identifier of the created listener, or 0 if nothing was registered
  EventID registerCallback(MayaEventType event,
      const Callback& callback,
      void* userData = nullptr,
      bool isPython = false,
      const char* command = "",
      uint32_t weight = AL::usdmaya::events::kPlaceLast,
      const char* tag = "");

  /// \brief Creates an event which listens to the specified Maya event,
  ///   adding the event to current last position.
  ///   Internally this method creates a Listener type
  ///   and passes this object onto the register function.
  /// \param event corresponding internal maya event
  /// \param callback function which will be called
  /// \param userData data which is returned to the user when the callback is triggered
  /// \param tag string to help classify the type of listener
  /// \param isPython true if the specified command should be executed as python
  /// \param command the string that will be executed when the callback happens
  /// \return the identifier of the created listener, or 0 if nothing was registered
  EventID registerLast(MayaEventType event,
      const Callback& callback,
      void* userData = nullptr,
      bool isPython = false,
      const char* command = "",
      const char* tag = "");

  /// \brief Creates an event which listens to the specified Maya event,
  ///   adding the event to current last position.
  ///   Internally this method creates a Listener type
  ///   and passes this object onto the register function.
  /// \param event corresponding internal maya event
  /// \param callback function which will be called
  /// \param userData data which is returned to the user when the callback is triggered
  /// \param tag string to help classify the type of listener
  /// \param isPython true if the specified command should be executed as python
  /// \param command the string that will be executed when the callback happens
  /// \return the identifier of the created listener, or 0 if nothing was registered
  EventID registerFirst(MayaEventType event,
      const Callback& callback,
      void* userData = nullptr,
      bool isPython = false,
      const char* command = "",
      const char* tag = "");

  /// \brief Stores and orders the registered Maya callbacks
  /// \param eventType corresponding internal Maya event
  /// \param eventListener object which is executed when the corresponding Maya event triggers
  /// \return the identifier of the created listener, or 0 if nothing was registered
  EventID registerCallback(MayaEventType eventType, const Listener& eventListener);

//  /// \brief Remove the corresponding EventID
//  /// \param event internal type of the event
//  /// \param id unique identifier of the Listener that was returned when it was registered
//  /// \return true if an event was deregistered
//  bool deregister(AL::usdmaya::events::MayaEventType event, EventID id);

  /// \brief Remove the corresponding EventID. More costly than the deregister
  ///        method where the MayaEventType is passed, since it has to search for the corresponding ID
  ///        in all the available events.
  /// \param id internal type of the event
  /// \return true if an event was deregistered
  bool deregister(EventID id);

  /// \brief retrieves the container containing all the Maya listeners
  /// \return ListenerContainer containing all the listeners
  const ListenerContainer& listeners(){return m_mayaListeners;}

  bool isMayaCallbackRegistered(AL::usdmaya::events::MayaEventType event)
  {
    if(event >= AL::usdmaya::events::kSceneMessageLast)
    {
      return false;
    }
    return m_mayaCallbacks[event] != MCallbackId();
  }

  /// \brief retrieves the container containing all the Maya listeners
  /// \return ListenerContainer containing all the listeners
  const MayaCallbackIDContainer& mayaCallbackIDs(){return m_mayaCallbacks;}


private:
  bool registerMayaCallback(MayaEventType eventType);
  bool deregisterMayaCallback(MayaEventType eventType);
  EventID generateEventId(MayaEventType eventType);

  inline MayaEventType getEventTypeFromID(EventID eventId) const
  {
    return (MayaEventType)(eventId >> (ID_TOTAL_BITS-ID_MAYAEVENTTYPE_BITS));
  }

  inline uint64_t getCountFromID(EventID eventId) const
  {
    // Mask out the top 16 bits since they are
    return (eventId & 0x3ffffffffffff);
  }

private:
  ListenerContainer m_mayaListeners;
  MayaCallbackIDContainer m_mayaCallbacks;

};

//----------------------------------------------------------------------------------------------------------------------
} // events
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

