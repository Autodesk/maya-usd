from .includeExcludeWidget import IncludeExcludeWidget
from .expressionWidget import ExpressionWidget

try:
    from PySide6.QtGui import QIcon
    from PySide6.QtWidgets import QWidget, QTabWidget, QVBoxLayout
except ImportError:
    from PySide2.QtGui import QIcon # type: ignore
    from PySide2.QtWidgets import QWidget, QTabWidget, QVBoxLayout # type: ignore

from pxr import Usd

class CollectionWidget(QWidget):
    def __init__(self, collection: Usd.CollectionAPI = None, parent: QWidget = None):
        super(CollectionWidget, self).__init__(parent)

        mainLayout = QVBoxLayout()
        mainLayout.setContentsMargins(0,0,0,0)

        self._includeExcludeWidget = IncludeExcludeWidget(collection, self)

        # only create tab when usd version is greater then 23.11
        if Usd.GetVersion() >= (0, 23, 11):
            tabWidget = QTabWidget()
            tabWidget.currentChanged.connect(self.onTabChanged)

            self._expressionWidget = ExpressionWidget(collection, tabWidget, self.onExpressionChanged)
            tabWidget.addTab(self._includeExcludeWidget, QIcon(), "Include/Exclude")
            tabWidget.addTab(self._expressionWidget, QIcon(), "Expression")

            mainLayout.addWidget(tabWidget)
        else:
            mainLayout.addWidget(self._includeExcludeWidget)

        self.setLayout(mainLayout)


    if Usd.GetVersion() >= (0, 23, 11):
        def onTabChanged(self, index):
            self._includeExcludeWidget.update()
            self._expressionWidget.update()

        def onExpressionChanged(self):
            updateIncludeAll = len(self._includeExcludeWidget.getIncludedItems()) == 0 and len(self._includeExcludeWidget.getIncludedItems()) == 0 and self._includeExcludeWidget.getIncludeAll()
            if updateIncludeAll:
                self._includeExcludeWidget.setIncludeAll(False)
                print('"Include All" has been disabled for the expression to take effect.')