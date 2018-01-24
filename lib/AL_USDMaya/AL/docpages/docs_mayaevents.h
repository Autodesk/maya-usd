/**
 *
\defgroup  mayaevents  Maya Event System
\ingroup  mayautils

\section events_MayaEvents Maya Event System

As a direct replacement to MMessage (and related classes) the class AL::maya::MayaEventManager provides an interface
to register your own C++ callback functions. All of the static methods AL::maya::MayaEventManager::registerCallback take
the following arguments:

\li func - the C++ function pointer
\li eventName - the name of the the event (see list below)
\li tag - a unique a tag string to identify the creator of the callback
\li weight - the event weight (lowest weights are executed first, highest last, all usdmaya weights are 0x1000)
\li userData - an optional user data pointer

The list of registered event names can be queried by running the mel command:

\code
print `AL_usdmaya_ListEvents`;
\endcode

The following table lists the registered event name, and the type of callback function required to handle that callback.

\li \b "AnimCurveEdited" - AL::maya::MayaCallbackType::kObjArrayFunction
\li \b "AnimKeyFrameEdited" - AL::maya::MayaCallbackType::kObjArrayFunction
\li \b "AnimKeyframeEditCheck" - AL::maya::MayaCallbackType::kCheckPlugFunction
\li \b "PreBakeResults" - AL::maya::MayaCallbackType::kPlugsDGModFunction
\li \b "PostBakeResults" - AL::maya::MayaCallbackType::kPlugsDGModFunction
\li \b "DisableImplicitControl" - AL::maya::MayaCallbackType::kPlugsDGModFunction
\li \b "CameraLayer" - AL::maya::MayaCallbackType::kCameraLayerFunction
\li \b "CameraChanged" - AL::maya::MayaCallbackType::kCameraLayerFunction
\li \b "Command" - AL::maya::MayaCallbackType::kStringFunction
\li \b "CommandOuptut" - AL::maya::MayaCallbackType::kMessageFunction
\li \b "CommandOutputFilter" - AL::maya::MayaCallbackType::kMessageFilterFunction
\li \b "Proc" - AL::maya::MayaCallbackType::kStringIntBoolIntFunction
\li \b "PublishAttr" - AL::maya::MayaCallbackType::kNodeStringBoolFunction
\li \b "BoundAttr" - AL::maya::MayaCallbackType::kNodeStringBoolFunction
\li \b "ParentAdded" - AL::maya::MayaCallbackType::kParentChildFunction
\li \b "ParentRemoved" - AL::maya::MayaCallbackType::kParentChildFunction
\li \b "ChildAdded" - AL::maya::MayaCallbackType::kParentChildFunction
\li \b "ChildRemoved" - AL::maya::MayaCallbackType::kParentChildFunction
\li \b "ChildReordered" - AL::maya::MayaCallbackType::kParentChildFunction
\li \b "AllDagChanges" - AL::maya::MayaCallbackType::kMessageParentChildFunction
\li \b "InstanceAdded" - AL::maya::MayaCallbackType::kParentChildFunction
\li \b "InstanceRemoved" - AL::maya::MayaCallbackType::kParentChildFunction
\li \b "TimeChange" - AL::maya::MayaCallbackType::kTimeFunction
\li \b "DelayedTimeChange" - AL::maya::MayaCallbackType::kTimeFunction
\li \b "DelayedTimeChangeRunup" - AL::maya::MayaCallbackType::kTimeFunction
\li \b "ForceUpdate" - AL::maya::MayaCallbackType::kTimeFunction
\li \b "NodeAdded" - AL::maya::MayaCallbackType::kNodeFunction
\li \b "NodeRemoved" - AL::maya::MayaCallbackType::kNodeFunction
\li \b "Connection" - AL::maya::MayaCallbackType::kPlugFunction
\li \b "PreConnection" - AL::maya::MayaCallbackType::kPlugFunction
\li \b "Callback" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeDuplicate" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterDuplicate" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "VertexColor" - AL::maya::MayaCallbackType::kPathObjectPlugColoursFunction
\li \b "SceneUpdate" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeNew" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterNew" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeImport" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterImport" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeOpen" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterOpen" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeFileRead" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterFileRead" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterSceneReadAndRecordEdits" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeExport" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "ExportStarted" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterExport" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeSave" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterSave" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeCreateReference" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeLoadReferenceAndRecordEdits" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterCreateReference" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterCreateReferenceAndRecordEdits" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeRemoveReference" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterRemoveReference" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeImportReference" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterImportReference" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeExportReference" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterExportReference" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeUnloadReference" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterUnloadReference" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeLoadReference" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeCreateReferenceAndRecordEdits" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterLoadReference" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterLoadReferenceAndRecordEdits" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeSoftwareRender" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterSoftwareRender" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeSoftwareFrameRender" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "AfterSoftwareFrameRender" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "SoftwareRenderInterrupted" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "MayaInitialized" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "MayaExiting" - AL::maya::MayaCallbackType::kBasicFunction
\li \b "BeforeNewCheck" - AL::maya::MayaCallbackType::kCheckFunction
\li \b "BeforeImportCheck" - AL::maya::MayaCallbackType::kCheckFunction
\li \b "BeforeOpenCheck" - AL::maya::MayaCallbackType::kCheckFunction
\li \b "BeforeExportCheck" - AL::maya::MayaCallbackType::kCheckFunction
\li \b "BeforeSaveCheck" - AL::maya::MayaCallbackType::kCheckFunction
\li \b "BeforeCreateReferenceCheck" - AL::maya::MayaCallbackType::kCheckFunction
\li \b "BeforeLoadReferenceCheck" - AL::maya::MayaCallbackType::kCheckFunction
\li \b "BeforePluginLoad" - AL::maya::MayaCallbackType::kStringArrayFunction
\li \b "AfterPluginLoad" - AL::maya::MayaCallbackType::kStringArrayFunction
\li \b "BeforePluginUnload" - AL::maya::MayaCallbackType::kStringArrayFunction
\li \b "AfterPluginUnload" - AL::maya::MayaCallbackType::kStringArrayFunction

\section mayaevent_example example code

A quick example of replacing a MSceneMessage::kAfterNew message with the events system

\code

#include <maya/MFnPlugin.h>
#include "AL/maya/MayaEventManager.h"
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
  g_callback = AL::maya::MayaEventManager::registerCallback(
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
  AL::maya::MayaEventManager::registerCallback(g_callback);

  return MS::kSuccess;
}

\endcode


*/
