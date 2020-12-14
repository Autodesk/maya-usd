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

#ifndef LAYERTREEVIEWSTYLE_H
#define LAYERTREEVIEWSTYLE_H

#include "qtUtils.h"

#include <QtCore/QPointer>
#include <QtGui/QColor>
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCommonStyle>
#include <QtWidgets/QStyleOption>

namespace UsdLayerEditor {

/**
 * @brief overrides drawing of the treeview, mostly for the drag and drop indicator
 *
 */
class LayerTreeViewStyle : public QCommonStyle
{
public:
    LayerTreeViewStyle() { _appStyle = QApplication::style(); }

protected:
    QPointer<QStyle> _appStyle;

    QColor DROP_INDICATOR_COLOR = QColor(255, 255, 255);
    int    DROP_INDICATOR_WIDTH = DPIScale(3);
    int    ARROW_AREA_WIDTH = DPIScale(24);

    // overrides
    void drawComplexControl(
        ComplexControl             cc,
        const QStyleOptionComplex* opt,
        QPainter*                  p,
        const QWidget*             w = Q_NULLPTR) const override
    {
        _appStyle->drawComplexControl(cc, opt, p, w);
    }

    void drawControl(
        ControlElement      element,
        const QStyleOption* opt,
        QPainter*           p,
        const QWidget*      w = Q_NULLPTR) const override
    {
        _appStyle->drawControl(element, opt, p, w);
    }

    void drawItemPixmap(QPainter* painter, const QRect& rect, int alignment, const QPixmap& pixmap)
        const override
    {
        _appStyle->drawItemPixmap(painter, rect, alignment, pixmap);
    }

    void drawItemText(
        QPainter*           painter,
        const QRect&        rect,
        int                 flags,
        const QPalette&     pal,
        bool                enabled,
        const QString&      text,
        QPalette::ColorRole textRole = QPalette::NoRole) const override
    {
        _appStyle->drawItemText(painter, rect, flags, pal, enabled, text, textRole);
    }

    void drawPrimitive(
        PrimitiveElement    element,
        const QStyleOption* option,
        QPainter*           painter,
        const QWidget*      widget) const override
    {
        // Changes the way the drop indicator is drawn
        if (element == QStyle::PE_IndicatorItemViewItemDrop && !option->rect.isNull()) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            auto oldPen = painter->pen();
            painter->setPen(QPen(DROP_INDICATOR_COLOR, DROP_INDICATOR_WIDTH));
            auto rect = option->rect;
            rect.setLeft(DROP_INDICATOR_WIDTH);
            rect.setRight(widget->width() - DROP_INDICATOR_WIDTH * 2);
            if (option->rect.height() == 0) {
                painter->drawLine(rect.topLeft(), option->rect.topRight());
            } else {
                painter->drawRect(rect);
            }
            painter->setPen(oldPen);
            painter->restore();
        }
    }

    QPixmap generatedIconPixmap(
        QIcon::Mode         iconMode,
        const QPixmap&      pixmap,
        const QStyleOption* opt) const override
    {
        return _appStyle->generatedIconPixmap(iconMode, pixmap, opt);
    }

    SubControl hitTestComplexControl(
        ComplexControl             cc,
        const QStyleOptionComplex* opt,
        const QPoint&              pt,
        const QWidget*             w = Q_NULLPTR) const override
    {
        return _appStyle->hitTestComplexControl(cc, opt, pt, w);
    }

    QRect itemPixmapRect(const QRect& r, int flags, const QPixmap& pixmap) const override
    {
        return _appStyle->itemPixmapRect(r, flags, pixmap);
    }

    QRect itemTextRect(
        const QFontMetrics& fm,
        const QRect&        r,
        int                 flags,
        bool                enabled,
        const QString&      text) const override
    {
        return _appStyle->itemTextRect(fm, r, flags, enabled, text);
    }

    int pixelMetric(
        PixelMetric         m,
        const QStyleOption* opt = Q_NULLPTR,
        const QWidget*      widget = Q_NULLPTR) const override
    {
        return _appStyle->pixelMetric(m, opt, widget);
    }

    void polish(QPalette& pal) override { _appStyle->polish(pal); }
    void polish(QApplication* app) override { _appStyle->polish(app); }
    void polish(QWidget* widget) override { _appStyle->polish(widget); }
    void unpolish(QWidget* widget) override { _appStyle->unpolish(widget); }
    void unpolish(QApplication* application) override { _appStyle->unpolish(application); }

    int
    styleHint(StyleHint hint, const QStyleOption* opt, const QWidget* w, QStyleHintReturn* shret)
        const override
    {
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons) {
            return Qt::LeftButton | Qt::MidButton | Qt::RightButton;
        } else if (hint == QStyle::SH_ItemView_ShowDecorationSelected) {
            return 0;
        } else {
            return _appStyle->styleHint(hint, opt, w, shret);
        }
    }

    QRect subControlRect(
        ComplexControl             cc,
        const QStyleOptionComplex* opt,
        SubControl                 sc,
        const QWidget*             w = Q_NULLPTR) const override
    {
        return _appStyle->subControlRect(cc, opt, sc, w);
    }

    QRect subElementRect(
        SubElement          element,
        const QStyleOption* option,
        const QWidget*      widget = Q_NULLPTR) const override
    {
        if (element == QStyle::SE_TreeViewDisclosureItem) {
            auto rect = option->rect;
            rect.setRight(rect.left() + ARROW_AREA_WIDTH);
            return rect;
        } else {
            return _appStyle->subElementRect(element, option, widget);
        }
    }
    QSize sizeFromContents(
        ContentsType        ct,
        const QStyleOption* opt,
        const QSize&        contentsSize,
        const QWidget*      widget = Q_NULLPTR) const override
    {
        return _appStyle->sizeFromContents(ct, opt, contentsSize, widget);
    }
};

} // namespace UsdLayerEditor

#endif // LAYERTREEVIEWSTYLE_H
