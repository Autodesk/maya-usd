
from usd_shared_components.common.host import Host, MessageType
from usd_shared_components.common.theme import Theme
from usd_shared_components.usdData.usdCollectionData import UsdCollectionData
from usd_shared_components.usdData.usdCollectionStringListData import CollectionStringListData

from maya.api.OpenMaya import MPxCommand, MFnPlugin, MGlobal, MSyntax, MArgDatabase
import mayaUsd.lib
import maya.mel as mel

from pxr import Usd
from typing import AnyStr, Sequence, Tuple

class _UndoItemHolder:
    '''
    Hold USD undo items temporarily to transfer them between the USD undo block context
    and the undoable Maya command that will hold the USD undo item to be undone and redone.

    We need a holder because there might be multiple nested undo contexts in flight at
    the same time due to Qt signal or USD notifications triggering UI updates. We need
    to identify which undo context corresponds to which Maya command. We do this by
    assigning a unique ID to each one, so they can know which undo item to transfer.

    The undo context creates the item and records the ID and then pass the ID to the
    Maya command so that it can retrieve the undo item and clean up this holder.
    '''
    _undoId = 0
    _undoItems = {}

    @classmethod
    def createUndoItem(cls) -> Tuple[int, mayaUsd.lib.UsdUndoableItem]:
        '''
        Create a new unique ID and an undo item. Returns both.
        '''
        id = cls._undoId
        cls._undoId += 1

        undoItem = mayaUsd.lib.UsdUndoableItem()

        cls._undoItems[id] = undoItem

        return (id, undoItem)
    
    @classmethod
    def getUndoItem(cls, id: int) -> mayaUsd.lib.UsdUndoableItem:
        '''
        Retrieve the undo item corresponding to the ID, if any.
        '''
        if id not in cls._undoItems:
            return None
        return cls._undoItems[id]
    
    @classmethod
    def removeUndoItem(cls, id: int) -> None:
        '''
        Remove (forget) the given ID.
        '''
        if id not in cls._undoItems:
            return None
        del cls._undoItems[id]


class _UsdUndoBlockContext:
    '''
    Python context manager (IOW, a class that can be used with Python's `with` keyword)
    that captures USD changes in a USD undo block so the USD changes can be transferred
    into a Maya command that can be undone and redone.

    The transfer is done via the _UndoItemHolder class above.
    '''
    def __init__(self, cmd):
        '''
        Create a USD undo block context manager with the Maya command that will
        be invoked to transfer the USD undo items.
        '''
        super(_UsdUndoBlockContext, self).__init__()
        self._id = -1
        self._undoItem = None
        self._cmd = cmd
        self._usdUndoBlock = None

    # Python context manager API / special methods.

    def __enter__(self):
        '''
        Create the undo item and its ID that will hold the USD edits
        and the USD undo block that will capture these undo items.
        '''
        # Note: protect against bad usage and calling __enter__ multiple times.
        if self._usdUndoBlock:
            self._usdUndoBlock.__exit__(None, None, None)

        self._id, self._undoItem = _UndoItemHolder.createUndoItem()
        self._usdUndoBlock = mayaUsd.lib.UsdUndoBlock(self._undoItem)
        self._usdUndoBlock.__enter__()

    def __exit__(self, exc_type, exc_val, exc_tb):
        '''
        Ends the capture of undo items and call the Maya command that will
        transfer them from the holder class and ultimately hold them.
        '''
        # Note: protect against bad usage and calling __exit__ before __enter__.
        if not self._usdUndoBlock:
            return
        
        # Call __exit__ to transfer all USD undo tracking to the self._undoItem
        self._usdUndoBlock.__exit__(exc_type, exc_val, exc_tb)
        self._usdUndoBlock = None

        # Call a Maya command in which the undo items can be transferred.
        # That Maya command will be added to the undo stack in Maya, allowing
        # undo and redo.
        #
        # Note: we need to use MEL to call the command so that it shows up properly
        #       in the undo UI. We we invoke it in Python with maya.cmds.abc
        #       it shows up as an empty string in the undo UI.
        mel.eval(self._cmd + ' ' + str(self._id))
        self._id = -1
        self._undoItem = None


class _UsdUndoBlockCommand(MPxCommand):
    '''
    Custom Maya undoable command that receives the USD changes from the
    USD undo block context manager class above via the undo item holder
    class above.

    This command is only meant to transfer USD edits from a USD undo block
    to a Maya command with a nice name. Its execution does not do work, but
    transfer work already done. It is not meant to be usable in scripts alone
    by itself but only via the _UsdUndoBlockContext class above.

    We receive the undo item ID as an argument and transfer the corresponding
    undo item from the _UndoItemHolder into this command to be undoable and
    redoable afterward.

    This command is meant to be sub-classed so that the name of the command
    in the undo stack is nice and relevant.
    '''

    def __init__(self):
        super(_UsdUndoBlockCommand, self).__init__()
        self._undoItem = None

    # MPxCommand command implementation.

    @classmethod
    def creator(cls):
        return cls()
    
    @classmethod
    def createSyntax(cls):
        '''
        The command receives a single argument: the undo item ID to transfer.
        '''
        syntax = MSyntax()
        syntax.addArg(syntax.kLong)
        return syntax
    
    def isUndoable(self):
        return self._undoItem is not None

    def doIt(self, args):
        '''
        Transfer the undo item corresponding to the given ID and remove them
        from the undo item holder. This command is their final resting place.
        '''
        argDB = MArgDatabase(self.createSyntax(), args)
        id = argDB.commandArgumentInt(0)
        self._undoItem = _UndoItemHolder.getUndoItem(id)
        _UndoItemHolder.removeUndoItem(id)

    def undoIt(self):
        if self._undoItem:
            self._undoItem.undo()

    def redoIt(self):
        if self._undoItem:
            self._undoItem.redo()


class _SetIncludeAllCommand(_UsdUndoBlockCommand):
    commandName = 'usdCollectionSetIncludeAll'
    def __init__(self):
        super().__init__()


class _RemoveAllIncludeExcludeCommand(_UsdUndoBlockCommand):
    commandName = 'usdCollectionRemoveAll'
    def __init__(self):
        super().__init__()


class _SetExansionRuleCommand(_UsdUndoBlockCommand):
    commandName = 'usdCollectionSetExpansionRule'
    def __init__(self):
        super().__init__()


class _SetMembershipExpressionCommand(_UsdUndoBlockCommand):
    commandName = 'usdCollectionSetMembershipExpression'
    def __init__(self):
        super().__init__()


class _AddItemsCommand(_UsdUndoBlockCommand):
    commandName = 'usdCollectionAddItems'
    def __init__(self):
        super().__init__()


class _RemoveItemsCommand(_UsdUndoBlockCommand):
    commandName = 'usdCollectionRemoveItems'
    def __init__(self):
        super().__init__()


_allCommandClasses = [
    _SetIncludeAllCommand,
    _RemoveAllIncludeExcludeCommand,
    _SetExansionRuleCommand,
    _SetMembershipExpressionCommand,
    _AddItemsCommand,
    _RemoveItemsCommand]

def registerCommands(pluginName):
    '''
    Register the collection commands so that they can be invoked by
    the undo context manager class above and be in the Maya undo stack.
    '''
    plugin = MFnPlugin.findPlugin(pluginName)
    if not plugin:
        MGlobal.displayWarning('Cannot register collection commands')
        return
    
    plugin = MFnPlugin(plugin)
    
    for cls in _allCommandClasses:
        try:
            plugin.registerCommand(cls.commandName, cls.creator, cls.createSyntax) 
        except Exception as ex:
            MGlobal.displayError(ex)

def deregisterCommands(pluginName):
    '''
    Unregister the collection commands.
    '''
    plugin = MFnPlugin.findPlugin(pluginName)
    if not plugin:
        MGlobal.displayWarning('Cannot deregister collection commands')
        return
    
    plugin = MFnPlugin(plugin)
    
    for cls in _allCommandClasses:
        try:
            plugin.deregisterCommand(cls.commandName) 
        except Exception as ex:
            MGlobal.displayError(ex)


class MayaCollectionData(UsdCollectionData):
    '''
    Maya-specfic USD collection data, to add undo/redo support.
    '''
    def __init__(self, prim: Usd.Prim, collection: Usd.CollectionAPI):
        super().__init__(prim, collection)

    # Include and exclude

    def setIncludeAll(self, state: bool):
        with _UsdUndoBlockContext(_SetIncludeAllCommand.commandName):
            super().setIncludeAll(state)

    def removeAllIncludeExclude(self):
        with _UsdUndoBlockContext(_RemoveAllIncludeExcludeCommand.commandName):
            super().removeAllIncludeExclude()

    # Expression

    def setExpansionRule(self, rule):
        with _UsdUndoBlockContext(_SetExansionRuleCommand.commandName):
            super().setExpansionRule(rule)

    def setMembershipExpression(self, textExpression: AnyStr):
        with _UsdUndoBlockContext(_SetMembershipExpressionCommand.commandName):
            super().setMembershipExpression(textExpression)


class MayaStringListData(CollectionStringListData):
    '''
    Maya-specfic string list data, to add undo/redo support.
    '''
    def __init__(self, collection, isInclude: bool):
        super().__init__(collection, isInclude)

    def addStrings(self, items: Sequence[AnyStr]):
        '''
        Add the given strings to the model.
        '''
        with _UsdUndoBlockContext(_AddItemsCommand.commandName):
            super().addStrings(items)

    def removeStrings(self, items: Sequence[AnyStr]):
        '''
        Remove the given strings from the model.
        '''
        with _UsdUndoBlockContext(_RemoveItemsCommand.commandName):
            super().removeStrings(items)

class MayaTheme(Theme):
    def __init__(self):
        self._palette = None
        self._icons = {}

    def themeTab(self, tab):
        super().themeTab(tab)
        tab.setDocumentMode(False)
        tab.tabBar().setExpanding(False)

class MayaHost(Host):
    '''Class to host and override Maya-specific functions for the collection API.'''
    def __init__(self):
        pass

    @property
    def canPick(self) -> bool:
        return False

    @property
    def canDrop(self) -> bool:
        return True

    def pick(self, stage: Usd.Stage, *, dialogTitle: str = "") -> Sequence[Usd.Prim]:
        return [] # nothing to do yet

    def reportMessage(self, message: str, msgType: MessageType=MessageType.ERROR):
        if msgType == MessageType.INFO:
            MGlobal.displayInfo(message)
        elif msgType == MessageType.WARNING:
            MGlobal.displayWarning(message)
        elif msgType == MessageType.ERROR:
            MGlobal.displayError(message)

    def createCollectionData(self, prim: Usd.Prim, collection: Usd.CollectionAPI):
        return MayaCollectionData(prim, collection)
    
    def createStringListData(self, collection: Usd.CollectionAPI, isInclude: bool):
        return MayaStringListData(collection, isInclude)
