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

    # Item picking

    def canPick(self) -> bool:
        '''
        Return if there is a UI to pick items.
        '''
        return False
    
    def pick(self) -> Sequence[AnyStr]:
        '''
        Pick items and return the sequnce of picked items.
        '''
        return []

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