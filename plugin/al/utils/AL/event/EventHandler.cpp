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
#include "AL/event/EventHandler.h"

#include <algorithm>
#include <cstring>
#include <iostream>

namespace AL {
namespace event {

EventScheduler* g_scheduler = 0;

//----------------------------------------------------------------------------------------------------------------------
EventScheduler& EventScheduler::getScheduler()
{
    // assert(g_scheduler);
    return *g_scheduler;
}

//----------------------------------------------------------------------------------------------------------------------
void EventScheduler::initScheduler(EventSystemBinding* system)
{
    g_scheduler = new EventScheduler(system);
}

//----------------------------------------------------------------------------------------------------------------------
void EventScheduler::freeScheduler()
{
    delete g_scheduler;
    g_scheduler = 0;
}

//----------------------------------------------------------------------------------------------------------------------
EventScheduler::~EventScheduler()
{
    for (auto it : m_customHandlers) {
        delete it.second;
    }
}

//----------------------------------------------------------------------------------------------------------------------
Callback::Callback(
    const char* const tag,
    const char* const commandText,
    uint32_t          weight,
    bool              isPython,
    CallbackId        callbackId)
    : m_tag(tag)
    , m_userData(0)
    , m_callbackId(callbackId)
{
    size_t len = std::strlen(commandText) + 1;
    char*  ptr = new char[len];
    m_callbackString = ptr;
    std::memcpy(ptr, commandText, len);
    m_weight = weight;
    m_functionType = isPython ? kPython : kMEL;
}

//----------------------------------------------------------------------------------------------------------------------
Callback::~Callback()
{
    if (m_functionType != kCFunction) {
        delete[] m_callbackString;
    }
}

//----------------------------------------------------------------------------------------------------------------------
Callback EventDispatcher::buildCallbackInternal(
    const char* const tag,
    const void*       functionPointer,
    uint32_t          weight,
    void*             userData)
{
    CallbackId newId = makeCallbackId(eventId(), eventType(), InvalidCallbackId);
    for (auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it) {
        if (it->tag() == tag && it->userData() == userData) {
            m_system->error(
                "An attempt to register the same event tag twice occurred - \"%s\"", tag);
            return Callback();
        }
        newId = std::max(newId, it->callbackId());
    }
    return Callback(tag, functionPointer, weight, userData, ++newId);
}

//----------------------------------------------------------------------------------------------------------------------
CallbackId EventDispatcher::registerCallbackInternal(
    const char* const tag,
    const void*       functionPointer,
    uint32_t          weight,
    void*             userData)
{
    CallbackId newId = makeCallbackId(eventId(), eventType(), InvalidCallbackId);
    auto       insertLocation = m_callbacks.end();
    for (auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it) {
        if (insertLocation == e && it->weight() >= weight) {
            insertLocation = it;
        }

        if (it->tag() == tag && it->userData() == userData) {
            m_system->error(
                "An attempt to register the same event tag twice occurred - \"%s\"", tag);
            return InvalidCallbackId;
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
    uint32_t          weight,
    bool              isPython)
{
    CallbackId newId = makeCallbackId(eventId(), eventType(), InvalidCallbackId);
    auto       insertLocation = m_callbacks.end();
    for (auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it) {
        if (insertLocation == e && it->weight() >= weight) {
            insertLocation = it;
        }

        if (it->tag() == tag) {
            m_system->error(
                "An attempt to register the same event tag twice occurred - \"%s\"", tag);
            return InvalidCallbackId;
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
    uint32_t          weight,
    bool              isPython)
{
    CallbackId newId = makeCallbackId(eventId(), eventType(), InvalidCallbackId);
    for (auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it) {
        if (it->tag() == tag) {

            std::cerr
                << "EventDispatcher: An attempt to register the same event tag twice occurred - "
                << tag << std::endl;
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
    for (auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it) {
        if (insertLocation == e && it->weight() >= info.weight()) {
            insertLocation = it;
        }

        if (it->tag() == info.tag() && it->userData() == info.userData()) {
            m_system->error(
                "An attempt to register the same event tag twice occurred - \"%s\"",
                info.tag().c_str());
            return;
        }
    }
    m_callbacks.insert(insertLocation, std::move(info));
}

//----------------------------------------------------------------------------------------------------------------------
bool EventDispatcher::unregisterCallback(CallbackId callbackId)
{
    for (auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it) {
        if (it->callbackId() == callbackId) {
            m_callbacks.erase(it);
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool EventDispatcher::unregisterCallback(CallbackId callbackId, Callback& info)
{
    for (auto it = m_callbacks.begin(), e = m_callbacks.end(); it != e; ++it) {
        if (it->callbackId() == callbackId) {
            info = std::move(*it);
            m_callbacks.erase(it);
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
EventId EventScheduler::registerEvent(
    const char* eventName,
    EventType   eventType,
    const void* associatedData,
    CallbackId  parentCallback)
{
    auto    insertLocation = m_registeredEvents.end();
    EventId unusedId = 1;
    for (auto& it : m_registeredEvents) {
        if (it.name() == eventName) {
            if (it.eventType() == kUnknownEventType) {
                it.m_eventType = eventType;
                it.m_associatedData = associatedData;
                it.m_parentCallback = parentCallback;
                return it.eventId();
            } else if (
                it.parentCallbackId() == parentCallback && it.associatedData() == associatedData) {
                m_system->error("The event \"%s\" has already been registered", eventName);
                return 0;
            }
        }
    }
    for (auto it = m_registeredEvents.begin(), e = m_registeredEvents.end(); it != e;
         ++it, ++unusedId) {
        if (it->eventId() != unusedId) {
            insertLocation = it;
            break;
        }
    }

    m_registeredEvents.emplace(
        insertLocation, m_system, eventName, unusedId, eventType, associatedData, parentCallback);
    return unusedId;
}

//----------------------------------------------------------------------------------------------------------------------
bool EventScheduler::unregisterEvent(EventId eventId)
{
    auto it = std::lower_bound(m_registeredEvents.begin(), m_registeredEvents.end(), eventId);
    if (it != m_registeredEvents.end()) {
        if (it->eventId() == eventId) {
            m_registeredEvents.erase(it);
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool EventScheduler::unregisterEvent(const char* const eventName)
{
    for (auto it = m_registeredEvents.begin(), e = m_registeredEvents.end(); it != e; ++it) {
        if (it->name() == eventName && it->associatedData() == 0) {
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
    if (it != m_registeredEvents.end()) {
        if (it->eventId() == eventId) {
            return m_registeredEvents.data() + (it - m_registeredEvents.begin());
        }
    }
    return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------
const EventDispatcher* EventScheduler::event(EventId eventId) const
{
    auto it = std::lower_bound(m_registeredEvents.begin(), m_registeredEvents.end(), eventId);
    if (it != m_registeredEvents.end()) {
        if (it->eventId() == eventId) {
            return m_registeredEvents.data() + (it - m_registeredEvents.begin());
        }
    }
    return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------
EventDispatcher* EventScheduler::event(const char* const eventName)
{
    for (auto it = m_registeredEvents.begin(), e = m_registeredEvents.end(); it != e; ++it) {
        if (it->name() == eventName) {
            return m_registeredEvents.data() + (it - m_registeredEvents.begin());
        }
    }
    return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------
const EventDispatcher* EventScheduler::event(const char* const eventName) const
{
    for (auto it = m_registeredEvents.begin(), e = m_registeredEvents.end(); it != e; ++it) {
        if (it->name() == eventName) {
            return m_registeredEvents.data() + (it - m_registeredEvents.begin());
        }
    }
    return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------
bool EventScheduler::unregisterCallback(CallbackId callbackId)
{
    EventId          eventId = extractEventId(callbackId);
    EventDispatcher* eventInfo = event(eventId);
    if (eventInfo) {
        if (eventInfo->unregisterCallback(callbackId)) {
            auto et = extractEventType(callbackId);
            auto handler = m_customHandlers.find(et);
            if (handler != m_customHandlers.end()) {
                handler->second->onCallbackDestroyed(callbackId);
            }
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool EventScheduler::unregisterCallback(CallbackId callbackId, Callback& info)
{
    EventId          eventId = extractEventId(callbackId);
    EventDispatcher* eventInfo = event(eventId);
    if (eventInfo) {
        if (eventInfo->unregisterCallback(callbackId, info)) {
            auto et = extractEventType(callbackId);
            auto handler = m_customHandlers.find(et);
            if (handler != m_customHandlers.end()) {
                handler->second->onCallbackDestroyed(callbackId);
            }
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
Callback* EventScheduler::findCallback(CallbackId callbackId)
{
    EventId          eventId = extractEventId(callbackId);
    EventDispatcher* eventInfo = event(eventId);
    if (eventInfo) {
        return eventInfo->findCallback(callbackId);
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace event
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
