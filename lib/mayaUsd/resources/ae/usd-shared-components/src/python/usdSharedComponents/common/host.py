from typing import Sequence
from pxr import Usd


class Host(object):
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
