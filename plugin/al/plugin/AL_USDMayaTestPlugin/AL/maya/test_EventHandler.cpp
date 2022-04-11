
#include "AL/event/EventHandler.h"

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>

#include <gtest/gtest.h>

using namespace AL;
using namespace AL::event;

//----------------------------------------------------------------------------------------------------------------------
static const char* const eventTypeStrings[]
    = { "unknown", "custom", "schema", "coremaya", "usdmaya" };

//----------------------------------------------------------------------------------------------------------------------
class TestEventSystemBinding : public EventSystemBinding
{
public:
    TestEventSystemBinding()
        : EventSystemBinding(eventTypeStrings, sizeof(eventTypeStrings) / sizeof(const char*))
    {
    }

    enum Type
    {
        kInfo,
        kWarning,
        kError
    };

    bool executePython(const char* const code) override
    {
        return MGlobal::executePythonCommand(code, false, true);
    }

    bool executeMEL(const char* const code) override
    {
        return MGlobal::executeCommand(code, false, true);
    }

    void writeLog(EventSystemBinding::Type severity, const char* const text) override
    {
        switch (severity) {
        case EventSystemBinding::kInfo: MGlobal::displayInfo(text); break;
        case EventSystemBinding::kWarning: MGlobal::displayWarning(text); break;
        case EventSystemBinding::kError: MGlobal::displayError(text); break;
        }
    }
};

static TestEventSystemBinding g_eventSystem;

//----------------------------------------------------------------------------------------------------------------------
// EventInfo(const char* const tag, void* functionPointer, uint32_t weight,  void* userData,
// CallbackId eventId); EventInfo(const char* const tag, const char* const commandText, uint32_t
// weight, bool isPython, CallbackId eventId); ~EventInfo(); inline bool operator < (const
// EventInfo& info) const;
//  CallbackId callbackId() const;
//  EventId eventId() const;
//  const std::string& tag() const;

static void func(void* userdata) { }

TEST(Callback, Callback)
{
    // test C function initialisation
    int      value;
    Callback info1("tag", func, 1000, &value, makeCallbackId(1, 5, 3));
    Callback info2("tag", func, 1001, &value, makeCallbackId(2, 5, 4));

    EXPECT_EQ(info1.tag(), "tag");
    EXPECT_EQ(info1.callbackId(), makeCallbackId(1, 5, 3));
    EXPECT_EQ(info1.eventId(), 1u);
    EXPECT_TRUE(info1 < info2);
    EXPECT_FALSE(info2 < info1);
    EXPECT_TRUE(info1.userData() == &value);
    EXPECT_TRUE(info1.callback() == func);
    EXPECT_TRUE(info1.isCCallback());
    EXPECT_FALSE(info1.isMELCallback());
    EXPECT_FALSE(info1.isPythonCallback());
    EXPECT_EQ(info1.weight(), 1000u);

    // test python command
    Callback info3("tag", "i am a command", 1000, true, makeCallbackId(1, 5, 3));

    EXPECT_EQ(info3.tag(), "tag");
    EXPECT_EQ(info3.callbackId(), makeCallbackId(1, 5, 3));
    EXPECT_EQ(info3.eventId(), 1u);
    EXPECT_TRUE(info3.userData() == nullptr);
    EXPECT_EQ(strcmp(info3.callbackText(), "i am a command"), 0);
    EXPECT_FALSE(info3.isCCallback());
    EXPECT_FALSE(info3.isMELCallback());
    EXPECT_TRUE(info3.isPythonCallback());
    EXPECT_EQ(info3.weight(), 1000u);

    // test MEL command
    Callback info4("tag", "i am a command", 1000, false, makeCallbackId(1, 5, 3));
    EXPECT_FALSE(info4.isCCallback());
    EXPECT_TRUE(info4.isMELCallback());
    EXPECT_FALSE(info4.isPythonCallback());

    Callback info5(std::move(info4));
}

//----------------------------------------------------------------------------------------------------------------------
//  EventRegistrationInfo(const char* const name, EventId eventId, EventId parentEvent = -1);
//  EventRegistrationInfo(EventRegistrationInfo&& rhs);
//  EventRegistrationInfo& operator = (EventRegistrationInfo&& rhs);
//  const std::string& name() const;
//  const EventInfos& callbacks() const;
//  CallbackId registerEvent(const char* const tag, void* functionPointer, uint32_t weight, void*
//  userData); CallbackId registerEvent(const char* const tag, const char* const commandText,
//  uint32_t weight, bool isPython); EventId eventId() const;
TEST(EventDispatcher, EventDispatcher)
{
    int             associated;
    EventDispatcher info(&g_eventSystem, "eventName", 42, kUserSpecifiedEventType, &associated, 23);
    EXPECT_EQ(info.name(), "eventName");
    EXPECT_EQ(info.eventId(), 42u);
    EXPECT_EQ(info.parentCallbackId(), 23u);
    EXPECT_EQ(info.associatedData(), &associated);

    int        value;
    CallbackId id1 = info.registerCallback("tag", func, 1001, &value);

    {
        uint16_t eventPart = extractEventId(id1);
        EXPECT_EQ(eventPart, 42);
        uint64_t callbackPart = extractCallbackId(id1);

        EXPECT_EQ(callbackPart, 1u);

        ASSERT_FALSE(info.callbacks().empty());
        const Callback& callback = info.callbacks()[0];

        EXPECT_EQ(callback.callback(), ((const void*)func));
        EXPECT_EQ(callback.callbackId(), id1);
        EXPECT_TRUE(callback.tag() == "tag");
        EXPECT_TRUE(callback.userData() == &value);
        EXPECT_TRUE(callback.isCCallback());
        EXPECT_FALSE(callback.isMELCallback());
        EXPECT_FALSE(callback.isPythonCallback());
        EXPECT_EQ(callback.weight(), 1001u);
    }

    CallbackId id2 = info.registerCallback("tag2", "i am a command", 1003, false);

    {
        uint64_t callbackPart = extractCallbackId(id2);

        EXPECT_EQ(callbackPart, 2u);

        ASSERT_TRUE(info.callbacks().size() == 2);
        const Callback& callback = info.callbacks()[1];

        EXPECT_EQ(callback.callbackId(), id2);
        EXPECT_TRUE(callback.tag() == "tag2");
        EXPECT_TRUE(callback.userData() == nullptr);
        EXPECT_TRUE(strcmp(callback.callbackText(), "i am a command") == 0);
        EXPECT_FALSE(callback.isCCallback());
        EXPECT_TRUE(callback.isMELCallback());
        EXPECT_FALSE(callback.isPythonCallback());
        EXPECT_EQ(callback.weight(), 1003u);
    }

    CallbackId id3 = info.registerCallback("tag3", "i am a command", 1002, true);

    {
        uint64_t callbackPart = extractCallbackId(id3);

        EXPECT_EQ(callbackPart, 3u);

        ASSERT_TRUE(info.callbacks().size() == 3u);
        const Callback& callback = info.callbacks()[1];

        EXPECT_EQ(callback.callbackId(), id3);
        EXPECT_TRUE(callback.tag() == "tag3");
        EXPECT_TRUE(callback.userData() == nullptr);
        EXPECT_TRUE(strcmp(callback.callbackText(), "i am a command") == 0);
        EXPECT_FALSE(callback.isCCallback());
        EXPECT_FALSE(callback.isMELCallback());
        EXPECT_TRUE(callback.isPythonCallback());
        EXPECT_EQ(callback.weight(), 1002u);
    }

    EventDispatcher info2(std::move(info));
    EXPECT_TRUE(info2.name() == "eventName");
    EXPECT_TRUE(info2.callbacks().size() == 3u);
    EXPECT_EQ(info2.associatedData(), &associated);
    EXPECT_TRUE(info.name().empty());
    EXPECT_TRUE(info.callbacks().empty());

    info = std::move(info2);
    EXPECT_EQ(info.associatedData(), &associated);
    EXPECT_TRUE(info.name() == "eventName");
    EXPECT_TRUE(info.callbacks().size() == 3u);
    EXPECT_TRUE(info2.name().empty());
    EXPECT_TRUE(info2.callbacks().empty());

    // dont unregister an invalid event
    EXPECT_FALSE(info.unregisterCallback(488));

    EXPECT_TRUE(info.unregisterCallback(id1));
    ASSERT_TRUE(info.callbacks().size() == 2u);
    ASSERT_TRUE(info.callbacks()[0].callbackId() == id3);
    ASSERT_TRUE(info.callbacks()[1].callbackId() == id2);

    EXPECT_TRUE(info.unregisterCallback(id2));
    ASSERT_TRUE(info.callbacks().size() == 1u);
    ASSERT_TRUE(info.callbacks()[0].callbackId() == id3);

    EXPECT_TRUE(info.unregisterCallback(id3));
    ASSERT_TRUE(info.callbacks().empty());
}

// void dispatchEvent()

//----------------------------------------------------------------------------------------------------------------------
static void* g_userData = 0;
static void  func_dispatch1(void* userData) { g_userData = userData; }

TEST(EventDispatcher, triggerEvent1)
{
    g_userData = 0;
    EventDispatcher info(&g_eventSystem, "eventName", 42, kUserSpecifiedEventType, nullptr, 23);

    int        value;
    CallbackId id1 = info.registerCallback("tag", func_dispatch1, 1000, &value);

    // dispatch the event
    info.triggerEvent();

    EXPECT_EQ(g_userData, &value);

    EXPECT_TRUE(info.unregisterCallback(id1));
}

//----------------------------------------------------------------------------------------------------------------------
static int  g_value = 0;
static void func_dispatch2(void* userData, int value)
{
    g_userData = userData;
    g_value = value;
}
typedef void (*func_ptr_type)(void*, int);

TEST(EventDispatcher, triggerEvent2)
{
    g_userData = 0;
    EventDispatcher info(&g_eventSystem, "eventName", 42, kUserSpecifiedEventType, nullptr, 23);

    int        value;
    CallbackId id1 = info.registerCallback("tag", func_dispatch2, 1000, &value);

    struct Dispatcher
    {
        void operator()(void* userData, const void* callback)
        {
            func_ptr_type ptr = (func_ptr_type)callback;
            ptr(userData, 42);
        }
    };

    // dispatch the event
    info.triggerEvent(Dispatcher());

    EXPECT_EQ(g_userData, &value);
    EXPECT_EQ(g_value, 42);

    EXPECT_TRUE(info.unregisterCallback(id1));
}

//----------------------------------------------------------------------------------------------------------------------
// EventId registerEvent(const char* eventName, const void* associatedData = 0, const CallbackId
// parentEvent = 0); bool unregisterEvent(EventId eventId);
TEST(EventScheduler, registerEvent)
{
    EventScheduler registrar(&g_eventSystem);
    int            associated=0;
    EventId id1 = registrar.registerEvent("eventName", kUserSpecifiedEventType, &associated, 0);
    EXPECT_TRUE(id1 != 0);
    auto eventInfo = registrar.event(id1);
    EXPECT_TRUE(eventInfo != nullptr);
    EXPECT_EQ(eventInfo->eventId(), 1u);
    EXPECT_EQ(eventInfo->parentCallbackId(), 0u);
    EXPECT_EQ(eventInfo->associatedData(), &associated);

    // This should fail to register a new event (since the name is not unique)
    EventId id2 = registrar.registerEvent("eventName", kUserSpecifiedEventType, &associated, 0);
    EXPECT_EQ(id2, 0u);

    // We should be able to register a new event (since the associated data is different)
    int     associated2;
    EventId id3 = registrar.registerEvent("eventName", kUserSpecifiedEventType, &associated2, 0);
    EXPECT_TRUE(id3 != 0);
    eventInfo = registrar.event(id3);
    EXPECT_TRUE(eventInfo != nullptr);
    EXPECT_EQ(eventInfo->eventId(), 2u);
    EXPECT_EQ(eventInfo->parentCallbackId(), 0u);
    EXPECT_EQ(eventInfo->associatedData(), &associated2);

    EXPECT_TRUE(registrar.unregisterEvent(id1));
    eventInfo = registrar.event(id1);
    EXPECT_TRUE(eventInfo == nullptr);

    EXPECT_TRUE(registrar.unregisterEvent(id3));
    eventInfo = registrar.event(id3);
    EXPECT_TRUE(eventInfo == nullptr);
}

/// We can set up a hierarchy of events, so this test looks for:
///
/// EventType1 -> Register a callback called ChildCallback
/// set up EventType2 as a child event of the ChildCallback
TEST(EventScheduler, registerChildEvent)
{
    EventScheduler registrar(&g_eventSystem);
    int            associated=0;
    EventId id1 = registrar.registerEvent("EventType1", kUserSpecifiedEventType, &associated, 0);
    EXPECT_TRUE(id1 != 0);
    auto parentEventInfo = registrar.event(id1);
    EXPECT_TRUE(parentEventInfo != nullptr);
    EXPECT_EQ(parentEventInfo->eventId(), 1u);
    EXPECT_EQ(parentEventInfo->parentCallbackId(), 0u);
    EXPECT_EQ(parentEventInfo->associatedData(), &associated);

    int        value;
    CallbackId callbackId
        = parentEventInfo->registerCallback("ChildCallback", func_dispatch2, 1000, &value);

    EventId id2
        = registrar.registerEvent("EventType2", kUserSpecifiedEventType, &associated, callbackId);
    EXPECT_TRUE(id2 != 0);
    auto eventInfo = registrar.event(id2);
    EXPECT_TRUE(eventInfo != nullptr);
    EXPECT_EQ(eventInfo->eventId(), 2u);
    EXPECT_EQ(eventInfo->parentCallbackId(), callbackId);
    EXPECT_EQ(eventInfo->associatedData(), &associated);

    EXPECT_TRUE(registrar.unregisterEvent(id2));
    eventInfo = registrar.event(id2);
    EXPECT_TRUE(eventInfo == nullptr);

    parentEventInfo = registrar.event(id1);
    EXPECT_TRUE(parentEventInfo->unregisterCallback(callbackId));

    EXPECT_TRUE(registrar.unregisterEvent(id1));
    parentEventInfo = registrar.event(id1);
    EXPECT_TRUE(parentEventInfo == nullptr);
}

//----------------------------------------------------------------------------------------------------------------------
//
// template<typename func_type>
// CallbackId registerCallback(const EventId eventId, const char* const tag, const func_type
// functionPointer, uint32_t weight, void* userData = 0) CallbackId registerCallback(const EventId
// eventId, const char* const tag, const char* const commandText, uint32_t weight, bool isPython)
// bool unregisterCallback(CallbackId callbackId);
//
TEST(EventScheduler, registerCallback)
{
    EventScheduler registrar(&g_eventSystem);
    int            associated=0;
    EventId id1 = registrar.registerEvent("EventType1", kUserSpecifiedEventType, &associated, 0);
    EXPECT_TRUE(id1 != 0);
    auto parentEventInfo = registrar.event(id1);
    EXPECT_TRUE(parentEventInfo != nullptr);
    EXPECT_EQ(parentEventInfo->eventId(), 1u);
    EXPECT_EQ(parentEventInfo->parentCallbackId(), 0u);
    EXPECT_EQ(parentEventInfo->associatedData(), &associated);

    int        value;
    CallbackId callbackId
        = registrar.registerCallback(id1, "ChildCallback", func_dispatch2, 1000, &value);

    EventId id2
        = registrar.registerEvent("EventType2", kUserSpecifiedEventType, &associated, callbackId);
    EXPECT_TRUE(id2 != 0);
    auto eventInfo = registrar.event(id2);
    EXPECT_TRUE(eventInfo != nullptr);
    EXPECT_EQ(eventInfo->eventId(), 2u);
    EXPECT_EQ(eventInfo->parentCallbackId(), callbackId);
    EXPECT_EQ(eventInfo->associatedData(), &associated);

    EXPECT_TRUE(registrar.unregisterEvent(id2));
    eventInfo = registrar.event(id2);
    EXPECT_TRUE(eventInfo == nullptr);

    EXPECT_TRUE(registrar.unregisterCallback(callbackId));
    parentEventInfo = registrar.event(id1);
    EXPECT_TRUE(parentEventInfo->callbacks().empty());

    EXPECT_TRUE(registrar.unregisterEvent(id1));
    parentEventInfo = registrar.event(id1);
    EXPECT_TRUE(parentEventInfo == nullptr);
}

//----------------------------------------------------------------------------------------------------------------------
//
// template<typename func_type>
// CallbackId registerCallback(const EventId eventId, const char* const tag, const func_type
// functionPointer, uint32_t weight, void* userData = 0) CallbackId registerCallback(const EventId
// eventId, const char* const tag, const char* const commandText, uint32_t weight, bool isPython)
// bool unregisterCallback(CallbackId callbackId);
//
TEST(EventScheduler, registerCallbackAgainstEventThatDoesNotExist)
{
    EventScheduler registrar(&g_eventSystem);

    int      value;
    Callback cb
        = registrar.buildCallback("EventType1", "ChildCallback", func_dispatch2, 1000, &value);
    registrar.registerCallback(cb);

    // we want to be able to register callbacks to events that don't quite exist yet
    EXPECT_TRUE(cb.callbackId() != 0);

    int     associated;
    EventId id1 = registrar.registerEvent("EventType1", kUserSpecifiedEventType, &associated, 0);
    EXPECT_TRUE(id1 != 0);
    auto eventInfo = registrar.event(id1);
    EXPECT_TRUE(eventInfo != nullptr);
    EXPECT_EQ(eventInfo->eventId(), 1u);
    EXPECT_EQ(eventInfo->parentCallbackId(), 0u);
    EXPECT_EQ(eventInfo->associatedData(), &associated);

    CallbackId callbackId
        = registrar.registerCallback(id1, "ChildCallback2", func_dispatch2, 1000, &value);

    EXPECT_TRUE(registrar.unregisterCallback(cb.callbackId()));
    eventInfo = registrar.event(id1);
    EXPECT_EQ(1u, eventInfo->callbacks().size());

    EXPECT_TRUE(registrar.unregisterCallback(callbackId));
    eventInfo = registrar.event(id1);
    EXPECT_TRUE(eventInfo->callbacks().empty());

    EXPECT_TRUE(registrar.unregisterEvent(id1));
    eventInfo = registrar.event(id1);
    EXPECT_TRUE(eventInfo == nullptr);
}

//----------------------------------------------------------------------------------------------------------------------
static const char* const runBasicNodeEventTest = R"(

proc int runBasicNodeEventTest(string $eventName)
{
  file -f -new;

  $proxyTm = `createNode "transform"`;
  $proxy = `createNode -p $proxyTm "AL_usdmaya_ProxyShape"`;

  // generate something we can test to ensure the callback runs
  $tm = `createNode "transform"`;
  $cmd = ("select -r " + $tm + "; move -r 5 0 0;");

  // attach a callback to the proxy shape and check to make sure the expected callback ids are sane
  int $cb[] = `AL_usdmaya_Callback -mne $proxy $eventName "perch" 10000 $cmd`;
  if(size($cb) != 2) return -1;

  {
    // check that we can query the same callback from the proxy
    int $cb2[] = `AL_usdmaya_ListCallbacks $eventName $proxy`;
    if(size($cb2) != 2) return -1;
    for($i = 0; $i < 2; ++$i)
    {
      if($cb[$i] != $cb2[$i])
        return -1;
    }
  }

  // undo the AL_usdmaya_Callback call to make sure the callback is removed
  undo;
  if(size(`AL_usdmaya_ListCallbacks $eventName $proxy`) != 0)
    return -1;

  // redo the AL_usdmaya_Callback call to make sure the same callback is reinserted
  redo;
  if(size(`AL_usdmaya_ListCallbacks $eventName $proxy`) != 2)
    return -1;

  // check to make sure the meta data of the callback is correct
  if(`AL_usdmaya_CallbackQuery -c $cb[0] $cb[1]` != $cmd)
      return -1;

  int $eventId = `AL_usdmaya_EventQuery -e $eventName $proxy`;
  if(`AL_usdmaya_CallbackQuery -e $cb[0] $cb[1]` != $eventId)
      return -1;

  if(`AL_usdmaya_CallbackQuery -w $cb[0] $cb[1]` != 10000)
      return -1;

  if(`AL_usdmaya_CallbackQuery -et $cb[0] $cb[1]` != "perch")
      return -1;

  // attempt to register a callback with the same tag: this should fail!
  $cb2 = `AL_usdmaya_Callback -mne $proxy $eventName "perch" 10000 $cmd`;
  if(size($cb2) != 2) return -1;
  if($cb2[0] != 0 || $cb2[1] != 0) return -1;

  // trigger the event, and see if the translation has changed on the transform
  AL_usdmaya_TriggerEvent -n $proxy $eventName;

  float $pos[] = `getAttr ($tm + ".t")`;
  if($pos[0] != 5.0)
    return -1;

  // delete the callback with the delete callbacks command
  AL_usdmaya_DeleteCallbacks $cb;

  // make sure the callback has been deleted
  {
    int $cb2[] = `AL_usdmaya_ListCallbacks $eventName $proxy`;
    if(size($cb2) != 0)
       return -1;
  }

  // undo the deletion, and make sure the callback has been restored
  undo;
  {
    int $cb2[] = `AL_usdmaya_ListCallbacks $eventName $proxy`;
    if(size($cb2) != 2) return -1;
    for($i = 0; $i < 2; ++$i)
    {
      if($cb[$i] != $cb2[$i])
        return -1;
    }
  }

  // redo the deletion
  redo;
  {
    int $cb2[] = `AL_usdmaya_ListCallbacks $eventName $proxy`;
    if(size($cb2) != 0) return -1;
  }
  undo;

  // delete the callback via the callback command
  AL_usdmaya_Callback -de $cb[0] $cb[1];

  // make sure the callback has been deleted
  {
    int $cb2[] = `AL_usdmaya_ListCallbacks $eventName $proxy`;
    if(size($cb2) != 0)
       return -1;
  }

  // undo the deletion, and make sure the callback has been restored
  undo;
  {
    int $cb2[] = `AL_usdmaya_ListCallbacks $eventName $proxy`;
    if(size($cb2) != 2) return -1;
    for($i = 0; $i < 2; ++$i)
    {
      if($cb[$i] != $cb2[$i])
        return -1;
    }
  }
  // redo the deletion
  redo;
  {
    int $cb2[] = `AL_usdmaya_ListCallbacks $eventName $proxy`;
    if(size($cb2) != 0) return -1;
  }

  // delete the old nodes
  delete $tm;
  delete $proxy;
  delete $proxyTm;

  return 0;
}
runBasicNodeEventTest("PreStageLoaded");

)";

//----------------------------------------------------------------------------------------------------------------------
static const char* const runBasicGlobalEventTest = R"(

proc int runBasicGlobalEventTest()
{
  file -f -new;
  // generate something we can test to ensure the callback runs
  $tm = `createNode "transform"`;
  $cmd = ("select -r " + $tm + "; move -r 5 0 0;");

  // see what events we have before adding a dynamic event
  string $eventsBefore[] = `AL_usdmaya_ListEvents`;

  // the name of our new event
  string $eventName = "BasicGlobalEvent";

  // generate the new event
  AL_usdmaya_Event $eventName;
  int $eventId = `AL_usdmaya_EventQuery -e $eventName`;
  if($eventId == 0)
    return -1;

  // see whether the new event has been registered on the node
  string $eventsAfter[] = `AL_usdmaya_ListEvents`;
  if(size($eventsBefore) == size($eventsAfter))
  {
    return -1;
  }
  {
    $found = false;
    for($s in $eventsAfter)
    {
      if($s == $eventName)
      {
        $found = true;
        break;
      }
    }
    if(!$found)
      return -1;
  }

  // undo previous command, events should be the same as before
  undo;
  $eventsAfter = `AL_usdmaya_ListEvents`;
  if(size($eventsBefore) != size($eventsAfter))
  {
    return -1;
  }

  // redo to make sure it's now there again
  redo;
  $eventsAfter = `AL_usdmaya_ListEvents`;
  if(size($eventsBefore) == size($eventsAfter))
  {
    return -1;
  }

  {
    $found = false;
    for($s in $eventsAfter)
    {
      if($s == $eventName)
      {
        $found = true;
        break;
      }
    }
    if(!$found)
      return -1;
  }

  // assign a callback to it, and make sure it can be triggered
  int $cb[] = `AL_usdmaya_Callback -me $eventName "guppy" 10000 $cmd`;
  if(size($cb) != 2) return -1;

  {
    // check that we can query the same callback from the proxy
    int $cb2[] = `AL_usdmaya_ListCallbacks $eventName`;
    if(size($cb2) != 2) return -1;
    for($i = 0; $i < 2; ++$i)
    {
      if($cb[$i] != $cb2[$i])
        return -1;
    }
  }

  // undo the AL_usdmaya_Callback call to make sure the callback is removed
  undo;
  if(size(`AL_usdmaya_ListCallbacks $eventName`) != 0)
    return -1;

  // redo the AL_usdmaya_Callback call to make sure the same callback is reinserted
  redo;
  if(size(`AL_usdmaya_ListCallbacks $eventName`) != 2)
    return -1;

  // check to make sure the meta data of the callback is correct
  if(`AL_usdmaya_CallbackQuery -c $cb[0] $cb[1]` != $cmd)
      return -1;

  if(`AL_usdmaya_CallbackQuery -e $cb[0] $cb[1]` != $eventId)
      return -1;

  if(`AL_usdmaya_CallbackQuery -w $cb[0] $cb[1]` != 10000)
      return -1;

  if(`AL_usdmaya_CallbackQuery -et $cb[0] $cb[1]` != "guppy")
      return -1;

  // attempt to register a callback with the same tag: this should fail!
  $cb2 = `AL_usdmaya_Callback -me $eventName "guppy" 10000 $cmd`;
  if(size($cb2) != 2) return -1;
  if($cb2[0] != 0 || $cb2[1] != 0) return -1;

  // trigger the event, and see if the translation has changed on the transform
  AL_usdmaya_TriggerEvent $eventName;

  float $pos[] = `getAttr ($tm + ".t")`;
  if($pos[0] != 5.0)
    return -1;

  // delete the callback with the delete callbacks command
  AL_usdmaya_DeleteCallbacks $cb;

  AL_usdmaya_Event -d $eventName;

  // delete the old nodes
  delete $tm;

  return 0;
}
runBasicGlobalEventTest;

)";

//----------------------------------------------------------------------------------------------------------------------
static const char* const runDynamicNodeEventTest = R"(

proc int runDynamicNodeEventTest()
{
  file -f -new;
  $proxyTm = `createNode "transform"`;
  $proxy = `createNode -p $proxyTm "AL_usdmaya_ProxyShape"`;

  // generate something we can test to ensure the callback runs
  $tm = `createNode "transform"`;
  $cmd = ("select -r " + $tm + "; move -r 5 0 0;");

  // see what events we have before adding a dynamic event
  string $eventsBefore[] = `AL_usdmaya_ListEvents $proxy`;

  // the name of our new event
  string $eventName = "DynamicEvent";

  // generate the new event
  AL_usdmaya_Event $eventName $proxy;
  int $eventId = `AL_usdmaya_EventQuery -e $eventName $proxy`;
  if($eventId == 0)
    return -1;


  // see whether the new event has been registered on the node
  string $eventsAfter[] = `AL_usdmaya_ListEvents $proxy`;
  if(size($eventsBefore) == size($eventsAfter))
  {
    return -1;
  }
  {
    $found = false;
    for($s in $eventsAfter)
    {
      if($s == $eventName)
      {
        $found = true;
        break;
      }
    }
    if(!$found)
      return -1;
  }

  // undo previous command, events should be the same as before
  undo;
  $eventsAfter = `AL_usdmaya_ListEvents $proxy`;
  if(size($eventsBefore) != size($eventsAfter))
  {
    return -1;
  }

  // redo to make sure it's now there again
  redo;
  string $eventsAfter[] = `AL_usdmaya_ListEvents $proxy`;
  if(size($eventsBefore) == size($eventsAfter))
  {
    return -1;
  }
  {
    $found = false;
    for($s in $eventsAfter)
    {
      if($s == $eventName)
      {
        $found = true;
        break;
      }
    }
    if(!$found)
      return -1;
  }

  // assign a callback to it, and make sure it can be triggered
  int $cb[] = `AL_usdmaya_Callback -mne $proxy $eventName "tuna" 10000 $cmd`;
  if(size($cb) != 2) return -1;

  {
    // check that we can query the same callback from the proxy
    int $cb2[] = `AL_usdmaya_ListCallbacks $eventName $proxy`;
    if(size($cb2) != 2) return -1;
    for($i = 0; $i < 2; ++$i)
    {
      if($cb[$i] != $cb2[$i])
        return -1;
    }
  }

  // undo the AL_usdmaya_Callback call to make sure the callback is removed
  undo;
  if(size(`AL_usdmaya_ListCallbacks $eventName $proxy`) != 0)
    return -1;

  // redo the AL_usdmaya_Callback call to make sure the same callback is reinserted
  redo;
  if(size(`AL_usdmaya_ListCallbacks $eventName $proxy`) != 2)
    return -1;

  // check to make sure the meta data of the callback is correct
  if(`AL_usdmaya_CallbackQuery -c $cb[0] $cb[1]` != $cmd)
      return -1;

  if(`AL_usdmaya_CallbackQuery -e $cb[0] $cb[1]` != $eventId)
      return -1;

  if(`AL_usdmaya_CallbackQuery -w $cb[0] $cb[1]` != 10000)
      return -1;

  if(`AL_usdmaya_CallbackQuery -et $cb[0] $cb[1]` != "tuna")
      return -1;

  // attempt to register a callback with the same tag: this should fail!
  $cb2 = `AL_usdmaya_Callback -mne $proxy $eventName "tuna" 10000 $cmd`;
  if(size($cb2) != 2) return -1;
  if($cb2[0] != 0 || $cb2[1] != 0) return -1;

  // trigger the event, and see if the translation has changed on the transform
  AL_usdmaya_TriggerEvent -n $proxy $eventName;

  float $pos[] = `getAttr ($tm + ".t")`;
  if($pos[0] != 5.0)
    return -1;

  // delete the callback with the delete callbacks command
  AL_usdmaya_DeleteCallbacks $cb;

  // delete the old nodes
  delete $tm;
  delete $proxy;
  delete $proxyTm;

  return 0;
}
runDynamicNodeEventTest;

)";

//----------------------------------------------------------------------------------------------------------------------
static const char* const runParentNodeCallbackTest = R"(

proc int runParentNodeCallbackTest()
{
  file -f -new;
  $proxyTm = `createNode "transform"`;
  $proxy = `createNode -p $proxyTm "AL_usdmaya_ProxyShape"`;

  // generate something we can test to ensure the callback runs
  $tm = `createNode "transform"`;
  $childCommand = ("select -r " + $tm + "; move -r 5 0 0;");

  // the name of our new event
  string $mainEventName = "DynamicNodeEvent";
  string $childEventName = "ChildNodeEvent";

  // generate a high level event
  AL_usdmaya_Event $mainEventName $proxy;

  // generate a callback command that triggers the child event
  string $parentCommand = "AL_usdmaya_TriggerEvent -n " + $proxy + " " + $childEventName + ";";

  // assign a callback to it, and make sure it can be triggered
  int $parentCB[] = `AL_usdmaya_Callback -mne $proxy $mainEventName "salmon" 10000 $parentCommand`;

  // generate the new event, setting the callback as a
  AL_usdmaya_Event -p $parentCB[0] $parentCB[1] $childEventName $proxy;

  // assign a callback to it, and make sure it can be triggered
  int $childCB[] = `AL_usdmaya_Callback -mne $proxy $childEventName "gurnard" 10000 $childCommand`;

  // make sure the parent CB is correctly reported
  {
    int $cb[] = `AL_usdmaya_EventQuery -p $childEventName $proxy`;
    if(size($cb) != 2 ||
       $cb[0] != $parentCB[0] ||
       $cb[1] != $parentCB[1])
    {
      return -1;
    }
  }

  // trigger the main event, and see if it inturn runs the child callback (via the parent callback)
  AL_usdmaya_TriggerEvent -n $proxy $mainEventName;

  float $pos[] = `getAttr ($tm + ".t")`;
  if($pos[0] != 5.0)
    return -1;

  // delete the callback with the delete callbacks command
  AL_usdmaya_DeleteCallbacks $childCB;
  AL_usdmaya_DeleteCallbacks $parentCB;
  AL_usdmaya_Event -d $childEventName $proxy;
  AL_usdmaya_Event -d $mainEventName $proxy;

  // delete the old nodes
  delete $tm;
  delete $proxy;
  delete $proxyTm;

  return 0;
}
runParentNodeCallbackTest;

)";

//----------------------------------------------------------------------------------------------------------------------
static const char* const runParentGlobalCallbackTest = R"(

proc int runParentGlobalCallbackTest()
{
  file -f -new;
  // generate something we can test to ensure the callback runs
  $tm = `createNode "transform"`;
  $childCommand = ("select -r " + $tm + "; move -r 5 0 0;");

  // the name of our new event
  string $mainEventName = "DynamicGlobalEvent";
  string $childEventName = "ChildGlobalEvent";

  // generate a high level event
  AL_usdmaya_Event $mainEventName;

  // generate a callback command that triggers the child event
  string $parentCommand = "AL_usdmaya_TriggerEvent " + $childEventName + ";";

  // assign a callback to it, and make sure it can be triggered
  int $parentCB[] = `AL_usdmaya_Callback -me $mainEventName "whitebait" 10000 $parentCommand`;

  // generate the new event, setting the callback as a
  AL_usdmaya_Event -p $parentCB[0] $parentCB[1] $childEventName;

  // assign a callback to it, and make sure it can be triggered
  int $childCB[] = `AL_usdmaya_Callback -me $childEventName "carp" 10000 $childCommand`;

  // make sure the parent CB is correctly reported
  {
    int $cb[] = `AL_usdmaya_EventQuery -p $childEventName`;
    if(size($cb) != 2 ||
       $cb[0] != $parentCB[0] ||
       $cb[1] != $parentCB[1])
    {
      return -1;
    }
  }

  // trigger the main event, and see if it inturn runs the child callback (via the parent callback)
  AL_usdmaya_TriggerEvent $mainEventName;

  float $pos[] = `getAttr ($tm + ".t")`;
  if($pos[0] != 5.0)
    return -1;

  // delete the callback with the delete callbacks command
  AL_usdmaya_DeleteCallbacks $childCB;
  AL_usdmaya_DeleteCallbacks $parentCB;
  AL_usdmaya_Event -d $childEventName;
  AL_usdmaya_Event -d $mainEventName;

  // delete the old nodes
  delete $tm;

  return 0;
}
runParentGlobalCallbackTest;

)";

//----------------------------------------------------------------------------------------------------------------------
static const char* const runEventLookupTest = R"(

proc int runEventLookupTest()
{
  $proxyTm = `createNode "transform"`;
  $proxy = `createNode -p $proxyTm "AL_usdmaya_ProxyShape"`;

  // the name of our new event
  string $eventName = "LookupEvent";

  // generate the new event
  AL_usdmaya_Event $eventName $proxy;
  int $eventId = `AL_usdmaya_EventQuery -e $eventName $proxy`;
  if($eventId == 0)
    return -1;

  if(`AL_usdmaya_EventLookup -name $eventId` != $eventName)
    return -1;

  if(`AL_usdmaya_EventLookup -node $eventId` != $proxy)
    return -1;

  delete $proxy;
  delete $proxyTm;

  return 0;
}

runEventLookupTest;

)";

//----------------------------------------------------------------------------------------------------------------------
TEST(EventCommands, runBasicNodeEventTest)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand("undoInfo -st on;");
    int result = -1;
    EXPECT_TRUE(
        MGlobal::executeCommand(runBasicNodeEventTest, result, false, true) == MS::kSuccess);
    EXPECT_TRUE(result == 0);
    MGlobal::executeCommand("undoInfo -st off;");
}

//----------------------------------------------------------------------------------------------------------------------
TEST(EventCommands, runBasicGlobalEventTest)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand("undoInfo -st on;");
    int result = -1;
    EXPECT_TRUE(
        MGlobal::executeCommand(runBasicGlobalEventTest, result, false, true) == MS::kSuccess);
    EXPECT_TRUE(result == 0);
    MGlobal::executeCommand("undoInfo -st off;");
}

//----------------------------------------------------------------------------------------------------------------------
TEST(EventCommands, runDynamicNodeEventTest)
{
    MFileIO::newFile(true);
    MGlobal::executeCommand("undoInfo -st on;");
    int result = -1;
    EXPECT_TRUE(
        MGlobal::executeCommand(runDynamicNodeEventTest, result, false, true) == MS::kSuccess);
    EXPECT_TRUE(result == 0);
    MGlobal::executeCommand("undoInfo -st off;");
}

//----------------------------------------------------------------------------------------------------------------------
TEST(EventCommands, runParentNodeCallbackTest)
{
    MGlobal::executeCommand("undoInfo -st on;");
    int result = -1;
    EXPECT_TRUE(
        MGlobal::executeCommand(runParentNodeCallbackTest, result, false, true) == MS::kSuccess);
    EXPECT_TRUE(result == 0);
    MGlobal::executeCommand("undoInfo -st off;");
}

//----------------------------------------------------------------------------------------------------------------------
TEST(EventCommands, runParentGlobalCallbackTest)
{
    MGlobal::executeCommand("undoInfo -st on;");
    int result = -1;
    EXPECT_TRUE(
        MGlobal::executeCommand(runParentGlobalCallbackTest, result, false, true) == MS::kSuccess);
    EXPECT_TRUE(result == 0);
    MGlobal::executeCommand("undoInfo -st off;");
}

//----------------------------------------------------------------------------------------------------------------------
TEST(EventCommands, runEventLookupTest)
{
    MGlobal::executeCommand("undoInfo -st on;");
    int result = -1;
    EXPECT_TRUE(MGlobal::executeCommand(runEventLookupTest, result, false, true) == MS::kSuccess);
    EXPECT_TRUE(result == 0);
    MGlobal::executeCommand("undoInfo -st off;");
}
