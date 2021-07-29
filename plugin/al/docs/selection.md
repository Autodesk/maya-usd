# Selection

In general, We want the same kind of selection behaviour in the USD domain that we have in Maya, achieved through:
1. Selection by USD Prim Path using AL_usdmaya_ProxyShapeSelect
2. Mouse-based Viewport selection (which calls AL_usdmaya_ProxyShapeSelect command in the background)
3. Geometry highlighting using AL_usdmaya_InternalProxyShapeSelect

The commands support Undo/Redo.


@todo: document new Selection API introduced in commit 3a6d7a664a72cb57abbcf6f1f4fa781d4167e9c9, how to use it, how to switch etc.. 

## Selectability
Selectability can be configured on a per-prim basis by setting the selectability metadata via AL.usd.schemas.maya.ModelAPI schema. By default everything is selectable, and only the prims tagged as 'unselectable' and their children won't be selectable.

```
from pxr import Usd
from AL.usd.schemas import maya as maya_schemas

stage = Usd.Stage.CreateInMemory()
prim = stage.DefinePrim("/DontSelectMe")

alUsdModelShot = maya_schemas.ModelAPI(prim)
alUsdModelShot.SetSelectability(maya_schemas.Tokens.selectability_unselectable)
```

## Commands
### AL_usdmaya_InternalProxyShapeSelect Overview

This command is a simpler version of the AL_usdmaya_ProxyShapeSelect command. 
Unlike that command, AL_usdmaya_InternalProxyShapeSelect only highlights the geometry in UsdImaging (it does not generate the modifyable transform nodes in the scene).
```
AL_usdmaya_InternalProxyShapeSelect -r -pp "/root/hips/thigh_left" "AL_usdmaya_ProxyShape1";
```

To select more than one path, re-use the -pp flag, e.g.
```
AL_usdmaya_InternalProxyShapeSelect -r -pp "/root/hips/thigh_left" -pp "/root/hips/thigh_right" "AL_usdmaya_ProxyShape1";
```

The -pp flag specifies a prim path to select, and it can be re-used as many times as needed. When selecting prims on a proxy shape, you can specify a series of modifiers that change the behaviour of the AL_usdmaya_ProxyShapeSelect command. These modifiers roughly map to the flags in the standard maya 'select' command:

+ -a   / -append      : Add to the current selection list
+ -r   / -replace     : Replace the current selection list
+ -d   / -deselect    : Remove the prims from the current selection list
+ -tgl / -toggle      : If the prim is selected, deselect. If the prim is unselected, select.


If you wish to deselect all prims on a proxy shape node, use the -cl/-clear flag, e.g.

```
AL_usdmaya_InternalProxyShapeSelect -cl "AL_usdmaya_ProxyShape1";
```


### AL_usdmaya_ProxyShapeSelect Overview

This command is designed to mimic the maya select command, but instead of acting on maya node names or dag paths, it acts upon SdfPaths within a USD stage. 
So in the very simplest case, to select a USD prim with path "/root/hips/thigh_left" contained within the proxy shape "AL_usdmaya_ProxyShape1", you would execute the command in the following way:
```
AL_usdmaya_ProxyShapeSelect -r -pp "/root/hips/thigh_left" "AL_usdmaya_ProxyShape1";
```
To select more than one path, re-use the -pp flag, e.g.
```
AL_usdmaya_ProxyShapeSelect -r -pp "/root/hips/thigh_left" -pp "/root/hips/thigh_right" "AL_usdmaya_ProxyShape1";
```

The -pp flag specifies a prim path to select, and it can be re-used as many times as needed.
When selecting prims on a proxy shape, you can specify a series of modifiers that change the behaviour of the AL_usdmaya_ProxyShapeSelect command. 
These modifiers roughly map to the flags in the standard maya 'select' command:
```
+ -a   / -append      : Add to the current selection list
+ -r   / -replace     : Replace the current selection list
+ -d   / -deselect    : Remove the prims from the current selection list
+ -tgl / -toggle      : If the prim is selected, deselect. If the prim is unselected, select.
```

If you wish to deselect all prims on a proxy shape node, use the -cl/-clear flag, e.g.
```
AL_usdmaya_ProxyShapeSelect -cl "AL_usdmaya_ProxyShape1";
```

There is one final flag: -i/-internal. Please do not use (It will probably cause a crash!)
[The -i/-internal flag prevents changes to Maya's global selection list. This is occasionally needed internally within the USD Maya plugin, when the proxy shape is listening to state changes caused by the MEL command select, or via the API call MGlobal::setActiveSelectionList. 
The behaviour of this flag is driven by internal requirements, so no guarantee will be given about its behaviour in future]
