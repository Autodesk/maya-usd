from typing import Sequence
from enum import Enum
from pxr import Usd, Sdf


class MessageType(Enum):
    '''
    Message type used to report messages in the DCC.
    '''
    INFO = 1
    WARNING = 2
    ERROR = 3


class Host(object):
    '''
    Class to interact with the host DCC app (Digital Contents Creation application).
    Sub-classed by the host DCC app to provide the functionality.
    '''
    _instance = None

    def __init__(self):
        raise RuntimeError("Call instance() instead")

    @classmethod
    def instance(cls):
        '''
        Retrieve the DCC-specific instance of this Host interface.
        '''
        if cls._instance is None:
            cls._instance = cls.__new__(cls)
        return cls._instance

    @classmethod
    def injectInstance(cls, host):
        '''
        Set the DCC-specific instance of this Host interface.
        '''
        cls._instance = host

    @property
    def canPick(self) -> bool:
        '''
        Verify if the DCC-specific instance of this Host interface can pick USD prims.
        '''
        return False

    @property
    def canDrop(self) -> bool:
        '''
        Verify if the DCC-specific instance of this Host interface can drag-and-drop USD prims.
        '''
        return True

    def pick(self, stage: Usd.Stage, *, dialogTitle: str = "") -> Sequence[Usd.Prim]:
        '''
        Pick USD prims.

        Must be implemented by the DCC-specific sub-class of this Host interface if the DCC
        supports picking USD prims.
        '''
        return None
    
    def reportMessage(self, message: str, msgType: MessageType=MessageType.ERROR):
        '''
        Report an error message using the DCC-specific logging system.
        By default, simply print the error with an "Error: " prefix.
        '''
        print('%s: %s' % (msgType.name, message))

    def getSelectionAsText(self) -> Sequence[str]:
        '''
        Retrieve the current selection as a list of item paths in text form.
        '''
        return []
    
    def setSelectionFromText(self, paths: Sequence[str]) -> bool:
        '''
        Set the DCC current selection to the given list of paths.
        '''
        return False
    
    def getStagePath(self, stage: Usd.Stage) -> str:
        '''
        Retrieve the DCC path to where the given stage lives in the DCC.
        This is used to convert USD collection data back into UFE path.
        '''
        return None
    
    def createCollectionData(self, prim: Usd.Prim, collection: Usd.CollectionAPI):
        '''
        Create the data to hold a USD collection.

        Can be implemented by the DCC-specific sub-class of this Host interface
        to return a DCC-specific implementation of the data, to support undo and
        redo, for example.
        '''
        from ..usdData.usdCollectionData import UsdCollectionData
        return UsdCollectionData(prim, collection)
    
    def createStringListData(self, collection: Usd.CollectionAPI, isInclude: bool):
        '''
        Create the data to hold a list of included or excluded items.

        Can be implemented by the DCC-specific sub-class of this Host interface
        to return a DCC-specific implementation of the data, to support undo and
        redo, for example.
        '''
        from ..usdData.usdCollectionStringListData import CollectionStringListData
        return CollectionStringListData(collection, isInclude)
    
    def openHelp(self) -> bool:
        '''
        Open the help URL in the DCC-specific help system.
        '''
        pass
