from typing import Sequence, Union
from .theme import Theme

try:
    from PySide6.QtCore import (
        QModelIndex,
        QPersistentModelIndex,
        QSize,
        QStringListModel,
        Qt,
        Signal,
    )
    from PySide6.QtGui import QPainter
    from PySide6.QtWidgets import QStyleOptionViewItem, QStyledItemDelegate, QListView, QLabel, QVBoxLayout, QHBoxLayout, QWidget, QCheckBox
except:
    from PySide2.QtCore import ( # type: ignore
        QModelIndex,
        QPersistentModelIndex,
        QSize,
        QStringListModel,
        Qt,
        Signal,
    )
    from PySide2.QtGui import QPainter  # type: ignore
    from PySide2.QtWidgets import QStyleOptionViewItem, QStyledItemDelegate, QListView, QLabel, QVBoxLayout, QHBoxLayout, QWidget, QCheckBox  # type: ignore

class _StringList(QListView):
    selectedItemsChanged = Signal()

    class Delegate(QStyledItemDelegate):
        def __init__(self, model: QStringListModel, parent=None):
            super(_StringList.Delegate, self).__init__(parent)
            self._model = model

        def sizeHint(self, option: QStyleOptionViewItem, index: Union[QModelIndex, QPersistentModelIndex]):
            s: int = Theme.instance().uiScaled(24)
            return QSize(s, s)

        def paint(self, painter: QPainter, option: QStyleOptionViewItem, index: Union[QModelIndex, QPersistentModelIndex]):
            s: str = self._model.data(index, Qt.DisplayRole)
            Theme.instance().paintStringListEntry(painter, option.rect, s)

    def __init__(self, items: Sequence[str] = [],  headerTitle:str = "", parent=None):
        super(_StringList, self).__init__(parent)
        self._model = QStringListModel(items, self)
        self.setModel(self._model)
        self.headerTitle = headerTitle

        self.setUniformItemSizes(True)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setTextElideMode(Qt.TextElideMode.ElideMiddle)
        self.setSelectionBehavior(QListView.SelectRows)
        self.setSelectionMode(QListView.MultiSelection)
        self.setContentsMargins(0,0,0,0)
        self.setMovement(QListView.Free)

        self.selectionModel().selectionChanged.connect(lambda: self.selectedItemsChanged.emit())

        #                
        self.placeholder_label = QLabel("Drag objects here or click “+” to add", self)
        self.placeholder_label.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.placeholder_label.setStyleSheet("color: gray; font-size: 18px;")
        self.placeholder_label.hide()

    def drawFrame(self, painter: QPainter):
        pass

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


class StringList(QWidget):
    _dragging = False
    _mimeData = None
    def __init__(self, items: Sequence[str] = [], headerTitle: str = "", toggleTitle: str = "", parent=None):
        super().__init__()
        #self.setAcceptDrops(True)
        #self.installEventFilter(self)

        self.list = _StringList(items, headerTitle, self)
        self.list.update_placeholder()

        layout = QVBoxLayout(self)
        LEFT_RIGHT_MARGINS = 2
        layout.setContentsMargins(LEFT_RIGHT_MARGINS, 0, LEFT_RIGHT_MARGINS, 0)

        headerWidget = QWidget(self)
        headerLayout = QHBoxLayout(headerWidget)

        titleLabel = QLabel(headerTitle, self)
        headerLayout.addWidget(titleLabel)
        headerLayout.addStretch(1)
        headerLayout.setContentsMargins(0,0,0,0)

        # only add the check box on the header if there's a label
        if toggleTitle != None and toggleTitle != "":
            self.cbIncludeAll = QCheckBox(toggleTitle, self)
            self.cbIncludeAll.setCheckable(True)
            headerLayout.addWidget(self.cbIncludeAll)

        headerWidget.setLayout(headerLayout)

        layout.addWidget(headerWidget)
        layout.addWidget(self.list)
        self.setLayout(layout)
