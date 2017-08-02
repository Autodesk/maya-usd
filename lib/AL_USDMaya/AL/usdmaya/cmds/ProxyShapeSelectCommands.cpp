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
#include "AL/maya/CommandGuiHelper.h"
#include "AL/usdmaya/Utils.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/cmds/ProxyShapeCommands.h"
#include "AL/usdmaya/fileio/TransformIterator.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/SelectableDB.h"

#include "maya/MArgDatabase.h"
#include "maya/MFnDagNode.h"
#include "maya/MGlobal.h"
#include "maya/MSelectionList.h"
#include "maya/MStatus.h"
#include "maya/MStringArray.h"
#include "maya/MSyntax.h"
#include "maya/MDagPath.h"
#include "maya/MArgList.h"

#include <sstream>
#include <algorithm>

#include <AL/usdmaya/cmds/ProxyShapeSelectCommands.h>

namespace AL {
namespace usdmaya {
namespace cmds {

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_COMMAND(ConfigureSelectionDatabase, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax ConfigureSelectionDatabase::createSyntax()
{
  MSyntax syntax = setUpCommonSyntax();
  syntax.addFlag("-h", "-help", MSyntax::kNoArg);
  syntax.addFlag("-rs", "-restrictSelection", MSyntax::kBoolean);
  syntax.addFlag("-ps", "-printSelectables", MSyntax::kNoArg);
  return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
bool ConfigureSelectionDatabase::isUndoable() const
{
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ConfigureSelectionDatabase::doIt(const MArgList& args)
{
  TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("SelectDatabase::doIt\n");

  try
  {
    MArgDatabase db = makeDatabase(args);
    AL_MAYA_COMMAND_HELP(db, g_helpText);
    m_proxy = getShapeNode(db);
    if(!m_proxy)
    {
      throw MS::kFailure;
    }

    if(db.isFlagSet("-ps"))
    {
      m_printSelection = true;
    }
    else if(db.isFlagSet("-rs"))
    {
      db.getFlagArgument("-rs", 0, m_hasRequestedRestriction);
    }
  }
  catch(const MStatus& status)
  {
    return status;
  }
  return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------

MStatus ConfigureSelectionDatabase::undoIt()
{

  if(m_printSelection)
  {}
  else
  {
    if(m_hasRequestedRestriction)
    {
      TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("SelectDatabase:Undo:Unrestricting selection\n");
      m_proxy->unrestrictSelection();
    }
    else
    {
      TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("SelectDatabase:Undo:Restricting selection\n");
      m_proxy->restrictSelection();
    }
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------

MStatus ConfigureSelectionDatabase::redoIt()
{
  if(m_printSelection)
  {
    const AL::usdmaya::SelectableDB& selectionDb = m_proxy->selectableDB();
    for(auto selectable : selectionDb.getSelectablePaths())
    {
      MGlobal::displayInfo("ConfigureSelectionDatabase::Selectable string " + MString(selectable.GetString().c_str()));
    }
    MString isRestricted = m_proxy->isSelectionRestricted()? "yes" : "no";
    MGlobal::displayInfo("ConfigureSelectionDatabase::Is selection restricted? " + isRestricted);
  }
  else
  {
    if(m_hasRequestedRestriction)
    {
      m_proxy->restrictSelection();
    }
    else
    {
      m_proxy->unrestrictSelection();
    }
  }

  return MS::kSuccess;
}

const char* const ConfigureSelectionDatabase::g_helpText = R"(
AL_usdmaya_ConfigureSelectionDatabase Overview:
  This command configures the proxy shapes selection database's state. The selection database store paths that determine which prim paths can be selected.

  Through this command you can disable the SelectionDatabase which will make everything selectable, enabling the selection database makes everything unselectable except 
  for the usd prim and their children that have the selection tag in their properties.

  syntax.addFlag("-rs", "-restrictSelection", MSyntax::kBoolean);
  syntax.addFlag("-ps", "-printSelectables", MSyntax::kNoArg);

    -rs   / -restrictSelection      : If true it enables the selection restriction, if False the there will be no selection restriction
    -ps   / -printSelectables       : Prints which prims are are being tracked as selectable.

  Enable the selection restriction
  AL_usdmaya_ConfigureSelectionDatabase -rs true "AL_usdmaya_ProxyShape1"

  Disable the selection restriction
  AL_usdmaya_ConfigureSelectionDatabase -rs true "AL_usdmaya_ProxyShape1"

  Print the restriction state of the SelectionDB and all of the prims that are tagged as selectable
  AL_usdmaya_ConfigureSelectionDatabase -ps "AL_usdmaya_ProxyShape1" 

)";

//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
} // cmds
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
