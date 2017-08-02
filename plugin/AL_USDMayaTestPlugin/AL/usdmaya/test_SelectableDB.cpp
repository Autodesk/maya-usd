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

#include "AL/usdmaya/SelectableDB.h"
#include <gtest/gtest.h>
using namespace AL::usdmaya;

TEST(SelectableDB, makingParentPathsSelectable)
{
  SdfPath rootPath      ("/A");
  SdfPath childPath     ("/A/B");
  SdfPath grandchildPath("/A/B/C");

  SelectableDB selectable;
  // Test that adding a single and multiple path works.
  {
    selectable.addPathAsSelectable(childPath);
    const SdfPathVector& selectablePaths = selectable.getSelectablePaths();

    ASSERT_TRUE(selectablePaths.size() == 1);
    EXPECT_TRUE(selectablePaths[0] == childPath);

    selectable.addPathAsSelectable(grandchildPath);

    ASSERT_TRUE(selectablePaths.size() == 2);
    EXPECT_TRUE(selectablePaths[1] == grandchildPath);
  }
}
/*
 * Test that using SelectableDB's adding API is working
 */
// void SelectableDB::addPathAsSelectable(const SdfPath& path)

TEST(SelectableDB, selectedPaths)
{
  SdfPath rootPath        ("/A");
  SdfPath childPath       ("/A/B");
  SdfPath grandchildPath  ("/A/B/C");
  SdfPath secondChildPath ("/A/D");

  SelectableDB selectable;
  // Test that adding a single and multiple path works.
  {
    //
    selectable.addPathAsSelectable(childPath);
    //Test that all the paths that are expected to be selectable are.
    EXPECT_TRUE(selectable.isPathSelectable(childPath));
    EXPECT_TRUE(selectable.isPathSelectable(grandchildPath));

    EXPECT_FALSE(selectable.isPathSelectable(rootPath));
    EXPECT_FALSE(selectable.isPathSelectable(secondChildPath));
  }
}
/*
 * Test that using SelectableDB's removal API is working
 */
// void SelectableDB::addPathAsSelectable(const SdfPath& path)
// void SelectableDB::removePathAsSelectable(const SdfPath& path)
TEST(SelectableDB, removePaths)
{
  SdfPath rootPath        ("/A");
  SdfPath childPath       ("/A/B");
  SdfPath grandchildPath  ("/A/B/C");
  SdfPath secondChildPath ("/A/D");

  SelectableDB selectable;
  // Test that removing selectable paths from the SelectionDB works.
  {
    //
    selectable.addPathAsSelectable(childPath);
    const SdfPathVector& selectablePaths = selectable.getSelectablePaths();
    EXPECT_TRUE(selectablePaths.size() == 1);
    selectable.addPathAsSelectable(grandchildPath);
    EXPECT_TRUE(selectablePaths.size() == 2);

    selectable.removePathAsSelectable(childPath);
    EXPECT_TRUE(selectablePaths.size() == 1);
  }
}
