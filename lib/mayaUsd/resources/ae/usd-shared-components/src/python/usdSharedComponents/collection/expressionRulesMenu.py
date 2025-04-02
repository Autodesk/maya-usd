from ..data.collectionData import CollectionData
from ..common.theme import Theme

try:
    from PySide6.QtWidgets import QApplication, QMenu, QWidget # type: ignore
    from PySide6.QtGui import QActionGroup, QAction # type: ignore
except ImportError:
    from PySide2.QtWidgets import QApplication, QMenu, QWidget, QActionGroup, QAction # type: ignore

from pxr import Usd

# TODO: support I8N
EXPAND_PRIMS_MENU_OPTION = "Expand Prims"
EXPAND_PRIMS_PROPERTIES_MENU_OPTION = "Expand Prims and Properties"
EXPLICIT_ONLY_MENU_OPTION = "Explicit Only"
INCLUDE_EXCLUDE_LABEL = "Include/Exclude"
REMOVE_ALL_LABEL = "Remove All"
CLEAR_OPINIONS_LABEL = "Clear Opinions from Target Layer"
PRINT_PRIMS_LABEL = "Print Prims to Script Editor"
COPY_COLLECTION_PATH_LABEL = "Copy Collection Path"
HELP_LABEL = "Help"


class ExpressionMenu(QMenu):
    def __init__(self, data: CollectionData, parent: QWidget):
        super(ExpressionMenu, self).__init__(parent)
        self._collData = data

        theme = Theme.instance()

        self._removeAllAction = QAction(theme.themeLabel(REMOVE_ALL_LABEL), self)
        self._clearOpinionsAction = QAction(theme.themeLabel(CLEAR_OPINIONS_LABEL), self)
        self._printPrimsAction = QAction(theme.themeLabel(PRINT_PRIMS_LABEL), self)
        self._copyCollectionPathAction = QAction(theme.themeLabel(COPY_COLLECTION_PATH_LABEL), self)
        self._helpAction = QAction(theme.themeLabel(HELP_LABEL), self)

        self.addActions([self._removeAllAction, self._clearOpinionsAction])
        self.addSeparator()
        self.addActions([self._printPrimsAction, self._copyCollectionPathAction])

        self._removeAllAction.triggered.connect(self._onRemoveAll)
        self._clearOpinionsAction.triggered.connect(self._onClearOpinions)
        self._printPrimsAction.triggered.connect(self._onPrintPrims)
        self._copyCollectionPathAction.triggered.connect(self._copyCollectionPathToClipboard)
        self._helpAction.triggered.connect(self._showHelpAction)

        self._collData.dataChanged.connect(self._onDataChanged)
        expansionRulesMenu = QMenu("Expansion Rules", self)
        self.expandPrimsAction = QAction(theme.themeLabel(EXPAND_PRIMS_MENU_OPTION), expansionRulesMenu, checkable=True)
        self.expandPrimsPropertiesAction = QAction(theme.themeLabel(EXPAND_PRIMS_PROPERTIES_MENU_OPTION), expansionRulesMenu, checkable=True)
        self.explicitOnlyAction = QAction(theme.themeLabel(EXPLICIT_ONLY_MENU_OPTION), expansionRulesMenu, checkable=True)
        expansionRulesMenu.addActions([self.expandPrimsAction, self.expandPrimsPropertiesAction, self.explicitOnlyAction])

        actionGroup = QActionGroup(self)
        actionGroup.setExclusive(True)
        for action in expansionRulesMenu.actions():
            actionGroup.addAction(action)
        self.addMenu(expansionRulesMenu)

        self.addAction(self._helpAction)

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

    def _onClearOpinions(self):
        self._collData.clearIncludeExcludeOpinions()

    def _onPrintPrims(self):
        self._collData.printCollection()

    def _copyCollectionPathToClipboard(self):
        path = self._collData.getNamedCollectionPath()
        if path:
            QApplication.clipboard().setText(path)

    def _showHelpAction(self):
        self._collData.openHelp()

    def onExpressionSelected(self, menuOption):
        if menuOption == self.expandPrimsAction:
            self._collData.setExpansionRule(Usd.Tokens.expandPrims)
        elif menuOption == self.expandPrimsPropertiesAction:
            self._collData.setExpansionRule(Usd.Tokens.expandPrimsAndProperties)
        elif menuOption == self.explicitOnlyAction:
            self._collData.setExpansionRule(Usd.Tokens.explicitOnly)
