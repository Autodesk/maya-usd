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
#include "AL/maya/utils/MayaHelperMacros.h"

#include <maya/MPxCommand.h>

#include <sstream>
#include <vector>

namespace AL {
namespace maya {
namespace utils {

/// \brief  Callback type used to provide a list of *things* to an optionMenu control in an option
/// box GUI
typedef MStringArray (*GenerateListFn)(const MString& context);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A method used by the generated command GUI code to retrieve a list of items from C++.
/// \ingroup mayagui
//----------------------------------------------------------------------------------------------------------------------
class CommandGuiListGen : public MPxCommand
{
public:
    AL_MAYA_DECLARE_COMMAND();

    /// \brief  Internal method. Registers a custom list function with this command so that MEL can
    /// request a custom C++
    ///         command to generate a list of items, which are then displayed within a MEL GUI.
    /// \param  generateListFunc a C++ function that can take an optional user specified 'context',
    /// and return a list of
    ///         items to display within the GUI.
    /// \param  menuName the GUI control to which the menu items will be appended.
    /// \return the unique ID for this generator
    AL_MAYA_UTILS_PUBLIC
    static int32_t registerListFunc(GenerateListFn generateListFunc, const MString& menuName);

private:
    bool    isUndoable() const override { return false; }
    MStatus doIt(const MArgList& args) override;
    static std::vector<std::pair<MString, GenerateListFn>> m_funcs;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This class isn't really a wrapper around command options as such, it's mainly just a
/// helper
///         to auto generate some GUI code to create a menu item + option box dialog.
/// \note
/// The following example code demonstrates how to use the CommandGuiHelper class to autogenerate a
/// menu item, which will be available in the menu path  "USD" -> "polygons" -> "Create Cube"; This
/// will call the mel command "polyCube". The total command called will be something akin to:
///
///   polyCube -constructionHistory true -width 1 -height 1.1 -depth 1.2 -subdivisionsX 1
///   subdivisionsY 2 -subdivisionsZ 3 -name "pCube"
///
/// However all of the numeric values will actually be stored as optionVar's. (see the optionVar mel
/// command, or MGlobal class) If the command is "polyCube", and the flag is "constructionHistory",
/// then the optionVar used to store the preference will be "polyCube_constructionHistory".
///
/// Whilst I'm using "polyCube" as an example of how to use this class, you'd probably want to use
/// this for your own MPxCommand derived classes.
///
/// \code
/// {
///   AL::maya::CommandGuiHelper options("polyCube", "Create Polygon Cube", "Create",
///   "USD/polygons/Create Cube"); options.addBoolOption("constructionHistory", "Construction
///   History", true, true); options.addDoubleOption("width", "Width", 1.0, true);
///   options.addDoubleOption("height", "Height", 1.1, true);
///   options.addDoubleOption("depth", "Depth", 1.2, true);
///   options.addIntOption("subdivisionsX", "Subdivisions in X", 1, true);
///   options.addIntOption("subdivisionsY", "Subdivisions in Y", 2, true);
///   options.addIntOption("subdivisionsZ", "Subdivisions in Z", 3, true);
///   options.addStringOption("name", "Name", "pCube", false);
///   options.addVec3Option("axis", "Axis", 0, 1, 0, true);
/// }
/// \endcode
///
/// If the above code is called somewhere within your initialisePlugin method, and you end up
/// calling AL::maya::MenuBuilder::generatePluginUI() at the end of your initialise method, then
/// that command (+optionBox) will be available on the main maya menu. \ingroup mayagui
//----------------------------------------------------------------------------------------------------------------------
class CommandGuiHelper
{
public:
    /// \brief  Used to describe the type of file dialog that should be used for a file path
    /// attribute
    enum FileMode
    {
        kSave = 0,               ///< a save file dialog
        kLoad = 1,               ///< a load file dialog
        kDirectoryWithFiles = 2, ///< a directory dialog, but displays files.
        kDirectory = 3,          ///< a directory dialog
        kMultiLoad = 4           ///< multiple input files
    };

    /// \brief  determines if a text string argument is optional (e.g. a name of the object if
    /// specified, but falls back
    ///         to a default if not), or whether it must exist (e.g. for a file to open)
    enum StringPolicy
    {
        kStringOptional,     ///< if the string value is empty, the flag will be omitted.
        kStringMustHaveValue ///< if the string is empty, it is an error.
    };

    /// \brief  ctor
    /// \param  commandName  the name of the mel command to execute
    /// \param  windowTitle  the title to display at the top of the option box.
    /// \param  doitLabel    The label that will appear on the 'Create/Yes/DoIt' button on the left
    /// hand side of the dialog \param  menuItemPath determines the path to the menu item from the
    /// main menu. \param  hasOptionBox If true, the dialog will only be executed when the option
    /// box button is clicked.
    ///                      If false, there will be no option box, and the GUI will always be
    ///                      displayed.
    AL_MAYA_UTILS_PUBLIC
    CommandGuiHelper(
        const char* commandName,
        const char* windowTitle,
        const char* doitLabel,
        const char* menuItemPath,
        bool        hasOptionBox = true);

    /// \brief  ctor
    /// \brief  add a menu item with a checkbox
    /// \param  commandName    the name of the mel command to execute
    /// \param  menuItemPath   determines the path to the menu item from the main menu.
    /// \param  checkBoxValue  the default value for the checkbox
    AL_MAYA_UTILS_PUBLIC
    CommandGuiHelper(const char* commandName, const char* menuItemPath, bool checkBoxValue = false);

    /// \brief  dtor - auto generates, and executes the GUI code.
    AL_MAYA_UTILS_PUBLIC
    ~CommandGuiHelper();

    /// \brief  add some text to the execute command, unconditionally
    ///         Useful if, ie, you always want to set a non-default command flag
    /// \param  toAdd   extra flags / text to add to the execution command
    AL_MAYA_UTILS_PUBLIC
    void addExecuteText(const char* toAdd);

    /// \brief  add a boolean option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void
    addFlagOption(const char* commandFlag, const char* label, bool defaultVal, bool persist = true);

    /// \brief  add a boolean option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void
    addBoolOption(const char* commandFlag, const char* label, bool defaultVal, bool persist = true);

    /// \brief  add an integer option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void addIntOption(
        const char* commandFlag,
        const char* label,
        int32_t     defaultVal,
        bool        persist = true);

    /// \brief  add an integer option value to the GUI (with min/max, displayed as a slider)
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  minVal   the minimum value for the slider range
    /// \param  maxVal   the maximum value for the slider range
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void addIntSliderOption(
        const char* commandFlag,
        const char* label,
        int32_t     minVal,
        int32_t     maxVal,
        int32_t     defaultVal,
        bool        persist = true);

    /// \brief  add a 2D integer option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void addInt2Option(
        const char*   commandFlag,
        const char*   label,
        const int32_t defaultVal[2],
        bool          persist = true);

    /// \brief  add a 2D integer option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  a   the default value for the 1st value of the flag
    /// \param  b   the default value for the 2st value of the flag
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    void addInt2Option(
        const char*   commandFlag,
        const char*   label,
        const int32_t a,
        const int32_t b,
        bool          persist = true)
    {
        const int32_t temp[] = { a, b };
        addInt2Option(commandFlag, label, temp, persist);
    }

    /// \brief  add a 3D integer option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void addInt3Option(
        const char*   commandFlag,
        const char*   label,
        const int32_t defaultVal[3],
        bool          persist = true);

    /// \brief  add a 3D integer option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  a   the default value for the 1st value of the flag
    /// \param  b   the default value for the 2st value of the flag
    /// \param  c   the default value for the 3rd value of the flag
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    void addInt3Option(
        const char*   commandFlag,
        const char*   label,
        const int32_t a,
        const int32_t b,
        const int32_t c,
        bool          persist = true)
    {
        const int32_t temp[] = { a, b, c };
        addInt3Option(commandFlag, label, temp, persist);
    }

    /// \brief  add a 4D integer option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void addInt4Option(
        const char*   commandFlag,
        const char*   label,
        const int32_t defaultVal[4],
        bool          persist = true);

    /// \brief  add a 4D integer option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  a   the default value for the 1st value of the flag
    /// \param  b   the default value for the 2st value of the flag
    /// \param  c   the default value for the 3rd value of the flag
    /// \param  d   the default value for the 4th value of the flag
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    void addInt4Option(
        const char*   commandFlag,
        const char*   label,
        const int32_t a,
        const int32_t b,
        const int32_t c,
        const int32_t d,
        bool          persist = true)
    {
        const int32_t temp[] = { a, b, c, d };
        addInt4Option(commandFlag, label, temp, persist);
    }

    /// \brief  adds a dynamic drop down list of items that will be displayed within an optionMenu
    /// control.
    ///         The list of text strings will be generated within C++ by the custom generateList
    ///         function, which will then be chosen to represent the list of options available for
    ///         this command. When the command is executed, the selected item will be passed to the
    ///         command as a text string.
    /// \note   Only string command options are supported for this control type, and the command GUI
    /// must be created with
    ///         the 'hasOptionBox' option of the constructor set to false.
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  generateList   The C++ command to build up a list of text strings for the GUI to
    /// display \param  isMandatory   If true, our commandFlag is actually a mandatory argument, not
    /// a option / flag
    AL_MAYA_UTILS_PUBLIC
    void addListOption(
        const char*    commandFlag,
        const char*    label,
        GenerateListFn generateList,
        bool           isMandatory = false);

    /// \brief  add an enum option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultIndex   the default index into the enumNames/enumValues for the enum
    /// \param  enumNames   an array of text string names (last string must be NULL) for the enum
    /// entries. e.g. \code
    ///   const char* const enumStrings[] = {
    ///     "up",
    ///     "down",
    ///     "left",
    ///     "right",
    ///     0
    ///   };
    /// \endcode
    /// \param  enumValues   an array of integer values that match up to the enumNames. This array
    /// can be NULL,
    ///         in which case it is assumed that the enum values are 0, 1, 2, 3, etc.
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    /// \param  passAsString   if true, the command will be passed the text string value of the
    /// enum. If false,
    ///         a numeric value will be passed instead.
    AL_MAYA_UTILS_PUBLIC
    void addEnumOption(
        const char*       commandFlag,
        const char*       label,
        int               defaultIndex,
        const char* const enumNames[],
        const int32_t     enumValues[],
        bool              persist = true,
        bool              passAsString = false);

    /// \brief  Similar to the enum option, but this time with radio buttons. THE MAXIMUM NUMBER OF
    /// OPTIONS IS 4.
    ///         If you exceed this, the code will default to using a combo box for display.
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultIndex   the default index into the enumNames/enumValues for the enum
    /// \param  enumNames   an array of text string names (last string must be NULL) for the enum
    /// entries. e.g. \code
    ///   const char* const enumStrings[] = {
    ///     "up",
    ///     "down",
    ///     "left",
    ///     "right",
    ///     0
    ///   };
    /// \endcode
    /// \param  enumValues - an array of integer values that match up to the enumNames. This array
    /// can be NULL,
    ///         in which case it is assumed that the enum values are 0, 1, 2, 3, etc.
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    /// \param  passAsString - if true, the command will be passed the text string value of the
    /// enum. If false,
    ///         a numeric value will be passed instead.
    AL_MAYA_UTILS_PUBLIC
    void addRadioButtonGroupOption(
        const char*       commandFlag,
        const char*       label,
        int               defaultIndex,
        const char* const enumNames[],
        const int32_t     enumValues[],
        bool              persist = true,
        bool              passAsString = false);

    /// \brief  add a double precision option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void addDoubleOption(
        const char* commandFlag,
        const char* label,
        double      defaultVal,
        bool        persist = true);

    /// \brief  add a double precision option value to the GUI (with min/max, displayed as a slider)
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not setoption
    /// \param  minVal   the minimum value for the slider range
    /// \param  maxVal   the maximum value for the slider range
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void addDoubleSliderOption(
        const char* commandFlag,
        const char* label,
        double      minVal,
        double      maxVal,
        double      defaultVal,
        bool        persist = true);

    /// \brief  add a 2D vector option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void addVec2Option(
        const char*  commandFlag,
        const char*  label,
        const double defaultVal[2],
        bool         persist = true);

    /// \brief  add a 2D vector option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  a   the default value for the 1st value of the flag
    /// \param  b   the default value for the 2st value of the flag
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    void addVec2Option(
        const char*  commandFlag,
        const char*  label,
        const double a,
        const double b,
        bool         persist = true)
    {
        const double temp[] = { a, b };
        addVec2Option(commandFlag, label, temp, persist);
    }

    /// \brief  add a 3D vector option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void addVec3Option(
        const char*  commandFlag,
        const char*  label,
        const double defaultVal[3],
        bool         persist = true);

    /// \brief  add a 3D vector option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  a   the default value for the 1st value of the flag
    /// \param  b   the default value for the 2st value of the flag
    /// \param  c   the default value for the 3rd value of the flag
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    void addVec3Option(
        const char*  commandFlag,
        const char*  label,
        const double a,
        const double b,
        const double c,
        bool         persist = true)
    {
        const double temp[] = { a, b, c };
        addVec3Option(commandFlag, label, temp, persist);
    }

    /// \brief  add a 4D vector option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void addVec4Option(
        const char*  commandFlag,
        const char*  label,
        const double defaultVal[4],
        bool         persist = true);

    /// \brief  add a 4D vector option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  a   the default value for the 1st value of the flag
    /// \param  b   the default value for the 2st value of the flag
    /// \param  c   the default value for the 3rd value of the flag
    /// \param  d   the default value for the 4th value of the flag
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    void addVec4Option(
        const char*  commandFlag,
        const char*  label,
        const double a,
        const double b,
        const double c,
        const double d,
        bool         persist = true)
    {
        const double temp[] = { a, b, c, d };
        addVec4Option(commandFlag, label, temp, persist);
    }

    /// \brief  add a colour option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    AL_MAYA_UTILS_PUBLIC
    void addColourOption(
        const char*  commandFlag,
        const char*  label,
        const double defaultVal[3],
        bool         persist = true);

    /// \brief  add a colour option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  a   the default value for the 1st value of the flag
    /// \param  b   the default value for the 2st value of the flag
    /// \param  c   the default value for the 3rd value of the flag
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    void addColourOption(
        const char*  commandFlag,
        const char*  label,
        const double a,
        const double b,
        const double c,
        bool         persist = true)
    {
        const double temp[] = { a, b, c };
        addColourOption(commandFlag, label, temp, persist);
    }

    /// \brief  add a string option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  defaultVal   the default value for the flag if not set
    /// \param  persist   if true, the GUI option will be persisted as an optionVar
    /// \param  policy   is the string optional (kStringOptional), or required?
    /// (kStringMustHaveValue)
    AL_MAYA_UTILS_PUBLIC
    void addStringOption(
        const char*  commandFlag,
        const char*  label,
        MString      defaultVal,
        bool         persist = true,
        StringPolicy policy = kStringOptional);

    /// \brief  add a file path option value to the GUI
    /// \param  commandFlag   the flag for the command
    /// \param  label   human readable GUI label for the option
    /// \param  fileMode   the type of file dialog you wish to have present
    /// \param  filter   the file extension filter for the file dialog
    /// \param  policy   is the file path optional (kStringOptional), or required?
    /// (kStringMustHaveValue)
    AL_MAYA_UTILS_PUBLIC
    void addFilePathOption(
        const char*  commandFlag,
        const char*  label,
        FileMode     fileMode,
        const char*  filter = "All files (*) (*)",
        StringPolicy policy = kStringOptional);

private:
    std::ostringstream m_global;
    std::ostringstream m_init;
    std::ostringstream m_save;
    std::ostringstream m_load;
    std::ostringstream m_reset;
    std::ostringstream m_execute;
    std::ostringstream m_labels;
    std::ostringstream m_controls;
    std::string        m_commandName;
    bool               m_hasFilePath;
    bool               m_checkBoxCommand;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
