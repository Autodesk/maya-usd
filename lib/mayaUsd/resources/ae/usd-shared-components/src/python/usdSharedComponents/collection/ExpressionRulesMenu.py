
try:
    from PySide6.QtWidgets import QMenu, QWidget # type: ignore
    from PySide6.QtGui import QActionGroup, QAction # type: ignore
except ImportError:
    from PySide2.QtWidgets import QMenu, QWidget # type: ignore
    from PySide2.QtGui import QActionGroup  # type: ignore

from pxr import Usd

EXPAND_PRIMS_MENU_OPTION = "Expand Prims"
EXPAND_PRIMS_PROPERTIES_MENU_OPTION = "Expand Prims and Properties"
EXPLICIT_ONLY_MENU_OPTION = "Explicit Only"

class ExpressionMenu(QMenu):
    def __init__(self, collection, parent: QWidget):
        super(ExpressionMenu, self).__init__(parent)
        self._collection = collection

        expansionRulesMenu = QMenu("Expansion Rules", self)
        self.expandPrimsAction = QAction(EXPAND_PRIMS_MENU_OPTION, expansionRulesMenu, checkable=True)
        self.expandPrimsPropertiesAction = QAction(EXPAND_PRIMS_PROPERTIES_MENU_OPTION, expansionRulesMenu, checkable=True)
        self.explicitOnlyAction = QAction(EXPLICIT_ONLY_MENU_OPTION, expansionRulesMenu, checkable=True)
        expansionRulesMenu.addActions([self.expandPrimsAction, self.expandPrimsPropertiesAction, self.explicitOnlyAction])

        self.update()

        actionGroup = QActionGroup(self)
        actionGroup.setExclusive(True)
        for action in expansionRulesMenu.actions():
            actionGroup.addAction(action)

        self.triggered.connect(self.onExpressionSelected)
        self.addMenu(expansionRulesMenu)

    def update(self):
        usdExpansionRule = self._collection.GetExpansionRuleAttr().Get()
        if usdExpansionRule == Usd.Tokens.expandPrims:
            self.expandPrimsAction.setChecked(True)
        elif usdExpansionRule == Usd.Tokens.expandPrimsAndProperties:
            self.expandPrimsPropertiesAction.setChecked(True)
        elif usdExpansionRule == Usd.Tokens.explicitOnly:
            self.explicitOnlyAction.setChecked(True)

    def onExpressionSelected(self, menuOption):
        usdExpansionRule = self._collection.GetExpansionRuleAttr().Get()
        if menuOption == self.expandPrimsAction:
            if usdExpansionRule != Usd.Tokens.expandPrims:
                self._collection.CreateExpansionRuleAttr(Usd.Tokens.expandPrims)
        elif menuOption == self.expandPrimsPropertiesAction:
            if usdExpansionRule != Usd.Tokens.expandPrimsAndProperties:
                self._collection.CreateExpansionRuleAttr(Usd.Tokens.expandPrimsAndProperties)
        elif menuOption == self.explicitOnlyAction:
            if usdExpansionRule != Usd.Tokens.explicitOnly:
                self._collection.CreateExpansionRuleAttr(Usd.Tokens.explicitOnly)