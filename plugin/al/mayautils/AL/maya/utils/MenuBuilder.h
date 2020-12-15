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
#include "AL/maya/utils/DebugCodes.h"

#include <maya/MGlobal.h>

#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace AL {
namespace maya {
namespace utils {
//----------------------------------------------------------------------------------------------------------------------
/// \brief  You shouldn't need to care about this class (sort of). Probably the only thing you'll
/// want to do is execute \code
///             MenuBuilder::generatePluginUI<MFnPlugin>(fnPlugin, "myplugin");
/// \endcode
///         somewhere at the end of your plugins' initialize function. Any command GUI's that have
///         previously been defined, will end up being added into the main menu.
///
///         I suppose there is absolutely no reason why you couldn't add your own menu items. All it
///         needs is either:
/// \code
///             // simple command from the menu item
///             MenuBuilder::addEntry("My Menu/Sub Menu/Menu Item", "someMelCommand");
///
///             // or simple command connected to checkbox.
///             // first param indicates we want a checkbox, second param indicates the checkbox
///             value MenuBuilder::addEntry("My Menu/Sub Menu/Menu Item", "someMelCommand", true,
///             true);
///
///             // or with an option box
///             MenuBuilder::addEntry("My Menu/Sub Menu/Menu Item", "someMelCommand",
///             "optionBoxMelCommand");
/// \endcode
///         That wasn't quite what I had in mind when I wrote the class, but it will work just fine.
///         Just make sure that MenuBuilder::generatePluginUI is the last method you call in your
///         initializePlugin method.
/// \ingroup mayagui
//----------------------------------------------------------------------------------------------------------------------
class MenuBuilder
{
public:
    /// \brief  A structure that represents a menu item
    struct MenuItem
    {
        std::string label;            ///< the menu item label
        std::string command;          ///< the MEL command to execute
        std::string optionBox;        ///< the MEL command to execute when the option box is checked
        bool        checkBox;         ///< true if an option box exists for this item
        bool        checkBoxValue;    ///< the default value for the check box if it is enabled
        bool        radioButton;      ///< true if this is a radio butto menu item
        bool        radioButtonValue; ///< true if this radio button should be checked
    };

#ifndef AL_GENERATING_DOCS
    struct Menu
    {
        friend class MenuBuilder;

        ~Menu()
        {
            m_name.clear();
            m_childMenus.clear();
            m_menuItems.clear();
        }

        inline bool operator<(const Menu& menu) const { return m_name < menu.m_name; }

        inline bool operator==(const Menu& menu) const { return m_name == menu.m_name; }

        inline void print_indent(std::ostream& os, int indent) const
        {
            for (int i = 0; i <= indent; ++i)
                os << "  ";
        }

        AL_MAYA_UTILS_PUBLIC
        void generate(
            std::ostringstream& os,
            std::ostringstream& kill,
            const char*         prefix,
            int                 indent = 0) const;

        const std::string&           name() const { return m_name; }
        const std::set<Menu>&        childMenus() const { return m_childMenus; }
        const std::vector<MenuItem>& menuItems() const { return m_menuItems; }

    private:
        std::string                   m_name;
        mutable std::set<Menu>        m_childMenus;
        mutable std::vector<MenuItem> m_menuItems;
    };

    // for unit testing ONLY!!
    static const std::set<Menu>& rootMenus() { return m_menus; }

    // for unit testing ONLY!!
    static void clearRootMenus() { m_menus.clear(); }
#endif

    /// \brief  add an entry to the menu
    /// \param  menuItemPath forward slash seperated path to the menu item, e.g.
    /// "Create/polygons/Construct Teapot" \param  command the MEL command to execute when the item
    /// is clicked. \param  hasCheckbox if true, the menu item will have a check box. If false, only
    /// a menu item will exist. \param  defaultCheckBoxValue If the checkbox has been enabled, this
    /// will determine whether it is on or off by default \param  isRadioButton if true, this menu
    /// item will be created as a radio button. The radio button group declaration
    ///         command will be added automatically during code generation.
    /// \param  radioButtonCheckedState The checked state of the radio button. If multiple radio
    /// buttons from the same group
    ///         are initialised as checked, the last true state will supplant the others.
    /// \return pointer to the menu item added
    AL_MAYA_UTILS_PUBLIC
    static MenuItem* addEntry(
        const char* menuItemPath,
        const char* command,
        bool        hasCheckbox = false,
        bool        defaultCheckBoxValue = false,
        bool        isRadioButton = false,
        bool        radioButtonCheckedState = false);

    /// \brief  add an entry to the menu
    /// \param  menuItemPath forward slash seperated path to the menu item, e.g.
    /// "Create/polygons/Construct Teapot" \param  command the MEL command to execute when the item
    /// is clicked. \param  optionBoxCommand the MEL command to execute when the option box of the
    /// menu is clicked. \return true if the item was added successfully
    AL_MAYA_UTILS_PUBLIC
    static bool
    addEntry(const char* menuItemPath, const char* command, const char* optionBoxCommand);

    /// \brief  generates an init and exit script that intialises the GUI on plugin load/unload (via
    /// MFnPlugin::registerUI).
    ///         This method should only be called once during your plugins initializePlugin method,
    ///         and that should probably be at or near the end of that function call.
    /// \param  fnPlugin an instance of an MFnPlugin class
    /// \param  prefix some unique prefix that is unique to your plugin
    /// \param  extraOnInit some extra MEL code to execute within the initGUI method for your plugin
    /// \param  extraOnExit some extra MEL code to execute within the uninitGUI method for your
    /// plugin \return MS::kSuccess is everything went well.
    template <typename FnPlugin>
    static MStatus generatePluginUI(
        FnPlugin&      fnPlugin,
        const MString& prefix,
        const MString& extraOnInit = "",
        const MString& extraOnExit = "")
    {
        if (m_menus.empty())
            return MS::kSuccess;
        MString ui_init = prefix + "_initGUI";
        MString ui_exit = prefix + "_exitGUI";

        // set up init script
        std::ostringstream initGUI;
        initGUI << "global proc " << ui_init.asChar() << "()\n{\n  global string $gMainWindow;\n";

        if (extraOnInit.length())
            initGUI << "  " << extraOnInit.asChar() << std::endl;

        // set up exit script
        std::ostringstream exitGUI;
        exitGUI << "global proc " << ui_exit.asChar() << "()\n{\n";
        if (extraOnExit.length())
            exitGUI << "  " << extraOnExit.asChar() << std::endl;
        exitGUI << "  deleteUI ";

        // now construct all of the menu related gubbins.
        for (auto it = m_menus.begin(); it != m_menus.end(); ++it) {
            it->generate(initGUI, exitGUI, prefix.asChar());
        }
        m_menus.clear();

        // finish off

        initGUI << "}\n\n";
        exitGUI << ";\n}\n\n";

        MGlobal::executeCommand(initGUI.str().c_str());
        MGlobal::executeCommand(exitGUI.str().c_str());

        if (AL_MAYAUTILS_DEBUG) {
            // This use to use Pxr's TFDebug to print, but since after the refactor there is now no
            // dependency on the USD library list
            std::cout << initGUI.str() + "\n" << std::endl;
            std::cout << exitGUI.str() + "\n" << std::endl;
        }

        return fnPlugin.registerUI(ui_init, ui_exit);
    }

private:
    AL_MAYA_UTILS_PUBLIC
    static std::set<Menu> m_menus;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
