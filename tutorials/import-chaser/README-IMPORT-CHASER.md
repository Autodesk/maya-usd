# How to Write a USD Import Chaser

This tutorial will guide you in writing a USD import chaser that modifies the
imported data at the end of the import process. The example import chaser
described in this tutorial can be found in the Python file
[Import Chaser in Python](import_chaser.py).

## What is an Import Chaser

An import chaser is a class that can modify the crested Maya nodes imported from
USD into Maya. It runs after the import is complete and all USD prims have been
converted to Maya nodes.

## How to Invoke an Import Chaser

Each import chaser must have a unique name. The `mayaUSDImport` command has a
`-chaser` flag taking an array of import chaser names. Those named chasers will
be called at the end of the import.

## Anatomy of an Import Chaser

An import chaser is a class that derives from the `mayaUsd.lib.ImportChaser` class.
That class provides three functions:

- `Register`: a static function to register your import chaser class.
- `Unregister`: a static function to unregister your import chaser class.
- `GetSdfToDagMap`: a function to get the map of USD prim SDF paths to their
                    corresponding Maya node `MDagPath`.

One thing to note is that the `GetSdfToDagMap` only returns prims for which there
is a Maya DAG node. If a prim ws imported into a non-DAG node, it won't appear in
this map. For example, materials are not DAG nodes and thus imported materials
won't appear in this map.

An import chaser must implement four functions:

- Initialization: the normal `__init__` Python function to create an instance of a class.
- `PostImport`: called after the import is done. *This* is the most important function!
- `Undo`: called when the import is undone by the user.
- `Redo`: called when the import is redone by the user.

The chaser instance is created at the end of the import. That means that when its
`__init__` function is called, the import is already done. Still, the expectation
is that the chaser work be done in its `PostImport` function. The `__init__`
function receives the `FactoryContext`. This context contains four functions:

- `GetStage`: returns the USD stage from whichthe import was done.
- `GetImportedDagPaths`: the list of DAG paths created during the import.
- `GetImportedPrims`: the list of imported USD prim paths.
- `GetJobArgs`: the import jobs arguments (`UsdMayaJobImportArgs`).

Note that all that information is passed to the `PostImport` function. It receives
the following five arguments:

- `returnPredicate`: the USD prim filtering predicate used on the USD stage.
- `stage`: the USD stage from whichthe import was done.
- `dagPaths`: the list of DAG paths created during the import.
- `sdfPaths`: the list of imported USD prim paths.
- `jobArgs`: the import jobs arguments (`UsdMayaJobImportArgs`).

The `PostImport` function receives the lists of DAG paths and SDF paths of imported
DAG objects. If you only need one of those list, using the received argument is simple.
If you need to know the correspondence between the prim paths and DAG paths, you can use
the SDF-to-DAG map returned by the `GetSdfToDagMap` function (provided by the base class).

## Example Import Chaser

The [example import chaser](import_chaser.py) demonstrate the basic of an import chaser:

- How to iterate through the import DAG nodes.
- How to modify the nodes using undoable operations.
- How to undo and redo those operations.

The processing loops over the DAG node using the received `dagPaths`. Since those are
`MDagPath`, we use their `fullPathName` function to extract the Maya path as text to
pass to other functions.

We use the `cmds.listRelatives` and `cmds.listConnections` to find the related materials
and shader engines. The a `OpenMaya.MDGModifier` is used to change the names of those
materials. the modifier was created in the `__init__` function. Its `doIt` function is
called at the end of processing to apply all renaming at once.

Using a `OpenMaya.MDGModifier` makes undo and redo easy. We can call its `undoIt`
function in `Undo` and its `doIt` function in `Redo`.

We provide functions to register and unregister the chaser. They call the functions
`mayaUsd.lib.ImportChaser.Register` and `mayaUsd.lib.ImportChaser.Unregister`, passing
the example chaser class (`MaterialRenamerChaser`) and its unique name.

The rest of the example code is just there for the example. It shows how to call
the USD import with the example chaser.

**WARNING**: the example code registers and unregisters the chaser around the import,
             but that is *not* the usual way to do things and should *not* be done.

**NOTE**: normally, you register your chaser once at Maya startup and leave
          it always registered.
