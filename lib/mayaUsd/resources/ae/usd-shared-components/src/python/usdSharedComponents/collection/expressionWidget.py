from .expressionRulesMenu import ExpressionMenu
from ..common.menuButton import MenuButton
from ..common.theme import Theme

try:
    from PySide6.QtCore import QEvent, Qt  # type: ignore
    from PySide6.QtWidgets import QSizePolicy, QTextEdit, QWidget, QVBoxLayout, QHBoxLayout # type: ignore
except ImportError:
    from PySide2.QtCore import QEvent, Qt  # type: ignore
    from PySide2.QtWidgets import QSizePolicy, QTextEdit, QWidget, QVBoxLayout, QHBoxLayout # type: ignore

from pxr import Usd, Sdf

class ExpressionWidget(QWidget):
    def __init__(self, collection: Usd.CollectionAPI, parent: QWidget, expressionChangedCallback):
        super(ExpressionWidget, self).__init__(parent)
        self._collection = collection
        self._expressionCallback = expressionChangedCallback

        mainLayout = QVBoxLayout(self)
        margin: int = Theme.instance().uiScaled(2)
        mainLayout.setSpacing(margin)

        self._expressionMenu = ExpressionMenu(collection, self)
        menuButton = MenuButton(self._expressionMenu, self)

        menuWidget = QWidget(self)
        menuLayout = QHBoxLayout(menuWidget)

        margins = menuLayout.contentsMargins()
        margins.setRight(0)
        menuLayout.setContentsMargins(margins)

        menuLayout.addStretch(1)
        menuLayout.addWidget(menuButton)
        menuWidget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        self._expressionText = QTextEdit(self)
        self._expressionText.setLineWrapMode(QTextEdit.LineWrapMode.WidgetWidth)
        self._expressionText.setMinimumHeight(Theme.instance().uiScaled(80))
        self._expressionText.setSizePolicy(QSizePolicy.Ignored, QSizePolicy.Ignored)
        self._expressionText.setPlaceholderText("Type an expression here...")

        mainLayout.addWidget(menuWidget, 0)
        mainLayout.addWidget(self._expressionText, 1)

        self._expressionText.installEventFilter(self)
        self.update()

    def update(self):
        usdExpressionAttr = self._collection.GetMembershipExpressionAttr().Get()
        if usdExpressionAttr != None:
            self._expressionText.setPlainText(usdExpressionAttr.GetText())

        self._expressionMenu.update()

    def submitExpression(self):
        usdExpressionAttr = self._collection.GetMembershipExpressionAttr().Get()
        usdExpression = ""
        if usdExpressionAttr != None:
            usdExpression = usdExpressionAttr.GetText()

        textExpression = self._expressionText.toPlainText()
        if usdExpression != textExpression:
            # assign default value if text is empty
            if textExpression == "":
                self._collection.CreateMembershipExpressionAttr()
            else:
                self._collection.CreateMembershipExpressionAttr(Sdf.PathExpression(textExpression))

            if self._expressionCallback != None:
                self._expressionCallback()

    def eventFilter(self, obj, event):
        if event.type() == QEvent.KeyPress and obj is self._expressionText:
            if event.key() == Qt.Key_Return and self._expressionText.hasFocus():
                self._expressionText.clearFocus()
                return True
        elif event.type() == QEvent.FocusOut:
            self.submitExpression()

        return super().eventFilter(obj, event)
