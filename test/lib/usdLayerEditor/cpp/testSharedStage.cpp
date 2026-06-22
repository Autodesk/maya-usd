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

#include <testFixture.h>
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
        setSharedStage(true);
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
    setSharedStage(false);
    treeModel()->forceRefresh();
    QApplication::processEvents();

    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->needsSaving());
}

TEST_F(SharedStageFixture, GetAllNeedsSavingLayers_EmptyAfterSwitchingToNonSharedStage)
{
    setSharedStage(false);
    treeModel()->forceRefresh();
    QApplication::processEvents();

    EXPECT_TRUE(treeModel()->getAllNeedsSavingLayers().empty());
}

// ── ReferencedLayersFixture ───────────────────────────────────────────────────
// Uses the DCC-agnostic "adskSharedLayers" token.
// Skipped for the old Maya editor (which only writes/reads "mayaSharedLayers").
// See MayaReferencedLayersFixture below for the token the old editor uses.

#ifndef MAYAUSD_OLD_LAYER_EDITOR
class ReferencedLayersFixture : public LayerEditorTestFixture
{
protected:
    void SetUp() override
    {
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
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isReadOnly());
}

TEST_F(ReferencedLayersFixture, NeedsSaving_FalseForReferencedLayerOnNonSharedStage)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->needsSaving());
}

TEST_F(ReferencedLayersFixture, IsReadOnly_FalseForNonReferencedRootLayer)
{
    auto* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);
    EXPECT_FALSE(root->isReadOnly());
}
#endif // MAYAUSD_OLD_LAYER_EDITOR

// ── MayaReferencedLayersFixture ───────────────────────────────────────────────
// Uses the legacy Maya-specific "mayaSharedLayers" token written by proxyShapeBase.
// Both the old and new editors must honour this token.

class MayaReferencedLayersFixture : public LayerEditorTestFixture
{
protected:
    void SetUp() override
    {
        auto        rootLayer    = _sessionState.stage()->GetRootLayer();
        std::string sublayerPath = rootLayer->GetSubLayerPaths()[0];
        PXR_NS::VtArray<std::string> refs = { sublayerPath };
        CustomLayerData::setStringArray(refs, rootLayer, PXR_NS::TfToken("mayaSharedLayers"));

        LayerEditorTestFixture::SetUp();
        QApplication::processEvents();
    }
};

TEST_F(MayaReferencedLayersFixture, IsReadOnly_TrueForMayaReferencedLayer)
{
    // A sublayer stamped with the legacy "mayaSharedLayers" token must be read-only.
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isReadOnly());
}

TEST_F(MayaReferencedLayersFixture, NeedsSaving_FalseForMayaReferencedLayerOnNonSharedStage)
{
    auto* item = itemAt(treeModel(), firstSublayerIndex());
    ASSERT_NE(item, nullptr);
    EXPECT_FALSE(item->needsSaving());
}

TEST_F(MayaReferencedLayersFixture, IsReadOnly_FalseForNonMayaReferencedRootLayer)
{
    auto* root = itemAt(treeModel(), rootLayerIndex());
    ASSERT_NE(root, nullptr);
    EXPECT_FALSE(root->isReadOnly());
}

// ── IncomingStageFixture ──────────────────────────────────────────────────────
// isDccObjectStageIncoming() = true — the stage is driven by an external source,
// so all layers are tagged as incoming.

class IncomingStageFixture : public LayerEditorTestFixture
{
protected:
    void SetUp() override
    {
        setSharedStage(true);
        setStageIncoming(true);
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
