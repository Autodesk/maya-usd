from ..common.list import StringList
from ..common.resizable import Resizable
from ..common.menuButton import MenuButton
from .expressionRulesMenu import ExpressionMenu
from maya.OpenMaya import MGlobal

try:
    from PySide6.QtCore import QEvent, QObject  # type: ignore
    from PySide6.QtWidgets import QFrame, QWidget, QHBoxLayout, QVBoxLayout, QLineEdit, QSizePolicy, QApplication  # type: ignore
except ImportError:
    from PySide2.QtWidgets import QFrame, QWidget, QHBoxLayout, QVBoxLayout, QLineEdit, QSizePolicy # type: ignore

from pxr import Usd, Sdf

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
        EventFilter(self._include.list, self)
        EventFilter(self._exclude.list, self)
        
        self._include.list._model.dataChangedSignal.connect(self.updateCollection)

        self.setLayout(includeExcludeLayout)
        #self.update()

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
    
    def updateCollection(self):
        MGlobal.displayInfo("Update collection")
        includeList = []
        excludeList = []
        if self._include.list.items() is not None:
            includeList = list(self._include.list.items())
            MGlobal.displayInfo("Include list: ")
            self.printList(includeList)
        if self._exclude.list.items() is not None:
            excludeList = list(self._exclude.list.items())
            MGlobal.displayInfo("Exclude list: ")
            self.printList(excludeList)

        if self._collection is not None:
            self._collection.GetIncludesRel().SetTargets(includeList)
            self._collection.GetExcludesRel().SetTargets(excludeList)

        self._include.list.update_placeholder()
        self._exclude.list.update_placeholder()

    def printList(self, list):
        for item in list:
            MGlobal.displayInfo(item)

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
            MGlobal.displayInfo("Drop: ")
            mime_data = event.mimeData()
            MGlobal.displayInfo(mime_data.text())
            if mime_data.hasText():
                self.addItemToCollection(mime_data.text())
            event.acceptProposedAction()
            return True
        elif event.type() == QEvent.DragEnter:
            if obj == self.widget:
                self.widget.placeholder_label.hide()
            event.acceptProposedAction()
            return True
        elif event.type() == QEvent.DragMove:
            event.acceptProposedAction()
            return True
        elif event.type() == QEvent.DragLeave:
            return True

        return super().eventFilter(obj, event)

    def addItemToCollection(self, items):
        itemList = items.split('\n')
        for item in itemList:
            path = ""
            if ',' in item:
                path = item.split(',')[1]
            else:
                path = item
            
            if not self._validatePath(self.control._collection, path):
                return

            if self.control._collection is not None:
                if self.widget.headerTitle == "Include":
                    # Check if the path is already in the exclude list
                    targets = self.control._collection.GetExcludesRel().GetTargets()
                    if path in targets:
                        targets.remove(path)
                        #self.control._collection.GetExcludesRel().RemoveTarget(path)
                        self.control._collection.GetExcludesRel().SetTargets(targets)
                    self.control._collection.GetIncludesRel().AddTarget(path)
                elif self.widget.headerTitle == "Exclude":
                    # Check if the path is already in the include list
                    #if path in self.control._collection.GetIncludesRel().GetTargets():
                        #self.control._collection.GetIncludesRel().RemoveTarget(path)
                    targets = self.control._collection.GetIncludesRel().GetTargets()
                    if path in targets:
                        targets.remove(path)
                        #self.control._collection.GetExcludesRel().RemoveTarget(path)
                        self.control._collection.GetIncludesRel().SetTargets(targets)
                    self.control._collection.GetExcludesRel().AddTarget(path)

        self.control.update()

    def _validatePath(self, collection, path):
       
        if not Sdf.Path.IsValidPathString(path):
            raise ValueError("Invalid sdf path: " + path)

        stage = self.control._collection.GetPrim().GetStage()
        prim = stage.GetPrimAtPath(Sdf.Path(path))

        if not prim or not prim.IsValid():
            raise ValueError("Error: The dragged object is not in the same stage as the collection. Ensure that objects belong to the same stage before adding them")
        
        return True