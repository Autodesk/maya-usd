from .expressionRulesMenu import ExpressionMenu
from.warningWidget import WarningWidget
from ..common.menuButton import MenuButton
from ..common.host import Host
from ..common.theme import Theme
from ..data.collectionData import CollectionData

try:
    from PySide6.QtCore import QEvent, Qt  # type: ignore
    from PySide6.QtWidgets import QSizePolicy, QTextEdit, QToolButton, QWidget, QVBoxLayout, QHBoxLayout, QFrame # type: ignore
except ImportError:
    from PySide2.QtCore import QEvent, Qt  # type: ignore
    from PySide2.QtWidgets import QSizePolicy, QTextEdit, QToolButton, QWidget, QVBoxLayout, QHBoxLayout, QFrame # type: ignore

SELECT_OBJECTS_TOOLTIP = "Selects the objects in the Viewport."

class ExpressionWidget(QWidget):
    def __init__(self, data: CollectionData, parent: QWidget):
        super(ExpressionWidget, self).__init__(parent)
        self._collData = data

        mainLayout = QVBoxLayout(self)
        margin: int = Theme.instance().uiScaled(2)
        mainLayout.setSpacing(margin)
        mainLayout.setContentsMargins(0, 0, 0, 0)

        self._expressionMenu = ExpressionMenu(data, self)
        menuButton = MenuButton(self._expressionMenu, self)

        menuWidget = QWidget(self)
        menuLayout = QHBoxLayout(menuWidget)
        topMargin = Theme.instance().uiScaled(4)
        margin = Theme.instance().uiScaled(2)
        menuLayout.setContentsMargins(margin, topMargin, margin, margin)

        self._selectBtn = QToolButton(menuWidget)
        self._selectBtn.setToolTip(SELECT_OBJECTS_TOOLTIP)
        self._selectBtn.setIcon(Theme.instance().icon("selector"))
        self._selectBtn.setEnabled(False)
        self._selectBtn.clicked.connect(self._onSelectItemsClicked)
        menuLayout.addWidget(self._selectBtn)

        menuLayout.addStretch(1)

        self._warningWidget = WarningWidget(menuWidget)
        self._warningSeparator = QFrame()
        self._warningSeparator.setFrameShape(QFrame.VLine)
        self._warningSeparator.setMaximumHeight(Theme.instance().uiScaled(20))
        self._warningSeparator.setVisible(False)
        menuLayout.addWidget(self._warningWidget)
        menuLayout.addWidget(self._warningSeparator)

        menuLayout.addWidget(menuButton)
        menuWidget.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        self._expressionText = QTextEdit(self)
        self._expressionText.setLineWrapMode(QTextEdit.LineWrapMode.WidgetWidth)
        self._expressionText.setMinimumHeight(Theme.instance().uiScaled(80))
        self._expressionText.setSizePolicy(QSizePolicy.Ignored, QSizePolicy.Ignored)
        self._expressionText.setPlaceholderText("Type an expression here...")
        self._expressionText.setAcceptRichText(False)

        mainLayout.addWidget(menuWidget, 0)
        mainLayout.addWidget(self._expressionText, 1)

        self._expressionText.installEventFilter(self)
        self.setLayout(mainLayout)

        self._collData.dataChanged.connect(self._onDataChanged)

        self._onDataChanged()

    def _onDataChanged(self):
        usdExpressionAttr = self._collData.getMembershipExpression()
        text = usdExpressionAttr or ''
        self._expressionText.setPlainText(text)
        self._selectBtn.setEnabled(bool(text))

        isConflicted = self._collData.hasDataConflict()
        self._warningWidget.setVisible(isConflicted)
        self._warningWidget.setVisible(isConflicted)

    def _onSelectItemsClicked(self):
        membershipItems = self._collData.computeMembership()
        # Note: for membership expression, it does not matter if we use
        #       the include or exclude version of convertToItemPaths.
        membershipItems = self._collData.getIncludeData().convertToItemPaths(membershipItems)
        Host.instance().setSelectionFromText(membershipItems)

    def submitExpression(self):
        newText = self._expressionText.toPlainText() or ''
        oldText = self._collData.getMembershipExpression() or ''
        if newText == oldText:
            return
        self._collData.setMembershipExpression(newText)

        # If there was an error setting the expression, 
        # revert the text in the ui to the original text since it was not properly set
        membershipExpression = self._collData.getMembershipExpression()
        if membershipExpression != self._expressionText.toPlainText():
            self._expressionText.setPlainText(membershipExpression)

    def eventFilter(self, obj, event):
        if event.type() == QEvent.KeyPress and obj is self._expressionText:
            if event.key() == Qt.Key_Return or event.key() == Qt.Key_Enter:
                self.submitExpression()
                return True
        # For the case when they change prim without hitting enter;
        # or click somewhere else in the UI
        elif event.type() == QEvent.FocusOut:
            self.submitExpression()

        return super().eventFilter(obj, event)
