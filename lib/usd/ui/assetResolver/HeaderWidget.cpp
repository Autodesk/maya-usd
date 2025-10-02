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

#include "HeaderWidget.h"

#include "ApplicationHost.h"

#include <QStyle>
#include <QStyleOptionHeader>
#include <QStylePainter>

namespace Adsk {

class HeaderWidgetPrivate
{
public:
    HeaderWidgetPrivate(const QString& title)
        : m_Title(title)
    {
    }
    QString m_Title;
};

HeaderWidget::HeaderWidget(const QString& title, QWidget* parent)
    : QWidget(parent)
    , d_ptr(new HeaderWidgetPrivate(title))
{
    setMinimumHeight(ApplicationHost::instance().pm(ApplicationHost::PixelMetric::ItemHeight));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

HeaderWidget::~HeaderWidget() = default;
QString HeaderWidget::title() const
{
    Q_D(const HeaderWidget);
    return d->m_Title;
}

void HeaderWidget::setTitle(const QString& title)
{
    Q_D(HeaderWidget);
    if (d->m_Title != title) {
        d->m_Title = title;
        update();
    }
}

void HeaderWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(const HeaderWidget);
    QStylePainter painter(this);

    QStyleOptionHeaderV2 opt;
    opt.initFrom(this);

    opt.position = QStyleOptionHeaderV2::SectionPosition::Middle;
    opt.orientation = Qt::Horizontal;
    opt.state = QStyle::State_None | QStyle::State_Raised | QStyle::State_Horizontal;
    opt.section = 0;

    // adjust the rect to not draw the right border
    opt.rect.adjust(-1, 0, 1, 0);

    if (isEnabled()) {
        opt.state |= QStyle::State_Enabled;
        if (isActiveWindow()) {
            opt.state |= QStyle::State_Active;
        }
    }
    opt.text = d->m_Title;
    painter.drawControl(QStyle::CE_Header, opt);
}

} // namespace Adsk