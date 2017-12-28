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

#include "AL/usdmaya/Common.h"
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MMessage.h>
#include <maya/MPxNode.h>
#include <string>
#include <vector>
#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {

/// \ingroup events
typedef uint16_t EventId;
/// \ingroup events
typedef uint64_t CallbackId;
/// \ingroup events
typedef std::vector<EventId> EventIds;
/// \ingroup events
typedef std::vector<CallbackId> CallbackIds;

/// \brief  extracts the event ID from a callback ID
/// \ingroup events
inline EventId extractEventId(CallbackId id)
{
  return id >> 48;
}

/// \brief  extracts the unique 48bit callback ID
/// \ingroup events
inline CallbackId extractCallbackId(CallbackId id)
{
  return id & 0xFFFFFFFFFFFFULL;
}

/// \brief  constructs a 64bit callback ID from a event ID and unique callback id
/// \param  event the event id
/// \param  id the callback id
/// \return the combined callback ID
/// \ingroup events
inline CallbackId makeCallbackId(EventId event, CallbackId id)
{
  return (CallbackId(event) << 48) | id;
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Stores the information required for a single callback.
/// \ingroup events
//----------------------------------------------------------------------------------------------------------------------
class Callback
{
  friend class EventDispatcher;
public:

  /// \brief  construct an event structure associated with a C function callback
  /// \param  tag a unique identifier for this tag
  /// \param  functionPointer the pointer to the function for this callback
  template<typename func_type>
  Callback(
    const char* const tag,
    const func_type functionPointer,
    uint32_t weight,
    void* userData,
    CallbackId callbackId)
    : m_tag(tag), m_userData(userData), m_callbackId(callbackId)
  {
    m_callback = (const void*)functionPointer;
    m_weight = weight;
    m_isPython = 0;
    m_isCFunction = 1;
  }

  /// \brief  construct an event structure associated with a C function callback
  /// \param  tag a unique identifier for this tag
  /// \param  functionPointer the pointer to the function for this callback
  Callback(
    const char* const tag,
    const char* const commandText,
    uint32_t weight,
    bool isPython,
    CallbackId eventId);

  /// \brief  default ctor
  Callback()
    : m_tag(), m_userData(nullptr), m_callbackId(0)
  {
    m_callback = 0;
    m_weight = 0;
    m_isPython = 0;
    m_isCFunction = 0;
  }

  /// \brief  move ctor
  /// \param  rhs the rvalue to move
  Callback(Callback&& rhs)
    : m_tag(std::move(rhs.m_tag)), m_userData(rhs.m_userData), m_callbackId(rhs.m_callbackId)
  {
    m_callback = rhs.m_callback; rhs.m_callback = 0;
    m_weight = rhs.m_weight;
    m_isPython = rhs.m_isPython;
    m_isCFunction = rhs.m_isCFunction;
  }

  /// \brief  move assignment
  /// \param  rhs the rvalue to move
  /// \return a reference to this
  Callback& operator = (Callback&& rhs)
    {
      m_tag = std::move(rhs.m_tag);
      m_userData = rhs.m_userData;
      m_callbackId = rhs.m_callbackId;
      m_callback = rhs.m_callback;
      rhs.m_callback = 0;
      m_weight = rhs.m_weight;
      m_isPython = rhs.m_isPython;
      m_isCFunction = rhs.m_isCFunction;
      return *this;
    }

  /// \brief  dtor
  ~Callback();

  /// \brief  less than comparison. Sorts so that the lowest weight is first
  /// \param  info the structure to compare against
  /// \return true if the weight of this is lower than that of info
  inline bool operator < (const Callback& info) const
    { return m_weight < info.m_weight; }

  /// \brief  returns the callback id for this callback
  CallbackId callbackId() const
    { return m_callbackId; }

  /// \brief  returns the event id that triggers this callback
  EventId eventId() const
    { return m_callbackId >> 48; }

  /// \brief  returns the tag assigned to this callback (so we know which tool/script created it)
  const std::string& tag() const
    { return m_tag; }

  /// \brief  returns the user data pointer associated with this callback (or null if no pointer set)
  /// \return the custom user data pointer
  void* userData() const
    { return m_userData; }

  /// \brief  returns a raw pointer to the function pointer (up to call location to trigger with correct args).
  const void* callback() const
    { return isCCallback() ? m_callback : nullptr; }

  /// \brief  returns the callback text
  const char* callbackText() const
    { return !isCCallback() ? m_callbackString : ""; }

  /// \brief  returns the weight associated with this callback
  uint32_t weight() const
    { return m_weight; }

  /// \returns true if this callback is python code
  bool isPythonCallback() const
    { return (!m_isCFunction) && m_isPython; }

  /// \returns true if this callback is MEL code
  bool isMELCallback() const
    { return (!m_isCFunction) && !m_isPython; }

  /// \returns true if this callback is a C++ callback
  bool isCCallback() const
    { return m_isCFunction; }

private:
  std::string m_tag;
  void* m_userData;
  CallbackId m_callbackId;
  union
  {
    const void* m_callback;
    const char* m_callbackString;
  };
  struct
  {
    uint32_t m_weight : 30; ///< the weighting value for the event
    uint32_t m_isPython : 1; ///< if true (and the C++ function pointer is NULL), the command string will be treated as python, otherwise MEL
    uint32_t m_isCFunction : 1; ///< true if the code is a C function
  };
};
typedef std::vector<Callback> Callbacks;

//----------------------------------------------------------------------------------------------------------------------
/// \brief
/// \ingroup events
//----------------------------------------------------------------------------------------------------------------------
class EventDispatcher
{
public:

  /// \brief  default ctor
  EventDispatcher(
      const char* const name,
      EventId eventId,
      const void* associatedData = 0,
      CallbackId parentCallback = 0)
    : m_name(name), m_callbacks(), m_associatedData(associatedData), m_eventId(eventId), m_parentCallback(parentCallback)
    {}

  /// \brief  move ctor
  EventDispatcher(EventDispatcher&& rhs)
    : m_name(std::move(rhs.m_name)),
        m_callbacks(std::move(rhs.m_callbacks)),
        m_associatedData(rhs.m_associatedData),
        m_eventId(rhs.m_eventId),
        m_parentCallback(rhs.m_parentCallback)
    {}

  /// \brief  move assignment
  EventDispatcher& operator = (EventDispatcher&& rhs)
    {
      m_name = std::move(rhs.m_name);
      m_callbacks = std::move(rhs.m_callbacks);
      m_associatedData = rhs.m_associatedData;
      m_eventId = rhs.m_eventId;
      m_parentCallback = rhs.m_parentCallback;
      return *this;
    }

  /// \brief  returns the name of the registered event
  const std::string& name() const
    { return m_name; }

  /// \brief  returns the array of registered callbacks against this event
  const Callbacks& callbacks() const
    { return m_callbacks; }

  /// \brief  construct an event structure associated with a C function callback
  /// \param  tag a unique identifier for this tag
  /// \param  functionPointer the pointer to the function for this callback
  template<typename func_type>
  CallbackId registerCallback(
    const char* const tag,
    const func_type functionPointer,
    uint32_t weight,
    void* userData)
    { return registerCallbackInternal(tag, (const void*)functionPointer, weight, userData); }

  /// \brief  construct an event structure associated with a C function callback
  /// \param  tag a unique identifier for this tag
  /// \param  functionPointer the pointer to the function for this callback
  template<typename func_type>
  Callback buildCallback(
    const char* const tag,
    const func_type functionPointer,
    uint32_t weight,
    void* userData)
    { return buildCallbackInternal(tag, (const void*)functionPointer, weight, userData); }

  /// \brief  construct an event structure associated with a C function callback
  /// \param  tag a unique identifier for this tag
  /// \param  functionPointer the pointer to the function for this callback
  CallbackId registerCallback(
    const char* const tag,
    const char* const commandText,
    uint32_t weight,
    bool isPython);

  /// \brief  construct an event structure associated with a C function callback
  /// \param  tag a unique identifier for this tag
  /// \param  functionPointer the pointer to the function for this callback
  Callback buildCallback(
    const char* const tag,
    const char* const commandText,
    uint32_t weight,
    bool isPython);

  /// \brief  registers a new event with the system (method used when undo/redo is needed).
  ///         The event callback structure passed into this function will be moved into the event handler
  ///         (so the event will become invalidated afterwards). Please do not use this function!!
  void registerCallback(Callback& info);

  /// \brief  unregister a registered  event
  /// \param  callbackId the event to unregister
  bool unregisterCallback(CallbackId callbackId);

  /// \brief  unregister a registered  event
  /// \param  callbackId the event to unregister
  /// \param  returnedInfo the returned event info (used to undo insertion of events)
  bool unregisterCallback(CallbackId callbackId, Callback& returnedInfo);

  /// \brief  returns the event id
  /// \return the event id
  EventId eventId() const
    { return m_eventId; }

  /// \brief  returns the event id
  /// \return the event id
  CallbackId parentCallbackId() const
    { return m_parentCallback; }

  /// \brief  This method dispatches the event to all callbacks. It is important that you provide a
  ///         function binding interface to pass the stored callback args to the callback signiture of your
  ///         event handlers. By default the code assumes a C function prototype of:
  /// \code
  /// void default_function_prototype(void* userData);
  /// \endcode
  /// However if we wished to use a prototype such as:
  /// \code
  /// void proxy_function_prototype(void* userData, AL::usdmaya::nodes::ProxyShape* proxyInstance);
  /// \endcode
  /// Then we would need to provide a specific function binder, like so:
  /// \code
  /// // function prototype of callback we wish to register
  /// typedef void (*proxy_function_prototype)(void*, AL::usdmaya::nodes::ProxyShape*);
  ///
  /// // the function binding interface
  /// struct ProxyShapeFunctionBinder
  /// {
  ///   void operator(void* userPtr, const void* callback)
  ///   {
  ///     proxy_function_prototype func = (proxy_function_prototype)callback;
  ///     func(userPtr, m_nodeToPass);
  ///   }
  ///
  ///   AL::usdmaya::nodes::ProxyShape* m_nodeToPass;
  /// };
  ///
  /// // and finally, when we wish to dispatch the specific event, we can do...
  /// void myCustomEventDispatch(EventDispatcher& eventThing, AL::usdmaya::nodes::ProxyShape* proxy)
  /// {
  ///   eventThing.dispatchEvent( ProxyShapeFunctionBinder{ proxy } );
  /// }
  /// \endcode
  template<typename FunctionBinder>
  void triggerEvent(FunctionBinder binder)
  {
    for(auto& callback : m_callbacks)
    {
      if(callback.isCCallback())
      {
        binder(callback.userData(), callback.callback());
      }
      else
      if(callback.isPythonCallback())
      {
        if(!MGlobal::executePythonCommand(callback.callbackText(), false, true))
        {
          MGlobal::displayError(
              MString("The python callback of event name \"") +
              MString(m_name.c_str()) +
              MString("\" and tag \"") +
              MString(callback.tag().c_str()) +
              MString("failed to execute correctly"));

        }
      }
      else
      {
        if(!MGlobal::executeCommand(callback.callbackText(), false, true))
        {
          MGlobal::displayError(
              MString("The MEL callback of event name \"") +
              MString(m_name.c_str()) +
              MString("\" and tag \"") +
              MString(callback.tag().c_str()) +
              MString("failed to execute correctly"));
        }
      }
    }
  }

  /// \brief  a default version of dispatchEvent that assumes a function callback type of
  /// \code
  /// void (*function_prototype)(void* userData);
  /// \endcode
  void triggerEvent()
  {
    for(auto& callback : m_callbacks)
    {
      if(callback.isCCallback())
      {
        MMessage::MBasicFunction basic = (MMessage::MBasicFunction)callback.callback();
        basic(callback.userData());
      }
      else
      if(callback.isPythonCallback())
      {
        if(!MGlobal::executePythonCommand(callback.callbackText(), false, true))
        {
          MGlobal::displayError(
              MString("The python callback of event name \"") +
              MString(m_name.c_str()) +
              MString("\" and tag \"") +
              MString(callback.tag().c_str()) +
              MString("failed to execute correctly"));

        }
      }
      else
      {
        if(!MGlobal::executeCommand(callback.callbackText(), false, true))
        {
          MGlobal::displayError(
              MString("The MEL callback of event name \"") +
              MString(m_name.c_str()) +
              MString("\" and tag \"") +
              MString(callback.tag().c_str()) +
              MString("failed to execute correctly"));
        }
      }
    }
  }

  /// \brief  used to sort the events based on their ID
  bool operator < (const EventId eventId) const
    { return m_eventId < eventId; }

  /// \brief  used to sort the events based on their ID
  friend bool operator < (const EventId eventId, const EventDispatcher& info)
    { return eventId < info.m_eventId; }

  /// \brief  returns the data pointer associated with this event
  const void* associatedData() const
    { return m_associatedData; }

  /// \brief  utility function to locate a callback node
  Callback* findCallback(CallbackId id)
    {
      for(size_t i = 0, n = m_callbacks.size(); i < n; ++i)
      {
        auto& cb = m_callbacks[i];
        if(cb.callbackId() == id)
        {
          return &m_callbacks[i];
        }
      }
      return 0;
    }

private:
  CallbackId registerCallbackInternal(
    const char* const tag,
    const void* functionPointer,
    uint32_t weight,
    void* userData);
  Callback buildCallbackInternal(
    const char* const tag,
    const void* functionPointer,
    uint32_t weight,
    void* userData);
private:
  std::string m_name;
  Callbacks m_callbacks;
  const void* m_associatedData;
  CallbackId m_parentCallback;
  EventId m_eventId;
};
typedef std::vector<EventDispatcher> EventDispatchers;


//----------------------------------------------------------------------------------------------------------------------
/// \brief  a global object that maintains all of the various events registered within the system.
/// \ingroup events
//----------------------------------------------------------------------------------------------------------------------
class EventScheduler
{
  friend class EventIterator;
public:

  static EventScheduler& getScheduler();

  EventScheduler() = default;
  ~EventScheduler() = default;

  /// \brief  register a new event
  /// \param  eventName the name of event to register
  /// \param  associatedData an associated data pointer for this event [optional]
  /// \param  parentCallback if this event is triggered by a callback
  EventId registerEvent(const char* eventName, const void* associatedData = 0, const CallbackId parentCallback = 0);

  /// \brief  unregister an event handler
  /// \param  eventId the event to unregister
  /// \return true if the event is a valid id
  bool unregisterEvent(EventId eventId);

  /// \brief  unregister an event handler
  /// \param  eventId the event to unregister
  /// \return true if the event is a valid id
  bool unregisterEvent(const char* const eventId);

  /// \brief  returns a pointer to the requested event id
  /// \param  eventId the event id
  /// \return the registered callbacks for the specified event
  EventDispatcher* event(EventId eventId);

  /// \brief  returns a pointer to the requested event id
  /// \param  eventId the event id
  /// \return the registered callbacks for the specified event
  const EventDispatcher* event(EventId eventId) const;

  /// \brief  returns a pointer to the requested event id
  /// \param  eventId the event id
  /// \return the registered callbacks for the specified event
  EventDispatcher* event(const char* eventName);

  /// \brief  returns a pointer to the requested event id
  /// \param  eventId the event id
  /// \return the registered callbacks for the specified event
  const EventDispatcher* event(const char* eventName) const;

  /// \brief  dispatches an event using a function binder
  /// \param  eventId the event to dispatch
  /// \param  binder the binder to dispatch the event
  /// \return true if the event is valid
  template<typename FunctionBinder>
  bool triggerEvent(EventId eventId, FunctionBinder binder)
  {
    EventDispatcher* e = event(eventId);
    if(e)
    {
      e->triggerEvent(binder);
      return true;
    }
    return false;
  }

  /// \brief  dispatches an event using the standard void (*func)(void* userData) signature
  /// \param  eventId the event to dispatch
  /// \return true if the event is valid
  bool triggerEvent(EventId eventId)
  {
    EventDispatcher* e = event(eventId);
    if(e)
    {
      e->triggerEvent();
      return true;
    }
    return false;
  }

  /// \brief  dispatches an event using the standard void (*func)(void* userData) signature
  /// \param  eventId the event to dispatch
  /// \return true if the event is valid
  bool triggerEvent(const char* const eventName)
  {
    EventDispatcher* e = event(eventName);
    if(e)
    {
      e->triggerEvent();
      return true;
    }
    return false;
  }

  /// \brief  register a new event callback
  /// \param  eventId the event id
  /// \param  tag the tag for the callback
  /// \param  functionPointer the function pointer to call
  /// \param  weight the weight for the callback (determines the ordering)
  /// \param  userData associated user data for the callback
  /// \return the callback id for the newly registered callback
  template<typename func_type>
  CallbackId registerCallback(
    const EventId eventId,
    const char* const tag,
    const func_type functionPointer,
    uint32_t weight,
    void* userData = 0)
    {
      EventDispatcher* eventInfo = event(eventId);
      if(eventInfo)
      {
        return eventInfo->registerCallback(tag, functionPointer, weight, userData);
      }
      return 0;
    }

  /// \brief  register a new event callback (in python or MEL)
  /// \param  eventId the event id
  /// \param  tag the tag for the callback
  /// \param  commandText
  /// \param  weight the weight for the callback (determines the ordering)
  /// \param  isPython true if the callback is python, false if MEL
  /// \return the callback id for the newly registered callback
  CallbackId registerCallback(
    const EventId eventId,
    const char* const tag,
    const char* const commandText,
    uint32_t weight,
    bool isPython)
  {
    EventDispatcher* eventInfo = event(eventId);
    if(eventInfo)
    {
      return eventInfo->registerCallback(tag, commandText, weight, isPython);
    }
    return 0;
  }

  /// \brief  register a new event callback
  /// \param  eventId the event id
  /// \param  tag the tag for the callback
  /// \param  functionPointer the function pointer to call
  /// \param  weight the weight for the callback (determines the ordering)
  /// \param  userData associated user data for the callback
  /// \return the callback id for the newly registered callback
  template<typename func_type>
  Callback buildCallback(
    const EventId eventId,
    const char* const tag,
    const func_type functionPointer,
    uint32_t weight,
    void* userData = 0)
    {
      EventDispatcher* eventInfo = event(eventId);
      if(eventInfo)
      {
        return eventInfo->buildCallback(tag, functionPointer, weight, userData);
      }
      return Callback();
    }

  /// \brief  register a new event callback (in python or MEL)
  /// \param  eventId the event id
  /// \param  tag the tag for the callback
  /// \param  commandText
  /// \param  weight the weight for the callback (determines the ordering)
  /// \param  isPython true if the callback is python, false if MEL
  /// \return the callback id for the newly registered callback
  Callback buildCallback(
    const EventId eventId,
    const char* const tag,
    const char* const commandText,
    uint32_t weight,
    bool isPython)
  {
    EventDispatcher* eventInfo = event(eventId);
    if(eventInfo)
    {
      return eventInfo->buildCallback(tag, commandText, weight, isPython);
    }
    return Callback();
  }

  /// \brief  register a new event callback
  /// \param  eventId the event id
  /// \param  tag the tag for the callback
  /// \param  functionPointer the function pointer to call
  /// \param  weight the weight for the callback (determines the ordering)
  /// \param  userData associated user data for the callback
  /// \return the callback id for the newly registered callback
  template<typename func_type>
  Callback buildCallback(
    const char* const eventName,
    const char* const tag,
    const func_type functionPointer,
    uint32_t weight,
    void* userData = 0)
    {
      EventDispatcher* eventInfo = event(eventName);
      if(eventInfo)
      {
        return eventInfo->buildCallback(tag, functionPointer, weight, userData);
      }
      return Callback();
    }

  /// \brief  register a new event callback (in python or MEL)
  /// \param  eventId the event id
  /// \param  tag the tag for the callback
  /// \param  commandText
  /// \param  weight the weight for the callback (determines the ordering)
  /// \param  isPython true if the callback is python, false if MEL
  /// \return the callback id for the newly registered callback
  Callback buildCallback(
    const char* const eventName,
    const char* const tag,
    const char* const commandText,
    uint32_t weight,
    bool isPython)
  {
    EventDispatcher* eventInfo = event(eventName);
    if(eventInfo)
    {
      return eventInfo->buildCallback(tag, commandText, weight, isPython);
    }
    return Callback();
  }

  /// \brief  unregister an event handler
  bool unregisterCallback(CallbackId callbackId);

  /// \brief  unregister an event handler
  bool unregisterCallback(CallbackId callbackId, Callback& info);


  /// \brief  unregister an event handler
  CallbackId registerCallback(Callback& info)
  {
    EventDispatcher* eventInfo = event(info.eventId());
    if(eventInfo)
    {
      CallbackId id = info.callbackId();
      eventInfo->registerCallback(info);
      return id;
    }
    return 0;
  }

  const EventDispatchers& registeredEvents() const
    { return m_registeredEvents; }

  Callback* findCallback(CallbackId callbackId);

private:
  EventDispatchers m_registeredEvents;
};

typedef void (*maya_node_dispatch_func)(void* userData, MPxNode* node);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  defines an interface that can be applied to custom nodes to allow them the ability to manage and dispatch
///         internal events.
/// \ingroup events
//----------------------------------------------------------------------------------------------------------------------
class MayaNodeEvents
{
  typedef std::unordered_map<std::string, EventId> EventMap;
public:

  /// \brief  ctor
  /// \param  registrar the event registrar
  MayaNodeEvents(EventScheduler* const scheduler = &EventScheduler::getScheduler())
    : m_scheduler(scheduler)
    {}

  /// \brief  trigger the event of the given name
  /// \param  eventName the name of the event to trigger on this node
  bool triggerEvent(const char* const eventName)
  {
    auto it = m_events.find(eventName);
    if(it !=  m_events.end())
    {
      return m_scheduler->triggerEvent(it->second,
          [this](void* userData, const void* callback) {
              ((maya_node_dispatch_func)callback)(userData, dynamic_cast<MPxNode*>(this));
          });
    }
    return false;
  }

  /// \brief  dtor
  virtual ~MayaNodeEvents()
  {
    for(auto event : m_events)
    {
      m_scheduler->unregisterEvent(event.second);
    }
  }

  /// \brief  returns the event registrar
  EventScheduler* scheduler() const
    { return m_scheduler; }

  /// \brief  returns the event ID for the specified event name
  /// \param  eventName the name of the event
  /// \return the ID of the event (or zero)
  EventId getId(const char* const eventName) const
  {
    const auto it = m_events.find(eventName);
    if(it != m_events.end())
    {
      return it->second;
    }
    return 0;
  }

  /// \brief  returns the internal event map
  /// \return the event map
  const EventMap& events() const
    { return m_events; }

  /// \brief  registers an event on this node
  /// \param  eventName the name of the event to register
  /// \param  parentId the callback id of the callback that triggers this event (or null)
  /// \return true if the event could be registered
  bool registerEvent(const char* const eventName, const CallbackId parentId = 0)
  {
    EventId id = m_scheduler->registerEvent(eventName, this, parentId);
    if(id)
    {
      m_events.emplace(eventName, id);
    }
    return id != 0;
  }

  /// \brief  registers an event on this node
  /// \param  eventName the name of the event to register
  /// \param  parentId the callback id of the callback that triggers this event (or null)
  /// \return true if the event could be registered
  bool unregisterEvent(const char* const eventName)
  {
    auto it = m_events.find(eventName);
    if(it != m_events.end())
    {
      m_events.erase(it);
      return m_scheduler->unregisterEvent(it->second);
    }
    return false;
  }

private:
  EventMap m_events;
  EventScheduler* m_scheduler;
};

//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
