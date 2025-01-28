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

    def convertToCollectionString(self, s) -> AnyStr:
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
        items = []
        reportedError = False
        for text in multiLineText.split("\n"):
            text = self.convertToCollectionString(text)
            if not text:
                if not reportedError:
                    reportedError = True
                    from ..common.host import Host
                    Host.instance().reportMessage("Only objects in the same stage as the collection can be added.")
                continue
            items.append(text)

        self.addStrings(items)
