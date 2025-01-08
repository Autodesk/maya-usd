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
    
    def setIncludeAll(self, state: bool):
        '''
        Sets if the collection should include all items by default.
        '''
        pass
    
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

    def removeAllIncludeExclude(self):
        '''
        Remove all included and excluded items.
        '''
        pass

    # Expression

    def getExpansionRule(self):
        '''
        Returns expansion rule as a USD token.
        '''
        return None
    
    def setExpansionRule(self, rule):
        '''
        Sets the expansion rule as a USD token.
        '''
        pass

    def getMembershipExpression(self) -> AnyStr:
        '''
        Returns the membership expression as text.
        '''
        return None
    
    def setMembershipExpression(self, textExpression: AnyStr):
        '''
        Set the textual membership expression.
        '''
        pass