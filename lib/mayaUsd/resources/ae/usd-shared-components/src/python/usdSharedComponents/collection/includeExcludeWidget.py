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
SEARCH_PLACEHOLDER_LABEL = "Search..."
ADD_OBJECTS_TOOLTIP = "Add Objects to the Include/Exclude list"
REMOVE_OBJECTS_TOOLTIP = "Remove Selected Objects from Include/Exclude list"
REMOVE_FROM_INCLUDE_TOOLTIP = "Remove Selected Objects from Include"
REMOVE_FROM_EXCLUDE_TOOLTIP = "Remove Selected Objects from Exclude"
INCLUDE_OBJECTS_LABEL = "Include Objects..."
EXCLUDE_OBJECTS_LABEL ="Exclude Objects..."
REMOVE_FROM_INCLUDES_LABEL = "Remove Selected Objects from Include"
REMOVE_FROM_EXCLUDES_LABEL = "Remove Selected Objects from Exclude"
INCLUDE_LABEL = "Include"
EXCLUDE_LABEL = "Exclude"
ADD_INCLUDE_OBJECTS_TITLE = "Add Include Objects"
ADD_EXCLUDE_OBJECTS_TITLE = "Add Exclude Objects"

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
        self._filterWidget.setPlaceholderText(SEARCH_PLACEHOLDER_LABEL)
        self._filterWidget.setClearButtonEnabled(True)

        separator = QFrame()
        separator.setFrameShape(QFrame.VLine)

        headerWidget = QWidget(self)
        headerLayout = QHBoxLayout(headerWidget)

        if Host.instance().canPick:
            addBtn = QToolButton(headerWidget)
            addBtn.setToolTip(ADD_OBJECTS_TOOLTIP)
            addBtn.setIcon(Theme.instance().icon("add"))
            addBtn.setPopupMode(QToolButton.InstantPopup)
            addBtnMenu = QMenu(addBtn)
            addBtnMenu.addAction(INCLUDE_OBJECTS_LABEL, self.onAddToIncludePrimClicked)
            addBtnMenu.addAction(EXCLUDE_OBJECTS_LABEL, self.onAddToExcludePrimClicked)
            addBtn.setMenu(addBtnMenu)
            headerLayout.addWidget(addBtn)

        self._deleteBtn = QToolButton(headerWidget)
        self._deleteBtn.setToolTip(REMOVE_OBJECTS_TOOLTIP)
        self._deleteBtn.setIcon(Theme.instance().icon("delete"))
        self._deleteBtn.setPopupMode(QToolButton.InstantPopup)
        self._deleteBtnMenu = QMenu(self._deleteBtn)
        self._deleteBtnMenu.addAction(
            REMOVE_FROM_INCLUDES_LABEL, self.onRemoveSelectionFromInclude
        )
        self._deleteBtnMenu.addAction(
            REMOVE_FROM_EXCLUDES_LABEL, self.onRemoveSelectionFromExclude
        )
        self._deleteBtn.setMenu(self._deleteBtnMenu)
        headerLayout.addWidget(self._deleteBtn)

        self._deleteBtn.setEnabled(False)

        headerLayout.addWidget(self._filterWidget)
        headerLayout.addWidget(separator)
        headerLayout.addWidget(menuButton)
        mainLayout.addWidget(headerWidget)

        self._include = StringListPanel(data.getIncludeData(), True, INCLUDE_LABEL, self)
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

        self._exclude = StringListPanel(data.getExcludeData(), False, EXCLUDE_LABEL, self)
        self._resizableExclude = Resizable(
            self._exclude,
            "USD_Light_Linking",
            "ExcludeListHeight",
            self,
            defaultSize=Theme.instance().uiScaled(80),
        )
        self._resizableExclude.minContentSize = Theme.instance().uiScaled(44)
        mainLayout.addWidget(self._resizableExclude)

        self._include.list.itemSelectionChanged.connect(self.onListSelectionChanged)
        self._exclude.list.itemSelectionChanged.connect(self.onListSelectionChanged)

        self._filterWidget.textChanged.connect(self._include.list._model.setFilter)
        self._filterWidget.textChanged.connect(self._exclude.list._model.setFilter)
        self._collData.dataChanged.connect(self._onDataChanged)

        self.onListSelectionChanged()
        self._onDataChanged()

    def _onDataChanged(self):
        incAll = self._collData.includesAll()
        if incAll != self._include.cbIncludeAll.isChecked():
            self._include.cbIncludeAll.setChecked(incAll)
        self.onListSelectionChanged()

    def onAddToIncludePrimClicked(self):
        stage = self._collData.getStage()
        if not stage:
            return
        items = Host.instance().pick(stage, dialogTitle=ADD_INCLUDE_OBJECTS_TITLE)
        self._collData.getIncludeData().addStrings(map(lambda x: str(x.GetPath()), items))

    def onAddToExcludePrimClicked(self):
        stage = self._collData.getStage()
        if not stage:
            return
        items = Host.instance().pick(stage, dialogTitle=ADD_EXCLUDE_OBJECTS_TITLE)
        self._collData.getExcludeData().addStrings(map(lambda x: str(x.GetPath()), items))

    def onRemoveSelectionFromInclude(self):
        self._collData.getIncludeData().removeStrings(self._include.list.selectedItems())
        self.onListSelectionChanged()

    def onRemoveSelectionFromExclude(self):
        self._collData.getExcludeData().removeStrings(self._exclude.list.selectedItems())
        self.onListSelectionChanged()

    def _findAction(self, label):
        for act in self._deleteBtnMenu.actions():
            if act.text() == label:
                return act
        return None

    def onListSelectionChanged(self):
        includesSelected = self._include.list.hasSelectedItems()
        excludeSelected = self._exclude.list.hasSelectedItems()
        self._deleteBtn.setEnabled(includesSelected or excludeSelected)

        deleteFromIncludesAction = self._findAction(REMOVE_FROM_INCLUDES_LABEL)
        if deleteFromIncludesAction:
            deleteFromIncludesAction.setEnabled(includesSelected)
        deleteFromExcludesAction = self._findAction(REMOVE_FROM_EXCLUDES_LABEL)
        if deleteFromExcludesAction:
            deleteFromExcludesAction.setEnabled(excludeSelected)

        try:
            self._deleteBtn.pressed.disconnect(self.onRemoveSelectionFromInclude)
            self._deleteBtn.pressed.disconnect(self.onRemoveSelectionFromExclude)
        except Exception:
            pass

        if includesSelected and excludeSelected:
            self._deleteBtn.setToolTip(REMOVE_OBJECTS_TOOLTIP)
            self._deleteBtn.setPopupMode(QToolButton.InstantPopup)
            self._deleteBtn.setStyleSheet("")
        else:
            if includesSelected:
                self._deleteBtn.setToolTip(REMOVE_FROM_INCLUDE_TOOLTIP)
                self._deleteBtn.pressed.connect(self.onRemoveSelectionFromInclude)
            elif excludeSelected:
                self._deleteBtn.setToolTip(REMOVE_FROM_EXCLUDE_TOOLTIP)
                self._deleteBtn.pressed.connect(self.onRemoveSelectionFromExclude)
            self._deleteBtn.setPopupMode(QToolButton.DelayedPopup)
            self._deleteBtn.setStyleSheet(
                """QToolButton::menu-indicator { width: 0px; }"""
            )

    def onIncludeAllToggle(self, _: Qt.CheckState):
        incAll = self._include.cbIncludeAll.isChecked()
        self._collData.setIncludeAll(incAll)
