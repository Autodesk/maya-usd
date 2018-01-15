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

#include <pxr/pxr.h>
#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <array>
#include "maya/MCommandMessage.h"
#include "maya/MDagMessage.h"
#include "maya/MPaintMessage.h"
#include "maya/MSceneMessage.h"

#include "AL/usdmaya/EventHandler.h"
#include "AL/events/Events.h"

PXR_NAMESPACE_USING_DIRECTIVE
namespace AL {
namespace usdmaya {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The MMessage derived class in which the callback is registered
//----------------------------------------------------------------------------------------------------------------------
enum class MayaMessageType
{
  kAnimMessage, ///< messages from the MAnimMessage class
  kCameraSetMessage, ///< messages from the MCameraSetMessage class
  kCommandMessage, ///< messages from the MCommandMessage class
  kConditionMessage, ///< messages from the MConditionMessage class
  kContainerMessage, ///< messages from the MContainerMessage class
  kDagMessage, ///< messages from the MDagMessage class
  kDGMessage, ///< messages from the MDGMessage class
  kEventMessage, ///< messages from the MEventMessage class
  kLockMessage, ///< messages from the MLockMessage class
  kModelMessage, ///< messages from the MModelMessage class
  kNodeMessage, ///< messages from the MNodeMessage class
  kObjectSetMessage, ///< messages from the MObjectSetMessage class
  kPaintMessage, ///< messages from the MPaintMessage class
  kPolyMessage, ///< messages from the MPolyMessage class
  kSceneMessage, ///< messages from the MSceneMessage class
  kTimerMessage, ///< messages from the MTimerMessage class
  kUiMessage ///< messages from the MUiMessage class
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  An enum describing which of the standard maya callback functions are to be used
//----------------------------------------------------------------------------------------------------------------------
enum class MayaCallbackType
{
  kBasicFunction, ///< \code typedef void (*MBasicFunction) (void* userData) \endcode
  kElapsedTimeFunction, ///< \code typedef void (*MElapsedTimeFunction) (float elapsedTime, float lastTime, void* userData); \endcode
  kCheckFunction, ///< \code typedef void (*MCheckFunction) (bool* retCode, void* userData); \endcode
  kCheckFileFunction, ///< \code typedef void (*MCheckFileFunction) (bool* retCode, MFileObject& file, void* userData); \endcode
  kCheckPlugFunction, ///< \code typedef void (*MCheckPlugFunction) (bool* retCode, MPlug& plug, void* userData); \endcode
  kComponentFunction, ///< \code typedef void (*MComponentFunction) (MUintArray componentIds[], unsigned int count, void* userData); \endcode
  kNodeFunction, ///< \code typedef void (*MNodeFunction) (MObject& node, void* userData); \endcode
  kStringFunction, ///< \code typedef void (*MStringFunction) (const MString& str, void* userData); \endcode
  kTwoStringFunction, ///< \code typedef void (*MTwoStringFunction) (const MString& str1, const MString& str2, void* userData); \endcode
  kThreeStringFunction, ///< \code typedef void (*MThreeStringFunction) (const MString& str1, const MString& str2, const MString& str2, void* userData); \endcode
  kStringIntBoolIntFunction, ///< \code typedef void (*MStringIntBoolIntFunction) (const MString& str, unsigned index, bool flag, unsigned type, void* userData); \endcode
  kStringIndexFunction, ///< \code typedef void (*MStringIndexFunction) (const MString& str, unsigned int index, void* userData); \endcode
  kStateFunction, ///< \code typedef void (*MStateFunction) (bool state, void* userData); \endcode
  kTimeFunction, ///< \code typedef void (*MTimeFunction) (MTime& time, void* userData); \endcode
  kPlugFunction, ///< \code typedef void (*MPlugFunction) (MPlug& srcPlug, MPlug& destPlug, bool made, void* userData); \endcode
  kNodePlugFunction, ///< \code typedef void (*MNodePlugFunction) (MObject& node, MPlug& plug, void* userData); \endcode
  kNodeStringFunction, ///< \code typedef void (*MNodeStringFunction) (MObject& node, const MString& str, void* userData); \endcode
  kNodeStringBoolFunction, ///< \code typedef void (*MNodeStringBoolFunction) (MObject& node, const MString& str, bool flag, void* userData); \endcode
  kParentChildFunction, ///< \code typedef void (*MParentChildFunction) (MDagPath& child, MDagPath& parent, void* userData); \endcode
  kModifierFunction, ///< \code typedef void (*MModifierFunction) (MDGModifier& modifier, void* userData); \endcode
  kStringArrayFunction, ///< \code typedef void (*MStringArrayFunction) (const MStringArray& strs, void* userData); \endcode
  kNodeModifierFunction, ///< \code typedef void (*MNodeModifierFunction) (MObject& node, MDGModifier& modifier, void* userData); \endcode
  kObjArrayFunction, ///< \code typedef void (*MObjArray) (MObjectArray& objects, void* userData); \endcode
  kNodeObjArrayFunction, ///< \code typedef void (*MNodeObjArray) (MObject& node, MObjectArray& objects, void* userData); \endcode
  kStringNodeFunction, ///< \code typedef void (*MStringNode) (const MString& str, MObject& node, void* userData); \endcode
  kCameraLayerFunction, ///< \code typedef void (*MCameraLayerFunction) (MObject& cameraSetNode, unsigned multiIndex, bool added, void* userData); \endcode
  kCameraLayerCameraFunction, ///< \code typedef void (*MCameraLayerCameraFunction) (MObject& cameraSetNode, unsigned multiIndex, MObject& oldCamera, MObject& newCamera, void* userData); \endcode
  kConnFailFunction, ///< \code typedef void (*MConnFailFunction) (MPlug& srcPlug, MPlug& destPlug, const MString& srcPlugName, const MString& dstPlugName, void* userData); \endcode
  kPlugsDGModFunction, ///< \code typedef void (*MPlugsDGModFunction) (MPlugArray& plugs, MDGModifier& modifier, void* userData); \endcode
  kNodeUuidFunction, ///< \code typedef void (*MNodeUuidFunction) (MObject& node, const MUuid& uuid, void* userData); \endcode
  kCheckNodeUuidFunction, ///< \code typedef Action (*MCheckNodeUuidFunction) (bool doAction, MObject& node, MUuid& uuid, void* userData); \endcode
  kObjectFileFunction, ///< \code typedef void (*MObjectFileFunction) (const MObject& object, const MFileObject& file, void* userData); \endcode
  kCheckObjectFileFunction, ///< \code typedef void (*MCheckObjectFileFunction) (bool* retCode, const MObject& referenceNode, MFileObject& file, void* userData); \endcode
  kRenderTileFunction, ///< \code typedef void (*MRenderTileFunction) (int originX, int originY, int tileMaxX, int tileMaxY, void* userData); \endcode
  kMessageFunction, ///< \code typedef void (*MMessageFunction) (const MString& message, MCommandMessage::MessageType messageType, void* userData); \endcode
  kMessageFilterFunction, ///< \code typedef void (*MMessageFilterFunction) (const MString& message, MCommandMessage::MessageType messageType, bool& filterOutput, void* userData); \endcode
  kMessageParentChildFunction, ///< \code typedef void (*MMessageParentChildFunction) (MDagMessage::DagMessage msgType, MDagPath& child, MDagPath& parent, void* userData); \endcode
  kPathObjectPlugColoursFunction, ///< \code typedef void (*MPathObjectPlugColorsFunction) (MDagPath& path, MObject& object, MPlug& plug, MColorArray& colors, void* userData); \endcode
  kWorldMatrixModifiedFunction ///< \code typedef void (*MWorldMatrixModifiedFunction) (MObject& transformNode, MDagMessage::MatrixModifiedFlags& modified, void* userData); \endcode
};

//----------------------------------------------------------------------------------------------------------------------
namespace AnimMessage {
enum Type
{
  kAnimCurveEdited,
  kAnimKeyFrameEdited,
  kNodeAnimKeyframeEdited, ///< unsupported
  kAnimKeyframeEditCheck,
  kPreBakeResults,
  kPostBakeResults,
  kDisableImplicitControl
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace CameraSetMessage {
enum Type
{
  kCameraLayer,  ///< MCamerSetMessage::addCameraLayerCallback
  kCameraChanged ///< MCamerSetMessage::addCameraChangedCallback
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace CommandMessage {
enum Type
{
  kCommand,  ///< MCommandMessage::addCommandCallback
  kCommandOuptut, ///< MCommandMessage::addCommandOutputCallback
  kCommandOutputFilter, ///< MCommandMessage::addCommandOutputFilterCallback
  kProc ///< MCommandMessage::addProcCallback
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace ConditionMessage {
enum Type
{
  kCondition  ///< unsupported
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace ContainerMessage {
enum Type
{
  kPublishAttr, ///< MContainerMessage::addPublishAttrCallback
  kBoundAttr ///< MContainerMessage::addBoundAttrCallback
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace DagMessage {
enum Type
{
  kParentAdded, ///< MDagMessage::addParentAddedCallback
  kParentAddedDagPath, ///< MDagMessage::addParentAddedDagPathCallback  - unsupported
  kParentRemoved, ///< MDagMessage::addParentRemovedCallback
  kParentRemovedDagPath, ///< MDagMessage::addParentRemovedDagPathCallback  - unsupported
  kChildAdded, ///< MDagMessage::addChildAddedCallback
  kChildAddedDagPath, ///< MDagMessage::addChildAddedDagPathCallback  - unsupported
  kChildRemoved, ///< MDagMessage::addChildRemovedCallback
  kChildRemovedDagPath, ///< MDagMessage::addChildRemovedDagPathCallback  - unsupported
  kChildReordered, ///< MDagMessage::addChildReorderedCallback
  kChildReorderedDagPath, ///< MDagMessage::addChildReorderedDagPathCallback  - unsupported
  kDag, ///< MDagMessage::addDagCallback  - unsupported
  kDagDagPath, ///< MDagMessage::addDagDagPathCallback  - unsupported
  kAllDagChanges, ///< MDagMessage::addAllDagChangesCallback
  kAllDagChangesDagPath, ///< MDagMessage::addAllDagChangesDagPathCallback  - unsupported
  kInstanceAdded, ///< MDagMessage::addInstanceAddedCallback
  kInstanceAddedDagPath, ///< MDagMessage::addInstanceAddedDagPathCallback  - unsupported
  kInstanceRemoved, ///< MDagMessage::addInstanceRemovedCallback
  kInstanceRemovedDagPath, ///< MDagMessage::addInstanceRemovedDagPathCallback  - unsupported
  kWorldMatrixModified, ///< MDagMessage::addWorldMatrixModifiedCallback  - unsupported
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace DGMessage {
enum Type
{
  kTimeChange, ///< DGMessage::addTimeChangeCallback
  kDelayedTimeChange, ///< DGMessage::addDelayedTimeChangeCallback
  kDelayedTimeChangeRunup, ///< DGMessage::addDelayedTimeChangeRunupCallback
  kForceUpdate, ///< DGMessage::addForceUpdateCallback
  kNodeAdded, ///< DGMessage::addNodeAddedCallback
  kNodeRemoved, ///< DGMessage::addNodeRemovedCallback
  kConnection, ///< DGMessage::addConnectionCallback
  kPreConnection, ///< DGMessage::addPreConnectionCallback
  kNodeChangeUuidCheck, ///< DGMessage::addNodeChangeUuidCheckCallback - unsupported
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace EventMessage {
enum Type
{
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace LockMessage {
enum Type
{
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace ModelMessage {
enum Type
{
  kCallback, ///< DGMessage::addCallback
  kBeforeDuplicate, ///< DGMessage::addBeforeDuplicateCallback
  kAfterDuplicate, ///< DGMessage::addAfterDuplicateCallback
  kNodeAddedToModel, ///< DGMessage::addNodeAddedToModelCallback   - upsupported
  kNodeRemovedFromModel, ///< DGMessage::addNodeRemovedFromModelCallback   - upsupported
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace NodeMessage {
enum Type
{
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace ObjectSetMessage {
enum Type
{
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace PaintMessage {
enum Type
{
  kVertexColor, ///< MPaintMessage::addVertexColorCallback
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace PolyMessage {
enum Type
{
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace SceneMessage {
enum Type
{
  kSceneUpdate = MSceneMessage::kSceneUpdate,
  kBeforeNew = MSceneMessage::kBeforeNew,
  kAfterNew = MSceneMessage::kAfterNew,
  kBeforeImport = MSceneMessage::kBeforeImport,
  kAfterImport = MSceneMessage::kAfterImport,
  kBeforeOpen = MSceneMessage::kBeforeOpen,
  kAfterOpen = MSceneMessage::kAfterOpen,
  kBeforeFileRead = MSceneMessage::kBeforeFileRead,
  kAfterFileRead = MSceneMessage::kAfterFileRead,
  kAfterSceneReadAndRecordEdits = MSceneMessage::kAfterSceneReadAndRecordEdits,
  kBeforeExport = MSceneMessage::kBeforeExport,
  kExportStarted = MSceneMessage::kExportStarted,
  kAfterExport = MSceneMessage::kAfterExport,
  kBeforeSave = MSceneMessage::kBeforeSave,
  kAfterSave = MSceneMessage::kAfterSave,
  kBeforeCreateReference = MSceneMessage::kBeforeCreateReference,
  kBeforeLoadReferenceAndRecordEdits = MSceneMessage::kBeforeLoadReferenceAndRecordEdits,
  kAfterCreateReference = MSceneMessage::kAfterCreateReference,
  kAfterCreateReferenceAndRecordEdits = MSceneMessage::kAfterCreateReferenceAndRecordEdits,
  kBeforeRemoveReference = MSceneMessage::kBeforeRemoveReference,
  kAfterRemoveReference = MSceneMessage::kAfterRemoveReference,
  kBeforeImportReference = MSceneMessage::kBeforeImportReference,
  kAfterImportReference = MSceneMessage::kAfterImportReference,
  kBeforeExportReference = MSceneMessage::kBeforeExportReference,
  kAfterExportReference = MSceneMessage::kAfterExportReference,
  kBeforeUnloadReference = MSceneMessage::kBeforeUnloadReference,
  kAfterUnloadReference = MSceneMessage::kAfterUnloadReference,
  kBeforeLoadReference = MSceneMessage::kBeforeLoadReference,
  kBeforeCreateReferenceAndRecordEdits = MSceneMessage::kBeforeCreateReferenceAndRecordEdits,
  kAfterLoadReference = MSceneMessage::kAfterLoadReference,
  kAfterLoadReferenceAndRecordEdits = MSceneMessage::kAfterLoadReferenceAndRecordEdits,
  kBeforeSoftwareRender = MSceneMessage::kBeforeSoftwareRender,
  kAfterSoftwareRender = MSceneMessage::kAfterSoftwareRender,
  kBeforeSoftwareFrameRender = MSceneMessage::kBeforeSoftwareFrameRender,
  kAfterSoftwareFrameRender = MSceneMessage::kAfterSoftwareFrameRender,
  kSoftwareRenderInterrupted = MSceneMessage::kSoftwareRenderInterrupted,
  kMayaInitialized = MSceneMessage::kMayaInitialized,
  kMayaExiting = MSceneMessage::kMayaExiting,
  kBeforeNewCheck = MSceneMessage::kBeforeNewCheck,
  kBeforeImportCheck = MSceneMessage::kBeforeImportCheck,
  kBeforeOpenCheck = MSceneMessage::kBeforeOpenCheck,
  kBeforeExportCheck = MSceneMessage::kBeforeExportCheck,
  kBeforeSaveCheck = MSceneMessage::kBeforeSaveCheck,
  kBeforeCreateReferenceCheck = MSceneMessage::kBeforeCreateReferenceCheck,
  kBeforeLoadReferenceCheck = MSceneMessage::kBeforeLoadReferenceCheck,
  kBeforePluginLoad = MSceneMessage::kBeforePluginLoad,
  kAfterPluginLoad = MSceneMessage::kAfterPluginLoad,
  kBeforePluginUnload = MSceneMessage::kBeforePluginUnload,
  kAfterPluginUnload = MSceneMessage::kAfterPluginUnload,
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace TimerMessage {
enum Type
{
};
}

//----------------------------------------------------------------------------------------------------------------------
namespace UiMessage {
enum Type
{
};
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  An interface that provides the event system with some utilities from the underlying DCC application.
///         This class is responsible for keeping track of the number of maya events registered, and creating/destroying
///         the MMessage callbacks.
/// \ingroup events
//----------------------------------------------------------------------------------------------------------------------
class MayaEventHandler
  : public CustomEventHandler
{
  typedef std::unordered_map<EventId, uint32_t> EventToMaya;
public:

  /// \brief  a structure to store some binding info for the MMessage event info
  struct MayaCallbackInfo
  {
    EventId eventId; ///< the event id from the event scheduler
    uint32_t refCount; ///< the ref count for this callback
    MayaMessageType mayaMessageType; ///< the maya MMessage class that defines the message
    MayaCallbackType mayaCallbackType; ///< the type of C callback function needed to execute the callback
    uint32_t mmessageEnum; ///< the enum value from one of the MSceneMessage / MEventMessage etc classes.
    MCallbackId mayaCallback; ///< the maya callback ID
  };

  /// \brief  constructor function
  /// \param  scheduler the event scheduler
  /// \param  eventType the event type
  MayaEventHandler(EventScheduler* scheduler, EventType eventType);

  /// \brief  dtor
  ~MayaEventHandler();

  /// \brief  returns the event type string
  /// \return "maya"
  const char* eventTypeString() const override
    { return "maya"; }

  /// \brief  returns the event scheduler
  /// \return the event scheduler used for these maya events
  EventScheduler* scheduler() const
    { return m_scheduler; }

  /// \brief  queries the maya event information for the specified maya event
  /// \param  event the event ID
  /// \return a pointer to the maya event information (or null for an invalid event)
  const MayaCallbackInfo* getEventInfo(const EventId event) const
  {
    const auto it = m_eventMapping.find(event);
    if(it == m_eventMapping.end()) return 0;
    return m_callbacks.data() + it->second;
  }

  /// \brief  queries whether the event has an associated MCallbackId (indicating the callback is active with maya)
  /// \param  event the event to query
  /// \return true if callback is active with maya, false otherwise
  bool isMayaCallbackRegistered(const EventId event) const
  {
    const MayaCallbackInfo* cbi = getEventInfo(event);
    return cbi ? cbi->refCount != 0 : false;
  }

  /// \brief  queries the maya event information for the specified maya event
  /// \param  event the event ID
  /// \return a pointer to the maya event information (or null for an invalid event)
  const MayaCallbackInfo* getEventInfo(const char* const eventName) const
  {
    const EventDispatcher* const dispatcher = m_scheduler->event(eventName);
    if(dispatcher)
    {
      return getEventInfo(dispatcher->eventId());
    }
    return 0;
  }

  /// \brief  queries whether the event has an associated MCallbackId (indicating the callback is active with maya)
  /// \param  event the event to query
  /// \return true if callback is active with maya, false otherwise
  bool isMayaCallbackRegistered(const char* const eventName) const
  {
    const MayaCallbackInfo* cbi = getEventInfo(eventName);
    return cbi ? cbi->refCount != 0 : false;
  }

private:
  void onCallbackCreated(const CallbackId callbackId) override;
  void onCallbackDestroyed(const CallbackId callbackId) override;
  bool registerEvent(
      EventScheduler* scheduler,
      const char* eventName,
      EventType eventType,
      MayaMessageType messageType,
      MayaCallbackType callbackType,
      uint32_t mmessageEnum);
  void initEvent(MayaCallbackInfo& cbi);
  void initAnimMessage(MayaCallbackInfo& cbi);
  void initCameraSetMessage(MayaCallbackInfo& cbi);
  void initCommandMessage(MayaCallbackInfo& cbi);
  void initConditionMessage(MayaCallbackInfo& cbi);
  void initContainerMessage(MayaCallbackInfo& cbi);
  void initDagMessage(MayaCallbackInfo& cbi);
  void initDGMessage(MayaCallbackInfo& cbi);
  void initEventMessage(MayaCallbackInfo& cbi);
  void initLockMessage(MayaCallbackInfo& cbi);
  void initModelMessage(MayaCallbackInfo& cbi);
  void initNodeMessage(MayaCallbackInfo& cbi);
  void initObjectSetMessage(MayaCallbackInfo& cbi);
  void initPaintMessage(MayaCallbackInfo& cbi);
  void initPolyMessage(MayaCallbackInfo& cbi);
  void initSceneMessage(MayaCallbackInfo& cbi);
  void initTimerMessage(MayaCallbackInfo& cbi);
  void initUiMessage(MayaCallbackInfo& cbi);
  void registerAnimMessages(EventScheduler* scheduler, EventType eventType);
  void registerCameraSetMessages(EventScheduler* scheduler, EventType eventType);
  void registerCommandMessages(EventScheduler* scheduler, EventType eventType);
  void registerConditionMessages(EventScheduler* scheduler, EventType eventType);
  void registerContainerMessages(EventScheduler* scheduler, EventType eventType);
  void registerDagMessages(EventScheduler* scheduler, EventType eventType);
  void registerDGMessages(EventScheduler* scheduler, EventType eventType);
  void registerEventMessages(EventScheduler* scheduler, EventType eventType);
  void registerLockMessages(EventScheduler* scheduler, EventType eventType);
  void registerModelMessages(EventScheduler* scheduler, EventType eventType);
  void registerNodeMessages(EventScheduler* scheduler, EventType eventType);
  void registerObjectSetMessages(EventScheduler* scheduler, EventType eventType);
  void registerPaintMessages(EventScheduler* scheduler, EventType eventType);
  void registerPolyMessages(EventScheduler* scheduler, EventType eventType);
  void registerSceneMessages(EventScheduler* scheduler, EventType eventType);
  void registerTimerMessages(EventScheduler* scheduler, EventType eventType);
  void registerUiMessages(EventScheduler* scheduler, EventType eventType);
  std::vector<MayaCallbackInfo> m_callbacks;
  EventToMaya m_eventMapping;
  EventScheduler* m_scheduler;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class that replaces the MMessage (and derived message classes) in order to provide a level of scheduling
///         and debugability across multiple plugins, which is not possible in the standard maya API. Effectively the
///         class wraps the core EventScheduler API to provide a little bit of additional type safety on the callback
///         type provided.
//----------------------------------------------------------------------------------------------------------------------
class MayaEventManager
{
  static MayaEventManager* g_instance;
public:

  /// \brief  returns the global maya event manager instance
  static MayaEventManager& instance()
    { return *g_instance; }

  /// \brief  constructor
  /// \param  mayaEvents the custom event handler
  MayaEventManager(MayaEventHandler* mayaEvents)
    : m_mayaEvents(mayaEvents) { g_instance = this; }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MBasicFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kBasicFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MElapsedTimeFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kElapsedTimeFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MCheckFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kCheckFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MCheckFileFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kCheckFileFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MCheckPlugFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kCheckPlugFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MComponentFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kComponentFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MNodeFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kNodeFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MStringFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kStringFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MTwoStringFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kTwoStringFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MThreeStringFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kThreeStringFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MStringIntBoolIntFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kStringIntBoolIntFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MStringIndexFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kStringIndexFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MNodeStringBoolFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kNodeStringBoolFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MStateFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kStateFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MTimeFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kTimeFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MPlugFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kPlugFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MNodePlugFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kNodePlugFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MNodeStringFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kNodeStringFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MParentChildFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kParentChildFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MModifierFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kModifierFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MStringArrayFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kStringArrayFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MNodeModifierFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kNodeModifierFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MObjArray func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kObjArrayFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MNodeObjArray func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kNodeObjArrayFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MStringNode func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kStringNodeFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MCameraLayerFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kCameraLayerFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MCameraLayerCameraFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kCameraLayerCameraFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MConnFailFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kConnFailFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MPlugsDGModFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kPlugsDGModFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MNodeUuidFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kNodeUuidFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MCheckNodeUuidFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kCheckNodeUuidFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MObjectFileFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kObjectFileFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MCheckObjectFileFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kCheckObjectFileFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MMessage::MRenderTileFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kRenderTileFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MCommandMessage::MMessageFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kMessageFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MCommandMessage::MMessageFilterFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kMessageFilterFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MDagMessage::MMessageParentChildFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kMessageParentChildFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MDagMessage::MWorldMatrixModifiedFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kWorldMatrixModifiedFunction, eventName, tag, weight, userData); }

  /// \brief  registers a C++ callback against a maya event
  /// \param  func the C++ function
  /// \param  eventName the event
  /// \param  tag the unique tag for the callback
  /// \param  weight the weight (lower weights at executed before higher weights)
  /// \param  userData custom user data pointer
  /// \return the callback id
  CallbackId registerCallback(MPaintMessage::MPathObjectPlugColorsFunction func, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0)
    { return registerCallbackInternal((void*)func, MayaCallbackType::kPathObjectPlugColoursFunction, eventName, tag, weight, userData); }

  /// \brief  unregisters the callback id
  /// \param  id the callback id to unregister
  bool unregisterCallback(CallbackId id)
    {
      EventScheduler* scheduler = m_mayaEvents->scheduler();
      return scheduler->unregisterCallback(id);
    }

  /// \brief  returns the custom maya event handler
  /// \return the event handler in charge of registering and unregistering maya events
  MayaEventHandler* mayaEventsHandler() const
    { return m_mayaEvents; }

private:
  CallbackId registerCallbackInternal(const void* func, MayaCallbackType type, const char* const eventName, const char* const tag, uint32_t weight, void* userData = 0);
  MayaEventHandler* m_mayaEvents;
};


//----------------------------------------------------------------------------------------------------------------------
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

