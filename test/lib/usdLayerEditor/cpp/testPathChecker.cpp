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
#include <pxr/usd/usd/stage.h>

#include <QtCore/QString>
#include <QtWidgets/QApplication>

#include <ghc/fs_std.hpp>

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

// ── Cycle / alias detection (file-based layers) ───────────────────────────────

// Base fixture that wires up two real .usda files (A and B) and registers a
// stage backed by A as the active session entry.  Subclasses control exactly
// which sublayer relationships exist on disk.
class PathCheckerFileTestBase : public LayerEditorTestFixture
{
protected:
    void SetUp() override
    {
        LayerEditorTestFixture::SetUp();

        namespace fss = fs::filesystem;
        std::error_code ec;

        _pathA = (fss::temp_directory_path() / "pc_test_a.usda").generic_string();
        _pathB = (fss::temp_directory_path() / "pc_test_b.usda").generic_string();
        fss::remove(_pathA, ec);
        fss::remove(_pathB, ec);

        createFiles();

        _stage = PXR_NS::UsdStage::Open(_pathA);
        ASSERT_TRUE(_stage);
        _sessionState.addStage(_stage);
        _sessionState.setStageEntry(_sessionState.allStages().back());
        QApplication::processEvents();
    }

    // Subclasses create _layerA and _layerB with desired relationships.
    virtual void createFiles() = 0;

    void TearDown() override
    {
        _stage.Reset();
        _layerA.Reset();
        _layerB.Reset();

        namespace fss = fs::filesystem;
        std::error_code ec;
        fss::remove(_pathA, ec);
        fss::remove(_pathB, ec);

        LayerEditorTestFixture::TearDown();
    }

    std::string             _pathA;
    std::string             _pathB;
    SdfLayerRefPtr          _layerA;
    SdfLayerRefPtr          _layerB;
    PXR_NS::UsdStageRefPtr  _stage;
};

// A sublayers B via relative path; passing B's absolute path must be rejected
// via the handle-comparison loop (foundLayerInStack = true).
class PathCheckerAliasTest : public PathCheckerFileTestBase
{
protected:
    void createFiles() override
    {
        _layerB = SdfLayer::CreateNew(_pathB);
        ASSERT_TRUE(_layerB);
        _layerB->Save();

        // Use a relative path so the stack string differs from _pathB.
        _layerA = SdfLayer::CreateNew(_pathA);
        ASSERT_TRUE(_layerA);
        _layerA->InsertSubLayerPath("./pc_test_b.usda", 0);
        _layerA->Save();
    }
};

TEST_F(PathCheckerAliasTest, AliasPathRejected)
{
    // A's stack stores B as "./pc_test_b.usda"; proxy.Find(_pathB) == -1.
    // FindOrOpen(_pathB) returns the same handle, so the handle-comparison
    // loop sets foundLayerInStack=true and the function returns false.
    auto* parentItem = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(parentItem, nullptr);

    EXPECT_FALSE(checkIfPathIsSafeToAdd(nullptr, QString("test"), parentItem, _pathB));
    EXPECT_GE(_modalDialogCount, 1);
}

// A has no sublayers; B has A as a sublayer, creating a potential cycle.
// Adding B to A must be rejected by checkPathRecursive.
class PathCheckerCycleTest : public PathCheckerFileTestBase
{
protected:
    void createFiles() override
    {
        // B sublayers A — adding B to A creates the cycle A → B → A.
        _layerA = SdfLayer::CreateNew(_pathA);
        ASSERT_TRUE(_layerA);
        _layerA->Save();

        _layerB = SdfLayer::CreateNew(_pathB);
        ASSERT_TRUE(_layerB);
        _layerB->InsertSubLayerPath(_pathA, 0);
        _layerB->Save();
    }
};

TEST_F(PathCheckerCycleTest, CycleDetected)
{
    // A has no sublayers, so proxy.Find(_pathB) == -1.
    // FindOrOpen(_pathB) succeeds; the handle-comparison loop finds nothing
    // (foundLayerInStack=false).  checkPathRecursive walks B's children,
    // encounters A which is in parentHandles, and returns false.
    auto* parentItem = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(parentItem, nullptr);

    EXPECT_FALSE(checkIfPathIsSafeToAdd(nullptr, QString("test"), parentItem, _pathB));
    EXPECT_GE(_modalDialogCount, 1);
}

} // namespace UsdLayerEditor
