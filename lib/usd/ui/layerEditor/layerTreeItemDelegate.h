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

#ifndef LAYERTREEITEMDELEGATE_H
#define LAYERTREEITEMDELEGATE_H

#include "layerTreeItem.h"
#include "layerTreeModel.h"
#include "layerTreeView.h"
#include "qtUtils.h"

#include <QtCore/QPointer>
#include <QtWidgets/QStyledItemDelegate>

class QTreeView;

namespace UsdLayerEditor {
class LayerTreeItem;
class LayerTreeView;
struct LayerActionInfo;

/**
 * @brief Overrides one the drawing and mouse click for individual items in the tree view.
 * Only one instance of this class exists per tree.
 *
 */
class LayerTreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    LayerTreeItemDelegate(LayerTreeView* in_parent);

    // API for LayerTreeView
    // gets the rectangle of the item, adjusted for the tree indentation
    QRect getAdjustedItemRect(LayerTreeItem const* item, QRect const& optionRect) const;
    // gets the rectangle where the text is drawn
    QRect getTextRect(QRect const& itemRect) const;
    // get the rectangle for the "set current target" icon
    QRect getTargetIconRect(QRect const& itemRect) const;

    // QStyledItemDelegate API
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index)
        const override;
    bool editorEvent(
        QEvent*                     event,
        QAbstractItemModel*         model,
        const QStyleOptionViewItem& option,
        const QModelIndex&          index) override;

public:
    // slot:
    void onModelReset()
    {
        _lastHitAction.clear();
        _pressedTarget = nullptr;
    }

    bool    isTargetPressed() const { return _pressedTarget != nullptr; }
    void    clearPressedTarget() { _pressedTarget = nullptr; }
    QString lastHitAction() const { return _lastHitAction; }
    void    clearLastHitAction() { _lastHitAction = ""; }

protected:
    QPointer<LayerTreeView> _treeView;
    QString                 _lastHitAction;

    // used to implement target column
    const LayerTreeItem* _pressedTarget;

    void setPressedTarget(QModelIndex index, LayerTreeItem* item)
    {
        _pressedTarget = item;
        if (item != nullptr)
            item->parentModel()->dataChanged(index, index);
    }

protected:
    typedef QStyleOptionViewItem const& Options;
    typedef QRect const&                QRectC;
    typedef LayerTreeItem const*        Item;

    const int BOTTOM_GAP_OFFSET = DPIScale(2);

    const QColor ARROW_COLOR = QColor(189, 189, 189);
    const int    ARROW_OFFSET = DPIScale(6);
    const int    ARROW_AREA_WIDTH = DPIScale(24);
    const int    EXPANDED_ARROW_OFFSET = DPIScale(3.0);
    const int    COLLAPSED_ARROW_OFFSET = DPIScale(6.0);
    QPointF      EXPANDED_ARROW[3] = { DPIScale(QPointF(0.0, 11.0)),
                                  DPIScale(QPointF(10.0, 11.0)),
                                  DPIScale(QPointF(5.0, 16.0)) };
    QPointF      COLLAPSED_ARROW[3] = { DPIScale(QPointF(0.0, 8.0)),
                                   DPIScale(QPointF(5.0, 13.0)),
                                   DPIScale(QPointF(0.0, 18.0)) };
    // action icon area
    const int ACTION_BORDER = DPIScale(2);
    const int ICON_WIDTH = DPIScale(20);
    const int ACTION_WIDTH = ICON_WIDTH + (2 * ACTION_BORDER);
    const int WARNING_ICON_WIDTH = DPIScale(11);
    const int ICON_TOP_OFFSET = DPIScale(4);

    const int    CHECK_MARK_AREA_WIDTH = DPIScale(28);
    const int    TEXT_LEFT_OFFSET = (ARROW_AREA_WIDTH + CHECK_MARK_AREA_WIDTH);
    const int    TEXT_RIGHT_OFFSET = DPIScale(36);
    const int    HIGHLIGHTED_FILL_OFFSET = DPIScale(1);
    const double DISABLED_OPACITY = 0.6;
    QPixmap      DISABLED_BACKGROUND_IMAGE;
    QPixmap      DISABLED_HIGHLIGHT_IMAGE;
    QPixmap      TARGET_ON_IMAGES[3];
    QPixmap      TARGET_OFF_IMAGES[3];
    QPixmap      WARNING_IMAGE;

    void paint_drawTarget(QPainter* painter, QRectC rect, Item item, Options option) const;
    void paint_drawFill(
        QPainter*     painter,
        QRectC        rect,
        Item          item,
        bool          isHighlighted,
        const QColor& highlightColor) const;
    void paint_drawArrow(QPainter* painter, QRectC rect, Item item) const;
    void paint_drawText(QPainter* painter, QRectC rect, Item item) const;
    void paint_ActionIcons(QPainter* painter, QRectC rect, Item item, const QColor& highlightColor)
        const;
    void paint_drawToolbarFrame(QPainter* painter, QRectC rect, int iconCount) const;
    void paint_drawOneAction(
        QPainter*              painter,
        int                    left,
        int                    top,
        const LayerActionInfo& action,
        const QColor&          highlightColor) const;
    void drawStdIcon(QPainter* painter, int left, int top, const QPixmap& pixmap) const;
};

} // namespace UsdLayerEditor

#endif // LAYERTREEITEMDELEGATE_H
