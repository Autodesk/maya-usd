from typing import Callable

from ..common.stringListPanel import StringListPanel
from ..common.resizable import Resizable
from ..common.menuButton import MenuButton
from .expressionRulesMenu import ExpressionMenu
from ..common.host import Host
from ..common.theme import Theme
from ..data.collectionData import CollectionData
from ..data.stringListData import StringListData

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
ADD_SELECTION_TO_INCLUDE_LABEL ="Add Selection to Include"
ADD_SELECTION_TO_EXCLUDE_LABEL ="Add Selection to Exclude"
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

        spacer = QWidget()
        spacer.setMinimumWidth(0)
        spacer.setSizePolicy(
            QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Fixed
        )

        self._filterWidget = QLineEdit()
        self._filterWidget.setSizePolicy(
            QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed
        )
        self._filterWidget.setMaximumWidth(Theme.instance().uiScaled(165))
        self._filterWidget.setPlaceholderText(SEARCH_PLACEHOLDER_LABEL)
        self._filterWidget.setClearButtonEnabled(True)

        separator = QFrame()
        separator.setFrameShape(QFrame.VLine)
        separator.setMaximumHeight(Theme.instance().uiScaled(20))

        headerWidget = QWidget(self)
        headerLayout = QHBoxLayout(headerWidget)
        topMargin = Theme.instance().uiScaled(4)
        margin = Theme.instance().uiScaled(2)
        headerLayout.setContentsMargins(margin, topMargin, margin, margin)

        addBtn = QToolButton(headerWidget)
        addBtn.setToolTip(ADD_OBJECTS_TOOLTIP)
        addBtn.setIcon(Theme.instance().icon("add"))
        addBtn.setPopupMode(QToolButton.InstantPopup)
        Theme.instance().themeMenuButton(addBtn, True)
        
        self._addBtnMenu = QMenu(addBtn)
        if Host.instance().canPick:
            self._addBtnMenu.addAction(INCLUDE_OBJECTS_LABEL, self.onAddToIncludePrimClicked)
            self._addBtnMenu.addAction(EXCLUDE_OBJECTS_LABEL, self.onAddToExcludePrimClicked)
            self._addBtnMenu.addSeparator()
        self._addBtnMenu.addAction(ADD_SELECTION_TO_INCLUDE_LABEL, self._onAddSelectionToInclude)
        self._addBtnMenu.addAction(ADD_SELECTION_TO_EXCLUDE_LABEL, self._onAddSelectionToExclude)
        self._addBtnMenu.aboutToShow.connect(self._onAboutToShowAddMenu)
        addBtn.setMenu(self._addBtnMenu)

        self._deleteBtn = QToolButton(headerWidget)
        self._deleteBtn.setToolTip(REMOVE_OBJECTS_TOOLTIP)
        self._deleteBtn.setIcon(Theme.instance().icon("delete"))
        self._deleteBtn.setPopupMode(QToolButton.InstantPopup)
        self._deleteBtn.setEnabled(False)

        self._deleteBtnMenu = QMenu(self._deleteBtn)
        self._deleteBtnMenu.addAction(
            REMOVE_FROM_INCLUDES_LABEL, self.onRemoveSelectionFromInclude
        )
        self._deleteBtnMenu.addAction(
            REMOVE_FROM_EXCLUDES_LABEL, self.onRemoveSelectionFromExclude
        )
        self._deleteBtn.setMenu(self._deleteBtnMenu)

        headerLayout.addWidget(addBtn)
        headerLayout.addWidget(self._deleteBtn)
        headerLayout.addWidget(spacer)

        self._deleteBtnPressedConnectedTo: Callable = None

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

    def _hasValidSelection(self, stringList: StringListData) -> bool:
        for item in Host.instance().getSelectionAsText():
            if stringList._isValidString(item):
                return True
        return False
    
    def _onAboutToShowAddMenu(self):
        labelsAndDatas = [
            (ADD_SELECTION_TO_INCLUDE_LABEL, self._collData.getIncludeData()),
            (ADD_SELECTION_TO_EXCLUDE_LABEL, self._collData.getExcludeData()),
        ]
        for label, data in labelsAndDatas:
            action = self._findAction(label)
            if not action:
                continue
            action.setEnabled(self._hasValidSelection(data))

    def _onAddSelectionToInclude(self):
        self._addSelectionToList(self._collData.getIncludeData())
        
    def _onAddSelectionToExclude(self):
        self._addSelectionToList(self._collData.getExcludeData())

    def _addSelectionToList(self, stringList: StringListData):
        multilineSelection = Host.instance().getSelectionAsText()
        if not multilineSelection:
            return
        # Note: we use addMultiLineStrings becuase it validates the given path.
        #
        # TODO: maybe move the validation into the addStrings and removeStrings
        #       functions instead? But that would cause code duplication...
        stringList.addMultiLineStrings('\n'.join(multilineSelection))

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
        for act in self._addBtnMenu.actions():
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

        if self._deleteBtnPressedConnectedTo:
            self._deleteBtn.pressed.disconnect(self._deleteBtnPressedConnectedTo)
            self._deleteBtnPressedConnectedTo = None

        if includesSelected and excludeSelected:
            self._deleteBtn.setToolTip(REMOVE_OBJECTS_TOOLTIP)
            self._deleteBtn.setPopupMode(QToolButton.InstantPopup)
            Theme.instance().themeMenuButton(self._deleteBtn, True)
        else:
            if includesSelected:
                self._deleteBtn.setToolTip(REMOVE_FROM_INCLUDE_TOOLTIP)
                self._deleteBtn.pressed.connect(self.onRemoveSelectionFromInclude)
                self._deleteBtnPressedConnectedTo = self.onRemoveSelectionFromInclude
            elif excludeSelected:
                self._deleteBtn.setToolTip(REMOVE_FROM_EXCLUDE_TOOLTIP)
                self._deleteBtn.pressed.connect(self.onRemoveSelectionFromExclude)
                self._deleteBtnPressedConnectedTo = self.onRemoveSelectionFromExclude
            self._deleteBtn.setPopupMode(QToolButton.DelayedPopup)
            Theme.instance().themeMenuButton(self._deleteBtn, False)

    def onIncludeAllToggle(self, _: Qt.CheckState):
        incAll = self._include.cbIncludeAll.isChecked()
        self._collData.setIncludeAll(incAll)
