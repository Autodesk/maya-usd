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
using namespace AL::usdmaya::events;

MayaEventManager::MayaEventManager() : m_mayaCallbacks({0})
{

}

static void onMayaCommand(void* userData)
{
  const Listeners& listeners = *(static_cast<const Listeners*>(userData));

  for(const auto& l : listeners)
  {
    if(l->callback)
    {
      l->callback(&l->userData);
    }
    else
    {
      if(l->isPython)
      {
        MGlobal::executePythonCommand(l->command);
      }
      else
      {
        MGlobal::executeCommand(l->command);
      }
    }
  }
}

EventID MayaEventManager::registerCallback(MayaEventType eventType,
                                           const Callback& callback,
                                           uint32_t weight,
                                           void* userData,
                                           const char* tag,
                                           bool isPython,
                                           const char* command
                                           )
{
  Listener eventListener;
  eventListener.userData = userData;
  eventListener.callback = callback;
  eventListener.command = command;
  eventListener.tag = tag;
  eventListener.weight = weight;
  eventListener.isPython = isPython;

  return registerCallback(eventType, eventListener);
}

EventID MayaEventManager::registerCallback(MayaEventType eventType, const Listener& eventListener)
{
  Listeners& listeners = m_mayaListeners[eventType];
  ListenerPtr newEvent = make_unique<Listener>(eventListener);

  // Retrive the pointer location as the ID
  EventID id = (EventID)newEvent.get();

  listeners.push_back(std::move(newEvent));
  // Start the Maya callback if the recently Maya listener has been added
  if(m_mayaListeners[eventType].size() == 1)
  {
    m_mayaCallbacks[eventType] = registerMayaCallback(eventType);
  }

  std::sort(listeners.begin(), listeners.end(), [](const ListenerPtr& a, const ListenerPtr& b) { return a->weight < b->weight; });

  return id;
}

MCallbackId MayaEventManager::registerMayaCallback(MayaEventType eventType)
{
  MSceneMessage::Message mayaEvent = AL::usdmaya::events::EventToMayaEvent(eventType);

  if(mayaEvent == MSceneMessage::kLast)
  {
    return false;
  }

  MStatus status;
  return MSceneMessage::addCallback(mayaEvent, onMayaCommand, &m_mayaListeners[eventType], &status);
}

bool MayaEventManager::deregister(AL::usdmaya::events::MayaEventType event, EventID id)
{
  auto eventComparison = [id](const ListenerPtr& listener){
    Listener* event = listener.get();
    EventID listenerID = (EventID)event;
    return (listenerID == id);
  };
  auto foundListener = std::find_if(m_mayaListeners[event].begin(), m_mayaListeners[event].end(), eventComparison);

  if(foundListener == m_mayaListeners[event].end())
  {
    return false;
  }

  m_mayaListeners[event].erase(foundListener);

  if(m_mayaListeners[event].size() == 0)
  {
    MCallbackId callbackID = m_mayaCallbacks[event];
    MStatus status = MSceneMessage::removeCallback(callbackID);
    MStatus::MStatusCode statusCode = status.statusCode();

    if(statusCode != MStatus::kSuccess)
    {
      switch(statusCode)
      {
        case MStatus::kFailure:
          std::cout << "Callback has already been removed" << std::endl;
          return false;
        case MStatus::kInvalidParameter:
          std::cout << "An invalid callback id was specfied" << std::endl;
          return false;
      }
    }
  }
  return true;
}

bool MayaEventManager::deregister(EventID id)
{
  for(size_t i = 0; i < m_mayaListeners.size(); ++i)
  {
    auto endItr = m_mayaListeners[i].end();
    auto foundListener = endItr;
    for(auto it=m_mayaListeners[i].begin(); it < endItr; ++it)
    {
      EventID listenerID = (EventID)&(*it);

      if(listenerID == id)
      {
        foundListener = it;
      }
    }

    if(foundListener != m_mayaListeners[i].end())
    {
      m_mayaListeners[i].erase(foundListener);
      return true;
    }
  }
  //for(auto& listener : m_mayaListeners)
  return false;

}
