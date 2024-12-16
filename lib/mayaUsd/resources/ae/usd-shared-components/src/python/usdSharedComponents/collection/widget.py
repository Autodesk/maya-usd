from .includeExcludeWidget import IncludeExcludeWidget
from .expressionWidget import ExpressionWidget
from ..usdData.usdCollectionData import UsdCollectionData

try:
    from PySide6.QtCore import Slot  # type: ignore
    from PySide6.QtGui import QIcon  # type: ignore
    from PySide6.QtWidgets import QWidget, QTabWidget, QVBoxLayout  # type: ignore
except ImportError:
    from PySide2.QtCore import Slot  # type: ignore
    from PySide2.QtGui import QIcon  # type: ignore
    from PySide2.QtWidgets import QWidget, QTabWidget, QVBoxLayout  # type: ignore

from pxr import Usd, Tf


class CollectionWidget(QWidget):

    def __init__(
        self,
        prim: Usd.Prim = None,
        collection: Usd.CollectionAPI = None,
        parent: QWidget = None,
    ):
        super(CollectionWidget, self).__init__(parent)

        self._collection: Usd.CollectionAPI = collection
        self._prim: Usd.Prim = prim
        self._collData = UsdCollectionData(prim, collection)

        mainLayout = QVBoxLayout()
        mainLayout.setContentsMargins(0, 0, 0, 0)

        self._includeExcludeWidget = IncludeExcludeWidget(self._collData, self)

        # this is used to avoid infinite loop when updating the UI
        self._updatingUI: bool = False

        # only create tab when usd version is greater then 23.11
        if Usd.GetVersion() >= (0, 23, 11):
            tabWidget = QTabWidget()
            tabWidget.currentChanged.connect(self.onTabChanged)

            self._expressionWidget = ExpressionWidget(self._collData, tabWidget, self.onExpressionChanged)
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
            updateIncludeAll = (
                len(self._collData._includes.getStrings()) == 0
                and len(self._collData._includes.getStrings()) == 0
                and self._collData.includesAll()
            )
            if updateIncludeAll:
                self._collData.setIncludeAll(False)
                print(
                    '"Include All" has been disabled for the expression to take effect.'
                )
