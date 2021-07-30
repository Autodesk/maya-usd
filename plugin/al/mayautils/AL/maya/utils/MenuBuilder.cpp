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
#include "AL/maya/utils/MenuBuilder.h"

namespace AL {
namespace maya {
namespace utils {
//----------------------------------------------------------------------------------------------------------------------
std::set<MenuBuilder::Menu> MenuBuilder::m_menus;

//----------------------------------------------------------------------------------------------------------------------
MenuBuilder::MenuItem* MenuBuilder::addEntry(
    const char* menuItemPath,
    const char* command,
    bool        hasCheckbox,
    bool        defaultCheckBoxValue,
    bool        isRadioButton,
    bool        radioButtonCheckedState)
{
    std::string str(menuItemPath);

    // sort out highest level menu
    size_t begin = 0;
    size_t index = str.find_first_of('/', begin);

    // no seperator found, can't continue
    if (index == std::string::npos) {
        return 0;
    }

    Menu hack;
    hack.m_name = str.substr(begin, index - begin);
    begin = index + 1;
    auto it = m_menus.find(hack);
    if (it == m_menus.end()) {
        it = m_menus.insert(hack).first;
    }

    MenuItem* menuItem = 0;
    while (!menuItem) {
        index = str.find_first_of('/', begin);
        if (index == std::string::npos) {
            // we have the actual menu item, so insert it
            MenuItem item;
            item.label = str.substr(begin);
            item.command = command;
            item.checkBox = hasCheckbox;
            item.checkBoxValue = defaultCheckBoxValue;
            item.radioButton = isRadioButton;
            item.radioButtonValue = radioButtonCheckedState;

            for (size_t i = 0; i < it->m_menuItems.size(); ++i) {
                const MenuItem& curr = it->m_menuItems[i];
                if (curr.label == item.label) {
                    // we have a duplicate menu item, do not add!
                    return 0;
                }
            }
            it->m_menuItems.push_back(item);

            menuItem = it->m_menuItems.data() + (it->m_menuItems.size() - 1);
        } else {
            Menu hack;
            hack.m_name = str.substr(begin, index - begin);
            begin = index + 1;
            auto itc = it->m_childMenus.find(hack);
            if (itc == it->m_childMenus.end()) {
                it = it->m_childMenus.insert(hack).first;
            } else {
                it = itc;
            }
        }
    }
    return menuItem;
}

//----------------------------------------------------------------------------------------------------------------------
bool MenuBuilder::addEntry(
    const char* menuItemPath,
    const char* command,
    const char* optionBoxCommand)
{
    MenuItem* item = addEntry(menuItemPath, command);
    if (item) {
        item->optionBox = optionBoxCommand;
    }
    return item != 0;
}

//----------------------------------------------------------------------------------------------------------------------
void MenuBuilder::Menu::generate(
    std::ostringstream& os,
    std::ostringstream& kill,
    const char*         prefix,
    int                 indent) const
{
    print_indent(os, indent);
    if (!indent) {
        // make sure we explicitly name the highest level menu item
        std::string nameWithNoSpaces = m_name + prefix;
        for (size_t i = 0; i < nameWithNoSpaces.size(); ++i) {
            if (nameWithNoSpaces[i] == ' ' || nameWithNoSpaces[i] == '\t'
                || nameWithNoSpaces[i] == '\n' || nameWithNoSpaces[i] == '\r')
                nameWithNoSpaces[i] = '_';
        }
        os << "if(`menu -exists " << nameWithNoSpaces << "`) return;\n";
        os << "menu -tearOff true -parent $gMainWindow -l \"" << m_name << "\" -aob 1 "
           << nameWithNoSpaces << ";\n";
        kill << nameWithNoSpaces << " ";
    } else {
        os << "menuItem -subMenu true -l \"" << m_name << "\";\n";
    }

    ++indent;
    for (auto it = m_childMenus.begin(); it != m_childMenus.end(); ++it) {
        it->generate(os, kill, prefix, indent);
    }

    // Track if we're in a radio button group or not
    bool inRadioButtonGroup = false;

    for (auto it = m_menuItems.begin(); it != m_menuItems.end(); ++it) {

        // Radio button group must be declared before adding menuItems
        if (!inRadioButtonGroup && it->radioButton) {
            print_indent(os, indent);
            os << "radioMenuItemCollection;\n";
            inRadioButtonGroup = true;
        } else if (inRadioButtonGroup && !it->radioButton) {
            inRadioButtonGroup = false;
        }

        print_indent(os, indent);
        os << "menuItem -l \"" << it->label << "\" -c \"" << it->command << "\"";

        if (it->checkBox) {
            os << " -cb " << it->checkBoxValue;
        } else if (it->radioButton) {
            os << " -radioButton " << (it->radioButtonValue ? "on" : "off");
        }

        os << ";\n";

        if (!it->optionBox.empty()) {
            print_indent(os, indent);
            os << "menuItem -ob 1 -c \"" << it->optionBox << "\";\n";
        }
    }

    print_indent(os, indent);
    os << "setParent -menu ..;\n";
    --indent;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace maya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
