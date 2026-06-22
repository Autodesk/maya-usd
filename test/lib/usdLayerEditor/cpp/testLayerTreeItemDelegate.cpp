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
    ASSERT_NE(delegate(), nullptr);
    QStyleOptionViewItem opt;
    QEvent               ev(QEvent::KeyPress);
    bool handled = delegate()->editorEvent(&ev, treeModel(), opt, QModelIndex());
    EXPECT_FALSE(handled);
}

TEST_F(LayerTreeItemDelegateTest, EditorEvent_UnhandledEventType_ReturnsFalse)
{
    // QEvent::KeyPress hits the default branch → returns false without crashing.
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

    auto paintIndex = [&](const QModelIndex& idx, QStyle::State state) {
        ASSERT_TRUE(idx.isValid());
        QStyleOptionViewItem opt;
        opt.rect  = QRect(0, 0, 400, 22);
        opt.state = state;
        QPixmap  pm(400, 22);
        QPainter p(&pm);
        EXPECT_NO_THROW(delegate()->paint(&p, opt, idx));
        p.end();
    };

    paintIndex(sessionLayerIndex(), QStyle::State_Enabled);
    paintIndex(rootLayerIndex(), QStyle::State_Enabled);
    paintIndex(firstSublayerIndex(), QStyle::State_Enabled);
    paintIndex(firstSublayerIndex(), QStyle::State_Enabled | QStyle::State_Selected);

    auto* item = treeModel()->layerItemFromIndex(firstSublayerIndex());
    ASSERT_NE(item, nullptr);

    lockLayer("", item->layer(), LayerLock_Locked, false);
    paintIndex(firstSublayerIndex(), QStyle::State_Enabled);
    lockLayer("", item->layer(), LayerLock_Unlocked, false);

    addMutedLayer(item->layer());
    paintIndex(firstSublayerIndex(), QStyle::State_Enabled);
    removeMutedLayer(item->layer());
}

} // namespace UsdLayerEditor
