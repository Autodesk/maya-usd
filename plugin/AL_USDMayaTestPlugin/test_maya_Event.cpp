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

using namespace AL::usdmaya::events;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test that the Register is working correctly
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, registerEvent)
{
  MFileIO::newFile(true);
  MayaEventManager ev;
  MayaEventType testEvent = MayaEventType::kAfterNew;
  auto& listeners = ev.listeners();
  Listener l;
  l.callback = [](void*){std::cout << "I'm registered!" << std::endl;};
  EventID id = ev.registerCallback(testEvent, l);
  const MayaCallbackIDContainer& mayaIDs = ev.mayaCallbackIDs();
  EXPECT_TRUE(ev.isMayaCallbackRegistered(testEvent));
  EXPECT_EQ(listeners[testEvent].size(), 1);
  ev.deregister(id);
}


//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test registering invalid types doesn't crash or register a listener
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, invalidRegisteredEvent)
{
  MFileIO::newFile(true);
  MayaEventManager ev;
  MayaEventType testEvent = MayaEventType::kSceneMessageLast; // Invalid event
  auto& listeners = ev.listeners();
  Listener l;
  l.callback = [](void*){std::cout << "I shouldn't be registered!" << std::endl;};
  EventID id = ev.registerCallback(testEvent, l);
  EXPECT_EQ(id, 0);
  EXPECT_EQ(listeners[testEvent].size(), 0);
  ev.deregister(id);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test registerLast method is working
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, testRegisterLast)
{
  MFileIO::newFile(true);
  MayaEventManager ev;
  MayaEventType testEvent = MayaEventType::kAfterNew;
  auto& listeners = ev.listeners();
  Listener l;
  l.callback = [](void*){};
  EventID first = ev.registerCallback(testEvent, l);
  EventID second = ev.registerCallback(testEvent, l);
  EventID last = ev.registerLast(testEvent, l.callback);

  EXPECT_EQ(listeners[testEvent].size(), 3);
  EXPECT_EQ((EventID)listeners[testEvent].back().get(), last);

  ev.deregister(last);
  ev.deregister(second);
  ev.deregister(first);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test registerFirst method is working
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, testRegisterFirst)
{
  MFileIO::newFile(true);
  MayaEventManager ev;
  MayaEventType testEvent = MayaEventType::kAfterNew;
  auto& listeners = ev.listeners();
  Listener l;
  l.callback = [](void*){};
  EventID first = ev.registerCallback(testEvent, l);
  EventID second = ev.registerCallback(testEvent, l);
  EventID actuallyImFirst = ev.registerFirst(testEvent, l.callback);

  EXPECT_EQ(listeners[testEvent].size(), 3);

  EXPECT_EQ((EventID)listeners[testEvent].front().get(), actuallyImFirst);

  ev.deregister(second);
  ev.deregister(first);
  ev.deregister(actuallyImFirst);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test that a simple Deregister is working correctly
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, simpleDeregisterEvent)
{
  MFileIO::newFile(true);
  MayaEventManager ev;
  Listener l;
  l.callback = [](void*){std::cout << "I'm registered!" << std::endl;};
  auto& listeners = ev.listeners();
  MayaEventType testEvent = MayaEventType::kAfterNew;

  EventID id = ev.registerCallback(testEvent, l);
  EXPECT_TRUE(ev.isMayaCallbackRegistered(testEvent));
  EXPECT_EQ(listeners[testEvent].size(), 1);

  EXPECT_TRUE(ev.deregister(testEvent, id));
  EXPECT_FALSE(ev.isMayaCallbackRegistered(testEvent));
  EXPECT_EQ(listeners[testEvent].size(), 0);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test deregistering invalid types doesn't crash or cause any side effects
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, invalidDeregisteredEvent)
{
  MFileIO::newFile(true);
  MayaEventManager ev;
  MayaEventType testEvent = MayaEventType::kSceneMessageLast; // Invalid event
  EXPECT_FALSE(ev.deregister(testEvent));
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test that the event ordering is working correctly
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, eventOrdering)
{
  MFileIO::newFile(true);
  MayaEventManager ev;
  Listener first, middle, last;
  first.callback = [](void*){std::cout << "First callback " << std::endl;};
  first.weight = 0;

  middle.command = "print 'middle'";
  middle.isPython = true;
  middle.weight = 1;

  last.command = "print \"last\"";
  last.weight = 2;
  MayaEventType testEvent = AL::usdmaya::events::kAfterNew;

  EventID middleCallback = ev.registerCallback(testEvent, middle);
  EventID lastCallback = ev.registerCallback(testEvent, last);
  EventID firstCallback = ev.registerCallback(testEvent, first);

  MFileIO::newFile(true);

  auto& listeners = ev.listeners();
  EXPECT_EQ(listeners[testEvent].size(), 3);

  // check they are ordered correctly
  EXPECT_EQ(firstCallback, (EventID)listeners[testEvent][0].get());
  EXPECT_EQ(middleCallback, (EventID)listeners[testEvent][1].get());
  EXPECT_EQ(lastCallback, (EventID)listeners[testEvent][2].get());

  ev.deregister(testEvent, lastCallback);
  ev.deregister(testEvent, middleCallback);
  ev.deregister(testEvent, firstCallback);
  EXPECT_EQ(listeners[testEvent].size(), 0);
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

  MayaEventType testEvent = AL::usdmaya::events::kAfterNew;
  AL::usdmaya::events::Callback callback = [](void* userData)
      {
        SomeUserData* data = static_cast<SomeUserData*>(userData);
        data->name = "changed";
      };

  MayaEventManager ev;
  EventID id = ev.registerCallback(testEvent, callback, (void*)d);
  MFileIO::newFile(true);
  EXPECT_EQ(d->name, "changed");
  EXPECT_TRUE(ev.deregister(testEvent, id));
}
