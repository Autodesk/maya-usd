# Edit Routing

## What is edit routing

By default, when a Maya user modifies USD data, the modifications are written
to the current edit target. That is, to the current targeted layer. The current
layer is selected in the Layer Editor window in Maya.

Edit routing is a mechanism to select which USD layer will receive edits.
When a routable edit is about to happen, the Maya USD plugin can temporarily
change the targeted layer so that the modifications are written to a specific
layer, instead of the current target layer.

The mechanism allows users to write code (scripts or plugins) to handle the
routing. The script receives information about which USD prim is about to be
change and the name of the operation that is about to modify that prim. In
the case of attribute changes, the name of the attribute is also provided.

Given these informations, the script can choose a specific layer among the
available layers. It returns the layer it wants to be targeted. If the script
does not wish to select a specific layer, it just returns nothing and the
current targeted layer will be used.

## What can be routed

Currently, edit routing is divided in two categories: commands and attributes.

For commands, when the edit routing is called it receives an operation name that
corresponds to the specific command to be routed. Only a subset of commands can
be routed, but this subset is expected to grow. The operations that can be
routed are named:

- duplicate
- parent (including parenting and grouping)
- visibility
- mayaReferencePush

For attributes, any modifications to the attribute value, including metadata,
can be routed. The edit routing is called with an 'attribute' operation name
and receives the name of the attribute that is about to be modified.

## API for edit routing

An edit router can be written in C++ or Python. The principle is the same in
both cases, so we will describe the Python version. The C++ version is similar.

The edit router is a function that receives two arguments. The arguments are
both dictionaries (dict in Python, VtDictionary in C++). Each is filled with
data indexed by USD tokens (TfToken):

- Context: the input context of the routing.
- Routing: the output data of the routing.

In theory, each edit routing operation could fill the context differently
and expect different data in the output dictionary. In practice many operations
share the same inputs and outputs. Currently, the operations can be divided in
three categories:

- Simple commands
- Attributes
- Maya references

The following sections describe the input and output of each category. Each
input or output is listed with its token name and the data that it contains.

### Simple commands

Inputs:
- prim: the USD prim (UsdPrim) that is being affected.
- operation: the operation name (TfToken). Either visibility, duplicate or parent.

Outputs:
- layer: the desired layer ID (text string) or layer handle (SdfLayerHandle).

On return, if the layer entry is empty, no routing is done and the current edit
target is used. Here is an example of a simple edit router:

```Python
def routeToSessionLayer(context, routingData):
    '''
    Edit router implementation for that routes to the session layer
    of the stage that contains the prim.
    '''
    prim = context.get('prim')
    if prim is None:
        print('Prim not in context')
        return

    routingData['layer'] = prim.GetStage().GetSessionLayer().identifier

```

### Attributes

Inputs:
- prim: the USD prim (UsdPrim) that is being affected.
- operation: the operation name (TfToken). Always attribute.
- attribute: the attribute name, including its namespace, if any (TfToken).

Outputs:
- layer: the desired layer ID (text string) or layer handle (SdfLayerHandle).

On return, if the layer entry is empty, no routing is done and the current edit
target is used. Here is an example of an attribute edit router:

```Python
def routeAttrToSessionLayer(context, routingData):
    '''
    Edit router implementation for 'attribute' operations that routes
    to the session layer of the stage that contains the prim.
    '''
    prim = context.get('prim')
    if prim is None:
        print('Prim not in context')
        return

    attrName = context.get('attribute')
    if attrName != "visibility":
        return

    routingData['layer'] = prim.GetStage().GetSessionLayer().identifier
```

### Maya references

The maya reference edit routing is more complex than the other ones. It is
described in the following documentation: [Maya Reference Edit Router](../lib/usd/translators/mayaReferenceEditRouter.md).

## Managing edit routers

The Maya USD plugin provides C++ and Python functions to register edit routers
and to remove them. The function to register an edit router is called
`registerEditRouter` and takes as arguments the name of the operation to be
routed, as a USD token (TfToken) and the function that will do the routing.
For example, the following Python script routes the `visibility` operation
using a function called `routeToSessionLayer`:

```Python
import mayaUsd.lib
mayaUsd.lib.registerEditRouter('visibility', routeToSessionLayer)
```
Routing attributes that are affected by certain commands is still necessary, as 
routing a command might not route changes on the attribute itself.
When Routing Attributes, using the `routeAttrToSessionLayer` example above,
the function used to register would be similar - only using the `attribute` argument:

```Python
import mayaUsd.lib
mayaUsd.lib.registerEditRouter('attribute', routeAttrToSessionLayer)
```

The function to turn off an edit router for a given operation is called
`restoreDefaultEditRouter`. It restores the default edit router for that
operation. For example, to turn off the edit router for `visibility`:

```Python
import mayaUsd.lib
mayaUsd.lib.restoreDefaultEditRouter('visibility')
```

There is also a function to turn off all edit routing for all operations,
called `restoreAllDefaultEditRouters`. It takes no argument. For example:

```Python
import mayaUsd.lib
mayaUsd.lib.restoreAllDefaultEditRouters()
```

## Canceling commands

It is possible to prevent a command from executing instead of simply routing to
a layer. This is done by raising an exception in the edit router. The command
handles the exception and does not execute. This is how an end-user (or studio)
can block certain types of edits. For example, to prevent artists from modifying
things they should not touch. For instance, a lighting specialist might not be
allowed to move props in a scene.

For example, to prevent all opertions for which it is registered, one could use
the following Python edit router:

```Python
def preventCommandEditRouter(context, routingData):
    '''
    Edit router that prevents an operation from happening.
    '''
    opName = context.get('operation') or 'unknown operation'
    raise Exception('Sorry, %s is not permitted' % opName)
```

## Routing custom composite commands

When writing composite commands, that is a command made-up of sub-commands,
it is possible to route the whole command. This will route all sub-commands
to the same destination layer and ignore individual routing of each individual
sub-command.

To do this, one must choose a new and unique operation name for the composite
command. We suggest that you use some unique prefix for your operation names
to avoid conflict with future operation the Maya team might add in the future.

Then one use the `OperationEditRouterContext` object in the composite command
implementation to automatically do the routing. It is important to use the
router countext in all functions so that the sub-commands are routed to the
correct layer. Here is an example of a composite command written in Python:

```Python
import mayaUsd.lib
import ufe

class CustomCompositeCmd(ufe.CompositeUndoableCommand):
    '''
    Custom composite command that can be routed.
    '''

    customOpName = 'my studio name: custom operation name'

    def __init__(self, prim, sceneItem):
        super().__init__()
        self._prim = prim
        ctx = mayaUsd.lib.OperationEditRouterContext(self.customOpName, self._prim)
        o3d = ufe.Object3d.object3d(sceneItem)
        self.append(o3d.setVisibleCmd(False))

    def execute(self):
        ctx = mayaUsd.lib.OperationEditRouterContext(self.customOpName, self._prim)
        super().execute()

    def undo(self):
        ctx = mayaUsd.lib.OperationEditRouterContext(self.customOpName, self._prim)
        super().undo()

    def redo(self):
        ctx = mayaUsd.lib.OperationEditRouterContext(self.customOpName, self._prim)
        super().redo()
```

Afterward, to route the new custom command, one would register an edit router
with the custom operation name, like any other simple command. For example, in
Python, it would look like this:

```Python
import mayaUsd.lib

def routeCmdToRootLayer(context, routingData):
    '''
    Route the command to the root layer.
    '''
    prim = context.get('prim')
    if prim:
        routingData['layer'] = prim.GetStage().GetRootLayer().identifier

mayaUsd.lib.registerEditRouter('my studio name: custom operation name', routeCmdToRootLayer)
```

## Persisting the edit routers

Edit routers must be registered with the MayaUSD plugin each time Maya is
launched. This can be automated via the standard Maya startup scripts.
The user startup script is called `userSetup.mel`. It is located in the
`scripts` folder in the yearly Maya release folder in the user's `Documents`
folder. For example: `Documents\maya\2024\scripts\userSetup.mel`.

This is a MEL script, so any logic necessary to register a given set of edit
routers can be performed. For example, one can detect that the Maya USD plugin
is loaded and register the custom edit routers like this:

```MEL
global proc registerUserEditRouters() {
    if (`pluginInfo -q -loaded mayaUsdPlugin`) {
        python("import userEditRouters; userEditRouters.registerEditRouters()");
    } else {
        print("*** Missing Maya USD plugin!!! ***");
    }
}

scriptJob -permanent -event "NewSceneOpened" "registerUserEditRouters";
```

This requires the Python script that does the registration of edit routers
to exists in the user `site-packages`, located next to the user scripts in
this folder: `Documents\maya\2024\scripts\site-packages`.

For example, to continue the example given above, the following Python script
could be used:

```Python
import mayaUsd.lib

sessionAttributes = set(['visibility', 'radius'])

def routeToSessionLayer(context, routingData):
    '''
    Edit router implementation for that routes to the session layer
    of the stage that contains the prim.
    '''
    prim = context.get('prim')
    if prim is None:
        print('Prim not in context')
        return

    routingData['layer'] = prim.GetStage().GetSessionLayer().identifier

def routeAttrToSessionLayer(context, routingData):
    '''
    Edit router implementation for 'attribute' operations that routes
    to the session layer of the stage that contains the prim.
    '''
    prim = context.get('prim')
    if prim is None:
        print('Prim not in context')
        return

    attrName = context.get('attribute')
    if attrName not in sessionAttributes:
        return

    routingData['layer'] = prim.GetStage().GetSessionLayer().identifier

def registerAttributeEditRouter():
    '''
    Register an edit router for the 'attribute' operation that routes to
    the session layer.
    '''
    mayaUsd.lib.registerEditRouter('attribute', routeAttrToSessionLayer)

def registerVisibilityEditRouter():
    '''
    Register an edit router for the 'visibility' operation that routes to
    the session layer.
    '''
    mayaUsd.lib.registerEditRouter('visibility', routeToSessionLayer)

def registerEditRouters():
    registerAttributeEditRouter()
    registerVisibilityEditRouter()

```
