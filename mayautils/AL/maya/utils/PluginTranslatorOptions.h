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

#include "./Api.h"

#include "maya/MString.h"

#include <map>
#include <vector>
#include <string>

#include "AL/maya/utils/ForwardDeclares.h"

namespace AL {
namespace maya {
namespace utils {

class PluginTranslatorOptions;
class PluginTranslatorOptionsInstance;
typedef std::vector<PluginTranslatorOptions*> PluginTranslatorOptionsArray;

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
enum class OptionType
{
  kBool,
  kInt,
  kFloat,
  kString,
  kEnum
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Maintains a set of options all relating to a set of 
//----------------------------------------------------------------------------------------------------------------------
class PluginTranslatorOptionsContext
{
public:

  /// \brief  build the additional frames for the translator options into the export options dialog
  /// \param  parentLayout the GUI layout to insert the frames into 
  AL_MAYA_UTILS_PUBLIC
  void appendExportGUI(MString& parentLayout);

  /// \brief  build the additional frames for the translator options into the import options dialog
  /// \param  parentLayout the GUI layout to insert the frames into
  AL_MAYA_UTILS_PUBLIC
  void appendImportGUI(MString& parentLayout);

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
  size_t numOptionGroups() const
    { return m_optionGroups.size(); }

  const PluginTranslatorOptions* optionGroup(size_t index) const
    { return m_optionGroups[index]; }

  /// \brief  and
  /// \param  prefix
  AL_MAYA_UTILS_PUBLIC
  void resyncGUI(const char* const prefix);

  /// \brief  extract the current settings from optionVars 
  AL_MAYA_UTILS_PUBLIC
  void generateGUI(const char* const prefix, MString& guiCode);

  bool dirty() const
    { return m_dirty; }

  void setClean() 
    { m_dirty = false; }

private:
  PluginTranslatorOptionsArray m_optionGroups;
  bool m_dirty = true;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class tht can be used to locate a registered plugin translator context.
//----------------------------------------------------------------------------------------------------------------------
class PluginTranslatorOptionsContextManager
{
public:

  /// \brief  register a context for the specified file translator name
  /// \param  translatorName name of the translator to register
  /// \param  context the context to register
  static void registerContext(const char* const translatorName, PluginTranslatorOptionsContext* context)
  {
    m_contexts[translatorName] = context;
  }

  /// \brief  unregisters the context for the given name 
  /// \param  translatorName the name of the context to unregister
  static void unregisterContext(const char* const translatorName)
  {
    auto it = m_contexts.find(translatorName);
    if(it != m_contexts.end())
    {
      m_contexts.erase(it);
    }
  }

  /// \brief  find the translator context for the specified file translator
  /// \param  translatorName the name of the translator to find
  /// \return a pointer to the context, or null if not found
  static PluginTranslatorOptionsContext* find(const char* const translatorName)
  {
    auto it = m_contexts.find(translatorName);
    if(it != m_contexts.end())
    {
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
/// \brief  Maintains a set of options all relating to a set of 
//----------------------------------------------------------------------------------------------------------------------
class PluginTranslatorOptionsInstance
{
public:

  /// \brief  constructor 
  /// \param  context the context that describes the options for this file translator
  AL_MAYA_UTILS_PUBLIC
  PluginTranslatorOptionsInstance(PluginTranslatorOptionsContext& context);

  /// \brief  Add a boolean value to the translator options
  /// \param  optionName the name of the option
  /// \param  defaultValue the default value for the option
  /// \return true if the option was successfully added. False if the option is a duplicate
  AL_MAYA_UTILS_PUBLIC
  bool setBool(const char* optionName, bool value);

  /// \brief  Add an integer value to the translator options
  /// \param  optionName the name of the option
  /// \param  defaultValue the default value for the option
  /// \return true if the option was successfully added. False if the option is a duplicate
  AL_MAYA_UTILS_PUBLIC
  bool setInt(const char* optionName, int value);

  /// \brief  Add a float value to the translator options
  /// \param  optionName the name of the option
  /// \param  defaultValue the default value for the option
  /// \return true if the option was successfully added. False if the option is a duplicate
  AL_MAYA_UTILS_PUBLIC
  bool setFloat(const char* optionName, float value);

  /// \brief  Add a string value to the translator options
  /// \param  optionName the name of the option
  /// \param  defaultValue the default value for the option
  /// \return true if the option was successfully added. False if the option is a duplicate
  AL_MAYA_UTILS_PUBLIC
  bool setString(const char* optionName, const char* const value);

  /// \brief  Add an integer value to the translator options
  /// \param  optionName the name of the option
  /// \param  enumValues a null terminated list of strings for each enum entry
  /// \param  defaultValue the default value for the option
  /// \return true if the option was successfully added. False if the option is a duplicate
  AL_MAYA_UTILS_PUBLIC
  bool setEnum(const char* optionName, int32_t value);

  /// \brief  Add a boolean value to the translator options
  /// \param  optionName the name of the option
  /// \param  defaultValue the default value for the option
  /// \return true if the option was successfully added. False if the option is a duplicate
  AL_MAYA_UTILS_PUBLIC
  bool getBool(const char* optionName);

  /// \brief  Add an integer value to the translator options
  /// \param  optionName the name of the option
  /// \param  defaultValue the default value for the option
  /// \return true if the option was successfully added. False if the option is a duplicate
  AL_MAYA_UTILS_PUBLIC
  int getInt(const char* optionName);

  /// \brief  Add a float value to the translator options
  /// \param  optionName the name of the option
  /// \param  defaultValue the default value for the option
  /// \return true if the option was successfully added. False if the option is a duplicate
  AL_MAYA_UTILS_PUBLIC
  float getFloat(const char* optionName);

  /// \brief  Add a string value to the translator options
  /// \param  optionName the name of the option
  /// \param  defaultValue the default value for the option
  /// \return true if the option was successfully added. False if the option is a duplicate
  AL_MAYA_UTILS_PUBLIC
  MString getString(const char* optionName);

  /// \brief  Add an integer value to the translator options
  /// \param  optionName the name of the option
  /// \param  enumValues a null terminated list of strings for each enum entry
  /// \param  defaultValue the default value for the option
  /// \return true if the option was successfully added. False if the option is a duplicate
  AL_MAYA_UTILS_PUBLIC
  int32_t getEnum(const char* optionName);

  /// \brief  generates the option vars for this set of options
  AL_MAYA_UTILS_PUBLIC
  void toOptionVars(const char* const prefix);

  /// \brief  pulls the option vars for this set of options
  AL_MAYA_UTILS_PUBLIC
  void fromOptionVars(const char* const prefix);

  /// \brief  extract the current settings from optionVars 
  //AL_MAYA_UTILS_PUBLIC
  //void generateGUI(const char* const prefix, MString& guiCode);

  /// \brief  
  AL_MAYA_UTILS_PUBLIC
  void resyncGUI();


  AL_MAYA_UTILS_PUBLIC
  void parse(const MString& key, const MString& value);

private:

  struct OptionSet 
  {
    OptionSet(const PluginTranslatorOptions* def);
    void toOptionVars(const char* const prefix);
    void fromOptionVars(const char* const prefix);
    //static void generateBoolGlobals(const char* const prefix, const MString& niceName, const MString& optionName, MString& code, bool);
    //static void generateIntGlobals(const char* const prefix, const MString& niceName, const MString& optionName, MString& code, int);
    //static void generateFloatGlobals(const char* const prefix, const MString& niceName, const MString& optionName, MString& code, float);
    //static void generateStringGlobals(const char* const prefix, const MString& niceName, const MString& optionName, MString& code, const char*);
    //static void generateEnumGlobals(const char* const prefix, const MString& niceName, const MString& optionName, const MString enumValues[], uint32_t enumCount, MString& code, int);
    struct Option 
    {
      union
      {
        float m_float;
        int m_int;
        bool m_bool;
      };
      MString m_string;
    };
    const PluginTranslatorOptions* const m_def;
    std::vector<Option> m_options;
  };
  std::vector<OptionSet> m_optionSets;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Container for a set of export/import options that can be registered alongside a set of plugin translators.
//----------------------------------------------------------------------------------------------------------------------
class PluginTranslatorOptions
{
public:

  AL_MAYA_UTILS_PUBLIC
  PluginTranslatorOptions(PluginTranslatorOptionsContext& context, const char* const pluginTranslatorGrouping);

  AL_MAYA_UTILS_PUBLIC
  ~PluginTranslatorOptions();

  PluginTranslatorOptions(PluginTranslatorOptions&&) = delete;
  PluginTranslatorOptions(const PluginTranslatorOptions&) = delete;
  PluginTranslatorOptions& operator = (PluginTranslatorOptions&&) = delete;
  PluginTranslatorOptions& operator = (const PluginTranslatorOptions&) = delete;


  /// \name   Add Exporter Options
  /// \brief  Add custom export/import options using the following methods. There must be at least 1 frame layout
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

  AL_MAYA_UTILS_PUBLIC
  bool isOption(const char* const optionName) const;

  AL_MAYA_UTILS_PUBLIC
  OptionType optionType(const char* const optionName) const;

  AL_MAYA_UTILS_PUBLIC
  bool defaultBool(const char* const optionName) const;

  AL_MAYA_UTILS_PUBLIC
  int defaultInt(const char* const optionName) const;

  AL_MAYA_UTILS_PUBLIC
  float defaultFloat(const char* const optionName) const;

  AL_MAYA_UTILS_PUBLIC
  MString defaultString(const char* const optionName) const;

  /// \brief  Store the current settings as optionVars 
  AL_MAYA_UTILS_PUBLIC
  void toOptionVars();

  /// \brief  extract the current settings from optionVars 
  AL_MAYA_UTILS_PUBLIC
  void fromOptionVars();

  /// \brief  extract the current settings from optionVars 
  AL_MAYA_UTILS_PUBLIC
  void fromDefaults();

  /// \brief  extract the current settings from optionVars 
  AL_MAYA_UTILS_PUBLIC
  void generateExportGUI(MString& guiCode);

  /// \brief  extract the current settings from optionVars 
  AL_MAYA_UTILS_PUBLIC
  void generateImportGUI(MString& guiCode);

  const MString& grouping() const
    { return m_grouping; }

  AL_MAYA_UTILS_PUBLIC
  MString generateGUI(const char* const prefix, MString& guiCode);;

  struct Option 
  {
    MString name;
    union
    {
      float defFloat;
      int defInt;
      bool defBool;
    };
    MString defString;
    std::vector<MString> enumStrings;
    OptionType type;

    Option(const char* const name, const bool& defVal)
    : name(name), type(OptionType::kBool) { defBool = defVal; }

    Option(const char* const name, const int& defVal)
    : name(name), type(OptionType::kInt) { defInt = defVal; }

    Option(const char* const name, const float& defVal)
    : name(name), type(OptionType::kFloat) { defFloat = defVal; }

    Option(const char* const name, const char* const defVal)
    : name(name), type(OptionType::kString) { defString = defVal; }

    Option(const char* const name, const int& defVal, const char* const enumStrs[])
    : name(name), type(OptionType::kEnum) 
    {
      defInt = defVal;
      for(auto ptr = enumStrs; *ptr; ++ptr)
      {
        enumStrings.emplace_back(*ptr);
      }
    }

    bool operator == (const char* const name) const
      { return this->name == name; }
  };

  size_t numOptions() const 
    { return m_options.size(); }

  Option* option(size_t i)
    { return m_options.data() + i; }

  const Option* option(size_t i) const
    { return m_options.data() + i; }

private:
    static void generateBoolGlobals(const char* const prefix, const MString& niceName, const MString& optionName, MString& code, bool);
    static void generateIntGlobals(const char* const prefix, const MString& niceName, const MString& optionName, MString& code, int);
    static void generateFloatGlobals(const char* const prefix, const MString& niceName, const MString& optionName, MString& code, float);
    static void generateStringGlobals(const char* const prefix, const MString& niceName, const MString& optionName, MString& code, const char*);
    static void generateEnumGlobals(const char* const prefix, const MString& niceName, const MString& optionName, const MString enumValues[], uint32_t enumCount, MString& code, int);
private:
  MString m_grouping;
  std::vector<Option> m_options;
  PluginTranslatorOptionsContext& m_context;
};

//----------------------------------------------------------------------------------------------------------------------
inline PluginTranslatorOptionsInstance::OptionSet::OptionSet(const PluginTranslatorOptions* def) : m_def(def)
{
  size_t n = def->numOptions();
  m_options.resize(n);
  for(size_t i = 0; i < n; ++i)
  {
    const PluginTranslatorOptions::Option* opt = def->option(i);
    switch(opt->type)
    {
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
} // utils
} // maya
} // AL
//----------------------------------------------------------------------------------------------------------------------
