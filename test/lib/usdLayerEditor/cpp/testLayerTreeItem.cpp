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
#pragma once

#include <testFixture.h>
#include "testUtils.h"
#include "layerLocking.h"
#include "layerTreeItem.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <QtWidgets/QApplication>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

static LayerTreeItem* itemAt(LayerTreeModel* model, const QModelIndex& idx)
{
    return dynamic_cast<LayerTreeItem*>(model->itemFromIndex(idx));
}

class LayerTreeItemTest : public LayerEditorTestFixture
{
protected:
    void TearDown() override
    {
        LayerEditorTestFixture::TearDown();
        forgetLockedLayers();
        forgetSystemLockedLayers();
    }
};

// ── isMuted / appearsMuted ────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, IsMuted_ReturnsFalseByDefault)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isMuted());
}

TEST_F(LayerTreeItemTest, IsMuted_ReturnsTrueAfterStageMute)
{
    auto* item  = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    auto  stage = _sessionState.stage();
    stage->MuteLayer(item->layer()->GetIdentifier());
    QApplication::processEvents();
    EXPECT_TRUE(item->isMuted());
    stage->UnmuteLayer(item->layer()->GetIdentifier());
}

TEST_F(LayerTreeItemTest, AppearsMuted_FalseWhenNeitherSelfNorParentMuted)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->appearsMuted());
}

TEST_F(LayerTreeItemTest, AppearsMuted_TrueWhenSelfIsMuted)
{
    auto* item  = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    auto  stage = _sessionState.stage();
    stage->MuteLayer(item->layer()->GetIdentifier());
    QApplication::processEvents();
    EXPECT_TRUE(item->appearsMuted());
    stage->UnmuteLayer(item->layer()->GetIdentifier());
}

// ── isReadOnly ────────────────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, IsReadOnly_FalseForNormalSublayer)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isReadOnly());
}

// ── isDirty / needsSaving ─────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, IsDirty_FalseForCleanLayer)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isDirty());
}

TEST_F(LayerTreeItemTest, IsDirty_TrueAfterLayerModified)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    item->layer()->SetComment("mark dirty");
    EXPECT_TRUE(item->isDirty());
}

TEST_F(LayerTreeItemTest, NeedsSaving_FalseForSessionLayer)
{
    auto* item = itemAt(treeModel(), sessionLayerIndex());
    ASSERT_NE(item, nullptr);
    item->layer()->SetComment("mark dirty");
    // Session layer: needsSaving always false regardless of dirty state.
    EXPECT_FALSE(item->needsSaving());
}

// ── isLocked / appearsLocked ──────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, IsLocked_FalseByDefault)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isLocked());
}

TEST_F(LayerTreeItemTest, IsLocked_TrueWhenPermissionToEditRevoked)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    TestUtils::lockLayerDirect(item->layer());
    EXPECT_TRUE(item->isLocked());
    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerTreeItemTest, AppearsLocked_FalseForRootItemWithUnlockedSelf)
{
    auto* item = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->appearsLocked());
}

TEST_F(LayerTreeItemTest, AppearsLocked_TrueWhenParentIsLocked)
{
    // The sublayer's parent in the tree is the root layer item.
    auto* parentItem = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(parentItem, nullptr);
    TestUtils::lockLayerDirect(parentItem->layer());

    auto* child = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(child, nullptr);
    EXPECT_TRUE(child->appearsLocked());

    TestUtils::unlockLayerDirect(parentItem->layer());
}

TEST_F(LayerTreeItemTest, AppearsLocked_DoesNotCheckSelf)
{
    // A locked item does NOT report appearsLocked for itself — only propagation from parent.
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    TestUtils::lockLayerDirect(item->layer());
    EXPECT_FALSE(item->appearsLocked());
    TestUtils::unlockLayerDirect(item->layer());
}

// ── isSystemLocked / appearsSystemLocked ──────────────────────────────────────

TEST_F(LayerTreeItemTest, IsSystemLocked_FalseByDefault)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isSystemLocked());
}

TEST_F(LayerTreeItemTest, IsSystemLocked_TrueAfterSystemLockApplied)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    addSystemLockedLayer(item->layer());
    item->layer()->SetPermissionToEdit(false);
    EXPECT_TRUE(item->isSystemLocked());
    removeSystemLockedLayer(item->layer());
    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerTreeItemTest, AppearsSystemLocked_FalseWhenParentNotSystemLocked)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->appearsSystemLocked());
}

// ── isMovable ─────────────────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, IsMovable_FalseForSessionLayer)
{
    auto* item = itemAt(treeModel(), sessionLayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isMovable());
}

TEST_F(LayerTreeItemTest, IsMovable_FalseForRootLayer)
{
    auto* item = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isMovable());
}

TEST_F(LayerTreeItemTest, IsMovable_TrueForNormalSublayer)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isMovable());
}

TEST_F(LayerTreeItemTest, IsMovable_FalseWhenLocked)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    TestUtils::lockLayerDirect(item->layer());
    EXPECT_FALSE(item->isMovable());
    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerTreeItemTest, IsMovable_FalseWhenAppearsLocked)
{
    auto* parent = itemAt(treeModel(), rootLayerIndex());
    TestUtils::lockLayerDirect(parent->layer());
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isMovable());
    TestUtils::unlockLayerDirect(parent->layer());
}

TEST_F(LayerTreeItemTest, IsMovable_FalseWhenMuted)
{
    auto* item  = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    _sessionState.stage()->MuteLayer(item->layer()->GetIdentifier());
    QApplication::processEvents();
    EXPECT_FALSE(item->isMovable());
    _sessionState.stage()->UnmuteLayer(item->layer()->GetIdentifier());
}

// ── misc ──────────────────────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, IsTargetLayer_TrueForCurrentEditTarget)
{
    // Root layer is the default edit target.
    auto* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(root->isTargetLayer());
}

TEST_F(LayerTreeItemTest, HasSubLayers_TrueWhenSublayersExist)
{
    // StubSessionState creates a root layer with one sublayer.
    auto* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(root->hasSubLayers());
}

TEST_F(LayerTreeItemTest, HasSubLayers_FalseForLeafSublayer)
{
    auto* sub = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(sub, nullptr);
    EXPECT_FALSE(sub->hasSubLayers());
}

TEST_F(LayerTreeItemTest, IsAnonymous_TrueForAnonymousLayer)
{
    auto* sub = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(sub, nullptr);
    // StubSessionState creates anonymous sublayers.
    EXPECT_TRUE(sub->isAnonymous());
}

TEST_F(LayerTreeItemTest, GetActionButton_LockCheckedMatchesIsLocked)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    TestUtils::lockLayerDirect(item->layer());

    LayerActionInfo info;
    item->getActionButton(LayerActionType::Lock, info);
    EXPECT_TRUE(info._checked);

    TestUtils::unlockLayerDirect(item->layer());
}

TEST_F(LayerTreeItemTest, GetActionButton_MuteCheckedMatchesIsMuted)
{
    auto* item  = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    _sessionState.stage()->MuteLayer(item->layer()->GetIdentifier());
    QApplication::processEvents();

    LayerActionInfo info;
    item->getActionButton(LayerActionType::Mute, info);
    EXPECT_TRUE(info._checked);

    _sessionState.stage()->UnmuteLayer(item->layer()->GetIdentifier());
}

TEST_F(LayerTreeItemTest, ActionButtons_MuteAppliesToSublayerOnly)
{
    const auto& buttons = LayerTreeItem::actionButtonsDefinition();
    auto        it      = buttons.find(LayerActionType::Mute);
    ASSERT_NE(it, buttons.end());
    EXPECT_TRUE(IsLayerActionAllowed(it->second, LayerMasks_SubLayer));
    EXPECT_FALSE(IsLayerActionAllowed(it->second, LayerMasks_Root));
}

TEST_F(LayerTreeItemTest, ActionButtons_LockAppliesToRootAndSublayer)
{
    const auto& buttons = LayerTreeItem::actionButtonsDefinition();
    auto        it      = buttons.find(LayerActionType::Lock);
    ASSERT_NE(it, buttons.end());
    EXPECT_TRUE(IsLayerActionAllowed(it->second, LayerMasks_Root));
    EXPECT_TRUE(IsLayerActionAllowed(it->second, LayerMasks_SubLayer));
}

// ── depth ─────────────────────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, Depth_IsZeroForSessionLayerItem)
{
    auto* item = itemAt(treeModel(), sessionLayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->depth(), 0);
}

TEST_F(LayerTreeItemTest, Depth_IsZeroForRootLayerItem)
{
    auto* item = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->depth(), 0);
}

TEST_F(LayerTreeItemTest, Depth_IsOneForFirstSublayer)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->depth(), 1);
}

// ── childrenVector ────────────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, ChildrenVector_RootItemHasOneSublayer)
{
    // The stub stage was created with one anonymous sublayer on the root.
    auto* item = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->childrenVector().size(), 1u);
}

TEST_F(LayerTreeItemTest, ChildrenVector_EmptyForLeafSublayer)
{
    // The single sublayer has no children of its own.
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->childrenVector().empty());
}

// ── isIdenticalItem ────────────────────────────────────────────────────────────

#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(LayerTreeItemTest, IsIdenticalItem_NullOtherReturnsFalse)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->isIdenticalItem(nullptr));
}

TEST_F(LayerTreeItemTest, IsIdenticalItem_SamePointerReturnsTrue)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIdenticalItem(item));
}

TEST_F(LayerTreeItemTest, IsIdenticalItem_DifferentLayerReturnsFalse)
{
    // Root layer item vs first sublayer item — different layers.
    auto* root = itemAt(treeModel(), treeModel()->rootLayerIndex());
    ASSERT_NE(root, nullptr);
    auto* sub = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(sub, nullptr);
    EXPECT_FALSE(root->isIdenticalItem(sub));
}
#endif

// ── saveAnonymousLayer early-out (D10) ───────────────────────────────────────

// A non-component stage's anonymous layer goes through the generic save path
// (SessionState::saveLayerUI), which the stub records via _saveLayerCallCount.
TEST_F(LayerTreeItemTest, SaveAnonymousLayer_NonComponentStage_UsesGenericPath)
{
#ifndef MAYAUSD_OLD_LAYER_EDITOR
    // Configures the new editor's component early-out to treat the stage as a
    // non-component. The old editor has no such early-out and ignores the DCC
    // registry, so this setup is omitted there (and the registry isn't linked).
    ScopedLayerEditorDCCFunctions guard;
    ComponentFns                  comp;
    comp.isStageAComponent = [](const std::string&) { return false; };
    setComponentFns(comp);
#endif

    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isAnonymous());

    _sessionState._saveLayerCallCount = 0;
    item->saveEditsNoPrompt(nullptr);
    QApplication::processEvents();

    EXPECT_EQ(_sessionState._saveLayerCallCount, 1)
        << "non-component anonymous layer should use the generic saveLayerUI path";
}

// A component stage's anonymous layer must NOT take the generic save path; the
// early-out delegates to LayerTreeModel::saveStage (which shows a modal
// SaveLayersDialog, dismissed here). _saveLayerCallCount staying 0 proves the
// generic path was skipped -- it would be 1 if the early-out were missing.
#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(LayerTreeItemTest, SaveAnonymousLayer_ComponentStage_SkipsGenericPath)
{
    ScopedLayerEditorDCCFunctions guard;
    ComponentFns                  comp;
    comp.isStageAComponent = [](const std::string&) { return true; };
    // isAnonymous() now reflects unsaved-component state for component stages;
    // an unsaved component reports anonymous, which the save early-out requires.
    comp.isUnsavedComponent = [](const PXR_NS::UsdStageRefPtr&) { return true; };
    setComponentFns(comp);

    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isAnonymous());

    // saveStage shows a modal SaveLayersDialog; schedule its dismissal so exec() returns.
    TestUtils::dismissNextModal(100);

    _sessionState._saveLayerCallCount = 0;
    item->saveEditsNoPrompt(nullptr);
    QApplication::processEvents();

    EXPECT_EQ(_sessionState._saveLayerCallCount, 0)
        << "component stage should delegate to saveStage, skipping the generic "
           "anonymous-save path";
}
#endif

// ── isAnonymous component override (match OLD editor) ─────────────────────────

#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(LayerTreeItemTest, IsAnonymous_FalseForSavedComponent)
{
    setIsComponent(true);
    setIsUnsavedComponent(false);

    LayerTreeItem* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);
    // Root layer is anonymous (in-memory stage); the component override must
    // make a SAVED component report not-anonymous, flipping the layer flag.
    EXPECT_FALSE(root->isAnonymous());
}
#endif

TEST_F(LayerTreeItemTest, IsAnonymous_TrueForUnsavedComponent)
{
    setIsComponent(true);
    setIsUnsavedComponent(true);

    LayerTreeItem* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(root->isAnonymous());
}

#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(LayerTreeItemTest, SaveEdits_ComponentRoutesToSaveStageSkippingOverwriteConfirm)
{
    // A component reports non-anonymous when saved, which would normally make
    // saveEdits show its overwrite-confirm dialog. The component early-exit must
    // route straight to saveStage (the component creator owns the save flow)
    // before that confirm is ever shown -- matching the old editor.
    setIsComponent(true);
    setIsUnsavedComponent(false);
    _confirmExistingFileSave = true;
    _modalDialogCount = 0;

    LayerTreeItem* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);

    root->saveEdits(nullptr);

    EXPECT_EQ(_modalDialogCount, 0)
        << "component saveEdits must route to saveStage before the overwrite-confirm dialog";
}

TEST_F(LayerTreeItemTest, DiscardEdits_ComponentStageConfirmsThenReloadsComponent)
{
    // A saved component (isUnsavedComponent=false) reports non-anonymous, so a dirty
    // one must prompt for confirmation FIRST and then reload as a unit -- matching the
    // old editor, which checks the component only after the revert confirmation.
    setIsComponent(true);
    setIsUnsavedComponent(false);
    _reloadComponentCalls = 0;
    _modalDialogCount = 0;

    LayerTreeItem* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);
    root->layer()->SetComment("make dirty"); // force the confirmation path

    root->discardEdits(nullptr);

    EXPECT_EQ(_modalDialogCount, 1)
        << "a dirty saved component must be confirmed before reloading";
    EXPECT_EQ(_reloadComponentCalls, 1)
        << "after confirmation, discardEdits on a component must route through reloadComponent";
}
#endif

// ── type() ───────────────────────────────────────────────────────────────────

TEST_F(LayerTreeItemTest, Type_ReturnsUserType)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->type(), QStandardItem::UserType);
}

// ── invalid layer (unresolvable sublayer path) ────────────────────────────────

TEST_F(LayerTreeItemTest, FetchData_InvalidLayer_DisplayNameIsSubLayerPath)
{
    // Insert a path that cannot be resolved to exercise the invalid-layer
    // branch of fetchData() where _displayName is set from _subLayerPath.
    const std::string fakePath = "/nonexistent/fake_display_test.usda";
    _sessionState.stage()->GetRootLayer()->InsertSubLayerPath(fakePath, 0);
    treeModel()->forceRefresh();
    QApplication::processEvents();

    auto* invalid = itemAt(treeModel(), treeModel()->index(0, 0, rootLayerIndex()));
    ASSERT_NE(invalid, nullptr);
    ASSERT_TRUE(invalid->isInvalidLayer());
    EXPECT_EQ(invalid->displayName(), fakePath);
}

TEST_F(LayerTreeItemTest, HasSubLayers_ReturnsFalse_ForInvalidLayer)
{
    // An invalid layer item has _layer == nullptr so hasSubLayers() must return false
    // via the early-out guard, not the GetNumSubLayerPaths() path.
    const std::string fakePath = "/nonexistent/fake_has_sublayers.usda";
    _sessionState.stage()->GetRootLayer()->InsertSubLayerPath(fakePath, 0);
    treeModel()->forceRefresh();
    QApplication::processEvents();

    auto* invalid = itemAt(treeModel(), treeModel()->index(0, 0, rootLayerIndex()));
    ASSERT_NE(invalid, nullptr);
    ASSERT_TRUE(invalid->isInvalidLayer());
    EXPECT_FALSE(invalid->hasSubLayers());
}

} // namespace UsdLayerEditor
