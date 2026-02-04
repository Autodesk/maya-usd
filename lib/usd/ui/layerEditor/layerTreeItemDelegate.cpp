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

using ItemPaintContext = UsdLayerEditor::LayerTreeItemDelegate::ItemPaintContext;

bool isInaccessible(const ItemPaintContext& ctx)
{
    return ctx.isMuted || ctx.isLocked || ctx.isSystemLocked || ctx.isReadOnly;
}

bool isHover(const ItemPaintContext& ctx) { return ctx.isHover; }

bool isInaccessibleNotHover(const ItemPaintContext& ctx)
{
    return !isHover(ctx) && isInaccessible(ctx);
}

bool isSystemLocked(const ItemPaintContext& ctx) { return ctx.isSystemLocked; }

// Automatically adjust the opacity when the item should appear disabled.
class AutoOpacity
{
public:
    AutoOpacity(QPainter& painter, double opacity, bool isOpaque)
        : _painter(painter)
        , _previousOpacity(_painter.opacity())
        , _isOpaque(isOpaque)
    {
        if (_isOpaque)
            _painter.setOpacity(opacity);
    }

    ~AutoOpacity()
    {
        if (_isOpaque)
            _painter.setOpacity(_previousOpacity);
    }

private:
    QPainter&    _painter;
    const double _previousOpacity;
    const bool   _isOpaque;
};

// Automatically save and restore the painter state.
class AutoPainterRestore
{
public:
    AutoPainterRestore(QPainter& painter)
        : _painter(painter)
    {
        _painter.save();
    }

    ~AutoPainterRestore() { _painter.restore(); }

private:
    QPainter& _painter;
};

} // namespace

namespace UsdLayerEditor {

LayerTreeItemDelegate::LayerTreeItemDelegate(LayerTreeView* in_parent)
    : QStyledItemDelegate(in_parent)
    , _treeView(in_parent)
{
    DISABLED_BACKGROUND_IMAGE = utils->createPNGResPixmap(QString(":/UsdLayerEditor/striped"));
    DISABLED_HIGHLIGHT_IMAGE
        = utils->createPNGResPixmap(QString(":/UsdLayerEditor/striped_selected"));
    WARNING_IMAGE = utils->createPNGResPixmap("RS_warning");

    const char* targetOnPixmaps[3] { "target_on", "target_on_hover", "target_on_pressed" };
    const char* targetOffPixmaps[3] { "target_off", "target_off_hover", "target_off_pressed" };
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

    // Note: action order starts from zero. We also check both hover lock and mute
    //       because we show all buttons when hovering.
    int rightOffset = 0;
    if (ctx.isLocked || ctx.isSystemLocked || ctx.isHover)
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
    if (ctx.isInvalid)
        return;

    AutoOpacity autoOpacity(*painter, DISABLED_OPACITY, isInaccessible(ctx));

    auto targetRect = getTargetIconRect(ctx.itemRect);
    bool isInRect = QtUtils::isMouseInRectangle(_treeView, targetRect);
    bool hover = !ctx.isMuted && !ctx.isReadOnly && !ctx.isLocked && !ctx.isSystemLocked
        && ctx.isHover && isInRect && (option.state & QStyle::State_MouseOver);

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
        ctx.isHoverAction = true;
    }
}

void LayerTreeItemDelegate::paint_drawFill(
    QPainter*               painter,
    const ItemPaintContext& ctx,
    QRectC                  rect) const
{
    AutoOpacity autoOpacity(*painter, HOVER_OPACITY, isHover(ctx));

    // Offset necessary to align the disabled background stripes between rows.
    static volatile int rowOffset = DPIScale(7);
    static volatile int depthOffset = DPIScale(0);
    int height = std::max(1, ctx.item->data(Qt::SizeHintRole).value<QSize>().height());
    int offset = (rect.top() / height) * rowOffset + ctx.item->depth() * depthOffset + rect.left();

    if (ctx.isSelected) {
        if (ctx.isMuted) {
            painter->drawTiledPixmap(rect, DISABLED_HIGHLIGHT_IMAGE, QPoint(offset, 0));
        } else {
            painter->fillRect(rect, ctx.highlightColor);
        }
    } else {
        painter->fillRect(rect, ctx.item->data(Qt::BackgroundRole).value<QBrush>());
        if (ctx.isMuted) {
            painter->drawTiledPixmap(rect, DISABLED_BACKGROUND_IMAGE, QPoint(offset, 0));
        }
    }
}

void LayerTreeItemDelegate::paint_drawArrow(QPainter* painter, const ItemPaintContext& ctx) const
{
    if (ctx.item->rowCount() == 0)
        return;

    AutoPainterRestore painterRestore(*painter);

    const QPointF* arrow = nullptr;
    if (_treeView->isExpanded(ctx.item->index())) {
        arrow = &EXPANDED_ARROW[0];
    } else {
        arrow = &COLLAPSED_ARROW[0];
    }

    int left = ctx.itemRect.left() + ARROW_OFFSET + (ARROW_AREA_WIDTH - ARROW_SIZE) / 2;
    painter->translate(left, ctx.itemRect.y() + (ctx.itemRect.height() - ARROW_SIZE) / 2);

    painter->setBrush(ARROW_COLOR);
    painter->setPen(Qt::NoPen);
    painter->drawPolygon(arrow, 3);
}

void LayerTreeItemDelegate::paint_drawText(QPainter* painter, const ItemPaintContext& ctx) const
{
#if QT_DISABLE_DEPRECATED_BEFORE || QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QColor penColor = ctx.item->data(Qt::ForegroundRole).value<QColor>();
#else
    QColor penColor = ctx.item->data(Qt::TextColorRole).value<QColor>();
#endif
    // We lighten the text color when the item is selected unless the item is inaccessible.
    if (ctx.isSelected && (!isInaccessible(ctx) || isHover(ctx)))
        penColor = penColor.lighter();
    painter->setPen(QPen(penColor, 1));
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
    QString      elidedText = fm.elidedText(text, Qt::ElideMiddle, textRect.width());

    QRect boundingRect;
    {
        AutoOpacity autoOpacity(
            *painter, DISABLED_OPACITY, isInaccessibleNotHover(ctx) && !ctx.isSelected);
        painter->drawText(
            textRect,
            ctx.item->data(Qt::TextAlignmentRole).value<int>(),
            elidedText,
            &boundingRect);
    }

    {
        AutoOpacity autoOpacity(*painter, DISABLED_OPACITY, isInaccessibleNotHover(ctx));

        if (ctx.isInvalid) {
            int x = boundingRect.right() + DPIScale(4);
            int y = boundingRect.top();
            painter->drawPixmap(x, y, WARNING_IMAGE);
        }
    }
}

void LayerTreeItemDelegate::drawStdIcon(QPainter* painter, int left, int top, const QPixmap& pixmap)
    const
{
    painter->drawPixmap(QRect(left, top, ICON_WIDTH, ICON_WIDTH), pixmap);
}

void LayerTreeItemDelegate::paint_drawOneAction(
    QPainter*               painter,
    const QRect&            actionRect,
    const LayerActionInfo&  actionInfo,
    const ItemPaintContext& ctx) const
{
    // MAYA 84884: Created a background rectangle underneath the icon to extend the mouse coverage
    // region

    const bool hover = QtUtils::isMouseInRectangle(_treeView, actionRect);
    bool       appearsChecked = actionAppearsChecked(actionInfo, ctx);
    const bool active = appearsChecked;

    const QPixmap* icon = nullptr;
    if (hover) {
        // The System-Lock icon should not have a hover state by design.
        if (actionInfo._actionType == Lock && ctx.isSystemLocked) {
            icon = &actionInfo._pixmap_on;
        } else {
            icon = active ? &actionInfo._pixmap_on_hover : &actionInfo._pixmap_off_hover;
        }
        const_cast<LayerTreeItemDelegate*>(this)->_lastHitAction = actionInfo._name;
    } else {
        icon = active ? &actionInfo._pixmap_on : &actionInfo._pixmap_off;
    }

    drawIconInRect(painter, *icon, actionRect, actionInfo._borderColor);

    if (_lastHitAction == actionInfo._name) {
        if (ctx.isSystemLocked) {
            ctx.tooltip = StringResources::getAsQString(StringResources::kLayerIsSystemLocked);
        } else {
            ctx.tooltip = actionInfo._tooltip;
        }
    }

    ctx.isHoverAction |= hover;
}

QRect LayerTreeItemDelegate::getActionRect(const ItemPaintContext& ctx, LayerActionType actionType)
    const
{
    LayerActionInfo action;
    ctx.item->getActionButton(actionType, action);
    return getActionRect(ctx, action);
}

QRect LayerTreeItemDelegate::getActionRect(
    const ItemPaintContext& ctx,
    const LayerActionInfo&  action) const
{
    const int top = ctx.itemRect.top() + ICON_TOP_OFFSET;
    const int iconLeft = (action._order + 1) * (ACTION_WIDTH + ACTION_BORDER + action._extraPadding)
        + action._order * 2 * ACTION_BORDER;
    const int left = ctx.itemRect.right() - iconLeft;
    const int BACKGROUND_RECT_LENGTH = DPIScale(28);
    const int BACKGROUND_RECT_LEFT_OFFSET = DPIScale(4);
    return QRect(
        left - BACKGROUND_RECT_LEFT_OFFSET,
        top - ACTION_BORDER,
        BACKGROUND_RECT_LENGTH,
        ACTION_WIDTH);
}

void LayerTreeItemDelegate::paint_ActionIcon(
    QPainter*               painter,
    const ItemPaintContext& ctx,
    LayerActionType         actionType) const
{
    LayerActionInfo action;
    ctx.item->getActionButton(actionType, action);

    bool appearsChecked = actionAppearsChecked(action, ctx);

    // Draw the icon if it is checked or if the mouse is over the item.
    const bool shouldDraw
        = (appearsChecked || QtUtils::isMouseInRectangle(_treeView, ctx.itemRect));
    if (!shouldDraw)
        return;

    LayerMasks layerMask = CreateLayerMask(
        ctx.item->isRootLayer(), ctx.item->isSublayer(), ctx.item->isSessionLayer());
    if (!IsLayerActionAllowed(action, layerMask)) {
        return;
    }

    paint_drawOneAction(painter, getActionRect(ctx, action), action, ctx);
}

void LayerTreeItemDelegate::paint_ActionIcons(QPainter* painter, const ItemPaintContext& ctx) const
{
    if (ctx.isInvalid)
        return;

    {
        AutoOpacity autoOpacity(*painter, DISABLED_OPACITY, isSystemLocked(ctx));
        paint_ActionIcon(painter, ctx, LayerActionType::Lock);
    }
    paint_ActionIcon(painter, ctx, LayerActionType::Mute);
}

bool LayerTreeItemDelegate::actionAppearsChecked(
    const LayerActionInfo&  actionInfo,
    const ItemPaintContext& ctx) const
{
    if (actionInfo._checked) {
        return true;
    }

    // This is used to display un-sharable layers as system-locked
    if (actionInfo._actionType == LayerActionType::Lock && ctx.isSystemLocked) {
        return true;
    }

    return false;
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
    ctx.isHover = QtUtils::isMouseInRectangle(_treeView, ctx.itemRect);
    ctx.isPressed = (_pressedTarget == ctx.item);
    ctx.isInvalid = ctx.item->isInvalidLayer();
    ctx.isMuted = ctx.item->appearsMuted();
    ctx.isLocked = ctx.item->isLocked();
    ctx.isSystemLocked = ctx.item->isSystemLocked();
    ctx.isReadOnly = ctx.item->isReadOnly();
    ctx.isSelected = option.showDecorationSelected && (option.state & QStyle::State_Selected);
    ctx.highlightColor = option.palette.color(QPalette::Highlight);
    ctx.isHoverAction = false;

    if (ctx.isInvalid) {
        ctx.tooltip = StringResources::getAsQString(StringResources::kPathNotFound)
            + ctx.item->subLayerPath().c_str();
    } else {
        ctx.tooltip = ctx.item->layer()->GetRealPath().c_str();
    }

    paint_drawFill(painter, ctx, ctx.itemRect);
    paint_drawArrow(painter, ctx);
    paint_drawTarget(painter, ctx, option);
    paint_drawText(painter, ctx);
    paint_ActionIcons(painter, ctx);

    if (ctx.item->toolTip() != ctx.tooltip)
        const_cast<LayerTreeItem*>(ctx.item)->setToolTip(ctx.tooltip);

    const_cast<LayerTreeItem*>(ctx.item)->setData(
        ctx.isHoverAction, LayerTreeItem::kHoverActionRole);
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
