from typing import Sequence, Union
from .theme import Theme
from .host import Host
from .filteredStringListModel import FilteredStringListModel
from ..data.stringListData import StringListData

try:
    from PySide6.QtCore import (  # type: ignore
        QEvent,
        QObject,
        Qt,
        Signal,
        QMimeData
    )
    from PySide6.QtGui import QDrag, QPaintEvent # type: ignore
    from PySide6.QtWidgets import (  # type: ignore
        QLabel,
        QListView,
        QItemDelegate,
        QCompleter
    )
except:
    from PySide2.QtCore import (  # type: ignore
        QEvent,
        QObject,
        Qt,
        Signal,
        QMimeData
    )
    from PySide2.QtGui import QDrag, QPaintEvent # type: ignore
    from PySide2.QtWidgets import QLabel, QListView, QCompleter, QItemDelegate # type: ignore


NO_OBJECTS_FOUND_LABEL = "No objects found"
DRAG_OBJECTS_HERE_LABEL = "Drag objects here"
PICK_OBJECTS_LABEL = "Click '+' to add"
DRAG_OR_PICK_OBJECTS_LABEL = "Drag objects here or click '+' to add"

class FilteredStringDelegate(QItemDelegate):
    def __init__(self, listView, parent=None):
        super(FilteredStringDelegate, self).__init__(parent)
        self._listView = listView

    def createEditor(self, parent, option, index):
        editLine = super(FilteredStringDelegate, self).createEditor(parent, option, index)
        if editLine and self._listView:
            suggestions = self._listView.model()._collData.getSuggestions()
            self._completer = QCompleter(suggestions)
            editLine.setCompleter(self._completer)
        return editLine

    def setModelData(self, editor, model, index):
        oldValue = model.data(index, Qt.DisplayRole)
        newValue = editor.text().strip()
        if newValue:
            model._collData.replaceStrings(oldValue, newValue)
        super(FilteredStringDelegate, self).setModelData(editor, model, index)
        # Note: set focus on list view to avoid starting to edit items in outliner.
        self._listView.setFocus()

    def sizeHint(self, option, index):
        size = super(FilteredStringDelegate, self).sizeHint(option, index)
        size.setHeight(Theme.instance().uiScaled(24))
        return size


class FilteredStringListView(QListView):

    itemSelectionChanged = Signal()

    def __init__(self, data: StringListData, headerTitle: str = "", parent=None):
        super(FilteredStringListView, self).__init__(parent)
        self.headerTitle = headerTitle
        self._model = FilteredStringListModel(data, self)
        self.setModel(self._model)
        self.setItemDelegate(FilteredStringDelegate(self))

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

        DragAndDropAndDeleteEventFilter(self, data)

        self.selectionModel().selectionChanged.connect(self.itemSelectionChanged)

    def paintEvent(self, event: QPaintEvent):
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
        theme = Theme.instance()
        self.placeholder_label.setText(theme.themeLabel(placeHolderText))
        self.placeholder_label.setGeometry(self.viewport().geometry())
        self.placeholder_label.show()

    def startDrag(self, supportedActions):
        drag = QDrag(self)
        mimeData = QMimeData()

        selectedIndexes = self.selectedIndexes()
        if selectedIndexes:
            mimeData.setText("\n".join([index.data() for index in selectedIndexes]))

        drag.setMimeData(mimeData)
        drag.exec(Qt.MoveAction)

    def selectedItems(self) -> Sequence[str]:
        return [str(index.data(Qt.DisplayRole)) for index in self.selectedIndexes()]

    def hasSelectedItems(self) -> bool:
        return bool(self.selectionModel().hasSelection())

class DragAndDropAndDeleteEventFilter(QObject):
    def __init__(self, widget, data: StringListData):
        super().__init__(widget)
        self._collData = data
        widget.installEventFilter(self)
        widget.setAcceptDrops(True)
        widget.setDragEnabled(True)
        widget.setDropIndicatorShown(True)

    def eventFilter(self, obj: QObject, event: QEvent):
        if event.type() == QEvent.Drop:
            mime_data = event.mimeData()
            draggedItems = mime_data.text()

            # The source model will exist when draggin/dropping from within lists.
            # For example: dragging from include to exclude list and vice-versa.
            try:
                sourceModel = event.source().model()

                # prevent dropping elements from the same list
                if sourceModel and sourceModel._collData == self._collData:
                    return False

                # Also remove the elements dragged from the original list
                if sourceModel:
                    sourceModel._collData.removeMultiLineStrings(draggedItems)
            except Exception:
                pass

            self._collData.addMultiLineStrings(draggedItems)

            return True
        elif event.type() == QEvent.DragEnter:
            event.acceptProposedAction()
            return True
        elif event.type() == QEvent.DragMove:
            event.acceptProposedAction()
            return True
        elif event.type() == QEvent.KeyPress:
            # These are to prevent deleting the overall DCC selection when
            # an item in the listview is selected.
            if event.key() == Qt.Key_Delete or event.key() == Qt.Key_Backspace:
                return True

        return super().eventFilter(obj, event)
