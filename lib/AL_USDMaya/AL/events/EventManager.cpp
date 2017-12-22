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
// distributed under the License is distributed oncommand an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "EventManager.h"

#include <string>
#include <utility>
#include <vector>
#include <algorithm>
#include <memory>
#include <maya/MGlobal.h>
#include <pxr/pxr.h>
#include "AL/events/Events.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace events {

//----------------------------------------------------------------------------------------------------------------------
MayaEventManager::MayaEventManager() : m_mayaCallbacks({MCallbackId()})
{

}

//----------------------------------------------------------------------------------------------------------------------
static void onMayaCommand(void* userData)
{
  const Listeners& listeners = *(static_cast<const Listeners*>(userData));

  for(const auto& l : listeners)
  {
    if(l.callback)
    {
      l.callback(l.userData);
    }
    else
    {
      if(l.isPython)
      {
        MGlobal::executePythonCommand(l.command);
      }
      else
      {
        MGlobal::executeCommand(l.command);
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
EventID MayaEventManager::registerLast(MayaEventType event,
    const Callback& callback,
    void* userData,
    bool isPython,
    const char* command,
    const char* tag)
{
  return registerCallback(event, callback, userData, isPython, command, AL::usdmaya::events::kPlaceLast, tag);
}

//----------------------------------------------------------------------------------------------------------------------
EventID MayaEventManager::registerFirst(MayaEventType event,
    const Callback& callback,
    void* userData,
    bool isPython,
    const char* command,
    const char* tag)
{
  return registerCallback(event, callback, userData, isPython, command, AL::usdmaya::events::kPlaceFirst, tag);
}

//----------------------------------------------------------------------------------------------------------------------
EventID MayaEventManager::registerCallback(MayaEventType event,
     const Callback& callback,
     void* userData,
     bool isPython,
     const char* command,
     uint32_t weight,
     const char* tag)
{
  Listener eventListener;
  eventListener.userData = userData;
  eventListener.callback = callback;
  eventListener.command = command;
  eventListener.tag = tag;
  eventListener.weight = weight;
  eventListener.isPython = isPython;

  return registerCallback(event, eventListener);
}

//----------------------------------------------------------------------------------------------------------------------
EventID MayaEventManager::registerCallback(MayaEventType eventType, const Listener& eventListener)
{
  if(eventType >= MayaEventType::kSceneMessageLast)
  {
    return EventID();
  }

  Listeners& listeners = m_mayaListeners[eventType];
  Listener newListener = eventListener;

  // Generate and ID that would be currently unique
  newListener.id = generateEventId(eventType);

  if(newListener.weight & AL::usdmaya::events::kPlaceLast)
  {
    if(!listeners.empty())
    {
      // If the weight is tagged to place last, make it +1 more than the element at the end of the vector
      newListener.weight = listeners.back().weight + 1;
    }
    listeners.push_back(newListener);
  }
  else if(newListener.weight & AL::usdmaya::events::kPlaceFirst)
  {
    // If the weight is tagged as the first place, then push it directly to the front and give it a light weight
    newListener.weight = 0;
    listeners.insert(listeners.begin(), newListener);
  }
  else
  {
    listeners.push_back(newListener);
  }

  // Start the Maya callback if the recently Maya listener has been added
  if(listeners.size() == 1)
  {
    registerMayaCallback(eventType);
  }

  std::sort(listeners.begin(), listeners.end(), [](const ListenerEntry& a, const ListenerEntry& b) { return a.weight < b.weight; });

  return newListener.id;
}

//----------------------------------------------------------------------------------------------------------------------
bool MayaEventManager::registerMayaCallback(MayaEventType eventType)
{
  MSceneMessage::Message mayaEvent = AL::usdmaya::events::eventToMayaEvent(eventType);

  if(mayaEvent == MSceneMessage::kLast)
  {
    return false;
  }

  MStatus status;
  MCallbackId mayaEventID = MSceneMessage::addCallback(mayaEvent, onMayaCommand, &m_mayaListeners[eventType], &status);
  MStatus::MStatusCode statusCode = status.statusCode();

  if(statusCode != MStatus::kSuccess)
  {
    switch(statusCode)
    {
    case MStatus::kFailure:
      std::cout << "registerMayaCallback: Error adding callback" << std::endl;
      return false;
    case MStatus::kInsufficientMemory:
      std::cout << "registerMayaCallback: No memory available" << std::endl;
      return false;
    }
  }

  m_mayaCallbacks[eventType] = mayaEventID;
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool MayaEventManager::deregisterMayaCallback(AL::usdmaya::events::MayaEventType event)
{
  if(event >= AL::usdmaya::events::kSceneMessageLast)
  {
    return false;
  }

  MCallbackId callbackID = m_mayaCallbacks[event];
  MStatus status = MSceneMessage::removeCallback(callbackID);
  MStatus::MStatusCode statusCode = status.statusCode();

  if(statusCode != MStatus::kSuccess)
  {
    switch(statusCode)
    {
      case MStatus::kFailure:
        std::cout << "deregisterMayaCallback: Callback has already been removed" << std::endl;
        return false;
      case MStatus::kInvalidParameter:
        std::cout << "deregisterMayaCallback: An invalid callback id was specified" << std::endl;
        return false;
    }
  }

  // Deregister and reset the Maya callback ID
  m_mayaCallbacks[event] = MCallbackId();
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool MayaEventManager::deregister(EventID id)
{
  MayaEventType eventType = getEventTypeFromID(id);
  if(eventType >= AL::usdmaya::events::kSceneMessageLast)
  {
    return false;
  }


  auto eventComparison = [id](const ListenerEntry& listener){
    return listener.id == id;
  };

  auto& mayaListeners = m_mayaListeners[eventType];
  auto foundListener = std::find_if(mayaListeners.begin(), mayaListeners.end(), eventComparison);

  if(foundListener == mayaListeners.end())
  {
    return false;
  }

  // There will be no listeners soon, deregister for the Maya Event
  if(mayaListeners.size() == 1)
  {
    deregisterMayaCallback(eventType);
  }
  mayaListeners.erase(foundListener);
  return true;
}

EventID MayaEventManager::generateEventId(MayaEventType eventType)
{
  // Find the the listener with the largest count
  uint64_t count = 1;
  if(m_mayaListeners[eventType].size() != 0)
  {
    for(const Listener& listener : m_mayaListeners[eventType])
    {
      uint64_t listenersCount = getCountFromID(listener.id);
      if(listenersCount > count)
      {
        count = listenersCount;
      }
    }
  }

  // push the eventtype to the front of the bits, and the count to the back
  return EventID((EventID)eventType << (ID_TOTAL_BITS-ID_MAYAEVENTTYPE_BITS) | (count+1));
}

//----------------------------------------------------------------------------------------------------------------------
} // events
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
