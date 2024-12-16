from typing import Sequence, Union
from .theme import Theme
from .filteredStringListModel import FilteredStringListModel
from ..data.stringListData import StringListData

try:
    from PySide6.QtCore import (  # type: ignore
        QEvent,
        QItemSelection,
        QItemSelectionModel,
        QModelIndex,
        QObject,
        QPersistentModelIndex,
        QSize,
        QStringListModel,
        Qt,
        Signal,
    )
    from PySide6.QtGui import QPainter, QPaintEvent, QPen, QColor, QFont  # type: ignore
    from PySide6.QtWidgets import (  # type: ignore
        QStyleOptionViewItem,
        QStyledItemDelegate,
        QListView,
        QLabel
    )
except:
    from PySide2.QtCore import (  # type: ignore
        QEvent,
        QItemSelection,
        QItemSelectionModel,
        QModelIndex,
        QObject,
        QPersistentModelIndex,
        QSize,
        QStringListModel,
        Qt,
        Signal,
    )
    from PySide2.QtGui import QPainter, QPaintEvent, QPen, QColor, QFont  # type: ignore
    from PySide2.QtWidgets import QStyleOptionViewItem, QStyledItemDelegate, QListView, QLabel  # type: ignore


kNoObjectFoundLabel = "No objects found"
kDragObjectsHereLabel = "Drag objects here or click “+” to add"


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

    def __init__(self, data: StringListData, headerTitle: str = "", parent=None):
        super(FilteredStringListView, self).__init__(parent)
        self._model = FilteredStringListModel(data, self)
        self.setModel(self._model)
        self.headerTitle = headerTitle

        self.setUniformItemSizes(True)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.setTextElideMode(Qt.TextElideMode.ElideMiddle)
        self.setSelectionBehavior(QListView.SelectionBehavior.SelectRows)
        self.setSelectionMode(QListView.SelectionMode.ExtendedSelection)
        self.setContentsMargins(0, 0, 0, 0)

        self.setCursor(Qt.ArrowCursor)

        DragAndDropEventFilter(self, data)

        self.selectionModel().selectionChanged.connect(
            lambda: self.selectionChanged.emit()
        )

    def drawFrame(self, painter: QPainter):
        pass

    @property
    def selectedItems(self) -> Sequence[str]:
        return [str(index.data(Qt.DisplayRole)) for index in self.selectedIndexes()]

    @property
    def hasSelectedItems(self) -> bool:
        return self.selectionModel().hasSelection()

    def paintEvent(self, event: QPaintEvent):
        super(FilteredStringListView, self).paintEvent(event)

        model = self.model()
        if not model:
            return
    
        if model.isFilteredEmpty():
            self._paintPlaceHolder(kNoObjectFoundLabel)
        elif model.rowCount() == 0:
            self._paintPlaceHolder(kDragObjectsHereLabel)

    def _paintPlaceHolder(self, placeHolderText):
        painter = QPainter(self.viewport())
        theme = Theme.instance()
        painter.setPen(theme.palette.colorPlaceHolderText)
        font: QFont = painter.font()
        font.setPixelSize(theme.uiScaled(18))
        painter.setFont(font)
        painter.drawText(self.rect(), Qt.AlignCenter, placeHolderText)

class DragAndDropEventFilter(QObject):
    def __init__(self, widget, data: StringListData):
        super().__init__(widget)
        self._collData = data
        widget.installEventFilter(self)
        widget.setAcceptDrops(True)

    def eventFilter(self, obj: QObject, event: QEvent):
        if event.type() == QEvent.Drop:
            mime_data = event.mimeData()
            self._collData.addMultiLineStrings(mime_data.text())
            return True
        elif event.type() == QEvent.DragEnter:
            event.acceptProposedAction()
            return True
        elif event.type() == QEvent.DragMove:
            event.acceptProposedAction()
            return True

        return super().eventFilter(obj, event)
