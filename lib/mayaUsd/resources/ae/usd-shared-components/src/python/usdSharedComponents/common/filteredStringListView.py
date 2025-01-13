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
        QLabel,
        QListView,
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
    from PySide2.QtWidgets import QLabel, QListView, QStyleOptionViewItem  # type: ignore


NO_OBJECTS_FOUND_LABEL = "No objects found"
DRAG_OBJECTS_HERE_LABEL = "Drag objects here"
PICK_OBJECTS_LABEL = "Click '+' to add"
DRAG_OR_PICK_OBJECTS_LABEL = "Drag objects here or click '+' to add"


class FilteredStringListView(QListView):

    itemSelectionChanged = Signal()

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

        self.placeholder_label = QLabel(self)
        self.placeholder_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        palette = self.placeholder_label.palette()
        palette.setColor(
            self.placeholder_label.foregroundRole(),
            Theme.instance().palette.colorPlaceHolderText,
        )
        self.placeholder_label.setPalette(palette)
        self.placeholder_label.hide()

        self.setCursor(Qt.ArrowCursor)

        DragAndDropEventFilter(self, data)

        self.selectionModel().selectionChanged.connect(self.itemSelectionChanged)

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
            else:
                self.placeholder_label.hide()
        else:
            self.placeholder_label.hide()

    def _paintPlaceHolder(self, placeHolderText):
        self.placeholder_label.setText(placeHolderText)
        self.placeholder_label.setGeometry(self.viewport().geometry())
        self.placeholder_label.show()

    def selectedItems(self) -> Sequence[str]:
        return [str(index.data(Qt.DisplayRole)) for index in self.selectedIndexes()]

    def hasSelectedItems(self) -> bool:
        return bool(self.selectionModel().hasSelection())

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
