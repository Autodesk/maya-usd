# OpUndo Undo/Redo Helper Framework

## Goal

The OpUndo system is a set of C++ classes with the goal of supporting undo/redo
when using any combination of frameworks, each with their own way of recording
work that would ptentially need to be undone and redone later.

Note that the goal OpUndo system is *only* to record what is done and would need
to be undone and redone. It is *not* an undo/redo stack nor an undo manager.
For that, we still rely on Maya's built-in undo system.

The reason we need this is that the MayaUSD plugin uses multiple frameworks that
each have their own undo system or none at all. For example, the plugin uses:

- UFE
- USD
- OpenMaya
- MEL
- pure C++ code
- Python


## Building Blocks

The OpUndo system is made of three main building blocks:
- `OpUndoItem`: a generic single item that can be undone or redone.
- `OpUndoItemList`: a group of multiple individual items.
- `OpUndoItemRecorder`: an automatic recorder of undo item.

A few other elements are provided for convenience.
- `OpUndoItemMuting`: to temporarily turn off the recording of undo items.
- `OpUndoItems.h`: a header containing the implementations of the OpUndoItem
                   for typical operations.

## How to Use the Framework

The general pattern to use the framework is as follow:
- Declare an `OpUndoItemList` to hold the things that will need to be undone
  and redone.
- When a large operation that need to do many individual steps to be potentially
  undone and redone, declare a `OpUndoItemRecorder`.
- Within the large operation, use the `OpUndoItem` implementation already provided
  to do the actual work. For example, use `MDagModifierUndoItem` to do DAG operations,
  `SelectionUndoItem` to change the selection, `PythonUndoItem` to execute Python, etc.
- Note that you don't need to manually add these items to your `OpUndoItemList`. The
  `OpUndoItemRecorder` will do that automatically for you.
- When Maya asks your operation to be undone or redone, call `undo` or `redo` on the
  `OpUndoItemList` that holds all the undo items.

So, a concrete example would be a Maya command derived from `MPxCommand`. Your
`MPxCommand` would contain an `OpUndoItemList` as a member variable. In your
commands' `doIt` function, you would declare an `OpUndoItemRecorder` and use
various `OpUndoItem` sub-classes. In The `MPxCommand` `undoIt` and `redoIt`
functions, you would call the `OpUndoItemList` `undo` and `redo` functions.
