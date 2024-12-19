from ..data.stringListData import StringListData

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

    def getStrings(self) -> Sequence[AnyStr]:
        '''
        Retrieve the full list of all text strings.
        '''
        items = []
        if self._collection is None:
            return items

        if self._isInclude:
            targets = self._collection.GetIncludesRel().GetTargets()
        else:
            targets = self._collection.GetExcludesRel().GetTargets()
            
        for p in targets:
            items.append(p.pathString)
        return items

    def addStrings(self, items: Sequence[AnyStr]):
        '''
        Add the given strings to the model.
        '''
        if self._collection is None:
            return
        
        # Use a SdfChangeBlock to group all updates in a single USD recomposition.
        with Sdf.ChangeBlock():
            if self._isInclude:
                for path in items:
                    self._collection.GetIncludesRel().AddTarget(Sdf.Path(path))
            else:
                for path in items:
                    self._collection.GetExcludesRel().AddTarget(Sdf.Path(path))

    def removeStrings(self, items: Sequence[AnyStr]):
        '''
        Remove the given strings from the model.
        '''
        if self._collection is None:
            return
        
        with Sdf.ChangeBlock():
            if self._isInclude:
                for item in items:
                    self._collection.GetIncludesRel().RemoveTarget(Sdf.Path(item))
            else:
                for item in items:
                    self._collection.GetExcludesRel().RemoveTarget(Sdf.Path(item))

    def _isValidString(self, text) -> AnyStr:
        '''
        Validates if the string is valid and possibly alter it to make it valid
        or conform to the expected format or value. Return None if invalid.
        '''
        # We probably received the UFE path, which contains a Maya path and
        # a USD (Sdf) path separated by a comma. Extract the USD path.
        if self._collection is None:
            return None

        if "," in text:
            text = text.split(",")[1]
        if not Sdf.Path.IsValidPathString(text):
            return None

        stage = self._collection.GetPrim().GetStage()
        prim = stage.GetPrimAtPath(Sdf.Path(text))

        if not prim or not prim.IsValid():
            # TODO: maybe report a warning...? Would it be really useful?
            return None

        return text
