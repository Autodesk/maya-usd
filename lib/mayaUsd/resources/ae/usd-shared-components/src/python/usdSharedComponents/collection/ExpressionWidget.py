from .ExpressionRulesMenu import ExpressionMenu
from ..common.kebabMenu import KebabMenu

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

        self._expressionText = QTextEdit(self)
        self._expressionText.setContentsMargins(0,0,0,0)
        self._expressionText.setLineWrapMode(QTextEdit.LineWrapMode.WidgetWidth)
        self._expressionText.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self._expressionText.setPlaceholderText("Type an expression here...")

        self._expressionMenu = ExpressionMenu(collection, self)
        menuButton = KebabMenu(self._expressionMenu, self)

        menuWidget = QWidget(self)
        menuLayout = QHBoxLayout(menuWidget)
        menuLayout.addStretch(1)
        menuLayout.addWidget(menuButton)

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(0,0,0,0)
        mainLayout.addWidget(menuWidget)
        mainLayout.addWidget(self._expressionText)
        
        self._expressionText.installEventFilter(self)
        self.setLayout(mainLayout)
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