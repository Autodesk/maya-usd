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
#include "AL/usdmaya/cmds/EventCommand.h"

#include "AL/usdmaya/DebugCodes.h"

#include <pxr/pxr.h>

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MPxNode.h>
#include <maya/MSelectionList.h>
#include <maya/MSyntax.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(Event, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax Event::createSyntax()
{
    MSyntax syn;
    syn.addFlag("-h", "-help", MSyntax::kString);
    syn.addFlag("-d", "-delete");
    syn.addFlag("-p", "-parent", MSyntax::kLong, MSyntax::kLong);
    syn.addArg(MSyntax::kString);
    syn.useSelectionAsDefault(false);
    syn.setObjectType(MSyntax::kSelectionList, 0, 1);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool Event::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
MStatus Event::doIt(const MArgList& argList)
{
    try {
        MStatus      status;
        MArgDatabase db(syntax(), argList, &status);
        if (!status)
            throw status;
        AL_MAYA_COMMAND_HELP(db, g_helpText);

        db.getCommandArgument(0, m_eventName);

        if (db.isFlagSet("-p")) {
            union
            {
                int                   asInt[2];
                AL::event::CallbackId asId;
            };
            db.getFlagArgument("-p", 0, asInt[0]);
            db.getFlagArgument("-p", 1, asInt[1]);
            m_parentEvent = asId;
        }

        m_deleting = db.isFlagSet("-d");

        MSelectionList items;
        status = db.getObjects(items);
        if (status && items.length()) {
            MObject object;
            if (items.getDependNode(0, object)) {
                MFnDependencyNode fn(object);
                m_associatedData = dynamic_cast<AL::event::NodeEvents*>(fn.userNode());
                if (!m_associatedData) {
                    MGlobal::displayError(
                        MString("AL_usdmaya_Event, specified node does not support the NodeEvents "
                                "interface: ")
                        + fn.name());
                    return MS::kFailure;
                }
                AL::event::EventId id = m_associatedData->getId(m_eventName.asChar());
                if (m_deleting && !id) {
                    MGlobal::displayError(
                        MString("AL_usdmaya_Event, cannot delete an event that doesn't exist: ")
                        + fn.name());
                    return MS::kFailure;
                }
                if (!m_deleting && id) {
                    MGlobal::displayError(
                        MString("AL_usdmaya_Event, specified event already exists on node: ")
                        + fn.name());
                    return MS::kFailure;
                }
            } else {
                MGlobal::displayError(
                    "AL_usdmaya_Event, specified node could not be retrieved from selection list.");
                return MS::kFailure;
            }
        } else {
            auto event = AL::event::EventScheduler::getScheduler().event(m_eventName.asChar());
            if (m_deleting && !event) {
                MGlobal::displayError(
                    MString("AL_usdmaya_Event, cannot delete an event that doesn't exist: ")
                    + m_eventName);
                return MS::kFailure;
            }
            if (!m_deleting && event) {
                MGlobal::displayError(
                    MString("AL_usdmaya_Event, specified event already exists on node: ")
                    + m_eventName);
                return MS::kFailure;
            }
        }
    } catch (MStatus status) {
        return status;
    }
    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Event::redoIt()
{
    if (m_associatedData) {
        if (m_deleting) {
            m_associatedData->unregisterEvent(m_eventName.asChar());
        } else {
            m_associatedData->registerEvent(
                m_eventName.asChar(), AL::event::kUserSpecifiedEventType, m_parentEvent);
        }
    } else {
        if (m_deleting) {
            AL::event::EventScheduler::getScheduler().unregisterEvent(m_eventName.asChar());
        } else {
            AL::event::EventScheduler::getScheduler().registerEvent(
                m_eventName.asChar(),
                AL::event::kUserSpecifiedEventType,
                m_associatedData,
                m_parentEvent);
        }
    }
    m_deleting = !m_deleting;
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Event::undoIt() { return redoIt(); }

//----------------------------------------------------------------------------------------------------------------------
MStatus BaseCallbackCommand::redoItImplementation()
{
    AL::event::CallbackIds callbacksToDelete(m_callbacksToInsert.size());
    AL::event::Callbacks   callbacksToInsert(m_callbacksToDelete.size());

    for (size_t i = 0, n = m_callbacksToDelete.size(); i < n; ++i) {
        if (!AL::event::EventScheduler::getScheduler().unregisterCallback(
                m_callbacksToDelete[i], callbacksToInsert[i])) {
            union
            {
                int32_t               asInt[2];
                AL::event::CallbackId asCb;
            };
            asCb = m_callbacksToDelete[i];
            MString errorString = "failed to unregister callback with ID: ";
            errorString += asInt[0];
            errorString += ' ';
            errorString += asInt[1];
            MGlobal::displayError(errorString);
        }
    }

    for (size_t i = 0, n = m_callbacksToInsert.size(); i < n; ++i) {
        callbacksToDelete[i]
            = AL::event::EventScheduler::getScheduler().registerCallback(m_callbacksToInsert[i]);
        if (!callbacksToDelete[i]) {
            MGlobal::displayError(
                MString("failed to register callback with tag: ")
                + m_callbacksToInsert[i].tag().c_str());
        }
    }

    std::swap(callbacksToDelete, m_callbacksToDelete);
    std::swap(callbacksToInsert, m_callbacksToInsert);
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(Callback, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax Callback::createSyntax()
{
    MSyntax syn;
    syn.addFlag("-h", "-help", MSyntax::kString);
    syn.addFlag(
        "-pe",
        "-pythonEvent",
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kUnsigned,
        MSyntax::kString);
    syn.addFlag(
        "-pne",
        "-pythonNodeEvent",
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kUnsigned,
        MSyntax::kString);
    syn.addFlag(
        "-me",
        "-melEvent",
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kUnsigned,
        MSyntax::kString);
    syn.addFlag(
        "-mne",
        "-melNodeEvent",
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kString,
        MSyntax::kUnsigned,
        MSyntax::kString);
    syn.addFlag("-se", "-supportsEvent", MSyntax::kString);
    syn.addFlag("-de", "-deleteEvent", MSyntax::kLong, MSyntax::kLong);
    syn.makeFlagMultiUse("-pe");
    syn.makeFlagMultiUse("-pne");
    syn.makeFlagMultiUse("-me");
    syn.makeFlagMultiUse("-mne");
    syn.makeFlagMultiUse("-de");
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool Callback::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
MStatus Callback::doIt(const MArgList& argList)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("Callback::doIt\n");
    try {
        MStatus      status;
        MArgDatabase db(syntax(), argList, &status);
        if (!status)
            throw status;
        AL_MAYA_COMMAND_HELP(db, g_helpText);

        if (db.isFlagSet("se")) {
            MString nodeName;
            if (db.getFlagArgument("se", 0, nodeName)) {
                MSelectionList sl;
                if (sl.add(nodeName)) {
                    MObject node;
                    sl.getDependNode(0, node);
                    MFnDependencyNode fn(node, &status);
                    if (status) {
                        if (dynamic_cast<AL::event::NodeEvents*>(fn.userNode())) {
                            setResult(true);
                        } else {
                            setResult(false);
                        }
                    } else {
                        setResult(false);
                    }
                } else {
                    MGlobal::displayError(
                        MString("AL_usdmaya_Event, unknown node specified: ") + nodeName);
                    return MS::kFailure;
                }
            }
        }

        MIntArray returnedIds;
        auto      storeId = [&returnedIds](const AL::event::CallbackId id) {
            const int* const ptr = (const int*)&id;
            returnedIds.append(ptr[0]);
            returnedIds.append(ptr[1]);
        };

        for (uint32_t i = 0, n = db.numberOfFlagUses("-pe"); i < n; ++i) {
            MArgList args;
            db.getFlagArgumentList("pe", i, args);
            MString  eventName = args.asString(0);
            MString  tag = args.asString(1);
            unsigned weight = args.asInt(2);
            MString  commandText = args.asString(3);

            auto cb = AL::event::EventScheduler::getScheduler().buildCallback(
                eventName.asChar(), tag.asChar(), commandText.asChar(), weight, true);
            if (cb.callbackId()) {
                m_callbacksToInsert.push_back(std::move(cb));
            }
            storeId(cb.callbackId());
        }

        for (uint32_t i = 0, n = db.numberOfFlagUses("-me"); i < n; ++i) {
            MArgList args;
            db.getFlagArgumentList("me", i, args);
            MString  eventName = args.asString(0);
            MString  tag = args.asString(1);
            unsigned weight = args.asInt(2);
            MString  commandText = args.asString(3);

            auto cb = AL::event::EventScheduler::getScheduler().buildCallback(
                eventName.asChar(), tag.asChar(), commandText.asChar(), weight, false);
            if (cb.callbackId()) {
                m_callbacksToInsert.push_back(std::move(cb));
            }
            storeId(cb.callbackId());
        }

        for (uint32_t i = 0, n = db.numberOfFlagUses("-pne"); i < n; ++i) {
            MArgList args;
            db.getFlagArgumentList("pne", i, args);
            MString  nodeName = args.asString(0);
            MString  eventName = args.asString(1);
            MString  tag = args.asString(2);
            unsigned weight = args.asInt(3);
            MString  commandText = args.asString(4);

            MSelectionList sl;
            if (sl.add(nodeName)) {
                MObject nodeHandle;
                sl.getDependNode(0, nodeHandle);
                MFnDependencyNode fn(nodeHandle);

                AL::event::NodeEvents* event = dynamic_cast<AL::event::NodeEvents*>(fn.userNode());
                if (!event) {
                    MGlobal::displayError(
                        MString("specified node does not support the NodeEvents interface: ")
                        + nodeName);
                } else {
                    AL::event::EventId eventId = event->getId(eventName.asChar());
                    if (eventId) {
                        auto* scheduler = event->scheduler();
                        auto  cb = scheduler->buildCallback(
                            eventId, tag.asChar(), commandText.asChar(), weight, true);
                        if (cb.callbackId()) {
                            m_callbacksToInsert.push_back(std::move(cb));
                        }
                        storeId(cb.callbackId());
                        continue;
                    }
                }
            }
            storeId(0);
        }

        for (uint32_t i = 0, n = db.numberOfFlagUses("-mne"); i < n; ++i) {
            MArgList args;
            db.getFlagArgumentList("mne", i, args);
            MString  nodeName = args.asString(0);
            MString  eventName = args.asString(1);
            MString  tag = args.asString(2);
            unsigned weight = args.asInt(3);
            MString  commandText = args.asString(4);

            MSelectionList sl;
            if (sl.add(nodeName)) {
                MObject nodeHandle;
                sl.getDependNode(0, nodeHandle);
                MFnDependencyNode fn(nodeHandle);

                AL::event::NodeEvents* event = dynamic_cast<AL::event::NodeEvents*>(fn.userNode());
                if (!event) {
                    MGlobal::displayError(
                        MString("specified node does not support the NodeEvents interface: ")
                        + nodeName);
                } else {
                    AL::event::EventId eventId = event->getId(eventName.asChar());
                    if (eventId) {
                        auto* scheduler = event->scheduler();
                        auto  cb = scheduler->buildCallback(
                            eventId, tag.asChar(), commandText.asChar(), weight, false);
                        if (cb.callbackId()) {
                            m_callbacksToInsert.push_back(std::move(cb));
                        }
                        storeId(cb.callbackId());
                        continue;
                    }
                }
            }
            storeId(0);
        }

        for (uint32_t i = 0, n = db.numberOfFlagUses("-de"); i < n; ++i) {
            MArgList args;
            db.getFlagArgumentList("de", i, args);
            union
            {
                int                   asInt[2];
                AL::event::CallbackId asId;
            };
            asInt[0] = args.asInt(0);
            asInt[1] = args.asInt(1);
            m_callbacksToDelete.push_back(asId);
        }

        setResult(returnedIds);
    } catch (MStatus status) {
        return status;
    }
    return redoItImplementation();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Callback::redoIt() { return redoItImplementation(); }

//----------------------------------------------------------------------------------------------------------------------
MStatus Callback::undoIt() { return redoItImplementation(); }

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ListEvents, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
bool ListEvents::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MSyntax ListEvents::createSyntax()
{
    MSyntax syntax;
    syntax.useSelectionAsDefault(false);
    syntax.setObjectType(MSyntax::kSelectionList, 0, 1);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ListEvents::doIt(const MArgList& args)
{
    MStatus      status = MS::kSuccess;
    MStringArray eventNames;
    try {
        MArgDatabase database(syntax(), args, &status);
        if (database.isFlagSet("-h")) {
            return MGlobal::executeCommand("AL_usdmaya_Event -h");
        }

        if (!status) {
            return status;
        }

        MSelectionList items;
        status = database.getObjects(items);
        if (status && items.length()) {
            MObject objectHandle;
            items.getDependNode(0, objectHandle);

            MFnDependencyNode fn(objectHandle, &status);
            if (status) {
                MPxNode* ptr = fn.userNode();
                if (ptr) {
                    AL::event::NodeEvents* event = dynamic_cast<AL::event::NodeEvents*>(ptr);
                    if (event) {
                        for (const auto& eventInfo : event->events()) {
                            eventNames.append(eventInfo.first.c_str());
                        }
                    }
                }
            }
        } else {
            for (auto& it : AL::event::EventScheduler::getScheduler().registeredEvents()) {
                if (!it.associatedData()) {
                    eventNames.append(it.name().c_str());
                }
            }
        }
    } catch (const MStatus&) {
    }
    setResult(eventNames);
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(TriggerEvent, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
bool TriggerEvent::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MSyntax TriggerEvent::createSyntax()
{
    MSyntax syntax;
    syntax.addFlag("-n", "-node", MSyntax::kString);
    syntax.addArg(MSyntax::kString);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TriggerEvent::doIt(const MArgList& args)
{
    MStatus      status = MS::kSuccess;
    MStringArray eventNames;
    try {
        MArgDatabase database(syntax(), args, &status);
        if (!status)
            return status;

        MString nodeName, eventName;
        database.getCommandArgument(0, eventName);

        bool nodeSpecified = database.isFlagSet("-n");
        if (nodeSpecified) {
            database.getFlagArgument("-n", 0, nodeName);

            MSelectionList items;
            items.add(nodeName);
            MObject objectHandle;
            items.getDependNode(0, objectHandle);

            MFnDependencyNode fn(objectHandle, &status);
            if (status) {
                MPxNode* ptr = fn.userNode();
                if (ptr) {
                    AL::event::NodeEvents* event = dynamic_cast<AL::event::NodeEvents*>(ptr);
                    if (event) {
                        setResult(event->triggerEvent(eventName.asChar()));
                    } else {
                        MGlobal::displayError(
                            MString("specified node does not support events: ") + nodeName);
                        return status;
                    }
                } else {
                    MGlobal::displayError(
                        MString("specified node does not support events: ") + nodeName);
                    return status;
                }
            } else {
                MGlobal::displayError(
                    MString("failed to attach function set to node: ") + nodeName);
                return status;
            }
        } else {
            setResult(AL::event::EventScheduler::getScheduler().triggerEvent(eventName.asChar()));
        }
    } catch (const MStatus&) {
    }
    setResult(eventNames);
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(DeleteCallbacks, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
bool DeleteCallbacks::isUndoable() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
MSyntax DeleteCallbacks::createSyntax() { return MSyntax(); }

//----------------------------------------------------------------------------------------------------------------------
MStatus DeleteCallbacks::doIt(const MArgList& args)
{
    for (uint32_t i = 0, n = args.length(); i < n; ++i) {
        MStatus   status;
        MIntArray items = args.asIntArray(i, &status);
        if (status && (items.length() & 1) == 0) {
            union
            {
                int                   asint[2];
                AL::event::CallbackId asid;
            };
            for (uint32_t j = 0, m = items.length(); j < m; j += 2) {
                asint[0] = items[j];
                asint[1] = items[j + 1];
                m_callbacksToDelete.push_back(asid);
            }
        } else {
            MGlobal::displayError("AL_usdmaya_DeleteEvents: failed to parse input callback IDs");
            return MS::kFailure;
        }
    }
    return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DeleteCallbacks::undoIt() { return redoItImplementation(); }

//----------------------------------------------------------------------------------------------------------------------
MStatus DeleteCallbacks::redoIt() { return redoItImplementation(); }

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ListCallbacks, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
bool ListCallbacks::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MSyntax ListCallbacks::createSyntax()
{
    MSyntax syntax;
    syntax.addArg(MSyntax::kString);
    syntax.useSelectionAsDefault(false);
    syntax.setObjectType(MSyntax::kSelectionList, 0, 1);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ListCallbacks::doIt(const MArgList& args)
{
    MStatus status = MS::kSuccess;
    try {
        MArgDatabase database(syntax(), args, &status);
        if (!status) {
            return status;
        }
        MString eventName;
        database.getCommandArgument(0, eventName);

        MSelectionList items;
        status = database.getObjects(items);
        if (status && items.length()) {
            MObject objectHandle;
            items.getDependNode(0, objectHandle);

            MIntArray         callbacks;
            MFnDependencyNode fn(objectHandle, &status);
            if (status) {
                MPxNode* ptr = fn.userNode();
                if (ptr) {
                    AL::event::NodeEvents* event = dynamic_cast<AL::event::NodeEvents*>(ptr);
                    if (event) {
                        const auto it = event->scheduler()->event(eventName.asChar());
                        if (it) {
                            for (const auto& eventInfo : it->callbacks()) {
                                union
                                {
                                    int                   ii[2];
                                    AL::event::CallbackId id;
                                };
                                id = eventInfo.callbackId();
                                callbacks.append(ii[0]);
                                callbacks.append(ii[1]);
                            }
                        }
                    }
                }
            }
            setResult(callbacks);
        } else {
            MIntArray callbacks;
            auto eventHandler = AL::event::EventScheduler::getScheduler().event(eventName.asChar());
            if (eventHandler) {
                for (const auto& eventInfo : eventHandler->callbacks()) {
                    union
                    {
                        int                   ii[2];
                        AL::event::CallbackId id;
                    };
                    id = eventInfo.callbackId();
                    callbacks.append(ii[0]);
                    callbacks.append(ii[1]);
                }
            }
            setResult(callbacks);
        }
    } catch (const MStatus&) {
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(EventLookup, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax EventLookup::createSyntax()
{
    MSyntax syntax;
    syntax.addFlag("-h", "-help");
    syntax.addFlag("-n", "-name");
    syntax.addFlag("-nd", "-node");
    syntax.addArg(MSyntax::kLong);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus EventLookup::doIt(const MArgList& args)
{
    MStatus      status = MS::kSuccess;
    MStringArray eventNames;
    try {
        MArgDatabase database(syntax(), args, &status);
        if (!status)
            return status;
        AL_MAYA_COMMAND_HELP(database, g_helpText);

        int eventId = 0;
        if (!database.getCommandArgument(0, eventId)) {
            return MS::kFailure;
        }

        AL::event::EventDispatcher* dispatcher
            = AL::event::EventScheduler::getScheduler().event(eventId);
        if (dispatcher) {
            if (database.isFlagSet("-n")) {
                setResult(dispatcher->name().c_str());
            } else if (database.isFlagSet("-nd")) {
                MString                nodeName = "";
                AL::event::NodeEvents* node = (AL::event::NodeEvents*)dispatcher->associatedData();
                if (node) {
                    MPxNode* mpxNode = dynamic_cast<MPxNode*>(node);
                    if (mpxNode) {
                        MFnDependencyNode fn(mpxNode->thisMObject());
                        nodeName = fn.name();
                    }
                }
                setResult(nodeName);
            } else {
                MGlobal::displayError("AL_usdmaya_EventLookup: no flag specified");
                return MS::kFailure;
            }
        } else {
            MGlobal::displayError("AL_usdmaya_EventLookup: invalid event specified");
            return MS::kFailure;
        }
    } catch (...) {
        return MS::kFailure;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(EventQuery, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax EventQuery::createSyntax()
{
    MSyntax syntax;
    syntax.addFlag("-h", "-help");
    syntax.addFlag("-e", "-eventId");
    syntax.addFlag("-p", "-parentId");
    syntax.addArg(MSyntax::kString);
    syntax.useSelectionAsDefault(false);
    syntax.setObjectType(MSyntax::kSelectionList, 0, 1);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus EventQuery::doIt(const MArgList& args)
{
    MStatus      status = MS::kSuccess;
    MStringArray eventNames;
    try {
        MArgDatabase database(syntax(), args, &status);
        if (!status)
            return status;
        AL_MAYA_COMMAND_HELP(database, g_helpText);

        MString eventName;
        if (!database.getCommandArgument(0, eventName)) {
            return MS::kFailure;
        }

        AL::event::EventDispatcher* dispatcher = 0;

        MSelectionList items;
        status = database.getObjects(items);
        if (status && items.length()) {
            MObject obj;
            items.getDependNode(0, obj);
            AL::event::NodeEvents* handler
                = dynamic_cast<AL::event::NodeEvents*>(MFnDependencyNode(obj).userNode());
            if (handler) {
                AL::event::EventId eventId = handler->getId(eventName.asChar());
                dispatcher = handler->scheduler()->event(eventId);
            }
        } else {
            dispatcher = AL::event::EventScheduler::getScheduler().event(eventName.asChar());
        }

        if (dispatcher) {
            if (database.isFlagSet("-p")) {
                union
                {
                    AL::event::CallbackId id;
                    int                   asInt[2];
                };
                id = dispatcher->parentCallbackId();
                appendToResult(asInt[0]);
                appendToResult(asInt[1]);
            } else if (database.isFlagSet("-e")) {
                AL::event::EventId eventId = dispatcher->eventId();
                setResult(eventId);
            } else {
                MGlobal::displayError("AL_usdmaya_EventQuery: no flag specified");
                return MS::kFailure;
            }
        } else {
            MGlobal::displayError("AL_usdmaya_EventQuery: invalid event specified");
            return MS::kFailure;
        }
    } catch (...) {
        return MS::kFailure;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(CallbackQuery, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
bool CallbackQuery::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MSyntax CallbackQuery::createSyntax()
{
    MSyntax syntax;
    syntax.addFlag("-h", "-help");
    syntax.addFlag("-e", "-eventId");
    syntax.addFlag("-u", "-userData");
    syntax.addFlag("-et", "-eventTag");
    syntax.addFlag("-ty", "-type");
    syntax.addFlag("-w", "-weight");
    syntax.addFlag("-c", "-command");
    syntax.addFlag("-fp", "-functionPointer");
    syntax.addFlag("-ce", "-childEvents");
    syntax.addArg(MSyntax::kLong);
    syntax.addArg(MSyntax::kLong);
    return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus CallbackQuery::doIt(const MArgList& args)
{
    MStatus      status = MS::kSuccess;
    MStringArray eventNames;
    try {
        MArgDatabase database(syntax(), args, &status);
        if (!status)
            return status;
        AL_MAYA_COMMAND_HELP(database, g_helpText);

        union
        {
            int                   asint[2];
            AL::event::CallbackId asCb;
        };

        if (!database.getCommandArgument(0, asint[0])
            || !database.getCommandArgument(1, asint[1])) {
            return MS::kFailure;
        }

        auto event = AL::event::EventScheduler::getScheduler().findCallback(asCb);
        if (event) {
            auto writeHex = [](const uint8_t b[8]) {
                MString           text = "0x";
                const char* const hex = "0123456789ABCDEF";
                for (int i = 0; i < 8; ++i) {
                    const uint8_t c = b[i];
                    text += hex[(c >> 4) & 0xF];
                    text += hex[c & 0xF];
                }
                return text;
            };

            if (database.isFlagSet("-ce")) {
                MIntArray                        events;
                const AL::event::EventScheduler& scheduler
                    = AL::event::EventScheduler::getScheduler();
                for (auto& e : scheduler.registeredEvents()) {
                    if (e.parentCallbackId() == asCb) {
                        events.append(e.eventId());
                    }
                }
                setResult(events);
            } else if (database.isFlagSet("-e")) {
                AL::event::EventId id = event->eventId();
                setResult(int(id));
            } else if (database.isFlagSet("-et")) {
                setResult(event->tag().c_str());
            } else if (database.isFlagSet("-ty")) {
                if (event->isPythonCallback())
                    setResult("Python");
                else if (event->isPythonCallback())
                    setResult("MEL");
                else
                    setResult("C");
            } else if (database.isFlagSet("-w")) {
                setResult(int(event->weight()));
            } else if (database.isFlagSet("-c")) {
                setResult(event->callbackText());
            } else if (database.isFlagSet("-fp")) {
                union
                {
                    uint8_t     b[8];
                    const void* p;
                };
                p = event->callback();
                setResult(writeHex(b));
            } else if (database.isFlagSet("-u")) {
                union
                {
                    uint8_t     b[8];
                    const void* p;
                };
                p = event->userData();
                setResult(writeHex(b));
            }
        }
    } catch (...) {
        return MS::kFailure;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
const char* const Event::g_helpText = R"(
    AL_usdmaya_Event Overview:

    This command allows the ability to register / unregister new events.

Global Events
-------------

    To register a new global event, simply specify the name of the event you wish to create:

        AL_usdmaya_Event "eventName";

    This call will fail if the event name is already in use. Once created, you can use the AL_usdmaya_Callback command
to register a callback against that event, e.g.

        AL_usdmaya_Callback -me "eventName" "callbackTag" 100 "print \"hello!\"";

    To trigger the event, simply pass the newly created event name to the AL_usdmaya_TriggerEvent command, e.g.

        AL_usdmaya_TriggerEvent "eventName";

    To delete the event, use the -d/-delete flag to the AL_usdmaya_Event command

        AL_usdmaya_Event -d "eventName";

Node Events
-----------

    As well as global events, it's also possible to attach an event to a Maya node. For this to work, the maya node
in question must have been derived from the NodeEvents class in C++. To register the event, specify the name of
the event, and the node you wish to register the event on.

        AL_usdmaya_Event "eventName" "mayaNode";

    This call will fail if the event name is already in use. Once created, you can use the AL_usdmaya_Callback command
to register a callback against that event, e.g.

        AL_usdmaya_Callback -mne "mayaNode" "eventName" "callbackTag" 100 "print \"hello!\"";

    To trigger the event, simply pass the newly created event name to the AL_usdmaya_TriggerEvent command, e.g.

        AL_usdmaya_TriggerEvent "eventName" "mayaNode";

    To delete the event, use the -d/-delete flag to the AL_usdmaya_Event command

        AL_usdmaya_Event -d "eventName" "mayaNode";

Parent Events
-------------

    In order to ease with debugging, it is possible to set up a parent callback ID for an event using the
-p/-parent flag


    // set up the parent event
    AL_usdmaya_Event "parentEventName";

    // add a child callback which will trigger a child event
    $cb = `AL_usdmaya_Callback -me "parentEventName" "parentTag" 100 "AL_usdmaya_TriggerEvent \"childEventName\""`;

    // set up the child event
    AL_usdmaya_Event -p $cb[0] $cb[1] "childEventName";
)";

//----------------------------------------------------------------------------------------------------------------------
const char* const EventQuery::g_helpText = R"(
    AL_usdmaya_EventQuery Overview:

)";

//----------------------------------------------------------------------------------------------------------------------
const char* const EventLookup::g_helpText = R"(
    AL_usdmaya_EventLookup Overview:

)";

//----------------------------------------------------------------------------------------------------------------------
const char* const CallbackQuery::g_helpText = R"(
    AL_usdmaya_CallbackQuery Overview:

    Given the 2 integer identifier for a callback, this command can return some information about that callback. e.g.

      // print the internal 16bit event ID
      AL_usdmaya_CallbackQuery -eventId $cb[0] $cb[1];

      // print the textual tag for the callback
      AL_usdmaya_CallbackQuery -eventTag $cb[0] $cb[1];

      // prints 'Python', 'MEL' or 'C'
      AL_usdmaya_CallbackQuery -type $cb[0] $cb[1];

      // returns the weight for the callback
      AL_usdmaya_CallbackQuery -weight $cb[0] $cb[1];

      // if the type is Python or MEL, returns the code attached to the callback
      AL_usdmaya_CallbackQuery -command $cb[0] $cb[1];

)";

//----------------------------------------------------------------------------------------------------------------------
const char* const Callback::g_helpText = R"(
    AL_usdmaya_Callback Overview:

    This command allows the user the ability to create and destroy callbacks that will be triggered during certain
actions within the workflow of the AL_USDMaya plugin. These events can be assigned to nodes, or they may be global
processes.

Why not use scriptJob / MMessage???
-----------------------------------

Problem1: One of the primary reasons why we are recommending NOT using the Maya scriptJob/MMessage system for
events, is that the order in which the callbacks are triggered very much depends on the order in which they are
registered. If you have two plugins that both listen to the same event, and happen to modify the same types of
nodes; then you can see differing behaviour when plugin1 is loaded before plugin2, and vice versa.

Problem2: Secondly, there isn't a way for a tool developer to understand which events are being triggered, and
by whom. The debugging capability of the scriptJob/MMessage system is fairly poor.

Event Weights
-------------

To solve problem 1, this event system introduces the concept of an event weight. Each callback registered must
provide its own weight value. This is simply a positive integer value that detemines when the event will be
triggered. All callbacks assigned to an event will be triggered based on their weight, so the lowest event weights
will be triggered first, highest last. This means that should you ever find that two callbacks need to be executed
in a certain order, simply modify the weight value to ensure the correct ordering.

Event Tags
----------

In order to improve the ability to debug events, each callback needs to provide its own unique global tag to
identify which tool has registered the callback. The purpose of this is to be able to see which callbacks are
triggered by an event, and which tool registered those events.

Node Based Events
-----------------

In order for a node to be compatible with this event system, the C++ definition must have been derived from the
AL::AL::event::NodeEvents interface (so it is possible internally developed maya nodes can support events, but
the standard Maya nodes types will not). To test whether a Maya node supports events, you may query support like so:

    AL_usdmaya_Callback -supportsEvents "nameOfNode"

This will returns true if the node supports the required interface, false otherwise. If a given node supports
events, you may determine the list of supported events by using the AL_usdmaya_ListEvents command, e.g.

    AL_usdmaya_ListEvents "nameOfNode";

    Let's imagine that one of the event names returned by that call was "SomeEventName", then we would be able to
register a MEL callabck like so:

    int $callbacks[] = `AL_usdmaya_Callback -mne "nameOfNode" "SomeEventName" "MyUniqueTag" 10000 $melCodeToExecute`;

The arguments to the -mne/-melNodeEvent flag are:

  1. the name of the node to attach the event to
  2. the name of the event we wish to register the callback on
  3. a unique tag to identify the tool that care about this event
  4. the weight for the callback (executed from lowest to highest)
  5. the MEL command string to execute.

The $callbacks value is a set of pairs of integers that together make up the callback ids (the event creation
flags are all multi-use, so multiple callback ID's may be returned as an array)

    AL_usdmaya_Callback -de $callbacks[0] $callbacks[1];

As well as assigning MEL code to be executed, it is also possible to assign some python code to a node, which
can be done via the -pne/-pythonNodeEvent flag:

    int $callbacks[] = `AL_usdmaya_Callback -pne "nameOfNode" "SomeEventName" "MyUniqueTag" 10000 $pythonCodeToExecute`;

Global Events
-------------

To query the list of global events (i.e. those that are not assigned to a node type), run the following command:

      AL_usdmaya_ListEvents

To assign a mel callback to an event, use the -me/-melEvent flag:

      $callbacks = `AL_usdmaya_Callback -me "SomeEventName" "MyUniqueTag" 10000 $melCodeToExecute`;

To assign a python callback to an event, use the -pe/-pythonEvent flag:

      $callbacks = `AL_usdmaya_Callback -pe "SomeEventName" "MyUniqueTag" 10000 $pythonCodeToExecute`;

In both cases the arguments are:

 1. the name of the event we wish to register the callback on
 2. a unique tag to identify the tool that care about this event
 3. the weight for the callback (executed from lowest to highest)
 4. the MEL / Python code to execute.

Returned Callback IDs
---------------------

The callback IDs used internally in C++ are 64bit unsigned integers. Sadly, MPxCommands do not allow you to
specify any 64bit values as return types. This causes a slight problem when scripts have to deal with callback
IDs! As a result of this, any callback IDs you create will be returned as a pair of integers (where each pair
represents a single callback).

Since the -me/-pe/-mne/-pne commands are all multi-use, the IDs will be returned in the following order:

  -pythonEvent / -pe
  -melEvent / -me
  -pythonNodeEvent / -pne
  -melNodeEvent / -mne

In general however, it is not recommended to mix/match the above flags. Generally speaking the reason we allow
multi use for all these flags is to allow you to do a one hit creation of all events you wish to bind to a node.



)";

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
