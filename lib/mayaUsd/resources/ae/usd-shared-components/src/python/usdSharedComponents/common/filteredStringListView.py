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
        Qt,
        Signal,
        QMimeData
    )
    from PySide6.QtGui import QDrag, QPaintEvent # type: ignore
    from PySide6.QtWidgets import (  # type: ignore
        QLabel,
        QListView,
        QLineEdit,
        QListWidget
    )
except:
    from PySide2.QtCore import (  # type: ignore
        QEvent,
        QModelIndex,
        QObject,
        Qt,
        Signal,
        QMimeData
    )
    from PySide2.QtGui import QDrag, QPaintEvent # type: ignore
    from PySide2.QtWidgets import QLabel, QListView, QLineEdit, QListWidget  # type: ignore


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
        self._editLine = None
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

        DragAndDropAndDeleteEventFilter(self, data)

        self.selectionModel().selectionChanged.connect(self.itemSelectionChanged)

    def mouseDoubleClickEvent(self, event):
        index = self.indexAt(event.pos())
        if index.isValid():
            self._openEditor(index)

    def _openEditor(self, index: QModelIndex):
        if self._editLine:
            self._editLine.deleteLater()

        self._editLine = QLineEdit(self)
        self._editLine.setText(self._model.data(index, Qt.DisplayRole))
        self._editLine.setGeometry(self.visualRect(index))
        self._editLine.setFocus()
        self._editLine.show()

        self._suggestionList = QListWidget(self)
        currentPos = self._suggestionList.pos()
        newX = currentPos.x()
        newY = currentPos.y() + self._editLine.height()*(index.row()+1)
        self._suggestionList.move(newX, newY)

        # Stretch the suggestion Bar to Be the same length as the edit_line
        self._suggestionList.setFixedWidth(self._editLine.width())
        self.suggestions = self.model()._collData.getSuggestions()
        self.filteredSuggestions = []

        self._editLine.textChanged.connect(self._updateSuggestion)
        self._suggestionList.itemClicked.connect(lambda item:self._selectSuggestion(item, index))
        self._editLine.returnPressed.connect(lambda: self._closeEditor(index))

    def _closeEditor(self, index: QModelIndex):
        oldValue = self.model().data(index, Qt.DisplayRole)
        newValue = self._editLine.text()

        if not newValue.strip():
            newValue = oldValue
        self._model.setData(index, newValue, Qt.EditRole)
        self._saveSuggestions(oldValue, newValue)
        self._editLine.deleteLater()
        self._editLine = None

    def _updateSuggestion(self, text):
        if not text:
            self._suggestionList.hide()
            return

        self.filteredSuggestions = [s.pathString for s in self.suggestions if text.lower() in s.pathString]
        self._suggestionList.clear()
        self._suggestionList.addItems(self.filteredSuggestions[:5])
        self._suggestionList.setVisible(bool(self.filteredSuggestions))
    
    def _saveSuggestions(self, oldTarget, newTarget):
        if oldTarget == newTarget:
            return

        if not self.model()._collData:
            return

        self.model()._collData.replaceStrings(oldTarget, newTarget)

    def _selectSuggestion(self, item, index):
        self._editLine.setText(item.text())
        self._suggestionList.hide()
        self._closeEditor(index)

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
        self.placeholder_label.setText(placeHolderText)
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
