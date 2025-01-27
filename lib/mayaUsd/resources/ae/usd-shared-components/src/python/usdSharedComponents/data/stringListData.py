from typing import AnyStr, Sequence

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

    def _isValidString(self, s) -> AnyStr:
        '''
        Validates if the string is valid and possibly alter it to make it valid
        or conform to the expected format or value. Return None if invalid.
        '''
        return None

    # Function provided to help using this class, call
    # other functions above.

    def addMultiLineStrings(self, multiLineText):
        items = []
        reportedError = False
        for text in multiLineText.split("\n"):
            text = self._isValidString(text)
            if not text:
                if not reportedError:
                    reportedError = True
                    from ..common.host import Host
                    Host.instance().reportMessage("Only objects in the same stage as the collection can be added.")
                continue
            items.append(text)

        self.addStrings(items)
