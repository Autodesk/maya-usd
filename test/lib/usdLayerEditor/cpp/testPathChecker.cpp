// Copyright 2026 Autodesk
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
#include "testFixture.h"
#include "pathChecker.h"
#include "layerTreeItem.h"

#include <pxr/usd/sdf/layer.h>

#include <QtCore/QString>

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

namespace {
LayerTreeItem* itemAt(LayerTreeModel* model, const QModelIndex& idx)
{
    return dynamic_cast<LayerTreeItem*>(model->itemFromIndex(idx));
}
} // namespace

class PathCheckerTest : public LayerEditorTestFixture { };

// ── checkIfPathIsSafeToAdd ────────────────────────────────────────────────────

TEST_F(PathCheckerTest, NullParentItemAlwaysTrue)
{
    // No parent item → always safe (early return in implementation).
    EXPECT_TRUE(checkIfPathIsSafeToAdd(nullptr, QString(), nullptr, "any/path.usda"));
}

TEST_F(PathCheckerTest, NonExistentPathIsSafe)
{
    // A path that can't be opened by FindOrOpen is assumed safe
    // (could be a custom URI or future path).
    auto* parentItem = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(parentItem, nullptr);
    EXPECT_TRUE(checkIfPathIsSafeToAdd(
        nullptr, QString("test"), parentItem, "/does/not/exist/layer.usda"));
}

TEST_F(PathCheckerTest, DuplicatePathInStackIsFalse)
{
    // The fixture root layer already has the first sublayer in its stack.
    // Trying to add the same identifier again must be rejected.
    auto* parentItem    = itemAt(treeModel(), rootLayerIndex());
    auto* sublayerItem  = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(parentItem,   nullptr);
    ASSERT_NE(sublayerItem, nullptr);

    // Modal warning is suppressed by the fixture's dialog handler.
    const std::string existingId = sublayerItem->layer()->GetIdentifier();
    EXPECT_FALSE(checkIfPathIsSafeToAdd(nullptr, QString("test"), parentItem, existingId));
}

} // namespace UsdLayerEditor
