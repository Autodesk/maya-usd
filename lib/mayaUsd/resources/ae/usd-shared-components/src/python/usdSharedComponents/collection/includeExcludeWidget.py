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
    from PySide6.QtGui import QAction # type: ignore
except ImportError:
    from PySide2.QtCore import Qt  # type: ignore
    from PySide2.QtWidgets import QFrame, QHBoxLayout, QVBoxLayout, QLineEdit, QMenu, QSizePolicy, QToolButton, QWidget  # type: ignore
    from PySide2.QtGui import QAction # type: ignore

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

        includeExcludeLayout = QVBoxLayout(self)

        includeExcludeLayout.setContentsMargins(0, 0, 0, 0)

        self._expressionMenu = ExpressionMenu(data, self)
        menuButton = MenuButton(self._expressionMenu, self)

        self._filterWidget = QLineEdit()
        self._filterWidget.setContentsMargins(0, 0, 0, 0)
        self._filterWidget.setSizePolicy(
            QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed
        )
        self._filterWidget.setPlaceholderText(kSearchPlaceHolder)
        self._filterWidget.setClearButtonEnabled(True)

        separator = QFrame()
        separator.setFrameShape(QFrame.VLine)

        headerWidget = QWidget(self)
        headerWidget.setContentsMargins(0, 0, 0, 0)
        headerLayout = QHBoxLayout(headerWidget)

        headerLayout.setContentsMargins(0, 2, 2, 0)

        if Host.instance().canPick:
            addBtn = QToolButton(headerWidget)
            addBtn.setToolTip("Add prims to Include or Exclude")
            addBtn.setIcon(Theme.instance().icon("add"))
            addBtn.setPopupMode(QToolButton.InstantPopup)
            addBtnMenu = QMenu(addBtn)
            addBtnMenu.addAction("Add prims to Include", self.onAddToIncludePrimClicked)
            addBtnMenu.addAction("Add prims to Exclude", self.onAddToExcludePrimClicked)
            addBtn.setMenu(addBtnMenu)
            headerLayout.addWidget(addBtn)

        self._deleteBtn = QToolButton(headerWidget)
        self._deleteBtn.setToolTip("Remove selected prims from Include or Exclude")
        self._deleteBtn.setIcon(Theme.instance().icon("delete"))
        self._deleteBtn.setPopupMode(QToolButton.InstantPopup)
        self._deleteBtnMenu = QMenu(self._deleteBtn)
        self._deleteBtnActionFromIncludes = QAction("Remove selected prims from Include", self._deleteBtnMenu)
        self._deleteBtnActionFromIncludes.triggered.connect(self.onRemoveSelectionFromInclude)
        self._deleteBtnMenu.addAction(self._deleteBtnActionFromIncludes)
        self._deleteBtnActionFromExcludes = QAction("Remove selected prims from Exclude", self._deleteBtnMenu)
        self._deleteBtnActionFromExcludes.triggered.connect(self.onRemoveSelectionFromExclude)
        self._deleteBtnMenu.addAction(self._deleteBtnActionFromExcludes)
        self._deleteBtn.setMenu(self._deleteBtnMenu)
        headerLayout.addWidget(self._deleteBtn)

        self._deleteBtn.setEnabled(False)

        headerLayout.addWidget(self._filterWidget)
        headerLayout.addWidget(separator)
        headerLayout.addWidget(menuButton)
        includeExcludeLayout.addWidget(headerWidget)

        self._include = StringListPanel(data.getIncludeData(), True, "Include", self)
        self._include.cbIncludeAll.stateChanged.connect(self.onIncludeAllToggle)
        self._resizableInclude = Resizable(
            self._include,
            "USD_Light_Linking",
            "IncludeListHeight",
            self,
            defaultSize=80,
        )
        includeExcludeLayout.addWidget(self._resizableInclude)

        self._exclude = StringListPanel(data.getExcludeData(), False, "Exclude", self)
        self._resizableExclude = Resizable(
            self._exclude,
            "USD_Light_Linking",
            "ExcludeListHeight",
            self,
            defaultSize=80,
        )
        includeExcludeLayout.addWidget(self._resizableExclude)

        self._include.list.selectionChanged.connect(self.onListSelectionChanged)
        self._exclude.list.selectionChanged.connect(self.onListSelectionChanged)

        self._filterWidget.textChanged.connect(self._include.list._model.setFilter)
        self._filterWidget.textChanged.connect(self._exclude.list._model.setFilter)
        self._collData.dataChanged.connect(self._onDataChanged)

        self.setLayout(includeExcludeLayout)
        self.onListSelectionChanged()
        self._onDataChanged()

    def _onDataChanged(self):
        incAll = self._collData.includesAll()
        if incAll != self._include.cbIncludeAll.isChecked():
            self._include.cbIncludeAll.setChecked(incAll)

    def onAddToIncludePrimClicked(self):
        items = self._collData.pick()
        self._collData.getIncludeData().addStrings(items)

    def onAddToExcludePrimClicked(self):
        items = self._collData.pick()
        self._collData.getExcludeData().addStrings(items)

    def onRemoveSelectionFromInclude(self):
        self._collData.getIncludeData().removeStrings(self._include.list.selectedItems)
        self.onListSelectionChanged()

    def onRemoveSelectionFromExclude(self):
        self._collData.getExcludeData().removeStrings(self._include.list.selectedItems)
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
                "Remove selected prims from Include or Exclude..."
            )
            self._deleteBtn.setPopupMode(QToolButton.InstantPopup)
            self._deleteBtn.setStyleSheet("")
        else:
            if includesSelected:
                self._deleteBtn.setToolTip("Remove selected prims from Include")
                self._deleteBtn.pressed.connect(self.onRemoveSelectionFromInclude)
            elif excludeSelected:
                self._deleteBtn.setToolTip("Remove selected prims from Exclude")
                self._deleteBtn.pressed.connect(self.onRemoveSelectionFromExclude)
            self._deleteBtn.setPopupMode(QToolButton.DelayedPopup)
            self._deleteBtn.setStyleSheet(
                """QToolButton::menu-indicator { width: 0px; }"""
            )

    def onIncludeAllToggle(self, state: Qt.CheckState):
        incAll = self._include.cbIncludeAll.isChecked()
        self._collData.setIncludeAll(incAll)


