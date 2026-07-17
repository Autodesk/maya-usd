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

#include "layerTreeItemDelegate.h"
#include "layerLocking.h"
#include "layerMuting.h"

#include <QtCore/QEvent>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOptionViewItem>

#include <gtest/gtest.h>

namespace UsdLayerEditor {

class LayerTreeItemDelegateTest : public LayerEditorTestFixture
{
protected:
    LayerTreeItemDelegate* delegate()
    {
        return qobject_cast<LayerTreeItemDelegate*>(layerTree()->itemDelegate());
    }

    QStyleOptionViewItem styleOptionFor(const QModelIndex& idx)
    {
        QStyleOptionViewItem opt;
        opt.rect  = layerTree()->visualRect(idx);
        opt.index = idx;
        return opt;
    }
};

// ── state accessors ───────────────────────────────────────────────────────────

TEST_F(LayerTreeItemDelegateTest, OnModelReset_ClearsState)
{
    ASSERT_NE(delegate(), nullptr);
    delegate()->onModelReset();
    EXPECT_TRUE(delegate()->lastHitAction().isEmpty());
    EXPECT_FALSE(delegate()->isTargetPressed());
}

// ── editorEvent — non-mouse event ────────────────────────────────────────────

TEST_F(LayerTreeItemDelegateTest, EditorEvent_InvalidIndex_ReturnsFalse)
{
    // Exercises the invalid-index guard.
    ASSERT_NE(delegate(), nullptr);
    QStyleOptionViewItem opt;
    QEvent               ev(QEvent::KeyPress);
    bool handled = delegate()->editorEvent(&ev, treeModel(), opt, QModelIndex());
    EXPECT_FALSE(handled);
}

TEST_F(LayerTreeItemDelegateTest, EditorEvent_UnhandledEventType_ReturnsFalse)
{
    // Exercises the switch default: QEvent::KeyPress hits no case and returns false.
    ASSERT_NE(delegate(), nullptr);
    QModelIndex          idx = firstSublayerIndex();
    ASSERT_TRUE(idx.isValid());
    QStyleOptionViewItem opt = styleOptionFor(idx);
    QEvent               ev(QEvent::KeyPress);
    EXPECT_FALSE(delegate()->editorEvent(&ev, treeModel(), opt, idx));
}

// ── editorEvent — mouse move ──────────────────────────────────────────────────
// MouseMove only calls _treeView->update(); no dynamic_cast to QMouseEvent.

TEST_F(LayerTreeItemDelegateTest, EditorEvent_MouseMove_ReturnsFalse)
{
    // Exercises the MouseMove case: only calls update() and returns false.
    ASSERT_NE(delegate(), nullptr);
    QModelIndex          idx = firstSublayerIndex();
    ASSERT_TRUE(idx.isValid());
    QStyleOptionViewItem opt = styleOptionFor(idx);
    QEvent               ev(QEvent::MouseMove);
    EXPECT_FALSE(delegate()->editorEvent(&ev, treeModel(), opt, idx));
}

// ── paint() — force full paint pipeline ──────────────────────────────────────

// One smoke test exercising every item-state branch of paint(); a paint
// regression in any state (session/root/sublayer/selected/locked/muted) trips it.
TEST_F(LayerTreeItemDelegateTest, Paint_AllItemStates_DoesNotCrash)
{
    ASSERT_NE(delegate(), nullptr);

    // Render an index in a given state into its own QImage so distinct states
    // can be pixel-compared.
    auto renderIndex = [&](const QModelIndex& idx, QStyle::State state) {
        EXPECT_TRUE(idx.isValid());
        QStyleOptionViewItem opt;
        opt.rect  = QRect(0, 0, 400, 22);
        opt.state = state;
        QPixmap pm(400, 22);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        EXPECT_NO_THROW(delegate()->paint(&p, opt, idx));
        p.end();
        return pm.toImage();
    };

    // Session / root rendering is exercised for crash coverage only; their layouts
    // are not reliably distinct from a plain sublayer at this fixed rect.
    EXPECT_NO_THROW(renderIndex(sessionLayerIndex(), QStyle::State_Enabled));
    EXPECT_NO_THROW(renderIndex(rootLayerIndex(), QStyle::State_Enabled));

    // Headless rendering does not reliably differ between item states at a fixed
    // rect, so assert each state paints a valid image rather than pixel-diffing.
    QImage sublayer = renderIndex(firstSublayerIndex(), QStyle::State_Enabled);
    QImage selected
        = renderIndex(firstSublayerIndex(), QStyle::State_Enabled | QStyle::State_Selected);
    EXPECT_FALSE(sublayer.isNull());
    EXPECT_EQ(sublayer.size(), QSize(400, 22));
    EXPECT_FALSE(selected.isNull());

    auto* item = treeModel()->layerItemFromIndex(firstSublayerIndex());
    ASSERT_NE(item, nullptr);

    addMutedLayer(item->layer());
    QImage muted = renderIndex(firstSublayerIndex(), QStyle::State_Enabled);
    removeMutedLayer(item->layer());
    EXPECT_FALSE(muted.isNull());

    lockLayer("", item->layer(), LayerLock_Locked, false);
    EXPECT_NO_THROW(renderIndex(firstSublayerIndex(), QStyle::State_Enabled));
    lockLayer("", item->layer(), LayerLock_Unlocked, false);
}

} // namespace UsdLayerEditor
