from .expressionRulesMenu import ExpressionMenu
from ..common.menuButton import MenuButton
from ..common.theme import Theme
from ..data.collectionData import CollectionData

try:
    from PySide6.QtCore import QEvent, Qt  # type: ignore
    from PySide6.QtWidgets import QSizePolicy, QTextEdit, QWidget, QVBoxLayout, QHBoxLayout # type: ignore
except ImportError:
    from PySide2.QtCore import QEvent, Qt  # type: ignore
    from PySide2.QtWidgets import QSizePolicy, QTextEdit, QWidget, QVBoxLayout, QHBoxLayout # type: ignore

from pxr import Usd, Sdf

class ExpressionWidget(QWidget):
    def __init__(self, data: CollectionData, parent: QWidget, expressionChangedCallback):
        super(ExpressionWidget, self).__init__(parent)
        self._collData = data
        self._expressionCallback = expressionChangedCallback

        mainLayout = QVBoxLayout(self)
        margin: int = Theme.instance().uiScaled(2)
        mainLayout.setSpacing(margin)

        self._expressionMenu = ExpressionMenu(data, self)
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
        self.setLayout(mainLayout)

        self._collData.dataChanged.connect(self._onDataChanged)
        self._onDataChanged()

    def _onDataChanged(self):
        usdExpressionAttr = self._collData.getMembershipExpression()
        if usdExpressionAttr != None:
            self._expressionText.setPlainText(usdExpressionAttr.GetText())

    def submitExpression(self):
        self._collData.setMembershipExpression(self._expressionText.toPlainText())
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
