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
#include "AL/usdmaya/Common.h"

#include "maya/MPxCommand.h"

#include "pxr/pxr.h"

#include "AL/usdmaya/EventHandler.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
/// \brief   A command that allows you to create / delete callbacks assigned to a specific event within AL_usdmaya
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
struct BaseCallbackCommand
{
  MStatus redoItImplementation();
  std::vector<CallbackId> m_callbacksToDelete;
  Callbacks m_callbacksToInsert;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief   A command that allows you to create / delete callbacks assigned to a specific event within AL_usdmaya
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class Event
  : public MPxCommand
{
  MString m_eventName;
  NodeEvents* m_associatedData = 0;
  CallbackId m_parentEvent = 0;
  bool m_deleting = false;
public:
  AL_MAYA_DECLARE_COMMAND();
private:
  bool isUndoable() const override;
  MStatus doIt(const MArgList& args) override;
  MStatus redoIt() override;
  MStatus undoIt() override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief   A command that allows you to query information about an event
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class EventQuery
  : public MPxCommand
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
class EventLookup
  : public MPxCommand
{
public:
  AL_MAYA_DECLARE_COMMAND();
private:
  MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief   A command that allows you to create / delete callbacks assigned to a specific event within AL_usdmaya
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class Callback
  : public MPxCommand,
    public BaseCallbackCommand
{
public:
  AL_MAYA_DECLARE_COMMAND();
private:
  bool isUndoable() const override;
  MStatus doIt(const MArgList& args) override;
  MStatus redoIt() override;
  MStatus undoIt() override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command that lists the events available on either a particular node, or the global set of events
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ListEvents
  : public MPxCommand
{
public:
  AL_MAYA_DECLARE_COMMAND();
private:
  bool isUndoable() const override;
  MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command that will trigger all callbacks on an event
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class TriggerEvent
  : public MPxCommand
{
public:
  AL_MAYA_DECLARE_COMMAND();
private:
  bool isUndoable() const override;
  MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command that will delete all callback ids specified as an argument
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class DeleteCallbacks
  : public MPxCommand,
    public BaseCallbackCommand
{
public:
  AL_MAYA_DECLARE_COMMAND();
private:
  bool isUndoable() const override;
  MStatus doIt(const MArgList& args) override;
  MStatus redoIt() override;
  MStatus undoIt() override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command that lists the events available on either a particular node, or the global set of events
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class ListCallbacks
  : public MPxCommand
{
public:
  AL_MAYA_DECLARE_COMMAND();
private:
  bool isUndoable() const override;
  MStatus doIt(const MArgList& args) override;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A command that lists the events available on either a particular node, or the global set of events
/// \ingroup commands
//----------------------------------------------------------------------------------------------------------------------
class CallbackQuery
  : public MPxCommand
{
public:
  AL_MAYA_DECLARE_COMMAND();
private:
  bool isUndoable() const override;
  MStatus doIt(const MArgList& args) override;
};

/// builds the GUI for the TfDebug notices
extern void constructEventCommandGuis();

} // cmds
} // usdmaya
} // AL



