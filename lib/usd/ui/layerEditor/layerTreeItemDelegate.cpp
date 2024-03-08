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

namespace {

// Automatically adjust the opacity when the item should appear disabled.
class AutoOpacity
{
public:
    AutoOpacity(QPainter& painter, UsdLayerEditor::LayerTreeItem const& item, double opacity)
        : _painter(painter)
        , _muted(item.appearsMuted())
        , _locked(item.isLocked())
        , _systemLocked(item.isSystemLocked())
        , _readOnly(item.isReadOnly())
    {
        if (_muted || _readOnly || _locked || _systemLocked)
            _painter.setOpacity(opacity);
    }

    ~AutoOpacity()
    {
        if (_muted || _readOnly || _locked || _systemLocked)
            _painter.setOpacity(1.0);
    }

private:
    QPainter&  _painter;
    const bool _muted;
    const bool _locked;
    const bool _systemLocked;
    const bool _readOnly;
};
} // namespace

namespace UsdLayerEditor {

LayerTreeItemDelegate::LayerTreeItemDelegate(LayerTreeView* in_parent)
    : QStyledItemDelegate(in_parent)
    , _treeView(in_parent)
{
    DISABLED_BACKGROUND_IMAGE = utils->createPNGResPixmap(
        QString(":/UsdLayerEditor/") + QtUtils::getDPIPixmapName("striped"));
    DISABLED_HIGHLIGHT_IMAGE = utils->createPNGResPixmap(
        QString(":/UsdLayerEditor/") + QtUtils::getDPIPixmapName("striped_selected"));
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

static int getActionRightOffset(const LayerTreeItem& item, LayerActionType actionType)
{
    LayerActionInfo action;
    item.getActionButton(actionType, action);
    return action._order + 1;
}

QRect LayerTreeItemDelegate::getTextRect(const ItemPaintContext& ctx) const
{

    QRect textRect(ctx.itemRect);
    textRect.setLeft(textRect.left() + TEXT_LEFT_OFFSET);

    // Note: action order starts from zero.
    int rightOffset = 0;
    if (ctx.isLocked || ctx.isSystemLocked)
        rightOffset = std::max(rightOffset, getActionRightOffset(*ctx.item, LayerActionType::Lock));
    if (ctx.isMuted)
        rightOffset = std::max(rightOffset, getActionRightOffset(*ctx.item, LayerActionType::Mute));
    textRect.setRight(textRect.right() - (rightOffset * ACTION_WIDTH + DPIScale(6)));

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
    QPainter*               painter,
    const ItemPaintContext& ctx,
    Options                 option) const
{
    if (ctx.item->isInvalidLayer())
        return;

    auto targetRect = getTargetIconRect(ctx.itemRect);
    bool isInRect = QtUtils::isMouseInRectangle(_treeView, targetRect);
    bool hover = ctx.isHover && !ctx.isLocked && !ctx.isSystemLocked && isInRect
        && (option.state & QStyle::State_MouseOver);

    QPixmap icon;
    if (ctx.item->isTargetLayer()) {
        icon = ctx.isPressed ? TARGET_ON_IMAGES[2] : TARGET_ON_IMAGES[hover ? 1 : 0];
    } else {
        icon = ctx.isPressed ? TARGET_OFF_IMAGES[2] : TARGET_OFF_IMAGES[hover ? 1 : 0];
    }

    static const QColor noBorder;
    drawIconInRect(painter, icon, targetRect, noBorder);

    if (isInRect) {
        ctx.tooltip = StringResources::getAsQString(StringResources::kSetLayerAsTargetLayerTooltip);
    }
}

void LayerTreeItemDelegate::paint_drawFill(
    QPainter*               painter,
    const ItemPaintContext& ctx,
    QRectC                  rect) const
{
    bool muted = ctx.item->appearsMuted();

    // Offset necessary to align the disabled background stripes between rows.
    static volatile int rowOffset = DPIScale(7);
    static volatile int depthOffset = DPIScale(0);
    int height = std::max(1, ctx.item->data(Qt::SizeHintRole).value<QSize>().height());
    int offset = (rect.top() / height) * rowOffset + ctx.item->depth() * depthOffset + rect.left();

    if (ctx.isHighlighted) {
        if (muted) {
            painter->drawTiledPixmap(rect, DISABLED_HIGHLIGHT_IMAGE, QPoint(offset, 0));
        } else if (ctx.isHover) {
            painter->fillRect(rect, ctx.highlightColor.darker());
        } else {
            painter->fillRect(rect, ctx.highlightColor);
        }
    } else {
        if (ctx.isHover) {
            painter->fillRect(
                rect, ctx.item->data(Qt::BackgroundRole).value<QBrush>().color().darker());
        } else {
            painter->fillRect(rect, ctx.item->data(Qt::BackgroundRole).value<QBrush>());
        }
        if (muted) {
            painter->drawTiledPixmap(rect, DISABLED_BACKGROUND_IMAGE, QPoint(offset, 0));
        }
    }
}

void LayerTreeItemDelegate::paint_drawArrow(QPainter* painter, const ItemPaintContext& ctx) const
{
    painter->save();
    if (ctx.item->rowCount() != 0) {
        auto           left = ctx.itemRect.left() + ARROW_OFFSET + 1;
        const QPointF* arrow = nullptr;
        if (_treeView->isExpanded(ctx.item->index())) {
            painter->translate(left + EXPANDED_ARROW_OFFSET, ctx.itemRect.top());
            arrow = &EXPANDED_ARROW[0];
        } else {
            painter->translate(left + COLLAPSED_ARROW_OFFSET, ctx.itemRect.top());
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

void LayerTreeItemDelegate::paint_drawText(QPainter* painter, const ItemPaintContext& ctx) const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    painter->setPen(QPen(ctx.item->data(Qt::ForegroundRole).value<QColor>(), 1));
#else
    painter->setPen(QPen(ctx.item->data(Qt::TextColorRole).value<QColor>(), 1));
#endif
    const auto textRect = getTextRect(ctx);

    QString text = ctx.item->data(Qt::DisplayRole).value<QString>();

    // draw a * for dirty layers
    const bool readOnly = ctx.item->isReadOnly();
    if (ctx.item->needsSaving() || (ctx.item->isDirty() && !readOnly)) {
        // item.needsSaving returns false for sessionLayer, but I think we should show the dirty
        // flag for it
        text += "*";
    }

    QFontMetrics fm = painter->fontMetrics();
    QString      elidedText = fm.elidedText(text, Qt::ElideRight, textRect.width());

    QRect boundingRect;
    painter->drawText(
        textRect, ctx.item->data(Qt::TextAlignmentRole).value<int>(), elidedText, &boundingRect);
    if (ctx.item->isInvalidLayer()) {
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
    QPainter*               painter,
    int                     left,
    int                     top,
    const LayerActionInfo&  actionInfo,
    const ItemPaintContext& ctx) const
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

    paint_drawFill(painter, ctx, backgroundRect);
    drawIconInRect(painter, *icon, backgroundRect, actionInfo._borderColor);

    if (_lastHitAction == actionInfo._name) {
        if (ctx.isSystemLocked) {
            ctx.tooltip = StringResources::getAsQString(StringResources::kLayerIsSystemLocked);
        } else {
            ctx.tooltip = actionInfo._tooltip;
        }
    }
}

void LayerTreeItemDelegate::paint_ActionIcon(
    QPainter*               painter,
    const ItemPaintContext& ctx,
    LayerActionType         actionType) const
{
    LayerActionInfo action;
    ctx.item->getActionButton(actionType, action);

    // Draw the icon if it is checked or if the mouse is over the item.
    const bool shouldDraw
        = (action._checked || QtUtils::isMouseInRectangle(_treeView, ctx.itemRect));
    if (!shouldDraw)
        return;

    LayerMasks layerMask = CreateLayerMask(
        ctx.item->isRootLayer(), ctx.item->isSublayer(), ctx.item->isSessionLayer());
    if (!IsLayerActionAllowed(action, layerMask)) {
        return;
    }

    int top = ctx.itemRect.top() + ICON_TOP_OFFSET;
    int iconLeft = (action._order + 1) * (ACTION_WIDTH + ACTION_BORDER + action._extraPadding)
        + action._order * 2 * ACTION_BORDER;

    paint_drawOneAction(painter, ctx.itemRect.right() - iconLeft, top, action, ctx);
}

void LayerTreeItemDelegate::paint_ActionIcons(QPainter* painter, const ItemPaintContext& ctx) const
{
    {
        AutoOpacity autoOpacity(*painter, *ctx.item, ctx.isSystemLocked ? DISABLED_OPACITY : 1.0);
        paint_ActionIcon(painter, ctx, LayerActionType::Lock);
    }
    paint_ActionIcon(painter, ctx, LayerActionType::Mute);
}

void LayerTreeItemDelegate::paint(
    QPainter*                   painter,
    const QStyleOptionViewItem& option,
    const QModelIndex&          index) const
{
    if (!index.isValid())
        return;

    ItemPaintContext ctx;
    ctx.item = _treeView->layerItemFromIndex(index);
    ctx.itemRect = getAdjustedItemRect(ctx.item, option.rect);
    ctx.isPressed = (_pressedTarget == ctx.item);
    ctx.isMuted = ctx.item->appearsMuted();
    ctx.isLocked = ctx.item->isLocked();
    ctx.isSystemLocked = ctx.item->isSystemLocked();
    ctx.isReadOnly = ctx.item->isReadOnly();
    ctx.isHighlighted = option.showDecorationSelected && (option.state & QStyle::State_Selected);
    ctx.isHover
        = !ctx.isMuted && !ctx.isReadOnly && QtUtils::isMouseInRectangle(_treeView, ctx.itemRect);
    ctx.highlightColor = option.palette.color(QPalette::Highlight);

    if (ctx.item->isInvalidLayer()) {
        ctx.tooltip = StringResources::getAsQString(StringResources::kPathNotFound)
            + ctx.item->subLayerPath().c_str();
    } else {
        ctx.tooltip = ctx.item->layer()->GetRealPath().c_str();
    }

    paint_drawFill(painter, ctx, ctx.itemRect);
    paint_drawArrow(painter, ctx);

    {
        AutoOpacity autoOpacity(*painter, *ctx.item, DISABLED_OPACITY);
        paint_drawTarget(painter, ctx, option);
        paint_drawText(painter, ctx);
    }

    paint_ActionIcons(painter, ctx);

    if (ctx.item->toolTip() != ctx.tooltip)
        const_cast<LayerTreeItem*>(ctx.item)->setToolTip(ctx.tooltip);
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