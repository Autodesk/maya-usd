//
// Copyright 2019 Animal Logic
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
#include "AL/maya/utils/PluginTranslatorOptions.h"

#include <maya/MGlobal.h>
#include <maya/MStringArray.h>

namespace AL {
namespace maya {
namespace utils {

std::map<std::string, PluginTranslatorOptionsContext*>
    PluginTranslatorOptionsContextManager::m_contexts;

//----------------------------------------------------------------------------------------------------------------------
extern MString niceNameToOptionString(MString n);

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
void PluginTranslatorOptionsContext::registerPluginTranslatorOptions(
    PluginTranslatorOptions* options)
{
    m_optionGroups.push_back(options);
    m_dirty = true;
}

//----------------------------------------------------------------------------------------------------------------------
PluginTranslatorOptionsArray::iterator
find(PluginTranslatorOptionsArray& array, const char* const groupName)
{
    for (auto it = array.begin(); it != array.end(); ++it) {
        if ((*it)->grouping() == groupName)
            return it;
    }
    return array.end();
}

//----------------------------------------------------------------------------------------------------------------------
PluginTranslatorOptionsArray::const_iterator
find(const PluginTranslatorOptionsArray& array, const char* const groupName)
{
    for (auto it = array.begin(); it != array.end(); ++it) {
        if ((*it)->grouping() == groupName)
            return it;
    }
    return array.end();
}

//----------------------------------------------------------------------------------------------------------------------
std::vector<PluginTranslatorOptions::Option>::iterator
find(std::vector<PluginTranslatorOptions::Option>& array, const char* const groupName)
{
    for (auto it = array.begin(); it != array.end(); ++it) {
        if (it->name == groupName)
            return it;
    }
    return array.end();
}

//----------------------------------------------------------------------------------------------------------------------
std::vector<PluginTranslatorOptions::Option>::const_iterator
find(const std::vector<PluginTranslatorOptions::Option>& array, const char* const groupName)
{
    for (auto it = array.begin(); it != array.end(); ++it) {
        if (it->name == groupName)
            return it;
    }
    return array.end();
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
void PluginTranslatorOptionsContext::unregisterPluginTranslatorOptions(
    const char* const pluginTranslatorGrouping)
{
    auto group = find(m_optionGroups, pluginTranslatorGrouping);
    if (group != m_optionGroups.end()) {
        m_optionGroups.erase(group);
    }
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptionsContext::isRegistered(const char* const pluginTranslatorGrouping) const
{
    auto group = find(m_optionGroups, pluginTranslatorGrouping);
    return group != m_optionGroups.end();
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
PluginTranslatorOptions::PluginTranslatorOptions(
    PluginTranslatorOptionsContext& context,
    const char* const               pluginTranslatorGrouping)
    : m_grouping(pluginTranslatorGrouping)
    , m_options()
    , m_context(context)
{
    context.registerPluginTranslatorOptions(this);
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
PluginTranslatorOptions::~PluginTranslatorOptions()
{
    m_context.registerPluginTranslatorOptions(this);
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptions::addBool(const char* optionName, bool defaultValue)
{
    if (isOption(optionName))
        return false;
    m_options.emplace_back(optionName, defaultValue);
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptions::addInt(const char* optionName, int defaultValue)
{
    if (isOption(optionName))
        return false;
    m_options.emplace_back(optionName, defaultValue);
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptions::addFloat(const char* optionName, float defaultValue)
{
    return addFloat(optionName, defaultValue, 1, "", true);
}

//----------------------------------------------------------------------------------------------------------------------
bool PluginTranslatorOptions::addFloat(const char* optionName, float value, int precision)
{
    return addFloat(optionName, value, precision, "", true);
}

bool PluginTranslatorOptions::addFloat(
    const char* optionName,
    float       value,
    int         precision,
    const char* controller,
    bool        state)
{
    if (isOption(optionName))
        return false;
    m_options.emplace_back(optionName, value, precision, controller, state);
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptions::addString(const char* optionName, const char* const defaultValue)
{
    if (isOption(optionName))
        return false;
    m_options.emplace_back(optionName, defaultValue);
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptions::addEnum(
    const char*       optionName,
    const char* const enumValues[],
    int               defaultValue)
{
    if (isOption(optionName))
        return false;
    m_options.push_back(Option(optionName, defaultValue, enumValues));
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptions::isOption(const char* const optionName) const
{
    auto option = find(m_options, optionName);
    return option != m_options.end();
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
OptionType PluginTranslatorOptions::optionType(const char* const optionName) const
{
    auto option = find(m_options, optionName);
    if (option != m_options.end()) {
        return option->type;
    }
    return OptionType::kBool;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptions::defaultBool(const char* const optionName) const
{
    auto option = find(m_options, optionName);
    if (option != m_options.end()) {
        return option->defBool;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
int PluginTranslatorOptions::defaultInt(const char* const optionName) const
{
    auto option = find(m_options, optionName);
    if (option != m_options.end()) {
        return option->defInt;
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
float PluginTranslatorOptions::defaultFloat(const char* const optionName) const
{
    auto option = find(m_options, optionName);
    if (option != m_options.end()) {
        return option->defFloat;
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
MString PluginTranslatorOptions::defaultString(const char* const optionName) const
{
    auto option = find(m_options, optionName);
    if (option != m_options.end()) {
        return option->defString;
    }
    return MString();
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
PluginTranslatorOptionsInstance::PluginTranslatorOptionsInstance(
    PluginTranslatorOptionsContext& context)
{
    m_optionSets.reserve(context.numOptionGroups());
    for (size_t i = 0, n = context.numOptionGroups(); i < n; ++i) {
        m_optionSets.emplace_back(context.optionGroup(i));
    }
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
void PluginTranslatorOptionsInstance::parse(MString key, const MString& value)
{
    key.substitute("_", " ");
    for (auto& set : m_optionSets) {
        const PluginTranslatorOptions* const def = set.m_def;
        for (size_t i = 0, n = def->numOptions(); i < n; ++i) {
            auto opt = def->option(i);
            if (opt->name == key) {
                auto& optData = set.m_options[i];
                switch (opt->type) {
                case OptionType::kBool: optData.m_bool = (value.asInt() > 0); break;
                case OptionType::kInt: optData.m_int = value.asInt(); break;
                case OptionType::kEnum: optData.m_int = value.asInt(); break;
                case OptionType::kFloat: optData.m_float = value.asFloat(); break;
                case OptionType::kString: optData.m_string = value; break;
                default: break;
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptionsInstance::setBool(const char* optionName, bool value)
{
    for (auto& set : m_optionSets) {
        for (size_t i = 0, n = set.m_options.size(); i < n; ++i) {
            auto it = set.m_def->option(i);
            if (it->name == optionName) {
                set.m_options[i].m_bool = value;
                return true;
            }
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptionsInstance::setInt(const char* optionName, int value)
{
    for (auto& set : m_optionSets) {
        for (size_t i = 0, n = set.m_options.size(); i < n; ++i) {
            auto it = set.m_def->option(i);
            if (it->name == optionName) {
                set.m_options[i].m_int = value;
                return true;
            }
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptionsInstance::setFloat(const char* optionName, float value)
{
    for (auto& set : m_optionSets) {
        for (size_t i = 0, n = set.m_options.size(); i < n; ++i) {
            auto it = set.m_def->option(i);
            if (it->name == optionName) {
                set.m_options[i].m_float = value;
                return true;
            }
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptionsInstance::setString(const char* optionName, const char* const value)
{
    for (auto& set : m_optionSets) {
        for (size_t i = 0, n = set.m_options.size(); i < n; ++i) {
            auto it = set.m_def->option(i);
            if (it->name == optionName) {
                set.m_options[i].m_string = value;
                return true;
            }
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptionsInstance::setEnum(const char* optionName, int32_t value)
{
    for (auto& set : m_optionSets) {
        for (size_t i = 0, n = set.m_options.size(); i < n; ++i) {
            auto it = set.m_def->option(i);
            if (it->name == optionName) {
                set.m_options[i].m_int = value;
                return true;
            }
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
bool PluginTranslatorOptionsInstance::getBool(const char* optionName)
{
    for (auto& set : m_optionSets) {
        for (size_t i = 0, n = set.m_options.size(); i < n; ++i) {
            auto it = set.m_def->option(i);
            if (it->name == optionName) {
                return set.m_options[i].m_bool;
            }
        }
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
int PluginTranslatorOptionsInstance::getInt(const char* optionName)
{
    for (auto& set : m_optionSets) {
        for (size_t i = 0, n = set.m_options.size(); i < n; ++i) {
            auto it = set.m_def->option(i);
            if (it->name == optionName) {
                return set.m_options[i].m_int;
            }
        }
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
float PluginTranslatorOptionsInstance::getFloat(const char* optionName)
{
    for (auto& set : m_optionSets) {
        for (size_t i = 0, n = set.m_options.size(); i < n; ++i) {
            auto it = set.m_def->option(i);
            if (it->name == optionName) {
                return set.m_options[i].m_float;
            }
        }
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
MString PluginTranslatorOptionsInstance::getString(const char* optionName)
{
    for (auto& set : m_optionSets) {
        for (size_t i = 0, n = set.m_options.size(); i < n; ++i) {
            auto it = set.m_def->option(i);
            if (it->name == optionName) {
                return set.m_options[i].m_string;
            }
        }
    }
    return MString();
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
int32_t PluginTranslatorOptionsInstance::getEnum(const char* optionName)
{
    for (auto& set : m_optionSets) {
        for (size_t i = 0, n = set.m_options.size(); i < n; ++i) {
            auto it = set.m_def->option(i);
            if (it->name == optionName) {
                return set.m_options[i].m_int;
            }
        }
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
void PluginTranslatorOptionsInstance::OptionSet::toOptionVars(const char* const prefix)
{
    int     i = 0;
    MString options;
    for (auto& opt : m_options) {
        const PluginTranslatorOptions::Option* option = m_def->option(i++);
        MString                                name = option->name;
        name.substitute(" ", "_");
        options += name;
        options += "=";
        switch (option->type) {
        case OptionType::kBool: options += opt.m_bool; break;
        case OptionType::kInt:
        case OptionType::kEnum: options += opt.m_int; break;
        case OptionType::kFloat: options += opt.m_float; break;
        case OptionType::kString: options += opt.m_string; break;
        }
        options += ";";
    }
    MString optionVarName = prefix;
    optionVarName += m_def->grouping();
    MGlobal::setOptionVarValue(optionVarName, options);
}

//----------------------------------------------------------------------------------------------------------------------
void PluginTranslatorOptionsInstance::OptionSet::fromOptionVars(const char* const prefix)
{
    MString optionVarName = prefix;
    optionVarName += m_def->grouping();

    MString      options = MGlobal::optionVarStringValue(optionVarName);
    MStringArray splitOptions;
    options.split(';', splitOptions);
    for (uint32_t i = 0, n = splitOptions.length(); i < n; ++i) {
        MStringArray split;
        splitOptions[i].split('=', split);
        MString optName = split[0];
        optName.substitute("_", " ");
        const MString& optValue = split[1];
        for (uint32_t j = 0; j < m_options.size(); ++j) {
            auto opt = m_def->option(j);
            if (opt->name == optName) {
                switch (opt->type) {
                case OptionType::kBool: m_options[j].m_bool = optValue.asInt() != 0; break;

                case OptionType::kInt:
                case OptionType::kEnum: m_options[j].m_int = optValue.asInt(); break;

                case OptionType::kFloat: m_options[j].m_float = optValue.asFloat(); break;

                case OptionType::kString: m_options[j].m_string = optValue; break;
                }
                break;
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
static inline MString makeName(MString s)
{
    s.substitute(" ", "_");
    return s;
}

//----------------------------------------------------------------------------------------------------------------------
void PluginTranslatorOptions::generateBoolGlobals(
    const char* const prefix,
    const MString&    niceName,
    const MString&    optionName,
    MString&          code,
    bool              value)
{
    MString valueStr;
    valueStr = int(value);
    MString controlName = MString(prefix) + "_" + makeName(optionName);
    MString createCommand = MString("global proc create_") + controlName + "() {"
        + MString("checkBox -l \"") + niceName + "\" -v " + valueStr + " " + controlName + ";}\n";
    MString postCommand = MString("global proc post_") + controlName
        + "(string $value){ eval (\"checkBox -e -v \" + $value + \" " + controlName + "\");}\n";
    MString buildCommand = MString("global proc string build_") + controlName
        + "(){ string $str = \"" + makeName(optionName) + "=\"; if(` checkBox -q -v " + controlName
        + "`) $str = $str + \"1;\"; else $str = $str + \"0;\"; return $str;}\n";

    code += createCommand;
    code += postCommand;
    code += buildCommand;
    code += "\n";
}

//----------------------------------------------------------------------------------------------------------------------
void PluginTranslatorOptions::generateIntGlobals(
    const char* const prefix,
    const MString&    niceName,
    const MString&    optionName,
    MString&          code,
    int               value)
{
    MString valueStr;
    valueStr = value;
    MString controlName = MString(prefix) + "_" + makeName(optionName);
    MString createCommand = MString("global proc create_") + controlName + "() {"
        + MString("intFieldGrp -l \"") + niceName + "\" -v1 " + valueStr + " " + controlName
        + ";}\n";
    MString postCommand = MString("global proc post_") + controlName
        + "(string $value){ eval (\"intFieldGrp -e -v1 \" + $value + \" " + controlName + "\");}\n";
    MString buildCommand = MString("global proc string build_") + controlName
        + "(){ string $str = \"" + makeName(optionName) + "=\" + `intFieldGrp -q -v1 " + controlName
        + "` + \";\"; return $str;}\n";

    code += createCommand;
    code += postCommand;
    code += buildCommand;
    code += "\n";
}

//----------------------------------------------------------------------------------------------------------------------
void PluginTranslatorOptions::generateFloatGlobals(
    const char* const  prefix,
    const MString&     niceName,
    const MString&     optionName,
    MString&           code,
    float              value,
    int                precision,
    const std::string& controller,
    bool               enableState)
{
    MString valueStr;
    valueStr = value;
    MString controlName = MString(prefix) + "_" + makeName(optionName);
    // Check and add on/off command if any
    MString onOffCmd;
    MString postUpdateCmd;
    MString deferredPostUpdateCmd;
    if (!controller.empty()) {
        MString prefixedCtrl(MString(prefix) + "_" + makeName(controller.c_str()));
        if (enableState) {
            onOffCmd = "checkBox -e "
                       "-onCommand \"floatFieldGrp -e -en 1 "
                + controlName
                + "\" "
                  "-offCommand \"floatFieldGrp -e -en 0 "
                + controlName + "\" " + prefixedCtrl + "; ";
            // And update the default state according to the checkbox
            postUpdateCmd = "floatFieldGrp -e -en `checkBox -q -v " + prefixedCtrl + "` "
                + controlName + "; ";
        } else {
            onOffCmd = "checkBox -e "
                       "-onCommand \"floatFieldGrp -e -en 0 "
                + controlName
                + "\" "
                  "-offCommand \"floatFieldGrp -e -en 1 "
                + controlName + "\" " + prefixedCtrl + "; ";
            postUpdateCmd = "floatFieldGrp -e -en (`checkBox -q -v " + prefixedCtrl + "` ? 0: 1) "
                + controlName + "; ";
        }
        onOffCmd += postUpdateCmd;
        deferredPostUpdateCmd = MString() + "eval(\"" + postUpdateCmd + "\");";
    }

    MString createCommand = MString("global proc create_") + controlName + "() {"
        + MString("floatFieldGrp -l \"") + niceName + "\" -v1 " + valueStr + " -pre " + precision
        + " " + controlName + ";" + onOffCmd + "}\n";
    MString postCommand = MString("global proc post_") + controlName
        + "(string $value){ eval (\"floatFieldGrp -e -v1 \" + $value + \" " + controlName + "\");"
        + deferredPostUpdateCmd + "}\n";
    MString buildCommand = MString("global proc string build_") + controlName
        + "(){ string $str = \"" + makeName(optionName) + "=\" + `floatFieldGrp -q -v1 "
        + controlName + "` + \";\"; return $str;}\n";

    code += createCommand;
    code += postCommand;
    code += buildCommand;
    code += "\n";
}

std::string stringify(const char* str);
//----------------------------------------------------------------------------------------------------------------------
void PluginTranslatorOptions::generateStringGlobals(
    const char* const prefix,
    const MString&    niceName,
    const MString&    optionName,
    MString&          code,
    const char*       value)
{
    MString controlName = MString(prefix) + "_" + makeName(optionName);
    MString createCommand = MString("global proc create_") + controlName + "() {"
        + MString("textFieldGrp -l \"") + niceName + "\" -tx \"" + stringify(value).c_str() + "\" "
        + controlName + ";}\n";
    MString postCommand = MString("global proc post_") + controlName
        + "(string $value){ eval (\"textFieldGrp -e -tx \" + $value + \" " + controlName
        + "\");}\n";
    MString buildCommand = MString("global proc string build_") + controlName
        + "(){ string $str = \"" + makeName(optionName) + "=\" + `textFieldGrp -q -tx "
        + controlName + "` + \";\"; return $str;}\n";

    code += createCommand;
    code += postCommand;
    code += buildCommand;
    code += "\n";
}

//----------------------------------------------------------------------------------------------------------------------
void PluginTranslatorOptions::generateEnumGlobals(
    const char* const prefix,
    const MString&    niceName,
    const MString&    optionName,
    const MString     enumValues[],
    uint32_t          enumCount,
    MString&          code,
    int               value)
{
    MString controlName = MString(prefix) + "_" + makeName(optionName);
    MString createCommand = MString("global proc create_") + controlName + "() {"
        + MString("optionMenuGrp -l \"") + niceName + "\" " + controlName + ";";

    for (uint32_t i = 0; i < enumCount; ++i) {
        createCommand += MString("menuItem -l \"") + enumValues[i] + "\";";
    }
    MString valueStr;
    valueStr = value + 1;
    createCommand
        += "eval (\"optionMenuGrp -e -sl \" + " + valueStr + " + \" " + controlName + "\");\n";
    createCommand += "}\n";
    MString postCommand = MString("global proc post_") + controlName
        + "(string $value){ int $v=$value; eval (\"optionMenuGrp -e -sl \" + ($v + 1) + \" "
        + controlName + "\");}\n";
    MString buildCommand = MString("global proc string build_") + controlName
        + "(){ string $str = \"" + makeName(optionName) + "=\" + (`optionMenuGrp -q -sl "
        + controlName + "` -1) + \";\"; return $str;}\n";

    code += createCommand;
    code += postCommand;
    code += buildCommand;
    code += "\n";
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
MString PluginTranslatorOptions::generateGUI(const char* const prefix, MString& guiCode)
{
    for (uint32_t j = 0; j < m_options.size(); ++j) {
        auto opt = option(j);
        switch (opt->type) {
        case OptionType::kBool:
            generateBoolGlobals(prefix, opt->name, opt->name, guiCode, opt->defBool);
            break;
        case OptionType::kInt:
            generateIntGlobals(prefix, opt->name, opt->name, guiCode, opt->defInt);
            break;
        case OptionType::kFloat:
            generateFloatGlobals(
                prefix,
                opt->name,
                opt->name,
                guiCode,
                opt->defFloat,
                opt->precision,
                opt->controller,
                opt->enableState);
            break;
        case OptionType::kString:
            generateStringGlobals(prefix, opt->name, opt->name, guiCode, opt->defString.asChar());
            break;
        case OptionType::kEnum:
            generateEnumGlobals(
                prefix,
                opt->name,
                opt->name,
                opt->enumStrings.data(),
                opt->enumStrings.size(),
                guiCode,
                opt->defInt);
            break;
        }
    }

    MString groupName = makeName(grouping());
    MString methodName = prefix;
    methodName += groupName;

    MString code = "global proc create_";
    code += methodName;
    code += "()\n{\n";
    code += "  frameLayout -cll true -l \"" + grouping() + "\";\n";
    code += "  columnLayout;\n";

    for (uint32_t j = 0; j < m_options.size(); ++j) {
        auto    opt = option(j);
        MString controlName = MString(prefix) + "_" + makeName(opt->name);
        code += (MString("  create_" + controlName + "();\n"));
    }
    code += "}\n";

    code += "global proc int post_";
    code += methodName;
    code += "(string $name, string $value)\n{\n";

    for (uint32_t j = 0; j < m_options.size(); ++j) {
        auto    opt = option(j);
        MString controlName = MString(prefix) + "_" + makeName(opt->name);
        code += (MString(
            "  if($name == \"" + makeName(opt->name) + "\") { post_" + controlName
            + "($value); return 1; } else\n"));
    }
    code += "  {}\n";
    code += "  return 0;\n";
    code += "}\n";

    code += "global proc string query_";
    code += methodName;
    code += "()\n{\n  string $result;\n";

    for (uint32_t j = 0; j < m_options.size(); ++j) {
        auto    opt = option(j);
        MString controlName = MString(prefix) + "_" + makeName(opt->name);
        code += (MString("  $result += `build_" + controlName + "`;\n"));
    }
    code += "  return $result;\n";
    code += "}\n";

    guiCode += code;

    return methodName;
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
void PluginTranslatorOptionsInstance::toOptionVars(const char* const prefix)
{
    for (auto& set : m_optionSets) {
        set.toOptionVars(prefix);
    }
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
void PluginTranslatorOptionsInstance::fromOptionVars(const char* const prefix)
{
    for (auto& set : m_optionSets) {
        set.fromOptionVars(prefix);
    }
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
void PluginTranslatorOptionsContext::generateGUI(const char* const prefix, MString& code)
{
    MStringArray methodNames;
    for (auto& set : m_optionGroups) {
        MString methodName = set->generateGUI(prefix, code);
        methodNames.append(methodName);
    }

    code += "global proc fromOptionVars_";
    code += prefix;
    code += "()\n{\n";
    code += "  string $optionList[];\n";
    code += "  string $optionBreakDown[];\n";
    code += "  string $result;\n";

    for (uint32_t j = 0; j < methodNames.length(); ++j) {
        MString optionVarName = prefix;
        optionVarName += makeName(m_optionGroups[j]->grouping());
        code += MString("  if(`optionVar -ex \"") + optionVarName + "\"`) {\n";
        code += MString("    $result = `optionVar -q \"") + optionVarName + "\"`;\n";
        code += "    tokenize($result, \";\", $optionList);\n";
        code += "    for ($index = 0; $index < size($optionList); $index++) {\n";
        code += "      tokenize($optionList[$index], \"=\", $optionBreakDown);\n";
        code += "      if(size($optionBreakDown) < 2) continue;\n";
        code += MString("      post_") + methodNames[j]
            + "($optionBreakDown[0], $optionBreakDown[1]);\n";
        code += "    }\n";
        code += "  }\n";
    }
    code += "}\n";

    code += "global proc create_";
    code += prefix;
    code += "(string $parent)\n{\n";
    for (uint32_t j = 0; j < methodNames.length(); ++j) {
        code += MString("  setParent $parent; create_") + methodNames[j] + "();\n";
    }
    code += "  fromOptionVars_";
    code += prefix;
    code += "();\n";
    code += "}\n";

    code += "global proc post_";
    code += prefix;
    code += "(string $name, string $value)\n{\n";

    for (uint32_t j = 0; j < methodNames.length(); ++j) {
        code += MString("  if(post_") + methodNames[j] + "($name, $value)) return;\n";
    }
    code += "}\n";

    code += "global proc string query_";
    code += prefix;
    code += "()\n{\n";
    code += "  string $result, $temp;\n";
    for (uint32_t j = 0; j < methodNames.length(); ++j) {
        code += MString("  $temp = query_") + methodNames[j] + "();\n";
        code += "  $result += $temp;\n";

        MString optionVarName = prefix;
        optionVarName += makeName(m_optionGroups[j]->grouping());
        code += MString("  optionVar -sv \"") + optionVarName + "\" $temp;\n";
    }
    code += "  return $result;\n}\n";
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
void PluginTranslatorOptionsContextManager::resyncGUI(const char* const translatorName)
{
    auto it = m_contexts.find(translatorName);
    if (it != m_contexts.end()) {
        PluginTranslatorOptionsContext* context = it->second;
        if (context->dirty()) {
            context->resyncGUI(translatorName);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_UTILS_PUBLIC
void PluginTranslatorOptionsContext::resyncGUI(const char* const translatorName)
{
    MString guiCode;
    generateGUI(translatorName, guiCode);
    MGlobal::executeCommand(guiCode);
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
