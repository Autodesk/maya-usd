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
// distributed under the License is distributed oncommand an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "AL/maya/event/MayaEventManager.h"

#include <maya/MAnimMessage.h>
#include <maya/MCameraSetMessage.h>
#include <maya/MContainerMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MGlobal.h>
#include <maya/MModelMessage.h>

#include <iostream>

namespace AL {
namespace maya {
namespace event {

//----------------------------------------------------------------------------------------------------------------------
static void bindBasicFunction(void* ptr)
{
    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindCheckFunction(bool* retCode, void* ptr)
{
    bool result = true;
    auto binder = [&result](void* ud, const void* cb) {
        MMessage::MCheckFunction cf = (MMessage::MCheckFunction)cb;
        bool                     temp = true;
        cf(&temp, ud);
        result = result && temp;
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindCheckPlugFunction(bool* retCode, MPlug& plug, void* ptr)
{
    bool result = true;
    auto binder = [&result, &plug](void* ud, const void* cb) {
        MMessage::MCheckPlugFunction cf = (MMessage::MCheckPlugFunction)cb;
        bool                         temp = true;
        cf(&temp, plug, ud);
        result = result && temp;
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindNodeFunction(MObject& node, void* ptr)
{
    auto binder = [&node](void* ud, const void* cb) {
        MMessage::MNodeFunction cf = (MMessage::MNodeFunction)cb;
        cf(node, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindStringFunction(const MString& str, void* ptr)
{
    auto binder = [str](void* ud, const void* cb) {
        MMessage::MStringFunction cf = (MMessage::MStringFunction)cb;
        cf(str, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindStringIntBoolIntFunction(
    const MString& str,
    uint32_t       index,
    bool           flag,
    uint32_t       type,
    void*          ptr)
{
    auto binder = [str, index, flag, type](void* ud, const void* cb) {
        MMessage::MStringIntBoolIntFunction cf = (MMessage::MStringIntBoolIntFunction)cb;
        cf(str, index, flag, type, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindNodeStringBoolFunction(MObject& node, const MString& str, bool flag, void* ptr)
{
    auto binder = [&node, str, flag](void* ud, const void* cb) {
        MMessage::MNodeStringBoolFunction cf = (MMessage::MNodeStringBoolFunction)cb;
        cf(node, str, flag, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindTimeFunction(MTime& time, void* ptr)
{
    auto binder = [&time](void* ud, const void* cb) {
        MMessage::MTimeFunction cf = (MMessage::MTimeFunction)cb;
        cf(time, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindPlugFunction(MPlug& src, MPlug& dst, bool made, void* ptr)
{
    auto binder = [&src, &dst, made](void* ud, const void* cb) {
        MMessage::MPlugFunction cf = (MMessage::MPlugFunction)cb;
        cf(src, dst, made, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindParentChildFunction(MDagPath& child, MDagPath& parent, void* ptr)
{
    auto binder = [&child, &parent](void* ud, const void* cb) {
        MMessage::MParentChildFunction cf = (MMessage::MParentChildFunction)cb;
        cf(child, parent, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindStringArrayFunction(const MStringArray& strs, void* ptr)
{
    auto binder = [strs](void* ud, const void* cb) {
        MMessage::MStringArrayFunction cf = (MMessage::MStringArrayFunction)cb;
        cf(strs, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindObjArrayFunction(MObjectArray& objects, void* ptr)
{
    auto binder = [&objects](void* ud, const void* cb) {
        MMessage::MObjArray cf = (MMessage::MObjArray)cb;
        cf(objects, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindCameraLayerFunction(MObject& cameraSetNode, uint32_t index, bool added, void* ptr)
{
    auto binder = [&cameraSetNode, index, added](void* ud, const void* cb) {
        MMessage::MCameraLayerFunction cf = (MMessage::MCameraLayerFunction)cb;
        cf(cameraSetNode, index, added, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindCameraLayerCameraFunction(
    MObject& cameraSetNode,
    uint32_t index,
    MObject& oldCamera,
    MObject& newCamera,
    void*    ptr)
{
    auto binder = [&cameraSetNode, index, &oldCamera, &newCamera](void* ud, const void* cb) {
        MMessage::MCameraLayerCameraFunction cf = (MMessage::MCameraLayerCameraFunction)cb;
        cf(cameraSetNode, index, oldCamera, newCamera, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindPlugsDGModFunction(MPlugArray& plugs, MDGModifier& modifier, void* ptr)
{
    auto binder = [&plugs, &modifier](void* ud, const void* cb) {
        MMessage::MPlugsDGModFunction cf = (MMessage::MPlugsDGModFunction)cb;
        cf(plugs, modifier, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void
bindMessageFunction(const MString& message, MCommandMessage::MessageType messageType, void* ptr)
{
    auto binder = [message, messageType](void* ud, const void* cb) {
        MCommandMessage::MMessageFunction cf = (MCommandMessage::MMessageFunction)cb;
        cf(message, messageType, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindMessageFilterFunction(
    const MString&               message,
    MCommandMessage::MessageType messageType,
    bool&                        filterOutput,
    void*                        ptr)
{
    filterOutput = false;
    auto binder = [&filterOutput, message, messageType](void* ud, const void* cb) {
        bool                                    temp = false;
        MCommandMessage::MMessageFilterFunction cf = (MCommandMessage::MMessageFilterFunction)cb;
        cf(message, messageType, temp, ud);
        filterOutput = filterOutput || temp;
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindMessageParentChildFunction(
    MDagMessage::DagMessage msgType,
    MDagPath&               child,
    MDagPath&               parent,
    void*                   ptr)
{
    auto binder = [msgType, &child, &parent](void* ud, const void* cb) {
        MDagMessage::MMessageParentChildFunction cf = (MDagMessage::MMessageParentChildFunction)cb;
        cf(msgType, child, parent, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindPathObjectPlugColoursFunction(
    MDagPath&    path,
    MObject&     object,
    MPlug&       plug,
    MColorArray& colors,
    void*        ptr)
{
    auto binder = [&path, &object, &plug, &colors](void* ud, const void* cb) {
        MPaintMessage::MPathObjectPlugColorsFunction cf
            = (MPaintMessage::MPathObjectPlugColorsFunction)cb;
        cf(path, object, plug, colors, ud);
    };

    MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
    auto&                               scheduler = AL::event::EventScheduler::getScheduler();
    scheduler.triggerEvent(cbi->eventId, binder);
}

// these are functions we will probably want in the long term, however to ensure the OpenSource
// build does not complain about unused symbols, they are hidden behind a #if 0 for now.
#if 0
//----------------------------------------------------------------------------------------------------------------------
static void bindElapsedTimeFunction(float elapsed, float last, void* ptr)
{
  auto binder = [elapsed, last](void* ud, const void* cb)
  {
    MMessage::MElapsedTimeFunction cf = (MMessage::MElapsedTimeFunction)cb;
    cf(elapsed, last, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindCheckFileFunction(bool* retCode, MFileObject& obj, void* ptr)
{
  bool result = true;
  auto binder = [&result, &obj](void* ud, const void* cb)
  {
    MMessage::MCheckFileFunction cf = (MMessage::MCheckFileFunction)cb;
    bool temp = true;
    cf(&temp, obj, ud);
    result = result && temp;
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindComponentFunction(MUintArray componentIds[], unsigned int count, void* ptr)
{
  auto binder = [componentIds, count](void* ud, const void* cb)
  {
    MMessage::MComponentFunction cf = (MMessage::MComponentFunction)cb;
    cf(componentIds, count, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindTwoStringFunction(const MString& str1, const MString& str2, void* ptr)
{
  auto binder = [str1, str2](void* ud, const void* cb)
  {
    MMessage::MTwoStringFunction cf = (MMessage::MTwoStringFunction)cb;
    cf(str1, str2, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindThreeStringFunction(const MString& str1, const MString& str2, const MString& str3, void* ptr)
{
  auto binder = [str1, str2, str3](void* ud, const void* cb)
  {
    MMessage::MThreeStringFunction cf = (MMessage::MThreeStringFunction)cb;
    cf(str1, str2, str3, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindStringIndexFunction(const MString& str, uint32_t index, void* ptr)
{
  auto binder = [str, index](void* ud, const void* cb)
  {
    MMessage::MStringIndexFunction cf = (MMessage::MStringIndexFunction)cb;
    cf(str, index, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindStateFunction(bool flag, void* ptr)
{
  auto binder = [flag](void* ud, const void* cb)
  {
    MMessage::MStateFunction cf = (MMessage::MStateFunction)cb;
    cf(flag, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindNodePlugFunction(MObject& node, MPlug& plug, void* ptr)
{
  auto binder = [&node, &plug](void* ud, const void* cb)
  {
    MMessage::MNodePlugFunction cf = (MMessage::MNodePlugFunction)cb;
    cf(node, plug, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindNodeStringFunction(MObject& node, const MString& str, void* ptr)
{
  auto binder = [&node, str](void* ud, const void* cb)
  {
    MMessage::MNodeStringFunction cf = (MMessage::MNodeStringFunction)cb;
    cf(node, str, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindModifierFunction(MDGModifier& mod, void* ptr)
{
  auto binder = [&mod](void* ud, const void* cb)
  {
    MMessage::MModifierFunction cf = (MMessage::MModifierFunction)cb;
    cf(mod, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindNodeModifierFunction(MObject& node, MDGModifier& mod, void* ptr)
{
  auto binder = [&node, &mod](void* ud, const void* cb)
  {
    MMessage::MNodeModifierFunction cf = (MMessage::MNodeModifierFunction)cb;
    cf(node, mod, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindNodeObjArrayFunction(MObject& node, MObjectArray& objects, void* ptr)
{
  auto binder = [&node, &objects](void* ud, const void* cb)
  {
    MMessage::MNodeObjArray cf = (MMessage::MNodeObjArray)cb;
    cf(node, objects, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindStringNodeFunction(const MString& str, MObject& node, void* ptr)
{
  auto binder = [str, &node](void* ud, const void* cb)
  {
    MMessage::MStringNode cf = (MMessage::MStringNode)cb;
    cf(str, node, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindConnFailFunction(MPlug& src, MPlug& dst, const MString& srcName, const MString& dstName, void* ptr)
{
  auto binder = [&src, &dst, srcName, dstName](void* ud, const void* cb)
  {
    MMessage::MConnFailFunction cf = (MMessage::MConnFailFunction)cb;
    cf(src, dst, srcName, dstName, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindNodeUuidFunction(MObject& node, MUuid& uuid, void* ptr)
{
  auto binder = [&node, &uuid](void* ud, const void* cb)
  {
    MMessage::MNodeUuidFunction cf = (MMessage::MNodeUuidFunction)cb;
    cf(node, uuid, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindObjectFileFunction(MObject& node, MFileObject& file, void* ptr)
{
  auto binder = [&node, &file](void* ud, const void* cb)
  {
    MMessage::MObjectFileFunction cf = (MMessage::MObjectFileFunction)cb;
    cf(node, file, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindCheckObjectFileFunction(bool* retCode, const MObject& referenceNode, MFileObject& fo, void* ptr)
{
  bool result = true;
  auto binder = [&result, referenceNode, &fo](void* ud, const void* cb)
  {
    MMessage::MCheckObjectFileFunction cf = (MMessage::MCheckObjectFileFunction)cb;
    bool temp = true;
    cf(&temp, referenceNode, fo, ud);
    result = result && temp;
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

//----------------------------------------------------------------------------------------------------------------------
static void bindWorldMatrixModifiedFunction(MObject& transform, MDagMessage::MatrixModifiedFlags& modified, void* ptr)
{
  auto binder = [&transform, &modified](void* ud, const void* cb)
  {
    MDagMessage::MWorldMatrixModifiedFunction cf = (MDagMessage::MWorldMatrixModifiedFunction)cb;
    cf(transform, modified, ud);
  };

  MayaEventHandler::MayaCallbackInfo* cbi = (MayaEventHandler::MayaCallbackInfo*)ptr;
  auto& scheduler = AL::event::EventScheduler::getScheduler();
  scheduler.triggerEvent(cbi->eventId, binder);
}

#endif

//----------------------------------------------------------------------------------------------------------------------
MayaEventHandler::MayaEventHandler(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
    m_scheduler = scheduler;
    registerAnimMessages(scheduler, eventType);
    registerCameraSetMessages(scheduler, eventType);
    registerCommandMessages(scheduler, eventType);
    registerConditionMessages(scheduler, eventType);
    registerContainerMessages(scheduler, eventType);
    registerDagMessages(scheduler, eventType);
    registerDGMessages(scheduler, eventType);
    registerEventMessages(scheduler, eventType);
    registerLockMessages(scheduler, eventType);
    registerModelMessages(scheduler, eventType);
    registerNodeMessages(scheduler, eventType);
    registerObjectSetMessages(scheduler, eventType);
    registerPaintMessages(scheduler, eventType);
    registerPolyMessages(scheduler, eventType);
    registerSceneMessages(scheduler, eventType);
    registerTimerMessages(scheduler, eventType);
    registerUiMessages(scheduler, eventType);
    scheduler->registerHandler(eventType, this);
}

//----------------------------------------------------------------------------------------------------------------------
bool MayaEventHandler::registerEvent(
    AL::event::EventScheduler* scheduler,
    const char*                eventName,
    AL::event::EventType       eventType,
    MayaMessageType            messageType,
    MayaCallbackType           callbackType,
    uint32_t                   mmessageEnum)
{
    // first register the new event with the scheduler
    AL::event::EventId id = scheduler->registerEvent(eventName, eventType);
    if (!id) {
        std::cout << "failed to register event " << eventName << std::endl;
        return false;
    }

    auto             index = m_callbacks.size();
    MayaCallbackInfo cb = { id, 0, messageType, callbackType, mmessageEnum, 0 };
    m_callbacks.push_back(cb);
    m_eventMapping[id] = index;

    return true;
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::onCallbackCreated(const AL::event::CallbackId callbackId)
{
    AL::event::EventId id = AL::event::extractEventId(callbackId);

    EventToMaya::iterator it = m_eventMapping.find(id);
    if (it != m_eventMapping.end()) {
        const uint32_t index = it->second;
        auto&          event = m_callbacks[index];
        // if ref count is zero, register callback with maya
        if (!event.refCount) {
            initEvent(event);
        }
        ++event.refCount;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::onCallbackDestroyed(const AL::event::CallbackId callbackId)
{
    AL::event::EventId    id = AL::event::extractEventId(callbackId);
    EventToMaya::iterator it = m_eventMapping.find(id);
    if (it != m_eventMapping.end()) {
        const uint32_t index = it->second;
        auto&          event = m_callbacks[index];
        --event.refCount;

        // if reduced callback count has hit zero, destroy maya callback
        if (!event.refCount) {
            MMessage::removeCallback(event.mayaCallback);
            event.mayaCallback = 0;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initAnimMessage(MayaCallbackInfo& cbi)
{
    switch (cbi.mmessageEnum) {
    case AnimMessage::kAnimCurveEdited:
        cbi.mayaCallback = MAnimMessage::addAnimCurveEditedCallback(bindObjArrayFunction, &cbi);
        break;
    case AnimMessage::kAnimKeyFrameEdited:
        cbi.mayaCallback = MAnimMessage::addAnimKeyframeEditedCallback(bindObjArrayFunction, &cbi);
        break;
    case AnimMessage::kNodeAnimKeyframeEdited: /* unsupported */ break;
    case AnimMessage::kAnimKeyframeEditCheck:
        cbi.mayaCallback
            = MAnimMessage::addAnimKeyframeEditCheckCallback(bindCheckPlugFunction, &cbi);
        break;
    case AnimMessage::kPreBakeResults:
        cbi.mayaCallback = MAnimMessage::addPreBakeResultsCallback(bindPlugsDGModFunction, &cbi);
        break;
    case AnimMessage::kPostBakeResults:
        cbi.mayaCallback = MAnimMessage::addPostBakeResultsCallback(bindPlugsDGModFunction, &cbi);
        break;
    case AnimMessage::kDisableImplicitControl:
        cbi.mayaCallback
            = MAnimMessage::addDisableImplicitControlCallback(bindPlugsDGModFunction, &cbi);
        break;
    default: break;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerAnimMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
    registerEvent(
        scheduler,
        "AnimCurveEdited",
        eventType,
        MayaMessageType::kAnimMessage,
        MayaCallbackType::kObjArrayFunction,
        AnimMessage::kAnimCurveEdited);
    registerEvent(
        scheduler,
        "AnimKeyFrameEdited",
        eventType,
        MayaMessageType::kAnimMessage,
        MayaCallbackType::kObjArrayFunction,
        AnimMessage::kAnimKeyFrameEdited);
    // registerEvent(scheduler, "NodeAnimKeyframeEdited", eventType, MayaMessageType::kAnimMessage,
    // MayaCallbackType::kNodeObjArrayFunction, AnimMessage::kNodeAnimKeyframeEdited);
    registerEvent(
        scheduler,
        "AnimKeyframeEditCheck",
        eventType,
        MayaMessageType::kAnimMessage,
        MayaCallbackType::kCheckPlugFunction,
        AnimMessage::kAnimKeyframeEditCheck);
    registerEvent(
        scheduler,
        "PreBakeResults",
        eventType,
        MayaMessageType::kAnimMessage,
        MayaCallbackType::kPlugsDGModFunction,
        AnimMessage::kPreBakeResults);
    registerEvent(
        scheduler,
        "PostBakeResults",
        eventType,
        MayaMessageType::kAnimMessage,
        MayaCallbackType::kPlugsDGModFunction,
        AnimMessage::kPostBakeResults);
    registerEvent(
        scheduler,
        "DisableImplicitControl",
        eventType,
        MayaMessageType::kAnimMessage,
        MayaCallbackType::kPlugsDGModFunction,
        AnimMessage::kDisableImplicitControl);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initCameraSetMessage(MayaCallbackInfo& cbi)
{
    switch (cbi.mmessageEnum) {
    case CameraSetMessage::kCameraLayer:
        cbi.mayaCallback = MCameraSetMessage::addCameraLayerCallback(bindCameraLayerFunction, &cbi);
        break;
    case CameraSetMessage::kCameraChanged:
        cbi.mayaCallback
            = MCameraSetMessage::addCameraChangedCallback(bindCameraLayerCameraFunction, &cbi);
        break;
    default: break;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerCameraSetMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
    registerEvent(
        scheduler,
        "CameraLayer",
        eventType,
        MayaMessageType::kCameraSetMessage,
        MayaCallbackType::kCameraLayerFunction,
        CameraSetMessage::kCameraLayer);
    registerEvent(
        scheduler,
        "CameraChanged",
        eventType,
        MayaMessageType::kCameraSetMessage,
        MayaCallbackType::kCameraLayerCameraFunction,
        CameraSetMessage::kCameraChanged);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initCommandMessage(MayaCallbackInfo& cbi)
{
    switch (cbi.mmessageEnum) {
    case CommandMessage::kCommand:
        cbi.mayaCallback = MCommandMessage::addCommandCallback(bindStringFunction, &cbi);
        break;
    case CommandMessage::kCommandOuptut:
        cbi.mayaCallback = MCommandMessage::addCommandOutputCallback(bindMessageFunction, &cbi);
        break;
    case CommandMessage::kCommandOutputFilter:
        cbi.mayaCallback
            = MCommandMessage::addCommandOutputFilterCallback(bindMessageFilterFunction, &cbi);
        break;
    case CommandMessage::kProc:
        cbi.mayaCallback = MCommandMessage::addProcCallback(bindStringIntBoolIntFunction, &cbi, 0);
        break;
    default: break;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerCommandMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
    registerEvent(
        scheduler,
        "Command",
        eventType,
        MayaMessageType::kCommandMessage,
        MayaCallbackType::kStringFunction,
        CommandMessage::kCommand);
    registerEvent(
        scheduler,
        "CommandOuptut",
        eventType,
        MayaMessageType::kCommandMessage,
        MayaCallbackType::kMessageFunction,
        CommandMessage::kCommandOuptut);
    registerEvent(
        scheduler,
        "CommandOutputFilter",
        eventType,
        MayaMessageType::kCommandMessage,
        MayaCallbackType::kMessageFilterFunction,
        CommandMessage::kCommandOutputFilter);
    registerEvent(
        scheduler,
        "Proc",
        eventType,
        MayaMessageType::kCommandMessage,
        MayaCallbackType::kStringIntBoolIntFunction,
        CommandMessage::kProc);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initConditionMessage(MayaCallbackInfo& cbi) { }

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerConditionMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initContainerMessage(MayaCallbackInfo& cbi)
{
    switch (cbi.mmessageEnum) {
    case ContainerMessage::kPublishAttr:
        cbi.mayaCallback
            = MContainerMessage::addPublishAttrCallback(bindNodeStringBoolFunction, &cbi);
        break;
    case ContainerMessage::kBoundAttr:
        cbi.mayaCallback
            = MContainerMessage::addBoundAttrCallback(bindNodeStringBoolFunction, &cbi);
        break;
    default: break;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerContainerMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
    registerEvent(
        scheduler,
        "PublishAttr",
        eventType,
        MayaMessageType::kContainerMessage,
        MayaCallbackType::kNodeStringBoolFunction,
        ContainerMessage::kPublishAttr);
    registerEvent(
        scheduler,
        "BoundAttr",
        eventType,
        MayaMessageType::kContainerMessage,
        MayaCallbackType::kNodeStringBoolFunction,
        ContainerMessage::kBoundAttr);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initDagMessage(MayaCallbackInfo& cbi)
{
    switch (cbi.mmessageEnum) {
    case DagMessage::kParentAdded:
        cbi.mayaCallback = MDagMessage::addParentAddedCallback(bindParentChildFunction, &cbi);
        break;
    case DagMessage::kParentAddedDagPath: /* cbi.mayaCallback =
                                             MDagMessage::addParentAddedDagPathCallback(bindNodeStringBoolFunc,
                                             &cbi); */
        break;
    case DagMessage::kParentRemoved:
        cbi.mayaCallback = MDagMessage::addParentRemovedCallback(bindParentChildFunction, &cbi);
        break;
    case DagMessage::kParentRemovedDagPath: /* cbi.mayaCallback =
                                               MDagMessage::addParentRemovedDagPathCallback(bindNodeStringBoolFunc,
                                               &cbi); */
        break;
    case DagMessage::kChildAdded:
        cbi.mayaCallback = MDagMessage::addChildAddedCallback(bindParentChildFunction, &cbi);
        break;
    case DagMessage::kChildAddedDagPath: /* cbi.mayaCallback =
                                            MContainerMessage::addChildAddedDagPathCallback(bindNodeStringBoolFunc,
                                            &cbi); */
        break;
    case DagMessage::kChildRemoved:
        cbi.mayaCallback = MDagMessage::addChildRemovedCallback(bindParentChildFunction, &cbi);
        break;
    case DagMessage::kChildRemovedDagPath: /* cbi.mayaCallback =
                                              MDagMessage::addChildRemovedDagPathCallback(bindNodeStringBoolFunc,
                                              &cbi); */
        break;
    case DagMessage::kChildReordered:
        cbi.mayaCallback = MDagMessage::addChildReorderedCallback(bindParentChildFunction, &cbi);
        break;
    case DagMessage::kChildReorderedDagPath: /* cbi.mayaCallback =
                                                MDagMessage::addChildReorderedDagPathCallback(bindNodeStringBoolFunc,
                                                &cbi); */
        break;
    case DagMessage::kDag: /* cbi.mayaCallback =
                              MDagMessage::addDagCallback(bindMessageParentChildFunction, &cbi); */
        break;
    case DagMessage::kDagDagPath: /* cbi.mayaCallback =
                                     MDagMessage::addDagDagPathCallback(bindNodeStringBoolFunc,
                                     &cbi); */
        break;
    case DagMessage::kAllDagChanges:
        cbi.mayaCallback
            = MDagMessage::addAllDagChangesCallback(bindMessageParentChildFunction, &cbi);
        break;
    case DagMessage::kAllDagChangesDagPath: /* cbi.mayaCallback =
                                               MDagMessage::addAllDagChangesDagPathCallback(bindNodeStringBoolFunc,
                                               &cbi); */
        break;
    case DagMessage::kInstanceAdded:
        cbi.mayaCallback = MDagMessage::addInstanceAddedCallback(bindParentChildFunction, &cbi);
        break;
    case DagMessage::kInstanceAddedDagPath: /* cbi.mayaCallback =
                                               MDagMessage::addInstanceAddedDagPathCallback(bindNodeStringBoolFunc,
                                               &cbi); */
        break;
    case DagMessage::kInstanceRemoved:
        cbi.mayaCallback = MDagMessage::addInstanceRemovedCallback(bindParentChildFunction, &cbi);
        break;
    case DagMessage::kInstanceRemovedDagPath: /* cbi.mayaCallback =
                                                 MDagMessage::addInstanceRemovedDagPathCallback(bindNodeStringBoolFunc,
                                                 &cbi); */
        break;
    case DagMessage::kWorldMatrixModified: /* cbi.mayaCallback =
                                              MDagMessage::addWorldMatrixModifiedCallback(bindNodeStringBoolFunc,
                                              &cbi); */
        break;
    default: break;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerDagMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
    registerEvent(
        scheduler,
        "ParentAdded",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kParentChildFunction,
        DagMessage::kParentAdded);
    registerEvent(
        scheduler,
        "ParentRemoved",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kParentChildFunction,
        DagMessage::kParentRemoved);
    registerEvent(
        scheduler,
        "ChildAdded",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kParentChildFunction,
        DagMessage::kChildAdded);
    registerEvent(
        scheduler,
        "ChildRemoved",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kParentChildFunction,
        DagMessage::kChildRemoved);
    registerEvent(
        scheduler,
        "ChildReordered",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kParentChildFunction,
        DagMessage::kChildReordered);
    // registerEvent(scheduler, "Dag", eventType, MayaMessageType::kDagMessage,
    // MayaCallbackType::kMessageParentChildFunction, DagMessage::kDag);
    registerEvent(
        scheduler,
        "AllDagChanges",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kMessageParentChildFunction,
        DagMessage::kAllDagChanges);
    registerEvent(
        scheduler,
        "InstanceAdded",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kParentChildFunction,
        DagMessage::kInstanceAdded);
    registerEvent(
        scheduler,
        "InstanceRemoved",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kParentChildFunction,
        DagMessage::kInstanceRemoved);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initDGMessage(MayaCallbackInfo& cbi)
{
    switch (cbi.mmessageEnum) {
    case DGMessage::kTimeChange:
        cbi.mayaCallback = MDGMessage::addTimeChangeCallback(bindTimeFunction, &cbi);
        break;
    case DGMessage::kDelayedTimeChange:
        cbi.mayaCallback = MDGMessage::addDelayedTimeChangeCallback(bindTimeFunction, &cbi);
        break;
    case DGMessage::kDelayedTimeChangeRunup:
        cbi.mayaCallback = MDGMessage::addDelayedTimeChangeRunupCallback(bindTimeFunction, &cbi);
        break;
    case DGMessage::kForceUpdate:
        cbi.mayaCallback = MDGMessage::addForceUpdateCallback(bindTimeFunction, &cbi);
        break;
    case DGMessage::kNodeAdded:
        cbi.mayaCallback
            = MDGMessage::addNodeAddedCallback(bindNodeFunction, kDefaultNodeType, &cbi);
        break;
    case DGMessage::kNodeRemoved:
        cbi.mayaCallback
            = MDGMessage::addNodeRemovedCallback(bindNodeFunction, kDefaultNodeType, &cbi);
        break;
    case DGMessage::kConnection:
        cbi.mayaCallback = MDGMessage::addConnectionCallback(bindPlugFunction, &cbi);
        break;
    case DGMessage::kPreConnection:
        cbi.mayaCallback = MDGMessage::addPreConnectionCallback(bindPlugFunction, &cbi);
        break;
    default: break;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerDGMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
    registerEvent(
        scheduler,
        "TimeChange",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kTimeFunction,
        DGMessage::kTimeChange);
    registerEvent(
        scheduler,
        "DelayedTimeChange",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kTimeFunction,
        DGMessage::kDelayedTimeChange);
    registerEvent(
        scheduler,
        "DelayedTimeChangeRunup",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kTimeFunction,
        DGMessage::kDelayedTimeChangeRunup);
    registerEvent(
        scheduler,
        "ForceUpdate",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kTimeFunction,
        DGMessage::kForceUpdate);
    registerEvent(
        scheduler,
        "NodeAdded",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kNodeFunction,
        DGMessage::kNodeAdded);
    registerEvent(
        scheduler,
        "NodeRemoved",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kNodeFunction,
        DGMessage::kNodeRemoved);
    registerEvent(
        scheduler,
        "Connection",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kPlugFunction,
        DGMessage::kConnection);
    registerEvent(
        scheduler,
        "PreConnection",
        eventType,
        MayaMessageType::kDagMessage,
        MayaCallbackType::kPlugFunction,
        DGMessage::kPreConnection);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initEventMessage(MayaCallbackInfo& cbi) { }

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerEventMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initLockMessage(MayaCallbackInfo& cbi) { }

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerLockMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initModelMessage(MayaCallbackInfo& cbi)
{
    switch (cbi.mmessageEnum) {
    case ModelMessage::kCallback:
        cbi.mayaCallback = MModelMessage::addCallback(
            MModelMessage::kActiveListModified, bindBasicFunction, &cbi);
        break;
    case ModelMessage::kBeforeDuplicate:
        cbi.mayaCallback = MModelMessage::addBeforeDuplicateCallback(bindBasicFunction, &cbi);
        break;
    case ModelMessage::kAfterDuplicate:
        cbi.mayaCallback = MModelMessage::addAfterDuplicateCallback(bindBasicFunction, &cbi);
        break;
    case ModelMessage::kNodeAddedToModel: /* cbi.mayaCallback =
                                             MModelMessage::addNodeAddedToModelCallback(bindTimeFunction,
                                             &cbi); */
        break;
    case ModelMessage::kNodeRemovedFromModel: /* cbi.mayaCallback =
                                                 MModelMessage::addNodeRemovedFromModelCallback(bindTimeFunction,
                                                 &cbi); */
        break;
    default: break;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerModelMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
    registerEvent(
        scheduler,
        "Callback",
        eventType,
        MayaMessageType::kModelMessage,
        MayaCallbackType::kBasicFunction,
        ModelMessage::kCallback);
    registerEvent(
        scheduler,
        "BeforeDuplicate",
        eventType,
        MayaMessageType::kModelMessage,
        MayaCallbackType::kBasicFunction,
        ModelMessage::kBeforeDuplicate);
    registerEvent(
        scheduler,
        "AfterDuplicate",
        eventType,
        MayaMessageType::kModelMessage,
        MayaCallbackType::kBasicFunction,
        ModelMessage::kAfterDuplicate);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initNodeMessage(MayaCallbackInfo& cbi) { }

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerNodeMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initObjectSetMessage(MayaCallbackInfo& cbi) { }

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerObjectSetMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initPaintMessage(MayaCallbackInfo& cbi)
{
    switch (cbi.mmessageEnum) {
    case PaintMessage::kVertexColor:
        cbi.mayaCallback
            = MPaintMessage::addVertexColorCallback(bindPathObjectPlugColoursFunction, &cbi);
        break;
    default: break;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerPaintMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
    registerEvent(
        scheduler,
        "VertexColor",
        eventType,
        MayaMessageType::kModelMessage,
        MayaCallbackType::kPathObjectPlugColoursFunction,
        PaintMessage::kVertexColor);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initPolyMessage(MayaCallbackInfo& cbi) { }

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerPolyMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initSceneMessage(MayaCallbackInfo& cbi)
{
    switch (cbi.mmessageEnum) {
    case SceneMessage::kSceneUpdate:
    case SceneMessage::kBeforeNew:
    case SceneMessage::kAfterNew:
    case SceneMessage::kBeforeImport:
    case SceneMessage::kAfterImport:
    case SceneMessage::kBeforeOpen:
    case SceneMessage::kAfterOpen:
    case SceneMessage::kBeforeFileRead:
    case SceneMessage::kAfterFileRead:
    case SceneMessage::kAfterSceneReadAndRecordEdits:
    case SceneMessage::kBeforeExport:
    case SceneMessage::kExportStarted:
    case SceneMessage::kAfterExport:
    case SceneMessage::kBeforeSave:
    case SceneMessage::kAfterSave:
    case SceneMessage::kBeforeCreateReference:
    case SceneMessage::kBeforeLoadReferenceAndRecordEdits:
    case SceneMessage::kAfterCreateReference:
    case SceneMessage::kAfterCreateReferenceAndRecordEdits:
    case SceneMessage::kBeforeRemoveReference:
    case SceneMessage::kAfterRemoveReference:
    case SceneMessage::kBeforeImportReference:
    case SceneMessage::kAfterImportReference:
    case SceneMessage::kBeforeExportReference:
    case SceneMessage::kAfterExportReference:
    case SceneMessage::kBeforeUnloadReference:
    case SceneMessage::kAfterUnloadReference:
    case SceneMessage::kBeforeLoadReference:
    case SceneMessage::kBeforeCreateReferenceAndRecordEdits:
    case SceneMessage::kAfterLoadReference:
    case SceneMessage::kAfterLoadReferenceAndRecordEdits:
    case SceneMessage::kBeforeSoftwareRender:
    case SceneMessage::kAfterSoftwareRender:
    case SceneMessage::kBeforeSoftwareFrameRender:
    case SceneMessage::kAfterSoftwareFrameRender:
    case SceneMessage::kSoftwareRenderInterrupted:
    case SceneMessage::kMayaInitialized:
    case SceneMessage::kMayaExiting:
        cbi.mayaCallback = MSceneMessage::addCallback(
            MSceneMessage::Message(cbi.mmessageEnum), bindBasicFunction, &cbi, 0);
        break;

    case SceneMessage::kBeforeNewCheck:
    case SceneMessage::kBeforeImportCheck:
    case SceneMessage::kBeforeOpenCheck:
    case SceneMessage::kBeforeExportCheck:
    case SceneMessage::kBeforeSaveCheck:
    case SceneMessage::kBeforeCreateReferenceCheck:
    case SceneMessage::kBeforeLoadReferenceCheck:
        cbi.mayaCallback = MSceneMessage::addCheckCallback(
            MSceneMessage::Message(cbi.mmessageEnum), bindCheckFunction, &cbi, 0);
        break;

    case SceneMessage::kBeforePluginLoad:
    case SceneMessage::kAfterPluginLoad:
    case SceneMessage::kBeforePluginUnload:
    case SceneMessage::kAfterPluginUnload:
        cbi.mayaCallback = MSceneMessage::addStringArrayCallback(
            MSceneMessage::Message(cbi.mmessageEnum), bindStringArrayFunction, &cbi, 0);
        break;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerSceneMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
    registerEvent(
        scheduler,
        "SceneUpdate",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kSceneUpdate);
    registerEvent(
        scheduler,
        "BeforeNew",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeNew);
    registerEvent(
        scheduler,
        "AfterNew",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterNew);
    registerEvent(
        scheduler,
        "BeforeImport",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeImport);
    registerEvent(
        scheduler,
        "AfterImport",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterImport);
    registerEvent(
        scheduler,
        "BeforeOpen",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeOpen);
    registerEvent(
        scheduler,
        "AfterOpen",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterOpen);
    registerEvent(
        scheduler,
        "BeforeFileRead",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeFileRead);
    registerEvent(
        scheduler,
        "AfterFileRead",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterFileRead);
    registerEvent(
        scheduler,
        "AfterSceneReadAndRecordEdits",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterSceneReadAndRecordEdits);
    registerEvent(
        scheduler,
        "BeforeExport",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeExport);
    registerEvent(
        scheduler,
        "ExportStarted",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kExportStarted);
    registerEvent(
        scheduler,
        "AfterExport",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterExport);
    registerEvent(
        scheduler,
        "BeforeSave",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeSave);
    registerEvent(
        scheduler,
        "AfterSave",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterSave);
    registerEvent(
        scheduler,
        "BeforeCreateReference",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeCreateReference);
    registerEvent(
        scheduler,
        "BeforeLoadReferenceAndRecordEdits",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeLoadReferenceAndRecordEdits);
    registerEvent(
        scheduler,
        "AfterCreateReference",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterCreateReference);
    registerEvent(
        scheduler,
        "AfterCreateReferenceAndRecordEdits",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterCreateReferenceAndRecordEdits);
    registerEvent(
        scheduler,
        "BeforeRemoveReference",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeRemoveReference);
    registerEvent(
        scheduler,
        "AfterRemoveReference",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterRemoveReference);
    registerEvent(
        scheduler,
        "BeforeImportReference",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeImportReference);
    registerEvent(
        scheduler,
        "AfterImportReference",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterImportReference);
    registerEvent(
        scheduler,
        "BeforeExportReference",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeExportReference);
    registerEvent(
        scheduler,
        "AfterExportReference",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterExportReference);
    registerEvent(
        scheduler,
        "BeforeUnloadReference",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeUnloadReference);
    registerEvent(
        scheduler,
        "AfterUnloadReference",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterUnloadReference);
    registerEvent(
        scheduler,
        "BeforeLoadReference",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeLoadReference);
    registerEvent(
        scheduler,
        "BeforeCreateReferenceAndRecordEdits",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeCreateReferenceAndRecordEdits);
    registerEvent(
        scheduler,
        "AfterLoadReference",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterLoadReference);
    registerEvent(
        scheduler,
        "AfterLoadReferenceAndRecordEdits",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterLoadReferenceAndRecordEdits);
    registerEvent(
        scheduler,
        "BeforeSoftwareRender",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeSoftwareRender);
    registerEvent(
        scheduler,
        "AfterSoftwareRender",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterSoftwareRender);
    registerEvent(
        scheduler,
        "BeforeSoftwareFrameRender",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kBeforeSoftwareFrameRender);
    registerEvent(
        scheduler,
        "AfterSoftwareFrameRender",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kAfterSoftwareFrameRender);
    registerEvent(
        scheduler,
        "SoftwareRenderInterrupted",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kSoftwareRenderInterrupted);
    registerEvent(
        scheduler,
        "MayaInitialized",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kMayaInitialized);
    registerEvent(
        scheduler,
        "MayaExiting",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kBasicFunction,
        MSceneMessage::kMayaExiting);
    registerEvent(
        scheduler,
        "BeforeNewCheck",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kCheckFunction,
        MSceneMessage::kBeforeNewCheck);
    registerEvent(
        scheduler,
        "BeforeImportCheck",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kCheckFunction,
        MSceneMessage::kBeforeImportCheck);
    registerEvent(
        scheduler,
        "BeforeOpenCheck",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kCheckFunction,
        MSceneMessage::kBeforeOpenCheck);
    registerEvent(
        scheduler,
        "BeforeExportCheck",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kCheckFunction,
        MSceneMessage::kBeforeExportCheck);
    registerEvent(
        scheduler,
        "BeforeSaveCheck",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kCheckFunction,
        MSceneMessage::kBeforeSaveCheck);
    registerEvent(
        scheduler,
        "BeforeCreateReferenceCheck",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kCheckFunction,
        MSceneMessage::kBeforeCreateReferenceCheck);
    registerEvent(
        scheduler,
        "BeforeLoadReferenceCheck",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kCheckFunction,
        MSceneMessage::kBeforeLoadReferenceCheck);
    registerEvent(
        scheduler,
        "BeforePluginLoad",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kStringArrayFunction,
        MSceneMessage::kBeforePluginLoad);
    registerEvent(
        scheduler,
        "AfterPluginLoad",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kStringArrayFunction,
        MSceneMessage::kAfterPluginLoad);
    registerEvent(
        scheduler,
        "BeforePluginUnload",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kStringArrayFunction,
        MSceneMessage::kBeforePluginUnload);
    registerEvent(
        scheduler,
        "AfterPluginUnload",
        eventType,
        MayaMessageType::kSceneMessage,
        MayaCallbackType::kStringArrayFunction,
        MSceneMessage::kAfterPluginUnload);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initTimerMessage(MayaCallbackInfo& cbi) { }

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerTimerMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initUiMessage(MayaCallbackInfo& cbi) { }

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::registerUiMessages(
    AL::event::EventScheduler* scheduler,
    AL::event::EventType       eventType)
{
}

//----------------------------------------------------------------------------------------------------------------------
void MayaEventHandler::initEvent(MayaCallbackInfo& cbi)
{
    switch (cbi.mayaMessageType) {
    case MayaMessageType::kAnimMessage: initAnimMessage(cbi); break;
    case MayaMessageType::kCameraSetMessage: initCameraSetMessage(cbi); break;
    case MayaMessageType::kCommandMessage: initCommandMessage(cbi); break;
    case MayaMessageType::kConditionMessage: initConditionMessage(cbi); break;
    case MayaMessageType::kContainerMessage: initContainerMessage(cbi); break;
    case MayaMessageType::kDagMessage: initDagMessage(cbi); break;
    case MayaMessageType::kDGMessage: initDGMessage(cbi); break;
    case MayaMessageType::kEventMessage: initEventMessage(cbi); break;
    case MayaMessageType::kLockMessage: initLockMessage(cbi); break;
    case MayaMessageType::kModelMessage: initModelMessage(cbi); break;
    case MayaMessageType::kNodeMessage: initNodeMessage(cbi); break;
    case MayaMessageType::kObjectSetMessage: initObjectSetMessage(cbi); break;
    case MayaMessageType::kPaintMessage: initPaintMessage(cbi); break;
    case MayaMessageType::kPolyMessage: initPolyMessage(cbi); break;
    case MayaMessageType::kSceneMessage: initSceneMessage(cbi); break;
    case MayaMessageType::kTimerMessage: initTimerMessage(cbi); break;
    case MayaMessageType::kUiMessage: initUiMessage(cbi); break;
    }
}

//----------------------------------------------------------------------------------------------------------------------
AL::event::CallbackId MayaEventManager::registerCallbackInternal(
    const void*            func,
    const MayaCallbackType type,
    const char* const      eventName,
    const char* const      tag,
    const uint32_t         weight,
    void* const            userData)
{
    AL::event::EventScheduler*        scheduler = m_mayaEvents->scheduler();
    const AL::event::EventDispatcher* event = scheduler->event(eventName);
    if (!event)
        return 0;
    const AL::event::EventId id = event->eventId();

    const MayaEventHandler::MayaCallbackInfo* info = m_mayaEvents->getEventInfo(id);
    if (!info)
        return 0;

    if (info->mayaCallbackType != type) {
        std::cerr << "unable to register maya event " << eventName
                  << " - incorrect function prototype specified" << std::endl;
        return 0;
    }
    return scheduler->registerCallback(id, tag, func, weight, userData);
}

//----------------------------------------------------------------------------------------------------------------------
MayaEventHandler::~MayaEventHandler()
{
    for (auto& r : m_callbacks) {
        if (r.refCount) {
            MMessage::removeCallback(r.mayaCallback);
            r.mayaCallback = 0;
            r.refCount = 0;
        }
        m_scheduler->unregisterEvent(r.eventId);
    }
}

//----------------------------------------------------------------------------------------------------------------------
MayaEventManager* MayaEventManager::g_instance = 0;

//----------------------------------------------------------------------------------------------------------------------
static const char* const eventTypeStrings[]
    = { "unknown", "custom", "schema", "coremaya", "usdmaya" };

//----------------------------------------------------------------------------------------------------------------------
class MayaEventSystemBinding : public AL::event::EventSystemBinding
{
public:
    MayaEventSystemBinding()
        : EventSystemBinding(eventTypeStrings, sizeof(eventTypeStrings) / sizeof(const char*))
    {
    }

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
        case kInfo: MGlobal::displayInfo(text); break;
        case kWarning: MGlobal::displayWarning(text); break;
        case kError: MGlobal::displayError(text); break;
        }
    }
};

static MayaEventSystemBinding g_eventSystem;

//----------------------------------------------------------------------------------------------------------------------
MayaEventManager& MayaEventManager::instance()
{
    if (!g_instance) {
        AL::event::EventScheduler::initScheduler(&g_eventSystem);
        auto ptr = new AL::maya::event::MayaEventHandler(
            &AL::event::EventScheduler::getScheduler(), AL::event::kMayaEventType);
        new AL::maya::event::MayaEventManager(ptr);
    }
    return *g_instance;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace event
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
