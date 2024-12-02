from ..common.list import StringList
from ..common.list import _StringList
from ..common.resizable import Resizable
from ..common.kebabMenu import KebabMenu
from .ExpressionRulesMenu import ExpressionMenu

try:
    from PySide6.QtCore import QEvent, Signal, QObject  # type: ignore
    from PySide6.QtWidgets import QFrame, QWidget, QHBoxLayout, QVBoxLayout # type: ignore
except ImportError:
    from PySide2.QtWidgets import QFrame, QWidget, QHBoxLayout, QVBoxLayout # type: ignore

from pxr import Usd, Sdf, Tf

class IncludeExcludeWidget(QWidget):
    dropped = Signal(str)

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

        #self._includeExcludeMenu = IncludeExcludeMenu(self._collection, self)

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

        EventFilter(self._include.list, self)
        EventFilter(self._exclude.list, self)

        self.setLayout(includeExcludeLayout)
        self.update()

    def update(self):
        self._expressionMenu.update()
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

        self._include.list.items = includes
        self._exclude.list.items = excludes
        self._include.list.update_placeholder()
        self._exclude.list.update_placeholder()

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

class EventFilter(QObject):
    def __init__(self, widget, control):
        super().__init__(widget)
        self._widget = widget
        self.control = control
        self.widget.installEventFilter(self)
        self.widget.setAcceptDrops(True)

    @property
    def widget(self):
        return self._widget

    def eventFilter(self, obj: QObject, event: QEvent):
        if event.type() == QEvent.Drop:
            mime_data = event.mimeData()
            self.addItemToCollection(mime_data.text())
            return True
        elif event.type() == QEvent.DragEnter:
            event.acceptProposedAction()
            return True
        elif event.type() == QEvent.DragMove:
            event.acceptProposedAction()
            return True

        return super().eventFilter(obj, event)

    def addItemToCollection(self, item):
        path = ""
        if ',' in item:
            path = item.split(',')[1]
        else:
            path = item
        
        if not self._validatePath(self.control._collection, path):
            return

        if self.control._collection is not None:
            if self.widget.headerTitle == "Include":
                self.control._collection.GetIncludesRel().AddTarget(path)
            elif self.widget.headerTitle == "Exclude":
                self.control._collection.GetExcludesRel().AddTarget(path)

        self.control.update()
    '''
    def removeItemToCollection(self, item):
        if self.control._collection is not None:
            if self.widget.headerTitle == "Include":
                self.control._collection.GetIncludesRel().RemoveTarget(item)
            elif self.widget.headerTitle == "Exclude":
                self.control._collection.GetExcludesRel().RemoveTarget(item)

        self.control.update()
    '''

    def _validatePath(self, collection, path):
       
        if not Sdf.Path.IsValidPathString(path):
            raise ValueError("Invalid sdf path: " + path)

        stage = self.control._collection.GetPrim().GetStage()
        prim = stage.GetPrimAtPath(Sdf.Path(path))

        if not prim or not prim.IsValid():
            raise ValueError("Value must be a float or an int")("Error: The dragged object is not in the same stage as the collection. Ensure that objects belong to the same stage before adding them")
        
        return True