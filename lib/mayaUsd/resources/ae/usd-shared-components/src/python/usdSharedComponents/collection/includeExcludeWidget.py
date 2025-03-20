from typing import Callable

from ..common.stringListPanel import StringListPanel
from ..common.resizable import Resizable
from ..common.menuButton import MenuButton
from .expressionRulesMenu import ExpressionMenu
from .warningWidget import WarningWidget
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
SELECT_OBJECTS_TOOLTIP = "Selects the objects in the Viewport."
INCLUDE_OBJECTS_LABEL = "Include Objects..."
EXCLUDE_OBJECTS_LABEL ="Exclude Objects..."
ADD_SELECTION_TO_INCLUDE_LABEL ="Add Selection to Include"
ADD_SELECTION_TO_EXCLUDE_LABEL ="Add Selection to Exclude"
REMOVE_FROM_INCLUDES_LABEL = "Remove Selected from Include"
REMOVE_FROM_EXCLUDES_LABEL = "Remove Selected from Exclude"
REMOVE_FROM_BOTH_LABEL = "Remove Selected from Both"
INCLUDE_LABEL = "Include"
EXCLUDE_LABEL = "Exclude"
ADD_INCLUDE_OBJECTS_TITLE = "Add Include Objects"
ADD_EXCLUDE_OBJECTS_TITLE = "Add Exclude Objects"
CONFLICT_WARNING_MSG = "Both Include/Exclude rules and Expressions are currently defined. Expressions will be ignored."


class IncludeExcludeWidget(QWidget):
    def __init__(
        self,
        data: CollectionData,
        parent: QWidget = None,
    ):
        super(IncludeExcludeWidget, self).__init__(parent)
        self._collData = data

        theme = Theme.instance()

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
        self._filterWidget.setPlaceholderText(theme.themeLabel(SEARCH_PLACEHOLDER_LABEL))
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
        addBtn.setToolTip(theme.themeLabel(ADD_OBJECTS_TOOLTIP))
        addBtn.setStatusTip(theme.themeLabel(ADD_OBJECTS_TOOLTIP))
        addBtn.setIcon(theme.icon("add"))
        addBtn.setPopupMode(QToolButton.InstantPopup)
        Theme.instance().themeMenuButton(addBtn, True)
        
        self._addBtnMenu = QMenu(addBtn)
        if Host.instance().canPick:
            self._addBtnMenu.addAction(theme.themeLabel(INCLUDE_OBJECTS_LABEL), self.onAddToIncludePrimClicked)
            self._addBtnMenu.addAction(theme.themeLabel(EXCLUDE_OBJECTS_LABEL), self.onAddToExcludePrimClicked)
            self._addBtnMenu.addSeparator()
        self._addBtnMenu.addAction(theme.themeLabel(ADD_SELECTION_TO_INCLUDE_LABEL), self._onAddSelectionToInclude)
        self._addBtnMenu.addAction(theme.themeLabel(ADD_SELECTION_TO_EXCLUDE_LABEL), self._onAddSelectionToExclude)
        self._addBtnMenu.aboutToShow.connect(self._onAboutToShowAddMenu)
        addBtn.setMenu(self._addBtnMenu)

        self._deleteBtn = QToolButton(headerWidget)
        self._deleteBtn.setToolTip(theme.themeLabel(REMOVE_OBJECTS_TOOLTIP))
        self._deleteBtn.setStatusTip(theme.themeLabel(REMOVE_OBJECTS_TOOLTIP))
        self._deleteBtn.setIcon(theme.icon("delete"))
        self._deleteBtn.setPopupMode(QToolButton.InstantPopup)
        self._deleteBtn.setEnabled(False)

        self._deleteBtnMenu = QMenu(self._deleteBtn)
        self._deleteBtnMenu.addAction(
            REMOVE_FROM_INCLUDES_LABEL, self.onRemoveSelectionFromInclude
        )
        self._deleteBtnMenu.addAction(
            theme.themeLabel(REMOVE_FROM_EXCLUDES_LABEL), self.onRemoveSelectionFromExclude
        )
        self._deleteBtnMenu.addAction(
            theme.themeLabel(REMOVE_FROM_BOTH_LABEL), self.onRemoveSelectionFromBoth
        )
        self._deleteBtn.setMenu(self._deleteBtnMenu)

        self._selectBtn = QToolButton(headerWidget)
        self._selectBtn.setToolTip(theme.themeLabel(SELECT_OBJECTS_TOOLTIP))
        self._selectBtn.setStatusTip(theme.themeLabel(SELECT_OBJECTS_TOOLTIP))
        self._selectBtn.setIcon(Theme.instance().icon("selector"))
        self._selectBtn.setEnabled(False)
        self._selectBtn.clicked.connect(self._onSelectItemsClicked)
        theme.themeMenuButton(self._selectBtn, False)

        self._warningWidget = WarningWidget(headerWidget)
        self._warningSeparator = QFrame()
        self._warningSeparator.setFrameShape(QFrame.VLine)
        self._warningSeparator.setMaximumHeight(Theme.instance().uiScaled(20))
        self._warningSeparator.setVisible(False)

        headerLayout.addWidget(addBtn)
        headerLayout.addWidget(self._deleteBtn)
        headerLayout.addWidget(self._selectBtn)
        headerLayout.addWidget(spacer)

        self._deleteBtnPressedConnectedTo: Callable = None

        headerLayout.addWidget(self._warningWidget)
        headerLayout.addWidget(self._warningSeparator)
        headerLayout.addWidget(self._filterWidget)
        headerLayout.addWidget(separator)
        headerLayout.addWidget(menuButton)

        mainLayout.addWidget(headerWidget)

        self._include = StringListPanel(data.getIncludeData(), True, theme.themeLabel(INCLUDE_LABEL), self)
        self._include.cbIncludeAll.stateChanged.connect(self.onIncludeAllToggle)
        self._resizableInclude = Resizable(
            self._include,
            "USD_Light_Linking",
            "IncludeListHeight",
            self,
            defaultSize=Theme.instance().uiScaled(124),
        )
        self._resizableInclude.minContentSize = Theme.instance().uiScaled(44)
        mainLayout.addWidget(self._resizableInclude)

        self._exclude = StringListPanel(data.getExcludeData(), False, theme.themeLabel(EXCLUDE_LABEL), self)
        self._resizableExclude = Resizable(
            self._exclude,
            "USD_Light_Linking",
            "ExcludeListHeight",
            self,
            defaultSize=Theme.instance().uiScaled(124),
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

        wasConflicted = self._warningWidget.isConflicted()
        isConflicted = self._collData.hasDataConflict()
        self._warningWidget.setConflicted(isConflicted)
        self._warningSeparator.setVisible(isConflicted)
        if wasConflicted != isConflicted:
            if isConflicted:
                from ..common.host import Host, MessageType
                Host.instance().reportMessage(CONFLICT_WARNING_MSG, MessageType.WARNING)

    def onAddToIncludePrimClicked(self):
        stage = self._collData.getStage()
        if not stage:
            return
        theme = Theme.instance()
        items = Host.instance().pick(stage, dialogTitle=theme.themeLabel(ADD_INCLUDE_OBJECTS_TITLE))
        self._collData.getIncludeData().addStrings(map(lambda x: str(x.GetPath()), items))

    def onAddToExcludePrimClicked(self):
        stage = self._collData.getStage()
        if not stage:
            return
        theme = Theme.instance()
        items = Host.instance().pick(stage, dialogTitle=theme.themeLabel(ADD_EXCLUDE_OBJECTS_TITLE))
        self._collData.getExcludeData().addStrings(map(lambda x: str(x.GetPath()), items))

    def _hasValidSelection(self, stringList: StringListData) -> bool:
        for item in Host.instance().getSelectionAsText():
            if stringList.convertToCollectionString(item)[0]:
                return True
        return False
    
    def _onAboutToShowAddMenu(self):
        theme = Theme.instance()
        labelsAndDatas = [
            (theme.themeLabel(ADD_SELECTION_TO_INCLUDE_LABEL), self._collData.getIncludeData()),
            (theme.themeLabel(ADD_SELECTION_TO_EXCLUDE_LABEL), self._collData.getExcludeData()),
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

    def onRemoveSelectionFromBoth(self):
        # Note: need to retrieve both the lists becuase change cuase UI updates.
        included = self._include.list.selectedItems()
        excluded = self._exclude.list.selectedItems()
        self._collData.getIncludeData().removeStrings(included)
        self._collData.getExcludeData().removeStrings(excluded)
        self.onListSelectionChanged()

    def _onSelectItemsClicked(self):
        included = self._collData.getIncludeData().convertToItemPaths(self._include.list.selectedItems())
        excluded = self._collData.getExcludeData().convertToItemPaths(self._exclude.list.selectedItems())
        Host.instance().setSelectionFromText(included + excluded)

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
        self._selectBtn.setEnabled(includesSelected or excludeSelected)

        theme = Theme.instance()
        deleteFromIncludesAction = self._findAction(theme.themeLabel(REMOVE_FROM_INCLUDES_LABEL))
        if deleteFromIncludesAction:
            deleteFromIncludesAction.setEnabled(includesSelected)
        deleteFromExcludesAction = self._findAction(theme.themeLabel(REMOVE_FROM_EXCLUDES_LABEL))
        if deleteFromExcludesAction:
            deleteFromExcludesAction.setEnabled(excludeSelected)

        if self._deleteBtnPressedConnectedTo:
            self._deleteBtn.pressed.disconnect(self._deleteBtnPressedConnectedTo)
            self._deleteBtnPressedConnectedTo = None

        if includesSelected and excludeSelected:
            self._deleteBtn.setToolTip(theme.themeLabel(REMOVE_OBJECTS_TOOLTIP))
            self._deleteBtn.setStatusTip(theme.themeLabel(REMOVE_OBJECTS_TOOLTIP))
            self._deleteBtn.setPopupMode(QToolButton.InstantPopup)
            Theme.instance().themeMenuButton(self._deleteBtn, True)
        else:
            if includesSelected:
                self._deleteBtn.setToolTip(theme.themeLabel(REMOVE_FROM_INCLUDE_TOOLTIP))
                self._deleteBtn.setStatusTip(theme.themeLabel(REMOVE_FROM_INCLUDE_TOOLTIP))
                self._deleteBtn.pressed.connect(self.onRemoveSelectionFromInclude)
                self._deleteBtnPressedConnectedTo = self.onRemoveSelectionFromInclude
            elif excludeSelected:
                self._deleteBtn.setToolTip(theme.themeLabel(REMOVE_FROM_EXCLUDE_TOOLTIP))
                self._deleteBtn.setStatusTip(theme.themeLabel(REMOVE_FROM_EXCLUDE_TOOLTIP))
                self._deleteBtn.pressed.connect(self.onRemoveSelectionFromExclude)
                self._deleteBtnPressedConnectedTo = self.onRemoveSelectionFromExclude
            self._deleteBtn.setPopupMode(QToolButton.DelayedPopup)
            Theme.instance().themeMenuButton(self._deleteBtn, False)

    def onIncludeAllToggle(self, _: Qt.CheckState):
        incAll = self._include.cbIncludeAll.isChecked()
        self._collData.setIncludeAll(incAll)
