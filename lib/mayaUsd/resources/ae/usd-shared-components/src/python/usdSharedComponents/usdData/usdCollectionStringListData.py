from ..data.stringListData import StringListData
from .validator import validateCollection
from ..common.host import Host, MessageType

from typing import AnyStr, Sequence, Tuple
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

    @validateCollection([])
    def getSuggestions(self) -> Sequence[AnyStr]:
        suggestions = []

        stage = self._prim.GetStage()
        for childPrim in stage.Traverse():
            suggestions.append(childPrim.GetPath().pathString)

        return suggestions

    @validateCollection(False)
    def addStrings(self, items: Sequence[AnyStr]) -> bool:
        '''
        Add the given strings to the model.
        '''
        # Use a SdfChangeBlock to group all updates in a single USD recomposition.
        existingItems = set(self.getStrings())
        items = set(items).difference(existingItems)
        addedNewIncludes = False
        failedPaths = []
        with Sdf.ChangeBlock():
            if self._isInclude:
                for path in items:
                    try:
                        self._collection.GetIncludesRel().AddTarget(Sdf.Path(path))
                        addedNewIncludes = True
                    except Exception:
                        failedPaths.append(path)
            else:
                for path in items:
                    try:
                        self._collection.GetExcludesRel().AddTarget(Sdf.Path(path))
                    except Exception:
                        failedPaths.append(path)

        # If the list of included strings was empty and current items has elements, it means
        # a new element was added to the list. For this case, deselect "Include all"
        # and show the user an message stating that it happened.
        if addedNewIncludes and not existingItems and items:
            self._collection.GetIncludeRootAttr().Set(False)
            Host.instance().reportMessage(
                "'Include All' has been disabled for the included prim(s) to take effect, in collection: %s." 
                % self._collection.GetName(), 
                MessageType.INFO)

        if failedPaths:
            Host.instance().reportMessage(
                """Relationship target paths must be absolute prim, property or mapper paths.
                Can't add the following prims to the {0} list:
                {1}""".format("Includes" if self._isInclude else "Excludes", failedPaths))

        return addedNewIncludes

    @validateCollection(False)
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

    @validateCollection(False)
    def replaceStrings(self, oldString, newString) -> bool:
        if newString == oldString:
            return True
        success: bool = False
        with Sdf.ChangeBlock():
            success = self.addStrings([newString]) and self.removeStrings([oldString])
        return success

    @validateCollection((None, "Could not add prim to the collection."))
    def convertToCollectionString(self, text) -> Tuple[AnyStr, AnyStr]:
        '''
        Validates if the string is valid and possibly alter it to make it valid
        or conform to the expected format or value. Return None if invalid.
        '''
        # We probably received the UFE path, which contains a Maya path and
        # a USD (Sdf) path separated by a comma. Extract the USD path.
        if "," in text:
            text = text.split(",")[1]
        if not Sdf.Path.IsValidPathString(text):
            return None, "Only valid USD paths can be added."

        stage = self._prim.GetStage()
        prim = stage.GetPrimAtPath(Sdf.Path(text))

        if not prim or not prim.IsValid():
            return None, "Only objects in the same stage as the collection can be added."

        # We don't allow adding a prim to its own collection.
        if prim == self._prim:
            return None, "Prim cannot be added to its own collection."

        return text, None

    @validateCollection([])
    def convertToItemPaths(self, items: Sequence[str]) -> Sequence[str]:
        from ..common.host import Host
        stagePath = Host.instance().getStagePath(self._prim.GetStage())
        # Protect in case the stage was deleted.
        if not stagePath:
            return []

        return ['%s,%s' % (stagePath, item) for item in items if items]
