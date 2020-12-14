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
#pragma once

#include "AL/event/EventHandler.h"
#include "AL/maya/event/Api.h"

#include <maya/MCommandMessage.h>
#include <maya/MDagMessage.h>
#include <maya/MPaintMessage.h>
#include <maya/MSceneMessage.h>

namespace AL {
namespace maya {
namespace event {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The MMessage derived class in which the callback is registered
/// \ingroup mayaevents
//----------------------------------------------------------------------------------------------------------------------
enum class MayaMessageType
{
    kAnimMessage,      ///< messages from the MAnimMessage class
    kCameraSetMessage, ///< messages from the MCameraSetMessage class
    kCommandMessage,   ///< messages from the MCommandMessage class
    kConditionMessage, ///< messages from the MConditionMessage class
    kContainerMessage, ///< messages from the MContainerMessage class
    kDagMessage,       ///< messages from the MDagMessage class
    kDGMessage,        ///< messages from the MDGMessage class
    kEventMessage,     ///< messages from the MEventMessage class
    kLockMessage,      ///< messages from the MLockMessage class
    kModelMessage,     ///< messages from the MModelMessage class
    kNodeMessage,      ///< messages from the MNodeMessage class
    kObjectSetMessage, ///< messages from the MObjectSetMessage class
    kPaintMessage,     ///< messages from the MPaintMessage class
    kPolyMessage,      ///< messages from the MPolyMessage class
    kSceneMessage,     ///< messages from the MSceneMessage class
    kTimerMessage,     ///< messages from the MTimerMessage class
    kUiMessage         ///< messages from the MUiMessage class
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  An enum describing which of the standard maya callback functions are to be used for a
/// given callback. This
///         is primarily used as a runtime check to ensure the function prototype you are binding to
///         a given event is of the correct type.
/// \ingroup mayaevents
//----------------------------------------------------------------------------------------------------------------------
enum class MayaCallbackType
{
    kBasicFunction, ///< \brief describes the maya callback type \b MMessage::MBasicFunction \code
                    ///< typedef void (*MBasicFunction) (void* userData) \endcode
    kElapsedTimeFunction, ///< \brief describes the maya callback type \b
                          ///< MMessage::MElapsedTimeFunction \code typedef void
                          ///< (*MElapsedTimeFunction) (float elapsedTime, float lastTime, void*
                          ///< userData); \endcode
    kCheckFunction, ///< \brief describes the maya callback type \b MMessage::MCheckFunction \code
                    ///< typedef void (*MCheckFunction) (bool* retCode, void* userData); \endcode
    kCheckFileFunction, ///< \brief describes the maya callback type \b MMessage::MCheckFileFunction
                        ///< \code typedef void (*MCheckFileFunction) (bool* retCode, MFileObject&
                        ///< file, void* userData); \endcode
    kCheckPlugFunction, ///< \brief describes the maya callback type \b MMessage::MCheckPlugFunction
                        ///< \code typedef void (*MCheckPlugFunction) (bool* retCode, MPlug& plug,
                        ///< void* userData); \endcode
    kComponentFunction, ///< \brief describes the maya callback type \b MMessage::MComponentFunction
                        ///< \code typedef void (*MComponentFunction) (MUintArray componentIds[],
                        ///< unsigned int count, void* userData); \endcode
    kNodeFunction,   ///< \brief describes the maya callback type \b MMessage::MNodeFunction \code
                     ///< typedef void (*MNodeFunction) (MObject& node, void* userData); \endcode
    kStringFunction, ///< \brief describes the maya callback type \b MMessage::MStringFunction \code
                     ///< typedef void (*MStringFunction) (const MString& str, void* userData);
                     ///< \endcode
    kTwoStringFunction, ///< \brief describes the maya callback type \b MMessage::MTwoStringFunction
                        ///< \code typedef void (*MTwoStringFunction) (const MString& str1, const
                        ///< MString& str2, void* userData); \endcode
    kThreeStringFunction, ///< \brief describes the maya callback type \b
                          ///< MMessage::MThreeStringFunction \code typedef void
                          ///< (*MThreeStringFunction) (const MString& str1, const MString& str2,
                          ///< const MString& str2, void* userData); \endcode
    kStringIntBoolIntFunction, ///< \brief describes the maya callback type \b
                               ///< MMessage::MStringIntBoolIntFunction \code typedef void
                               ///< (*MStringIntBoolIntFunction) (const MString& str, unsigned
                               ///< index, bool flag, unsigned type, void* userData); \endcode
    kStringIndexFunction,      ///< \brief describes the maya callback type \b
                               ///< MMessage::MStringIndexFunction \code typedef void
                               ///< (*MStringIndexFunction) (const MString& str, unsigned int index,
                               ///< void* userData); \endcode
    kStateFunction, ///< \brief describes the maya callback type \b MMessage::MStateFunction \code
                    ///< typedef void (*MStateFunction) (bool state, void* userData); \endcode
    kTimeFunction,  ///< \brief describes the maya callback type \b MMessage::MTimeFunction \code
                    ///< typedef void (*MTimeFunction) (MTime& time, void* userData); \endcode
    kPlugFunction,  ///< \brief describes the maya callback type \b MMessage::MPlugFunction \code
                    ///< typedef void (*MPlugFunction) (MPlug& srcPlug, MPlug& destPlug, bool made,
                    ///< void* userData); \endcode
    kNodePlugFunction,   ///< \brief describes the maya callback type \b MMessage::MNodePlugFunction
                         ///< \code typedef void (*MNodePlugFunction) (MObject& node, MPlug& plug,
                         ///< void* userData); \endcode
    kNodeStringFunction, ///< \brief describes the maya callback type \b
                         ///< MMessage::MNodeStringFunction \code typedef void
                         ///< (*MNodeStringFunction) (MObject& node, const MString& str, void*
                         ///< userData); \endcode
    kNodeStringBoolFunction, ///< \brief describes the maya callback type \b
                             ///< MMessage::MNodeStringBoolFunction \code typedef void
                             ///< (*MNodeStringBoolFunction) (MObject& node, const MString& str,
                             ///< bool flag, void* userData); \endcode
    kParentChildFunction,    ///< \brief describes the maya callback type \b
                             ///< MMessage::MParentChildFunction \code typedef void
                             ///< (*MParentChildFunction) (MDagPath& child, MDagPath& parent, void*
                             ///< userData); \endcode
    kModifierFunction, ///< \brief describes the maya callback type \b MMessage::MModifierFunction
                       ///< \code typedef void (*MModifierFunction) (MDGModifier& modifier, void*
                       ///< userData); \endcode
    kStringArrayFunction,  ///< \brief describes the maya callback type \b
                           ///< MMessage::MStringArrayFunction \code typedef void
                           ///< (*MStringArrayFunction) (const MStringArray& strs, void* userData);
                           ///< \endcode
    kNodeModifierFunction, ///< \brief describes the maya callback type \b
                           ///< MMessage::MNodeModifierFunction \code typedef void
                           ///< (*MNodeModifierFunction) (MObject& node, MDGModifier& modifier,
                           ///< void* userData); \endcode
    kObjArrayFunction,     ///< \brief describes the maya callback type \b MMessage::MObjArray \code
                           ///< typedef void (*MObjArray) (MObjectArray& objects, void* userData);
                           ///< \endcode
    kNodeObjArrayFunction, ///< \brief describes the maya callback type \b MMessage::MNodeObjArray
                           ///< \code typedef void (*MNodeObjArray) (MObject& node, MObjectArray&
                           ///< objects, void* userData); \endcode
    kStringNodeFunction, ///< \brief describes the maya callback type \b MMessage::MStringNode \code
                         ///< typedef void (*MStringNode) (const MString& str, MObject& node, void*
                         ///< userData); \endcode
    kCameraLayerFunction, ///< \brief describes the maya callback type \b
                          ///< MMessage::MCameraLayerFunction \code typedef void
                          ///< (*MCameraLayerFunction) (MObject& cameraSetNode, unsigned multiIndex,
                          ///< bool added, void* userData); \endcode
    kCameraLayerCameraFunction, ///< \brief describes the maya callback type \b
                                ///< MMessage::MCameraLayerCameraFunction \code typedef void
                                ///< (*MCameraLayerCameraFunction) (MObject& cameraSetNode, unsigned
                                ///< multiIndex, MObject& oldCamera, MObject& newCamera, void*
                                ///< userData); \endcode
    kConnFailFunction, ///< \brief describes the maya callback type \b MMessage::MConnFailFunction
                       ///< \code typedef void (*MConnFailFunction) (MPlug& srcPlug, MPlug&
                       ///< destPlug, const MString& srcPlugName, const MString& dstPlugName, void*
                       ///< userData); \endcode
    kPlugsDGModFunction, ///< \brief describes the maya callback type \b
                         ///< MMessage::MPlugsDGModFunction \code typedef void
                         ///< (*MPlugsDGModFunction) (MPlugArray& plugs, MDGModifier& modifier,
                         ///< void* userData); \endcode
    kNodeUuidFunction,   ///< \brief describes the maya callback type \b MMessage::MNodeUuidFunction
                         ///< \code typedef void (*MNodeUuidFunction) (MObject& node, const MUuid&
                         ///< uuid, void* userData); \endcode
    kCheckNodeUuidFunction,   ///< \brief describes the maya callback type \b
                              ///< MMessage::MCheckNodeUuidFunction \code typedef Action
                              ///< (*MCheckNodeUuidFunction) (bool doAction, MObject& node, MUuid&
                              ///< uuid, void* userData); \endcode
    kObjectFileFunction,      ///< \brief describes the maya callback type \b
                              ///< MMessage::MObjectFileFunction \code typedef void
                              ///< (*MObjectFileFunction) (const MObject& object, const MFileObject&
                              ///< file, void* userData); \endcode
    kCheckObjectFileFunction, ///< \brief describes the maya callback type \b
                              ///< MMessage::MCheckObjectFileFunction \code typedef void
                              ///< (*MCheckObjectFileFunction) (bool* retCode, const MObject&
                              ///< referenceNode, MFileObject& file, void* userData); \endcode
    kRenderTileFunction,      ///< \brief describes the maya callback type \b
                              ///< MMessage::MRenderTileFunction \code typedef void
    ///< (*MRenderTileFunction) (int originX, int originY, int tileMaxX, int
    ///< tileMaxY, void* userData); \endcode
    kMessageFunction, ///< \brief describes the maya callback type \b MMessage::MMessageFunction
                      ///< \code typedef void (*MMessageFunction) (const MString& message,
                      ///< MCommandMessage::MessageType messageType, void* userData); \endcode
    kMessageFilterFunction, ///< \brief describes the maya callback type \b
                            ///< MMessage::MMessageFilterFunction \code typedef void
                            ///< (*MMessageFilterFunction) (const MString& message,
                            ///< MCommandMessage::MessageType messageType, bool& filterOutput, void*
                            ///< userData); \endcode
    kMessageParentChildFunction, ///< \brief describes the maya callback type \b
                                 ///< MMessage::MMessageParentChildFunction \code typedef void
                                 ///< (*MMessageParentChildFunction) (MDagMessage::DagMessage
                                 ///< msgType, MDagPath& child, MDagPath& parent, void* userData);
                                 ///< \endcode
    kPathObjectPlugColoursFunction, ///< \brief describes the maya callback type \b
                                    ///< MMessage::MPathObjectPlugColorsFunction \code typedef void
                                    ///< (*MPathObjectPlugColorsFunction) (MDagPath& path, MObject&
                                    ///< object, MPlug& plug, MColorArray& colors, void* userData);
                                    ///< \endcode
    kWorldMatrixModifiedFunction    ///< \brief describes the maya callback type \b
                                    ///< MMessage::MWorldMatrixModifiedFunction \code typedef void
                                    ///< (*MWorldMatrixModifiedFunction) (MObject& transformNode,
    ///< MDagMessage::MatrixModifiedFlags& modified, void* userData);
    ///< \endcode
};

//----------------------------------------------------------------------------------------------------------------------
namespace AnimMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MAnimMessage class
enum AnimMessage
{
    kAnimCurveEdited,        ///< MAnimMessage::addAnimCurveEditedCallback
    kAnimKeyFrameEdited,     ///< MAnimMessage::addAnimKeyframeEditedCallback
    kNodeAnimKeyframeEdited, ///< MAnimMessage::addNodeAnimKeyframeEditedCallback - \b unsupported
    kAnimKeyframeEditCheck,  ///< MAnimMessage::addAnimKeyframeEditCheckCallback
    kPreBakeResults,         ///< MAnimMessage::addPreBakeResultsCallback
    kPostBakeResults,        ///< MAnimMessage::addPostBakeResultsCallback
    kDisableImplicitControl  ///< MAnimMessage::addDisableImplicitControlCallback
};
} // namespace AnimMessage

//----------------------------------------------------------------------------------------------------------------------
namespace CameraSetMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MCameraSetMessage class
enum CameraSetMessage
{
    kCameraLayer,  ///< MCamerSetMessage::addCameraLayerCallback
    kCameraChanged ///< MCamerSetMessage::addCameraChangedCallback
};
} // namespace CameraSetMessage

//----------------------------------------------------------------------------------------------------------------------
namespace CommandMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MCommandMessage class
enum CommandMessage
{
    kCommand,             ///< MCommandMessage::addCommandCallback
    kCommandOuptut,       ///< MCommandMessage::addCommandOutputCallback
    kCommandOutputFilter, ///< MCommandMessage::addCommandOutputFilterCallback
    kProc                 ///< MCommandMessage::addProcCallback
};
} // namespace CommandMessage

//----------------------------------------------------------------------------------------------------------------------
namespace ConditionMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MConditionMessage class
enum ConditionMessage
{
    kCondition ///< unsupported
};
} // namespace ConditionMessage

//----------------------------------------------------------------------------------------------------------------------
namespace ContainerMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MContainerMessage class
enum ContainerMessage
{
    kPublishAttr, ///< MContainerMessage::addPublishAttrCallback
    kBoundAttr    ///< MContainerMessage::addBoundAttrCallback
};
} // namespace ContainerMessage

//----------------------------------------------------------------------------------------------------------------------
namespace DagMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MDagMessage class
enum DagMessage
{
    kParentAdded,            ///< MDagMessage::addParentAddedCallback
    kParentAddedDagPath,     ///< MDagMessage::addParentAddedDagPathCallback  - \b unsupported
    kParentRemoved,          ///< MDagMessage::addParentRemovedCallback
    kParentRemovedDagPath,   ///< MDagMessage::addParentRemovedDagPathCallback  - \b unsupported
    kChildAdded,             ///< MDagMessage::addChildAddedCallback
    kChildAddedDagPath,      ///< MDagMessage::addChildAddedDagPathCallback  - \b unsupported
    kChildRemoved,           ///< MDagMessage::addChildRemovedCallback
    kChildRemovedDagPath,    ///< MDagMessage::addChildRemovedDagPathCallback  - \b unsupported
    kChildReordered,         ///< MDagMessage::addChildReorderedCallback
    kChildReorderedDagPath,  ///< MDagMessage::addChildReorderedDagPathCallback  - \b unsupported
    kDag,                    ///< MDagMessage::addDagCallback  - \b unsupported
    kDagDagPath,             ///< MDagMessage::addDagDagPathCallback  - \b unsupported
    kAllDagChanges,          ///< MDagMessage::addAllDagChangesCallback
    kAllDagChangesDagPath,   ///< MDagMessage::addAllDagChangesDagPathCallback  - \b unsupported
    kInstanceAdded,          ///< MDagMessage::addInstanceAddedCallback
    kInstanceAddedDagPath,   ///< MDagMessage::addInstanceAddedDagPathCallback  - \b unsupported
    kInstanceRemoved,        ///< MDagMessage::addInstanceRemovedCallback
    kInstanceRemovedDagPath, ///< MDagMessage::addInstanceRemovedDagPathCallback  - \b unsupported
    kWorldMatrixModified,    ///< MDagMessage::addWorldMatrixModifiedCallback  - \b unsupported
};
} // namespace DagMessage

//----------------------------------------------------------------------------------------------------------------------
namespace DGMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MDGMessage class
enum DGMessage
{
    kTimeChange,             ///< DGMessage::addTimeChangeCallback
    kDelayedTimeChange,      ///< DGMessage::addDelayedTimeChangeCallback
    kDelayedTimeChangeRunup, ///< DGMessage::addDelayedTimeChangeRunupCallback
    kForceUpdate,            ///< DGMessage::addForceUpdateCallback
    kNodeAdded,              ///< DGMessage::addNodeAddedCallback
    kNodeRemoved,            ///< DGMessage::addNodeRemovedCallback
    kConnection,             ///< DGMessage::addConnectionCallback
    kPreConnection,          ///< DGMessage::addPreConnectionCallback
    kNodeChangeUuidCheck,    ///< DGMessage::addNodeChangeUuidCheckCallback  - \b unsupported
};
} // namespace DGMessage

//----------------------------------------------------------------------------------------------------------------------
namespace EventMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MEventMessage class
enum EventMessage
{
};
} // namespace EventMessage

//----------------------------------------------------------------------------------------------------------------------
namespace LockMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MLockMessage class
enum LockMessage
{
};
} // namespace LockMessage

//----------------------------------------------------------------------------------------------------------------------
namespace ModelMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MModelMessage class
enum ModelMessage
{
    kCallback,             ///< MModelMessage::addCallback
    kBeforeDuplicate,      ///< MModelMessage::addBeforeDuplicateCallback
    kAfterDuplicate,       ///< MModelMessage::addAfterDuplicateCallback
    kNodeAddedToModel,     ///< MModelMessage::addNodeAddedToModelCallback   - \b upsupported
    kNodeRemovedFromModel, ///< MModelMessage::addNodeRemovedFromModelCallback   - \b upsupported
};
} // namespace ModelMessage

//----------------------------------------------------------------------------------------------------------------------
namespace NodeMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MNodeMessage class
enum NodeMessage
{
};
} // namespace NodeMessage

//----------------------------------------------------------------------------------------------------------------------
namespace ObjectSetMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MObjectSetMessage class
enum ObjectSetMessage
{
};
} // namespace ObjectSetMessage

//----------------------------------------------------------------------------------------------------------------------
namespace PaintMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MPaintMessage class
enum PaintMessage
{
    kVertexColor, ///< MPaintMessage::addVertexColorCallback
};
} // namespace PaintMessage

//----------------------------------------------------------------------------------------------------------------------
namespace PolyMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MPolyMessage class
enum PolyMessage
{
};
} // namespace PolyMessage

//----------------------------------------------------------------------------------------------------------------------
namespace SceneMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MSceneMessage class
enum SceneMessage
{
    kSceneUpdate
    = MSceneMessage::kSceneUpdate, ///< MSceneMessage::addCallback(MSceneMessage::kSceneUpdate)
    kBeforeNew
    = MSceneMessage::kBeforeNew,          ///< MSceneMessage::addCallback(MSceneMessage::kBeforeNew)
    kAfterNew = MSceneMessage::kAfterNew, ///< MSceneMessage::addCallback(MSceneMessage::kAfterNew)
    kBeforeImport
    = MSceneMessage::kBeforeImport, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeImport)
    kAfterImport
    = MSceneMessage::kAfterImport, ///< MSceneMessage::addCallback(MSceneMessage::kAfterImport)
    kBeforeOpen
    = MSceneMessage::kBeforeOpen, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeOpen)
    kAfterOpen
    = MSceneMessage::kAfterOpen, ///< MSceneMessage::addCallback(MSceneMessage::kAfterOpen)
    kBeforeFileRead = MSceneMessage::
        kBeforeFileRead, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeFileRead)
    kAfterFileRead
    = MSceneMessage::kAfterFileRead, ///< MSceneMessage::addCallback(MSceneMessage::kAfterFileRead)
    kAfterSceneReadAndRecordEdits = MSceneMessage::
        kAfterSceneReadAndRecordEdits, ///< MSceneMessage::addCallback(MSceneMessage::kAfterSceneReadAndRecordEdits)
    kBeforeExport
    = MSceneMessage::kBeforeExport, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeExport)
    kExportStarted
    = MSceneMessage::kExportStarted, ///< MSceneMessage::addCallback(MSceneMessage::kExportStarted)
    kAfterExport
    = MSceneMessage::kAfterExport, ///< MSceneMessage::addCallback(MSceneMessage::kAfterExport)
    kBeforeSave
    = MSceneMessage::kBeforeSave, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeSave)
    kAfterSave
    = MSceneMessage::kAfterSave, ///< MSceneMessage::addCallback(MSceneMessage::kAfterSave)
    kBeforeCreateReference = MSceneMessage::
        kBeforeCreateReference, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeCreateReference)
    kBeforeLoadReferenceAndRecordEdits = MSceneMessage::
        kBeforeLoadReferenceAndRecordEdits, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeLoadReferenceAndRecordEdits)
    kAfterCreateReference = MSceneMessage::
        kAfterCreateReference, ///< MSceneMessage::addCallback(MSceneMessage::kAfterCreateReference)
    kAfterCreateReferenceAndRecordEdits = MSceneMessage::
        kAfterCreateReferenceAndRecordEdits, ///< MSceneMessage::addCallback(MSceneMessage::kAfterCreateReferenceAndRecordEdits)
    kBeforeRemoveReference = MSceneMessage::
        kBeforeRemoveReference, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeRemoveReference)
    kAfterRemoveReference = MSceneMessage::
        kAfterRemoveReference, ///< MSceneMessage::addCallback(MSceneMessage::kAfterRemoveReference)
    kBeforeImportReference = MSceneMessage::
        kBeforeImportReference, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeImportReference)
    kAfterImportReference = MSceneMessage::
        kAfterImportReference, ///< MSceneMessage::addCallback(MSceneMessage::kAfterImportReference)
    kBeforeExportReference = MSceneMessage::
        kBeforeExportReference, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeExportReference)
    kAfterExportReference = MSceneMessage::
        kAfterExportReference, ///< MSceneMessage::addCallback(MSceneMessage::kAfterExportReference)
    kBeforeUnloadReference = MSceneMessage::
        kBeforeUnloadReference, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeUnloadReference)
    kAfterUnloadReference = MSceneMessage::
        kAfterUnloadReference, ///< MSceneMessage::addCallback(MSceneMessage::kAfterUnloadReference)
    kBeforeLoadReference = MSceneMessage::
        kBeforeLoadReference, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeLoadReference)
    kBeforeCreateReferenceAndRecordEdits = MSceneMessage::
        kBeforeCreateReferenceAndRecordEdits, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeCreateReferenceAndRecordEdits)
    kAfterLoadReference = MSceneMessage::
        kAfterLoadReference, ///< MSceneMessage::addCallback(MSceneMessage::kAfterLoadReference)
    kAfterLoadReferenceAndRecordEdits = MSceneMessage::
        kAfterLoadReferenceAndRecordEdits, ///< MSceneMessage::addCallback(MSceneMessage::kAfterLoadReferenceAndRecordEdits)
    kBeforeSoftwareRender = MSceneMessage::
        kBeforeSoftwareRender, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeSoftwareRender)
    kAfterSoftwareRender = MSceneMessage::
        kAfterSoftwareRender, ///< MSceneMessage::addCallback(MSceneMessage::kAfterSoftwareRender)
    kBeforeSoftwareFrameRender = MSceneMessage::
        kBeforeSoftwareFrameRender, ///< MSceneMessage::addCallback(MSceneMessage::kBeforeSoftwareFrameRender)
    kAfterSoftwareFrameRender = MSceneMessage::
        kAfterSoftwareFrameRender, ///< MSceneMessage::addCallback(MSceneMessage::kAfterSoftwareFrameRender)
    kSoftwareRenderInterrupted = MSceneMessage::
        kSoftwareRenderInterrupted, ///< MSceneMessage::addCallback(MSceneMessage::kSoftwareRenderInterrupted)
    kMayaInitialized = MSceneMessage::
        kMayaInitialized, ///< MSceneMessage::addCallback(MSceneMessage::kMayaInitialized)
    kMayaExiting
    = MSceneMessage::kMayaExiting, ///< MSceneMessage::addCallback(MSceneMessage::kMayaExiting)
    kBeforeNewCheck = MSceneMessage::
        kBeforeNewCheck, ///< MSceneMessage::addCheckCallback(MSceneMessage::kBeforeNewCheck)
    kBeforeImportCheck = MSceneMessage::
        kBeforeImportCheck, ///< MSceneMessage::addCheckCallback(MSceneMessage::kBeforeImportCheck)
    kBeforeOpenCheck = MSceneMessage::
        kBeforeOpenCheck, ///< MSceneMessage::addCheckCallback(MSceneMessage::kBeforeOpenCheck)
    kBeforeExportCheck = MSceneMessage::
        kBeforeExportCheck, ///< MSceneMessage::addCheckCallback(MSceneMessage::kBeforeExportCheck)
    kBeforeSaveCheck = MSceneMessage::
        kBeforeSaveCheck, ///< MSceneMessage::addCheckCallback(MSceneMessage::kBeforeSaveCheck)
    kBeforeCreateReferenceCheck = MSceneMessage::
        kBeforeCreateReferenceCheck, ///< MSceneMessage::addCheckCallback(MSceneMessage::kBeforeCreateReferenceCheck)
    kBeforeLoadReferenceCheck = MSceneMessage::
        kBeforeLoadReferenceCheck, ///< MSceneMessage::addCheckCallback(MSceneMessage::kBeforeLoadReferenceCheck)
    kBeforePluginLoad = MSceneMessage::
        kBeforePluginLoad, ///< MSceneMessage::addStringArrayCallback(MSceneMessage::kBeforePluginLoad)
    kAfterPluginLoad = MSceneMessage::
        kAfterPluginLoad, ///< MSceneMessage::addStringArrayCallback(MSceneMessage::kAfterPluginLoad)
    kBeforePluginUnload = MSceneMessage::
        kBeforePluginUnload, ///< MSceneMessage::addStringArrayCallback(MSceneMessage::kBeforePluginUnload)
    kAfterPluginUnload = MSceneMessage::
        kAfterPluginUnload, ///< MSceneMessage::addStringArrayCallback(MSceneMessage::kAfterPluginUnload)
};
} // namespace SceneMessage

//----------------------------------------------------------------------------------------------------------------------
namespace TimerMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MTimerMessage class
enum TimerMessage
{
};
} // namespace TimerMessage

//----------------------------------------------------------------------------------------------------------------------
namespace UiMessage {
/// \ingroup mayaevents
/// \brief  Maya events defined in the \b MUiMessage class
enum UiMessage
{
};
} // namespace UiMessage

//----------------------------------------------------------------------------------------------------------------------
/// \brief  An interface that provides the event system with some utilities from the underlying DCC
/// application.
///         This class is responsible for keeping track of the number of maya events registered, and
///         creating/destroying the MMessage callbacks.
/// \ingroup mayaevents
//----------------------------------------------------------------------------------------------------------------------
class MayaEventHandler : public AL::event::CustomEventHandler
{
    typedef std::unordered_map<AL::event::EventId, size_t> EventToMaya;

public:
    /// \brief  a structure to store some binding info for the MMessage event info
    struct MayaCallbackInfo
    {
        AL::event::EventId eventId;         ///< the event id from the event scheduler
        uint32_t           refCount;        ///< the ref count for this callback
        MayaMessageType    mayaMessageType; ///< the maya MMessage class that defines the message
        MayaCallbackType
                 mayaCallbackType; ///< the type of C callback function needed to execute the callback
        uint32_t mmessageEnum; ///< the enum value from one of the MSceneMessage / MEventMessage etc
                               ///< classes.
        MCallbackId mayaCallback; ///< the maya callback ID
    };

    /// \brief  constructor function
    /// \param  scheduler the event scheduler
    /// \param  eventType the event type
    AL_MAYA_EVENTS_PUBLIC
    MayaEventHandler(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);

    /// \brief  dtor
    ~MayaEventHandler();

    /// \brief  returns the event type string
    /// \return "maya"
    const char* eventTypeString() const override { return "maya"; }

    /// \brief  returns the event scheduler
    /// \return the event scheduler used for these maya events
    AL::event::EventScheduler* scheduler() const { return m_scheduler; }

    /// \brief  queries the maya event information for the specified maya event
    /// \param  event the event ID
    /// \return a pointer to the maya event information (or null for an invalid event)
    const MayaCallbackInfo* getEventInfo(const AL::event::EventId event) const
    {
        const auto it = m_eventMapping.find(event);
        if (it == m_eventMapping.end())
            return 0;
        return m_callbacks.data() + it->second;
    }

    /// \brief  queries whether the event has an associated MCallbackId (indicating the callback is
    /// active with maya) \param  event the event to query \return true if callback is active with
    /// maya, false otherwise
    bool isMayaCallbackRegistered(const AL::event::EventId event) const
    {
        const MayaCallbackInfo* cbi = getEventInfo(event);
        return cbi ? cbi->refCount != 0 : false;
    }

    /// \brief  queries the maya event information for the specified maya event
    /// \param  eventName the event name
    /// \return a pointer to the maya event information (or null for an invalid event)
    const MayaCallbackInfo* getEventInfo(const char* const eventName) const
    {
        const AL::event::EventDispatcher* const dispatcher = m_scheduler->event(eventName);
        if (dispatcher) {
            return getEventInfo(dispatcher->eventId());
        }
        return 0;
    }

    /// \brief  queries whether the event has an associated MCallbackId (indicating the callback is
    /// active with maya) \param  eventName the event to query \return true if callback is active
    /// with maya, false otherwise
    bool isMayaCallbackRegistered(const char* const eventName) const
    {
        const MayaCallbackInfo* cbi = getEventInfo(eventName);
        return cbi ? cbi->refCount != 0 : false;
    }

private:
    void onCallbackCreated(const AL::event::CallbackId callbackId) override;
    void onCallbackDestroyed(const AL::event::CallbackId callbackId) override;
    bool registerEvent(
        AL::event::EventScheduler* scheduler,
        const char*                eventName,
        AL::event::EventType       eventType,
        MayaMessageType            messageType,
        MayaCallbackType           callbackType,
        uint32_t                   mmessageEnum);
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
    void registerAnimMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void
    registerCameraSetMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void
    registerCommandMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void
    registerConditionMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void
         registerContainerMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void registerDagMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void registerDGMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void
         registerEventMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void registerLockMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void
         registerModelMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void registerNodeMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void
    registerObjectSetMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void
         registerPaintMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void registerPolyMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void
    registerSceneMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void
         registerTimerMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    void registerUiMessages(AL::event::EventScheduler* scheduler, AL::event::EventType eventType);
    std::vector<MayaCallbackInfo> m_callbacks;
    EventToMaya                   m_eventMapping;
    AL::event::EventScheduler*    m_scheduler;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class that replaces the MMessage (and derived message classes) in order to provide a
/// level of scheduling
///         and debugability across multiple plugins, which is not possible in the standard maya
///         API. Effectively the class wraps the core EventScheduler API to provide a little bit of
///         additional type safety on the callback type provided.
/// \ingroup mayaevents
//----------------------------------------------------------------------------------------------------------------------
class MayaEventManager
{
    AL_MAYA_EVENTS_PUBLIC
    static MayaEventManager* g_instance;

public:
    /// \brief  returns the global maya event manager instance
    AL_MAYA_EVENTS_PUBLIC
    static MayaEventManager& instance();

    /// \brief  returns the global maya event manager instance
    static void freeInstance()
    {
        delete g_instance;
        g_instance = 0;
    }

    /// \brief  constructor
    /// \param  mayaEvents the custom event handler
    MayaEventManager(MayaEventHandler* mayaEvents)
        : m_mayaEvents(mayaEvents)
    {
        g_instance = this;
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MBasicFunction func,
        const char* const        eventName,
        const char* const        tag,
        uint32_t                 weight,
        void*                    userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kBasicFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MElapsedTimeFunction func,
        const char* const              eventName,
        const char* const              tag,
        uint32_t                       weight,
        void*                          userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kElapsedTimeFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MCheckFunction func,
        const char* const        eventName,
        const char* const        tag,
        uint32_t                 weight,
        void*                    userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kCheckFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MCheckFileFunction func,
        const char* const            eventName,
        const char* const            tag,
        uint32_t                     weight,
        void*                        userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kCheckFileFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MCheckPlugFunction func,
        const char* const            eventName,
        const char* const            tag,
        uint32_t                     weight,
        void*                        userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kCheckPlugFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MComponentFunction func,
        const char* const            eventName,
        const char* const            tag,
        uint32_t                     weight,
        void*                        userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kComponentFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MNodeFunction func,
        const char* const       eventName,
        const char* const       tag,
        uint32_t                weight,
        void*                   userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kNodeFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MStringFunction func,
        const char* const         eventName,
        const char* const         tag,
        uint32_t                  weight,
        void*                     userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kStringFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MTwoStringFunction func,
        const char* const            eventName,
        const char* const            tag,
        uint32_t                     weight,
        void*                        userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kTwoStringFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MThreeStringFunction func,
        const char* const              eventName,
        const char* const              tag,
        uint32_t                       weight,
        void*                          userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kThreeStringFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MStringIntBoolIntFunction func,
        const char* const                   eventName,
        const char* const                   tag,
        uint32_t                            weight,
        void*                               userData = 0)
    {
        return registerCallbackInternal(
            (void*)func,
            MayaCallbackType::kStringIntBoolIntFunction,
            eventName,
            tag,
            weight,
            userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MStringIndexFunction func,
        const char* const              eventName,
        const char* const              tag,
        uint32_t                       weight,
        void*                          userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kStringIndexFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MNodeStringBoolFunction func,
        const char* const                 eventName,
        const char* const                 tag,
        uint32_t                          weight,
        void*                             userData = 0)
    {
        return registerCallbackInternal(
            (void*)func,
            MayaCallbackType::kNodeStringBoolFunction,
            eventName,
            tag,
            weight,
            userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MStateFunction func,
        const char* const        eventName,
        const char* const        tag,
        uint32_t                 weight,
        void*                    userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kStateFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MTimeFunction func,
        const char* const       eventName,
        const char* const       tag,
        uint32_t                weight,
        void*                   userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kTimeFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MPlugFunction func,
        const char* const       eventName,
        const char* const       tag,
        uint32_t                weight,
        void*                   userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kPlugFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MNodePlugFunction func,
        const char* const           eventName,
        const char* const           tag,
        uint32_t                    weight,
        void*                       userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kNodePlugFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MNodeStringFunction func,
        const char* const             eventName,
        const char* const             tag,
        uint32_t                      weight,
        void*                         userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kNodeStringFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MParentChildFunction func,
        const char* const              eventName,
        const char* const              tag,
        uint32_t                       weight,
        void*                          userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kParentChildFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MModifierFunction func,
        const char* const           eventName,
        const char* const           tag,
        uint32_t                    weight,
        void*                       userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kModifierFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MStringArrayFunction func,
        const char* const              eventName,
        const char* const              tag,
        uint32_t                       weight,
        void*                          userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kStringArrayFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MNodeModifierFunction func,
        const char* const               eventName,
        const char* const               tag,
        uint32_t                        weight,
        void*                           userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kNodeModifierFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MObjArray func,
        const char* const   eventName,
        const char* const   tag,
        uint32_t            weight,
        void*               userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kObjArrayFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MNodeObjArray func,
        const char* const       eventName,
        const char* const       tag,
        uint32_t                weight,
        void*                   userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kNodeObjArrayFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MStringNode func,
        const char* const     eventName,
        const char* const     tag,
        uint32_t              weight,
        void*                 userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kStringNodeFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MCameraLayerFunction func,
        const char* const              eventName,
        const char* const              tag,
        uint32_t                       weight,
        void*                          userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kCameraLayerFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MCameraLayerCameraFunction func,
        const char* const                    eventName,
        const char* const                    tag,
        uint32_t                             weight,
        void*                                userData = 0)
    {
        return registerCallbackInternal(
            (void*)func,
            MayaCallbackType::kCameraLayerCameraFunction,
            eventName,
            tag,
            weight,
            userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MConnFailFunction func,
        const char* const           eventName,
        const char* const           tag,
        uint32_t                    weight,
        void*                       userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kConnFailFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MPlugsDGModFunction func,
        const char* const             eventName,
        const char* const             tag,
        uint32_t                      weight,
        void*                         userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kPlugsDGModFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MNodeUuidFunction func,
        const char* const           eventName,
        const char* const           tag,
        uint32_t                    weight,
        void*                       userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kNodeUuidFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MCheckNodeUuidFunction func,
        const char* const                eventName,
        const char* const                tag,
        uint32_t                         weight,
        void*                            userData = 0)
    {
        return registerCallbackInternal(
            (void*)func,
            MayaCallbackType::kCheckNodeUuidFunction,
            eventName,
            tag,
            weight,
            userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MObjectFileFunction func,
        const char* const             eventName,
        const char* const             tag,
        uint32_t                      weight,
        void*                         userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kObjectFileFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MCheckObjectFileFunction func,
        const char* const                  eventName,
        const char* const                  tag,
        uint32_t                           weight,
        void*                              userData = 0)
    {
        return registerCallbackInternal(
            (void*)func,
            MayaCallbackType::kCheckObjectFileFunction,
            eventName,
            tag,
            weight,
            userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MMessage::MRenderTileFunction func,
        const char* const             eventName,
        const char* const             tag,
        uint32_t                      weight,
        void*                         userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kRenderTileFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MCommandMessage::MMessageFunction func,
        const char* const                 eventName,
        const char* const                 tag,
        uint32_t                          weight,
        void*                             userData = 0)
    {
        return registerCallbackInternal(
            (void*)func, MayaCallbackType::kMessageFunction, eventName, tag, weight, userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MCommandMessage::MMessageFilterFunction func,
        const char* const                       eventName,
        const char* const                       tag,
        uint32_t                                weight,
        void*                                   userData = 0)
    {
        return registerCallbackInternal(
            (void*)func,
            MayaCallbackType::kMessageFilterFunction,
            eventName,
            tag,
            weight,
            userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MDagMessage::MMessageParentChildFunction func,
        const char* const                        eventName,
        const char* const                        tag,
        uint32_t                                 weight,
        void*                                    userData = 0)
    {
        return registerCallbackInternal(
            (void*)func,
            MayaCallbackType::kMessageParentChildFunction,
            eventName,
            tag,
            weight,
            userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MDagMessage::MWorldMatrixModifiedFunction func,
        const char* const                         eventName,
        const char* const                         tag,
        uint32_t                                  weight,
        void*                                     userData = 0)
    {
        return registerCallbackInternal(
            (void*)func,
            MayaCallbackType::kWorldMatrixModifiedFunction,
            eventName,
            tag,
            weight,
            userData);
    }

    /// \brief  registers a C++ callback against a maya event
    /// \param  func the C++ function
    /// \param  eventName the event
    /// \param  tag the unique tag for the callback
    /// \param  weight the weight (lower weights at executed before higher weights)
    /// \param  userData custom user data pointer
    /// \return the callback id
    AL::event::CallbackId registerCallback(
        MPaintMessage::MPathObjectPlugColorsFunction func,
        const char* const                            eventName,
        const char* const                            tag,
        uint32_t                                     weight,
        void*                                        userData = 0)
    {
        return registerCallbackInternal(
            (void*)func,
            MayaCallbackType::kPathObjectPlugColoursFunction,
            eventName,
            tag,
            weight,
            userData);
    }

    /// \brief  unregisters the callback id
    /// \param  id the callback id to unregister
    bool unregisterCallback(AL::event::CallbackId id)
    {
        AL::event::EventScheduler* scheduler = m_mayaEvents->scheduler();
        return scheduler->unregisterCallback(id);
    }

    /// \brief  returns the custom maya event handler
    /// \return the event handler in charge of registering and unregistering maya events
    MayaEventHandler* mayaEventsHandler() const { return m_mayaEvents; }

private:
    AL_MAYA_EVENTS_PUBLIC
    AL::event::CallbackId registerCallbackInternal(
        const void*       func,
        MayaCallbackType  type,
        const char* const eventName,
        const char* const tag,
        uint32_t          weight,
        void*             userData = 0);
    MayaEventHandler* m_mayaEvents;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace event
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
