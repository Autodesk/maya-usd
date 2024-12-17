from .includeExcludeWidget import IncludeExcludeWidget
from .expressionWidget import ExpressionWidget
from ..common.theme import Theme

try:
    from PySide6.QtCore import QEvent, QObject, Qt, Slot  # type: ignore
    from PySide6.QtGui import QIcon, QWheelEvent  # type: ignore
    from PySide6.QtWidgets import QTabBar, QTabWidget, QVBoxLayout, QWidget  # type: ignore
except ImportError:
    from PySide2.QtCore import QEvent, QObject, Qt, Slot  # type: ignore
    from PySide2.QtGui import QIcon, QWheelEvent  # type: ignore
    from PySide2.QtWidgets import QTabBar, QTabWidget, QVBoxLayout, QWidget  # type: ignore

from pxr import Usd, Tf


class NonScrollingTabBar(QTabBar):
    """A QTabBar that does not switch tabs when the mouse wheel is used."""

    def __init__(self, parent: QWidget = None):
        super(NonScrollingTabBar, self).__init__(parent)

    def wheelEvent(self, event: QWheelEvent):
        # Ignore the event. This cannot be done using an event filter as the
        # event needs to be properly ignored to 'bubble up' the parent chain.
        event.ignore()

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

        mainLayout = QVBoxLayout()
        mainLayout.setContentsMargins(0, 0, 0, 0)

        self._includeExcludeWidget = IncludeExcludeWidget(prim, collection, self)

        # this is used to avoid infinite loop when updating the UI
        self._updatingUI: bool = False

        # register to the object changed notification
        self._noticeKey = Tf.Notice.Register(
            Usd.Notice.ObjectsChanged, self.onObjectsChanged, self._prim.GetStage()
        )

        self._tabWidget = None

        # only create tab when usd version is greater then 23.11
        if Usd.GetVersion() >= (0, 23, 11):
            self._tabWidget = QTabWidget()
            self._tabWidget.setTabBar(NonScrollingTabBar(self._tabWidget))
            self._tabWidget.currentChanged.connect(self.onTabChanged)
            self._tabWidget.setDocumentMode(True)

            self._expressionWidget = ExpressionWidget(
                collection, self._tabWidget, self.onExpressionChanged
            )
            self._tabWidget.addTab(
                self._includeExcludeWidget, QIcon(), "Include/Exclude"
            )
            self._tabWidget.addTab(self._expressionWidget, QIcon(), "Expression")

            offset: int = Theme.instance().uiScaled(4)
            self._includeExcludeWidget.setContentsMargins(0, offset, 0, 0)
            self._expressionWidget.setContentsMargins(0, offset, 0, 0)

            self._tabWidget.tabBar().setExpanding(True)
            self._tabWidget.tabBar().setCursor(Qt.ArrowCursor)
            mainLayout.addWidget(self._tabWidget)
        else:
            mainLayout.addWidget(self._includeExcludeWidget)

        self.setLayout(mainLayout)

    def updateUI(self):
        self._updatingUI = True
        self._includeExcludeWidget.updateUI()
        self._updatingUI = False

    def onObjectsChanged(self, notice, sender):
        # TODO: check if the collection was actually touched by this change!
        if not self._updatingUI:
            self.updateUI()

    @Slot()
    def cleanup(self):
        # unregister from the object changed notification
        self._noticeKey.Revoke()

    if Usd.GetVersion() >= (0, 23, 11):

        def onTabChanged(self, index):
            self._includeExcludeWidget.update()
            self._expressionWidget.update()

        def onExpressionChanged(self):
            updateIncludeAll = (
                len(self._includeExcludeWidget.getIncludedItems()) == 0
                and len(self._includeExcludeWidget.getIncludedItems()) == 0
                and self._includeExcludeWidget.getIncludeAll()
            )
            if updateIncludeAll:
                self._includeExcludeWidget.setIncludeAll(False)
                print(
                    '"Include All" has been disabled for the expression to take effect.'
                )
