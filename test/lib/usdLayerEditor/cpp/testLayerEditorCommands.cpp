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
#include "layerEditorDCCFunctions.h"

#include <usdUfe/ufe/Utils.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>

#include <gtest/gtest.h>

#include <filesystem>

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
        // Register a no-op displayError so commands can report errors without crashing.
        { UsdLayerEditor::ComponentFns c; c.displayError = [](const std::string&){}; UsdLayerEditor::setComponentFns(c); }

        _parent = PXR_NS::SdfLayer::CreateAnonymous("parent");
        _layerA = PXR_NS::SdfLayer::CreateAnonymous("A");
        _layerB = PXR_NS::SdfLayer::CreateAnonymous("B");
        _parent->InsertSubLayerPath(_layerA->GetIdentifier(), 0);
    }

    void TearDown() override
    {
        // Reset the error display callback to avoid leaking state into other tests.
        UsdLayerEditor::setComponentFns(UsdLayerEditor::ComponentFns{});
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

    cmd->redo();
    // Re-fetch the proxy: the one captured above does not reflect the redo's mutation.
    const auto& pathsAfterRedo = _parent->GetSubLayerPaths();
    EXPECT_EQ(pathsAfterRedo.Find(_layerA->GetIdentifier()), static_cast<size_t>(-1));
    EXPECT_NE(pathsAfterRedo.Find(_layerB->GetIdentifier()), static_cast<size_t>(-1));
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
    cmd->redo();
    EXPECT_TRUE(_layer->GetComment().empty());
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
    cmd->redo();
    EXPECT_TRUE(_layer->GetComment().empty());
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
    cmd->redo();
    EXPECT_TRUE(_stage->IsLayerMuted(_layer->GetIdentifier()));
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
    cmd->redo();
    EXPECT_FALSE(_stage->IsLayerMuted(_layer->GetIdentifier()));
}

// MuteLayerCmd must hold the layer before muting it (OpenUSD lets go of muted
// layers), so the authored content of a dirty anonymous sublayer survives a
// mute / unmute / undo / redo cycle.
TEST_F(MuteLayerCmdTest, MuteUnmuteUndoRedo_PreservesDirtyLayerContent)
{
    const PXR_NS::SdfPath fooPath("/Foo");
    PXR_NS::SdfPrimSpec::New(_layer, "Foo", PXR_NS::SdfSpecifierDef);
    ASSERT_TRUE(_layer->GetPrimAtPath(fooPath));

    auto cmd = std::make_shared<MuteLayerCmd>(_stage, _layer, /*muteIt=*/true);

    cmd->execute(); // hold-before-mute, then mute
    EXPECT_TRUE(_stage->IsLayerMuted(_layer->GetIdentifier()));
    EXPECT_TRUE(_layer->GetPrimAtPath(fooPath)) << "muted dirty layer content must be retained";

    cmd->undo(); // unmute, then release
    EXPECT_FALSE(_stage->IsLayerMuted(_layer->GetIdentifier()));
    EXPECT_TRUE(_layer->GetPrimAtPath(fooPath));

    cmd->redo(); // re-hold, then re-mute
    EXPECT_TRUE(_stage->IsLayerMuted(_layer->GetIdentifier()));
    EXPECT_TRUE(_layer->GetPrimAtPath(fooPath)) << "content must survive a re-mute on redo";
}

// ============================================================================
// Task 7: LockLayerCmd
// ============================================================================

class LockLayerCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        UsdUfe::setStagePathAccessorFn(stubStagePathAccessor);
        forgetLockedLayers();
        _stage = PXR_NS::UsdStage::CreateInMemory();
        _layer = PXR_NS::SdfLayer::CreateAnonymous("lockable");
        _stage->GetRootLayer()->InsertSubLayerPath(_layer->GetIdentifier(), 0);
    }
    void TearDown() override { forgetLockedLayers(); }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _layer;
};

TEST_F(LockLayerCmdTest, DoIt_LocksLayer)
{
    auto cmd = std::make_shared<LockLayerCmd>(_stage, _layer, LayerLock_Locked);
    cmd->execute();
    EXPECT_TRUE(isLayerLocked(_layer));
}

TEST_F(LockLayerCmdTest, Undo_UnlocksLayer)
{
    auto cmd = std::make_shared<LockLayerCmd>(_stage, _layer, LayerLock_Locked);
    cmd->execute();
    cmd->undo();
    EXPECT_FALSE(isLayerLocked(_layer));
    cmd->redo();
    EXPECT_TRUE(isLayerLocked(_layer));
}

TEST_F(LockLayerCmdTest, SkipSystemLocked_DoesNotLockSystemLockedSublayers)
{
    auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sub");
    _layer->InsertSubLayerPath(sublayer->GetIdentifier(), 0);
    // Mark sublayer as system-locked.
    lockLayer("", sublayer, LayerLock_SystemLocked, /*updateDCCAttr=*/false);

    auto cmd = std::make_shared<LockLayerCmd>(
        _stage, _layer, LayerLock_Locked,
        /*includeSubLayers=*/true, /*skipSystemLocked=*/true);
    cmd->execute();

    // Parent should be locked, system-locked sublayer should remain system-locked (not relocked).
    EXPECT_TRUE(isLayerLocked(_layer));
    EXPECT_TRUE(isLayerSystemLocked(sublayer));
    EXPECT_FALSE(isLayerLocked(sublayer)); // must not have been changed to plain locked
}

// ============================================================================
// Task 8: InsertSubPathCmd, RemoveSubPathCmd, AddAnonSubLayerCmd
// ============================================================================

class InsertSubPathCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _stage  = PXR_NS::UsdStage::CreateInMemory();
        _parent = _stage->GetRootLayer();
        _sub    = PXR_NS::SdfLayer::CreateAnonymous("sub");
    }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _parent;
    PXR_NS::SdfLayerRefPtr _sub;
};

TEST_F(InsertSubPathCmdTest, DoIt_InsertsSubLayerAtIndex)
{
    auto cmd = std::make_shared<InsertSubPathCmd>(
        _stage, _parent, _sub->GetIdentifier(), 0);
    cmd->execute();
    EXPECT_NE(_parent->GetSubLayerPaths().Find(_sub->GetIdentifier()), static_cast<size_t>(-1));
}

TEST_F(InsertSubPathCmdTest, Undo_RemovesInsertedSubLayer)
{
    auto cmd = std::make_shared<InsertSubPathCmd>(
        _stage, _parent, _sub->GetIdentifier(), 0);
    cmd->execute();
    cmd->undo();
    EXPECT_EQ(_parent->GetSubLayerPaths().Find(_sub->GetIdentifier()), static_cast<size_t>(-1));
    cmd->redo();
    EXPECT_NE(_parent->GetSubLayerPaths().Find(_sub->GetIdentifier()), static_cast<size_t>(-1));
}

class RemoveSubPathCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        UsdUfe::setStagePathAccessorFn(stubStagePathAccessor);
        if (!Ufe::GlobalSelection::get()) {
            Ufe::GlobalSelection::initializeInstance(
                std::make_shared<Ufe::ObservableSelection>());
        }
        _stage  = PXR_NS::UsdStage::CreateInMemory();
        _parent = _stage->GetRootLayer();
        _sub    = PXR_NS::SdfLayer::CreateAnonymous("sub");
        _parent->InsertSubLayerPath(_sub->GetIdentifier(), 0);
    }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _parent;
    PXR_NS::SdfLayerRefPtr _sub;
};

TEST_F(RemoveSubPathCmdTest, DoIt_RemovesSubLayer)
{
    auto cmd = std::make_shared<RemoveSubPathCmd>(_stage, _parent, 0);
    cmd->execute();
    EXPECT_EQ(_parent->GetSubLayerPaths().Find(_sub->GetIdentifier()), static_cast<size_t>(-1));
}

TEST_F(RemoveSubPathCmdTest, Undo_RestoresSubLayer)
{
    auto cmd = std::make_shared<RemoveSubPathCmd>(_stage, _parent, 0);
    cmd->execute();
    cmd->undo();
    EXPECT_NE(_parent->GetSubLayerPaths().Find(_sub->GetIdentifier()), static_cast<size_t>(-1));
    cmd->redo();
    EXPECT_EQ(_parent->GetSubLayerPaths().Find(_sub->GetIdentifier()), static_cast<size_t>(-1));
}

class AddAnonSubLayerCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _stage  = PXR_NS::UsdStage::CreateInMemory();
        _parent = _stage->GetRootLayer();
    }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _parent;
};

TEST_F(AddAnonSubLayerCmdTest, DoIt_InsertsAnonLayer)
{
    auto cmd = std::make_shared<AddAnonSubLayerCmd>(_stage, _parent);
    cmd->_anonName = "myLayer";
    cmd->execute();
    EXPECT_EQ(_parent->GetNumSubLayerPaths(), static_cast<size_t>(1));
}

TEST_F(AddAnonSubLayerCmdTest, DoIt_ReturnsNonEmptyIdentifier)
{
    auto cmd = std::make_shared<AddAnonSubLayerCmd>(_stage, _parent);
    cmd->_anonName = "myLayer";
    cmd->execute();
    EXPECT_FALSE(cmd->addedLayer().empty());
}

TEST_F(AddAnonSubLayerCmdTest, Undo_RemovesAnonLayer)
{
    auto cmd = std::make_shared<AddAnonSubLayerCmd>(_stage, _parent);
    cmd->_anonName = "myLayer";
    cmd->execute();
    cmd->undo();
    EXPECT_EQ(_parent->GetNumSubLayerPaths(), static_cast<size_t>(0));
}

class MoveSubPathCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _stage  = PXR_NS::UsdStage::CreateInMemory();
        _parent = _stage->GetRootLayer();
        _subA   = PXR_NS::SdfLayer::CreateAnonymous("subA");
        _subB   = PXR_NS::SdfLayer::CreateAnonymous("subB");
        _subC   = PXR_NS::SdfLayer::CreateAnonymous("subC");
        _parent->InsertSubLayerPath(_subA->GetIdentifier(), 0);
        _parent->InsertSubLayerPath(_subB->GetIdentifier(), 1);
        _parent->InsertSubLayerPath(_subC->GetIdentifier(), 2);
        // _parent sublayers: [A, B, C] at indices 0, 1, 2
    }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _parent;
    PXR_NS::SdfLayerRefPtr _subA, _subB, _subC;
};

TEST_F(MoveSubPathCmdTest, DoIt_SameParent_ReordersSubLayer)
{
    // Move A from index 0 to index 2 → expect [B, C, A]
    auto cmd = std::make_shared<MoveSubPathCmd>(_parent, _parent, _subA->GetIdentifier(), 2);
    cmd->execute();
    EXPECT_EQ(_parent->GetSubLayerPaths()[2], _subA->GetIdentifier());
    EXPECT_EQ(_parent->GetSubLayerPaths()[0], _subB->GetIdentifier());
}

TEST_F(MoveSubPathCmdTest, Undo_SameParent_RestoresOriginalOrder)
{
    auto cmd = std::make_shared<MoveSubPathCmd>(_parent, _parent, _subA->GetIdentifier(), 2);
    cmd->execute();
    cmd->undo();
    EXPECT_EQ(_parent->GetSubLayerPaths()[0], _subA->GetIdentifier());
    EXPECT_EQ(_parent->GetSubLayerPaths()[1], _subB->GetIdentifier());
    EXPECT_EQ(_parent->GetSubLayerPaths()[2], _subC->GetIdentifier());
    cmd->redo();
    EXPECT_EQ(_parent->GetSubLayerPaths()[0], _subB->GetIdentifier());
    EXPECT_EQ(_parent->GetSubLayerPaths()[2], _subA->GetIdentifier());
}

TEST_F(MoveSubPathCmdTest, DoIt_CrossParent_MovesSubLayerToNewParent)
{
    auto newParent = PXR_NS::SdfLayer::CreateAnonymous("newParent");
    auto cmd = std::make_shared<MoveSubPathCmd>(
        _parent, newParent, _subA->GetIdentifier(), 0);
    cmd->execute();
    EXPECT_EQ(
        _parent->GetSubLayerPaths().Find(_subA->GetIdentifier()), static_cast<size_t>(-1));
    EXPECT_NE(
        newParent->GetSubLayerPaths().Find(_subA->GetIdentifier()), static_cast<size_t>(-1));
}

TEST_F(MoveSubPathCmdTest, Undo_CrossParent_RestoresSubLayerToOriginalParent)
{
    auto newParent = PXR_NS::SdfLayer::CreateAnonymous("newParent");
    auto cmd = std::make_shared<MoveSubPathCmd>(
        _parent, newParent, _subA->GetIdentifier(), 0);
    cmd->execute();
    cmd->undo();
    EXPECT_NE(
        _parent->GetSubLayerPaths().Find(_subA->GetIdentifier()), static_cast<size_t>(-1));
    EXPECT_EQ(
        newParent->GetSubLayerPaths().Find(_subA->GetIdentifier()), static_cast<size_t>(-1));

    cmd->redo();
    EXPECT_EQ(
        _parent->GetSubLayerPaths().Find(_subA->GetIdentifier()), static_cast<size_t>(-1));
    EXPECT_NE(
        newParent->GetSubLayerPaths().Find(_subA->GetIdentifier()), static_cast<size_t>(-1));
}

TEST(RefreshSystemLockCallbackContextTest, AddCallbackContext_StoresEntry)
{
    auto stage     = PXR_NS::UsdStage::CreateInMemory();
    auto rootLayer = stage->GetRootLayer();
    auto cmd = std::make_shared<RefreshSystemLockLayerCmd>(stage, rootLayer, false);
    cmd->addCallbackContext("proxyShapePath", PXR_NS::VtValue(std::string("/myShape")));
    ASSERT_NE(
        cmd->_extraCallbackContext.find("proxyShapePath"),
        cmd->_extraCallbackContext.end());
    EXPECT_EQ(
        cmd->_extraCallbackContext["proxyShapePath"].UncheckedGet<std::string>(),
        std::string("/myShape"));
}

// ============================================================================
// Task A-special: AddAnonSubLayerCmd redo reuses the same identifier
// ============================================================================

TEST_F(AddAnonSubLayerCmdTest, Redo_ReusesSameIdentifier)
{
    auto cmd = std::make_shared<AddAnonSubLayerCmd>(_stage, _parent);
    cmd->_anonName = "myLayer";
    cmd->execute();
    const std::string id1 = cmd->addedLayer();
    cmd->undo();
    cmd->redo();
    const std::string id2 = cmd->addedLayer();
    // The command intentionally caches the anonymous identifier so later commands
    // referencing it stay valid across undo/redo.
    EXPECT_EQ(id1, id2);
    EXPECT_EQ(_parent->GetSubLayerPaths()[0], id2);
}

// ============================================================================
// Task B: SetEditTargetCmd
// ============================================================================

class SetEditTargetCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _stage = PXR_NS::UsdStage::CreateInMemory();
        _sub   = PXR_NS::SdfLayer::CreateAnonymous("sub");
        _stage->GetRootLayer()->InsertSubLayerPath(_sub->GetIdentifier(), 0);
        _stage->SetEditTarget(_stage->GetRootLayer());
    }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _sub;
};

TEST_F(SetEditTargetCmdTest, DoIt_SetsTarget)
{
    auto cmd = std::make_shared<SetEditTargetCmd>(_stage, _sub);
    cmd->execute();
    EXPECT_EQ(_stage->GetEditTarget().GetLayer(), _sub);
}

TEST_F(SetEditTargetCmdTest, Undo_RestoresPreviousTarget)
{
    auto cmd = std::make_shared<SetEditTargetCmd>(_stage, _sub);
    cmd->execute();
    cmd->undo();
    EXPECT_EQ(_stage->GetEditTarget().GetLayer(), _stage->GetRootLayer());
}

TEST_F(SetEditTargetCmdTest, Redo_ReappliesTarget)
{
    auto cmd = std::make_shared<SetEditTargetCmd>(_stage, _sub);
    cmd->execute();
    cmd->undo();
    cmd->redo();
    EXPECT_EQ(_stage->GetEditTarget().GetLayer(), _sub);
}

// ============================================================================
// Task C: FlattenLayerCmd
// ============================================================================

class FlattenLayerCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        { UsdLayerEditor::ComponentFns c; c.displayError = [](const std::string&){}; UsdLayerEditor::setComponentFns(c); }
        _root = PXR_NS::SdfLayer::CreateAnonymous("flattenRoot");
        _sub  = PXR_NS::SdfLayer::CreateAnonymous("flattenSub");
        _root->InsertSubLayerPath(_sub->GetIdentifier(), 0);
        // Define a prim only in the sublayer; flattening must inline it into _root.
        PXR_NS::SdfPrimSpec::New(_sub, "Foo", PXR_NS::SdfSpecifierDef);
    }

    void TearDown() override { UsdLayerEditor::setComponentFns(UsdLayerEditor::ComponentFns{}); }

    PXR_NS::SdfLayerRefPtr _root;
    PXR_NS::SdfLayerRefPtr _sub;
};

TEST_F(FlattenLayerCmdTest, DoIt_FlattensSublayerContentIntoLayer)
{
    ASSERT_FALSE(_root->GetPrimAtPath(PXR_NS::SdfPath("/Foo")));
    auto cmd = std::make_shared<FlattenLayerCmd>(_root);
    cmd->execute();
    EXPECT_TRUE(_root->GetPrimAtPath(PXR_NS::SdfPath("/Foo")));
}

TEST_F(FlattenLayerCmdTest, Undo_RestoresPreFlattenContent)
{
    auto cmd = std::make_shared<FlattenLayerCmd>(_root);
    cmd->execute();
    cmd->undo();
    EXPECT_FALSE(_root->GetPrimAtPath(PXR_NS::SdfPath("/Foo")));
}

TEST_F(FlattenLayerCmdTest, Redo_ReflattensContent)
{
    auto cmd = std::make_shared<FlattenLayerCmd>(_root);
    cmd->execute();
    cmd->undo();
    cmd->redo();
    EXPECT_TRUE(_root->GetPrimAtPath(PXR_NS::SdfPath("/Foo")));
}

// ============================================================================
// Task D: StitchLayersCmd
// ============================================================================

class StitchLayersCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _stage  = PXR_NS::UsdStage::CreateInMemory();
        _strong = PXR_NS::SdfLayer::CreateAnonymous("strong");
        _weak   = PXR_NS::SdfLayer::CreateAnonymous("weak");
        // Index 0 is the strongest sublayer.
        _stage->GetRootLayer()->InsertSubLayerPath(_strong->GetIdentifier(), 0);
        _stage->GetRootLayer()->InsertSubLayerPath(_weak->GetIdentifier(), 1);
        PXR_NS::SdfPrimSpec::New(_strong, "Strong", PXR_NS::SdfSpecifierDef);
        PXR_NS::SdfPrimSpec::New(_weak, "Weak", PXR_NS::SdfSpecifierDef);
    }

    std::vector<std::string> identifiers() const
    {
        return { _strong->GetIdentifier(), _weak->GetIdentifier() };
    }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _strong;
    PXR_NS::SdfLayerRefPtr _weak;
};

TEST_F(StitchLayersCmdTest, DoIt_MergesWeakIntoStrongAndRemovesWeak)
{
    auto cmd = std::make_shared<StitchLayersCmd>(_stage, identifiers());
    cmd->execute();

    EXPECT_TRUE(_strong->GetPrimAtPath(PXR_NS::SdfPath("/Weak")));
    EXPECT_EQ(
        _stage->GetRootLayer()->GetSubLayerPaths().Find(_weak->GetIdentifier()),
        static_cast<size_t>(-1));
    EXPECT_NE(
        _stage->GetRootLayer()->GetSubLayerPaths().Find(_strong->GetIdentifier()),
        static_cast<size_t>(-1));
}

TEST_F(StitchLayersCmdTest, Undo_RestoresOriginalLayers)
{
    auto cmd = std::make_shared<StitchLayersCmd>(_stage, identifiers());
    cmd->execute();
    cmd->undo();

    EXPECT_NE(
        _stage->GetRootLayer()->GetSubLayerPaths().Find(_weak->GetIdentifier()),
        static_cast<size_t>(-1));
    EXPECT_FALSE(_strong->GetPrimAtPath(PXR_NS::SdfPath("/Weak")));
}

TEST_F(StitchLayersCmdTest, Redo_RestitchesLayers)
{
    auto cmd = std::make_shared<StitchLayersCmd>(_stage, identifiers());
    cmd->execute();
    cmd->undo();
    cmd->redo();

    EXPECT_TRUE(_strong->GetPrimAtPath(PXR_NS::SdfPath("/Weak")));
    EXPECT_EQ(
        _stage->GetRootLayer()->GetSubLayerPaths().Find(_weak->GetIdentifier()),
        static_cast<size_t>(-1));
}

TEST_F(StitchLayersCmdTest, DoIt_ReturnsFalse_WhenTargetLayerIsLocked)
{
    // Locking the merge target (strongest layer) aborts the entire operation.
    _strong->SetPermissionToEdit(false);
    auto cmd = std::make_shared<StitchLayersCmd>(_stage, identifiers());
    EXPECT_THROW(cmd->execute(), std::runtime_error);
    EXPECT_NE(
        _stage->GetRootLayer()->GetSubLayerPaths().Find(_weak->GetIdentifier()),
        static_cast<size_t>(-1));
    _strong->SetPermissionToEdit(true);
}

TEST(StitchLayersCmdPartialMergeTest, DoIt_SkipsWeakWithLockedParent_MergesRest)
{
    // Stack: root → layerA → layerB
    // Lock layerA so layerB's parent is locked.
    // Expected: layerA merges into root (layerA's parent root is unlocked);
    //           layerB is skipped (its parent layerA is locked).
    auto stage  = PXR_NS::UsdStage::CreateInMemory();
    auto root   = stage->GetRootLayer();
    auto layerA = PXR_NS::SdfLayer::CreateAnonymous("A");
    auto layerB = PXR_NS::SdfLayer::CreateAnonymous("B");

    root->InsertSubLayerPath(layerA->GetIdentifier(), 0);
    layerA->InsertSubLayerPath(layerB->GetIdentifier(), 0);

    PXR_NS::SdfPrimSpec::New(layerA, "FromA", PXR_NS::SdfSpecifierDef);
    PXR_NS::SdfPrimSpec::New(layerB, "FromB", PXR_NS::SdfSpecifierDef);

    layerA->SetPermissionToEdit(false);

    auto cmd = std::make_shared<StitchLayersCmd>(
        stage,
        std::vector<std::string> {
            root->GetIdentifier(),
            layerA->GetIdentifier(),
            layerB->GetIdentifier() });

    // Should succeed (partial merge, not a hard failure).
    EXPECT_NO_THROW(cmd->execute());

    // layerA's content was merged into root.
    EXPECT_TRUE(root->GetPrimAtPath(PXR_NS::SdfPath("/FromA")));
    // layerB was skipped — its content did not reach root.
    EXPECT_FALSE(root->GetPrimAtPath(PXR_NS::SdfPath("/FromB")));
    // layerA was removed from root's sublayers.
    EXPECT_EQ(
        root->GetSubLayerPaths().Find(layerA->GetIdentifier()),
        static_cast<size_t>(-1));
    // layerB is still a sublayer of layerA (unchanged).
    EXPECT_NE(
        layerA->GetSubLayerPaths().Find(layerB->GetIdentifier()),
        static_cast<size_t>(-1));
    // layerB was adopted by root as a sublayer (inherited from merged layerA).
    EXPECT_NE(
        root->GetSubLayerPaths().Find(layerB->GetIdentifier()),
        static_cast<size_t>(-1));

    layerA->SetPermissionToEdit(true);
}

// ============================================================================
// Task E: RefreshSystemLockLayerCmd (needs a real read-only file on disk)
// ============================================================================

class RefreshSystemLockLayerCmdTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        UsdUfe::setStagePathAccessorFn(stubStagePathAccessor);
        if (!Ufe::GlobalSelection::get()) {
            Ufe::GlobalSelection::initializeInstance(
                std::make_shared<Ufe::ObservableSelection>());
        }
        forgetLockedLayers();
        forgetSystemLockedLayers();

        namespace fs = std::filesystem;
        std::error_code ec;
        _filePath = (fs::temp_directory_path() / "le_systemlock_test.usda").generic_string();
        // Clear any leftover from a previous run (restore write first so remove succeeds).
        fs::permissions(_filePath, fs::perms::owner_write, fs::perm_options::add, ec);
        fs::remove(_filePath, ec);

        _stage     = PXR_NS::UsdStage::CreateInMemory();
        _fileLayer = PXR_NS::SdfLayer::CreateNew(_filePath);
        _fileLayer->Save();
        _stage->GetRootLayer()->InsertSubLayerPath(_fileLayer->GetIdentifier(), 0);

        // Make the file read-only so checkWriteAccess() reports no write access.
        fs::permissions(
            _filePath,
            fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write,
            fs::perm_options::remove);
    }

    void TearDown() override
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::permissions(_filePath, fs::perms::owner_write, fs::perm_options::add, ec);
        fs::remove(_filePath, ec);
        forgetSystemLockedLayers();
        forgetLockedLayers();
    }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _fileLayer;
    std::string            _filePath;
};

TEST_F(RefreshSystemLockLayerCmdTest, DoIt_SystemLocksReadOnlyFileLayer)
{
    ASSERT_FALSE(isLayerSystemLocked(_fileLayer));
    auto cmd = std::make_shared<RefreshSystemLockLayerCmd>(_stage, _fileLayer, false);
    cmd->execute();
    EXPECT_TRUE(isLayerSystemLocked(_fileLayer));
}

TEST_F(RefreshSystemLockLayerCmdTest, Undo_RestoresLockState)
{
    auto cmd = std::make_shared<RefreshSystemLockLayerCmd>(_stage, _fileLayer, false);
    cmd->execute();
    cmd->undo();
    EXPECT_FALSE(isLayerSystemLocked(_fileLayer));
}

TEST_F(RefreshSystemLockLayerCmdTest, Redo_ReappliesSystemLock)
{
    auto cmd = std::make_shared<RefreshSystemLockLayerCmd>(_stage, _fileLayer, false);
    cmd->execute();
    cmd->undo();
    cmd->redo();
    EXPECT_TRUE(isLayerSystemLocked(_fileLayer));
}

// ============================================================================
// Task F: error paths
// ============================================================================

TEST_F(InsertSubPathCmdTest, DoIt_ReturnsFalse_WhenIndexOutOfBounds)
{
    { UsdLayerEditor::ComponentFns c; c.displayError = [](const std::string&){}; UsdLayerEditor::setComponentFns(c); }
    auto cmd = std::make_shared<InsertSubPathCmd>(_stage, _parent, _sub->GetIdentifier(), 99);
    EXPECT_THROW(cmd->execute(), std::runtime_error);
    EXPECT_EQ(_parent->GetSubLayerPaths().Find(_sub->GetIdentifier()), static_cast<size_t>(-1));
    UsdLayerEditor::setComponentFns(UsdLayerEditor::ComponentFns{});
}

TEST_F(RemoveSubPathCmdTest, DoIt_ReturnsFalse_WhenIndexOutOfBounds)
{
    { UsdLayerEditor::ComponentFns c; c.displayError = [](const std::string&){}; UsdLayerEditor::setComponentFns(c); }
    auto cmd = std::make_shared<RemoveSubPathCmd>(_stage, _parent, 99);
    EXPECT_THROW(cmd->execute(), std::runtime_error);
    EXPECT_NE(_parent->GetSubLayerPaths().Find(_sub->GetIdentifier()), static_cast<size_t>(-1));
    UsdLayerEditor::setComponentFns(UsdLayerEditor::ComponentFns{});
}

TEST_F(MoveSubPathCmdTest, DoIt_ReturnsFalse_WhenSubPathNotFound)
{
    auto cmd = std::make_shared<MoveSubPathCmd>(_parent, _parent, "nonexistent.usda", 0);
    EXPECT_THROW(cmd->execute(), std::runtime_error);
}

TEST_F(MoveSubPathCmdTest, DoIt_ReturnsFalse_WhenSameParentIndexOutOfBounds)
{
    auto cmd = std::make_shared<MoveSubPathCmd>(_parent, _parent, _subA->GetIdentifier(), 99);
    EXPECT_THROW(cmd->execute(), std::runtime_error);
}

TEST_F(MoveSubPathCmdTest, DoIt_ReturnsFalse_WhenCrossParentIndexOutOfBounds)
{
    auto newParent = PXR_NS::SdfLayer::CreateAnonymous("np_oob");
    auto cmd = std::make_shared<MoveSubPathCmd>(_parent, newParent, _subA->GetIdentifier(), 99);
    EXPECT_THROW(cmd->execute(), std::runtime_error);
}

TEST_F(MoveSubPathCmdTest, DoIt_ReturnsFalse_WhenSubPathExistsInNewParent)
{
    auto newParent = PXR_NS::SdfLayer::CreateAnonymous("np_dup");
    newParent->InsertSubLayerPath(_subA->GetIdentifier(), 0);
    auto cmd = std::make_shared<MoveSubPathCmd>(_parent, newParent, _subA->GetIdentifier(), 0);
    EXPECT_THROW(cmd->execute(), std::runtime_error);
}

// ============================================================================
// Task G: RemoveSubPathCmd edit-target redirect (documented crash-fix)
// ============================================================================

TEST_F(RemoveSubPathCmdTest, DoIt_RetargetsToRootWhenRemovingEditTargetLayer)
{
    _stage->SetEditTarget(_sub);
    ASSERT_EQ(_stage->GetEditTarget().GetLayer(), _sub);

    auto cmd = std::make_shared<RemoveSubPathCmd>(_stage, _parent, 0);
    cmd->execute();
    EXPECT_EQ(_stage->GetEditTarget().GetLayer(), _stage->GetRootLayer());
}

TEST_F(RemoveSubPathCmdTest, Undo_RestoresEditTargetToReinsertedLayer)
{
    _stage->SetEditTarget(_sub);

    auto cmd = std::make_shared<RemoveSubPathCmd>(_stage, _parent, 0);
    cmd->execute();
    cmd->undo();
    EXPECT_EQ(_stage->GetEditTarget().GetLayer(), _sub);
}

} // namespace UsdLayerEditor
