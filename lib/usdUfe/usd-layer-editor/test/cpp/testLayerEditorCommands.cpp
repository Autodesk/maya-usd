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

#include "testUtils.h"
#include "LayerEditorCommands.h"
#include "layerLocking.h"
#include "layerMuting.h"
#include "utilUI.h"

#include <usdUfe/ufe/Utils.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

namespace {

// Stub stage-path accessor: returns an empty UFE path for any stage.
// This is sufficient for tests that don't inspect the path value.
Ufe::Path stubStagePathAccessor(PXR_NS::UsdStageWeakPtr /*stage*/)
{
    return Ufe::Path();
}

} // namespace

TEST(LayerEditorCommandsSmokeTest, HeaderIncludesCompile)
{
    SUCCEED();
}

class UpdateEditTargetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Register a no-op stage-path accessor so that MuteLayerCmd::saveSelection()
        // can call UsdUfe::stagePath() without crashing in this test context.
        UsdUfe::setStagePathAccessorFn(stubStagePathAccessor);

        // Initialize the UFE global selection singleton if not already done.
        // MuteLayerCmd::saveSelection() calls Ufe::GlobalSelection::get().
        if (!Ufe::GlobalSelection::get()) {
            Ufe::GlobalSelection::initializeInstance(
                std::make_shared<Ufe::ObservableSelection>());
        }

        forgetLockedLayers();
        _stage    = PXR_NS::UsdStage::CreateInMemory();
        _subLayer = PXR_NS::SdfLayer::CreateAnonymous("sub");
        _stage->GetRootLayer()->InsertSubLayerPath(_subLayer->GetIdentifier(), 0);
    }

    void TearDown() override
    {
        BaseCmd::setAutoRetargetDisabledChecker(nullptr);
        forgetLockedLayers();
    }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _subLayer;
};

// When all layers are non-modifiable, updateEditTarget should switch to session layer.
TEST_F(UpdateEditTargetTest, WhenNoModifiableLayers_EditTargetChangesToSessionLayer)
{
    // Lock all non-session layers so nothing is modifiable.
    lockLayer("", _stage->GetRootLayer(), LayerLock_Locked, /*updateDCCAttr=*/false);
    lockLayer("", _subLayer,              LayerLock_Locked, /*updateDCCAttr=*/false);
    _stage->SetEditTarget(_stage->GetRootLayer());

    // Mute the sublayer — this calls updateEditTarget() internally.
    auto cmd = std::make_shared<MuteLayerCmd>(_stage, _subLayer, /*muteIt=*/true);
    cmd->execute();

    EXPECT_EQ(_stage->GetEditTarget().GetLayer(), _stage->GetSessionLayer());
}

// When the checker returns true, updateEditTarget should be suppressed entirely.
TEST_F(UpdateEditTargetTest, WhenCheckerDisablesAutoRetarget_EditTargetUnchanged)
{
    BaseCmd::setAutoRetargetDisabledChecker([] { return true; });

    lockLayer("", _stage->GetRootLayer(), LayerLock_Locked, false);
    lockLayer("", _subLayer,              LayerLock_Locked, false);
    _stage->SetEditTarget(_stage->GetRootLayer());

    auto cmd = std::make_shared<MuteLayerCmd>(_stage, _subLayer, /*muteIt=*/true);
    cmd->execute();

    // Must NOT have changed to session layer.
    EXPECT_NE(_stage->GetEditTarget().GetLayer(), _stage->GetSessionLayer());
}

class BackupEditTargetsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Build: root -> A -> B.  Edit target = B.
        _stage  = PXR_NS::UsdStage::CreateInMemory();
        _layerA = PXR_NS::SdfLayer::CreateAnonymous("A");
        _layerB = PXR_NS::SdfLayer::CreateAnonymous("B");
        _stage->GetRootLayer()->InsertSubLayerPath(_layerA->GetIdentifier(), 0);
        _layerA->InsertSubLayerPath(_layerB->GetIdentifier(), 0);
        _stage->SetEditTarget(_layerB);
    }

    void TearDown() override
    {
        BackupLayerBaseCmd::setStagesProvider(nullptr);
    }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _layerA;
    PXR_NS::SdfLayerRefPtr _layerB;
};

// Clearing A removes B from the graph (it was A's sublayer).
// Without the provider the in-memory stage is not in the global cache → backup is skipped.
// USD retains the stale edit target reference (B) throughout — no reset, no restore.
TEST_F(BackupEditTargetsTest, WithoutProvider_EditTargetNotRestoredOnUndo)
{
    // _stage is NOT in UsdUtilsStageCache — backupEditTargets won't find it.
    auto cmd = std::make_shared<ClearLayerCmd>(_layerA);
    cmd->execute();
    // B is gone from the graph; USD retains the stale edit target reference.
    // Undo: restores A's content (B is back) but the edit target was never backed up.
    cmd->undo();
    // Without the provider, no backup/restore cycle occurred; USD simply kept the stale
    // edit target pointing at B throughout (never reset, never restored via backup).
    // This confirms backupEditTargets was a no-op for this stage.
    EXPECT_EQ(_stage->GetEditTarget().GetLayer(), _layerB);
}

// With the provider registered, backupEditTargets finds _stage → saves B → restores on undo.
TEST_F(BackupEditTargetsTest, WithProvider_EditTargetRestoredOnUndo)
{
    BackupLayerBaseCmd::setStagesProvider([this]() -> std::vector<PXR_NS::UsdStageRefPtr> {
        return { _stage };
    });

    auto cmd = std::make_shared<ClearLayerCmd>(_layerA);
    cmd->execute();
    // At this point edit target was backed up and reset to root.
    EXPECT_NE(_stage->GetEditTarget().GetLayer(), _layerB);

    cmd->undo();
    // Undo restored A's content (B is back) and restored the edit target to B.
    EXPECT_EQ(_stage->GetEditTarget().GetLayer(), _layerB);
}

class ReplaceSubPathCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Register a no-op error display callback so that UIUtils::displayError
        // does not throw std::bad_function_call when doIt() reports an error.
        UIUtils::setErrorDisplayCallbackFunction([](std::string) {});

        _parent = PXR_NS::SdfLayer::CreateAnonymous("parent");
        _layerA = PXR_NS::SdfLayer::CreateAnonymous("A");
        _layerB = PXR_NS::SdfLayer::CreateAnonymous("B");
        _parent->InsertSubLayerPath(_layerA->GetIdentifier(), 0);
    }

    void TearDown() override
    {
        // Reset the error display callback to avoid leaking state into other tests.
        UIUtils::setErrorDisplayCallbackFunction(nullptr);
    }

    PXR_NS::SdfLayerRefPtr _parent;
    PXR_NS::SdfLayerRefPtr _layerA;
    PXR_NS::SdfLayerRefPtr _layerB;
};

TEST_F(ReplaceSubPathCmdTest, DoIt_ReplacesOldPathWithNewPath)
{
    auto cmd = std::make_shared<ReplaceSubPathCmd>(
        _parent, _layerA->GetIdentifier(), _layerB->GetIdentifier());
    cmd->execute();

    const auto& paths = _parent->GetSubLayerPaths();
    EXPECT_EQ(paths.Find(_layerA->GetIdentifier()), static_cast<size_t>(-1));
    EXPECT_NE(paths.Find(_layerB->GetIdentifier()), static_cast<size_t>(-1));
}

TEST_F(ReplaceSubPathCmdTest, UndoIt_RestoresOldPath)
{
    auto cmd = std::make_shared<ReplaceSubPathCmd>(
        _parent, _layerA->GetIdentifier(), _layerB->GetIdentifier());
    cmd->execute();
    cmd->undo();

    const auto& paths = _parent->GetSubLayerPaths();
    EXPECT_NE(paths.Find(_layerA->GetIdentifier()), static_cast<size_t>(-1));
    EXPECT_EQ(paths.Find(_layerB->GetIdentifier()), static_cast<size_t>(-1));
}

TEST_F(ReplaceSubPathCmdTest, DoIt_ReturnsFalse_WhenOldPathNotFound)
{
    auto cmd = std::make_shared<ReplaceSubPathCmd>(
        _parent, "nonexistent.usda", _layerB->GetIdentifier());
    // doIt() returns false when the old path is not found, so redo() throws.
    EXPECT_THROW(cmd->execute(), std::runtime_error);

    // Nothing should have changed.
    const auto& paths = _parent->GetSubLayerPaths();
    EXPECT_NE(paths.Find(_layerA->GetIdentifier()), static_cast<size_t>(-1));
}

// ============================================================================
// Task 5: DiscardEditCmd and ClearLayerCmd
// ============================================================================

class BackupLayerCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        BackupLayerBaseCmd::setStagesProvider([this]() -> std::vector<PXR_NS::UsdStageRefPtr> {
            return { _stage };
        });
        _stage  = PXR_NS::UsdStage::CreateInMemory();
        _layer  = PXR_NS::SdfLayer::CreateAnonymous("target");
        _stage->GetRootLayer()->InsertSubLayerPath(_layer->GetIdentifier(), 0);
        // Write something to the layer so it is dirty.
        _layer->SetComment("original content");
    }

    void TearDown() override
    {
        BackupLayerBaseCmd::setStagesProvider(nullptr);
    }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _layer;
};

TEST_F(BackupLayerCmdTest, DiscardEditCmd_DoIt_ClearsLayerContent)
{
    auto cmd = std::make_shared<DiscardEditCmd>(_layer);
    cmd->execute();
    EXPECT_TRUE(_layer->GetComment().empty());
}

TEST_F(BackupLayerCmdTest, DiscardEditCmd_Undo_RestoresLayerContent)
{
    auto cmd = std::make_shared<DiscardEditCmd>(_layer);
    cmd->execute();
    cmd->undo();
    EXPECT_EQ(_layer->GetComment(), "original content");
}

TEST_F(BackupLayerCmdTest, ClearLayerCmd_DoIt_EmptiesLayer)
{
    auto cmd = std::make_shared<ClearLayerCmd>(_layer);
    cmd->execute();
    EXPECT_TRUE(_layer->GetComment().empty());
}

TEST_F(BackupLayerCmdTest, ClearLayerCmd_Undo_RestoresContent)
{
    auto cmd = std::make_shared<ClearLayerCmd>(_layer);
    cmd->execute();
    cmd->undo();
    EXPECT_EQ(_layer->GetComment(), "original content");
}

// ============================================================================
// Task 6: MuteLayerCmd
// ============================================================================

class MuteLayerCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        UsdUfe::setStagePathAccessorFn(stubStagePathAccessor);
        if (!Ufe::GlobalSelection::get()) {
            Ufe::GlobalSelection::initializeInstance(
                std::make_shared<Ufe::ObservableSelection>());
        }
        forgetMutedLayers();
        _stage  = PXR_NS::UsdStage::CreateInMemory();
        _layer  = PXR_NS::SdfLayer::CreateAnonymous("mutable");
        _stage->GetRootLayer()->InsertSubLayerPath(_layer->GetIdentifier(), 0);
    }
    void TearDown() override { forgetMutedLayers(); }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _layer;
};

TEST_F(MuteLayerCmdTest, DoIt_MutesLayer)
{
    auto cmd = std::make_shared<MuteLayerCmd>(_stage, _layer, /*muteIt=*/true);
    cmd->execute();
    EXPECT_TRUE(_stage->IsLayerMuted(_layer->GetIdentifier()));
}

TEST_F(MuteLayerCmdTest, Undo_UnmutesLayer)
{
    auto cmd = std::make_shared<MuteLayerCmd>(_stage, _layer, /*muteIt=*/true);
    cmd->execute();
    cmd->undo();
    EXPECT_FALSE(_stage->IsLayerMuted(_layer->GetIdentifier()));
}

TEST_F(MuteLayerCmdTest, DoIt_Unmute_UnmutesAlreadyMutedLayer)
{
    _stage->MuteLayer(_layer->GetIdentifier());
    auto cmd = std::make_shared<MuteLayerCmd>(_stage, _layer, /*muteIt=*/false);
    cmd->execute();
    EXPECT_FALSE(_stage->IsLayerMuted(_layer->GetIdentifier()));
}

TEST_F(MuteLayerCmdTest, Undo_Unmute_RestoresMutedState)
{
    _stage->MuteLayer(_layer->GetIdentifier());
    auto cmd = std::make_shared<MuteLayerCmd>(_stage, _layer, /*muteIt=*/false);
    cmd->execute();
    cmd->undo();
    EXPECT_TRUE(_stage->IsLayerMuted(_layer->GetIdentifier()));
}

} // namespace UsdLayerEditor
