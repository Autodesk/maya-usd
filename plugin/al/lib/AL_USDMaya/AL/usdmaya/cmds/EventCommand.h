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
#include "AL/maya/utils/MayaHelperMacros.h"

#include <maya/MPxCommand.h>

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The base class for all commands that need to create/delete callbacks in some way. Fill
/// m_callbacksToDelete
///         with the CallbackIds you want to delete, and fill the m_callbacksToInsert array with the
///         callbacks returned from AL::AL::event::EventScheduler::buildCallback. Within the
///         undo/redo implementation of a mel command, simply call redoItImplementation. This method
///         will destroy the callbacks requested, and insert the created callbacks. Once called, the
///         values of the m_callbacksToDelete and m_callbacksToInsert will be swapped, therefore
///         calling redoItImplementation again will undo the previous action.
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
struct BaseCallbackCommand
{
    /// call within both the undo and redo methods
    MStatus redoItImplementation();
    /// the callback ids that need to be deleted
    std::vector<AL::event::CallbackId> m_callbacksToDelete;
    /// the callback structures generated from EventScheduler::buildCallback
    AL::event::Callbacks m_callbacksToInsert;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief   A command that allows you to register and unregister new Event types from script.
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class Event : public MPxCommand
{
    MString                m_eventName;
    AL::event::NodeEvents* m_associatedData = 0;
    AL::event::CallbackId  m_parentEvent = 0;
    bool                   m_deleting = false;

public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
    MStatus redoIt() override;
    MStatus undoIt() override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief   A command that allows you to query information about a specific event
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class EventQuery : public MPxCommand
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief   A command that allows you to query information about an event
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class EventLookup : public MPxCommand
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief   A command that allows you to create / delete callbacks assigned to a specific event
/// within AL_usdmaya \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class Callback
    : public MPxCommand
    , public BaseCallbackCommand
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
    MStatus redoIt() override;
    MStatus undoIt() override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command that lists the events available on either a particular node, or the global set
/// of events \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ListEvents : public MPxCommand
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command that will trigger all callbacks on an event
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class TriggerEvent : public MPxCommand
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command that will delete all callback ids specified as an argument
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class DeleteCallbacks
    : public MPxCommand
    , public BaseCallbackCommand
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
    MStatus redoIt() override;
    MStatus undoIt() override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command that lists the events available on either a particular node, or the global set
/// of events \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ListCallbacks : public MPxCommand
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command that lists the events available on either a particular node, or the global set
/// of events \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class CallbackQuery : public MPxCommand
{
public:
    AL_MAYA_DECLARE_COMMAND();

private:
    bool    isUndoable() const override;
    MStatus doIt(const MArgList& args) override;
};

/// builds the GUI for the TfDebug notices
extern void constructEventCommandGuis();

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
