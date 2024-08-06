# USD Undo/Redo Support

## Motivation

The primary motivation for this service is to restore USD data model changes to its correct state after undo/redo calls. The current implementation for this system uses [SdfLayerStateDelegateBase](https://graphics.pixar.com/usd/docs/api/class_sdf_layer_state_delegate_base.html#details) by implementing a mechanism to collect an inverse of each authoring operation for undo purposes. This mechanism was inspired by Luma's USD undo/redo facilities found in [usdQt](https://github.com/LumaPictures/usd-qt) 

## Building Blocks

#### UsdUndoManager

The primary job of UsdUndoManager is to temporarily collect inverse edits for every state
change made to a SdfLayer. The UsdUndoManager later transfers the collected edits into
an UsdUndoableItem when asked. It has a global singleton instance and has to be told which
SdfLayer to track for changes. When a layer is set to be tracked, a UsdUndoStateDelegate
is created and set unto it. That delegate is how edits are detected.

#### UsdUndoStateDelegate

The state delegate is set on a SdfLayer and is invoked on every authoring operation
on that layer. This delegate is created by UsdUndoManager::trackLayerStates() when
called by StagesSubject::stageEditTargetChanged() and StagesSubject::onStageSet().
It creates function to do the inverse of each edits and adds those inverting functions
to the UsdUndoManager.

#### UsdUndoBlock

Similarly to how the SdfChangeBlock works, the UsdUndoBlock is meant to be instantiated
on the stack and is thus active for its lifetime. During that lifetime, it collects all
edits into a single undo operation. Nested UsdUndoBlock, for example instances declared
on the stack in different nested functions, work much like SdfChangeBlock. That means
only the top level UsdUndoBlock will transfer the edits to an UsdUndoableItem.

#### MayaUsdUndoBlock

This class derives from UsdUndoBlock. It creates its own UsdUndoableItem internally when
needed. Once the last MayaUsdUndoBlock is about to be destroyed, it creates a Maya command
to hold onto the UsdUndoableItem permanently and thus connect the item undo and redo
functions to the Maya undo and redo system.

It is important to note that inverse edits are ***only collected inside the scope of UsdUndoBlock***.
If no UsdUndoBlock or MayaUsdUndoBlock exist, then edits are not recorded and thus no undo/redo
can be done on them.

The choice of using a UsdUndoBlock or MayaUsdUndoBlock depends on circumstances. MayaUsdUndoBlock
is meant mainly for global code that need to record USD edits to be undoabe in the Maya und/redo
system directly. The UsdUndoBlock class is meant to be used inside other classes that have their
own undo and redo system. For example, it is used in UFE commands.

#### UsdUndoableItem

This is the object that ultimately stores all the edits. (Actually, it stores the invert
functions created by the UsdUndoStateDelegate and temporarily held by the UsdUndoManager.)
It has an undo function to undo all the recorded edits and an redo function to redo them
afterward. This is the object that is meant to be kept long-term to do the undo and redo.

Both undo() and redo() internally call doInvert() which calls each inverse edit function
in reverse order. Inside doInvert() there is a neat trick: a new UsdUndoBlock object is
created to collect the inverted edits. Thus undo reuses the item to collect what will be
needed to implement redo. A SdfChangeBlock object is also created to batch all the USD
notifications changes into a single operation, for performance reasons.

## How to use Undo/Redo service inside UFE Commands

Every UFE commands (e.g add newPrim, delete, reorder, etc...) must override execute(),
undo() and redo() functions. These functions are similar to Maya command's doIt(),
redoIt() and undoIt() functions. Inside the execute() call, USD data model state is
set for the first time. undo() can then be invoked to restore the state. Afterward,
redo() can be called to replay the command.  The method redo() must end-up in exactly
the same state as execute(). The following diagram shows how a UFE command object along
with its UsdUndoableItem object is placed in Maya's undo queue.

![](../../../doc/images/ufe_commands.png) 

***NOTE:*** Currently rename, parent, group commands haven't adopted this service yet.

##### Pseudo code in C++
``` cpp
UsdUndoableItem _undoableItem;

void UsdUndoYourCommand::undo() 
{
    _undoableItem.undo();
}

void UsdUndoYourCommand::redo()
{
    _undoableItem.redo();
}

void UsdUndoYourCommand::execute()
{
    UsdUndoBlock undoBlock(&_undoableItem);
    
    /* your usd data model changes */
}
```

## How to use Undo/Redo service outside UFE Commands ( e.g MPxCommand )

Collecting USD edits inside Maya MPxCommand happens automatically when a MayaUsdUndoBlock
is used. They are kept in a MayaUsdUndoBlockCmd. The idea is to perform the USD data model
changes while this this MayaUsdUndoBlock object exists on the stack. When the MayaUsdUndoBlock
expires then it stores the invert objects in a MayaUsdUndoBlockCmd for undo/redo purposes.

When this command executes, it will call no-op MPxCommand::doIt() to transfer the USD edits
in an internal UsdUndoableItem. Maya then pushes the command onto the Maya undo queue, and
thus its redoIt() and undoIt() can be used later to restore the state or replay the command.

## How to use Undo/Redo service in Python

For python users, we provide bindings for UsdUndoManager, UsdUndoableItem and UsdUndoBlock.
The signature for calling UsdUndoBlock is slightly different than C++. If you don't pass in
a UsdUndoableItem object, then a Maya command will automatically be created to hold the undo
and be undoable using teh usual Maya API. If a UsdUndoableItem is passed, then that item will
contain the undo information, and you will handle the undo and redo yourself.

##### Pseudo code in Python
``` python
import maya.cmds as cmds

# create a stage in memory
self.stage = Usd.Stage.CreateInMemory()

# track the layer state changes
mayaUsdLib.UsdUndoManager.trackLayerStates(self.stage.GetRootLayer())

# create undo block
with mayaUsdLib.UsdUndoBlock():
    prim = self.stage.DefinePrim('/World', 'Sphere')
    self.assertTrue(bool(prim))

#undo
cmds.undo()

#redo
cmds.redo()

# Explicit item used
undoItem = mayaUsdLib.UsdUndoableItem()
with mayaUsdLib.UsdUndoBlock(undoItem):
    prim = self.stage.DefinePrim('/World', 'Sphere')
    self.assertTrue(bool(prim))

#undo
undoItem.undo()

#redo
undoItem.redo()
```

***NOTE:*** Currently MayaUsdUndoBlockCmd command is only registered via mayaUsd plugin.
