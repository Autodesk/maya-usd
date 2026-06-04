//
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

#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED
#include "testFixture.h"
#endif
#include "testUtils.h"
#include "customLayerData.h"
#include "layerTreeItem.h"

#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/base/vt/array.h>

#include <string>

#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>

namespace UsdLayerEditor {

// ── helpers ───────────────────────────────────────────────────────────────────

static LayerTreeItem* itemAt(LayerTreeModel* m, const QModelIndex& idx)
{
    return dynamic_cast<LayerTreeItem*>(m->itemFromIndex(idx));
}

static QPushButton* findBtn(QWidget* root, const QString& tooltipSubstr)
{
    for (auto* btn : root->findChildren<QPushButton*>()) {
        if (btn->toolTip().contains(tooltipSubstr, Qt::CaseInsensitive))
            return btn;
    }
    return nullptr;
}

// ── SharedStageFixture ────────────────────────────────────────────────────────
// isDccObjectSharedStage() = true — exercises the "owned stage" path where the
// layer editor is responsible for saving layers.

class SharedStageFixture : public LayerEditorTestFixture
{
protected:
    void SetUp() override
    {
        _sessionState._commandHookImpl._isSharedStage = true;
        LayerEditorTestFixture::SetUp();
        QApplication::processEvents();
    }
};

TEST_F(SharedStageFixture, NeedsSaving_TrueForAnonymousLayer)
{
    // Anonymous layers on a shared stage must be saved by the layer editor.
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isAnonymous());
    EXPECT_TRUE(item->needsSaving());
}

TEST_F(SharedStageFixture, NeedsSaving_TrueForDirtyRootLayer)
{
    _sessionState.stage()->GetRootLayer()->SetComment("make dirty");
    auto* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(root->needsSaving());
}

TEST_F(SharedStageFixture, NeedsSaving_FalseForSessionLayer)
{
    // Session layers are Maya-managed — never counted as needing saving here.
    auto* session = itemAt(treeModel(), sessionLayerIndex());
    ASSERT_NE(session, nullptr);
    session->layer()->SetComment("mark dirty");
    EXPECT_FALSE(session->needsSaving());
}

TEST_F(SharedStageFixture, GetAllNeedsSavingLayers_NonEmpty)
{
    // At least one layer (the anonymous sublayer) needs saving on a shared stage.
    auto layers = treeModel()->getAllNeedsSavingLayers();
    EXPECT_FALSE(layers.empty());
}

TEST_F(SharedStageFixture, SaveButton_VisibleOnSharedStage)
{
    QPushButton* btn = findBtn(_widget, "Save all edits");
    ASSERT_NE(btn, nullptr);
    EXPECT_TRUE(btn->isVisible());
}

TEST_F(SharedStageFixture, SaveButton_EnabledWhenLayersNeedSaving)
{
    // The stub stage has an anonymous sublayer, so count >= 1 → button enabled.
    QPushButton* btn = findBtn(_widget, "Save all edits");
    ASSERT_NE(btn, nullptr);
    EXPECT_TRUE(btn->isVisible());
    EXPECT_TRUE(btn->isEnabled());
}

TEST_F(SharedStageFixture, NeedsSaving_FalseAfterSwitchingToNonSharedStage)
{
    // Dynamically flip to non-shared and rebuild the model.
    _sessionState._commandHookImpl._isSharedStage = false;
    treeModel()->forceRefresh();
    QApplication::processEvents();

    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->needsSaving());
}

TEST_F(SharedStageFixture, GetAllNeedsSavingLayers_EmptyAfterSwitchingToNonSharedStage)
{
    _sessionState._commandHookImpl._isSharedStage = false;
    treeModel()->forceRefresh();
    QApplication::processEvents();

    EXPECT_TRUE(treeModel()->getAllNeedsSavingLayers().empty());
}

// ── ReferencedLayersFixture ───────────────────────────────────────────────────
// isDccObjectSharedStage() = false (non-shared stage).  The root layer carries
// "adskSharedLayers" metadata listing sublayers that are owned by another asset
// and therefore read-only in this context.

class ReferencedLayersFixture : public LayerEditorTestFixture
{
protected:
    void SetUp() override
    {
        // _isSharedStage stays false (default) so the referenced-layers path activates.
        // Stamp the first stub sublayer as a referenced (shared) layer BEFORE the widget
        // is built, so rebuildModel() already sees the metadata on first run (no second
        // rebuild needed and no USD notice race).
        auto        rootLayer    = _sessionState.stage()->GetRootLayer();
        std::string sublayerPath = rootLayer->GetSubLayerPaths()[0];
        PXR_NS::VtArray<std::string> refs = { sublayerPath };
        CustomLayerData::setStringArray(refs, rootLayer, PXR_NS::TfToken("adskSharedLayers"));

        LayerEditorTestFixture::SetUp();
        QApplication::processEvents();
    }
};

TEST_F(ReferencedLayersFixture, IsReadOnly_TrueForReferencedLayer)
{
    // The sublayer listed in adskSharedLayers must be read-only.
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isReadOnly());
}

TEST_F(ReferencedLayersFixture, NeedsSaving_FalseForReferencedLayerOnNonSharedStage)
{
    // Non-shared stage: needsSaving() is always false regardless of layer state.
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->needsSaving());
}

TEST_F(ReferencedLayersFixture, IsReadOnly_FalseForNonReferencedRootLayer)
{
    // The root layer itself is NOT in the referenced set — should not be read-only.
    auto* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);
    EXPECT_FALSE(root->isReadOnly());
}

TEST_F(ReferencedLayersFixture, GetAllNeedsSavingLayers_EmptyOnNonSharedStage)
{
    // Even though a sublayer is referenced/shared, non-shared stage → nothing to save.
    EXPECT_TRUE(treeModel()->getAllNeedsSavingLayers().empty());
}

// ── IncomingStageFixture ──────────────────────────────────────────────────────
// isDccObjectStageIncoming() = true — the stage is driven by an external source,
// so all layers are tagged as incoming.

class IncomingStageFixture : public LayerEditorTestFixture
{
protected:
    void SetUp() override
    {
        _sessionState._commandHookImpl._isSharedStage   = true;
        _sessionState._commandHookImpl._isStageIncoming = true;
        LayerEditorTestFixture::SetUp();
        QApplication::processEvents();
    }
};

TEST_F(IncomingStageFixture, IsIncoming_TrueForRootLayer)
{
    auto* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(root->isIncoming());
}

TEST_F(IncomingStageFixture, IsIncoming_TrueForSublayer)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isIncoming());
}

TEST_F(IncomingStageFixture, IsIncoming_FalseForSessionLayer)
{
    // Session layers are never listed in the incoming set.
    auto* session = itemAt(treeModel(), sessionLayerIndex());
    ASSERT_NE(session, nullptr);
    EXPECT_FALSE(session->isIncoming());
}

TEST_F(IncomingStageFixture, NeedsSaving_TrueForAnonymousLayerOnIncomingStage)
{
    // Incoming flag does not suppress saving; shared-stage flag drives that.
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->needsSaving());
}

} // namespace UsdLayerEditor
