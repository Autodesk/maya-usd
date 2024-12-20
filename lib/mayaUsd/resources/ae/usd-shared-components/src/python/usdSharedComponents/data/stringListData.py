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

    def addStrings(self, items: Sequence[AnyStr]):
        '''
        Add the given strings to the model.
        '''
        pass

    def removeStrings(self, items: Sequence[AnyStr]):
        '''
        Remove the given strings from the model.
        '''
        pass

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
        for text in multiLineText.split("\n"):
            text = self._isValidString(text)
            if text is None:
                continue
            items.append(text)

        self.addStrings(items)
