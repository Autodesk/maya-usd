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
#pragma once

#include "AL/event/Api.h"

#include <cstdarg>
#include <string>
#include <unordered_map>
#include <vector>

namespace AL {
namespace event {

constexpr uint64_t kNumEventIdBits = 20;    ///< number of bits for the event ID
constexpr uint64_t kNumEventTypeBits = 4;   ///< number of bits for the event Type
constexpr uint64_t kNumCallbackIdBits = 40; ///< number of bits for the callback ID

constexpr uint64_t kNumEventIdBitMask = 0xFFFFFFFFFFFFFFFFULL
    << (kNumEventTypeBits + kNumCallbackIdBits); ///< bit mask storing the event ID
constexpr uint64_t kNumCallbackBitMask = (0xFFFFFFFFFFFFFFFFULL)
    >> (kNumEventIdBits + kNumEventTypeBits); ///< bit mask for the callbacks
constexpr uint64_t kNumEventTypeMask = (0xFFFFFFFFFFFFFFFFULL)
    ^ (kNumEventIdBitMask | kNumCallbackBitMask); ///< bit mask for the event type

constexpr uint32_t kUnknownEventType = 0;
constexpr uint32_t kUserSpecifiedEventType = 1;
constexpr uint32_t kSchemaEventType = 2;
constexpr uint32_t kUSDMayaEventType = 3;
constexpr uint32_t kMayaEventType = 4;

/// \brief  an enum describing the callback type
/// \ingroup events
enum CallbackType
{
    kCFunction, ///< a c function callback
    kPython,    ///< a python callback
    kMEL        ///< MEL script callback
};

/// \ingroup events
/// the default C++ function prototype for a callback
typedef void (*defaultEventFunction)(void*);

/// \ingroup events
/// \brief  A value to represent and event
typedef uint32_t EventId;
/// \ingroup events
/// \brief  A value used to describe the type of an event (e.g. does the event come from maya,
/// usdmaya, or is a custom user defined event?)
typedef uint32_t EventType;
/// \ingroup events
/// \brief  An identifier used to represent a callback. Within the 64bit value 3 pieces of
/// information are encoded: \li The event id, which can be extracted with the extractEventId()
/// method \li The event type, which can be extracted with the extractEventType() method \li The
/// callback id, which can be extracted with the extractCallbackId() method The 3 fields are used to
/// uniquely define a callback within the event system
typedef uint64_t CallbackId;
/// \ingroup events
/// \brief  an array of event IDs
typedef std::vector<EventId> EventIds;
/// \ingroup events
/// \brief  an array of callback IDs
typedef std::vector<CallbackId> CallbackIds;

/// \brief  extracts the event ID from a callback ID
/// \param  id the callback id from which you wish to extract the event id from
/// \ingroup events
inline EventId extractEventId(CallbackId id)
{
    return (kNumEventIdBitMask & id) >> (kNumEventTypeBits + kNumCallbackIdBits);
}

/// \brief  extracts the 4bit event type from the calback id
/// \param  id the callback id from which you wish to extract the event type from
/// \ingroup events
inline EventType extractEventType(CallbackId id)
{
    return (kNumEventTypeMask & id) >> kNumCallbackIdBits;
}

/// \brief  extracts the unique 40bit callback ID (which is an instance id of the specified event)
/// \param  id the callback id from which you wish to extract the callback id from
/// \ingroup events
inline CallbackId extractCallbackId(CallbackId id) { return (kNumCallbackBitMask & id); }

/// \brief  constructs a 64bit callback ID from a event ID and unique callback id
/// \param  event the event id
/// \param  type the event type
/// \param  id the callback id
/// \return the combined callback ID
/// \ingroup events
inline CallbackId makeCallbackId(EventId event, uint32_t type, CallbackId id)
{
    return (CallbackId(event) << (kNumEventTypeBits + kNumCallbackIdBits))
        | (CallbackId(type) << kNumCallbackIdBits) | id;
}

/// \brief  the invalid callback ID
/// \ingroup events
constexpr CallbackId InvalidCallbackId = 0;

/// \brief  the invalid callback ID
/// \ingroup events
constexpr EventId InvalidEventId = 0;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  An interface that provides the event system with some utilities from the underlying DCC
/// application. \ingroup events
//----------------------------------------------------------------------------------------------------------------------
class EventSystemBinding
{
public:
    /// \brief  ctor
    /// \param  eventTypeStrings the event types as text strings
    /// \param  numberOfEventTypes the number of event types in the event strings
    EventSystemBinding(const char* const eventTypeStrings[], size_t numberOfEventTypes)
        : m_eventTypeStrings(eventTypeStrings)
        , m_numberOfEventTypes(numberOfEventTypes)
    {
    }

    /// \brief  the logging severity
    enum Type
    {
        kInfo,
        kWarning,
        kError
    };

    /// \brief  log an info message
    /// \param  text printf style text string
    void info(const char* text, ...)
    {
        char    buffer[1024];
        va_list args;
        va_start(args, text);
        vsnprintf(buffer, 1024, text, args);
        writeLog(kInfo, buffer);
        va_end(args);
    }

    /// \brief  log an error message
    /// \param  text printf style text string
    void error(const char* text, ...)
    {
        char    buffer[1024];
        va_list args;
        va_start(args, text);
        vsnprintf(buffer, 1024, text, args);
        writeLog(kError, buffer);
        va_end(args);
    }

    /// \brief  log a warning message
    /// \param  text printf style text string
    void warning(const char* text, ...)
    {
        char    buffer[1024];
        va_list args;
        va_start(args, text);
        vsnprintf(buffer, 1024, text, args);
        writeLog(kWarning, buffer);
        va_end(args);
    }

    /// \brief  override to execute python code
    /// \param  code the code to execute
    /// \return true if executed correctly
    virtual bool executePython(const char* const code) = 0;

    /// \brief  override to execute MEL code
    /// \param  code the code to execute
    /// \return true if executed correctly
    virtual bool executeMEL(const char* const code) = 0;

    /// \brief  override to implement the logging system
    /// \param  severity
    /// \param  text the text to log
    virtual void writeLog(Type severity, const char* const text) = 0;

    /// \brief  returns the event type as a string
    /// \param  eventType the eventType you want as a string
    /// \return the eventType as a text string
    const char* eventTypeString(EventType eventType) const { return m_eventTypeStrings[eventType]; }

    /// \brief  returns the total number of event types supported by the underlying system
    /// \return the number of supported event types
    size_t numberOfEventTypes() const { return m_numberOfEventTypes; }

private:
    const char* const* m_eventTypeStrings;
    size_t             m_numberOfEventTypes;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  An interface that provides the event system with some utilities from the underlying DCC
/// application. \ingroup events
//----------------------------------------------------------------------------------------------------------------------
class CustomEventHandler
{
public:
    /// \brief  dtor
    virtual ~CustomEventHandler() { }

    /// \brief  returns the event type as a string
    /// \return the eventType as a text string
    virtual const char* eventTypeString() const = 0;

    /// \brief  override if you need to insert custom event handler
    /// \param  callbackId the ID of the callback that has been created
    virtual void onCallbackCreated(const CallbackId callbackId) { }

    /// \brief  override if you need to remove a custom event handler
    /// \param  callbackId the ID of the callback that has been destroyed
    virtual void onCallbackDestroyed(const CallbackId callbackId) { }
};

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
    /// \param  weight the weight value assigned to this callback
    /// \param  userData optional user data to assign
    /// \param  callbackId the id for the callback
    template <typename func_type>
    Callback(
        const char* const tag,
        const func_type   functionPointer,
        uint32_t          weight,
        void*             userData,
        CallbackId        callbackId)
        : m_tag(tag)
        , m_userData(userData)
        , m_callbackId(callbackId)
    {
        m_callback = (const void*)functionPointer;
        m_weight = weight;
        m_functionType = kCFunction;
    }

    /// \brief  construct an event structure associated with a C function callback
    /// \param  tag a unique identifier for this tag
    /// \param  commandText the pointer to the function for this callback
    /// \param  weight the weight for the callback
    /// \param  isPython true if the callback is python
    /// \param  callbackId the callback ID for this event
    AL_EVENT_PUBLIC
    Callback(
        const char* const tag,
        const char* const commandText,
        uint32_t          weight,
        bool              isPython,
        CallbackId        callbackId);

    /// \brief  default ctor
    Callback()
        : m_tag()
        , m_userData(nullptr)
        , m_callbackId(0)
    {
        m_callback = nullptr;
        m_weight = 0;
        m_functionType = 0;
    }

    /// \brief  move ctor
    /// \param  rhs the rvalue to move
    Callback(Callback&& rhs)
        : m_tag(std::move(rhs.m_tag))
        , m_userData(rhs.m_userData)
        , m_callbackId(rhs.m_callbackId)
    {
        m_callback = rhs.m_callback;
        rhs.m_callback = nullptr;
        m_weight = rhs.m_weight;
        m_functionType = rhs.m_functionType;
    }

    /// \brief  move assignment
    /// \param  rhs the rvalue to move
    /// \return a reference to this
    Callback& operator=(Callback&& rhs)
    {
        m_tag = std::move(rhs.m_tag);
        m_userData = rhs.m_userData;
        m_callbackId = rhs.m_callbackId;
        m_callback = rhs.m_callback;
        rhs.m_callback = nullptr;
        m_weight = rhs.m_weight;
        m_functionType = rhs.m_functionType;
        return *this;
    }

    /// \brief  dtor
    AL_EVENT_PUBLIC
    ~Callback();

    /// \brief  less than comparison. Sorts so that the lowest weight is first
    /// \param  info the structure to compare against
    /// \return true if the weight of this is lower than that of info
    inline bool operator<(const Callback& info) const { return m_weight < info.m_weight; }

    /// \brief  returns the callback id for this callback
    CallbackId callbackId() const { return m_callbackId; }

    /// \brief  returns the event id that triggers this callback
    EventId eventId() const { return extractEventId(m_callbackId); }

    /// \brief  returns the type of event this system is storing (e.g. maya, usdmaya, etc)
    EventType eventType() const { return extractEventType(m_callbackId); }

    /// \brief  returns the tag assigned to this callback (so we know which tool/script created it)
    const std::string& tag() const { return m_tag; }

    /// \brief  returns the user data pointer associated with this callback (or null if no pointer
    /// set) \return the custom user data pointer
    void* userData() const { return m_userData; }

    /// \brief  returns a raw pointer to the function pointer (up to call location to trigger with
    /// correct args).
    const void* callback() const { return isCCallback() ? m_callback : nullptr; }

    /// \brief  returns the callback text
    const char* callbackText() const { return !isCCallback() ? m_callbackString : ""; }

    /// \brief  returns the weight associated with this callback
    uint32_t weight() const { return m_weight; }

    /// \returns true if this callback is python code
    bool isPythonCallback() const { return m_functionType == kPython; }

    /// \returns true if this callback is MEL code
    bool isMELCallback() const { return m_functionType == kMEL; }

    /// \returns true if this callback is a C++ callback
    bool isCCallback() const { return m_functionType == kCFunction; }

private:
    std::string m_tag;
    void*       m_userData;
    CallbackId  m_callbackId;
    union
    {
        const void* m_callback;
        const char* m_callbackString;
    };
    struct
    {
        uint32_t m_weight : 30;      ///< the weighting value for the event
        uint32_t m_functionType : 2; ///< the type of callback (e.g. C++, python, MEL)
    };
};
typedef std::vector<Callback> Callbacks;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class that manages a single event, and all the callbacks registered against that
/// specific event \ingroup events
//----------------------------------------------------------------------------------------------------------------------
class EventDispatcher
{
    friend class EventScheduler;

public:
    /// \brief  default ctor
    /// \param  system the DCC specific backend services for the event system
    /// \param  name the name of the event
    /// \param  eventId the unique identifier for the event
    /// \param  eventType the type of the event (e.g. maya, usdmaya, or custom)
    /// \param  associatedData a user data pointer to the objct instance that can trigger the event
    /// \param  parentCallback the parent callback ID that triggers the event
    EventDispatcher(
        EventSystemBinding* system,
        const char* const   name,
        EventId             eventId,
        EventType           eventType,
        const void*         associatedData = 0,
        CallbackId          parentCallback = 0)
        : m_system(system)
        , m_name(name)
        , m_callbacks()
        , m_associatedData(associatedData)
        , m_parentCallback(parentCallback)
        , m_eventId(eventId)
        , m_eventType(eventType)
    {
    }

    /// \brief  move ctor
    /// \param  rhs the event dispatcher to move
    EventDispatcher(EventDispatcher&& rhs)
        : m_system(rhs.m_system)
        , m_name(std::move(rhs.m_name))
        , m_callbacks(std::move(rhs.m_callbacks))
        , m_associatedData(rhs.m_associatedData)
        , m_parentCallback(rhs.m_parentCallback)
        , m_eventId(rhs.m_eventId)
        , m_eventType(rhs.m_eventType)
    {
    }

    /// \brief  move assignment
    /// \param  rhs the event dispatcher to move
    /// \return *this
    EventDispatcher& operator=(EventDispatcher&& rhs)
    {
        m_system = rhs.m_system;
        m_name = std::move(rhs.m_name);
        m_callbacks = std::move(rhs.m_callbacks);
        m_associatedData = rhs.m_associatedData;
        m_parentCallback = rhs.m_parentCallback;
        m_eventId = rhs.m_eventId;
        m_eventType = rhs.m_eventType;
        return *this;
    }

    /// \brief  returns the name of the registered event
    /// \return the event name
    const std::string& name() const { return m_name; }

    /// \brief  returns the array of registered callbacks against this event
    /// \return const reference to the current callbacks on the event
    const Callbacks& callbacks() const { return m_callbacks; }

    /// \brief  construct an event structure associated with a C function callback
    /// \param  tag a unique identifier for this tag
    /// \param  functionPointer the pointer to the function for this callback
    /// \param  weight the user specified weight for the callback. Lower weighted callbacks are
    /// triggered before weighted ones. \param  userData a user defined object associated with the
    /// callback \return the unique callback ID, or zero if creation failed
    template <typename func_type>
    CallbackId registerCallback(
        const char* const tag,
        const func_type   functionPointer,
        uint32_t          weight,
        void*             userData)
    {
        return registerCallbackInternal(tag, (const void*)functionPointer, weight, userData);
    }

    /// \brief  construct an event structure associated with a C function callback
    /// \param  tag a unique identifier for this tag
    /// \param  functionPointer the pointer to the function for this callback
    /// \param  weight the user specified weight for the callback. Lower weighted callbacks are
    /// triggered before weighted ones. \param  userData a user defined object associated with the
    /// callback \return the unique callback ID, or zero if creation failed
    template <typename func_type>
    Callback buildCallback(
        const char* const tag,
        const func_type   functionPointer,
        uint32_t          weight,
        void*             userData)
    {
        return buildCallbackInternal(tag, (const void*)functionPointer, weight, userData);
    }

    /// \brief  construct an event structure associated with a C function callback
    /// \param  tag a unique identifier for this tag
    /// \param  commandText the code to execute on the callback
    /// \param  weight the user specified weight for the callback. Lower weighted callbacks are
    /// triggered before weighted ones. \param  isPython true if the callback is python, false for
    /// MEL \return the unique callback ID, or zero if creation failed
    AL_EVENT_PUBLIC
    CallbackId registerCallback(
        const char* const tag,
        const char* const commandText,
        uint32_t          weight,
        bool              isPython);

    /// \brief  construct an event structure associated with a C function callback. The new callback
    /// will not yet have been
    ///         registered, so you will need to pass the returned callback object to the
    ///         registerCallback method to perform the final registration step. This method is
    ///         provided in order to simplify the doIt/undoIt/redoIt mechanisms within maya.
    /// \param  tag a unique identifier for this tag
    /// \param  commandText the code to execute on the callback
    /// \param  weight the user specified weight for the callback. Lower weighted callbacks are
    /// triggered before weighted ones. \param  isPython true if the callback is python, false for
    /// MEL \return a structure that contains all of the required callback information, which can be
    /// inserted into the event dispatcher
    ///         at a later time with the registerCallback method.
    AL_EVENT_PUBLIC
    Callback buildCallback(
        const char* const tag,
        const char* const commandText,
        uint32_t          weight,
        bool              isPython);

    /// \brief  registers a new event with the system (method used when undo/redo is needed).
    ///         The event callback structure passed into this function will be moved into the event
    ///         handler (so the event will become invalidated afterwards). Please do not use this
    ///         function!!
    /// \param  info the callback to insert into the dispatcher. Once inserted, the info structure
    /// is invalidated
    AL_EVENT_PUBLIC
    void registerCallback(Callback& info);

    /// \brief  unregister a registered  event
    /// \param  callbackId the event to unregister
    /// \return true if the callback could be removed, false if the callbackId is unknown.
    AL_EVENT_PUBLIC
    bool unregisterCallback(CallbackId callbackId);

    /// \brief  unregister a registered  event
    /// \param  callbackId the event to unregister
    /// \param  returnedInfo the returned event info (used to undo insertion of events)
    /// \return true if the callback could be removed, false if the callbackId is unknown.
    AL_EVENT_PUBLIC
    bool unregisterCallback(CallbackId callbackId, Callback& returnedInfo);

    /// \brief  returns the event id
    /// \return the event id
    EventId eventId() const { return m_eventId; }

    /// \brief  returns the event type
    /// \return the event type
    EventType eventType() const { return m_eventType; }

    /// \brief  returns the parent callback id that triggers this event
    /// \return the parent callback id that triggers this event
    CallbackId parentCallbackId() const { return m_parentCallback; }

    /// \brief  This method dispatches the event to all callbacks. It is important that you provide
    /// a
    ///         function binding interface to pass the stored callback args to the callback
    ///         signiture of your event handlers. By default the code assumes a C function prototype
    ///         of:
    /// \code
    /// void default_function_prototype(void* userData);
    /// \endcode
    /// However if we wished to use a prototype such as:
    /// \code
    /// void proxy_function_prototype(void* userData, AL::usdmaya::nodes::ProxyShape*
    /// proxyInstance); \endcode Then we would need to provide a specific function binder, like so:
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
    /// void myCustomEventDispatch(EventDispatcher& eventThing, AL::usdmaya::nodes::ProxyShape*
    /// proxy)
    /// {
    ///   eventThing.dispatchEvent( ProxyShapeFunctionBinder{ proxy } );
    /// }
    /// \endcode
    /// \param  binder a function object that binds the call to the underlying function pointer
    template <typename FunctionBinder> void triggerEvent(FunctionBinder binder)
    {
        for (auto& callback : m_callbacks) {
            if (callback.isCCallback()) {
                binder(callback.userData(), callback.callback());
            } else if (callback.isPythonCallback()) {
                if (!m_system->executePython(callback.callbackText())) {
                    m_system->error(
                        "The python callback of event name \"%s\" and tag \"%s\" failed to execute "
                        "correctly",
                        m_name.c_str(),
                        callback.tag().c_str());
                }
            } else {
                if (!m_system->executeMEL(callback.callbackText())) {
                    m_system->error(
                        "The MEL callback of event name \"%s\" and tag \"%s\" failed to execute "
                        "correctly",
                        m_name.c_str(),
                        callback.tag().c_str());
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
        for (auto& callback : m_callbacks) {
            if (callback.isCCallback()) {
                defaultEventFunction basic = (defaultEventFunction)callback.callback();
                basic(callback.userData());
            } else if (callback.isPythonCallback()) {
                if (!m_system->executePython(callback.callbackText())) {
                    m_system->error(
                        "The python callback of event name \"%s\" and tag \"%s\" failed to execute "
                        "correctly",
                        m_name.c_str(),
                        callback.tag().c_str());
                }
            } else {
                if (!m_system->executeMEL(callback.callbackText())) {
                    m_system->error(
                        "The MEL callback of event name \"%s\" and tag \"%s\" failed to execute "
                        "correctly",
                        m_name.c_str(),
                        callback.tag().c_str());
                }
            }
        }
    }

    /// \brief  used to sort the events based on their ID
    /// \param  eventId the event id to compare to
    /// \return true if the event ID of this event  is lower than the comparison event
    bool operator<(const EventId eventId) const { return m_eventId < eventId; }

    /// \brief  used to sort the events based on their ID
    /// \param  eventId the event id to compare to
    /// \param  dispatcher the event dispatcher to compare the event id too
    /// \return true if the event ID is lower than the event id within the comparison event
    friend bool operator<(const EventId eventId, const EventDispatcher& dispatcher)
    {
        return eventId < dispatcher.m_eventId;
    }

    /// \brief  returns the data pointer associated with this event
    /// \return returns the data pointer associated with this event
    const void* associatedData() const { return m_associatedData; }

    /// \brief  utility function to locate a specific callback. If found, a pointer to the callback
    /// data will be returned.
    ///         If not found then nullptr is returned
    /// \param  id the id
    Callback* findCallback(CallbackId id)
    {
        for (size_t i = 0, n = m_callbacks.size(); i < n; ++i) {
            auto& cb = m_callbacks[i];
            if (cb.callbackId() == id) {
                return &m_callbacks[i];
            }
        }
        return 0;
    }

private:
    AL_EVENT_PUBLIC
    CallbackId registerCallbackInternal(
        const char* const tag,
        const void*       functionPointer,
        uint32_t          weight,
        void*             userData);
    AL_EVENT_PUBLIC
    Callback buildCallbackInternal(
        const char* const tag,
        const void*       functionPointer,
        uint32_t          weight,
        void*             userData);

private:
    EventSystemBinding* m_system;
    std::string         m_name;
    Callbacks           m_callbacks;
    const void*         m_associatedData;
    CallbackId          m_parentCallback;
    EventId             m_eventId;
    EventType           m_eventType;
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
    /// \brief  initialises the default event scheduler.
    /// \param  system the binding to the back end DCC services
    AL_EVENT_PUBLIC
    static void initScheduler(EventSystemBinding* system);

    /// \brief  returns the default scheduler
    /// \return the default scheduler
    AL_EVENT_PUBLIC
    static EventScheduler& getScheduler();

    /// \brief  destroys the internal default scheduler
    AL_EVENT_PUBLIC
    static void freeScheduler();

    /// \brief  ctor
    /// \param  system The object that provides a binding to the underlying DCC system utilities
    EventScheduler(EventSystemBinding* system)
        : m_system(system)
        , m_registeredEvents()
    {
    }

    /// \brief  dtor
    AL_EVENT_PUBLIC
    ~EventScheduler();

    /// \brief  returns the event type as a string
    /// \param  eventType the type of event to query the name of
    /// \return a text string name for the specified event type
    const char* eventTypeString(EventType eventType) const
    {
        return m_system->eventTypeString(eventType);
    }

    /// \brief  returns the total number of event types in use
    /// \return the number of event types
    size_t numberOfEventTypes() const { return m_system->numberOfEventTypes(); }

    /// \brief  register a new event
    /// \param  eventName the name of event to register
    /// \param  eventType the type of event being registered (e.g. maya, usdmaya, custom, etc)
    /// \param  associatedData an associated data pointer for this event [optional]
    /// \param  parentCallback if this event is triggered by a callback
    AL_EVENT_PUBLIC
    EventId registerEvent(
        const char*      eventName,
        EventType        eventType,
        const void*      associatedData = 0,
        const CallbackId parentCallback = 0);

    /// \brief  unregister an event handler
    /// \param  eventId the event to unregister
    /// \return true if the event is a valid id
    AL_EVENT_PUBLIC
    bool unregisterEvent(EventId eventId);

    /// \brief  unregister an event handler
    /// \param  eventId the event to unregister
    /// \return true if the event is a valid id
    AL_EVENT_PUBLIC
    bool unregisterEvent(const char* const eventId);

    /// \brief  returns a pointer to the requested event id
    /// \param  eventId the event id
    /// \return the registered callbacks for the specified event
    AL_EVENT_PUBLIC
    EventDispatcher* event(EventId eventId);

    /// \brief  returns a pointer to the requested event id
    /// \param  eventId the event id
    /// \return the registered callbacks for the specified event
    AL_EVENT_PUBLIC
    const EventDispatcher* event(EventId eventId) const;

    /// \brief  returns a pointer to the requested event id
    /// \param  eventName the name of the event
    /// \return the registered callbacks for the specified event
    AL_EVENT_PUBLIC
    EventDispatcher* event(const char* eventName);

    /// \brief  returns a pointer to the requested event id
    /// \param  eventName the name of the event
    /// \return the registered callbacks for the specified event
    AL_EVENT_PUBLIC
    const EventDispatcher* event(const char* eventName) const;

    /// \brief  dispatches an event using a function binder
    /// \param  eventId the event to dispatch
    /// \param  binder the binder to dispatch the event
    /// \return true if the event is valid
    template <typename FunctionBinder> bool triggerEvent(EventId eventId, FunctionBinder binder)
    {
        EventDispatcher* e = event(eventId);
        if (e) {
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
        if (e) {
            e->triggerEvent();
            return true;
        }
        return false;
    }

    /// \brief  dispatches an event using the standard void (*func)(void* userData) signature
    /// \param  eventName the name of the event to dispatch
    /// \return true if the event is valid
    bool triggerEvent(const char* const eventName)
    {
        EventDispatcher* e = event(eventName);
        if (e) {
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
    template <typename func_type>
    CallbackId registerCallback(
        const EventId     eventId,
        const char* const tag,
        const func_type   functionPointer,
        uint32_t          weight,
        void*             userData = 0)
    {
        EventDispatcher* eventInfo = event(eventId);
        if (eventInfo) {
            auto cb = eventInfo->registerCallback(tag, functionPointer, weight, userData);
            if (cb) {
                auto et = extractEventType(cb);
                auto handler = m_customHandlers.find(et);
                if (handler != m_customHandlers.end()) {
                    handler->second->onCallbackCreated(cb);
                }
            }
            return cb;
        }
        return InvalidCallbackId;
    }

    /// \brief  register a new event callback (in python or MEL)
    /// \param  eventId the event id
    /// \param  tag the tag for the callback
    /// \param  commandText
    /// \param  weight the weight for the callback (determines the ordering)
    /// \param  isPython true if the callback is python, false if MEL
    /// \return the callback id for the newly registered callback
    CallbackId registerCallback(
        const EventId     eventId,
        const char* const tag,
        const char* const commandText,
        uint32_t          weight,
        bool              isPython)
    {
        EventDispatcher* eventInfo = event(eventId);
        if (eventInfo) {
            auto cb = eventInfo->registerCallback(tag, commandText, weight, isPython);
            if (cb) {
                auto et = extractEventType(cb);
                auto handler = m_customHandlers.find(et);
                if (handler != m_customHandlers.end()) {
                    handler->second->onCallbackCreated(cb);
                }
            }
            return cb;
        }
        return InvalidCallbackId;
    }

    /// \brief  register a new event callback
    /// \param  eventId the event id
    /// \param  tag the tag for the callback
    /// \param  functionPointer the function pointer to call
    /// \param  weight the weight for the callback (determines the ordering)
    /// \param  userData associated user data for the callback
    /// \return the callback id for the newly registered callback
    template <typename func_type>
    Callback buildCallback(
        const EventId     eventId,
        const char* const tag,
        const func_type   functionPointer,
        uint32_t          weight,
        void*             userData = 0)
    {
        EventDispatcher* eventInfo = event(eventId);
        if (eventInfo) {
            return eventInfo->buildCallback(tag, functionPointer, weight, userData);
        }
        return Callback();
    }

    /// \brief  register a new event callback (in python or MEL)
    /// \param  eventId the event id
    /// \param  tag the tag for the callback
    /// \param  commandText the code to execute when the callback is triggered
    /// \param  weight the weight for the callback (determines the ordering)
    /// \param  isPython true if the callback is python, false if MEL
    /// \return the callback id for the newly registered callback
    Callback buildCallback(
        const EventId     eventId,
        const char* const tag,
        const char* const commandText,
        uint32_t          weight,
        bool              isPython)
    {
        EventDispatcher* eventInfo = event(eventId);
        if (eventInfo) {
            return eventInfo->buildCallback(tag, commandText, weight, isPython);
        }
        return Callback();
    }

    /// \brief  register a new event callback
    /// \param  eventName the name of the even
    /// \param  tag the tag for the callback
    /// \param  functionPointer the function pointer to call
    /// \param  weight the weight for the callback (determines the ordering)
    /// \param  userData associated user data for the callback
    /// \return the callback id for the newly registered callback
    template <typename func_type>
    Callback buildCallback(
        const char* const eventName,
        const char* const tag,
        const func_type   functionPointer,
        uint32_t          weight,
        void*             userData = 0)
    {
        EventDispatcher* eventInfo = event(eventName);
        if (eventInfo) {
            return eventInfo->buildCallback(tag, functionPointer, weight, userData);
        }

        // register an empty event handler so that we can catch any missing events
        registerEvent(eventName, kUnknownEventType);
        eventInfo = event(eventName);
        return eventInfo->buildCallback(tag, functionPointer, weight, userData);
    }

    /// \brief  register a new event callback (in python or MEL)
    /// \param  eventName the name of the even
    /// \param  tag the tag for the callback
    /// \param  commandText the code to execute when the callback is triggered
    /// \param  weight the weight for the callback (determines the ordering)
    /// \param  isPython true if the callback is python, false if MEL
    /// \return the callback id for the newly registered callback
    Callback buildCallback(
        const char* const eventName,
        const char* const tag,
        const char* const commandText,
        uint32_t          weight,
        bool              isPython)
    {
        EventDispatcher* eventInfo = event(eventName);
        if (eventInfo) {
            return eventInfo->buildCallback(tag, commandText, weight, isPython);
        }
        // register an empty event handler so that we can catch any missing events
        registerEvent(eventName, kUnknownEventType);
        eventInfo = event(eventName);
        return eventInfo->buildCallback(tag, commandText, weight, isPython);
    }

    /// \brief  unregister an event handler
    /// \param  callbackId the id of the callback to destroy
    /// \return true if the callback is removed, false if the id was invalid
    AL_EVENT_PUBLIC
    bool unregisterCallback(CallbackId callbackId);

    /// \brief  unregister an event handler, and move the callback information into the info
    /// structure provided. \param  callbackId the id of the callback to destroy \param  info a
    /// structure that will store the data removed from the event scheduler. \return true if the
    /// callback is removed, false if the id was invalid. If false is removed, no modifications to
    /// info would have occurred.
    AL_EVENT_PUBLIC
    bool unregisterCallback(CallbackId callbackId, Callback& info);

    /// \brief  Move the callback information from the info structure into the event system. Once
    /// this function has been called,
    ///         the contents of info will be null.
    /// \param  info the data to move into the event scheduler
    /// \return the callback id
    CallbackId registerCallback(Callback& info)
    {
        EventDispatcher* eventInfo = event(info.eventId());
        if (eventInfo) {
            CallbackId id = info.callbackId();
            eventInfo->registerCallback(info);
            if (id) {
                auto et = extractEventType(id);
                auto handler = m_customHandlers.find(et);
                if (handler != m_customHandlers.end()) {
                    handler->second->onCallbackCreated(id);
                }
            }
            return id;
        }
        return InvalidCallbackId;
    }

    /// \brief  provides internal access to the registered events
    /// \return the registered events
    const EventDispatchers& registeredEvents() const { return m_registeredEvents; }

    /// \brief  find the callback structure for the specified ID
    /// \param  callbackId the id of the callback to locate
    /// \return a pointer to the callback information, or nullptr if not found
    AL_EVENT_PUBLIC
    Callback* findCallback(CallbackId callbackId);

    /// \brief  A method that allows you to register a custom event handler. This handler can then
    /// be used to bind
    ///         in additional messages from 3rd party systems (e.g. MMessage events, or Usd
    ///         notifications)
    /// \param  type the type of event this handler processes
    /// \param  handler the custom handler object that will process these event types
    void registerHandler(EventType type, CustomEventHandler* handler)
    {
        m_customHandlers[type] = handler;
    }

private:
    EventSystemBinding*                                m_system;
    EventDispatchers                                   m_registeredEvents;
    std::unordered_map<EventType, CustomEventHandler*> m_customHandlers;
};

class NodeEvents;

/// the default node event callback type
typedef void (*node_dispatch_func)(void* userData, NodeEvents* node);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  defines an interface that can be applied to custom nodes to allow them the ability to
/// manage and dispatch
///         internal events.
/// \ingroup events
//----------------------------------------------------------------------------------------------------------------------
class NodeEvents
{
    typedef std::unordered_map<std::string, EventId> EventMap;

public:
    /// \brief  ctor
    /// \param  scheduler the event scheduler
    AL_EVENT_PUBLIC
    NodeEvents(EventScheduler* const scheduler = &EventScheduler::getScheduler())
        : m_scheduler(scheduler)
    {
    }

    /// \brief  trigger the event of the given name
    /// \param  eventName the name of the event to trigger on this node
    /// \return true if the events triggered correctly
    bool triggerEvent(const char* const eventName)
    {
        auto it = m_events.find(eventName);
        if (it != m_events.end()) {
            return m_scheduler->triggerEvent(
                it->second, [this](void* userData, const void* callback) {
                    ((node_dispatch_func)callback)(userData, this);
                });
        }
        return false;
    }

    /// \brief  dtor
    virtual ~NodeEvents()
    {
        for (auto event : m_events) {
            m_scheduler->unregisterEvent(event.second);
        }
    }

    /// \brief  returns the event scheduler associated with this node events structure
    /// \return the associated event scheduler.
    EventScheduler* scheduler() const { return m_scheduler; }

    /// \brief  returns the event ID for the specified event name
    /// \param  eventName the name of the event
    /// \return the ID of the event (or zero)
    EventId getId(const char* const eventName) const
    {
        const auto it = m_events.find(eventName);
        if (it != m_events.end()) {
            return it->second;
        }
        return InvalidEventId;
    }

    /// \brief  returns the internal event map
    /// \return the event map
    const EventMap& events() const { return m_events; }

    /// \brief  registers an event on this node
    /// \param  eventName the name of the event to register
    /// \param  eventType the type of event to register
    /// \param  parentId the callback id of the callback that triggers this event (or null)
    /// \return true if the event could be registered
    bool registerEvent(
        const char* const eventName,
        const EventType   eventType,
        const CallbackId  parentId = 0)
    {
        EventId id = m_scheduler->registerEvent(eventName, eventType, this, parentId);
        if (id) {
            m_events.emplace(eventName, id);
        }
        return id != InvalidEventId;
    }

    /// \brief  unregisters an event from this node
    /// \param  eventName the name of the event to remove
    /// \return true if the event could be removed
    bool unregisterEvent(const char* const eventName)
    {
        auto it = m_events.find(eventName);
        if (it != m_events.end()) {
            EventId eId = it->second;
            m_events.erase(it);
            return m_scheduler->unregisterEvent(eId);
        }
        return false;
    }

private:
    EventMap        m_events;
    EventScheduler* m_scheduler;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace event
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
