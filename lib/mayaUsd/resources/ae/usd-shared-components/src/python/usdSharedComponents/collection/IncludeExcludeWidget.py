from ..common.list import StringList
from ..common.resizable import Resizable
from ..common.kebabMenu import KebabMenu
from .ExpressionRulesMenu import ExpressionMenu

try:
    from PySide6.QtWidgets import QFrame, QWidget, QHBoxLayout, QVBoxLayout# type: ignore
except ImportError:
    from PySide2.QtWidgets import QFrame, QWidget, QHBoxLayout, QVBoxLayout # type: ignore

from pxr import Usd

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
        menuButton = KebabMenu(self._expressionMenu, self)

        separator = QFrame()
        separator.setFrameShape(QFrame.VLine)

        headerWidget = QWidget(self)
        headerLayout = QHBoxLayout(headerWidget)
        headerLayout.addStretch(1)
        # Will need a separator for future designs
        #headerLayout.addWidget(separator)
        headerLayout.addWidget(menuButton)
        includeExcludeLayout.addWidget(headerWidget)

        self._include = StringList(includes, "Include", "Include all", self)
        self._include.cbIncludeAll.setChecked(shouldIncludeAll)
        self._include.cbIncludeAll.stateChanged.connect(self.onIncludeAllToggle)
        self._resizableInclude = Resizable(self._include, "USD_Light_Linking", "IncludeListHeight", self)
        includeExcludeLayout.addWidget(self._resizableInclude)

        self._exclude = StringList(excludes, "Exclude", "", self)
        self._resizableExclude = Resizable(self._exclude, "USD_Light_Linking", "ExcludeListHeight", self)
        includeExcludeLayout.addWidget(self._resizableExclude)

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