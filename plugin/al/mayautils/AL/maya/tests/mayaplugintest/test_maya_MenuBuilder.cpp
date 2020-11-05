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
//#include "test_usdmaya.h"
#include "AL/maya/utils/MenuBuilder.h"

#include <gtest/gtest.h>

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test USD to attribute enum mappings
//----------------------------------------------------------------------------------------------------------------------
TEST(maya_MenuBuilder, simplePath)
{
    AL::maya::utils::MenuBuilder::clearRootMenus();
    EXPECT_TRUE(AL::maya::utils::MenuBuilder::addEntry("FOO/BAR/MOO/HI", "sphere", "sphereOB"));
    EXPECT_EQ(size_t(1), AL::maya::utils::MenuBuilder::rootMenus().size());

    const AL::maya::utils::MenuBuilder::Menu& menu0
        = *AL::maya::utils::MenuBuilder::rootMenus().begin();
    EXPECT_EQ(std::string("FOO"), menu0.name());
    EXPECT_EQ(size_t(1), menu0.childMenus().size());
    EXPECT_EQ(size_t(0), menu0.menuItems().size());

    const AL::maya::utils::MenuBuilder::Menu& menu1 = *menu0.childMenus().begin();
    EXPECT_EQ(std::string("BAR"), menu1.name());
    EXPECT_EQ(size_t(1), menu1.childMenus().size());
    EXPECT_EQ(size_t(0), menu1.menuItems().size());

    const AL::maya::utils::MenuBuilder::Menu& menu2 = *menu1.childMenus().begin();
    EXPECT_EQ(std::string("MOO"), menu2.name());
    EXPECT_EQ(size_t(0), menu2.childMenus().size());
    EXPECT_EQ(size_t(1), menu2.menuItems().size());

    const AL::maya::utils::MenuBuilder::MenuItem& item = menu2.menuItems()[0];
    EXPECT_EQ(std::string("HI"), item.label);
    EXPECT_EQ(std::string("sphere"), item.command);
    EXPECT_EQ(std::string("sphereOB"), item.optionBox);
    EXPECT_FALSE(item.checkBox);
    EXPECT_FALSE(item.checkBoxValue);

    AL::maya::utils::MenuBuilder::clearRootMenus();
}

// Make sure that when we add menu item paths, the entries get added into the correct menu paths.
TEST(maya_MenuBuilder, sharedPath)
{
    AL::maya::utils::MenuBuilder::clearRootMenus();
    EXPECT_TRUE(AL::maya::utils::MenuBuilder::addEntry("FOO/BAR/MOO/HI", "sphere", "sphereOB"));
    EXPECT_TRUE(AL::maya::utils::MenuBuilder::addEntry("FOO/BAR/OINK/HI", "cube", "cubeOB"));
    EXPECT_EQ(size_t(1), AL::maya::utils::MenuBuilder::rootMenus().size());

    const AL::maya::utils::MenuBuilder::Menu& menu0
        = *AL::maya::utils::MenuBuilder::rootMenus().begin();
    EXPECT_EQ(std::string("FOO"), menu0.name());
    EXPECT_EQ(size_t(1), menu0.childMenus().size());
    EXPECT_EQ(size_t(0), menu0.menuItems().size());

    const AL::maya::utils::MenuBuilder::Menu& menu1 = *menu0.childMenus().begin();
    EXPECT_EQ(std::string("BAR"), menu1.name());
    EXPECT_EQ(size_t(2), menu1.childMenus().size());
    EXPECT_EQ(size_t(0), menu1.menuItems().size());

    {
        const AL::maya::utils::MenuBuilder::Menu& menu2 = *menu1.childMenus().begin();
        EXPECT_EQ(std::string("MOO"), menu2.name());
        EXPECT_EQ(size_t(0), menu2.childMenus().size());
        EXPECT_EQ(size_t(1), menu2.menuItems().size());

        const AL::maya::utils::MenuBuilder::MenuItem& item = menu2.menuItems()[0];
        EXPECT_EQ(std::string("HI"), item.label);
        EXPECT_EQ(std::string("sphere"), item.command);
        EXPECT_EQ(std::string("sphereOB"), item.optionBox);
        EXPECT_FALSE(item.checkBox);
        EXPECT_FALSE(item.checkBoxValue);
    }

    {
        auto it = menu1.childMenus().begin();
        ++it;
        const AL::maya::utils::MenuBuilder::Menu& menu2 = *it;
        EXPECT_EQ(std::string("OINK"), menu2.name());
        EXPECT_EQ(size_t(0), menu2.childMenus().size());
        EXPECT_EQ(size_t(1), menu2.menuItems().size());

        const AL::maya::utils::MenuBuilder::MenuItem& item = menu2.menuItems()[0];
        EXPECT_EQ(std::string("HI"), item.label);
        EXPECT_EQ(std::string("cube"), item.command);
        EXPECT_EQ(std::string("cubeOB"), item.optionBox);
        EXPECT_FALSE(item.checkBox);
        EXPECT_FALSE(item.checkBoxValue);
    }
    AL::maya::utils::MenuBuilder::clearRootMenus();
}

// Make sure that when we add menu item paths under the same menu path, the entries get added
// correctly
TEST(maya_MenuBuilder, sharedPath2)
{
    AL::maya::utils::MenuBuilder::clearRootMenus();
    EXPECT_TRUE(AL::maya::utils::MenuBuilder::addEntry("FOO/BAR/MOO/HI", "sphere", "sphereOB"));
    EXPECT_TRUE(AL::maya::utils::MenuBuilder::addEntry("FOO/BAR/MOO/BYE", "cube", "cubeOB"));
    EXPECT_EQ(size_t(1), AL::maya::utils::MenuBuilder::rootMenus().size());

    const AL::maya::utils::MenuBuilder::Menu& menu0
        = *AL::maya::utils::MenuBuilder::rootMenus().begin();
    EXPECT_EQ(std::string("FOO"), menu0.name());
    EXPECT_EQ(size_t(1), menu0.childMenus().size());
    EXPECT_EQ(size_t(0), menu0.menuItems().size());

    const AL::maya::utils::MenuBuilder::Menu& menu1 = *menu0.childMenus().begin();
    EXPECT_EQ(std::string("BAR"), menu1.name());
    EXPECT_EQ(size_t(1), menu1.childMenus().size());
    EXPECT_EQ(size_t(0), menu1.menuItems().size());

    const AL::maya::utils::MenuBuilder::Menu& menu2 = *menu1.childMenus().begin();
    EXPECT_EQ(std::string("MOO"), menu2.name());
    EXPECT_EQ(size_t(0), menu2.childMenus().size());
    EXPECT_EQ(size_t(2), menu2.menuItems().size());

    const AL::maya::utils::MenuBuilder::MenuItem& item1 = menu2.menuItems()[0];
    EXPECT_EQ(std::string("HI"), item1.label);
    EXPECT_EQ(std::string("sphere"), item1.command);
    EXPECT_EQ(std::string("sphereOB"), item1.optionBox);
    EXPECT_FALSE(item1.checkBox);
    EXPECT_FALSE(item1.checkBoxValue);

    const AL::maya::utils::MenuBuilder::MenuItem& item2 = menu2.menuItems()[1];
    EXPECT_EQ(std::string("BYE"), item2.label);
    EXPECT_EQ(std::string("cube"), item2.command);
    EXPECT_EQ(std::string("cubeOB"), item2.optionBox);
    EXPECT_FALSE(item2.checkBox);
    EXPECT_FALSE(item2.checkBoxValue);

    AL::maya::utils::MenuBuilder::clearRootMenus();
}

// make sure we can't add a duplicate entry.
// I suppose there is no real reason why we can't do this from a technical standpoint,
// however from a UI standpoint, having two menu items in the same menu labeled the same,
// is probably going to confuse petest_alUsdMayaople
TEST(maya_MenuBuilder, duplicatePath)
{
    AL::maya::utils::MenuBuilder::clearRootMenus();
    EXPECT_TRUE(AL::maya::utils::MenuBuilder::addEntry("FOO/BAR/MOO/HI", "sphere", "sphereOB"));
    EXPECT_FALSE(AL::maya::utils::MenuBuilder::addEntry("FOO/BAR/MOO/HI", "cube", "cubeOB"));
    EXPECT_EQ(size_t(1), AL::maya::utils::MenuBuilder::rootMenus().size());

    const AL::maya::utils::MenuBuilder::Menu& menu0
        = *AL::maya::utils::MenuBuilder::rootMenus().begin();
    EXPECT_EQ(std::string("FOO"), menu0.name());
    EXPECT_EQ(size_t(1), menu0.childMenus().size());
    EXPECT_EQ(size_t(0), menu0.menuItems().size());

    const AL::maya::utils::MenuBuilder::Menu& menu1 = *menu0.childMenus().begin();
    EXPECT_EQ(std::string("BAR"), menu1.name());
    EXPECT_EQ(size_t(1), menu1.childMenus().size());
    EXPECT_EQ(size_t(0), menu1.menuItems().size());

    const AL::maya::utils::MenuBuilder::Menu& menu2 = *menu1.childMenus().begin();
    EXPECT_EQ(std::string("MOO"), menu2.name());
    EXPECT_EQ(size_t(0), menu2.childMenus().size());
    EXPECT_EQ(size_t(1), menu2.menuItems().size());

    const AL::maya::utils::MenuBuilder::MenuItem& item = menu2.menuItems()[0];
    EXPECT_EQ(std::string("HI"), item.label);
    EXPECT_EQ(std::string("sphere"), item.command);
    EXPECT_EQ(std::string("sphereOB"), item.optionBox);
    EXPECT_FALSE(item.checkBox);
    EXPECT_FALSE(item.checkBoxValue);

    AL::maya::utils::MenuBuilder::clearRootMenus();
}

// make sure the checkbox values are correctly assigned
TEST(maya_MenuBuilder, simpleCheckbox)
{
    AL::maya::utils::MenuBuilder::clearRootMenus();
    EXPECT_TRUE(
        0 != AL::maya::utils::MenuBuilder::addEntry("FOO/BAR/MOO/HI1", "Checky", true, false));
    EXPECT_TRUE(
        0 != AL::maya::utils::MenuBuilder::addEntry("FOO/BAR/MOO/HI2", "McCheckFace", true, true));
    EXPECT_EQ(size_t(1), AL::maya::utils::MenuBuilder::rootMenus().size());

    const AL::maya::utils::MenuBuilder::Menu& menu0
        = *AL::maya::utils::MenuBuilder::rootMenus().begin();
    EXPECT_EQ(std::string("FOO"), menu0.name());
    EXPECT_EQ(size_t(1), menu0.childMenus().size());
    EXPECT_EQ(size_t(0), menu0.menuItems().size());

    const AL::maya::utils::MenuBuilder::Menu& menu1 = *menu0.childMenus().begin();
    EXPECT_EQ(std::string("BAR"), menu1.name());
    EXPECT_EQ(size_t(1), menu1.childMenus().size());
    EXPECT_EQ(size_t(0), menu1.menuItems().size());

    const AL::maya::utils::MenuBuilder::Menu& menu2 = *menu1.childMenus().begin();
    EXPECT_EQ(std::string("MOO"), menu2.name());
    EXPECT_EQ(size_t(0), menu2.childMenus().size());
    EXPECT_EQ(size_t(2), menu2.menuItems().size());

    const AL::maya::utils::MenuBuilder::MenuItem& item1 = menu2.menuItems()[0];
    EXPECT_EQ(std::string("HI1"), item1.label);
    EXPECT_EQ(std::string("Checky"), item1.command);
    EXPECT_EQ(std::string(""), item1.optionBox);
    EXPECT_TRUE(item1.checkBox);
    EXPECT_FALSE(item1.checkBoxValue);

    const AL::maya::utils::MenuBuilder::MenuItem& item2 = menu2.menuItems()[1];
    EXPECT_EQ(std::string("HI2"), item2.label);
    EXPECT_EQ(std::string("McCheckFace"), item2.command);
    EXPECT_EQ(std::string(""), item2.optionBox);
    EXPECT_TRUE(item2.checkBox);
    EXPECT_TRUE(item2.checkBoxValue);

    AL::maya::utils::MenuBuilder::clearRootMenus();
}

// Test radio button values are correctly assigned
TEST(maya_MenuBuilder, simpleRadioButton)
{
    AL::maya::utils::MenuBuilder::clearRootMenus();
    EXPECT_TRUE(
        0
        != AL::maya::utils::MenuBuilder::addEntry(
            "FOO/BAR/MOO/HI1", "Radio", false, false, true, false));
    EXPECT_TRUE(
        0
        != AL::maya::utils::MenuBuilder::addEntry(
            "FOO/BAR/MOO/HI2", "McRadioFace", false, false, true, true));
    EXPECT_EQ(size_t(1), AL::maya::utils::MenuBuilder::rootMenus().size());

    const AL::maya::utils::MenuBuilder::Menu& menu0
        = *AL::maya::utils::MenuBuilder::rootMenus().begin();
    EXPECT_EQ(std::string("FOO"), menu0.name());
    EXPECT_EQ(size_t(1), menu0.childMenus().size());
    EXPECT_EQ(size_t(0), menu0.menuItems().size());

    const AL::maya::utils::MenuBuilder::Menu& menu1 = *menu0.childMenus().begin();
    EXPECT_EQ(std::string("BAR"), menu1.name());
    EXPECT_EQ(size_t(1), menu1.childMenus().size());
    EXPECT_EQ(size_t(0), menu1.menuItems().size());

    const AL::maya::utils::MenuBuilder::Menu& menu2 = *menu1.childMenus().begin();
    EXPECT_EQ(std::string("MOO"), menu2.name());
    EXPECT_EQ(size_t(0), menu2.childMenus().size());
    EXPECT_EQ(size_t(2), menu2.menuItems().size());

    const AL::maya::utils::MenuBuilder::MenuItem& item1 = menu2.menuItems()[0];
    EXPECT_EQ(std::string("HI1"), item1.label);
    EXPECT_EQ(std::string("Radio"), item1.command);
    EXPECT_EQ(std::string(""), item1.optionBox);
    EXPECT_TRUE(item1.radioButton);
    EXPECT_FALSE(item1.radioButtonValue);

    const AL::maya::utils::MenuBuilder::MenuItem& item2 = menu2.menuItems()[1];
    EXPECT_EQ(std::string("HI2"), item2.label);
    EXPECT_EQ(std::string("McRadioFace"), item2.command);
    EXPECT_EQ(std::string(""), item2.optionBox);
    EXPECT_TRUE(item2.radioButton);
    EXPECT_TRUE(item2.radioButtonValue);

    AL::maya::utils::MenuBuilder::clearRootMenus();
}