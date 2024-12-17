from typing import Sequence
from maya.OpenMaya import MGlobal

try:
    from PySide6.QtCore import (
        QStringListModel,
        Signal,
        QModelIndex,
    )
except:
    from PySide2.QtCore import (
        QStringListModel,
        Signal,
    )


class FilteredStringListModel(QStringListModel):
    '''
    A Qt string list model that can be filtered.
    '''
    filterChanged = Signal()   
    dataChangedSignal = Signal()

    def __init__(self, items: Sequence[str] = None, parent=None, headerTitle: str = "", controller = None):
        super(FilteredStringListModel, self).__init__(items if items else [], parent)
        self._unfilteredItems = items
        self._isFilteredEmpty = False
        self._filter = ""
        self.headerTitle = headerTitle
        self.controller = controller

    def setStringList(self, items: Sequence[str]):
        '''
        Override base class implementation to properly rebuild
        the filtered list.
        '''
        self._unfilteredItems = items
        self._isFilteredEmpty = False
        super(FilteredStringListModel, self).setStringList(items)
        self._rebuildFilteredModel()

    def _rebuildFilteredModel(self):
        '''
        Rebuild the model by applying the filter.
        '''
        if not self._filter:
            filteredItems = self._unfilteredItems
        else:
            filters = [filter.lower() for filter in self._filter.split('*')]
            filteredItems = [item for item in self._unfilteredItems if self._isValidItem(item, filters)]
        self._isFilteredEmpty = bool(self._unfilteredItems) and not bool(filteredItems)
        # Note: don't call our own version, otehrwise we would get infinite recursion.
        super(FilteredStringListModel, self).setStringList(filteredItems)

    def _isValidItem(self, item: str, filters: Sequence[str]):
        '''
        Verify if the item passes all the filters.

        We search each given filter in sequence, each one must match
        somewhere in the item starting at the end of the point where
        the preceeding filter ended:
        '''
        index = 0
        item = item.lower()
        for filter in filters:
            newIndex = item.find(filter, index)
            if newIndex < 0:
                return False
            index = newIndex + len(filter)
        return True
    
    def isFilteredEmpty(self):
        '''
        Verify if the model is empty because it was entirely filtered out.
        '''
        return self._isFilteredEmpty

    def setFilter(self, filter: str):
        '''
        Set the filter to be applied to the model and rebuild the model.
        '''
        if filter == self._filter:
            return
        self._filter = filter
        self._rebuildFilteredModel()
        self.filterChanged.emit()

    def removeRows(self, row, count, parent=QModelIndex()):
        MGlobal.displayInfo("removeRows")
        
        # Validate the range
        if row < 0 or row + count > len(self._unfilteredItems) or count <= 0:
            return False

        # Notify views about the rows to be removed
        self.beginRemoveRows(parent, row, row + count - 1)
        #self.removeItemToCollection(self._unfilteredItems[row])

        # Perform the data removal
        currentItems = self.stringList()
        del currentItems[row:row + count]
        self.setStringList(currentItems)

        MGlobal.displayInfo("removeRows, now we have: ")
        for i in self.stringList():
            MGlobal.displayInfo(i)

        # Notify views that rows have been removed
        self.endRemoveRows()
        self.dataChangedSignal.emit()

        return True
    '''
    def removeItemToCollection(self, item):
        if self.headerTitle == "Include":
            MGlobal.displayInfo("Remove include item")
            self.control._collection.GetIncludesRel().RemoveTarget(item)
        elif self.headerTitle == "Exclude":
            MGlobal.displayInfo("Remove exclude item")
            self.control._collection.GetExcludesRel().RemoveTarget(item)
    '''