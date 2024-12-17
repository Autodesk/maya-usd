from typing import Sequence, Union
from .theme import Theme
from .filteredStringListModel import FilteredStringListModel
from maya.OpenMaya import MGlobal

try:
    from PySide6.QtCore import (
        QModelIndex,
        QPersistentModelIndex,
        QRect,
        QSize,
        QStringListModel,
        Qt,
        Signal,
    )
    from PySide6.QtGui import QPainter, QPaintEvent, QPen, QColor
    from PySide6.QtWidgets import QStyleOptionViewItem, QStyledItemDelegate, QListView, QLabel
except:
    from PySide2.QtCore import (
        QModelIndex,
        QPersistentModelIndex,
        QRect,
        QSize,
        QStringListModel,
        Qt,
        Signal,
    )
    from PySide2.QtGui import QPainter, QPaintEvent, QPen, QColor  # type: ignore
    from PySide2.QtWidgets import QStyleOptionViewItem, QStyledItemDelegate, QListView


kNoObjectFoundLabel = 'No objects found'

class FilteredStringListView(QListView):
    selectedItemsChanged = Signal()

    class Delegate(QStyledItemDelegate):
        def __init__(self, model: QStringListModel, parent=None):
            super(FilteredStringListView.Delegate, self).__init__(parent)
            self._model = model

        def sizeHint(self, option: QStyleOptionViewItem, index: Union[QModelIndex, QPersistentModelIndex]):
            s: int = Theme.instance().uiScaled(24)
            return QSize(s, s)

        def paint(self, painter: QPainter, option: QStyleOptionViewItem, index: Union[QModelIndex, QPersistentModelIndex]):
            s: str = self._model.data(index, Qt.DisplayRole)
            Theme.instance().paintStringListEntry(painter, option.rect, s)

    def __init__(self, items: Sequence[str] = None,  headerTitle:str = "", parent=None):
        super(FilteredStringListView, self).__init__(parent)
        self._model = FilteredStringListModel(items if items else [], self, headerTitle)
        self.setModel(self._model)
        self.headerTitle = headerTitle

        self.setUniformItemSizes(True)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setTextElideMode(Qt.TextElideMode.ElideMiddle)
        self.setSelectionBehavior(QListView.SelectRows)
        self.setSelectionMode(QListView.MultiSelection)
        self.setContentsMargins(0,0,0,0)
        self.setDragDropMode(QListView.DragDrop)
        self.setDefaultDropAction(Qt.MoveAction)

        self.selectionModel().selectionChanged.connect(lambda: self.selectedItemsChanged.emit())

        self.placeholder_label = QLabel("Drag objects here or click “+” to add", self)
        self.placeholder_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.placeholder_label.setStyleSheet("color: gray; font-size: 18px;")
        self.placeholder_label.hide()
        
    def drawFrame(self, painter: QPainter):
        pass

    def paintEvent(self, event: QPaintEvent):
        super(FilteredStringListView, self).paintEvent(event)
        if self._model.isFilteredEmpty():
            painter = QPainter(self.viewport())
            painter.setPen(QColor(128, 128, 128))
            painter.drawText(self.rect(), Qt.AlignCenter, kNoObjectFoundLabel)

    @property
    def items(self) -> Sequence[str]:
        return self._model.stringList

    @items.setter
    def items(self, items: Sequence[str]):
        self._model.setStringList(items)

    @property
    def selectedItems(self) -> Sequence[str]:
        return [index.data(Qt.DisplayRole) for index in self.selectedIndexes()]

    def update_placeholder(self):
        """Show or hide placeholder based on the model's content."""
        model = self.model()
        if model and model.rowCount() == 0:
            self.placeholder_label.show()
        else:
            self.placeholder_label.hide()

    def resizeEvent(self, event):
        """Ensure placeholder is centered on resize."""
        super().resizeEvent(event)
        if self.placeholder_label.isVisible():
            self.placeholder_label.setGeometry(self.viewport().geometry())