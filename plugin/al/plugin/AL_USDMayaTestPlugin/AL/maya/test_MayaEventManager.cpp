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
#include "AL/maya/event/MayaEventManager.h"
#include "test_usdmaya.h"

#include <maya/MFileIO.h>

using namespace AL::maya::event;
using namespace AL::event;

static bool g_called = 0;
static void callback_test(void*) { g_called = true; };

static int  g_orderTest = 0;
static bool g_orderFailed = false;
static void callback_order_test(void* userData)
{
    if (*(const int*)userData != g_orderTest)
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
    MayaEventHandler* meh = ev.mayaEventsHandler();

    auto info = meh->getEventInfo("AfterNew");
    ASSERT_TRUE(info != nullptr);
    auto priorRefCount = info->refCount;

    auto callback = ev.registerCallback(callback_test, "AfterNew", "I'm a tag", 1234, &userData);

    ASSERT_TRUE(meh->isMayaCallbackRegistered("AfterNew"));

    Callback* callbackInfo = meh->scheduler()->findCallback(callback);

    EXPECT_EQ(priorRefCount + 1, info->refCount);
    EXPECT_EQ(&userData, callbackInfo->userData());
    EXPECT_EQ((void*)(callback_test), callbackInfo->callback());
    EXPECT_EQ(std::string(""), std::string(callbackInfo->callbackText()));
    EXPECT_EQ(std::string("I'm a tag"), std::string(callbackInfo->tag()));
    EXPECT_EQ(callback, callbackInfo->callbackId());
    EXPECT_EQ(1234u, callbackInfo->weight());
    EXPECT_FALSE(callbackInfo->isPythonCallback());
    EXPECT_FALSE(callbackInfo->isMELCallback());
    EXPECT_TRUE(callbackInfo->isCCallback());

    MFileIO::newFile(true);

    EXPECT_TRUE(g_called);
    g_called = false;

    ev.unregisterCallback(callback);
    EXPECT_EQ(priorRefCount, info->refCount);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test registering invalid types doesn't crash or register a listener
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, invalidRegisteredEvent)
{
    MFileIO::newFile(true);
    MayaEventManager& ev = MayaEventManager::instance();

    auto id = ev.registerCallback(callback_test, "StupidInavlidEventName", "I'm a tag", 1234, 0);

    EXPECT_EQ(id, 0u);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test that a simple Deregister is working correctly
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, simpleUnregisterEvent)
{
    MFileIO::newFile(true);
    MayaEventManager& ev = MayaEventManager::instance();
    MayaEventHandler* meh = ev.mayaEventsHandler();

    EXPECT_FALSE(meh->isMayaCallbackRegistered("BeforeSoftwareFrameRender"));

    auto id = ev.registerCallback(callback_test, "BeforeSoftwareFrameRender", "I'm a tag", 1234, 0);

    EXPECT_TRUE(meh->isMayaCallbackRegistered("BeforeSoftwareFrameRender"));

    ev.unregisterCallback(id);
    EXPECT_FALSE(meh->isMayaCallbackRegistered("BeforeSoftwareFrameRender"));
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test deregistering invalid types doesn't crash or cause any side effects
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, invalidDeregisteredEvent)
{
    MFileIO::newFile(true);

    CallbackId badId = makeCallbackId(0x4567, kMayaEventType, 0x999987);

    MayaEventManager& ev = MayaEventManager::instance();
    EXPECT_FALSE(ev.unregisterCallback(badId));
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test that the event ordering is working correctly
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_Event, eventOrdering)
{
    MFileIO::newFile(true);
    MayaEventManager& ev = MayaEventManager::instance();
    MayaEventHandler* meh = ev.mayaEventsHandler();
    EventDispatcher*  dispatcher = meh->scheduler()->event("AfterNew");
    auto&             cbs = dispatcher->callbacks();

    const size_t numCallbacks = cbs.size();

    int firstInt = 0;
    int secondInt = 1;
    int thirdInt = 2;

    auto middleCallback
        = ev.registerCallback(callback_order_test, "AfterNew", "middle", 22, &secondInt);

    auto lastCallback = ev.registerCallback(callback_order_test, "AfterNew", "last", 33, &thirdInt);

    auto firstCallback
        = ev.registerCallback(callback_order_test, "AfterNew", "first", 11, &firstInt);

    // check they are ordered correctly
    EXPECT_EQ(firstCallback, cbs[0].callbackId());
    EXPECT_EQ(middleCallback, cbs[1].callbackId());
    EXPECT_EQ(lastCallback, cbs[2].callbackId());

    // make sure the callbacks are triggered in the correct order
    MFileIO::newFile(true);
    EXPECT_FALSE(g_orderFailed);
    EXPECT_EQ(3, g_orderTest);
    EXPECT_EQ(numCallbacks + 3, cbs.size());

    ev.unregisterCallback(lastCallback);
    ev.unregisterCallback(middleCallback);
    ev.unregisterCallback(firstCallback);
    EXPECT_EQ(numCallbacks, cbs.size());
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

    auto callback = [](void* userData) {
        SomeUserData* data = static_cast<SomeUserData*>(userData);
        data->name = "changed";
    };

    MayaEventManager& ev = MayaEventManager::instance();
    auto              id = ev.registerCallback(callback, "AfterNew", "tag", 1000, d);

    MFileIO::newFile(true);
    EXPECT_EQ(d->name, "changed");
    EXPECT_TRUE(ev.unregisterCallback(id));
    delete d;
}
