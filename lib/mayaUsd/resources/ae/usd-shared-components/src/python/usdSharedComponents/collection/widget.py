from ..common.list import StringList
from ..common.resizable import Resizable

try:
    from PySide6.QtWidgets import QWidget, QVBoxLayout
except ImportError:
    from PySide2.QtWidgets import QWidget, QVBoxLayout # type: ignore

from pxr import Usd

class CollectionWidget(QWidget):
    def __init__(self, collection: Usd.CollectionAPI = None, parent: QWidget = None):
        super(CollectionWidget, self).__init__(parent)

        self._collection: Usd.CollectionAPI = collection

        includes = []
        excludes = []
        shouldIncludeAll = False

        if self._collection is not None:
            includeRootAttribute = self._collection.GetIncludeRootAttr()
            if includeRootAttribute.IsAuthored():
                shouldIncludeAll = self._collection.GetIncludeRootAttr().Get()

            for p in self._collection.GetIncludesRel().GetTargets():
                includes.append(p.pathString)
            for p in self._collection.GetExcludesRel().GetTargets():
                excludes.append(p.pathString)

        self.mainLayout = QVBoxLayout(self)
        self.mainLayout.setContentsMargins(0,0,0,0)

        self._include = StringList( includes, "Include", "Include all", self)
        self._include.cbIncludeAll.setChecked(shouldIncludeAll)
        self._include.cbIncludeAll.stateChanged.connect(self.onIncludeAllToggle)
        self._resizableInclude = Resizable(self._include, "USD_Light_Linking", "IncludeListHeight", self)

        self.mainLayout.addWidget(self._resizableInclude)

        self._exclude = StringList( excludes, "Exclude", "", self)
        self._resizableExclude = Resizable(self._exclude, "USD_Light_Linking", "ExcludeListHeight", self)

        self.mainLayout.addWidget(self._resizableExclude)
        self.mainLayout.addStretch(1)

    def onIncludeAllToggle(self):
        self._collection.GetIncludeRootAttr().Set(self._include.cbIncludeAll.isChecked())