//
// Copyright 2025 Autodesk
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
#include <optional>

#include <qevent.h>
#include <qpoint.h>
#include <qrect.h>
#include <qwidget.h>

namespace Adsk {

class Overlay : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive)
public:
    Overlay(QWidget* parent = nullptr);

    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

    bool isOverResizeHandle(const QPoint& pos) const;
    void updateMask();

    bool active() const;
    void setActive(bool value);

Q_SIGNALS:
    void dragged(int);
    void dragging(bool);

private:
    bool               m_Active = false;
    std::optional<int> m_MousePressGlobalPosY;
    QRect              m_ResizeHandleMask;
    int                m_ResizeHandleSize = 8;
};

} // namespace Adsk