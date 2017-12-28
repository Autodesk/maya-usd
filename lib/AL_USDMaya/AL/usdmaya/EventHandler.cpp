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
#include "AL/usdmaya/EventHandler.h"
#include <cstring>
#include "maya/MGlobal.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {

EventScheduler g_scheduler;
EventScheduler& EventScheduler::getScheduler()
{
  return g_scheduler;
}

//----------------------------------------------------------------------------------------------------------------------
Callback::Callback(const char* const tag, const char* const commandText, uint32_t weight, bool isPython, CallbackId callbackId)
  : m_tag(tag), m_userData(0), m_callbackId(callbackId)
{
  size_t len = std::strlen(commandText) + 1;
  char* ptr = new char[len];
  m_callbackString = ptr;
  std::memcpy(ptr, commandText, len);
  m_weight = weight;
  m_isPython = isPython ? 1 : 0;
  m_isCFunction = 0;
}

//----------------------------------------------------------------------------------------------------------------------
Callback::~Callback()
{
  if(!m_isCFunction)
  {
    delete [] m_callbackString;
  }
}

//----------------------------------------------------------------------------------------------------------------------
Callback EventDispatcher::buildCallbackInternal(
  const char* const tag,
  const void* functionPointer,
  uint32_t weight,
  void* userData)
{
  CallbackId newId = makeCallbackId(eventId(), 0);
  for(auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it)
  {
    if(it->tag() == tag)
    {
      MGlobal::displayError(MString("An attempt to register the same event tag twice occurred - ") + tag);
      return Callback();
    }
    newId = std::max(newId, it->callbackId());
  }
  return Callback(tag, functionPointer, weight, userData, ++newId);
}

//----------------------------------------------------------------------------------------------------------------------
CallbackId EventDispatcher::registerCallbackInternal(
  const char* const tag,
  const void* functionPointer,
  uint32_t weight,
  void* userData)
{
  CallbackId newId = makeCallbackId(eventId(), 0);
  auto insertLocation = m_callbacks.end();
  for(auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it)
  {
    if(insertLocation == e && it->weight() >= weight)
    {
      insertLocation = it;
    }

    if(it->tag() == tag)
    {
      MGlobal::displayError(MString("An attempt to register the same event tag twice occurred - ") + tag);
      return -1;
    }
    newId = std::max(newId, it->callbackId());
  }
  m_callbacks.emplace(insertLocation, tag, functionPointer, weight, userData, ++newId);
  return newId;
}

//----------------------------------------------------------------------------------------------------------------------
CallbackId EventDispatcher::registerCallback(
  const char* const tag,
  const char* const commandText,
  uint32_t weight,
  bool isPython)
{
  CallbackId newId = makeCallbackId(eventId(), 0);
  auto insertLocation = m_callbacks.end();
  for(auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it)
  {
    if(insertLocation == e && it->weight() >= weight)
    {
      insertLocation = it;
    }

    if(it->tag() == tag)
    {
      MGlobal::displayError(MString("An attempt to register the same event tag twice occurred - ") + tag);
      return -1;
    }
    newId = std::max(newId, it->callbackId());
  }

  m_callbacks.emplace(insertLocation, tag, commandText, weight, isPython, ++newId);
  return newId;
}

//----------------------------------------------------------------------------------------------------------------------
Callback EventDispatcher::buildCallback(
  const char* const tag,
  const char* const commandText,
  uint32_t weight,
  bool isPython)
{
  CallbackId newId = makeCallbackId(eventId(), 0);
  for(auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it)
  {
    if(it->tag() == tag)
    {
      MGlobal::displayError(MString("An attempt to register the same event tag twice occurred - ") + tag);
      return Callback();
    }
    newId = std::max(newId, it->callbackId());
  }

  return Callback(tag, commandText, weight, isPython, ++newId);
}

//----------------------------------------------------------------------------------------------------------------------
void EventDispatcher::registerCallback(Callback& info)
{
  auto insertLocation = m_callbacks.end();
  for(auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it)
  {
    if(insertLocation == e && it->weight() >= info.weight())
    {
      insertLocation = it;
    }

    if(it->tag() == info.tag())
    {
      MGlobal::displayError(MString("An attempt to register the same event tag twice occurred - ") + info.tag().c_str());
      return;
    }
  }
  m_callbacks.insert(insertLocation, std::move(info));
}

//----------------------------------------------------------------------------------------------------------------------
bool EventDispatcher::unregisterCallback(CallbackId callbackId)
{
  for(auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it)
  {
    if(it->callbackId() == callbackId)
    {
      m_callbacks.erase(it);
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool EventDispatcher::unregisterCallback(CallbackId callbackId, Callback& info)
{
  for(auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it)
  {
    if(it->callbackId() == callbackId)
    {
      info = std::move(*it);
      m_callbacks.erase(it);
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
EventId EventScheduler::registerEvent(const char* eventName, const void* associatedData, CallbackId parentCallback)
{
  auto insertLocation = m_registeredEvents.end();
  EventId unusedId = 1;
  for(auto& it : m_registeredEvents)
  {
    if(it.name() == eventName &&
       it.parentCallbackId() == parentCallback &&
       it.associatedData() == associatedData)
    {
      MGlobal::displayError(MString("The event \"") + MString(eventName) + " has already been registered");
      return 0;
    }
  }
  for(auto it = m_registeredEvents.begin(), e = m_registeredEvents.end(); it != e; ++it, ++unusedId)
  {
    if(it->eventId() != unusedId)
    {
      insertLocation = it;
      break;
    }
  }
  m_registeredEvents.emplace(insertLocation, eventName, unusedId, associatedData, parentCallback);
  return unusedId;
}

//----------------------------------------------------------------------------------------------------------------------
bool EventScheduler::unregisterEvent(EventId eventId)
{
  auto it = std::lower_bound(m_registeredEvents.begin(), m_registeredEvents.end(), eventId);
  if(it != m_registeredEvents.end())
  {
    if(it->eventId() == eventId)
    {
      m_registeredEvents.erase(it);
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool EventScheduler::unregisterEvent(const char* const eventName)
{
  for(auto it = m_registeredEvents.begin(), e = m_registeredEvents.end(); it != e; ++it)
  {
    if(it->name() == eventName &&
       it->associatedData() == 0)
    {
      m_registeredEvents.erase(it);
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
EventDispatcher* EventScheduler::event(EventId eventId)
{
  auto it = std::lower_bound(m_registeredEvents.begin(), m_registeredEvents.end(), eventId);
  if(it != m_registeredEvents.end())
  {
    if(it->eventId() == eventId)
    {
      return m_registeredEvents.data() + (it - m_registeredEvents.begin());
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------
const EventDispatcher* EventScheduler::event(EventId eventId) const
{
  auto it = std::lower_bound(m_registeredEvents.begin(), m_registeredEvents.end(), eventId);
  if(it != m_registeredEvents.end())
  {
    if(it->eventId() == eventId)
    {
      return m_registeredEvents.data() + (it - m_registeredEvents.begin());
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------
EventDispatcher* EventScheduler::event(const char* const eventName)
{
  for(auto it = m_registeredEvents.begin(), e = m_registeredEvents.end(); it != e; ++it)
  {
    if(it->name() == eventName)
    {
      return m_registeredEvents.data() + (it - m_registeredEvents.begin());
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------
const EventDispatcher* EventScheduler::event(const char* const eventName) const
{
  for(auto it = m_registeredEvents.begin(), e = m_registeredEvents.end(); it != e; ++it)
  {
    if(it->name() == eventName)
    {
      return m_registeredEvents.data() + (it - m_registeredEvents.begin());
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------
bool EventScheduler::unregisterCallback(CallbackId callbackId)
{
  EventId eventId = extractEventId(callbackId);
  EventDispatcher* eventInfo = event(eventId);
  if(eventInfo)
  {
    return eventInfo->unregisterCallback(callbackId);
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool EventScheduler::unregisterCallback(CallbackId callbackId, Callback& info)
{
  EventId eventId = extractEventId(callbackId);
  EventDispatcher* eventInfo = event(eventId);
  if(eventInfo)
  {
    return eventInfo->unregisterCallback(callbackId, info);
  }
  return false;
}


//----------------------------------------------------------------------------------------------------------------------
Callback* EventScheduler::findCallback(CallbackId callbackId)
{
  EventId eventId = extractEventId(callbackId);
  EventDispatcher* eventInfo = event(eventId);
  if(eventInfo)
  {
    return eventInfo->findCallback(callbackId);
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
