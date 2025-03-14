from typing import AnyStr, Sequence, Tuple

try:
    from PySide6.QtCore import QObject, Signal  # type: ignore
except ImportError:
    from PySide2.QtCore import QObject, Signal  # type: ignore

class StringListData(QObject):

    dataChanged = Signal()

    def __init__(self):
        super().__init__()

    # Functions to be implemented by sub-class

    def getStrings(self) -> Sequence[AnyStr]:
        '''
        Retrieve the full list of all text strings.
        '''
        return []

    def getSuggestions(self) -> Sequence[AnyStr]:
        '''
        Retrieve suggestions for paths completion.
        '''
        return []

    def addStrings(self, items: Sequence[AnyStr]) -> bool:
        '''
        Add the given strings to the model.
        Return True if successfully added.
        '''
        return False

    def removeStrings(self, items: Sequence[AnyStr]) -> bool:
        '''
        Remove the given strings from the model.
        Return True if successfully removed.
        Return False if already empty.
        '''
        return False
    
    def replaceStrings(self, oldString, newString) -> bool:
        '''
        Remove oldString from the model then add newString.
        Return True if successfully removed then added.
        Return False if already empty.
        '''
        return False

    def convertToCollectionString(self, s) -> Tuple[AnyStr, AnyStr]:
        '''
        Validates if the string is valid and possibly alter it to make it valid
        or conform to the expected format or value. Return None if invalid.
        '''
        return None
    
    def convertToItemPaths(self, items: Sequence[str]) -> Sequence[str]:
        '''
        Convert back the given strings in the format kept by this collection
        into their original format. Should be the opposite of the transformation
        made in the convertToCollectionString function above.
        '''
        return []

    # Function provided to help using this class, call
    # other functions above.

    def addMultiLineStrings(self, multiLineText):
        self.addStrings(self._splitMultiLineStrings(multiLineText))

    def removeMultiLineStrings(self, multiLineText):
        self.removeStrings(self._splitMultiLineStrings(multiLineText))

    def _splitMultiLineStrings(self, multiLineText):
        items = []
        reportedError = set()
        for text in multiLineText.split("\n"):
            text, error = self.convertToCollectionString(text)
            if not text:
                if error not in reportedError:
                    reportedError.add(error)
                    from ..common.host import Host
                    Host.instance().reportMessage(error)
                continue
            items.append(text)

        return items

