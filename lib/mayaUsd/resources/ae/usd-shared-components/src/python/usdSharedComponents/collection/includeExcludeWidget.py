from ..common.list import StringList
from ..common.resizable import Resizable
from ..common.menuButton import MenuButton
from .expressionRulesMenu import ExpressionMenu

try:
    from PySide6.QtWidgets import QFrame, QWidget, QHBoxLayout, QVBoxLayout, QLineEdit, QSizePolicy # type: ignore
except ImportError:
    from PySide2.QtWidgets import QFrame, QWidget, QHBoxLayout, QVBoxLayout, QLineEdit, QSizePolicy # type: ignore

from pxr import Usd

# TODO: support I8N
kSearchPlaceHolder = 'Search...'

class IncludeExcludeWidget(QWidget):
    def __init__(self, collection: Usd.CollectionAPI = None, parent: QWidget = None):
        super(IncludeExcludeWidget, self).__init__(parent)
        self._collection = collection

        shouldIncludeAll = False
        includes = []
        excludes = []

        if self._collection is not None:
            includeRootAttribute = self._collection.GetIncludeRootAttr()
            if includeRootAttribute.IsAuthored():
                shouldIncludeAll = self._collection.GetIncludeRootAttr().Get()

            for p in self._collection.GetIncludesRel().GetTargets():
                includes.append(p.pathString)
            for p in self._collection.GetExcludesRel().GetTargets():
                excludes.append(p.pathString)

        includeExcludeLayout = QVBoxLayout(self)
        includeExcludeLayout.setContentsMargins(0,0,0,0)

        self._expressionMenu = ExpressionMenu(self._collection, self)
        menuButton = MenuButton(self._expressionMenu, self)

        self._filterWidget = QLineEdit()
        self._filterWidget.setContentsMargins(0, 0, 0, 0)
        self._filterWidget.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed)
        self._filterWidget.setPlaceholderText(kSearchPlaceHolder)
        self._filterWidget.setClearButtonEnabled(True)

        separator = QFrame()
        separator.setFrameShape(QFrame.VLine)

        headerWidget = QWidget(self)
        headerWidget.setContentsMargins(0, 0, 0, 0)
        headerLayout = QHBoxLayout(headerWidget)
        headerLayout.setContentsMargins(0, 2, 2, 0)
        headerLayout.addWidget(self._filterWidget)
        headerLayout.addWidget(separator)
        headerLayout.addWidget(menuButton)
        includeExcludeLayout.addWidget(headerWidget)

        self._include = StringList(includes, "Include", "Include all", self)
        self._include.cbIncludeAll.setChecked(shouldIncludeAll)
        self._include.cbIncludeAll.stateChanged.connect(self.onIncludeAllToggle)
        self._resizableInclude = Resizable(self._include, "USD_Light_Linking", "IncludeListHeight", self, defaultSize=80)
        includeExcludeLayout.addWidget(self._resizableInclude)

        self._exclude = StringList(excludes, "Exclude", "", self)
        self._resizableExclude = Resizable(self._exclude, "USD_Light_Linking", "ExcludeListHeight", self, defaultSize=80)
        includeExcludeLayout.addWidget(self._resizableExclude)

        self._filterWidget.textChanged.connect(self._include.list._model.setFilter)
        self._filterWidget.textChanged.connect(self._exclude.list._model.setFilter)

        self.setLayout(includeExcludeLayout)

    def update(self):
        self._expressionMenu.update()

    def getIncludedItems(self):
        return self._include.list.items()

    def getExcludedItems(self):
        return self._exclude.list.items()
    
    def getIncludeAll(self):
        return self._include.cbIncludeAll.isChecked()

    def setIncludeAll(self, value: bool):
        self._include.cbIncludeAll.setChecked(value)

    def onIncludeAllToggle(self):
        self._collection.GetIncludeRootAttr().Set(self._include.cbIncludeAll.isChecked())