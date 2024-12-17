from typing import Sequence, Union
from .theme import Theme
from .host import Host
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
    from PySide6.QtGui import QPalette, QPainter, QPaintEvent  # type: ignore
    from PySide6.QtWidgets import (  # type: ignore
        QFrame,
        QLabel,
        QListView,
        QStyledItemDelegate,
        QStyleOptionViewItem
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
    from PySide2.QtGui import QPalette, QPainter, QPaintEvent  # type: ignore
    from PySide2.QtWidgets import QFrame, QLabel, QListView, QStyledItemDelegate, QStyleOptionViewItem  # type: ignore


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
        self.headerTitle = headerTitle
        self._model = FilteredStringListModel(items, self)
        self.setModel(self._model)
        self._model.filterChanged.connect(self.updatePlaceholder)

        self.setUniformItemSizes(True)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setVerticalScrollMode(QListView.ScrollMode.ScrollPerPixel)
        self.setTextElideMode(Qt.TextElideMode.ElideMiddle)
        self.setSelectionBehavior(QListView.SelectRows)
        self.setSelectionMode(QListView.MultiSelection)
        self.setContentsMargins(1, 0, 1, 1)

        self.setCursor(Qt.ArrowCursor)
        
        self.selectionModel().selectionChanged.connect(
            lambda: self.selectionChanged.emit()
        )

        if Host.instance().canDrop and Host.instance().canPick:
            self.placeholder_label = QLabel(
                "Drag objects here or click '+' to add", self
            )
        elif Host.instance().canDrop:
            self.placeholder_label = QLabel("Drag objects here", self)
        elif Host.instance().canPick:
            self.placeholder_label = QLabel("Click '+' to add", self)
        else:
            self.placeholder_label = None

        if self.placeholder_label is not None:
            self.placeholder_label.setAlignment(Qt.AlignmentFlag.AlignCenter)

            palette: QPalette = self.placeholder_label.palette()
            palette.setColor(
                self.placeholder_label.foregroundRole(),
                Theme.instance().palette.colorPlaceHolderText,
            )
            self.placeholder_label.setPalette(palette)
            self.placeholder_label.hide()

    def paintEvent(self, event: QPaintEvent):
        painter = QPainter(self)
        painter.fillRect(event.rect(), Theme.instance().palette.colorListBorder)
        super(FilteredStringListView, self).paintEvent(event)
        if self._model.isFilteredEmpty():
            painter = QPainter(self.viewport())
            painter.setPen(Theme.instance().palette.colorPlaceHolderText)
            painter.drawText(self.rect(), Qt.AlignCenter, kNoObjectFoundLabel)

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
        self.updatePlaceholder()

    @property
    def selectedItems(self) -> Sequence[str]:
        return [str(index.data(Qt.DisplayRole)) for index in self.selectedIndexes()]

    @property
    def hasSelectedItems(self) -> bool:
        return self.selectionModel().hasSelection()

    def updatePlaceholder(self):
        """Show or hide placeholder based on the model's content."""
        if self.placeholder_label is not None:
            model = self.model()
            if model and model.rowCount() == 0:
                self.placeholder_label.show()
            else:
                self.placeholder_label.hide()

    def resizeEvent(self, event):
        """Ensure placeholder is centered on resize."""
        super().resizeEvent(event)
        if self.placeholder_label is not None:
            self.placeholder_label.setGeometry(self.viewport().geometry())
