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

#include "generatedIconButton.h"

#include "qtUtils.h"

#include <QtCore/QEvent>
#include <QtGui/QHelpEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOptionButton>
#include <QtWidgets/QToolTip>

namespace {

enum class PixmapType
{
    DISABLED,
    HOVER
};

struct LUTS
{
    int valueLut[256]; // look up table to apply to HS[V] value
    int alphaLut[256]; // look up table to apply to alpha
};

// look up table to apply to HS[V] value
void generateVLut(LUTS& luts, PixmapType pixmapType)
{
    const int HIGH_LIMIT = 205, LOW_LIMIT = 30, MAX_VALUE = 255;
    const int ADJUSTEMENT_VALUE = MAX_VALUE - HIGH_LIMIT;

    int newValue;
    for (int v = 0; v < 256; v++) {
        newValue = v;
        if (v > LOW_LIMIT) {      // value below this limit will not be adjusted
            if (v < HIGH_LIMIT) { // value above this limit will just max up to 255
                if (pixmapType != PixmapType::DISABLED) {
                    newValue = v + ADJUSTEMENT_VALUE;
                }
            } else {
                newValue = MAX_VALUE;
            }
        }
        luts.valueLut[v] = newValue;
    }
}

// look up table to apply to alpha
void generateAlphaLut(LUTS& luts, PixmapType pixmapType)
{
    if (pixmapType != PixmapType::DISABLED) {
        for (int a = 0; a < 256; a++) {
            luts.alphaLut[a] = a;
        }
    } else {
        for (int a = 0; a < 256; a++) {
            luts.alphaLut[a] = static_cast<int>(a * 0.4);
        }
    }
}

// generates a hover or disabled bitmap from a source bitmatp
QPixmap generateIconPixmap(const QPixmap& pixmap, PixmapType pixmapType)
{
    LUTS luts;
    generateVLut(luts, pixmapType);
    generateAlphaLut(luts, pixmapType);

    int  h, s, v, a;
    auto img = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    int  height = img.height();
    int  width = img.width();

    QColor color;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            color = img.pixelColor(x, y);
            color.getHsv(&h, &s, &v, &a);
            color.setHsv(h, s, luts.valueLut[v], luts.alphaLut[a]);
            img.setPixelColor(x, y, color);
        }
    }
    return QPixmap::fromImage(img);
}

} // namespace

namespace UsdLayerEditor {

GeneratedIconButton::GeneratedIconButton(QWidget* in_parent, const QIcon& in_icon, int in_size)
{
    if (in_size == -1) {
        _size = DPIScale(20);
    } else {
        _size = in_size;
    }
    setIcon(in_icon);
    _noIcons = in_icon.availableSizes().size() == 0;
    _inHover = false;

    if (!_noIcons) {
        _basePixmap = in_icon.pixmap(_size, _size);
        _hoverPixmap = generateIconPixmap(_basePixmap, PixmapType::HOVER);
        _disabledPixmap = generateIconPixmap(_basePixmap, PixmapType::DISABLED);
    }
}

bool GeneratedIconButton::event(QEvent* in_event)
{
    switch (in_event->type()) {
    case QEvent::Enter: {
        _inHover = true;
        repaint();
    } break;
    case QEvent::Leave: {
        _inHover = false;
        repaint();
    } break;
    case QEvent::ToolTip: {
        QToolTip::showText(dynamic_cast<QHelpEvent*>(in_event)->globalPos(), toolTip());
    } break;
    default: {
        return QAbstractButton::event(in_event);
    }
    }
    return true;
}

void GeneratedIconButton::paintEvent(QPaintEvent* in_event)
{
    QPainter painter(this);
    if (_noIcons) {
        painter.drawRect(rect());
    } else {
        QPixmap pixmap;

        if (!isEnabled()) {
            pixmap = _disabledPixmap;
        } else if (_inHover) {
            pixmap = _hoverPixmap;
        } else {
            pixmap = _basePixmap;
        }
        QStyleOptionButton option;
        option.initFrom(this);
        painter.drawPixmap(option.rect, pixmap, pixmap.rect());
    }
}

QSize GeneratedIconButton::sizeHint() const { return QSize(_size, _size); }

} // namespace UsdLayerEditor
