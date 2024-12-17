from typing import Sequence

from ..common.list import StringList
from ..common.resizable import Resizable
from ..common.menuButton import MenuButton
from .expressionRulesMenu import ExpressionMenu
from ..common.host import Host
from ..common.theme import Theme

try:
    from PySide6.QtCore import QEvent, QObject, Qt  # type: ignore
    from PySide6.QtWidgets import QFrame, QHBoxLayout, QVBoxLayout, QLineEdit, QMenu, QSizePolicy, QToolButton, QWidget  # type: ignore
except ImportError:
    from PySide2.QtCore import QEvent, QObject, Qt  # type: ignore
    from PySide2.QtWidgets import QFrame, QHBoxLayout, QVBoxLayout, QLineEdit, QMenu, QSizePolicy, QToolButton, QWidget  # type: ignore

from pxr import Usd, Sdf

# TODO: support I8N
kSearchPlaceHolder = "Search..."

class IncludeExcludeWidget(QWidget):
    def __init__(
        self,
        prim: Usd.Prim = None,
        collection: Usd.CollectionAPI = None,
        parent: QWidget = None,
    ):
        super(IncludeExcludeWidget, self).__init__(parent)
        self._collection: Usd.CollectionAPI = collection
        self._prim: Usd.Prim = prim
        self._updatingUI = False

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(0, 0, 0, 0)

        self._expressionMenu = ExpressionMenu(self._collection, self)
        menuButton = MenuButton(self._expressionMenu, self)

        self._filterWidget = QLineEdit()
        self._filterWidget.setSizePolicy(
            QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed
        )
        self._filterWidget.setPlaceholderText(kSearchPlaceHolder)
        self._filterWidget.setClearButtonEnabled(True)

        separator = QFrame()
        separator.setFrameShape(QFrame.VLine)

        headerWidget = QWidget(self)
        headerLayout = QHBoxLayout(headerWidget)

        if Host.instance().canPick:
            addBtn = QToolButton(headerWidget)
            addBtn.setToolTip("Add Objects to the Include/Exclude list")
            addBtn.setIcon(Theme.instance().icon("add"))
            addBtn.setPopupMode(QToolButton.InstantPopup)
            addBtnMenu = QMenu(addBtn)
            addBtnMenu.addAction("Include Objects...", self.onAddToIncludePrimClicked)
            addBtnMenu.addAction("Exclude Objects...", self.onAddToExcludePrimClicked)
            addBtn.setMenu(addBtnMenu)
            headerLayout.addWidget(addBtn)

        self._deleteBtn = QToolButton(headerWidget)
        self._deleteBtn.setToolTip("Remove Selected Objects from Include/Exclude list")
        self._deleteBtn.setIcon(Theme.instance().icon("delete"))
        self._deleteBtn.setPopupMode(QToolButton.InstantPopup)
        self._deleteBtnMenu = QMenu(self._deleteBtn)
        self._deleteBtnActionFromIncludes = self._deleteBtnMenu.addAction(
            "Remove Selected Objects from Include", self.onRemoveSelectionFromInclude
        )
        self._deleteBtnActionFromExcludes = self._deleteBtnMenu.addAction(
            "Remove Selected Objects from Exclude", self.onRemoveSelectionFromExclude
        )
        self._deleteBtn.setMenu(self._deleteBtnMenu)
        headerLayout.addWidget(self._deleteBtn)

        self._deleteBtn.setEnabled(False)

        headerLayout.addWidget(self._filterWidget)
        headerLayout.addWidget(separator)
        headerLayout.addWidget(menuButton)
        mainLayout.addWidget(headerWidget)

        self._include = StringList([], "Include", "Include all", self)
        self._include.cbIncludeAll.stateChanged.connect(self.onIncludeAllToggle)
        self._resizableInclude = Resizable(
            self._include,
            "USD_Light_Linking",
            "IncludeListHeight",
            self,
            defaultSize=Theme.instance().uiScaled(80),
        )
        self._resizableInclude.minContentSize = Theme.instance().uiScaled(44)
        mainLayout.addWidget(self._resizableInclude)

        self._exclude = StringList([], "Exclude", "", self)
        self._resizableExclude = Resizable(
            self._exclude,
            "USD_Light_Linking",
            "ExcludeListHeight",
            self,
            defaultSize=Theme.instance().uiScaled(80),
        )
        self._resizableExclude.minContentSize = Theme.instance().uiScaled(44)
        mainLayout.addWidget(self._resizableExclude)

        self._include.list.selectionChanged.connect(self.onListSelectionChanged)
        self._exclude.list.selectionChanged.connect(self.onListSelectionChanged)

        self._filterWidget.textChanged.connect(self._include.list._model.setFilter)
        self._filterWidget.textChanged.connect(self._exclude.list._model.setFilter)

        if Host.instance().canDrop:
            EventFilter(self._include.list, self)
            EventFilter(self._exclude.list, self)

        self.updateUI()
        self.onListSelectionChanged()

    def update(self):
        self._expressionMenu.update()
        includes = []
        excludes = []

        if self._collection is not None:
            includeRootAttribute = self._collection.GetIncludeRootAttr()
            if includeRootAttribute.IsAuthored():
                shouldIncludeAll = self._collection.GetIncludeRootAttr().Get()

            for p in self._collection.GetIncludesRel().GetTargets():
                includes.append(p.pathString)
            for p in self._collection.GetExcludesRel().GetTargets():
                excludes.append(p.pathString)

        self._include.list.items = includes
        self._exclude.list.items = excludes
        self._include.list.updatePlaceholder()
        self._exclude.list.updatePlaceholder()

    def getIncludedItems(self):
        return self._include.list.items()

    def getExcludedItems(self):
        return self._exclude.list.items()

    def getIncludeAll(self):
        return self._include.cbIncludeAll.isChecked()

    def setIncludeAll(self, value: bool):
        self._include.cbIncludeAll.setChecked(value)

    def updateUI(self):

        if self._updatingUI:
            return

        self._updatingUI = True

        # update the include list
        includes = []
        for p in self._collection.GetIncludesRel().GetTargets():
            includes.append(p.pathString)
        self._include.list.items = includes

        # update the exclude list
        excludes = []
        for p in self._collection.GetExcludesRel().GetTargets():
            excludes.append(p.pathString)
        self._exclude.list.items = excludes

        self._include.cbIncludeAll.setChecked(
            self._collection.GetIncludeRootAttr().Get()
        )

        self._updatingUI = False

    def onAddToIncludePrimClicked(self):
        pickedItems: Sequence[Usd.Prim] = Host.instance().pick(self._prim.GetStage(), dialogTitle="Add Include Objects")
        if pickedItems is None:
            return
        self._updatingUI = True
        for item in pickedItems:
            self._collection.GetIncludesRel().AddTarget(item.GetPath())
        self._updatingUI = False
        self.updateUI()

    def onAddToExcludePrimClicked(self):
        pickedItems: Sequence[Usd.Prim] = Host.instance().pick(self._prim.GetStage(), dialogTitle="Add Exclude Objects")
        if pickedItems is None:
            return
        self._updatingUI = True
        for item in pickedItems:
            self._collection.GetExcludesRel().AddTarget(item.GetPath())
        self._updatingUI = False
        self.updateUI()

    def onRemoveSelectionFromInclude(self):
        self._updatingUI = True
        for item in self._include.list.selectedItems:
            self._collection.GetIncludesRel().RemoveTarget(Sdf.Path(item))
        self._updatingUI = False
        self.updateUI()
        self.onListSelectionChanged()

    def onRemoveSelectionFromExclude(self):
        self._updatingUI = True
        for item in self._exclude.list.selectedItems:
            self._collection.GetExcludesRel().RemoveTarget(Sdf.Path(item))
        self._updatingUI = False
        self.updateUI()
        self.onListSelectionChanged()

    def onListSelectionChanged(self):
        includesSelected = self._include.list.hasSelectedItems
        excludeSelected = self._exclude.list.hasSelectedItems
        self._deleteBtn.setEnabled(includesSelected or excludeSelected)

        self._deleteBtnActionFromIncludes.setEnabled(includesSelected)
        self._deleteBtnActionFromExcludes.setEnabled(excludeSelected)

        try:
            self._deleteBtn.pressed.disconnect(self.onRemoveSelectionFromInclude)
            self._deleteBtn.pressed.disconnect(self.onRemoveSelectionFromExclude)
        except Exception:
            pass

        if includesSelected and excludeSelected:
            self._deleteBtn.setToolTip(
                "Remove Selected Objects from Include/Exclude list"
            )
            self._deleteBtn.setPopupMode(QToolButton.InstantPopup)
            self._deleteBtn.setStyleSheet("")
        else:
            if includesSelected:
                self._deleteBtn.setToolTip("Remove Selected Objects from Include")
                self._deleteBtn.pressed.connect(self.onRemoveSelectionFromInclude)
            elif excludeSelected:
                self._deleteBtn.setToolTip("Remove Selected Objects from Exclude")
                self._deleteBtn.pressed.connect(self.onRemoveSelectionFromExclude)
            self._deleteBtn.setPopupMode(QToolButton.DelayedPopup)
            self._deleteBtn.setStyleSheet(
                """QToolButton::menu-indicator { width: 0px; }"""
            )

    def onIncludeAllToggle(self, state: Qt.CheckState):
        if not self._updatingUI:
            self._collection.GetIncludeRootAttr().Set(state == Qt.Checked)


class EventFilter(QObject):
    def __init__(self, widget, control):
        super().__init__(widget)
        self._widget = widget
        self.control = control
        self.widget.installEventFilter(self)
        self.widget.setAcceptDrops(True)

    @property
    def widget(self):
        return self._widget

    def eventFilter(self, obj: QObject, event: QEvent):
        if event.type() == QEvent.Drop:
            mime_data = event.mimeData()
            self.addItemToCollection(mime_data.text())
            return True
        elif event.type() == QEvent.DragEnter:
            event.acceptProposedAction()
            return True
        elif event.type() == QEvent.DragMove:
            event.acceptProposedAction()
            return True

        return super().eventFilter(obj, event)

    def addItemToCollection(self, items):
        itemList = items.split("\n")
        for item in itemList:
            path = ""
            if "," in item:
                path = item.split(",")[1]
            else:
                path = item

            if not self._validatePath(self.control._collection, path):
                return

            if self.control._collection is not None:
                if self.widget.headerTitle == "Include":
                    self.control._collection.GetIncludesRel().AddTarget(path)
                elif self.widget.headerTitle == "Exclude":
                    self.control._collection.GetExcludesRel().AddTarget(path)

        self.control.update()

    def _validatePath(self, collection, path):

        if not Sdf.Path.IsValidPathString(path):
            raise ValueError("Invalid sdf path: " + path)

        stage = self.control._collection.GetPrim().GetStage()
        prim = stage.GetPrimAtPath(Sdf.Path(path))

        if not prim or not prim.IsValid():
            raise ValueError(
                "Error: The dragged object is not in the same stage as the collection. Ensure that objects belong to the same stage before adding them"
            )

        return True
