from ..data.collectionData import CollectionData

try:
    from PySide6.QtWidgets import QMenu, QWidget # type: ignore
    from PySide6.QtGui import QActionGroup, QAction # type: ignore
except ImportError:
    from PySide2.QtWidgets import QMenu, QWidget, QActionGroup, QAction # type: ignore

from pxr import Usd

# TODO: support I8N
EXPAND_PRIMS_MENU_OPTION = "Expand Prims"
EXPAND_PRIMS_PROPERTIES_MENU_OPTION = "Expand Prims and Properties"
EXPLICIT_ONLY_MENU_OPTION = "Explicit Only"
INCLUDE_EXCLUDE_LABEL = "Include/Exclude"
REMOVE_ALL_LABEL = "Remove All"


class ExpressionMenu(QMenu):
    def __init__(self, data: CollectionData, parent: QWidget):
        super(ExpressionMenu, self).__init__(parent)
        self._collData = data

        # Note: this is necessary to avoid the separator not show up.
        self.setSeparatorsCollapsible(False)

        self._removeAllAction = QAction(REMOVE_ALL_LABEL, self)
        self.addActions([self._removeAllAction])

        self._removeAllAction.triggered.connect(self._onRemoveAll)

        self._collData.dataChanged.connect(self._onDataChanged)
        expansionRulesMenu = QMenu("Expansion Rules", self)
        self.expandPrimsAction = QAction(EXPAND_PRIMS_MENU_OPTION, expansionRulesMenu, checkable=True)
        self.expandPrimsPropertiesAction = QAction(EXPAND_PRIMS_PROPERTIES_MENU_OPTION, expansionRulesMenu, checkable=True)
        self.explicitOnlyAction = QAction(EXPLICIT_ONLY_MENU_OPTION, expansionRulesMenu, checkable=True)
        expansionRulesMenu.addActions([self.expandPrimsAction, self.expandPrimsPropertiesAction, self.explicitOnlyAction])

        actionGroup = QActionGroup(self)
        actionGroup.setExclusive(True)
        for action in expansionRulesMenu.actions():
            actionGroup.addAction(action)
        self.addMenu(expansionRulesMenu)

        actionGroup.triggered.connect(self.onExpressionSelected)

        self._onDataChanged()

    def _onDataChanged(self):
        usdExpansionRule = self._collData.getExpansionRule()
        if usdExpansionRule == Usd.Tokens.expandPrims:
            self.expandPrimsAction.setChecked(True)
        elif usdExpansionRule == Usd.Tokens.expandPrimsAndProperties:
            self.expandPrimsPropertiesAction.setChecked(True)
        elif usdExpansionRule == Usd.Tokens.explicitOnly:
            self.explicitOnlyAction.setChecked(True)

    def _onRemoveAll(self):
        self._collData.removeAllIncludeExclude()

    def onExpressionSelected(self, menuOption):
        if menuOption == self.expandPrimsAction:
            self._collData.setExpansionRule(Usd.Tokens.expandPrims)
        elif menuOption == self.expandPrimsPropertiesAction:
            self._collData.setExpansionRule(Usd.Tokens.expandPrimsAndProperties)
        elif menuOption == self.explicitOnlyAction:
            self._collData.setExpansionRule(Usd.Tokens.explicitOnly)
