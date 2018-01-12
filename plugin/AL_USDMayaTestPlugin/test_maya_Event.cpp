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
#include "test_usdmaya.h"

#include "AL/events/EventManager.h"
#include "maya/MFileIO.h"

#if 0

using namespace AL::usdmaya;

static bool g_called = 0;
static void callback_test(void*){ g_called = true; };

static int g_orderTest = 0;
static bool g_orderFailed = false;
static void callback_order_test(void* userData)
{
  if(*(const int*)userData != g_orderTest)
    g_orderFailed = true;
  ++g_orderTest;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test that the Register is working correctly
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, registerEvent)
{
  int userData = 0;

  MFileIO::newFile(true);

  MayaEventManager& ev = MayaEventManager::instance();
  auto callback = ev.registerCallback(
    callback_test,
    "AfterNew",
    "I'm a tag",
    1234,
    &userData);

  const MayaCallbackIDContainer& mayaIDs = ev.mayaCallbackIDs();
  auto& listeners = ev.listeners();
  EXPECT_TRUE(ev.isMayaCallbackRegistered(MayaEventType::kAfterNew));
  auto& callbacks = listeners[size_t(MayaEventType::kAfterNew)];
  ASSERT_EQ(callbacks.size(), 1);
  EXPECT_EQ(&userData, callbacks[0].userData);
  EXPECT_EQ(callback_test, callbacks[0].callback);
  EXPECT_EQ(MString(""), callbacks[0].command);
  EXPECT_EQ(MString("I'm a tag"), callbacks[0].tag);
  EXPECT_EQ(callback, callbacks[0].id);
  EXPECT_EQ(1234, callbacks[0].weight);
  EXPECT_EQ(0, callbacks[0].isPython);

  MFileIO::newFile(true);

  EXPECT_TRUE(g_called);
  g_called = false;

  ev.unregisterCallback(callback);
}


//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test registering invalid types doesn't crash or register a listener
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, invalidRegisteredEvent)
{
  MFileIO::newFile(true);
  MayaEventManager& ev = MayaEventManager::instance();
  MayaEventType testEvent = MayaEventType::kSceneMessageLast; // Invalid event

  auto id = ev.registerCallback(
    testEvent,
    callback_test,
    "I'm a tag",
    1234,
    0);

  EXPECT_EQ(id, 0);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test that a simple Deregister is working correctly
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, simpleUnregisterEvent)
{
  MFileIO::newFile(true);
  MayaEventManager& ev = MayaEventManager::instance();
  auto& listeners = ev.listeners();
  MayaEventType testEvent = MayaEventType::kAfterNew;
  auto id = ev.registerCallback(
    testEvent,
    callback_test,
    "I'm a tag",
    1234,
    0);


  EXPECT_TRUE(ev.isMayaCallbackRegistered(testEvent));
  EXPECT_EQ(listeners[size_t(testEvent)].size(), 1);

  EXPECT_TRUE(ev.unregisterCallback(id));
  EXPECT_FALSE(ev.isMayaCallbackRegistered(testEvent));
  EXPECT_EQ(listeners[size_t(testEvent)].size(), 0);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test deregistering invalid types doesn't crash or cause any side effects
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, invalidDeregisteredEvent)
{
  MFileIO::newFile(true);
  MayaEventManager& ev = MayaEventManager::instance();
  auto invalidEventType = MayaEventManager::makeEventId(MayaEventType::kSceneMessageLast, 22);
  auto invalidCallbackId = MayaEventManager::makeEventId(MayaEventType::kAfterOpen, 22);
  EXPECT_FALSE(ev.unregisterCallback(invalidEventType));
  EXPECT_FALSE(ev.unregisterCallback(invalidCallbackId));
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test that the event ordering is working correctly
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, eventOrdering)
{
  MFileIO::newFile(true);
  MayaEventManager& ev = MayaEventManager::instance();
  MayaEventType testEvent = MayaEventType::kAfterNew;

  int firstInt = 0;
  int secondInt = 1;
  int thirdInt = 2;

  auto middleCallback = ev.registerCallback(
    testEvent,
    callback_order_test,
    "middle",
    22,
    &secondInt);

  auto lastCallback = ev.registerCallback(
    testEvent,
    callback_order_test,
    "last",
    33,
    &thirdInt);

  auto firstCallback = ev.registerCallback(
    testEvent,
    callback_order_test,
    "first",
    11,
    &firstInt);

  auto& listeners = ev.listeners();
  EXPECT_EQ(listeners[size_t(testEvent)].size(), 3);

  // check they are ordered correctly
  EXPECT_EQ(listeners[size_t(testEvent)][0], firstCallback);
  EXPECT_EQ(listeners[size_t(testEvent)][1], middleCallback);
  EXPECT_EQ(listeners[size_t(testEvent)][2], lastCallback);

  // make sure the callbacks are triggered in the correct order
  MFileIO::newFile(true);
  EXPECT_FALSE(g_orderFailed);
  EXPECT_EQ(3, g_orderTest);

  ev.unregisterCallback(lastCallback);
  ev.unregisterCallback(middleCallback);
  ev.unregisterCallback(firstCallback);
  EXPECT_EQ(listeners[size_t(testEvent)].size(), 0);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test simple callback userdata is working
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, userDataIsWorking)
{
  MFileIO::newFile(true);

  struct SomeUserData
  {
    std::string name = "userDataIsWorking";
  };

  SomeUserData* d = new SomeUserData();

  MayaEventType testEvent = MayaEventType::kAfterNew;
  AL::usdmaya::Callback callback = [](void* userData)
      {
        SomeUserData* data = static_cast<SomeUserData*>(userData);
        data->name = "changed";
      };

  MayaEventManager& ev = MayaEventManager::instance();
  auto id = ev.registerCallback(
    testEvent,
    callback,
    "tag",
    1000,
    d);

  MFileIO::newFile(true);
  EXPECT_EQ(d->name, "changed");
  EXPECT_TRUE(ev.unregisterCallback(id));
}

#endif
