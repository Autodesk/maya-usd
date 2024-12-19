from typing import Sequence, Union
from .theme import Theme
from .host import Host
from .filteredStringListModel import FilteredStringListModel
from ..data.stringListData import StringListData

try:
    from PySide6.QtCore import (  # type: ignore
        QEvent,
        QModelIndex,
        QObject,
        QPersistentModelIndex,
        QSize,
        QStringListModel,
        Qt,
        Signal,
    )
    from PySide6.QtGui import QPainter, QPaintEvent, QFont  # type: ignore
    from PySide6.QtWidgets import (  # type: ignore
        QListView,
        QStyledItemDelegate,
        QStyleOptionViewItem
    )
except:
    from PySide2.QtCore import (  # type: ignore
        QEvent,
        QModelIndex,
        QObject,
        QPersistentModelIndex,
        QSize,
        QStringListModel,
        Qt,
        Signal,
    )
    from PySide2.QtGui import QPainter, QPaintEvent, QFont  # type: ignore
    from PySide2.QtWidgets import QListView, QStyledItemDelegate, QStyleOptionViewItem  # type: ignore


NO_OBJECTS_FOUND_LABEL = "No objects found"
DRAG_OBJECTS_HERE_LABEL = "Drag objects here"
PICK_OBJECTS_LABEL = "Click '+' to add"
DRAG_OR_PICK_OBJECTS_LABEL = "Drag objects here or click '+' to add"


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
        self.headerTitle = headerTitle
        self._model = FilteredStringListModel(data, self)
        self.setModel(self._model)

        self.setUniformItemSizes(True)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.setVerticalScrollMode(QListView.ScrollMode.ScrollPerPixel)
        self.setTextElideMode(Qt.TextElideMode.ElideMiddle)
        self.setSelectionBehavior(QListView.SelectionBehavior.SelectRows)
        self.setSelectionMode(QListView.SelectionMode.ExtendedSelection)
        self.setContentsMargins(1, 0, 1, 1)

        self.setCursor(Qt.ArrowCursor)

        DragAndDropEventFilter(self, data)

        self.selectionModel().selectionChanged.connect(
            lambda: self.selectionChanged.emit()
        )

    def paintEvent(self, event: QPaintEvent):
        painter = QPainter(self)
        painter.fillRect(event.rect(), Theme.instance().palette.colorListBorder)
        super(FilteredStringListView, self).paintEvent(event)

        model = self.model()
        if not model:
            return

        if model.isFilteredEmpty():
            self._paintPlaceHolder(NO_OBJECTS_FOUND_LABEL)
        elif model.rowCount() == 0:
            if Host.instance().canDrop and Host.instance().canPick:
                self._paintPlaceHolder(DRAG_OR_PICK_OBJECTS_LABEL)
            elif Host.instance().canDrop:
                self._paintPlaceHolder(DRAG_OBJECTS_HERE_LABEL)
            elif Host.instance().canPick:
                self._paintPlaceHolder(PICK_OBJECTS_LABEL)

    def selectedItems(self) -> Sequence[str]:
        return [str(index.data(Qt.DisplayRole)) for index in self.selectedIndexes()]

    def hasSelectedItems(self) -> bool:
        return bool(self.selectionModel().hasSelection())

    def _paintPlaceHolder(self, placeHolderText):
        painter = QPainter(self.viewport())
        theme = Theme.instance()
        painter.setPen(theme.palette.colorPlaceHolderText)
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
