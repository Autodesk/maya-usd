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
  l.callback = [](UserData* che){std::cout << "I'm registered!" << std::endl;};
  EventID id = ev.registerCallback(MayaEventType::kAfterNew, l);
  EXPECT_EQ(listeners[testEvent].size(), 1);
  std::cout << "REgister " << listeners[testEvent].size() << std::endl;
  ev.deregister(id);
  std::cout << "DeREgister " << listeners[testEvent].size() << std::endl;
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test that the Deregister is working correctly
//----------------------------------------------------------------------------------------------------------------------
//TEST(maya_Event, deregisterEvent)
//{
//  MFileIO::newFile(true);
//  MayaEventManager ev;
//  Listener l;
//  l.callback = [](UserData* che){std::cout << "I'm registered!" << std::endl;};
//  auto& listeners = ev.listeners();
//  MayaEventType testEvent = MayaEventType::kAfterNew;
//
//  EventID id = ev.registerCallback(testEvent, l);
//  EXPECT_EQ(listeners[testEvent].size(), 1);
//
//  ev.deregister(testEvent, id);
//  EXPECT_EQ(listeners[testEvent].size(), 0);
//
//  id = ev.registerCallback(testEvent, l);
//  ev.deregister(id);
//  EXPECT_EQ(listeners[testEvent].size(), 0);
//  std::cout << "Deregister " << listeners[testEvent].size() << std::endl;
//}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test that the event ordering is working correctly
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, eventOrdering)
{
  MFileIO::newFile(true);
  MayaEventManager ev;
  Listener first, middle, last;
  first.callback = [](UserData* che){std::cout << "First callback " << std::endl;};
  first.weight = 0;

  middle.command = "print 'middle'";
  middle.isPython = true;
  middle.weight = 1;

  last.command = "print \"last\"";
  last.weight = 1;
  MayaEventType testEvent = AL::usdmaya::events::kAfterNew;

  EventID middleCallback = ev.registerCallback(testEvent, middle);
  EventID lastCallback = ev.registerCallback(testEvent, last);
  EventID firstCallback = ev.registerCallback(testEvent, first);

  MFileIO::newFile(true);

  auto& listeners = ev.listeners();
  EXPECT_EQ(listeners[testEvent].size(), 3);

  ev.deregister(testEvent, lastCallback);
  ev.deregister(testEvent, middleCallback);
  ev.deregister(testEvent, firstCallback);

  EXPECT_EQ(listeners[MayaEventType::kBeforeNew].size(), 0);
}
