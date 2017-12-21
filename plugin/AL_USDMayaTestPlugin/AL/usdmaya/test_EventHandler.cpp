
#include "AL/usdmaya/EventHandler.h"
#include <gtest/gtest.h>
using namespace AL;
using namespace AL::usdmaya;

//----------------------------------------------------------------------------------------------------------------------
// EventInfo(const char* const tag, void* functionPointer, uint32_t weight,  void* userData, CallbackId eventId);
// EventInfo(const char* const tag, const char* const commandText, uint32_t weight, bool isPython, CallbackId eventId);
// ~EventInfo();
// inline bool operator < (const EventInfo& info) const;
//  CallbackId callbackId() const;
//  EventId eventId() const;
//  const std::string& tag() const;

static void func(void* userdata)
{
}

TEST(Callback, Callback)
{
  // test C function initialisation
  int value;
  Callback info1("tag", func, 1000, &value, makeCallbackId(1, 3));
  Callback info2("tag", func, 1001, &value, makeCallbackId(2, 4));

  EXPECT_EQ(info1.tag(), "tag");
  EXPECT_EQ(info1.callbackId(),  makeCallbackId(1, 3));
  EXPECT_EQ(info1.eventId(), 1);
  EXPECT_TRUE(info1 < info2);
  EXPECT_FALSE(info2 < info1);
  EXPECT_TRUE(info1.userData() == &value);
  EXPECT_TRUE(info1.callback() == func);
  EXPECT_TRUE(info1.isCCallback());
  EXPECT_FALSE(info1.isMELCallback());
  EXPECT_FALSE(info1.isPythonCallback());
  EXPECT_EQ(info1.weight(), 1000);

  // test python command
  Callback info3("tag", "i am a command" , 1000, true, makeCallbackId(1, 3));

  EXPECT_EQ(info3.tag(), "tag");
  EXPECT_EQ(info3.callbackId(), (1ULL << 48 | 3));
  EXPECT_EQ(info3.eventId(), 1);
  EXPECT_TRUE(info3.userData() == nullptr);
  EXPECT_EQ(strcmp(info3.callbackText(), "i am a command"), 0);
  EXPECT_FALSE(info3.isCCallback());
  EXPECT_FALSE(info3.isMELCallback());
  EXPECT_TRUE(info3.isPythonCallback());
  EXPECT_EQ(info3.weight(), 1000);

  // test MEL command
  Callback info4("tag", "i am a command" , 1000, false, makeCallbackId(1, 3));
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
//  CallbackId registerEvent(const char* const tag, void* functionPointer, uint32_t weight, void* userData);
//  CallbackId registerEvent(const char* const tag, const char* const commandText, uint32_t weight, bool isPython);
//  EventId eventId() const;
TEST(EventDispatcher, EventDispatcher)
{
  int associated;
  EventDispatcher info("eventName", 42, &associated, 23);
  EXPECT_EQ(info.name(), "eventName");
  EXPECT_EQ(info.eventId(), 42);
  EXPECT_EQ(info.parentEventId(), 23);
  EXPECT_EQ(info.associatedData(), &associated);


  int value;
  CallbackId id1 = info.registerCallback("tag", func, 1001, &value);

  {
    uint16_t eventPart = extractEventId(id1);
    EXPECT_EQ(eventPart, 42);
    uint64_t callbackPart = extractCallbackId(id1);

    EXPECT_EQ(callbackPart, 1);

    ASSERT_FALSE(info.callbacks().empty());
    const Callback& callback = info.callbacks()[0];

    EXPECT_EQ(callback.callback(), ((const void*)func));
    EXPECT_EQ(callback.callbackId(), id1);
    EXPECT_TRUE(callback.tag() == "tag");
    EXPECT_TRUE(callback.userData() == &value);
    EXPECT_TRUE(callback.isCCallback());
    EXPECT_FALSE(callback.isMELCallback());
    EXPECT_FALSE(callback.isPythonCallback());
    EXPECT_EQ(callback.weight(), 1001);
  }

  CallbackId id2 = info.registerCallback("tag2", "i am a command", 1003, false);

  {
    uint16_t eventPart = extractEventId(id2);
    uint64_t callbackPart = extractCallbackId(id2);

    EXPECT_EQ(callbackPart, 2);

    ASSERT_TRUE(info.callbacks().size() == 2);
    const Callback& callback = info.callbacks()[1];

    EXPECT_EQ(callback.callbackId(), id2);
    EXPECT_TRUE(callback.tag() == "tag2");
    EXPECT_TRUE(callback.userData() == nullptr);
    EXPECT_TRUE(strcmp(callback.callbackText(), "i am a command") == 0);
    EXPECT_FALSE(callback.isCCallback());
    EXPECT_TRUE(callback.isMELCallback());
    EXPECT_FALSE(callback.isPythonCallback());
    EXPECT_EQ(callback.weight(), 1003);
  }

  CallbackId id3 = info.registerCallback("tag3", "i am a command", 1002, true);

  {
    uint16_t eventPart = extractEventId(id3);
    uint64_t callbackPart = extractCallbackId(id3);

    EXPECT_EQ(callbackPart, 3);

    ASSERT_TRUE(info.callbacks().size() == 3);
    const Callback& callback = info.callbacks()[1];

    EXPECT_EQ(callback.callbackId(), id3);
    EXPECT_TRUE(callback.tag() == "tag3");
    EXPECT_TRUE(callback.userData() == nullptr);
    EXPECT_TRUE(strcmp(callback.callbackText(), "i am a command") == 0);
    EXPECT_FALSE(callback.isCCallback());
    EXPECT_FALSE(callback.isMELCallback());
    EXPECT_TRUE(callback.isPythonCallback());
    EXPECT_EQ(callback.weight(), 1002);
  }

  EventDispatcher info2(std::move(info));
  EXPECT_TRUE(info2.name() == "eventName");
  EXPECT_TRUE(info2.callbacks().size() == 3);
  EXPECT_EQ(info2.associatedData(), &associated);
  EXPECT_TRUE(info.name().empty());
  EXPECT_TRUE(info.callbacks().empty());

  info = std::move(info2);
  EXPECT_EQ(info.associatedData(), &associated);
  EXPECT_TRUE(info.name() == "eventName");
  EXPECT_TRUE(info.callbacks().size() == 3);
  EXPECT_TRUE(info2.name().empty());
  EXPECT_TRUE(info2.callbacks().empty());

  // dont unregister an invalid event
  EXPECT_FALSE(info.unregisterCallback(488));

  EXPECT_TRUE(info.unregisterCallback(id1));
  ASSERT_TRUE(info.callbacks().size() == 2);
  ASSERT_TRUE(info.callbacks()[0].callbackId() == id3);
  ASSERT_TRUE(info.callbacks()[1].callbackId() == id2);

  EXPECT_TRUE(info.unregisterCallback(id2));
  ASSERT_TRUE(info.callbacks().size() == 1);
  ASSERT_TRUE(info.callbacks()[0].callbackId() == id3);

  EXPECT_TRUE(info.unregisterCallback(id3));
  ASSERT_TRUE(info.callbacks().empty());
}

// void dispatchEvent()

//----------------------------------------------------------------------------------------------------------------------
static void* g_userData = 0;
static void func_dispatch1(void* userData)
{
  g_userData = userData;
}

TEST(EventDispatcher, triggerEvent1)
{
  g_userData = 0;
  EventDispatcher info("eventName", 42, nullptr, 23);

  int value;
  CallbackId id1 = info.registerCallback("tag", func_dispatch1, 1000, &value);

  // dispatch the event
  info.triggerEvent();

  EXPECT_EQ(g_userData, &value);

  EXPECT_TRUE(info.unregisterCallback(id1));
}

//----------------------------------------------------------------------------------------------------------------------
static int g_value = 0;
static void func_dispatch2(void* userData, int value)
{
  g_userData = userData;
  g_value = value;
}
typedef void (*func_ptr_type)(void*, int);

TEST(EventDispatcher, triggerEvent2)
{
  g_userData = 0;
  EventDispatcher info("eventName", 42, nullptr, 23);

  int value;
  CallbackId id1 = info.registerCallback("tag", func_dispatch2, 1000, &value);

  struct Dispatcher {
    void operator () (void* userData, const void* callback)
    {
      func_ptr_type ptr = (func_ptr_type) callback;
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
// EventId registerEvent(const char* eventName, const void* associatedData = 0, const CallbackId parentEvent = 0);
// bool unregisterEvent(EventId eventId);
TEST(EventScheduler, registerEvent)
{
  EventScheduler registrar;
  int associated;
  EventId id1 = registrar.registerEvent("eventName", &associated, 0);
  EXPECT_TRUE(id1 != 0);
  auto eventInfo = registrar.event(id1);
  EXPECT_TRUE(eventInfo != nullptr);
  EXPECT_EQ(eventInfo->eventId(), 1);
  EXPECT_EQ(eventInfo->parentEventId(), 0);
  EXPECT_EQ(eventInfo->associatedData(), &associated);

  // This should fail to register a new event (since the name is not unique)
  EventId id2 = registrar.registerEvent("eventName", &associated, 0);
  EXPECT_EQ(id2, 0);

  // We should be able to register a new event (since the associated data is different)
  int associated2;
  EventId id3 = registrar.registerEvent("eventName", &associated2, 0);
  EXPECT_TRUE(id3 != 0);
  eventInfo = registrar.event(id3);
  EXPECT_TRUE(eventInfo != nullptr);
  EXPECT_EQ(eventInfo->eventId(), 2);
  EXPECT_EQ(eventInfo->parentEventId(), 0);
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
  EventScheduler registrar;
  int associated;
  EventId id1 = registrar.registerEvent("EventType1", &associated, 0);
  EXPECT_TRUE(id1 != 0);
  auto parentEventInfo = registrar.event(id1);
  EXPECT_TRUE(parentEventInfo != nullptr);
  EXPECT_EQ(parentEventInfo->eventId(), 1);
  EXPECT_EQ(parentEventInfo->parentEventId(), 0);
  EXPECT_EQ(parentEventInfo->associatedData(), &associated);

  int value;
  CallbackId callbackId = parentEventInfo->registerCallback("ChildCallback", func_dispatch2, 1000, &value);

  EventId id2 = registrar.registerEvent("EventType2", &associated, callbackId);
  EXPECT_TRUE(id2 != 0);
  auto eventInfo = registrar.event(id2);
  EXPECT_TRUE(eventInfo != nullptr);
  EXPECT_EQ(eventInfo->eventId(), 2);
  EXPECT_EQ(eventInfo->parentEventId(), callbackId);
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
// CallbackId registerCallback(const EventId eventId, const char* const tag, const func_type functionPointer, uint32_t weight, void* userData = 0)
// CallbackId registerCallback(const EventId eventId, const char* const tag, const char* const commandText, uint32_t weight, bool isPython)
// bool unregisterCallback(CallbackId callbackId);
//
TEST(EventScheduler, registerCallback)
{
  EventScheduler registrar;
  int associated;
  EventId id1 = registrar.registerEvent("EventType1", &associated, 0);
  EXPECT_TRUE(id1 != 0);
  auto parentEventInfo = registrar.event(id1);
  EXPECT_TRUE(parentEventInfo != nullptr);
  EXPECT_EQ(parentEventInfo->eventId(), 1);
  EXPECT_EQ(parentEventInfo->parentEventId(), 0);
  EXPECT_EQ(parentEventInfo->associatedData(), &associated);

  int value;
  CallbackId callbackId = registrar.registerCallback(id1, "ChildCallback", func_dispatch2, 1000, &value);

  EventId id2 = registrar.registerEvent("EventType2", &associated, callbackId);
  EXPECT_TRUE(id2 != 0);
  auto eventInfo = registrar.event(id2);
  EXPECT_TRUE(eventInfo != nullptr);
  EXPECT_EQ(eventInfo->eventId(), 2);
  EXPECT_EQ(eventInfo->parentEventId(), callbackId);
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


