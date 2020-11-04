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
#include "AL/usdmaya/cmds/DebugCommands.h"

#include "AL/maya/utils/MenuBuilder.h"
#include "AL/usdmaya/DebugCodes.h"

#include <pxr/base/tf/debug.h>

#include <maya/MArgDatabase.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace cmds {

AL_MAYA_DEFINE_COMMAND(UsdDebugCommand, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
MSyntax UsdDebugCommand::createSyntax()
{
    MSyntax syn;
    syn.addFlag("-h", "-help", MSyntax::kNoArg);
    syn.addFlag("-ls", "-listSymbols", MSyntax::kNoArg);
    syn.addFlag("-en", "-enable", MSyntax::kString);
    syn.addFlag("-ds", "-disable", MSyntax::kString);
    syn.addFlag("-st", "-state", MSyntax::kString);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
bool UsdDebugCommand::isUndoable() const { return false; }

//----------------------------------------------------------------------------------------------------------------------
MStatus UsdDebugCommand::doIt(const MArgList& argList)
{
    TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("AL_usdmaya_UsdDebugCommand::doIt\n");
    try {
        MStatus      status;
        MArgDatabase args(syntax(), argList, &status);
        if (!status)
            return status;

        AL_MAYA_COMMAND_HELP(args, g_helpText);

        const bool listSymbols = args.isFlagSet("-ls");
        if (listSymbols) {
            MStringArray returned;
            auto         symbols = TfDebug::GetDebugSymbolNames();
            for (auto symbol : symbols) {
                returned.append(MString(symbol.c_str(), symbol.size()));
            }
            setResult(returned);
        } else if (args.isFlagSet("-en")) {
            MString arg;
            args.getFlagArgument("-en", 0, arg);
            TfDebug::SetDebugSymbolsByName(arg.asChar(), true);
        } else if (args.isFlagSet("-ds")) {
            MString arg;
            args.getFlagArgument("-ds", 0, arg);
            TfDebug::SetDebugSymbolsByName(arg.asChar(), false);
        } else if (args.isFlagSet("-st")) {
            MString arg;
            args.getFlagArgument("-st", 0, arg);
            bool state = TfDebug::IsDebugSymbolNameEnabled(arg.asChar());
            setResult(state);
        }
    } catch (const MStatus&) {
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
// The MEL script user interface code for the debug GUI
//----------------------------------------------------------------------------------------------------------------------
static const char* const g_usdmaya_debug_gui = R"(
global proc AL_usdmaya_debug_onEnableAllCB()
{
  $ca = `columnLayout -q -ca "USD_DEBUG_COLOUMNS"`;
  for($c in $ca)
  {
    $da = `frameLayout -q -ca $c`;
    $checks = `columnLayout -q -ca $da[0]`;
    for($d in $checks)
    {
      if(!`checkBox -q -v $d`)
      {
        $label = `checkBox -q -l $d`;
        AL_usdmaya_UsdDebugCommand -en $label;
        checkBox -e -v true $d;
      }
    }
  }
}
global proc AL_usdmaya_debug_onDisableAllCB()
{
  $ca = `columnLayout -q -ca "USD_DEBUG_COLOUMNS"`;
  for($c in $ca)
  {
    $da = `frameLayout -q -ca $c`;
    $checks = `columnLayout -q -ca $da[0]`;
    for($d in $checks)
    {
      if(`checkBox -q -v $d`)
      {
        $label = `checkBox -q -l $d`;
        AL_usdmaya_UsdDebugCommand -ds $label;
        checkBox -e -v false $d;
      }
    }
  }
}
global proc AL_usdmaya_debug_gui()
{
  if(!`window -ex "USD_DEBUG_WINDOW"`)
  {
    $win = `window -title "Usd Debug Symbols" -mb true "USD_DEBUG_WINDOW"`;
    $menu = `menu -label "Debugging"`;
    $m1 = `menuItem -label "Enable All" -c "AL_usdmaya_debug_onEnableAllCB"`;
    $m2 = `menuItem -label "Disable All" -c "AL_usdmaya_debug_onDisableAllCB"`;
    string $ff = `formLayout`;
    string $sl = `scrollLayout`;
    formLayout -e -af $sl "top" 5 -af $sl "left" 5 -af $sl "right" 5 -af $sl "bottom" 5 $ff;
    string $rl = `columnLayout -adjustableColumn true "USD_DEBUG_COLOUMNS"`;
    string $lastKey = ".....";
    string $lms[] = `AL_usdmaya_UsdDebugCommand -ls`;

    string $ii;
    for($ii in $lms)
    {
      if(!startsWith($ii, $lastKey))
      {
        if($lastKey != ".....")
        {
          setParent ..;
          setParent ..;
        }
        $lastKey = "";
        for($j = 0; $j < size($ii); ++$j)
        {
          int $place = $j + 1;
          string $sub = substring( $ii, ($j + 1), ($j + 1) );
          if($sub == "_")
            break;
          $lastKey += $sub;
        }
        frameLayout -l $lastKey -cll true;
        columnLayout;
      }
      $state = `AL_usdmaya_UsdDebugCommand -st $ii`;
      $command = "if(#1) AL_usdmaya_UsdDebugCommand -en \"" + $ii + "\"; else AL_usdmaya_UsdDebugCommand -ds \"" + $ii + "\";";
      $cb = `checkBox -l $ii -v $state -cc $command`;
    }
    showWindow;
  }
}
)";

//----------------------------------------------------------------------------------------------------------------------
void constructDebugCommandGuis()
{
    MGlobal::executeCommand(g_usdmaya_debug_gui);
    AL::maya::utils::MenuBuilder::addEntry("USD/Debug/TfDebug Options", "AL_usdmaya_debug_gui");
}

const char* const UsdDebugCommand::g_helpText = R"(
    AL_usdmaya_UsdDebugCommand Overview:

      This command allows you to modify the enabled/disabled state of the various TfDebug notifications. To retrieve a
      list of the debug symbols, use the -ls/-listSymbols flag:

        AL_usdmaya_UsdDebugCommand -ls;

      To enable a particiular noticification, use the -en/-enable flag, e.g.

        AL_usdmaya_UsdDebugCommand -en "ALUSDMAYA_TRANSLATORS";

      To find out whether a notification is enabled, use the -st/-state flag

        AL_usdmaya_UsdDebugCommand -st "ALUSDMAYA_TRANSLATORS";

      to disable a notification, use the -ds/-disable flags:

        AL_usdmaya_UsdDebugCommand -en "ALUSDMAYA_TRANSLATORS";

)";

//----------------------------------------------------------------------------------------------------------------------
} // namespace cmds
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
