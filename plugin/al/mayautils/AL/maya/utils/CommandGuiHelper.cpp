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
#include "AL/maya/utils/CommandGuiHelper.h"

#include "AL/maya/utils/MenuBuilder.h"
#include "AL/maya/utils/Utils.h"

#include <maya/MArgDatabase.h>
#include <maya/MColor.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>
#include <maya/MVector.h>

#include <iostream>
#include <string>

namespace AL {
namespace maya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
std::vector<std::pair<MString, GenerateListFn>> CommandGuiListGen::m_funcs;
AL_MAYA_DEFINE_COMMAND(CommandGuiListGen, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
int32_t
CommandGuiListGen::registerListFunc(GenerateListFn generateListFunc, const MString& menuName)
{
    int32_t n = (m_funcs.size());
    m_funcs.emplace_back(menuName, generateListFunc);
    return n;
}

//----------------------------------------------------------------------------------------------------------------------
MSyntax CommandGuiListGen::createSyntax()
{
    MSyntax syn;
    syn.addArg(MSyntax::kLong);
    return syn;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus CommandGuiListGen::doIt(const MArgList& args)
{
    MString context;
    int32_t genListId = 0;

    MStatus      status;
    MArgDatabase database(syntax(), args, &status);
    if (status) {
        // extract the list ID
        if (!database.getCommandArgument(0, genListId)) {
            return MS::kFailure;
        }

        // ensure the list id is valid
        if (genListId < 0 || size_t(genListId) >= m_funcs.size()) {
            MGlobal::displayError("Invalid gen list ID for the GUI");
            return MS::kFailure;
        }

        MStringArray strings = m_funcs[genListId].second(m_funcs[genListId].first);

        MString       result;
        const MString append_begin("menuItem -l \"");
        const MString append_mid("\" -p ");
        const MString append_end(";");
        for (uint32_t i = 0, n = strings.length(); i != n; ++i) {
            result
                += append_begin + strings[i] + append_mid + m_funcs[genListId].first + append_end;
        }
        setResult(result);
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
CommandGuiHelper::CommandGuiHelper(
    const char* commandName,
    const char* menuItemPath,
    bool        checkBoxValue)
    : m_checkBoxCommand(true)
{
    m_hasFilePath = false;

    // insert into the main menu
    MenuBuilder::addEntry(menuItemPath, commandName, true, checkBoxValue);
}

//----------------------------------------------------------------------------------------------------------------------
CommandGuiHelper::CommandGuiHelper(
    const char* commandName,
    const char* windowTitle,
    const char* doitLabel,
    const char* menuItemPath,
    bool        hasOptionBox)
    : m_checkBoxCommand(false)
{
    m_hasFilePath = false;
    m_commandName = commandName;
    // generate and execute the main option box routine
    std::string mycmd_optionGUI = commandName;
    mycmd_optionGUI += "_optionGUI";

    // insert into the main menu
    if (hasOptionBox) {
        MenuBuilder::addEntry(
            menuItemPath,
            (std::string("execute_") + mycmd_optionGUI).c_str(),
            (std::string("build_") + mycmd_optionGUI).c_str());
    } else {
        MenuBuilder::addEntry(menuItemPath, (std::string("build_") + mycmd_optionGUI).c_str());
    }

    std::ostringstream ui;
    ui << "global proc build_" << mycmd_optionGUI
       << "()\n"
          "{\n"
          "  if(`window -q -ex \""
       << mycmd_optionGUI
       << "\"`)\n"
          "  {\n"
          "    if(`window -q -visible \""
       << mycmd_optionGUI
       << "\"`) return;\n"
          // If there was an error building the window, the window may exist, but not be shown.
          // We assume that if it's not visible, there was an error, so we delete it and try again.
          "    deleteUI \""
       << mycmd_optionGUI
       << "\";\n"
          "  }\n"
          "  $window = `window -title \""
       << windowTitle << "\" -w 550 -h 350 \"" << mycmd_optionGUI
       << "\"`;\n"
          "  $menuBarLayout = `menuBarLayout`;\n"
          "    $menu = `menu -label \"Edit\"`;\n"
          "      menuItem -label \"Save Settings\" -c \"save_"
       << mycmd_optionGUI
       << "\";\n"
          "      menuItem -label \"Reset Settings\" -c \"reset_"
       << mycmd_optionGUI
       << "\";\n"
          "  setParent $window;\n"
          "  $form = `formLayout -numberOfDivisions 100`;\n"
          "    $columnLayout = `frameLayout -cll 0 -bv 1 -lv 0`;\n"
          "      rowLayout -cw 1 170 -nc 2 -ct2 \"left\" \"right\" -adj 2 -rat 1 \"top\" 0 -rat 2 "
          "\"top\" 0;\n"
          "        columnLayout -adj 1 -cat \"both\" 1 -rs 2;\n"
          "          build_"
       << commandName
       << "_labels();\n"
          "        setParent ..;\n"
          "        columnLayout -adj 1 -cat \"both\" 1 -rs 2;\n"
          "          build_"
       << commandName
       << "_controls();\n"
          "        setParent ..;\n"
          "      setParent ..;\n"
          "    setParent ..;\n"
          // Create / Apply / Cancel
          "    $rowLayout = `paneLayout -cn \"vertical3\"`;\n"
          "      $doit = `button -label \""
       << doitLabel << "\" -c (\"save_" << mycmd_optionGUI << ";execute_" << mycmd_optionGUI
       << ";deleteUI \" + $window)`;\n"
          "      $saveit = `button -label \"Apply\" -c \"save_"
       << mycmd_optionGUI
       << "\"`;\n"
          "      $close = `button -label \"Close\" -c (\"deleteUI \" + $window)`;\n"
          "    setParent ..;\n"
          "  formLayout -e\n"
          "  -attachForm $columnLayout \"top\" 1\n"
          "  -attachForm $columnLayout \"left\" 1\n"
          "  -attachForm $columnLayout \"right\" 1\n"
          "  -attachControl $columnLayout \"bottom\" 5 $rowLayout\n"
          "  -attachForm $rowLayout \"left\" 5\n"
          "  -attachForm $rowLayout \"right\" 5\n"
          "  -attachForm $rowLayout \"bottom\" 5\n"
          "  -attachNone $rowLayout \"top\"\n"
          "  $form;\n"
          "  init_"
       << mycmd_optionGUI
       << "();\n"
          "  load_"
       << mycmd_optionGUI
       << "();\n"
          "  showWindow;\n"
          "}\n";

    MGlobal::executeCommand(MString(ui.str().data(), ui.str().length()));

    // if you want to validate the output code
    if (AL_MAYAUTILS_DEBUG) {
        std::cout << ui.str() + "\n" << std::endl;
    }

    // begin construction of the 6 utils functions for this dialog
    m_init << "global proc init_" << mycmd_optionGUI << "()\n{\n";
    m_save << "global proc save_" << mycmd_optionGUI << "()\n{\n";
    m_load << "global proc load_" << mycmd_optionGUI << "()\n{\n";
    m_reset << "global proc reset_" << mycmd_optionGUI << "()\n{\n";
    m_execute << "global proc execute_" << mycmd_optionGUI << "()\n{\n  string $str = \""
              << commandName << " \";\n";
    m_labels << "global proc build_" << commandName << "_labels()\n{\n";
    m_controls << "global proc build_" << commandName << "_controls()\n{\n";
}

//----------------------------------------------------------------------------------------------------------------------
CommandGuiHelper::~CommandGuiHelper()
{
    if (m_checkBoxCommand) {
        return;
    }

    // close off our util functions
    m_init << "}\n";
    m_save << "}\n";
    m_load << "}\n";
    m_reset << "}\n";
    m_execute << "  eval $str;\n}\n";
    m_labels << "}\n";
    m_controls << "}\n";

    // if you want to validate the output code
    if (AL_MAYAUTILS_DEBUG) {
        std::cout << m_global.str() + "\n" << std::endl;
        std::cout << m_init.str() + "\n" << std::endl;
        std::cout << m_save.str() + "\n" << std::endl;
        std::cout << m_load.str() + "\n" << std::endl;
        std::cout << m_reset.str() + "\n" << std::endl;
        std::cout << m_execute.str() + "\n" << std::endl;
        std::cout << m_labels.str() + "\n" << std::endl;
        std::cout << m_controls.str() + "\n" << std::endl;
    }

    static const char* const alFileDialogHandler
        = "global proc alFileDialogHandler(string $filter, string $control, int $mode)\n"
          "{\n"
          "  string $result[] = `fileDialog2 -ff $filter -ds 2 -fm $mode`;\n"
          "  if(size($result))\n"
          "  {\n"
          "    string $r = $result[0];\n"
          "    for($i = 1; $i < size($result); ++$i)\n"
          "      $r += (\";\" + $result[$i]);\n"
          "    textFieldButtonGrp -e -fi $r $control;\n"
          "  }\n"
          "}\n";

    // if we happen to have a file dialog knocking about in our GUI, ensure the handler is
    // available.
    if (m_hasFilePath) {
        MGlobal::executeCommand(alFileDialogHandler);
    }

    // and execute them
    MGlobal::executeCommand(MString(m_global.str().data(), m_global.str().size()));
    MGlobal::executeCommand(MString(m_init.str().data(), m_init.str().size()));
    MGlobal::executeCommand(MString(m_save.str().data(), m_save.str().size()));
    MGlobal::executeCommand(MString(m_load.str().data(), m_load.str().size()));
    MGlobal::executeCommand(MString(m_reset.str().data(), m_reset.str().size()));
    MGlobal::executeCommand(MString(m_execute.str().data(), m_execute.str().size()));
    MGlobal::executeCommand(MString(m_labels.str().data(), m_labels.str().size()));
    MGlobal::executeCommand(MString(m_controls.str().data(), m_controls.str().size()));
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addExecuteText(const char* toAdd)
{
    // TODO: write a proper string encoder for MEL (tricky, because can't represent
    // all 256 character-bytes using mel string literals - would have to resort to
    // calling out to python for a truly string rerpr encoder.
    // (note that even mel's builtin "encodeString" command doesn't actually always
    // return a mel string that will evaluate to the input - ie, it will return
    // "\004", but that isn't a valid mel escape sequence, and will just result in "004"

    // for now, just check to make sure we have printable characters... only need to
    // implement a full MEL-string-encoder if we really have a need...
    m_execute << "    $str += \"";
    bool printedError = false;

    for (size_t i = 0; toAdd[i] != '\0'; ++i) {
        if (toAdd[i] < 32 || toAdd[i] > 126) {
            if (toAdd[i] == '\b')
                m_execute << "\\b";
            else if (toAdd[i] == '\t')
                m_execute << "\\t";
            else if (toAdd[i] == '\n')
                m_execute << "\\n";
            else if (toAdd[i] == '\r')
                m_execute << "\\r";
            // Just throwing, if we ever need to use a string with weird chars, need to
            // update this function...
            else if (!printedError) {
                // Make our own stream (instead of steaming straigt to cerr)
                // both because we also want to output to maya, and
                // because we need to set a flag (ie, std::hex)
                std::ostringstream err;
                err << "CommandGuiHelper::addExecuteText encountered bad character at index ";
                err << i;
                err << ": 0x";
                err << std::hex;
                // if you just reinterpret_cast as uint8_t, it still treats as a char
                err << static_cast<int>(reinterpret_cast<const unsigned char*>(toAdd)[i]);
                std::string errStr = err.str();
                MGlobal::displayError(errStr.c_str());
                std::cerr << errStr;
                printedError = true;
            }
        } else if (toAdd[i] == '"')
            m_execute << "\\\"";
        else if (toAdd[i] == '\\')
            m_execute << "\\\\";
        else
            m_execute << toAdd[i];
    }
    m_execute << "\";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addFlagOption(
    const char* commandFlag,
    const char* label,
    const bool  defaultVal,
    bool        persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  checkBox -l \"\" -h 20 " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -iv \"" << optionVar << "\" " << defaultVal << ";\n";
        m_load << "  checkBox -e -v " << getOptionVar << " " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -iv \"" << optionVar << "\" `checkBox -q -v " << optionVar << "`;\n";

        // build up command string using optionVar value
        m_execute << "  if(" << getOptionVar << ") $str += \" -" << commandFlag << " \";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  checkBox -e -v " << defaultVal << " " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`checkBox -ex " << optionVar << "` && `checkBox -q -v " << optionVar
                  << "`)\n";
        m_execute << "    $str += \" -" << commandFlag << " \";\n";
    }

    // reset checkbox back to the default value
    m_reset << "  checkBox -e -v " << defaultVal << " " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addBoolOption(
    const char* commandFlag,
    const char* label,
    const bool  defaultVal,
    bool        persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  checkBox -l \"\" -h 20 " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -iv \"" << optionVar << "\" " << defaultVal << ";\n";
        m_load << "  checkBox -e -v " << getOptionVar << " " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -iv \"" << optionVar << "\" `checkBox -q -v " << optionVar << "`;\n";

        // build up command string using optionVar value
        m_execute << "  $str += \" -" << commandFlag << " \" + " << getOptionVar << ";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  checkBox -e -v " << defaultVal << " " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`checkBox -ex " << optionVar << "`)\n";
        m_execute << "    $str += \" -" << commandFlag << " \" + `checkBox -q -v " << optionVar
                  << "`;\n";
    }

    // reset checkbox back to the default value
    m_reset << "  checkBox -e -v " << defaultVal << " " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addListOption(
    const char*    commandFlag,
    const char*    label,
    GenerateListFn generateList,
    bool           isMandatory)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q -sl \"") + optionVar + "\"`";

    m_global << "global proc " << optionVar
             << "_handle(string $sl) {\n"
                "  global string $"
             << optionVar << "_sl; $" << optionVar << "_sl = $sl;\n}\n";

    // Use `optionMenu -q -value` instead of "#1" because there's no good way to do string-escaping
    // with "#1"
    m_controls << "  optionMenu -h 20 -cc \"" << optionVar << "_handle `optionMenu -q -value "
               << optionVar << "`\" " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // register callback
    const int32_t id
        = CommandGuiListGen::registerListFunc(generateList, AL::maya::utils::convert(optionVar));

    // call into command to build up menu items for optionMenu control
    m_load << "  $temp_str = `AL_usdmaya_CommandGuiListGen " << id << "`;\n";
    m_load << "  eval($temp_str);\n";

    //
    m_execute << "  global string $" << optionVar << "_sl;"
              << "  $str += \" ";
    if (!isMandatory) {
        m_execute << "-" << commandFlag << " ";
    }
    m_execute << "$" << optionVar << "_sl \";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addIntOption(
    const char* commandFlag,
    const char* label,
    const int   defaultVal,
    bool        persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  intField -h 20 " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -iv \"" << optionVar << "\" " << defaultVal << ";\n";
        m_load << "  intField -e -v " << getOptionVar << " " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -iv \"" << optionVar << "\" `intField -q -v " << optionVar << "`;\n";

        // build up command string using optionVar value
        m_execute << "  $str += \" -" << commandFlag << " \" + " << getOptionVar << ";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  intField -e -v " << defaultVal << " " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`intField -ex " << optionVar << "`)\n";
        m_execute << "    $str += \" -" << commandFlag << " \" + `intField -q -v " << optionVar
                  << "`;\n";
    }

    // reset checkbox back to the default value
    m_reset << "  intField -e -v " << defaultVal << " " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addIntSliderOption(
    const char* commandFlag,
    const char* label,
    const int   minVal,
    const int   maxVal,
    const int   defaultVal,
    bool        persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  intSlider -h 20 -min " << minVal << " -max " << maxVal << " " << optionVar
               << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -iv \"" << optionVar << "\" " << defaultVal << ";\n";
        m_load << "  intSlider -e -v " << getOptionVar << " " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -iv \"" << optionVar << "\" `intSlider -q -v " << optionVar
               << "`;\n";

        // build up command string using optionVar value
        m_execute << "  $str += \" -" << commandFlag << " \" + " << getOptionVar << ";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  intSlider -e -v " << defaultVal << " " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`intSlider -ex " << optionVar << "`)\n";
        m_execute << "    $str += \" -" << commandFlag << " \" + `intSlider -q -v " << optionVar
                  << "`;\n";
    }

    // reset checkbox back to the default value
    m_reset << "  intSlider -e -v " << defaultVal << " " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addInt2Option(
    const char*   commandFlag,
    const char*   label,
    const int32_t defaultVal[2],
    bool          persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  intFieldGrp -h 20 -nf 2 " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -iv \"" << optionVar << "\" " << defaultVal[0] << " -iva \""
               << optionVar << "\" " << defaultVal[1] << ";\n";

        m_load << "  $iv = " << getOptionVar << ";\n";
        m_load << "  intFieldGrp -e -v1 $iv[0] -v2 $iv[1] " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -iv \"" << optionVar << "\" `intFieldGrp -q -v1 " << optionVar
               << "` -iva \"" << optionVar << "\" `intFieldGrp -q -v2 " << optionVar << "`;\n";

        // build up command string using optionVar value
        m_execute << "  $iv = " << getOptionVar << ";\n";
        m_execute << "  $str += \" -" << commandFlag << " \" + $iv[0] + \" \" + $iv[1] + \" \";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  intFieldGrp -e -v1 " << defaultVal[0] << " -v2 " << defaultVal[1] << " -v3 "
               << defaultVal[2] << " " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`intFieldGrp -ex " << optionVar << "`)\n";
        m_execute << "  $str += \" -" << commandFlag << " \" + `intFieldGrp -q -v1 " << optionVar
                  << "` + \" \" + `intFieldGrp -q -v2 " << optionVar << "`;\n";
    }

    // reset checkbox back to the default value
    m_reset << "  intFieldGrp -e -v1 " << defaultVal[0] << " -v2 " << defaultVal[1] << " "
            << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addInt3Option(
    const char*   commandFlag,
    const char*   label,
    const int32_t defaultVal[3],
    bool          persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  intFieldGrp -h 20 -nf 3 " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -iv \"" << optionVar << "\" " << defaultVal[0] << " -iva \""
               << optionVar << "\" " << defaultVal[1] << " -iva \"" << optionVar << "\" "
               << defaultVal[2] << ";\n";

        m_load << "  $iv = " << getOptionVar << ";\n";
        m_load << "  intFieldGrp -e -v1 $iv[0] -v2 $iv[1] -v3 $iv[2] " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -iv \"" << optionVar << "\" `intFieldGrp -q -v1 " << optionVar
               << "` -iva \"" << optionVar << "\" `intFieldGrp -q -v2 " << optionVar << "` -iva \""
               << optionVar << "\" `intFieldGrp -q -v3 " << optionVar << "`;\n";

        // build up command string using optionVar value
        m_execute << "  $iv = " << getOptionVar << ";\n";
        m_execute << "  $str += \" -" << commandFlag
                  << " \" + $iv[0] + \" \" + $iv[1] + \" \" + $iv[2] + \" \";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  intFieldGrp -e -v1 " << defaultVal[0] << " -v2 " << defaultVal[1] << " -v3 "
               << defaultVal[2] << " " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`intFieldGrp -ex " << optionVar << "`)\n";
        m_execute << "  $str += \" -" << commandFlag << " \" + `intFieldGrp -q -v1 " << optionVar
                  << "` + \" \" + `intFieldGrp -q -v2 " << optionVar
                  << "` + \" \" + `intFieldGrp -q -v3 " << optionVar << "`;\n";
    }

    // reset checkbox back to the default value
    m_reset << "  intFieldGrp -e -v1 " << defaultVal[0] << " -v2 " << defaultVal[1] << " -v3 "
            << defaultVal[2] << " " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addInt4Option(
    const char*   commandFlag,
    const char*   label,
    const int32_t defaultVal[4],
    bool          persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  intFieldGrp -h 20 -nf 4 " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -iv \"" << optionVar << "\" " << defaultVal[0] << " -iva \""
               << optionVar << "\" " << defaultVal[1] << " -iva \"" << optionVar << "\" "
               << defaultVal[2] << " -iva \"" << optionVar << "\" " << defaultVal[3] << ";\n";

        m_load << "  $iv = " << getOptionVar << ";\n";
        m_load << "  intFieldGrp -e -v1 $iv[0] -v2 $iv[1] -v3 $iv[2] -v4 $iv[3] " << optionVar
               << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -iv \"" << optionVar << "\" `intFieldGrp -q -v1 " << optionVar
               << "` -iva \"" << optionVar << "\" `intFieldGrp -q -v2 " << optionVar << "` -iva \""
               << optionVar << "\" `intFieldGrp -q -v3 " << optionVar << "` -iva \"" << optionVar
               << "\" `intFieldGrp -q -v4 " << optionVar << "`;\n";

        // build up command string using optionVar value
        m_execute << "  $iv = " << getOptionVar << ";\n";
        m_execute << "  $str += \" -" << commandFlag
                  << " \" + $iv[0] + \" \" + $iv[1] + \" \" + $iv[2] + \" \" + $iv[3] + \" \";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  intFieldGrp -e -v1 " << defaultVal[0] << " -v2 " << defaultVal[1] << " -v3 "
               << defaultVal[2] << " -v4 " << defaultVal[3] << " " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`intFieldGrp -ex " << optionVar << "`)\n";
        m_execute << "  $str += \" -" << commandFlag << " \" + `intFieldGrp -q -v1 " << optionVar
                  << "` + \" \" + `intFieldGrp -q -v2 " << optionVar
                  << "` + \" \" + `intFieldGrp -q -v3 " << optionVar
                  << "` + \" \" + `intFieldGrp -q -v4 " << optionVar << "`;\n";
    }

    // reset checkbox back to the default value
    m_reset << "  intFieldGrp -e -v1 " << defaultVal[0] << " -v2 " << defaultVal[1] << " -v3 "
            << defaultVal[2] << " -v4 " << defaultVal[3] << " " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addEnumOption(
    const char*       commandFlag,
    const char*       label,
    int               defaultIndex,
    const char* const enumNames[],
    const int32_t     enumValues[],
    bool              persist,
    bool              passAsString)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    std::ostringstream enumValuesAsString;
    if (passAsString) {
        enumValuesAsString << "$evs = {\"" << enumNames[0] << "\"";
        for (int i = 1; enumNames[i]; ++i) {
            enumValuesAsString << ",\"" << enumNames[i] << "\"";
        }
        enumValuesAsString << "};";
    } else if (enumValues) {
        enumValuesAsString << "$ev = {" << enumValues[0];
        for (int i = 1; enumNames[i]; ++i) {
            enumValuesAsString << "," << enumValues[i];
        }
        enumValuesAsString << "};";
    }

    // generate checkbox control for boolean argument
    m_controls << "  optionMenu -h 20 " << optionVar << ";\n";
    for (int i = 0; enumNames[i]; ++i) {
        m_controls << "    menuItem -label \"" << enumNames[i] << "\";\n";
    }

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -iv \"" << optionVar << "\" " << defaultIndex << ";\n";
        m_load << "  optionMenu -e -sl (" << getOptionVar << " + 1) " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -iv \"" << optionVar << "\" (`optionMenu -q -sl " << optionVar
               << "` - 1);\n";

        // build up command string using optionVar value
        if (passAsString) {
            m_execute << "  {\n";
            m_execute << "    " << enumValuesAsString.str() << "\n";
            m_execute << "    int $__index = " << getOptionVar << ";\n";
            m_execute << "    $str += \" -" << commandFlag
                      << " \\\"\" + $evs[ $__index ] + \"\\\"\";\n";
            m_execute << "  }\n";
        } else if (enumValues) {
            m_execute << "  {\n";
            m_execute << "    " << enumValuesAsString.str() << "\n";
            m_execute << "    int $__index = " << getOptionVar << ";\n";
            m_execute << "    $str += \" -" << commandFlag << " \" + $ev[ $__index ];\n";
            m_execute << "  }\n";
        } else {
            m_execute << "  $str += \" -" << commandFlag << " \" + " << getOptionVar << ";\n";
        }
    } else {
        // just set the checkbox to the default value
        m_load << "  optionMenu -e -sl (" << defaultIndex << " + 1) " << optionVar << ";\n";

        if (passAsString) {
            m_execute << "  if(`intField -ex " << optionVar << "`)\n";
            m_execute << "  {\n";
            m_execute << "    " << enumValuesAsString.str() << "\n";
            m_execute << "    int $__index = (`optionMenu -q -sl " << optionVar << "` - 1);\n";
            m_execute << "    $str += \" -" << commandFlag
                      << " \\\"\" + $evs[ $__index ] + \"\\\"\";\n";
            m_execute << "  }\n";
        } else if (enumValues) {
            m_execute << "  if(`intField -ex " << optionVar << "`)\n";
            m_execute << "  {\n";
            m_execute << "    " << enumValuesAsString.str() << "\n";
            m_execute << "    int $__index = (`optionMenu -q -sl " << optionVar << "` - 1);\n";
            m_execute << "    $str += \" -" << commandFlag << " \" + $ev[ $__index ];\n";
            m_execute << "  }\n";
        } else {
            m_execute << "  if(`intField -ex " << optionVar << "`)\n";
            m_execute << "    $str += \" -" << commandFlag << " \" + `optionMenu -q -sl "
                      << optionVar << "` - 1);\n";
        }
    }

    // reset checkbox back to the default value
    m_reset << "  optionMenu -e -sl (" << defaultIndex << " + 1) " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addRadioButtonGroupOption(
    const char*       commandFlag,
    const char*       label,
    int               defaultIndex,
    const char* const enumNames[],
    const int32_t     enumValues[],
    bool              persist,
    bool              passAsString)
{
    // do we have 4 or less options?
    int32_t optionCount = 0;
    {
        while (enumNames[optionCount++]) {
            // if for some reason this is being called with more than 4 options, just use a combo
            // box instead.
            if (optionCount > 4) {
                addEnumOption(
                    commandFlag, label, defaultIndex, enumNames, enumValues, persist, passAsString);
                return;
            }
        }
        --optionCount;
    }

    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    std::ostringstream enumValuesAsString;
    if (passAsString) {
        enumValuesAsString << "$evs = {\"" << enumNames[0] << "\"";
        for (int i = 1; enumNames[i]; ++i) {
            enumValuesAsString << ",\"" << enumNames[i] << "\"";
        }
        enumValuesAsString << "};";
    } else if (enumValues) {
        enumValuesAsString << "$ev = {" << enumValues[0];
        for (int i = 1; enumNames[i]; ++i) {
            enumValuesAsString << "," << enumValues[i];
        }
        enumValuesAsString << "};";
    }

    // generate checkbox control for boolean argument
    m_controls << "  radioButtonGrp -h 20 -nrb " << optionCount << " ";
    for (int i = 0; enumNames[i]; ++i) {
        m_controls << "-l" << (i + 1) << " \"" << enumNames[i] << "\" ";
    }
    m_controls << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -iv \"" << optionVar << "\" " << defaultIndex << ";\n";
        m_load << "  radioButtonGrp -e -sl (" << getOptionVar << " + 1) " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -iv \"" << optionVar << "\" (`radioButtonGrp -q -sl " << optionVar
               << "` - 1);\n";

        // build up command string using optionVar value
        if (passAsString) {
            m_execute << "  {\n";
            m_execute << "    " << enumValuesAsString.str() << "\n";
            m_execute << "    int $__index = " << getOptionVar << ";\n";
            m_execute << "    $str += \" -" << commandFlag
                      << " \\\"\" + $evs[ $__index ] + \"\\\"\";\n";
            m_execute << "  }\n";
        } else if (enumValues) {
            m_execute << "  {\n";
            m_execute << "    " << enumValuesAsString.str() << "\n";
            m_execute << "    int $__index = " << getOptionVar << ";\n";
            m_execute << "    $str += \" -" << commandFlag << " \" + $ev[ $__index ];\n";
            m_execute << "  }\n";
        } else {
            m_execute << "  $str += \" -" << commandFlag << " \" + " << getOptionVar << ";\n";
        }
    } else {
        // just set the checkbox to the default value
        m_load << "  radioButtonGrp -e -sl (" << defaultIndex << " + 1) " << optionVar << ";\n";

        if (passAsString) {
            m_execute << "  if(`radioButtonGrp -ex " << optionVar << "`)\n";
            m_execute << "  {\n";
            m_execute << "    " << enumValuesAsString.str() << "\n";
            m_execute << "    int $__index = (`radioButtonGrp -q -sl " << optionVar << "` - 1);\n";
            m_execute << "    $str += \" -" << commandFlag
                      << " \\\"\" + $evs[ $__index ] + \"\\\"\";\n";
            m_execute << "  }\n";
        } else if (enumValues) {
            m_execute << "  if(`radioButtonGrp -ex " << optionVar << "`)\n";
            m_execute << "  {\n";
            m_execute << "    " << enumValuesAsString.str() << "\n";
            m_execute << "    int $__index = (`radioButtonGrp -q -sl " << optionVar << "` - 1);\n";
            m_execute << "    $str += \" -" << commandFlag << " \" + $ev[ $__index ];\n";
            m_execute << "  }\n";
        } else {
            m_execute << "  if(`radioButtonGrp -ex " << optionVar << "`)\n";
            m_execute << "    $str += \" -" << commandFlag << " \" + `radioButtonGrp -q -sl "
                      << optionVar << "` - 1);\n";
        }
    }

    // reset checkbox back to the default value
    m_reset << "  radioButtonGrp -e -sl (" << defaultIndex << " + 1) " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addDoubleOption(
    const char*  commandFlag,
    const char*  label,
    const double defaultVal,
    bool         persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  floatField -h 20 " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -fv \"" << optionVar << "\" " << defaultVal << ";\n";
        m_load << "  floatField -e -v " << getOptionVar << " " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -fv \"" << optionVar << "\" `floatField -q -v " << optionVar
               << "`;\n";

        // build up command string using optionVar value
        m_execute << "  $str += \" -" << commandFlag << " \" + " << getOptionVar << ";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  floatField -e -v " << defaultVal << " " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`floatField -ex " << optionVar << "`)\n";
        m_execute << "    $str += \" -" << commandFlag << " \" + `floatField -q -v " << optionVar
                  << "`;\n";
    }

    // reset checkbox back to the default value
    m_reset << "  floatField -e -v " << defaultVal << " " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addDoubleSliderOption(
    const char*  commandFlag,
    const char*  label,
    const double minVal,
    const double maxVal,
    const double defaultVal,
    bool         persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  floatSlider -h 20 -min " << minVal << " -max " << maxVal << " " << optionVar
               << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -fv \"" << optionVar << "\" " << defaultVal << ";\n";
        m_load << "  floatSlider -e -v " << getOptionVar << " " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -fv \"" << optionVar << "\" `floatSlider -q -v " << optionVar
               << "`;\n";

        // build up command string using optionVar value
        m_execute << "  $str += \" -" << commandFlag << " \" + " << getOptionVar << ";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  floatSlider -e -v " << defaultVal << " " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`floatSlider -ex " << optionVar << "`)\n";
        m_execute << "    $str += \" -" << commandFlag << " \" + `floatSlider -q -v " << optionVar
                  << "`;\n";
    }

    // reset checkbox back to the default value
    m_reset << "  floatSlider -e -v " << defaultVal << " " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addVec2Option(
    const char*  commandFlag,
    const char*  label,
    const double defaultVal[2],
    bool         persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  floatFieldGrp -h 20 -nf 2 " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -fv \"" << optionVar << "\" " << defaultVal[0] << " -fva \""
               << optionVar << "\" " << defaultVal[1] << ";\n";

        m_load << "  $fv = " << getOptionVar << ";\n";
        m_load << "  floatFieldGrp -e -v1 $fv[0] -v2 $fv[1] " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -fv \"" << optionVar << "\" `floatFieldGrp -q -v1 " << optionVar
               << "` -fva \"" << optionVar << "\" `floatFieldGrp -q -v2 " << optionVar << "`;\n";

        // build up command string using optionVar value
        m_execute << "  $fv = " << getOptionVar << ";\n";
        m_execute << "  $str += \" -" << commandFlag << " \" + $fv[0] + \" \" + $fv[1] + \" \";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  floatFieldGrp -e -v1 " << defaultVal[0] << " -v2 " << defaultVal[1] << " "
               << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`floatFieldGrp -ex " << optionVar << "`)\n";
        m_execute << "  $str += \" -" << commandFlag << " \" + `floatFieldGrp -q -v1 " << optionVar
                  << "` + \" \" + `floatFieldGrp -q -v2 " << optionVar << "`;\n";
    }

    // reset checkbox back to the default value
    m_reset << "  floatFieldGrp -e -v1 " << defaultVal[0] << " -v2 " << defaultVal[1] << " "
            << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addVec3Option(
    const char*  commandFlag,
    const char*  label,
    const double defaultVal[3],
    bool         persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  floatFieldGrp -h 20 -nf 3 " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -fv \"" << optionVar << "\" " << defaultVal[0] << " -fva \""
               << optionVar << "\" " << defaultVal[1] << " -fva \"" << optionVar << "\" "
               << defaultVal[2] << ";\n";

        m_load << "  $fv = " << getOptionVar << ";\n";
        m_load << "  floatFieldGrp -e -v1 $fv[0] -v2 $fv[1] -v3 $fv[2] " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -fv \"" << optionVar << "\" `floatFieldGrp -q -v1 " << optionVar
               << "` -fva \"" << optionVar << "\" `floatFieldGrp -q -v2 " << optionVar
               << "` -fva \"" << optionVar << "\" `floatFieldGrp -q -v3 " << optionVar << "`;\n";

        // build up command string using optionVar value
        m_execute << "  $fv = " << getOptionVar << ";\n";
        m_execute << "  $str += \" -" << commandFlag
                  << " \" + $fv[0] + \" \" + $fv[1] + \" \" + $fv[2] + \" \";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  floatFieldGrp -e -v1 " << defaultVal[0] << " -v2 " << defaultVal[1] << " -v3 "
               << defaultVal[2] << " " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`floatFieldGrp -ex " << optionVar << "`)\n";
        m_execute << "  $str += \" -" << commandFlag << " \" + `floatFieldGrp -q -v1 " << optionVar
                  << "` + \" \" + `floatFieldGrp -q -v2 " << optionVar
                  << "` + \" \" + `floatFieldGrp -q -v3 " << optionVar << "`;\n";
    }

    // reset checkbox back to the default value
    m_reset << "  floatFieldGrp -e -v1 " << defaultVal[0] << " -v2 " << defaultVal[1] << " -v3 "
            << defaultVal[2] << " " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addVec4Option(
    const char*  commandFlag,
    const char*  label,
    const double defaultVal[4],
    bool         persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  floatFieldGrp -h 20 -nf 4 " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -fv \"" << optionVar << "\" " << defaultVal[0] << " -fva \""
               << optionVar << "\" " << defaultVal[1] << " -fva \"" << optionVar << "\" "
               << defaultVal[2] << " -fva \"" << optionVar << "\" " << defaultVal[3] << ";\n";

        m_load << "  $fv = " << getOptionVar << ";\n";
        m_load << "  floatFieldGrp -e -v1 $fv[0] -v2 $fv[1] -v3 $fv[2] -v4 $fv[3] " << optionVar
               << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -fv \"" << optionVar << "\" `floatFieldGrp -q -v1 " << optionVar
               << "` -fva \"" << optionVar << "\" `floatFieldGrp -q -v2 " << optionVar
               << "` -fva \"" << optionVar << "\" `floatFieldGrp -q -v3 " << optionVar
               << "` -fva \"" << optionVar << "\" `floatFieldGrp -q -v4 " << optionVar << "`;\n";

        // build up command string using optionVar value
        m_execute << "  $fv = " << getOptionVar << ";\n";
        m_execute << "  $str += \" -" << commandFlag
                  << " \" + $fv[0] + \" \" + $fv[1] + \" \" + $fv[2] + \" \" + $fv[3] + \" \";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  floatFieldGrp -e -v1 " << defaultVal[0] << " -v2 " << defaultVal[1] << " -v3 "
               << defaultVal[2] << " -v4 " << defaultVal[3] << " " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`floatFieldGrp -ex " << optionVar << "`)\n";
        m_execute << "  $str += \" -" << commandFlag << " \" + `floatFieldGrp -q -v1 " << optionVar
                  << "` + \" \" + `floatFieldGrp -q -v2 " << optionVar
                  << "` + \" \" + `floatFieldGrp -q -v3 " << optionVar
                  << "` + \" \" + `floatFieldGrp -q -v4 " << optionVar << "`;\n";
    }

    // reset checkbox back to the default value
    m_reset << "  floatFieldGrp -e -v1 " << defaultVal[0] << " -v2 " << defaultVal[1] << " -v3 "
            << defaultVal[2] << " -v4 " << defaultVal[3] << " " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addColourOption(
    const char*  commandFlag,
    const char*  label,
    const double defaultVal[3],
    bool         persist)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  colorSliderGrp -h 20 " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -fv \"" << optionVar << "\" " << defaultVal[0] << " -fva \""
               << optionVar << "\" " << defaultVal[1] << " -fva \"" << optionVar << "\" "
               << defaultVal[2] << ";\n";

        m_load << "  $fv = " << getOptionVar << ";\n";
        m_load << "  colorSliderGrp -e -rgb $fv[0] $fv[1] $fv[2] " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  $cv = `colorSliderGrp -q -rgb " << optionVar << "`;\n";
        m_save << "  optionVar -fv \"" << optionVar << "\" $cv[0] "
               << " -fva \"" << optionVar << "\" $cv[1] "
               << " -fva \"" << optionVar << "\" $cv[2];\n";

        // build up command string using optionVar value
        m_execute << "  $cv = " << getOptionVar << ";\n";
        m_execute << "  $str += \" -" << commandFlag
                  << " \" + $cv[0] + \" \" + $cv[1] + \" \" + $cv[2] + \" \";\n";
    } else {
        // just set the checkbox to the default value
        m_load << "  colorSliderGrp -e -rgb " << defaultVal[0] << " " << defaultVal[1] << " "
               << defaultVal[2] << " " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        m_execute << "  if(`colorSliderGrp -ex " << optionVar << "`) {\n";
        m_execute << "    $cv = `colorSliderGrp -q -rgb " << optionVar << "`;\n";
        m_execute << "    $str += \" -" << commandFlag << " \" + $cv[0] "
                  << "+ \" \" + $cv[1] "
                  << "+ \" \" + $cv[2];\n";
        m_execute << "  }\n";
    }

    // reset checkbox back to the default value
    m_reset << "  colorSliderGrp -e -rgb " << defaultVal[0] << " " << defaultVal[1] << " "
            << defaultVal[2] << " " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addStringOption(
    const char*  commandFlag,
    const char*  label,
    MString      defaultVal,
    bool         persist,
    StringPolicy policy)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;
    const std::string getOptionVar = std::string("`optionVar -q \"") + optionVar + "\"`";

    // generate checkbox control for boolean argument
    m_controls << "  textField -h 20 " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // if the default value does not exist, force it to exist using the default value. Then set the
    // control value
    if (persist) {
        m_init << "  if(!`optionVar -ex \"" << optionVar << "\"`)\n";
        m_init << "    optionVar -sv \"" << optionVar << "\" " << defaultVal << ";\n";
        m_load << "  textField -e -tx " << getOptionVar << " " << optionVar << ";\n";

        // pull value from checkbox, and insert into the optionVar
        m_save << "  optionVar -sv \"" << optionVar << "\" `textField -q -tx " << optionVar
               << "`;\n";

        // build up command string using optionVar value
        if (policy == kStringOptional) {
            m_execute << "  if(size(" << getOptionVar << "))\n";
            m_execute << "    $str += \" -" << commandFlag << " \\\"\" + " << getOptionVar
                      << " + \"\\\"\";\n";
        } else if (policy == kStringMustHaveValue) {
            m_execute << "  if(!size(" << getOptionVar << ")) {\n";
            m_execute << "    error \"" << label << " must be specified\";\n    return;\n  }\n";
            m_execute << "  $str += \" -" << commandFlag << " \\\"\" + " << getOptionVar
                      << " + \"\\\"\";\n";
        } else {
            std::cerr << "unknown string policy" << std::endl;
        }
    } else {
        // just set the checkbox to the default value
        m_load << "  textField -e -tx \"" << defaultVal << "\" " << optionVar << ";\n";

        // pull the value from the checkbox when executing
        if (policy == kStringOptional) {
            m_execute << "  if(`textField -ex " << optionVar << "`)\n";
            m_execute << "    if(size(`textField -q -tx " << optionVar << "`))\n";
            m_execute << "       $str += \" -" << commandFlag << " \\\"\" + `textField -q -tx "
                      << optionVar << "` + \"\\\"\";\n";
        } else if (policy == kStringMustHaveValue) {
            m_execute << "  if(`textField -ex " << optionVar << "`)\n";
            m_execute << "    if(!size(`textField -q -tx " << optionVar << "`)) {\n";
            m_execute << "      error \"" << label
                      << " must be specified\";\n      return;\n    }\n    else\n";
            m_execute << "      $str += \" -" << commandFlag << " \\\"\" + `textField -q -tx "
                      << optionVar << "` + \"\\\"\";\n";
        } else {
            std::cerr << "unknown string policy" << std::endl;
        }
    }

    // reset checkbox back to the default value
    m_reset << "  textField -e -tx \"" << defaultVal << "\" " << optionVar << ";\n";
}

//----------------------------------------------------------------------------------------------------------------------
void CommandGuiHelper::addFilePathOption(
    const char*  commandFlag,
    const char*  label,
    FileMode     fileMode,
    const char*  filter,
    StringPolicy policy)
{
    const std::string optionVar = m_commandName + "_" + commandFlag;

    // generate checkbox control for boolean argument
    m_controls << "  textFieldButtonGrp -h 20 -bl \"...\" -bc \"alFileDialogHandler(\\\"" << filter
               << "\\\", \\\"" << optionVar << "\\\", " << fileMode << ")\" " << optionVar << ";\n";

    // add label
    m_labels << "  text -al \"right\" -h 20 -w 160 -l \"" << label << ":\";\n";

    // pull the value from the checkbox when executing
    if (policy == kStringOptional) {
        m_execute << "  if(`textFieldButtonGrp -ex " << optionVar << "`)\n";
        m_execute << "    if(size(`textFieldButtonGrp -q -fi " << optionVar << "`))\n";
        m_execute << "       $str += \" -" << commandFlag << " \\\"\" + `textFieldButtonGrp -q -fi "
                  << optionVar << "` + \"\\\"\";\n";
    } else if (policy == kStringMustHaveValue) {
        m_execute << "  if(`textFieldButtonGrp -ex " << optionVar << "`)\n";
        m_execute << "    if(!size(`textFieldButtonGrp -q -fi " << optionVar << "`)) {\n";
        m_execute << "      error \"" << label << " must be specified\";\n    return;\n  }\n";
        m_execute << "  $str += \" -" << commandFlag << " \\\"\" + `textFieldButtonGrp -q -fi "
                  << optionVar << "` + \"\\\"\";\n";
    } else {
        std::cerr << "unknown string policy" << std::endl;
    }

    // reset checkbox back to the default value
    m_reset << "  textFieldButtonGrp -e -fi \"\" " << optionVar << ";\n";

    m_hasFilePath = true;
}
//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
