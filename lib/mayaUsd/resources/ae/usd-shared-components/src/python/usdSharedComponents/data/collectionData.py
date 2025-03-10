from typing import AnyStr, Sequence

from .stringListData import StringListData

try:
    from PySide6.QtCore import QObject, Signal  # type: ignore
except ImportError:
    from PySide2.QtCore import QObject, Signal  # type: ignore

class CollectionData(QObject):
    dataChanged = Signal()

    def __init__(self):
        super().__init__()

    # USD Stage information
    
    def getStage(self):
        '''
        Returns the USD stage in which the collection lives.
        '''
        return None

    # Include and exclude

    def includesAll(self) -> bool:
        '''
        Verify if the collection includes all by default.
        '''
        return False
    
    def setIncludeAll(self, state: bool) -> bool:
        '''
        Sets if the collection should include all items by default.
        Return True if successfully set.
        Return False if already set to the same value.
        '''
        return False
    
    def getIncludeData(self) -> StringListData:
        '''
        Returns the included items string list.
        '''
        return None

    def getExcludeData(self) -> StringListData:
        '''
        Returns the excluded items string list.
        '''
        return None

    def removeAllIncludeExclude(self) -> bool:
        '''
        Remove all included and excluded items.
        Return True if successfully removed.
        Return False if already empty.
        '''
        return False

    # Expression

    def getExpansionRule(self):
        '''
        Returns expansion rule as a USD token.
        '''
        return None
    
    def setExpansionRule(self, rule) -> bool:
        '''
        Sets the expansion rule as a USD token.
        Return True if successfully set.
        Return False if already set to the same value.
        '''
        return False

    def getMembershipExpression(self) -> AnyStr:
        '''
        Returns the membership expression as text.
        '''
        return None
    
    def setMembershipExpression(self, textExpression: AnyStr) -> bool:
        '''
        Set the textual membership expression.
        Return True if successfully set.
        Return False if already set to the same value.
        '''
        return False