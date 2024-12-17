from typing import Sequence

from ..common.stringListPanel import StringListPanel
from ..common.resizable import Resizable
from ..common.menuButton import MenuButton
from .expressionRulesMenu import ExpressionMenu
from ..common.host import Host
from ..common.theme import Theme
from ..data.collectionData import CollectionData

try:
    from PySide6.QtCore import Qt  # type: ignore
    from PySide6.QtWidgets import QFrame, QHBoxLayout, QVBoxLayout, QLineEdit, QMenu, QSizePolicy, QToolButton, QWidget  # type: ignore
except ImportError:
    from PySide2.QtCore import Qt  # type: ignore
    from PySide2.QtWidgets import QFrame, QHBoxLayout, QVBoxLayout, QLineEdit, QMenu, QSizePolicy, QToolButton, QWidget  # type: ignore

# TODO: support I8N
kSearchPlaceHolder = "Search..."

class IncludeExcludeWidget(QWidget):
    def __init__(
        self,
        data: CollectionData,
        parent: QWidget = None,
    ):
        super(IncludeExcludeWidget, self).__init__(parent)
        self._collData = data

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(0, 0, 0, 0)

        self._expressionMenu = ExpressionMenu(data, self)
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

        self._include = StringListPanel(data.getIncludeData(), True, "Include", self)
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

        self._exclude = StringListPanel(data.getExcludeData(), False, "Exclude", self)
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
        self._collData.dataChanged.connect(self._onDataChanged)

        self.onListSelectionChanged()
        self._onDataChanged()

    def _onDataChanged(self):
        incAll = self._collData.includesAll()
        if incAll != self._include.cbIncludeAll.isChecked():
            self._include.cbIncludeAll.setChecked(incAll)

    def onAddToIncludePrimClicked(self):
        items = self._collData.pick("Add Include Objects")
        self._collData.getIncludeData().addStrings(items)

    def onAddToExcludePrimClicked(self):
        items = self._collData.pick("Add Exclude Objects")
        self._collData.getExcludeData().addStrings(items)

    def onRemoveSelectionFromInclude(self):
        self._collData.getIncludeData().removeStrings(self._include.list.selectedItems())
        self.onListSelectionChanged()

    def onRemoveSelectionFromExclude(self):
        self._collData.getExcludeData().removeStrings(self._include.list.selectedItems())
        self.onListSelectionChanged()

    def onListSelectionChanged(self):
        includesSelected = self._include.list.hasSelectedItems()
        excludeSelected = self._exclude.list.hasSelectedItems()
        self._deleteBtn.setEnabled(includesSelected or excludeSelected)

        # TODO: these QAction are reported as already being deleted in C++
        # self._deleteBtnActionFromIncludes.setEnabled(includesSelected)
        # self._deleteBtnActionFromExcludes.setEnabled(excludeSelected)

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

    def onIncludeAllToggle(self, _: Qt.CheckState):
        incAll = self._include.cbIncludeAll.isChecked()
        self._collData.setIncludeAll(incAll)


