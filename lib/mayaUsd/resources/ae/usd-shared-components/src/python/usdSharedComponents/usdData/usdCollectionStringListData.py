from ..data.stringListData import StringListData
from ..data.collectionData import CollectionData
from .validator import validateCollection, detectDataConflict

from typing import AnyStr, Sequence
from pxr import Sdf

class CollectionStringListData(StringListData):
    def __init__(self, collection, isInclude: bool):
        super().__init__()
        self._isInclude = isInclude
        self.setCollection(collection)

    def setCollection(self, collection):
        '''
        Sets which USD collection is held by this data class.
        '''
        self._collection = collection
        self._prim = collection.GetPrim() if collection else None

    # Data conflicts

    def hasDataConflict(self) -> bool:
        '''
        Verify if the collection has both a membership expression and
        some explicit inclusions or exclusions.
        '''
        return self._collData.hasDataConflict()

    def reportDataConflict(self, isConflicted: bool):
        '''
        Report about data conflicts.
        '''
        return self._collData.reportDataConflict(isConflicted)

    # String list functions.

    @validateCollection([])
    def getStrings(self) -> Sequence[AnyStr]:
        '''
        Retrieve the full list of all text strings.
        '''
        items = []
        if self._isInclude:
            targets = self._collection.GetIncludesRel().GetTargets()
        else:
            targets = self._collection.GetExcludesRel().GetTargets()
            
        for p in targets:
            items.append(p.pathString)
        return items

    @validateCollection(False)
    @detectDataConflict
    def addStrings(self, items: Sequence[AnyStr]) -> bool:
        '''
        Add the given strings to the model.
        '''
        # Use a SdfChangeBlock to group all updates in a single USD recomposition.
        existingItems = set(self.getStrings())
        items = set(items).difference(existingItems)
        with Sdf.ChangeBlock():
            if self._isInclude:
                for path in items:
                    self._collection.GetIncludesRel().AddTarget(Sdf.Path(path))
            else:
                for path in items:
                    self._collection.GetExcludesRel().AddTarget(Sdf.Path(path))
        return True

    @validateCollection(False)
    @detectDataConflict
    def removeStrings(self, items: Sequence[AnyStr]) -> bool:
        '''
        Remove the given strings from the model.
        '''
        with Sdf.ChangeBlock():
            if self._isInclude:
                for item in items:
                    self._collection.GetIncludesRel().RemoveTarget(Sdf.Path(item))
            else:
                for item in items:
                    self._collection.GetExcludesRel().RemoveTarget(Sdf.Path(item))
        return True

    @validateCollection(None)
    def convertToCollectionString(self, text) -> AnyStr:
        '''
        Validates if the string is valid and possibly alter it to make it valid
        or conform to the expected format or value. Return None if invalid.
        '''
        # We probably received the UFE path, which contains a Maya path and
        # a USD (Sdf) path separated by a comma. Extract the USD path.
        if "," in text:
            text = text.split(",")[1]
        if not Sdf.Path.IsValidPathString(text):
            return None

        stage = self._prim.GetStage()
        prim = stage.GetPrimAtPath(Sdf.Path(text))

        if not prim or not prim.IsValid():
            return None
        
        # We don't allow adding a prim to its own collection.
        if prim == self._prim:
            return None

        return text

    @validateCollection([])
    def convertToItemPaths(self, items: Sequence[str]) -> Sequence[str]:
        from ..common.host import Host
        stagePath = Host.instance().getStagePath(self._prim.GetStage())
        # Protect in case the stage was deleted.
        if not stagePath:
            return []
        
        return ['%s,%s' % (stagePath, item) for item in items if items]
