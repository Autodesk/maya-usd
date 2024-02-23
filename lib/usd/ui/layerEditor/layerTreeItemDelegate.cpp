//
// Copyright 2023 Autodesk
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

#include "layerTreeItemDelegate.h"

#include "qtUtils.h"
#include "stringResources.h"

#include <memory>

namespace UsdLayerEditor {

LayerTreeItemDelegate::LayerTreeItemDelegate(LayerTreeView* in_parent)
    : QStyledItemDelegate(in_parent)
    , _treeView(in_parent)
{
    DISABLED_BACKGROUND_IMAGE = utils->createPNGResPixmap("RS_disabled_tile");
    DISABLED_HIGHLIGHT_IMAGE = utils->createPNGResPixmap("RS_disabled_tile_highlight");
    WARNING_IMAGE = utils->createPNGResPixmap("RS_warning");

    const char* targetOnPixmaps[3] { "target_on", "target_on_hover", "target_on_pressed" };
    const char* targetOffPixmaps[3] { "target_regular", "target_hover", "target_pressed" };
    for (int i = 0; i < 3; i++) {
        TARGET_ON_IMAGES[i]
            = utils->createPNGResPixmap(QString(":/UsdLayerEditor/") + targetOnPixmaps[i]);
        TARGET_OFF_IMAGES[i]
            = utils->createPNGResPixmap(QString(":/UsdLayerEditor/") + targetOffPixmaps[i]);
    }
}

QRect LayerTreeItemDelegate::getAdjustedItemRect(LayerTreeItem const* item, QRect const& optionRect)
    const
{
    int   indent = item->depth() * _treeView->indentation();
    QRect rect(optionRect);
    rect.setLeft(indent);
    rect.setBottom(rect.bottom() - BOTTOM_GAP_OFFSET);
    return rect;
}

QRect LayerTreeItemDelegate::getTextRect(QRect const& itemRect) const
{
    QRect textRect(itemRect);
    textRect.setLeft(textRect.left() + TEXT_LEFT_OFFSET);
    textRect.setRight(textRect.right() - TEXT_RIGHT_OFFSET);
    return textRect;
}

QRect LayerTreeItemDelegate::getTargetIconRect(QRect const& itemRect) const
{
    QRect rect(itemRect);
    rect.translate(ARROW_AREA_WIDTH, 0);
    rect.setWidth(CHECK_MARK_AREA_WIDTH);
    return rect;
}

static void drawIconInRect(
    QPainter*      painter,
    const QPixmap& icon,
    const QRect&   targetRect,
    const QColor&  borderColor)
{
    auto iconRect = icon.rect();

    // got to be careful. the icon rect is correct with DPI on windows/linux
    // but on mac, the icon is twice the resolution that we will actually draw in
    // so we need to scale that back, because we draw in logical pixels vs physical
    double deviceRatio = icon.devicePixelRatio();
    if (deviceRatio != 1.0) {
        iconRect.setWidth(static_cast<int>(iconRect.width() / deviceRatio));
        iconRect.setHeight(static_cast<int>(iconRect.height() / deviceRatio));
    }

    iconRect.moveCenter(targetRect.center());
    painter->drawPixmap(iconRect, icon);

    if (borderColor.isValid()) {
        auto oldPen = painter->pen();
        painter->setPen(QPen(borderColor, 1));
        painter->drawRect(iconRect);
        painter->setPen(oldPen);
    }
}

void LayerTreeItemDelegate::paint_drawTarget(
    QPainter* painter,
    QRectC    rect,
    Item      item,
    Options   option) const
{
    if (item->isInvalidLayer()) {
        return;
    }
    auto targetRect = getTargetIconRect(rect);

    bool pressed = (_pressedTarget == item);
    bool muted = item->appearsMuted();
    bool locked = item->isLocked();
    bool systemLocked = item->isSystemLocked();
    bool readOnly = item->isReadOnly();

    bool hover = !muted && !readOnly && !locked && !systemLocked
        && (option.state & QStyle::State_MouseOver)
        && QtUtils::isMouseInRectangle(_treeView, targetRect);

    QPixmap icon;
    if (item->isTargetLayer()) {
        icon = pressed ? TARGET_ON_IMAGES[2] : TARGET_ON_IMAGES[hover ? 1 : 0];
    } else {
        icon = pressed ? TARGET_OFF_IMAGES[2] : TARGET_OFF_IMAGES[hover ? 1 : 0];
    }

    static const QColor noBorder;
    drawIconInRect(painter, icon, targetRect, noBorder);
}

void LayerTreeItemDelegate::paint_drawFill(
    QPainter*     painter,
    QRectC        rect,
    Item          item,
    int           itemRow,
    bool          isHighlighted,
    const QColor& highlightColor) const
{
    bool muted = item->appearsMuted();

    // Offset necessary to align the disabled background stripes between rows.
    static const int rowOffset = 14;

    if (isHighlighted) {
        if (muted) {
            painter->drawTiledPixmap(
                rect, DISABLED_HIGHLIGHT_IMAGE, QPoint(itemRow * rowOffset, 0));
        } else {
            painter->fillRect(rect, highlightColor);
        }
    } else {
        painter->fillRect(rect, item->data(Qt::BackgroundRole).value<QBrush>());
        if (muted) {
            painter->drawTiledPixmap(
                rect, DISABLED_BACKGROUND_IMAGE, QPoint(itemRow * rowOffset, 0));
        }
    }
}

void LayerTreeItemDelegate::paint_drawArrow(QPainter* painter, QRectC rect, Item item) const
{
    painter->save();
    if (item->rowCount() != 0) {
        auto           left = rect.left() + ARROW_OFFSET + 1;
        const QPointF* arrow = nullptr;
        if (_treeView->isExpanded(item->index())) {
            painter->translate(left + EXPANDED_ARROW_OFFSET, rect.top());
            arrow = &EXPANDED_ARROW[0];
        } else {
            painter->translate(left + COLLAPSED_ARROW_OFFSET, rect.top());
            arrow = &COLLAPSED_ARROW[0];
        }
        auto const oldBrush = painter->brush();
        painter->setBrush(ARROW_COLOR);
        painter->setPen(Qt::NoPen);
        painter->drawPolygon(arrow, 3);
        painter->setBrush(oldBrush);
    }
    painter->restore();
}

void LayerTreeItemDelegate::paint_drawText(QPainter* painter, QRectC rect, Item item) const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    painter->setPen(QPen(item->data(Qt::ForegroundRole).value<QColor>(), 1));
#else
    painter->setPen(QPen(item->data(Qt::TextColorRole).value<QColor>(), 1));
#endif
    const auto textRect = getTextRect(rect);

    QString text = item->data(Qt::DisplayRole).value<QString>();

    // draw a * for dirty layers
    const bool readOnly = item->isReadOnly();
    if (item->needsSaving() || (item->isDirty() && !readOnly)) {
        // item.needsSaving returns false for sessionLayer, but I think we should show the dirty
        // flag for it
        text += "*";
    }

    QRect boundingRect;
    painter->drawText(
        textRect, item->data(Qt::TextAlignmentRole).value<int>(), text, &boundingRect);
    if (item->isInvalidLayer()) {
        int x = boundingRect.right() + DPIScale(4);
        int y = boundingRect.top();
        painter->drawPixmap(x, y, WARNING_IMAGE);
    }
}

void LayerTreeItemDelegate::drawStdIcon(QPainter* painter, int left, int top, const QPixmap& pixmap)
    const
{
    painter->drawPixmap(QRect(left, top, ICON_WIDTH, ICON_WIDTH), pixmap);
}

void LayerTreeItemDelegate::paint_drawOneAction(
    QPainter*              painter,
    int                    left,
    int                    top,
    const LayerActionInfo& actionInfo,
    const QColor&          highlightColor) const
{
    // MAYA 84884: Created a background rectangle underneath the icon to extend the mouse coverage
    // region
    int   BACKGROUND_RECT_LENGTH = DPIScale(28);
    int   BACKGROUND_RECT_LEFT_OFFSET = DPIScale(4);
    QRect backgroundRect(
        left - BACKGROUND_RECT_LEFT_OFFSET,
        top - ACTION_BORDER,
        BACKGROUND_RECT_LENGTH,
        ACTION_WIDTH);

    const bool     hover = QtUtils::isMouseInRectangle(_treeView, backgroundRect);
    const bool     active = actionInfo._checked;
    const QPixmap* icon = nullptr;
    if (hover) {
        icon = active ? &actionInfo._pixmap_on_hover : &actionInfo._pixmap_off_hover;
        const_cast<LayerTreeItemDelegate*>(this)->_lastHitAction = actionInfo._name;
    } else {
        icon = active ? &actionInfo._pixmap_on : &actionInfo._pixmap_off;
    }

    drawIconInRect(painter, *icon, backgroundRect, actionInfo._borderColor);
}

void LayerTreeItemDelegate::paint_ActionIcon(
    QPainter*       painter,
    QRectC          rect,
    Item            item,
    LayerActionType actionType,
    const QColor&   highlightColor) const
{
    LayerActionInfo action;
    item->getActionButton(actionType, action);

    // Draw the icon if it is checked or if the mouse is over the item.
    const bool shouldDraw = (action._checked || QtUtils::isMouseInRectangle(_treeView, rect));
    if (!shouldDraw)
        return;

    LayerMasks layerMask
        = CreateLayerMask(item->isRootLayer(), item->isSublayer(), item->isSessionLayer());
    if (!IsLayerActionAllowed(action, layerMask)) {
        return;
    }

    int top = rect.top() + ICON_TOP_OFFSET;
    int iconLeft = (action._order + 1) * (ACTION_WIDTH + ACTION_BORDER + action._extraPadding)
        + action._order * 2 * ACTION_BORDER;

    paint_drawOneAction(
        painter, rect.right() - iconLeft, top, action, action._checked ? highlightColor : QColor());

    QString tooltip;
    if (_lastHitAction == action._name) {
        if (item->isSystemLocked()) {
            tooltip = StringResources::getAsQString(StringResources::kLayerIsSystemLocked);
        } else {
            tooltip = action._tooltip;
        }
    }
    const_cast<LayerTreeItem*>(item)->setToolTip(tooltip);
}

void LayerTreeItemDelegate::paint_ActionIcons(
    QPainter*     painter,
    QRectC        rect,
    Item          item,
    const QColor& highlightColor) const
{
    paint_ActionIcon(painter, rect, item, LayerActionType::Lock, highlightColor);
    paint_ActionIcon(painter, rect, item, LayerActionType::Mute, highlightColor);
}

void LayerTreeItemDelegate::paint(
    QPainter*                   painter,
    const QStyleOptionViewItem& option,
    const QModelIndex&          index) const
{
    if (index.isValid()) {
        auto item = _treeView->layerItemFromIndex(index);

        QRect rect = getAdjustedItemRect(item, option.rect);
        bool  isHighlighted
            = option.showDecorationSelected && (option.state & QStyle::State_Selected);
        auto highlightedColor = option.palette.color(QPalette::Highlight);

        paint_drawFill(painter, rect, item, index.row(), isHighlighted, highlightedColor);
        paint_drawTarget(painter, rect, item, option);
        paint_drawArrow(painter, rect, item);

        bool muted = item->appearsMuted();
        bool locked = item->isLocked();
        bool systemLocked = item->isSystemLocked();
        bool readOnly = item->isReadOnly();
        if (muted || readOnly || locked || systemLocked)
            painter->setOpacity(DISABLED_OPACITY);

        paint_drawText(painter, rect, item);
        paint_ActionIcons(painter, rect, item, highlightedColor);

        if (muted || readOnly || locked || systemLocked)
            painter->setOpacity(1.0);
    }
}

bool LayerTreeItemDelegate::editorEvent(
    QEvent*                     event,
    QAbstractItemModel*         model,
    const QStyleOptionViewItem& option,
    const QModelIndex&          index)
{
    LayerTreeItem* item;
    if (index.isValid()) {
        item = _treeView->layerItemFromIndex(index);
        if (item->isInvalidLayer())
            return false;
    } else {
        return false;
    }

    auto handlePressed = [this, item, index, event, option]() -> bool {
        if (!item->appearsMuted()) {
            auto mouseEvent = dynamic_cast<QMouseEvent*>(event);
            if (event->type() == QEvent::MouseButtonRelease) {
                // if we're release the mouse in the same button we pressed, fire the command
                bool fireCommand = (this->_pressedTarget == item);
                this->_pressedTarget = nullptr;
                if (fireCommand) {
                    item->parentModel()->setEditTarget(item);
                    return true;
                }
            } else if (mouseEvent->button() == Qt::LeftButton) {
                auto rect = this->getAdjustedItemRect(item, option.rect);
                auto targetRect = this->getTargetIconRect(rect);
                auto mousePos = mouseEvent->pos();
                if (targetRect.contains(mousePos)) {
                    setPressedTarget(index, item);
                    return true;
                } else {
                    setPressedTarget(QModelIndex(), nullptr);
                }
            }
        }
        return false;
    };

    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        if (handlePressed())
            return true;
        break;
    case QEvent::MouseMove:
        _treeView->update(); // force redraw to property reflect hover
        break;
    default: break;
    }
    return false;
}

} // namespace UsdLayerEditor