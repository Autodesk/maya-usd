/**
 *
\defgroup  mayaevents  Maya Event System
\ingroup  mayautils

\section events_MayaEvents Maya Event System

As a direct replacement to MMessage (and related classes) the class AL::maya::MayaEventManager
provides an interface to register your own C++ callback functions. All of the static methods
AL::maya::MayaEventManager::registerCallback take the following arguments:

\li func - the C++ function pointer
\li eventName - the name of the the event (see list below)
\li tag - a unique a tag string to identify the creator of the callback
\li weight - the event weight (lowest weights are executed first, highest last, all usdmaya weights
are 0x1000) \li userData - an optional user data pointer

The list of registered event names can be queried by running the mel command:

\code
print `AL_usdmaya_ListEvents`;
\endcode

The following table lists the registered event name, and the type of callback function required to
handle that callback.

\li \b "AnimCurveEdited" - AL::maya::event::MayaCallbackType::kObjArrayFunction
\li \b "AnimKeyFrameEdited" - AL::maya::event::MayaCallbackType::kObjArrayFunction
\li \b "AnimKeyframeEditCheck" - AL::maya::event::MayaCallbackType::kCheckPlugFunction
\li \b "PreBakeResults" - AL::maya::event::MayaCallbackType::kPlugsDGModFunction
\li \b "PostBakeResults" - AL::maya::event::MayaCallbackType::kPlugsDGModFunction
\li \b "DisableImplicitControl" - AL::maya::event::MayaCallbackType::kPlugsDGModFunction
\li \b "CameraLayer" - AL::maya::event::MayaCallbackType::kCameraLayerFunction
\li \b "CameraChanged" - AL::maya::event::MayaCallbackType::kCameraLayerFunction
\li \b "Command" - AL::maya::event::MayaCallbackType::kStringFunction
\li \b "CommandOuptut" - AL::maya::event::MayaCallbackType::kMessageFunction
\li \b "CommandOutputFilter" - AL::maya::event::MayaCallbackType::kMessageFilterFunction
\li \b "Proc" - AL::maya::event::MayaCallbackType::kStringIntBoolIntFunction
\li \b "PublishAttr" - AL::maya::event::MayaCallbackType::kNodeStringBoolFunction
\li \b "BoundAttr" - AL::maya::event::MayaCallbackType::kNodeStringBoolFunction
\li \b "ParentAdded" - AL::maya::event::MayaCallbackType::kParentChildFunction
\li \b "ParentRemoved" - AL::maya::event::MayaCallbackType::kParentChildFunction
\li \b "ChildAdded" - AL::maya::event::MayaCallbackType::kParentChildFunction
\li \b "ChildRemoved" - AL::maya::event::MayaCallbackType::kParentChildFunction
\li \b "ChildReordered" - AL::maya::event::MayaCallbackType::kParentChildFunction
\li \b "AllDagChanges" - AL::maya::event::MayaCallbackType::kMessageParentChildFunction
\li \b "InstanceAdded" - AL::maya::event::MayaCallbackType::kParentChildFunction
\li \b "InstanceRemoved" - AL::maya::event::MayaCallbackType::kParentChildFunction
\li \b "TimeChange" - AL::maya::event::MayaCallbackType::kTimeFunction
\li \b "DelayedTimeChange" - AL::maya::event::MayaCallbackType::kTimeFunction
\li \b "DelayedTimeChangeRunup" - AL::maya::event::MayaCallbackType::kTimeFunction
\li \b "ForceUpdate" - AL::maya::event::MayaCallbackType::kTimeFunction
\li \b "NodeAdded" - AL::maya::event::MayaCallbackType::kNodeFunction
\li \b "NodeRemoved" - AL::maya::event::MayaCallbackType::kNodeFunction
\li \b "Connection" - AL::maya::event::MayaCallbackType::kPlugFunction
\li \b "PreConnection" - AL::maya::event::MayaCallbackType::kPlugFunction
\li \b "Callback" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeDuplicate" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterDuplicate" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "VertexColor" - AL::maya::event::MayaCallbackType::kPathObjectPlugColoursFunction
\li \b "SceneUpdate" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeNew" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterNew" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeImport" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterImport" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeOpen" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterOpen" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeFileRead" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterFileRead" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterSceneReadAndRecordEdits" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeExport" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "ExportStarted" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterExport" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeSave" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterSave" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeCreateReference" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeLoadReferenceAndRecordEdits" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterCreateReference" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterCreateReferenceAndRecordEdits" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeRemoveReference" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterRemoveReference" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeImportReference" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterImportReference" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeExportReference" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterExportReference" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeUnloadReference" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterUnloadReference" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeLoadReference" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeCreateReferenceAndRecordEdits" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterLoadReference" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterLoadReferenceAndRecordEdits" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeSoftwareRender" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterSoftwareRender" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeSoftwareFrameRender" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "AfterSoftwareFrameRender" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "SoftwareRenderInterrupted" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "MayaInitialized" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "MayaExiting" - AL::maya::event::MayaCallbackType::kBasicFunction
\li \b "BeforeNewCheck" - AL::maya::event::MayaCallbackType::kCheckFunction
\li \b "BeforeImportCheck" - AL::maya::event::MayaCallbackType::kCheckFunction
\li \b "BeforeOpenCheck" - AL::maya::event::MayaCallbackType::kCheckFunction
\li \b "BeforeExportCheck" - AL::maya::event::MayaCallbackType::kCheckFunction
\li \b "BeforeSaveCheck" - AL::maya::event::MayaCallbackType::kCheckFunction
\li \b "BeforeCreateReferenceCheck" - AL::maya::event::MayaCallbackType::kCheckFunction
\li \b "BeforeLoadReferenceCheck" - AL::maya::event::MayaCallbackType::kCheckFunction
\li \b "BeforePluginLoad" - AL::maya::event::MayaCallbackType::kStringArrayFunction
\li \b "AfterPluginLoad" - AL::maya::event::MayaCallbackType::kStringArrayFunction
\li \b "BeforePluginUnload" - AL::maya::event::MayaCallbackType::kStringArrayFunction
\li \b "AfterPluginUnload" - AL::maya::event::MayaCallbackType::kStringArrayFunction

\section mayaevent_example example code

A quick example of replacing a MSceneMessage::kAfterNew message with the events system

\code

#include "AL/maya/event/MayaEventManager.h"

#include <maya/MFnPlugin.h>

#include <iostream>

void onFileNewCallback()
{
  std::cout << "onFileNewCallback Callback called!" << std::endl;
}

AL::maya::CallbackId g_callback = 0;

MStatus initialisePlugin(MObject obj)
{
  MFnPlugin fn(obj);

  // the params are:
  //
  // * the callback function pointer
  // * the name of the event
  // * A unique tag to identify your callback
  // * the callback weight
  // * a custom userdata pointer
  //
  g_callback = AL::maya::event::MayaEventManager::registerCallback(
    onFileNewCallback,
    "AfterNew",
    "MyPlugin_MyCallback",
    99999,
    nullptr);

  return MS::kSuccess;
}

MStatus uninitialisePlugin(MObject obj)
{
  MFnPlugin fn(obj);

  // and to unregister the callback
  AL::maya::event::MayaEventManager::registerCallback(g_callback);

  return MS::kSuccess;
}

\endcode


*/
