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
#pragma once

#include "AL/maya/utils/Api.h"

#include <maya/MString.h>

#include <map>
#include <vector>

namespace AL {
namespace maya {
namespace utils {

class PluginTranslatorOptions;
class PluginTranslatorOptionsInstance;
typedef std::vector<PluginTranslatorOptions*> PluginTranslatorOptionsArray;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Defines the data type of a file translator option
//----------------------------------------------------------------------------------------------------------------------
enum class OptionType
{
    kBool,   ///< boolean export option
    kInt,    ///< integer export option
    kFloat,  ///< float export option
    kString, ///< string export option
    kEnum    ///< enum export option
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  The PluginTranslatorOptionsContext is used as a container for all export/import options
/// registered to a
///         specific export/import file translator. This class maintains a set of
///         PluginTranslatorOptions which can be registered by translator plugins.
//----------------------------------------------------------------------------------------------------------------------
class PluginTranslatorOptionsContext
{
public:
    /// \brief  register a new grouping of plugin translator options
    /// \param  options the option grouping to insert into this context
    AL_MAYA_UTILS_PUBLIC
    void registerPluginTranslatorOptions(PluginTranslatorOptions* options);

    /// \brief  unregister a new grouping of plugin translator options
    /// \param  pluginTranslatorGrouping the name of the options grouping to removes and destroy
    AL_MAYA_UTILS_PUBLIC
    void unregisterPluginTranslatorOptions(const char* const pluginTranslatorGrouping);

    /// \brief  unregister a new grouping of plugin translator options
    /// \param  pluginTranslatorGrouping the name of the options grouping to removes and destroy
    AL_MAYA_UTILS_PUBLIC
    bool isRegistered(const char* const pluginTranslatorGrouping) const;

    /// \brief  unregister a new grouping of plugin translator options
    /// \param  pluginTranslatorGrouping the name of the options grouping to removes and destroy
    size_t numOptionGroups() const { return m_optionGroups.size(); }

    /// \brief  return the optionGroup at the specified index
    /// \param  index the index of the option group to return
    /// \return a pointer to the option group
    const PluginTranslatorOptions* optionGroup(size_t index) const { return m_optionGroups[index]; }

    /// \brief  A method that is used to regenerate the GUI code (MEL) when a new set of options has
    /// been registered \param  prefix a unique identifier for this specific translator
    AL_MAYA_UTILS_PUBLIC
    void resyncGUI(const char* const prefix);

    /// \brief  generates the MEL script code to create the GUI
    /// \param  prefix a unique name for the translator
    /// \param  guiCode the returned MEL script code
    AL_MAYA_UTILS_PUBLIC
    void generateGUI(const char* const prefix, MString& guiCode);

    /// \brief  Does the GUI need to be resynced?
    /// \return true if the GUI code needs to be resynced, false if the old GUI is still valid
    bool dirty() const { return m_dirty; }

    /// \brief  clear the dirty flag
    void setClean() { m_dirty = false; }

private:
    PluginTranslatorOptionsArray m_optionGroups;
    bool                         m_dirty = true;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class that maintains all registered plugin contexts
//----------------------------------------------------------------------------------------------------------------------
class PluginTranslatorOptionsContextManager
{
public:
    /// \brief  register a context for the specified file translator name
    /// \param  translatorName name of the translator to register
    /// \param  context the context to register
    static void
    registerContext(const char* const translatorName, PluginTranslatorOptionsContext* context)
    {
        m_contexts[translatorName] = context;
    }

    /// \brief  unregisters the context for the given name
    /// \param  translatorName the name of the context to unregister
    static void unregisterContext(const char* const translatorName)
    {
        auto it = m_contexts.find(translatorName);
        if (it != m_contexts.end()) {
            m_contexts.erase(it);
        }
    }

    /// \brief  find the translator context for the specified file translator
    /// \param  translatorName the name of the translator to find
    /// \return a pointer to the context, or null if not found
    static PluginTranslatorOptionsContext* find(const char* const translatorName)
    {
        auto it = m_contexts.find(translatorName);
        if (it != m_contexts.end()) {
            return it->second;
        }
        return 0;
    }

    /// \brief  resyncs the autogenerated MEL code for the specified file translator name
    /// \param  translatorName the name of the file translator to resync the GUI
    AL_MAYA_UTILS_PUBLIC
    static void resyncGUI(const char* const translatorName);

private:
    AL_MAYA_UTILS_PUBLIC
    static std::map<std::string, PluginTranslatorOptionsContext*> m_contexts;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This class holds all of the current values for the plugin translator options
//----------------------------------------------------------------------------------------------------------------------
class PluginTranslatorOptionsInstance
{
public:
    /// \brief  constructor
    /// \param  context the context that describes the options for this file translator
    AL_MAYA_UTILS_PUBLIC
    PluginTranslatorOptionsInstance(PluginTranslatorOptionsContext& context);

    /// \brief  Sets a boolean value to the translator options
    /// \param  optionName the name of the option to set
    /// \param  value the new value for the option
    /// \return true if the option exists, false otherwise
    AL_MAYA_UTILS_PUBLIC
    bool setBool(const char* optionName, bool value);

    /// \brief  Sets an integer value to the translator options
    /// \param  optionName the name of the option to set
    /// \param  value the new value for the option
    /// \return true if the option exists, false otherwise
    AL_MAYA_UTILS_PUBLIC
    bool setInt(const char* optionName, int value);

    /// \brief  Sets a float value to the translator options
    /// \param  optionName the name of the option to set
    /// \param  value the new value for the option
    /// \return true if the option exists, false otherwise
    AL_MAYA_UTILS_PUBLIC
    bool setFloat(const char* optionName, float value);

    /// \brief  Sets a string value to the translator options
    /// \param  optionName the name of the option to set
    /// \param  value the new value for the option
    /// \return true if the option exists, false otherwise
    AL_MAYA_UTILS_PUBLIC
    bool setString(const char* optionName, const char* const value);

    /// \brief  Sets an enum value to the translator options
    /// \param  optionName the name of the option to set
    /// \param  value the new value for the option
    /// \return true if the option exists, false otherwise
    AL_MAYA_UTILS_PUBLIC
    bool setEnum(const char* optionName, int32_t value);

    /// \brief  Gets the current value of the named boolean option
    /// \param  optionName the name of the option to query
    /// \return the option value
    AL_MAYA_UTILS_PUBLIC
    bool getBool(const char* optionName);

    /// \brief  Gets the current value of the named integer option
    /// \param  optionName the name of the option to query
    /// \return the option value
    AL_MAYA_UTILS_PUBLIC
    int getInt(const char* optionName);

    /// \brief  Gets the current value of the named float option
    /// \param  optionName the name of the option to query
    /// \return the option value
    AL_MAYA_UTILS_PUBLIC
    float getFloat(const char* optionName);

    /// \brief  Gets the current value of the named string option
    /// \param  optionName the name of the option to query
    /// \return the option value
    AL_MAYA_UTILS_PUBLIC
    MString getString(const char* optionName);

    /// \brief  Gets the current value of the named enum option
    /// \param  optionName the name of the option to query
    /// \return the option value
    AL_MAYA_UTILS_PUBLIC
    int32_t getEnum(const char* optionName);

    /// \brief  generates the option vars for this set of options
    /// \param  prefix a unique name for the file translator
    AL_MAYA_UTILS_PUBLIC
    void toOptionVars(const char* const prefix);

    /// \brief  extracts the option values from the optionVars
    /// \param  prefix a unique name for the file translator
    AL_MAYA_UTILS_PUBLIC
    void fromOptionVars(const char* const prefix);

    /// \brief  Utility method to parse an option
    /// \param  key the name of the option
    /// \param  value the value for the option
    AL_MAYA_UTILS_PUBLIC
    void parse(MString key, const MString& value);

private:
    struct OptionSet
    {
        OptionSet(const PluginTranslatorOptions* def);
        void toOptionVars(const char* const prefix);
        void fromOptionVars(const char* const prefix);
        struct Option
        {
            union
            {
                float m_float;
                int   m_int;
                bool  m_bool;
            };
            MString m_string;
        };
        const PluginTranslatorOptions* const m_def;
        std::vector<Option>                  m_options;
    };
    std::vector<OptionSet> m_optionSets;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Container for a set of export/import options that can be registered alongside a set of
/// plugin translators.
//----------------------------------------------------------------------------------------------------------------------
class PluginTranslatorOptions
{
public:
    /// \brief  constructor
    /// \param  context the file translator context
    /// \param  pluginTranslatorGrouping
    AL_MAYA_UTILS_PUBLIC
    PluginTranslatorOptions(
        PluginTranslatorOptionsContext& context,
        const char* const               pluginTranslatorGrouping);

    /// \brief  dtor
    AL_MAYA_UTILS_PUBLIC
    ~PluginTranslatorOptions();

    PluginTranslatorOptions(PluginTranslatorOptions&&) = delete;
    PluginTranslatorOptions(const PluginTranslatorOptions&) = delete;
    PluginTranslatorOptions& operator=(PluginTranslatorOptions&&) = delete;
    PluginTranslatorOptions& operator=(const PluginTranslatorOptions&) = delete;

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

    /// \name   Query Exporter Options
    /// \brief  methods to query info about the

    /// \brief  returns true if the specified optionName exists
    /// \param  optionName the name of the option
    /// \return true if the name specified is an option
    AL_MAYA_UTILS_PUBLIC
    bool isOption(const char* const optionName) const;

    /// \brief  returns true if the specified optionName exists
    /// \param  optionName the name of the option
    /// \return the type of the option, or OptionType::kInvalid if the opion does not exist
    AL_MAYA_UTILS_PUBLIC
    OptionType optionType(const char* const optionName) const;

    /// \brief  returns the default value for the option
    /// \param  optionName the name of the option to return
    /// \return the default value for the option
    AL_MAYA_UTILS_PUBLIC
    bool defaultBool(const char* const optionName) const;

    /// \brief  returns the default value for the option
    /// \param  optionName the name of the option to return
    /// \return the default value for the option
    AL_MAYA_UTILS_PUBLIC
    int defaultInt(const char* const optionName) const;

    /// \brief  returns the default value for the option
    /// \param  optionName the name of the option to return
    /// \return the default value for the option
    AL_MAYA_UTILS_PUBLIC
    float defaultFloat(const char* const optionName) const;

    /// \brief  returns the default value for the option
    /// \param  optionName the name of the option to return
    /// \return the default value for the option
    AL_MAYA_UTILS_PUBLIC
    MString defaultString(const char* const optionName) const;

    /// \brief  returns the name of this option group
    /// \return the option group name
    const MString& grouping() const { return m_grouping; }

    /// \brief  Generates the MEL script GUI code
    /// \param  prefix unique name for the file translator
    /// \param  guiCode the returned GUI code
    /// \return the name of the method that needs to be invoked
    AL_MAYA_UTILS_PUBLIC
    MString generateGUI(const char* const prefix, MString& guiCode);

    /// \brief  struct used to store meta data about a particular translator option
    struct Option
    {
        MString name; ///< name of the translator option
        union
        {
            float defFloat; ///< default float value
            int   defInt;   ///< default int value
            bool  defBool;  ///< default bool value
        };
        MString              defString;   ///< default string value
        std::vector<MString> enumStrings; ///< the text values for the enums
        OptionType           type;        ///< the type of the option

        /// \brief  ctor
        /// \param  name the name of the option
        /// \param  defVal the default value
        Option(const char* const name, const bool& defVal)
            : name(name)
            , type(OptionType::kBool)
        {
            defBool = defVal;
        }

        /// \brief  ctor
        /// \param  name the name of the option
        /// \param  defVal the default value
        Option(const char* const name, const int& defVal)
            : name(name)
            , type(OptionType::kInt)
        {
            defInt = defVal;
        }

        /// \brief  ctor
        /// \param  name the name of the option
        /// \param  defVal the default value
        Option(const char* const name, const float& defVal)
            : name(name)
            , type(OptionType::kFloat)
        {
            defFloat = defVal;
        }

        /// \brief  ctor
        /// \param  name the name of the option
        /// \param  defVal the default value
        Option(const char* const name, const char* const defVal)
            : name(name)
            , type(OptionType::kString)
        {
            defString = defVal;
        }

        /// \brief  ctor
        /// \param  name the name of the option
        /// \param  defVal the default value
        /// \param  enumStrs a null terminated array of enum option strings
        Option(const char* const name, const int& defVal, const char* const enumStrs[])
            : name(name)
            , type(OptionType::kEnum)
        {
            defInt = defVal;
            for (auto ptr = enumStrs; *ptr; ++ptr) {
                enumStrings.emplace_back(*ptr);
            }
        }

        /// \brief  equivalence operator
        /// \param  name name of the option
        /// \return true if the name matches this option
        bool operator==(const char* const name) const { return this->name == name; }
    };

    /// \brief  returns the number of options
    /// \return the number of options
    size_t numOptions() const { return m_options.size(); }

    /// \brief  returns the option at the specified index
    /// \param  i the index of the option to return
    /// \return a pointer to the option
    Option* option(size_t i) { return m_options.data() + i; }

    /// \brief  returns the option at the specified index
    /// \param  i the index of the option to return
    /// \return a pointer to the option
    const Option* option(size_t i) const { return m_options.data() + i; }

private:
    static void generateBoolGlobals(
        const char* const prefix,
        const MString&    niceName,
        const MString&    optionName,
        MString&          code,
        bool);
    static void generateIntGlobals(
        const char* const prefix,
        const MString&    niceName,
        const MString&    optionName,
        MString&          code,
        int);
    static void generateFloatGlobals(
        const char* const prefix,
        const MString&    niceName,
        const MString&    optionName,
        MString&          code,
        float);
    static void generateStringGlobals(
        const char* const prefix,
        const MString&    niceName,
        const MString&    optionName,
        MString&          code,
        const char*);
    static void generateEnumGlobals(
        const char* const prefix,
        const MString&    niceName,
        const MString&    optionName,
        const MString     enumValues[],
        uint32_t          enumCount,
        MString&          code,
        int);

private:
    MString                         m_grouping;
    std::vector<Option>             m_options;
    PluginTranslatorOptionsContext& m_context;
};

//----------------------------------------------------------------------------------------------------------------------
inline PluginTranslatorOptionsInstance::OptionSet::OptionSet(const PluginTranslatorOptions* def)
    : m_def(def)
{
    size_t n = def->numOptions();
    m_options.resize(n);
    for (size_t i = 0; i < n; ++i) {
        const PluginTranslatorOptions::Option* opt = def->option(i);
        switch (opt->type) {
        case OptionType::kBool: m_options[i].m_bool = opt->defBool; break;
        case OptionType::kInt: m_options[i].m_int = opt->defInt; break;
        case OptionType::kFloat: m_options[i].m_float = opt->defFloat; break;
        case OptionType::kString: m_options[i].m_string = opt->defString; break;
        case OptionType::kEnum: m_options[i].m_int = opt->defInt; break;
        default: break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
