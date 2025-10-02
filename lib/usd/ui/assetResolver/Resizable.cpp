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

#include "Resizable.h"

#include "ApplicationHost.h"
#include "Resizable_p.h"

#include <math.h>
#include <qpainter.h>
#include <qrect.h>
#include <qsize.h>
#include <qstackedlayout.h>

namespace Adsk {

Overlay::Overlay(QWidget* parent)
    : QWidget(parent)
    , m_Active(false)
    , m_MousePressGlobalPosY(std::nullopt)
    , m_ResizeHandleSize(
          ApplicationHost::instance().pm(ApplicationHost::PixelMetric::ResizableActiveAreaSize))
{
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(m_ResizeHandleSize, m_ResizeHandleSize);
    setMouseTracking(true);
    setFocusPolicy(Qt::NoFocus);
}

void Overlay::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (!m_Active)
        return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 32));
    painter.drawRect(m_ResizeHandleMask);
}

void Overlay::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    const QSize& size = event->size();
    m_ResizeHandleMask
        = QRect(0, size.height() - m_ResizeHandleSize, size.width(), m_ResizeHandleSize);
    updateMask();
}

bool Overlay::isOverResizeHandle(const QPoint& pos) const
{
    int x = pos.x();
    if (x < 0 || x >= width())
        return false;
    int lowerBorder = height();
    int upperBorder = lowerBorder - m_ResizeHandleSize;
    int y = pos.y();
    return y >= upperBorder && y < lowerBorder;
}

void Overlay::updateMask()
{
    if (m_Active) {
        clearMask();
    } else {
        setMask(m_ResizeHandleMask);
    }
}

bool Overlay::active() const { return m_Active; }
void Overlay::setActive(bool active)
{
    if (m_Active == active)
        return;
    m_Active = active;
    updateMask();
    update();
    setCursor(active ? Qt::SizeVerCursor : Qt::ArrowCursor);
}

void Overlay::mouseMoveEvent(QMouseEvent* event)
{
    if (m_MousePressGlobalPosY.has_value()) {
        int    delta = event->globalPosition().y() - *m_MousePressGlobalPosY;
        Q_EMIT dragged(delta);
        event->accept();
    } else {
        setActive(isOverResizeHandle(event->pos()));
        event->ignore();
    }
}

void Overlay::mousePressEvent(QMouseEvent* event)
{
    if (m_Active) {
        clearMask();
        m_MousePressGlobalPosY = event->globalPosition().y();
        Q_EMIT dragging(true);
        event->accept();
    } else
        event->ignore();
}

void Overlay::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_Active) {
        m_MousePressGlobalPosY = std::nullopt;
        Q_EMIT dragging(false);
        event->accept();
    } else {
        event->ignore();
    }
}

void Overlay::enterEvent(QEnterEvent* event)
{
    setActive(isOverResizeHandle(event->position().toPoint()));
    if (m_Active) {
        event->accept();
    } else {
        event->ignore();
    }
}

void Overlay::leaveEvent(QEvent* event)
{
    if (m_Active) {
        setActive(false);
        event->accept();
    } else {
        event->ignore();
    }
}

class ResizablePrivate
{
public:
    Overlay*     m_Overlay = nullptr;
    QWidget*     m_Widget = nullptr;
    QVBoxLayout* m_ContentLayout = nullptr;

    int m_ContentSize = -1;
    int m_DragStartContentSize = -1;

    int m_MinContentSize = 0;
    int m_MaxContentSize = 500;

    QString m_PersistentStorageGroup;
    QString m_PersistentStorageKey;
};

Resizable::Resizable(
    QWidget* widget,
    QWidget* parent,
    QString  persistentStorageGroup,
    QString  persistentStorageKey,
    int      defaultSize)
    : QWidget(parent)
    , d_ptr(new ResizablePrivate())
{
    Q_D(Resizable);
    d->m_PersistentStorageGroup = persistentStorageGroup;
    d->m_PersistentStorageKey = persistentStorageKey;

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto stackedLayout = new QStackedLayout();
    stackedLayout->setContentsMargins(0, 0, 0, 0);
    stackedLayout->setSpacing(0);
    stackedLayout->setStackingMode(QStackedLayout::StackAll);

    auto contentWidget = new QWidget(this);
    d->m_ContentLayout = new QVBoxLayout(contentWidget);
    d->m_ContentLayout->setContentsMargins(
        0,
        0,
        0,
        ApplicationHost::instance().pm(ApplicationHost::PixelMetric::ResizableContentMargin));
    d->m_ContentLayout->setSpacing(0);

    QVariant v = ApplicationHost::instance().loadPersistentData(
        d->m_PersistentStorageGroup, d->m_PersistentStorageKey);
    if (v.isValid() && v.canConvert<float>()) {
        float f = v.toFloat();
        if (f != -1.0f) {
            d->m_ContentSize
                = static_cast<int>(std::round(f * ApplicationHost::instance().uiScale()));
        }
    }
    if (d->m_ContentSize < 0 && defaultSize > 0) {
        setContentSize(defaultSize);
    }

    if (widget) {
        setWidget(widget);
    }

    d->m_Overlay = new Overlay(this);
    stackedLayout->addWidget(contentWidget);
    stackedLayout->addWidget(d->m_Overlay);
    stackedLayout->setCurrentIndex(1);

    connect(d->m_Overlay, &Overlay::dragged, this, [this, d](int dy) {
        resize(width(), d->m_DragStartContentSize + dy);
        setContentSize(height());
    });

    connect(d->m_Overlay, &Overlay::dragging, this, [this, d](bool dragging) {
        if (dragging) {
            d->m_DragStartContentSize = d->m_ContentSize;
        } else {
            float f = d->m_ContentSize / ApplicationHost::instance().uiScale();
            ApplicationHost::instance().savePersistentData(
                d->m_PersistentStorageGroup, d->m_PersistentStorageKey, f);
            updateGeometry();
        }
    });

    auto mainLayout = new QVBoxLayout(this);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addLayout(stackedLayout, 1);
}

Resizable::~Resizable() = default;

QWidget* Resizable::widget() const
{
    Q_D(const Resizable);
    return d->m_Widget;
}

void Resizable::setWidget(QWidget* w)
{
    Q_D(Resizable);
    if (d->m_Widget == w)
        return;

    if (d->m_Widget) {
        d->m_ContentLayout->removeWidget(d->m_Widget);
        d->m_Widget->hide();
    }
    d->m_Widget = w;
    if (d->m_Widget) {
        d->m_ContentLayout->insertWidget(0, d->m_Widget, 1);
        d->m_Widget->show();
        if (d->m_ContentSize == -1) {
            setContentSize(d->m_Widget->height());
        } else {
            setContentSize(d->m_ContentSize);
        }
    }
}

int Resizable::contentSize() const
{
    Q_D(const Resizable);
    return d->m_ContentSize;
}

void Resizable::setContentSize(int s)
{
    Q_D(Resizable);
    d->m_ContentSize = std::clamp(s, d->m_MinContentSize, d->m_MaxContentSize);
    if (d->m_Widget) {
        d->m_Widget->setFixedHeight(d->m_ContentSize);
        updateGeometry();
    }
}

int Resizable::minContentSize() const
{
    Q_D(const Resizable);
    return d->m_MinContentSize;
}

void Resizable::setMinContentSize(int s)
{
    Q_D(Resizable);
    d->m_MinContentSize = std::max(s, 0);
    if (d->m_ContentSize != -1 && d->m_MinContentSize > d->m_ContentSize) {
        setContentSize(d->m_MinContentSize);
    }
}

int Resizable::maxContentSize() const
{
    Q_D(const Resizable);
    return d->m_MaxContentSize;
}

void Resizable::setMaxContentSize(int s)
{
    Q_D(Resizable);
    d->m_MaxContentSize = std::max(s, 0);
    if (d->m_ContentSize != -1 && d->m_MaxContentSize < d->m_ContentSize) {
        setContentSize(d->m_MaxContentSize);
    }
}

} // namespace Adsk
