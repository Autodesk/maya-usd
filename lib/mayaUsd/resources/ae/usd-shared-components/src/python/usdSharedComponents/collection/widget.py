from .includeExcludeWidget import IncludeExcludeWidget
from .expressionWidget import ExpressionWidget
from ..common.theme import Theme
from ..common.host import Host
from ..data.collectionData import CollectionData

try:
    from PySide6.QtCore import Qt  # type: ignore
    from PySide6.QtGui import QIcon, QWheelEvent  # type: ignore
    from PySide6.QtWidgets import QTabBar, QTabWidget, QVBoxLayout, QWidget  # type: ignore
except ImportError:
    from PySide2.QtCore import Qt  # type: ignore
    from PySide2.QtGui import QIcon, QWheelEvent  # type: ignore
    from PySide2.QtWidgets import QTabBar, QTabWidget, QVBoxLayout, QWidget  # type: ignore

from pxr import Usd


class NonScrollingTabBar(QTabBar):
    """A QTabBar that does not switch tabs when the mouse wheel is used."""

    def __init__(self, parent: QWidget = None):
        super(NonScrollingTabBar, self).__init__(parent)

    def wheelEvent(self, event: QWheelEvent):
        # Ignore the event. This cannot be done using an event filter as the
        # event needs to be properly ignored to 'bubble up' the parent chain.
        event.ignore()


def _hasOnlyExpression(collData: CollectionData) -> bool:
    '''
    Verify if the collection data has a membership expression and
    no include/exclude items.
    '''
    if not collData:
        return False
    
    # No membership expression? Then the data is not only expression.
    if not collData.getMembershipExpression():
        return False
    
    # Some inclusion? Then the data is not *only* expression.
    if collData.getIncludeData().getStrings():
        return False

    # Some exclusion? Then the data is not *only* expression.
    if collData.getExcludeData().getStrings():
        return False
    
    # Include all prims? Then the data is not *only* expression.
    if collData.includesAll():
        return False
    
    # We got expression and nothing else.
    return True


class CollectionWidget(QWidget):

    def __init__(
        self,
        prim: Usd.Prim = None,
        collection: Usd.CollectionAPI = None,
        parent: QWidget = None,
    ):
        super(CollectionWidget, self).__init__(parent)

        self._collData = Host.instance().createCollectionData(prim, collection)

        mainLayout = QVBoxLayout()
        mainLayout.setContentsMargins(0, 0, 0, 0)

        self._includeExcludeWidget = IncludeExcludeWidget(self._collData, self)
        self._tabWidget = None

        # only create tab when usd version is greater then 23.11
        if Usd.GetVersion() >= (0, 23, 11):
            self._tabWidget = QTabWidget()
            self._tabWidget.setTabBar(NonScrollingTabBar(self._tabWidget))
            self._tabWidget.currentChanged.connect(self.onTabChanged)

            self._expressionWidget = ExpressionWidget(self._collData, self._tabWidget)
            self._tabWidget.addTab(self._includeExcludeWidget, QIcon(), "Include/Exclude")
            self._tabWidget.addTab(self._expressionWidget, QIcon(), "Expression")

            if _hasOnlyExpression(self._collData):
                self._tabWidget.setCurrentWidget(self._expressionWidget)

            offset: int = Theme.instance().uiScaled(4)
            self._includeExcludeWidget.setContentsMargins(0, offset, 0, 0)
            self._expressionWidget.setContentsMargins(0, offset, 0, 0)

            Theme.instance().themeTab(self._tabWidget)
            mainLayout.addWidget(self._tabWidget)
        else:
            mainLayout.addWidget(self._includeExcludeWidget)

        self.setLayout(mainLayout)

    def setCollection(self, prim: Usd.Prim = None, collection: Usd.CollectionAPI = None):
        self._collData.setCollection(prim, collection)

    if Usd.GetVersion() >= (0, 23, 11):

        def onTabChanged(self, index):
            self._includeExcludeWidget.update()
            self._expressionWidget.update()
