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
#include "AL/maya/utils/FileTranslatorOptions.h"

#include <maya/MGlobal.h>

#include <algorithm>

namespace AL {
namespace maya {
namespace utils {
//----------------------------------------------------------------------------------------------------------------------
const MString OptionsParser::kNullString;

//----------------------------------------------------------------------------------------------------------------------
OptionsParser::OptionsParser(PluginTranslatorOptionsInstance* const pluginOptions)
    : m_optionNameToValue()
    , m_niceNameToValue()
    , m_pluginOptions(pluginOptions)
{
}

//----------------------------------------------------------------------------------------------------------------------
OptionsParser::~OptionsParser()
{
    auto it = m_optionNameToValue.begin();
    auto end = m_optionNameToValue.end();
    for (; it != end; ++it)
        delete it->second;
    m_optionNameToValue.clear();
    m_niceNameToValue.clear();
}

//----------------------------------------------------------------------------------------------------------------------
void OptionsParser::construct(MString& optionString)
{
    optionString.clear();

    for (auto& r : m_optionNameToValue) {
        optionString += r.first.c_str();
        optionString += "=";
        switch (r.second->m_type) {
        case kBool: optionString += r.second->m_bool; break;
        case kEnum:
        case kInt: optionString += r.second->m_int; break;
        case kFloat: optionString += r.second->m_float; break;
        case kString: optionString += r.second->m_string.c_str(); break;
        }
        optionString += ";";
    }
}

//----------------------------------------------------------------------------------------------------------------------
MStatus OptionsParser::parse(const MString& optionString)
{
    MStatus status = MS::kSuccess;
    {
        auto it = m_niceNameToValue.begin();
        auto end = m_niceNameToValue.end();
        for (; it != end; ++it) {
            it->second->init();
        }
    }

    if (optionString.length() > 0) {
        int i, length;
        // Start parsing.
        MStringArray optionList;
        MStringArray theOption;
        optionString.split(';', optionList); // break out all the options.

        length = optionList.length();
        for (i = 0; i < length; ++i) {
            theOption.clear();
            optionList[i].split('=', theOption);

            auto it = m_optionNameToValue.find(theOption[0].asChar());
            if (it != m_optionNameToValue.end()) {
                it->second->parse(theOption[1]);
            } else if (m_pluginOptions) {
                m_pluginOptions->parse(theOption[0], theOption[1]);
            } else {
                MGlobal::displayError(
                    MString("Unknown option: ") + theOption[0] + " { " + theOption[1] + " }");
                status = MS::kFailure;
            }
        }
    }
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
FileTranslatorOptions::FileTranslatorOptions(const char* fileTranslatorName)
    : m_frames()
    , m_visibility()
    , m_translatorName(fileTranslatorName)
    , m_code()
{
}

//----------------------------------------------------------------------------------------------------------------------
bool FileTranslatorOptions::addFrame(const char* frameName)
{
    m_frames.push_back(FrameLayout(frameName));
    /// \todo need error checking here
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool FileTranslatorOptions::removeFrame(const char* frameName)
{
    auto iter = std::find_if(m_frames.begin(), m_frames.end(), [&](FrameLayout& frame) {
        return frame.m_frameName == frameName;
    });
    if (iter != m_frames.end()) {
        m_frames.erase(iter);
        return true;
    }
    MGlobal::displayError(
        (std::string("FileTranslatorOptions: failed to remove frame: ") + frameName).c_str());
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
MString niceNameToOptionString(MString n)
{
    char* str = (char*)n.asChar();
    for (uint32_t i = 0, len = n.length(); i < len; ++i) {
        const char c = str[i];
        if (!isalnum(c) && c != '_') {
            str[i] = '_';
        }
    }
    return n;
}

//----------------------------------------------------------------------------------------------------------------------
bool FileTranslatorOptions::boolControlsVisibility(
    const char* controller,
    const char* controlled,
    bool        invertBehaviour)
{
    MString opt_controller = niceNameToOptionString(controller);
    MString opt_controlled = niceNameToOptionString(controlled);
    if (hasOption(opt_controller) && hasOption(opt_controlled)) {
        MString controllerControl = m_translatorName + "_" + opt_controller;
        MString controlledControl = m_translatorName + "_" + opt_controlled;
        m_visibility.emplace_back(controllerControl, controlledControl);
        return true;
    }
    MGlobal::displayError("FileTranslatorOptions: unknown option name");
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool FileTranslatorOptions::addBool(const char* niceName, bool defaultValue)
{
    if (m_frames.empty())
        return false;

    FrameLayout&        frame = lastFrame();
    FrameLayout::Option option;

    option.optionName = niceNameToOptionString(niceName);
    option.niceName = niceName;
    option.type = kBool;
    option.defaultBool = defaultValue;
    option.enumValues = 0;
    if (hasOption(option.optionName)) {
        MString msg("FileTranslatorOptions: cannot register the same bool option twice: ^1");
        msg.format(option.optionName);
        MGlobal::displayError(msg);
        return false;
    }

    frame.m_options.push_back(option);
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool FileTranslatorOptions::addInt(const char* niceName, int defaultValue)
{
    if (m_frames.empty())
        return false;

    FrameLayout&        frame = lastFrame();
    FrameLayout::Option option;

    option.optionName = niceNameToOptionString(niceName);
    option.niceName = niceName;
    option.type = kInt;
    option.defaultInt = defaultValue;
    option.enumValues = 0;
    if (hasOption(option.optionName)) {
        MString msg("FileTranslatorOptions: cannot register the same int option twice: ^1");
        msg.format(option.optionName);
        MGlobal::displayError(msg);
        return false;
    }
    frame.m_options.push_back(option);
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool FileTranslatorOptions::addFloat(const char* niceName, float defaultValue)
{
    if (m_frames.empty())
        return false;

    FrameLayout&        frame = lastFrame();
    FrameLayout::Option option;

    option.optionName = niceNameToOptionString(niceName);
    option.niceName = niceName;
    option.type = kFloat;
    option.defaultFloat = defaultValue;
    option.enumValues = 0;
    if (hasOption(option.optionName)) {
        MString msg("FileTranslatorOptions: cannot register the same float option twice: ^1");
        msg.format(option.optionName);
        MGlobal::displayError(msg);
        return false;
    }
    frame.m_options.push_back(option);
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool FileTranslatorOptions::addString(const char* niceName, const char* const defaultValue)
{
    if (m_frames.empty())
        return false;

    FrameLayout&        frame = lastFrame();
    FrameLayout::Option option;

    option.optionName = niceNameToOptionString(niceName);
    option.niceName = niceName;
    option.type = kString;
    option.defaultString = defaultValue;
    option.enumValues = 0;
    if (hasOption(option.optionName)) {
        MString msg("FileTranslatorOptions: cannot register the same string option twice: ^1");
        msg.format(option.optionName);
        MGlobal::displayError(msg);
        return false;
    }
    frame.m_options.push_back(option);
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool FileTranslatorOptions::addEnum(
    const char*       niceName,
    const char* const enumValues[],
    const int         defaultValue)
{
    if (m_frames.empty())
        return false;

    FrameLayout&        frame = lastFrame();
    FrameLayout::Option option;

    option.optionName = niceNameToOptionString(niceName);
    option.niceName = niceName;
    option.type = kEnum;
    option.defaultInt = defaultValue;
    option.enumValues = enumValues;

    if (hasOption(option.optionName)) {
        MString msg("FileTranslatorOptions: cannot register the same enum option twice: ^1");
        msg.format(option.optionName);
        MGlobal::displayError(msg);
        return false;
    }
    frame.m_options.push_back(option);
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
void FileTranslatorOptions::generateBoolGlobals(
    const MString& niceName,
    const MString& optionName,
    bool           defaultValue)
{
    MString controlName = m_translatorName + "_" + optionName;
    MString createCommand = MString("global proc create_") + controlName + "() {"
        + MString("checkBox -l \"") + niceName + "\" -v " + defaultValue + " " + controlName
        + ";}\n";
    MString postCommand = MString("global proc post_") + controlName
        + "(string $value){ eval (\"checkBox -e -v \" + $value + \" " + controlName + "\");}\n";
    MString buildCommand = MString("global proc string build_") + controlName
        + "(){ string $str = \"" + optionName + "=\"; if(` checkBox -q -v " + controlName
        + "`) $str = $str + \"1;\"; else $str = $str + \"0;\"; return $str;}\n";

    m_code += createCommand;
    m_code += postCommand;
    m_code += buildCommand;
    m_code += "\n";
}

//----------------------------------------------------------------------------------------------------------------------
void FileTranslatorOptions::generateIntGlobals(
    const MString& niceName,
    const MString& optionName,
    int            defaultValue)
{
    MString controlName = m_translatorName + "_" + optionName;
    MString createCommand = MString("global proc create_") + controlName + "() {"
        + MString("intFieldGrp -l \"") + niceName + "\" -v1 " + defaultValue + " " + controlName
        + ";}\n";
    MString postCommand = MString("global proc post_") + controlName
        + "(string $value){ eval (\"intFieldGrp -e -v1 \" + $value + \" " + controlName + "\");}\n";
    MString buildCommand = MString("global proc string build_") + controlName
        + "(){ string $str = \"" + optionName + "=\" + `intFieldGrp -q -v1 " + controlName
        + "` + \";\"; return $str;}\n";

    m_code += createCommand;
    m_code += postCommand;
    m_code += buildCommand;
    m_code += "\n";
}

//----------------------------------------------------------------------------------------------------------------------
void FileTranslatorOptions::generateFloatGlobals(
    const MString& niceName,
    const MString& optionName,
    float          defaultValue)
{
    MString controlName = m_translatorName + "_" + optionName;
    MString createCommand = MString("global proc create_") + controlName + "() {"
        + MString("floatFieldGrp -l \"") + niceName + "\" -v1 " + defaultValue + " " + controlName
        + ";}\n";
    MString postCommand = MString("global proc post_") + controlName
        + "(string $value){ eval (\"floatFieldGrp -e -v1 \" + $value + \" " + controlName
        + "\");}\n";
    MString buildCommand = MString("global proc string build_") + controlName
        + "(){ string $str = \"" + optionName + "=\" + `floatFieldGrp -q -v1 " + controlName
        + "` + \";\"; return $str;}\n";

    m_code += createCommand;
    m_code += postCommand;
    m_code += buildCommand;
    m_code += "\n";
}

//----------------------------------------------------------------------------------------------------------------------
void FileTranslatorOptions::generateStringGlobals(
    const MString& niceName,
    const MString& optionName,
    MString        defaultValue)
{
    MString controlName = m_translatorName + "_" + optionName;
    MString createCommand = MString("global proc create_") + controlName + "() {"
        + MString("textFieldGrp -l \"") + niceName + "\" " + controlName + ";}\n";
    MString postCommand = MString("global proc post_") + controlName
        + "(string $value){ eval (\"textFieldGrp -e -tx \" + $value + \" " + controlName
        + "\");}\n";
    MString buildCommand = MString("global proc string build_") + controlName
        + "(){ string $str = \"" + optionName + "=\" + `textFieldGrp -q -tx " + controlName
        + "` + \";\"; return $str;}\n";

    m_code += createCommand;
    m_code += postCommand;
    m_code += buildCommand;
    m_code += "\n";
}

//----------------------------------------------------------------------------------------------------------------------
void FileTranslatorOptions::generateEnumGlobals(
    const MString&    niceName,
    const MString&    optionName,
    const char* const enumValues[],
    int               defaultValue)
{
    MString controlName = m_translatorName + "_" + optionName;
    MString createCommand = MString("global proc create_") + controlName + "() {"
        + MString("optionMenuGrp -l \"") + niceName + "\" " + controlName + ";";

    for (int i = 0; enumValues && enumValues[i]; ++i) {
        createCommand += MString("menuItem -l \"") + enumValues[i] + "\";";
    }

    createCommand += "}\n";
    MString postCommand = MString("global proc post_") + controlName
        + "(string $value){ int $v=$value; eval (\"optionMenuGrp -e -sl \" + ($v + 1) + \" "
        + controlName + "\");}\n";
    MString buildCommand = MString("global proc string build_") + controlName
        + "(){ string $str = \"" + optionName + "=\" + (`optionMenuGrp -q -sl " + controlName
        + "` -1) + \";\"; return $str;}\n";

    m_code += createCommand;
    m_code += postCommand;
    m_code += buildCommand;
    m_code += "\n";
}

//----------------------------------------------------------------------------------------------------------------------
bool FileTranslatorOptions::hasOption(const char* const optionName) const
{
    for (auto frame : m_frames) {
        for (auto option : frame.m_options) {
            if (option.optionName == optionName)
                return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
std::string stringify(const char* str)
{
    std::string new_str;
    while (*str) {
        switch (*str) {
        case '\'': new_str += "\\\'"; break;
        case '\"': new_str += "\\\""; break;
        case '\\': new_str += "\\\\"; break;
        case '\n': new_str += "\\n"; break;
        case '\t': new_str += "\\t"; break;
        case '\r': new_str += "\\r"; break;
        case '\a': new_str += "\\a"; break;
        default: new_str += *str; break;
        }
        ++str;
    }
    return new_str;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus FileTranslatorOptions::initParser(OptionsParser& optionParser)
{
    // first generate a collection of methods to create, edit, and query each separate option. For
    // each exporter/importer option, we will generate three methods:
    //
    //   proc create_myOptionName();              ///< creates the GUI control for the option
    //   proc post_myOptionName(string $value);   ///< set the value in the control from the parsed
    //   option string proc string build_myOptionName();        ///< get the value from the control,
    //   and return it as a text string "myOptionName=<value>"
    //
    // We will also add in some entries into the optionParser, which will be used later on when
    // using the exporter. This option parser will know about the option names (both the 'nice'
    // names and the actual option name), as well as the associated data type. This will be able to
    // split apart the option string of the form "option1=10;option2=hello;option3=true"
    //
    auto itf = m_frames.begin();
    auto endf = m_frames.end();
    for (; itf != endf; ++itf) {
        auto ito = itf->m_options.begin();
        auto endo = itf->m_options.end();
        for (; ito != endo; ++ito) {
            switch (ito->type) {
            case kBool: {
                OptionsParser::OptionValue* value = new OptionsParser::OptionValue;
                value->m_type = OptionsParser::kBool;
                value->m_defaultBool = ito->defaultBool;
                optionParser.m_niceNameToValue.insert(
                    std::make_pair(ito->niceName.asChar(), value));
                optionParser.m_optionNameToValue.insert(
                    std::make_pair(ito->optionName.asChar(), value));
            } break;

            case kInt: {
                OptionsParser::OptionValue* value = new OptionsParser::OptionValue;
                value->m_type = OptionsParser::kInt;
                value->m_defaultInt = ito->defaultInt;
                optionParser.m_niceNameToValue.insert(
                    std::make_pair(ito->niceName.asChar(), value));
                optionParser.m_optionNameToValue.insert(
                    std::make_pair(ito->optionName.asChar(), value));
            } break;

            case kFloat: {
                OptionsParser::OptionValue* value = new OptionsParser::OptionValue;
                value->m_type = OptionsParser::kFloat;
                value->m_defaultFloat = ito->defaultFloat;
                optionParser.m_niceNameToValue.insert(
                    std::make_pair(ito->niceName.asChar(), value));
                optionParser.m_optionNameToValue.insert(
                    std::make_pair(ito->optionName.asChar(), value));
            } break;

            case kString: {
                OptionsParser::OptionValue* value = new OptionsParser::OptionValue;
                value->m_type = OptionsParser::kString;
                value->m_defaultString = ito->defaultString;
                optionParser.m_niceNameToValue.insert(
                    std::make_pair(ito->niceName.asChar(), value));
                optionParser.m_optionNameToValue.insert(
                    std::make_pair(ito->optionName.asChar(), value));
            } break;

            case kEnum: {
                OptionsParser::OptionValue* value = new OptionsParser::OptionValue;
                value->m_type = OptionsParser::kEnum;
                value->m_defaultInt = ito->defaultInt;
                optionParser.m_niceNameToValue.insert(
                    std::make_pair(ito->niceName.asChar(), value));
                optionParser.m_optionNameToValue.insert(
                    std::make_pair(ito->optionName.asChar(), value));
            } break;
            }
        }
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus
FileTranslatorOptions::generateScript(OptionsParser& optionParser, MString& defaultOptionString)
{
    initParser(optionParser);

    // first generate a collection of methods to create, edit, and query each separate option. For
    // each exporter/importer option, we will generate three methods:
    //
    //   proc create_myOptionName();              ///< creates the GUI control for the option
    //   proc post_myOptionName(string $value);   ///< set the value in the control from the parsed
    //   option string proc string build_myOptionName();        ///< get the value from the control,
    //   and return it as a text string "myOptionName=<value>"
    //
    // We will also add in some entries into the optionParser, which will be used later on when
    // using the exporter. This option parser will know about the option names (both the 'nice'
    // names and the actual option name), as well as the associated data type. This will be able to
    // split apart the option string of the form "option1=10;option2=hello;option3=true"
    //
    auto itf = m_frames.begin();
    auto endf = m_frames.end();
    for (; itf != endf; ++itf) {
        auto ito = itf->m_options.begin();
        auto endo = itf->m_options.end();
        for (; ito != endo; ++ito) {
            switch (ito->type) {
            case kBool: {
                generateBoolGlobals(ito->niceName, ito->optionName, ito->defaultBool);
                defaultOptionString += ito->optionName + "=";
                if (ito->defaultBool)
                    defaultOptionString += "1;";
                else
                    defaultOptionString += "0;";
            } break;

            case kInt: {
                generateIntGlobals(ito->niceName, ito->optionName, ito->defaultInt);
                defaultOptionString += ito->optionName + "=" + (MString() + ito->defaultInt) + ";";
            } break;

            case kFloat: {
                generateFloatGlobals(ito->niceName, ito->optionName, ito->defaultFloat);
                defaultOptionString
                    += ito->optionName + "=" + (MString() + ito->defaultFloat) + ";";
            } break;

            case kString: {
                generateStringGlobals(ito->niceName, ito->optionName, ito->defaultString);
                defaultOptionString += ito->optionName + "=" + ito->defaultString + ";";
            } break;

            case kEnum: {
                generateEnumGlobals(
                    ito->niceName, ito->optionName, ito->enumValues, ito->defaultInt);
                defaultOptionString += ito->optionName + "=" + (MString() + ito->defaultInt) + ";";
            } break;
            }
        }
    }

    m_code += "global proc fromOptionVars_";
    m_code += m_translatorName;
    m_code += "() {}\n";

    m_code += "global proc create_";
    m_code += m_translatorName;
    m_code += "(string $parent) {}\n";

    m_code += "global proc post_";
    m_code += m_translatorName;
    m_code += "(string $name, string $value) {}\n";

    m_code += "global proc string query_";
    m_code += m_translatorName;
    m_code += "() { return \"\"; }\n";

    // generate the actual entry point for our option dialog, e.g.
    //
    //   global proc int myExporterName(string $parent, string $action, string $initialSettings,
    //   string $resultCallback)
    //
    m_code += MString("global proc int ") + m_translatorName
        + "(string $parent, string $action, string $initialSettings, string $resultCallback)\n{\n";
    m_code += "  int $result = 1;\n"
              "  string $currentOptions;\n"
              "  string $optionList[];\n"
              "  string $optionBreakDown[];\n"
              "  int $index;\n"
              "  if ($action == \"post\")\n  {\n" //< start of the 'post' section of the script (set
                                                  //control values from option string)
              "    setParent $parent;\n"
              "    columnLayout -adj true;\n"
              "    AL_usdmaya_SyncFileIOGui \""
        + m_translatorName + "\";\n";

    itf = m_frames.begin();
    for (; itf != endf; ++itf) {
        auto ito = itf->m_options.begin();
        auto endo = itf->m_options.end();
        for (; ito != endo; ++ito) {
            MString controlName = m_translatorName + "_" + ito->optionName;
            m_code += MString("    create_") + controlName + "();\n";
        }
    }

    m_code += "    create_";
    m_code += m_translatorName;
    m_code += "($parent);\n";

    // generate the code to split apart the key-value pairs of options.
    m_code += "    if (size($initialSettings) > 0) {\n"
              "      tokenize($initialSettings, \";\", $optionList);\n"
              "      for ($index = 0; $index < size($optionList); $index++) {\n"
              "        tokenize($optionList[$index], \"=\", $optionBreakDown);\n"
              "        if(size($optionBreakDown) < 2) continue;\n";

    itf = m_frames.begin();
    for (; itf != endf; ++itf) {
        auto ito = itf->m_options.begin();
        auto endo = itf->m_options.end();
        for (; ito != endo; ++ito) {
            MString controlName = m_translatorName + "_" + ito->optionName;
            m_code += MString("        if ($optionBreakDown[0] == \"") + ito->optionName + "\")\n";
            m_code += MString("          post_") + controlName + "($optionBreakDown[1]);   else\n";
        }
    }

    m_code += "        {\n          post_";
    m_code += m_translatorName
        + "($optionBreakDown[0], $optionBreakDown[1]);\n        }\n"
          "      }\n    }\n"
          "  }\n  else\n  if ($action == \"query\")\n  {\n"; //< start of 'query' section - return
                                                             //all control values as key-value pairs
                                                             //in an option string.

    itf = m_frames.begin();
    for (; itf != endf; ++itf) {
        auto ito = itf->m_options.begin();
        auto endo = itf->m_options.end();
        for (; ito != endo; ++ito) {
            MString controlName = m_translatorName + "_" + ito->optionName;
            m_code += MString("    $currentOptions = $currentOptions + `build_") + controlName
                + "`;\n";
        }
    }

    m_code
        += MString("    $currentOptions = $currentOptions + `query_") + m_translatorName + "`;\n";
    m_code += "    eval($resultCallback+\" \\\"\"+$currentOptions+\"\\\"\");\n"
              "  }\n  else\n  {\n"
              "    $result = 0;\n  }\n"
              "  return $result;\n}\n";

#if 0
  std::cout << "--------------------------------------------------------\n";
  std::cout << "--------------------------------------------------------\n";
  std::cout << "--------------------------------------------------------\n";
  std::cout << m_code.asChar() << std::endl;
  std::cout << "--------------------------------------------------------\n";
  std::cout << "--------------------------------------------------------\n";
  std::cout << "--------------------------------------------------------\n";
#endif

    return MGlobal::executeCommand(m_code, false, false);
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
