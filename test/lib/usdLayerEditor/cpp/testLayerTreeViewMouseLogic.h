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

#include "layerTreeView.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QPoint>
#include <QtCore/QPointF>
#include <QtGui/QMouseEvent>

#include <gtest/gtest.h>

namespace UsdLayerEditor {

namespace {

QPoint itemCenter(LayerTreeView* tree, const QModelIndex& idx)
{
    QRect rect = tree->visualRect(idx);
    return rect.isEmpty() ? QPoint(10, 10) : rect.center();
}

void sendMousePress(QWidget* widget, const QPoint& pos)
{
    QMouseEvent event(
        QEvent::MouseButtonPress, QPointF(pos), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &event);
}

void sendMouseRelease(QWidget* widget, const QPoint& pos)
{
    QMouseEvent event(
        QEvent::MouseButtonRelease, QPointF(pos), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &event);
}

void sendMouseMove(QWidget* widget, const QPoint& pos)
{
    QMouseEvent event(
        QEvent::MouseMove, QPointF(pos), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(widget, &event);
}

} // namespace

class LayerTreeViewMouseTest : public LayerEditorTestFixture { };

TEST_F(LayerTreeViewMouseTest, MousePress_OnValidItem_DoesNotCrash)
{
    ASSERT_NE(layerTree(), nullptr);
    QModelIndex idx = firstSublayerIndex();
    ASSERT_TRUE(idx.isValid());
    QPoint pos = itemCenter(layerTree(), idx);

    EXPECT_NO_THROW(sendMousePress(layerTree()->viewport(), pos));
}

TEST_F(LayerTreeViewMouseTest, MouseRelease_AfterPress_DoesNotCrash)
{
    ASSERT_NE(layerTree(), nullptr);
    QModelIndex idx = firstSublayerIndex();
    ASSERT_TRUE(idx.isValid());
    QPoint pos = itemCenter(layerTree(), idx);

    sendMousePress(layerTree()->viewport(), pos);
    EXPECT_NO_THROW(sendMouseRelease(layerTree()->viewport(), pos));
}

TEST_F(LayerTreeViewMouseTest, MouseMove_OverItem_DoesNotCrash)
{
    ASSERT_NE(layerTree(), nullptr);
    QModelIndex idx = firstSublayerIndex();
    ASSERT_TRUE(idx.isValid());
    QPoint pos = itemCenter(layerTree(), idx);

    EXPECT_NO_THROW(sendMouseMove(layerTree()->viewport(), pos));
}

TEST_F(LayerTreeViewMouseTest, MouseClick_RootLayerItem_DoesNotCrash)
{
    ASSERT_NE(layerTree(), nullptr);
    QPoint pos = itemCenter(layerTree(), rootLayerIndex());

    sendMousePress(layerTree()->viewport(), pos);
    EXPECT_NO_THROW(sendMouseRelease(layerTree()->viewport(), pos));
}

TEST_F(LayerTreeViewMouseTest, Repaint_DoesNotCrash)
{
    ASSERT_NE(layerTree(), nullptr);
    EXPECT_NO_THROW(layerTree()->viewport()->repaint());
}

} // namespace UsdLayerEditor
