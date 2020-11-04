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

#include "AL/maya/utils/Api.h"
#include "AL/maya/utils/PluginTranslatorOptions.h"

#include <iostream>

namespace AL {
namespace maya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Utility class that parses the file translator options passed through by Maya
/// \ingroup mayagui
//----------------------------------------------------------------------------------------------------------------------
class OptionsParser
{
    friend class FileTranslatorOptions;

public:
    /// \brief  ctor
    AL_MAYA_UTILS_PUBLIC
    OptionsParser(PluginTranslatorOptionsInstance* pluginOptions = 0);

    /// \brief  dtor
    AL_MAYA_UTILS_PUBLIC
    ~OptionsParser();

    /// \brief  Given a string containing a semi-colon separated list of options passed to a file
    /// translator plugin,
    ///         this function will parse and extract all of the option values.
    /// \param  optionString the option string to parse
    /// \return MS::kSuccess if the parsing was successful, false otherwise.
    AL_MAYA_UTILS_PUBLIC
    MStatus parse(const MString& optionString);

    /// \brief  Given a string containing a semi-colon separated list of options passed to a file
    /// translator plugin,
    ///         this function will parse and extract all of the option values.
    /// \param  optionString the option string to parse
    /// \return MS::kSuccess if the parsing was successful, false otherwise.
    AL_MAYA_UTILS_PUBLIC
    void construct(MString& optionString);

    /// \brief  Given the text name of an option, returns the boolean value for that option.
    /// \param  str the name of the option
    /// \return the option value
    bool getBool(const char* const str) const
    {
        auto it = m_niceNameToValue.find(str);
        if (it != m_niceNameToValue.end()) {
            return it->second->m_bool;
        } else if (m_pluginOptions) {
            return m_pluginOptions->getBool(str);
        } else
            std::cout << "no plugin options" << std::endl;
        return false;
    }

    /// \brief  Given the text name of an option, returns the integer value for that option.
    /// \param  str the name of the option
    /// \return the option value
    int getInt(const char* const str) const
    {
        auto it = m_niceNameToValue.find(str);
        if (it != m_niceNameToValue.end()) {
            return it->second->m_int;
        } else if (m_pluginOptions) {
            return m_pluginOptions->getInt(str);
        }
        return 0;
    }

    /// \brief  Given the text name of an option, returns the floating point value for that option.
    /// \param  str the name of the option
    /// \return the option value
    float getFloat(const char* const str) const
    {
        auto it = m_niceNameToValue.find(str);
        if (it != m_niceNameToValue.end()) {
            return it->second->m_float;
        } else if (m_pluginOptions) {
            return m_pluginOptions->getFloat(str);
        }
        return 0.0f;
    }

    /// \brief  Given the text name of an option, returns the string value for that option.
    /// \param  str the name of the option
    /// \return the option value
    MString getString(const char* const str) const
    {
        auto it = m_niceNameToValue.find(str);
        if (it != m_niceNameToValue.end()) {
            return it->second->m_string.c_str();
        } else if (m_pluginOptions) {
            return m_pluginOptions->getString(str);
        }
        return kNullString;
    }

    /// \brief  Given the text name of an option, returns the boolean value for that option.
    /// \param  str the name of the option
    /// \return the option value
    void setBool(const char* const str, bool value)
    {
        auto it = m_niceNameToValue.find(str);
        if (it != m_niceNameToValue.end()) {
            it->second->m_bool = value;
        } else if (m_pluginOptions) {
            m_pluginOptions->setBool(str, value);
        }
    }

    /// \brief  Given the text name of an option, returns the integer value for that option.
    /// \param  str the name of the option
    /// \return the option value
    void setInt(const char* const str, int value)
    {
        auto it = m_niceNameToValue.find(str);
        if (it != m_niceNameToValue.end()) {
            it->second->m_int = value;
        } else if (m_pluginOptions) {
            m_pluginOptions->setInt(str, value);
        }
    }

    /// \brief  Given the text name of an option, returns the floating point value for that option.
    /// \param  str the name of the option
    /// \return the option value
    void setFloat(const char* const str, float value)
    {
        auto it = m_niceNameToValue.find(str);
        if (it != m_niceNameToValue.end()) {
            it->second->m_float = value;
        } else if (m_pluginOptions) {
            m_pluginOptions->setFloat(str, value);
        }
    }

    /// \brief  Given the text name of an option, returns the string value for that option.
    /// \param  str the name of the option
    /// \return the option value
    void setString(const char* const str, const MString& value)
    {
        auto it = m_niceNameToValue.find(str);
        if (it != m_niceNameToValue.end()) {
            it->second->m_string = std::string(value.asChar(), value.length());
        } else if (m_pluginOptions) {
            m_pluginOptions->setString(str, value.asChar());
        }
    }

    void setPluginOptionsContext(PluginTranslatorOptionsInstance* pluginOptions)
    {
        m_pluginOptions = pluginOptions;
    }

private:
#ifndef AL_GENERATING_DOCS
    enum OptionType
    {
        kBool,
        kInt,
        kFloat,
        kString,
        kEnum
    };

    AL_MAYA_UTILS_PUBLIC
    static const MString kNullString;
    struct OptionValue
    {
        void init()
        {
            switch (m_type) {
            case kBool: m_bool = m_defaultBool; break;
            case kInt: m_int = m_defaultInt; break;
            case kFloat: m_float = m_defaultFloat; break;
            case kString: m_string = m_defaultString; break;
            case kEnum: m_int = m_defaultInt; break;
            default: break;
            }
        }

        void parse(MString& str)
        {
            switch (m_type) {
            case kBool: m_bool = str.asInt() ? true : false; break;
            case kInt: m_int = str.asInt(); break;
            case kFloat: m_float = str.asFloat(); break;
            case kString: m_string = str.asChar(); break;
            case kEnum: m_int = str.asInt(); break;
            default: break;
            }
        }

        OptionType m_type;
        union
        {
            bool  m_defaultBool;
            int   m_defaultInt;
            float m_defaultFloat;
        };
        std::string m_defaultString;
        union
        {
            bool  m_bool;
            int   m_int;
            float m_float;
        };
        std::string m_string;
    };
    std::map<std::string, OptionValue*> m_optionNameToValue;
    std::map<std::string, OptionValue*> m_niceNameToValue;
    PluginTranslatorOptionsInstance*    m_pluginOptions;
#endif
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Utility class that constructs the file translator export GUI from the export options you
/// want to support.
//----------------------------------------------------------------------------------------------------------------------
class FileTranslatorOptions
{
public:
    /// \brief  ctor
    /// \param  fileTranslatorName the name of the file translator
    AL_MAYA_UTILS_PUBLIC
    FileTranslatorOptions(const char* fileTranslatorName);

    /// \name   High Level Layout
    /// \brief  A collection of file translator options can be grouped into 1 or more GUI frames
    /// within the GUI.
    ///         At a minimum, there must be at least 1 frame added to the GUI prior to any options
    ///         being added.

    /// \brief  add a new frame layout under which to group a set of controls. There must be at
    /// least 1 frame created
    ///         before you create any options (otherwise the controls will not have a location in
    ///         which to live)
    /// \param  frameName the name of the high level frame to add into the GUI
    AL_MAYA_UTILS_PUBLIC
    bool addFrame(const char* frameName);

    /// \brief  remove frame layout and it's set of controls
    /// \param  frameName the name of the high level frame to remove
    AL_MAYA_UTILS_PUBLIC
    bool removeFrame(const char* frameName);

    /// \name   Add Exporter Options
    /// \brief  Add custom export/import options using the following methods. There must be at least
    /// 1 frame layout
    ///         created prior to creating any new controls.

    /// \brief  Add a boolean value to the translator options
    /// \param  optionName the name of the option
    /// \param  defaultValue the default value for the option
    /// \return true if the option was successfully added. False if the option is a duplicate
    AL_MAYA_UTILS_PUBLIC
    bool addBool(const char* optionName, bool defaultValue = false);

    /// \brief  Add an integer value to the translator options
    /// \param  optionName the name of the option
    /// \param  defaultValue the default value for the option
    /// \return true if the option was successfully added. False if the option is a duplicate
    AL_MAYA_UTILS_PUBLIC
    bool addInt(const char* optionName, int defaultValue = 0);

    /// \brief  Add a float value to the translator options
    /// \param  optionName the name of the option
    /// \param  defaultValue the default value for the option
    /// \return true if the option was successfully added. False if the option is a duplicate
    AL_MAYA_UTILS_PUBLIC
    bool addFloat(const char* optionName, float defaultValue = 0.0f);

    /// \brief  Add a string value to the translator options
    /// \param  optionName the name of the option
    /// \param  defaultValue the default value for the option
    /// \return true if the option was successfully added. False if the option is a duplicate
    AL_MAYA_UTILS_PUBLIC
    bool addString(const char* optionName, const char* const defaultValue = "");

    /// \brief  Add an integer value to the translator options
    /// \param  optionName the name of the option
    /// \param  enumValues a null terminated list of strings for each enum entry
    /// \param  defaultValue the default value for the option
    /// \return true if the option was successfully added. False if the option is a duplicate
    AL_MAYA_UTILS_PUBLIC
    bool addEnum(const char* optionName, const char* const enumValues[], int defaultValue = 0);

    /// \brief  For a given boolean option (the controller), if enabled the 'controlled' option will
    /// be editable. If
    ///         the checkbox is uncecked, the controlled option will be disabled in the GUI. The
    ///         invertBehaviour param reverses this behaviour (i.e. if controller is true,
    ///         controlled will be disabled).
    /// \param  controller the name of the option that controls the enabled state of the controlled
    /// option. \param  controlled the name of the option to enable/disable based on the state of
    /// the controlled attribute. \param  invertBehaviour if true, controlled will be disabled when
    /// controller is enabled.
    ///         if false, controlled will be enabled when controller is enabled.
    /// \return true if the visibility relationship could be created. May return false if the
    /// controller or
    ///         controlled options are invalid.
    AL_MAYA_UTILS_PUBLIC
    bool boolControlsVisibility(
        const char* controller,
        const char* controlled,
        bool        invertBehaviour = false);

    /// \name   MEL script code generation
    /// \brief  Once all of the options have been added for the file translator, then you can easily
    /// generate the
    ///         MEL script code via the generateScript method.

    /// \brief  This method generates the MEL script for the import/export GUI, and evaluates it
    /// behind the scenes.
    ///         It also configure the option parser for use by the MPxFileTranslator object, and
    ///         generates the defaultOptionString required when registering the function.
    /// \param  optionParser the option parser in which the options for the file translator have
    /// been specified \param  defaultOptionString the returned default option string for the file
    /// translator (which can be used
    ///         when registering your file translator with the MFnPlugin)
    /// \return MS::kSuccess if ok
    AL_MAYA_UTILS_PUBLIC
    MStatus generateScript(OptionsParser& optionParser, MString& defaultOptionString);

    /// \brief  This method initialises all of the options stored within the option parser
    /// \param  optionParser the option parser in which the options for the file translator have
    /// been specified \return MS::kSuccess if ok
    AL_MAYA_UTILS_PUBLIC
    MStatus initParser(OptionsParser& optionParser);

protected:
#ifndef AL_GENERATING_DOCS
    bool hasOption(const char* const optionName) const;
    bool hasOption(const MString& optionName) const { return hasOption(optionName.asChar()); }
    void generateBoolGlobals(const MString& niceName, const MString& optionName, bool defaultValue);
    void generateIntGlobals(const MString& niceName, const MString& optionName, int defaultValue);
    void
    generateFloatGlobals(const MString& niceName, const MString& optionName, float defaultValue);
    void
         generateStringGlobals(const MString& niceName, const MString& optionName, MString defaultValue);
    void generateEnumGlobals(
        const MString&    niceName,
        const MString&    optionName,
        const char* const enumVals[],
        int               defaultValue);
#endif
private:
#ifndef AL_GENERATING_DOCS
    enum OptionType
    {
        kBool,
        kInt,
        kFloat,
        kString,
        kEnum
    };
    struct FrameLayout
    {
        FrameLayout(const char* frameName)
            : m_frameName(frameName)
            , m_options()
        {
        }
        MString m_frameName;
        struct Option
        {
            MString    optionName;
            MString    niceName;
            OptionType type;
            union
            {
                bool        defaultBool;
                int         defaultInt;
                float       defaultFloat;
                const char* defaultString;
            };
            const char* const* enumValues;

            Option() = default;
            Option(const Option& opt)
                : optionName(opt.optionName)
                , niceName(opt.niceName)
                , type(opt.type)
                , enumValues(opt.enumValues)
            {
                switch (type) {
                case kBool: defaultBool = opt.defaultBool; break;
                case kInt:
                case kEnum: defaultInt = opt.defaultInt; break;
                case kFloat: defaultFloat = opt.defaultFloat; break;
                case kString: defaultString = opt.defaultString; break;
                }
            }
        };
        std::vector<Option> m_options;
    };
    FrameLayout&                             lastFrame() { return *(m_frames.end() - 1); }
    std::vector<FrameLayout>                 m_frames;
    std::vector<std::pair<MString, MString>> m_visibility;
    MString                                  m_translatorName;
    MString                                  m_code;
#endif
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
