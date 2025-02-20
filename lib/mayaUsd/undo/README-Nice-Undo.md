# Nice Undo Labels

While having undo/redo support at all is necessary, it is less useful if the user
does not understand what are the operations that can be undone and redone. In
order to help the user, it is necessary to have each operation kept in Maya's
undo stack and shown in Maya UI have readable labels that convey what would be
undone or redone.

This document provides recipes and guidance on how to get nice labels in the
Maya UI for each individual undo item. This is necessary because it is not always
obvious how to achieve it and many easy way to implement an undoable command or
operation would result in bad labels. Getting nice labels often requires extra
efforts.

## Maya Built-in Label Generator

For its UI `Edit` menu, which contains the name of the operation that can be
undone and redone as a menu item label, Maya always filter the raw undo label.
It first split the raw label at the first space, then it limits its length to
at most 25 characters. These facts affect how you should design your undo label.

The first thing is to ensure that the first 25 characters will be sufficient to
identify what would be undone or redone. This means wording your undo label with
the most important information at the beginning.

The second thing is actually a trick to make nicer labels. The label will be
split at the first space character, but you can use non-breakable spaces to
still have spaces in your label. The unbreakable space character is UTF hex
code A0. That is the `"\xA0"` string in C++ or Python.

## Scripting Callbacks

When implementing actions triggered by UI in scripting, it is often done by
registering a callback. For example callbacks for menu items or for clickable
buttons.

For MEL callbacks, we don't have a nice solution. We recommend to instead use
Python. When implemented in Python, the Maya undo system uses the callback
function Python module name and function name to build the UI label. These
are often forced unto you, and may not be clear to the user, so it is better
to force a nice label. This is easy to achieve since Python allows editing the
metadata of a function. The module name is kept in the function's `__module__`
property and its name in the `__name__` property.

What we suggest is that you either write a function to modify other functions
or use a Python function decorator to modify these function properties. For
example, the menu items to add and remove USD schemas use a function to
generate its callback with nice undo label by editing the properties. We
needed a function to generate a function because the nice UI label can only
be known and generated at run-time from the name of the USD schema. See the
`_createApplySchemaCommand` function in `plugin\adsk\scripts\mayaUsdMenu.py`.

Alternatively, you could use a function decorator. For example, this decorator
sets a nice label on the given function:

```Python
def setUndoLabel(label):
    '''
    This function decorator sets the function metadata so that it has
    a nice label in the Maya undo system and UI.
    '''
    def wrap(func):
        nonBreakSpace = '\xa0'
        func.__module__ = label.replace(' ', nonBreakSpace)
        func.__name__ = ''
        return func
    return wrap

# Example of using the decorator.
@setUndoLabel("nice label")
def example(c):
    print(c)
```

## Creating Maya Commands

For `MPxCommand`, Maya uses the command name as the UI label. So you need
to ensure your command name is sufficient to clearly describe what would be
undone. Unfortunately, this goes somewhat against a desirable trait of making
a command be flexible. Indeed, if a command can do multiple things, then the name
of the command would not be enough to really know what would be undone. In this
case, it might be beneficial to create a base command class that contains the
whole funcitonality but is *not* registered with Maya as a command. Instead,
multiple sub-classes that only declare different command names are registered
with Maya for each specific actions.

An example of this was done for the USD collection editing. A single base class
deriving from `MPxCommand` contains the whole implementation of the command but
is not registered with Maya and multiple sub-classes are registered. See the
file `lib\mayaUsd\resources\ae\usdschemabase\collectionMayaHost.py`.

In short, this is what is done. First create the base command with all the
implementation code: `doIt`, `undoIt`, etc

```Python
class _BaseCommand(MPxCommand):
    def __init__(self):
        super().__init__()

    # MPxCommand command implementation.

    @classmethod
    def creator(cls):
        # Create the right-sub-class instance.
        return cls()
    
    @classmethod
    def createSyntax(cls):
        syntax = MSyntax()
        # Add your syntax arguments and flags
        return syntax

    def doIt(self, args):
        # implement your whole command
        pass

    def undoIt(self):
        # implement your whole command
        pass

    def redoIt(self):
        # implement your whole command
        pass
```

Then create the concrete sub-commands:

```Python
class FirstConcreteCommand(_BaseCommand):
    commandName = 'NiceComprehensibleName'
    def __init__(self):
        super().__init__()


class SecondConcreteCommand(_BaseCommand):
    commandName = 'AnotherNiceName'
    def __init__(self):
        super().__init__()

_allCommandClasses = [
    FirstConcreteCommand,
    SecondConcreteCommand,
]

def registerCommands(pluginName):
    plugin = MFnPlugin.findPlugin(pluginName)
    if not plugin:
        MGlobal.displayWarning('Cannot register commands')
        return
    
    plugin = MFnPlugin(plugin)
    
    for cls in _allCommandClasses:
        try:
            plugin.registerCommand(cls.commandName, cls.creator, cls.createSyntax) 
        except Exception as ex:
            print(ex)


def deregisterCommands(pluginName):
    plugin = MFnPlugin.findPlugin(pluginName)
    if not plugin:
        MGlobal.displayWarning('Cannot deregister commands')
        return
    
    plugin = MFnPlugin(plugin)
    
    for cls in _allCommandClasses:
        try:
            plugin.deregisterCommand(cls.commandName) 
        except Exception as ex:
            print(ex)
```

## Invoking Maya Commands

Giving commands a nice name is important, but unfortunately, it is not always
sufficient. In particular, Maya does not create proper UI label for commands
invoked from Python!

So, unfortunately, to get a nice UI label from Python, you must invoke the command
from MEL instead. This is simple, but is important to remember. For example, to
invoke the `NiceComprehensibleName` command we declared above, we must *not* call
`cmds.NiceComprehensibleName` but instead `mel.eval("NiceComprehensibleName")`
