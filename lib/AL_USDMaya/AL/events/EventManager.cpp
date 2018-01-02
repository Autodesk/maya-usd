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

static MayaEventManager g_mayaEventManager;

//----------------------------------------------------------------------------------------------------------------------
MayaEventManager& MayaEventManager::instance()
{
  return g_mayaEventManager;
}

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
EventID MayaEventManager::registerCallback(
     MayaEventType eventType,
     const Callback callback,
     const char* tag,
     uint32_t weight,
     void* userData,
     bool isPython,
     const char* command)
{

  if(eventType >= MayaEventType::kSceneMessageLast)
  {
    return EventID();
  }

  // Generate and ID that would be currently unique
  Listener eventListener;
  eventListener.userData = userData;
  eventListener.callback = callback;
  eventListener.command = command;
  eventListener.tag = tag;
  eventListener.id = generateEventId(eventType);
  eventListener.weight = weight;
  eventListener.isPython = isPython;

  Listeners& listeners = m_mayaListeners[size_t(eventType)];

  // Start the Maya callback if the recently Maya listener has been added
  if(listeners.empty())
  {
    registerMayaCallback(eventType);
  }

  auto it = std::lower_bound(
    listeners.begin(),
    listeners.end(),
    weight
  );

  listeners.insert(it, eventListener);

  return eventListener.id;
}

//----------------------------------------------------------------------------------------------------------------------
bool MayaEventManager::registerMayaCallback(const MayaEventType eventType)
{
  MSceneMessage::Message mayaEvent = eventToMayaEvent(eventType);

  if(mayaEvent == MSceneMessage::kLast)
  {
    return false;
  }

  MStatus status;
  MCallbackId mayaEventID = MSceneMessage::addCallback(mayaEvent, onMayaCommand, &m_mayaListeners[size_t(eventType)], &status);
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

  m_mayaCallbacks[size_t(eventType)] = mayaEventID;
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool MayaEventManager::unregisterMayaCallback(const MayaEventType event)
{
  if(event >= MayaEventType::kSceneMessageLast)
  {
    return false;
  }

  MCallbackId callbackID = m_mayaCallbacks[size_t(event)];
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
  m_mayaCallbacks[size_t(event)] = MCallbackId();
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool MayaEventManager::unregisterCallback(const EventID id)
{
  MayaEventType eventType = getEventTypeFromID(id);
  if(eventType >= MayaEventType::kSceneMessageLast)
  {
    return false;
  }

  auto eventComparison = [id](const ListenerEntry& listener){
    return listener.id == id;
  };

  auto& mayaListeners = m_mayaListeners[size_t(eventType)];
  auto foundListener = std::find_if(mayaListeners.begin(), mayaListeners.end(), eventComparison);

  if(foundListener == mayaListeners.end())
  {
    return false;
  }

  // There will be no listeners soon, deregister for the Maya Event
  if(mayaListeners.size() == 1)
  {
    unregisterMayaCallback(eventType);
  }
  mayaListeners.erase(foundListener);
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
EventID MayaEventManager::makeEventId(const MayaEventType eventType, uint64_t idPart)
{
  return EventID((EventID)eventType << (ID_TOTAL_BITS-ID_MAYAEVENTTYPE_BITS) | idPart);
}

//----------------------------------------------------------------------------------------------------------------------
EventID MayaEventManager::generateEventId(const MayaEventType eventType)
{
  // Find the the listener with the largest count
  uint64_t count = 1;
  if(!m_mayaListeners[size_t(eventType)].empty())
  {
    for(const Listener& listener : m_mayaListeners[size_t(eventType)])
    {
      uint64_t listenersCount = getCountFromID(listener.id);
      if(listenersCount > count)
      {
        count = listenersCount;
      }
    }
  }

  // push the eventtype to the front of the bits, and the count to the back
  return makeEventId(eventType, count+1);
}

//----------------------------------------------------------------------------------------------------------------------
} // events
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
