//
// Copyright 2020 Autodesk
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

#include "dirtyLayersCountBadge.h"

#include "qtUtils.h"

#include <QtGui/QPainter>

namespace {
const char HIG_YELLOW[] = "#fbb549";
}

namespace UsdLayerEditor {

DirtyLayersCountBadge::DirtyLayersCountBadge(QWidget* in_parent)
    : QWidget(in_parent)
{
    //
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void DirtyLayersCountBadge::updateCount(int newCount)
{
    if (newCount != _dirtyCount) {
        _dirtyCount = newCount;
        update();
    }
}

void DirtyLayersCountBadge::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (_dirtyCount > 0) {
        auto     thisRect = rect();
        QPainter painter(this);
        auto     oldPen = painter.pen();
        QString  textToDraw;
        if (_dirtyCount > 99) {
            textToDraw = "99+";
        } else {
            textToDraw = QString::number(_dirtyCount);
        }

        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(QColor(HIG_YELLOW)));

        int   size = DPIScale(14);
        int   buttonRightEdge = DPIScale(16);
        QRect drawRect(0, 0, size, size);
        int   charLen = textToDraw.length();
        int   extraWidth = (charLen - 1) * DPIScale(6);
        drawRect.adjust(0, 0, extraWidth, 0);

        drawRect.moveTopLeft(QPoint(buttonRightEdge, 0));
        if (drawRect.right() >= thisRect.right()) {
            drawRect.moveTopRight(thisRect.topRight());
        }

        painter.drawRoundedRect(drawRect, size / 2.0, size / 2.0);

        painter.setPen(QColor(0, 0, 0));
        QFont font;
        font.setPixelSize(DPIScale(11));
        font.setBold(true);
        painter.setFont(font);
        int nudge = DPIScale(-1);
        drawRect.adjust(0, 0, 1, nudge);
        painter.drawText(drawRect, Qt::AlignCenter, textToDraw);

        painter.setPen(oldPen);
    }
}

} // namespace UsdLayerEditor
