# Proxy Shape
The Proxy Shape is the nexus of the scenes we create with AL_USDMaya - it displays any native USD Data through Hydra, and any maya objects which have a live USD connection are connected via outStageData attribute.
You can have as many ProxyShape as you like. There is a 1:1 relationship between a ProxyShape and a USD Stage

**History** The node was originally modified from the Pixar USDMaya "ProxyShape", they now share very little code.

**Architecture** The Proxy Shape communicates with Hydra via the USDImaging layer i.e 
```
ProxyShape->USDImaging->Hydra
```

## Interaction between Maya and Hydra 
Hydra manages it's own shading and rendering. There are USD specific ways to hook into this, some of this API is still WIP (see [USD docs](https://graphics.pixar.com/usd/docs/api/usd_shade_page_front.html))

The only addition to default Hydra rendering AL_USDMaya currently adds is support for lights - you can create Maya Point lights and they will be used by Hydra, replacing the default Hydra light.


## The Node
see [code docs](https://animallogic.github.io/AL_USDMaya/classAL_1_1usdmaya_1_1nodes_1_1ProxyShape.html) for documentation on the attributes of the node.


## Importing with AL_usdmaya_ProxyShapeImport
+ loads the USD stage into memory
+ Creates an AL_usdmaya_ProxyShape node in Maya which points to the stage
+ Traverse the USD Stage, and for each prim, do one of:
1. Ignore it (excludeFromProxyShape Metadata)
2. Translate it into Maya data (importAsNative Metadata) 
3. If the Prim is of a type for which there is a SchemaTranslator registered, run the translator. See [here](schemaTranslatorPlugins.md) 
4. If none of the above... display it in the Maya Viewport, using Pixar's [Hydra](https://graphics.pixar.com/usd/docs/USD-Glossary.html#USDGlossary-HydraRenderer) to render it as OpenGL. (This data is not (initially at least) bridged into maya).


**Some additional facts:**
+ When additional things are imported, the proxyShape is the root node in maya under which other things will be parented.
+ You can create as many proxy shapes as you like in the maya scene.
+ A proxyShape contains a handle to a live USD Stage. This stage, and the handle - are recreated on scene open/load 

**Prim Metadata**
There is some metadata defined in (@todo add doxygen link)Metadata.h which if found on a prim, causes a specific behaviour for that prim:
+ al_usdmaya_importAsNative: import into maya - dont display in Hydra
+ al_usdmaya_excludeFromProxyShape - ignore completely

**Selecting Prims in the Proxy Shape**

We can select any USD prim being displayed in the proxy shape, and it's transform hierarchy (the entire chain of transforms up to the root) will be immediately mirrored into maya. 
These transforms will be removed once the USD object is unselected. 
These can be transformed using maya's transform tools and maintain a live connection to their master usd objects, so you will see the selected portion of the USD hierarchy move (via the proxy shape)

We can choose which layer to affect when we move these objects, so any modifications we make will be recorded as overrides in the relevant layer, which we can then export

## Commands

The Proxy Shape has a number of commands associated with it. (This documentation below is culled from the maya help strings of the commands)

### AL_usdmaya_ProxyShapeImport Overview

This command allows you to import a USD file as a proxy shape node. In the simplest case, you can do this:
```
AL_usdmaya_ProxyShapeImport -file "/scratch/dev/myaweomescene.usda" -name "MyAwesomeScene";
```
which will load the usda file specified, and create an ProxyShape of the specified name.
If you wish to instance that scene into maya a bunch of times, you can do this:
```
AL_usdmaya_ProxyShapeImport -file "/scratch/dev/myaweomescene.usda" -name "MyAwesomeScene" "transform1" "transform2";
```
This will load the file, create the proxy shape, and then add them as instances underneath transform1 and transform2.

Some other flags and stuff:

To load only a subset of the USD file, you can specify a root prim path with the -pp/-primPath flag:
```
-primPath "/myScene/foo/bar"
```
This will ignore everything in the USD file apart from the UsdPrim's underneath /myScene/foo/bar.

By default the imported proxy node will be connected to the time1.outTime attribute.
The -ctt/-connectToTime flag controls this behaviour, so adding this flag will mean the usd proxy is not driven by time at all:
```
-connectToTime false
```
If you wish to prevent certain prims from being displayed in the proxy, you can specify the -excludePrimPath/-epp flag, e.g.
```
-excludePrimPath "/do/not/show/this/prim"
```
The command will return a string array containing the names of all instances of the created node. 
(There will be more than one instance if more than one transform was selected or passed into the command).
By default, this will be the shortest-unique names; if -fp/-fullpaths is given, then they will be full path names.

This command is undoable.


### AL_usdmaya_ProxyShapeFindLoadable Overview

This command doesn't do what I thought it would, so therefore I have no idea whey it's here @todo: remove?
I had assumed this would produce a list of all asset references that can be loaded, however it seems to do nothing.

If you have an ProxyShape node selected, then:

+ AL_usdmaya_ProxyShapeFindLoadable              //< produce a list of all assets? payloads?
+ AL_usdmaya_ProxyShapeFindLoadable -unloaded    //< produce a list of all unloaded assets? unloaded payloads?
+ AL_usdmaya_ProxyShapeFindLoadable -loaded      //< produce a list of all loaded assets? loaded payloads?

You can also specify a prim path root, which in theory should end up restricting the returned results to just those under the specified path.

```
AL_usdmaya_ProxyShapeFindLoadable -pp "/only/assets/under/here";
AL_usdmaya_ProxyShapeFindLoadable -pp -loaded "/only/assets/under/here";
AL_usdmaya_ProxyShapeFindLoadable -pp -unloaded "/only/assets/under/here";
```

I think the code to this command is correct, however I have no idea what it's supposed to do. 
One day it might return a result, so I'll leave it here for now.

### AL_usdmaya_ProxyShapeImportAllTransforms Overview

Assuming you have selected an ProxyShape node, this command will traverse the prim hierarchy, and for each prim found, an Transform node will be created. 
The -pushToPrim/-p2p option controls whether the generated Transforms have their pushToPrim attribute set to true. 
If it's enabled, then the generated transforms will drive the USD prims. 
If however it is disabled, then the transform nodes will only be observing the UsdPrims
```
AL_usdmaya_ProxyShapeImportAllTransforms "ProxyShape1" -p2p true;  // drive the USD prims
AL_usdmaya_ProxyShapeImportAllTransforms "ProxyShape1" -p2p false ; // observe the USD prims
```
This command is undoable.

### AL_usdmaya_ProxyShapeRemoveAllTransforms Overview:

If you have previously generated a tonne of Transforms to drive the prims in a usd proxy shape, via a call to 'ProxyShapeImportAllTransforms', then this command will go and delete all of those transform nodes again.
```
AL_usdmaya_ProxyShapeRemoveAllTransforms "ProxyShape1";  // drive the USD prims
```
This command is undoable.


### AL_usdmaya_ProxyShapeImportPrimPathAsMaya Overview:

This command is a little bit interesting, and probably bug ridden. The following command:
```
AL_usdmaya_ProxyShapeImportPrimPathAsMaya "ProxyShape1" -pp "/some/prim/path";
```
..Will disable the rendering of the prim path "/some/prim/path" on the "ProxyShape1" node,and will run an import process to bring in all of the transforms/geometry/etc found under "/some/prim/path", as native maya transform and mesh nodes.
Adding in the -ap/-asProxy flag will build a transform hierarchy of Transform nodes to the specified prim, and then create a new ProxyShape to represent all of that geometry underneath that prim path.
e.g
```
AL_usdmaya_ProxyShapeImportPrimPathAsMaya "ProxyShape1" -ap -pp "/some/prim/path";
```
I'm not sure why anyone would want that, but you've got it, so there.


### AL_usdmaya_ProxyShapePrintRefCountState Overview:

Command used for debugging the internal transform reference counts.


### AL_usdmaya_ProxyShapeResync Overview
Used to inform AL_USDMaya that at the provided prim path and it's descendants, that the Maya scene at that point may be affected by some upcoming changes. 
    
After calling this command, clients are expected to make modifications to the stage and as a side effect will trigger a USDNotice call in AL_USDMaya which will update corresponding Maya nodes that live at or under the specified primpath; any other maintenance such as updating of internal caches will also be done. 

The provided prim path and it's descendants of  known schema type will have the AL::usdmaya::fileio::translators::TranslatorAbstract::preTearDown method called on each schema's translator
It's then up to the user to perform updates to the USD scene at or below that point in the hierarchy
On calling stage.Reload(),the relevant USDNotice will be triggered and and apply any changes and updates to the Maya scene.
```
AL_usdmaya_ProxyShapeResync -p "ProxyShape1" -pp "/some/prim/path"
```
## Selection
Selection-related commands are described [here](selection.md)

