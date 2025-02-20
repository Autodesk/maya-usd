from typing import Sequence
from ..data.stringListData import StringListData

try:
    from PySide6.QtCore import (
        QStringListModel,
        Signal,
        Qt
    )
except:
    from PySide2.QtCore import (
        QStringListModel,
        Signal,
        Qt
    )


class FilteredStringListModel(QStringListModel):
    '''
    A Qt string list model that can be filtered.
    '''
    filterChanged = Signal()

    def __init__(self, data: StringListData, parent=None):
        super(FilteredStringListModel, self).__init__(data.getStrings() if data else [], parent)
        self._collData = data
        self._isFilteredEmpty = False
        self._filter = ""

        self._collData.dataChanged.connect(self._onDataChanged)

    def _onDataChanged(self):
        self._rebuildFilteredModel()

    def _rebuildFilteredModel(self):
        '''
        Rebuild the model by applying the filter.
        '''
        if not self._filter:
            filteredItems = self._collData.getStrings()
        else:
            filters = [filter.lower() for filter in self._filter.split('*')]
            filteredItems = [item for item in self._collData.getStrings() if self._isValidItem(item, filters)]
        self._isFilteredEmpty = self._filter and not bool(filteredItems)
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
    
    def setData(self, index, value, role):
        if role == Qt.DisplayRole or role ==  Qt.ItemDataRole:
            oldValue = self.data(index, role)
            value = value.strip()
            if value:
                self._collData.replaceStrings(oldValue, value)
        return super(QStringListModel, self).setData(index, value, role)

    def flags(self, index):
        '''
        Return the flags for the given index.
        '''
        return super(FilteredStringListModel, self).flags(index) | Qt.ItemFlag.ItemIsEditable

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
