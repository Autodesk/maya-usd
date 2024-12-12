from typing import Sequence, Union
from .theme import Theme
from .filteredStringListModel import FilteredStringListModel

try:
    from PySide6.QtCore import (  # type: ignore
        QItemSelection,
        QItemSelectionModel,
        QModelIndex,
        QPersistentModelIndex,
        QSize,
        QStringListModel,
        Qt,
        Signal,
    )
    from PySide6.QtGui import QPainter, QPaintEvent, QPen, QColor  # type: ignore
    from PySide6.QtWidgets import (  # type: ignore
        QStyleOptionViewItem,
        QStyledItemDelegate,
        QListView,
    )
except:
    from PySide2.QtCore import (  # type: ignore
        QItemSelection,
        QItemSelectionModel,
        QModelIndex,
        QPersistentModelIndex,
        QSize,
        QStringListModel,
        Qt,
        Signal,
    )
    from PySide2.QtGui import QPainter, QPaintEvent, QPen, QColor  # type: ignore
    from PySide2.QtWidgets import QStyleOptionViewItem, QStyledItemDelegate, QListView  # type: ignore


kNoObjectFoundLabel = "No objects found"


class FilteredStringListView(QListView):
    selectionChanged = Signal()

    class Delegate(QStyledItemDelegate):
        def __init__(self, model: QStringListModel, parent=None):
            super(FilteredStringListView.Delegate, self).__init__(parent)
            self._model = model

        def sizeHint(
            self,
            option: QStyleOptionViewItem,
            index: Union[QModelIndex, QPersistentModelIndex],
        ):
            s: int = Theme.instance().uiScaled(24)
            return QSize(s, s)

        def paint(
            self,
            painter: QPainter,
            option: QStyleOptionViewItem,
            index: Union[QModelIndex, QPersistentModelIndex],
        ):
            s: str = self._model.data(index, Qt.DisplayRole)
            Theme.instance().paintStringListEntry(painter, option.rect, s)

    def __init__(self, items: Sequence[str] = [], headerTitle: str = "", parent=None):
        super(FilteredStringListView, self).__init__(parent)
        self._model = FilteredStringListModel(items, self)
        self.setModel(self._model)
        self.headerTitle = headerTitle

        self.setUniformItemSizes(True)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setTextElideMode(Qt.TextElideMode.ElideMiddle)
        self.setSelectionBehavior(QListView.SelectRows)
        self.setSelectionMode(QListView.MultiSelection)
        self.setContentsMargins(0, 0, 0, 0)

        self.setCursor(Qt.ArrowCursor)

        self.selectionModel().selectionChanged.connect(
            lambda: self.selectionChanged.emit()
        )

    def drawFrame(self, painter: QPainter):
        pass

    @property
    def items(self) -> Sequence[str]:
        return self._model.stringList

    @items.setter
    def items(self, items: Sequence[str]):
        # save the selected items
        prevSelectedIndices = []
        for s in self.selectedItems:
            try:
                newIdx = items.index(s)
            except ValueError:
                continue
            prevSelectedIndices.append(newIdx)

        self._model.setStringList(items)
        # restore the selected items
        newSelection = QItemSelection()
        for i in prevSelectedIndices:
            idx = self._model.index(i, 0)
            newSelection.select(idx, idx)
        self.selectionModel().select(newSelection, QItemSelectionModel.ClearAndSelect)

    @property
    def selectedItems(self) -> Sequence[str]:
        return [str(index.data(Qt.DisplayRole)) for index in self.selectedIndexes()]

    @property
    def hasSelectedItems(self) -> bool:
        return self.selectionModel().hasSelection()

    def update_placeholder(self):
        """Show or hide placeholder based on the model's content."""
        model = self.model()
        # if model and model.rowCount() == 0:
        #    self.placeholder_label.show()
        # else:
        #    self.placeholder_label.hide()

    def resizeEvent(self, event):
        """Ensure placeholder is centered on resize."""
        super().resizeEvent(event)
        # if self.placeholder_label.isVisible():
        #    self.placeholder_label.setGeometry(self.viewport().geometry())
