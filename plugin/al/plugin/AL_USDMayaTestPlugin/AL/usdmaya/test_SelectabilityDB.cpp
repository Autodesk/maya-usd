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

#include <AL/usdmaya/SelectabilityDB.h>

#include <gtest/gtest.h>
using namespace AL::usdmaya;

TEST(SelectabilityDB, makingParentPathsSelectable)
{
    SdfPath rootPath("/A");
    SdfPath childPath("/A/B");
    SdfPath grandchildPath("/A/B/C");

    SelectabilityDB selectableDB;
    // Test that adding a single and multiple path works.
    {
        selectableDB.addPathAsUnselectable(childPath);
        const SdfPathVector& selectablePaths = selectableDB.getUnselectablePaths();

        ASSERT_TRUE(selectablePaths.size() == 1);
        EXPECT_TRUE(selectablePaths[0] == childPath);

        selectableDB.addPathAsUnselectable(grandchildPath);

        ASSERT_TRUE(selectablePaths.size() == 2);
        EXPECT_TRUE(selectablePaths[1] == grandchildPath);
    }
}
/*
 * Test that using SelectableDB's adding API is working
 */
// void SelectableDB::addPathAsSelectable(const SdfPath& path)

TEST(SelectabilityDB, selectedPaths)
{
    SdfPath rootPath("/A");
    SdfPath childPath("/A/B");
    SdfPath grandchildPath("/A/B/C");
    SdfPath secondChildPath("/A/D");

    SelectabilityDB selectableDB;
    // Test that adding a single and multiple path works.
    {
        selectableDB.addPathAsUnselectable(childPath);
        // Test that all the paths that are expected to be selectable are.
        EXPECT_TRUE(selectableDB.isPathUnselectable(childPath));
        EXPECT_TRUE(selectableDB.isPathUnselectable(grandchildPath));

        EXPECT_FALSE(selectableDB.isPathUnselectable(rootPath));
        EXPECT_FALSE(selectableDB.isPathUnselectable(secondChildPath));
    }
}
/*
 * Test that using UnselectableDB's removal API is working
 */
// void SelectableDB::addPathAsUnselectable(const SdfPath& path)
// void SelectableDB::removePathAsUnselectable(const SdfPath& path)
TEST(SelectabilityDB, removePaths)
{
    SdfPath rootPath("/A");
    SdfPath childPath("/A/B");
    SdfPath grandchildPath("/A/B/C");
    SdfPath secondChildPath("/A/D");

    SelectabilityDB selectable;
    // Test that removing selectable paths from the SelectionDB works.
    {
        //
        selectable.addPathAsUnselectable(childPath);
        const SdfPathVector& unselectablePaths = selectable.getUnselectablePaths();
        EXPECT_TRUE(unselectablePaths.size() == 1);
        selectable.addPathAsUnselectable(grandchildPath);
        EXPECT_TRUE(unselectablePaths.size() == 2);
        selectable.removePathAsUnselectable(childPath);
        EXPECT_TRUE(unselectablePaths.size() == 1);
    }
}
