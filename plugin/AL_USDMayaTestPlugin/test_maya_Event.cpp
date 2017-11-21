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
/// \brief  Test that the event ordering is working correctly
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, event_ordering)
{
  MFileIO::newFile(true);
  std::cout << "Listener Ordering " << std::endl;
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
