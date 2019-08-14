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
1. Ignore it _(excludeFromProxyShape Metadata)_
2. Translate it into Maya data _(importAsNative Metadata)_ 
3. If the Prim is of a type for which there is a SchemaTranslator registered, run the translator. See [here](schemaTranslatorPlugins.md) 
4. If none of the above... display it in the Maya Viewport, using Pixar's [Hydra](https://graphics.pixar.com/usd/docs/USD-Glossary.html#USDGlossary-HydraRenderer) to render it as OpenGL. (This data is not (initially at least) bridged into maya).


**Some additional facts:**
+ When additional things are imported, the proxyShape is the root node in maya under which other things will be parented.
+ Stage that is available in StageCache can be loaded by providing it's Id to AL_usdmaya_ProxyShapeImport command.
+ When loading stage from StageCache, other than current EditTarget, all dirty layers composing onto the stage are being tracked internally.
+ You can create as many proxy shapes as you like in the maya scene.
+ A proxyShape contains a handle to a live USD Stage. This stage, and the handle - are recreated on scene open/load
+ You can define Hydra renderer plugin with AL_usdmaya_LayerManager.rendererPlugin. It affects rendering of all proxy shapes in the scene.

**Prim Metadata**
There is some metadata defined in (@todo add doxygen link)Metadata.h which if found on a prim, causes a specific behaviour for that prim:
+ al_usdmaya_importAsNative: import into maya - dont display in Hydra
+ al_usdmaya_excludeFromProxyShape - ignore completely

**Selecting Prims in the Proxy Shape**

We can select any USD prim being displayed in the proxy shape, and it's transform hierarchy _(the entire chain of transforms up to the root)_ will be immediately mirrored into maya. 
These transforms will be removed once the USD object is unselected. 
These can be transformed using maya's transform tools and maintain a live connection to their master usd objects, so you will see the selected portion of the USD hierarchy move _(via the proxy shape)_

We can choose which layer to affect when we move these objects, so any modifications we make will be recorded as overrides in the relevant layer, which we can then export

## Commands

The Proxy Shape has a number of commands associated with it. _(This documentation below is gleaned from the maya help strings of the commands)_

### AL_usdmaya_ProxyShapeImport Overview

This command allows you to import a USD file as a proxy shape node. In the simplest case, you can do this:
```c++
AL_usdmaya_ProxyShapeImport -file "/scratch/dev/myaweomescene.usda" -name "MyAwesomeScene";
```
which will load the usda file specified, and create an ProxyShape of the specified name.
If you want to load a stage that is already available in StageCache, you can point the Proxy to it's Id instead of file path.
```c++
AL_usdmaya_ProxyShapeImport -stageId 9223001 -name "MyAwesomeScene";
```
If you wish to instance that scene into maya a bunch of times, you can do this:
```c++
AL_usdmaya_ProxyShapeImport -file "/scratch/dev/myaweomescene.usda" -name "MyAwesomeScene" "transform1" "transform2";
```
This will load the file, create the proxy shape, and then add them as instances underneath transform1 and transform2.

Some other flags and stuff:

To load only a subset of the USD file, you can specify a root prim path with the -pp/-primPath flag:
```c++
-primPath "/myScene/foo/bar"
```
This will ignore everything in the USD file apart from the UsdPrim's underneath /myScene/foo/bar.

By default the imported proxy node will be connected to the time1.outTime attribute.
The -ctt/-connectToTime flag controls this behaviour, so adding this flag will mean the usd proxy is not driven by time at all:
```c++
-connectToTime false
```
If you wish to prevent certain prims from being displayed in the proxy, you can specify the -excludePrimPath/-epp flag, e.g.
```c++
-excludePrimPath "/do/not/show/this/prim"
```
If you want to exclude some prims from being read when stage is opened, use the -pmi/-populationMaskInclude flag, e.g.
```c++
-populationMaskInclude "/only/show/this/prim1,/only/show/this/prim2"
```
It is possible to import the USD stage with an overloaded session layer _(which can be useful if you wish to import 
the scene with a specific set of variants set)_.  To specify the session layer contents, use the -session flag:
```c++
-session "#usda 1.0"
```
It is also possible to prevent all loadable prims from being loaded when importing the USD stage by specifying the unloaded 
flag _(the default is false)_
```c++
-unloaded true   //< don't load any loadable prims
-unloaded false  //< load all loadable prims    
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

```c++
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
```c++
AL_usdmaya_ProxyShapeImportAllTransforms "ProxyShape1" -p2p true;  // drive the USD prims
AL_usdmaya_ProxyShapeImportAllTransforms "ProxyShape1" -p2p false ; // observe the USD prims
```
This command is undoable.

### AL_usdmaya_ProxyShapeRemoveAllTransforms Overview:

If you have previously generated a tonne of Transforms to drive the prims in a usd proxy shape, via a call to 'ProxyShapeImportAllTransforms', then this command will go and delete all of those transform nodes again.
```c++
AL_usdmaya_ProxyShapeRemoveAllTransforms "ProxyShape1";  // drive the USD prims
```
This command is undoable.


### AL_usdmaya_ProxyShapeImportPrimPathAsMaya Overview:


The following call will import the path "/some/prim/path", and all of it's parent transforms as AL_usdmaya_Transform
nodes into maya.
```c++
AL_usdmaya_ProxyShapeImportPrimPathAsMaya "ProxyShape1" -pp "/some/prim/path";
```
The custom Maya transforms generated will now act as thin wrapper over the transforms within USD. Any modifications
you make within Maya will be directly translated into USD _(and stored within the currect edit target)_.

Adding in the -ap/-asProxy flag will build a transform hierarchy of Transform nodes to the
specified prim, and then create a new ProxyShape to represent all of that geometry underneath
that prim path.
```c++
AL_usdmaya_ProxyShapeImportPrimPathAsMaya "ProxyShape1" -ap -pp "/some/prim/path";
```

### AL_usdmaya_ProxyShapePrintRefCountState Overview:

The AL_usdmaya_ProxyShape node maintains an internal set of reference counts that determine the life span of an
AL_usdmaya_Transform node (i.e. a transform may have been created because it has been selected, or because the
transform is required for a particular plugin translator node). For the average user of AL_USDMaya, these ref
counts are nothing more than an implementation detail that can be ignored. For some developers working on the
core of AL_USDMaya, being able to inspect these ref counts may be of use, which you can do so like this:

```c++
AL_usdmaya_ProxyShapePrintRefCountState -p "ProxyShapeName";
```

### AL_usdmaya_ProxyShapeResync Overview
Used to inform AL_USDMaya that at the provided prim path and it's descendants, that the Maya scene at that point may be affected by some upcoming changes. 
    
After calling this command, clients are expected to make modifications to the stage and as a side effect will trigger a USDNotice call in AL_USDMaya which will update corresponding Maya nodes that live at or under the specified primpath; any other maintenance such as updating of internal caches will also be done. 

The provided prim path and it's descendants of  known schema type will have the AL::usdmaya::fileio::translators::TranslatorAbstract::preTearDown method called on each schema's translator
It's then up to the user to perform updates to the USD scene at or below that point in the hierarchy
On calling stage.Reload(),the relevant USDNotice will be triggered and and apply any changes and updates to the Maya scene.
```c++
AL_usdmaya_ProxyShapeResync -p "ProxyShape1" -pp "/some/prim/path"
```
## Selection
Selection-related commands are described [here](selection.md)

## AL_usdmaya_TranslatePrim

TranslatePrim Overview:

The AL_usdmaya_TranslatePrim command is used to manually execute a translator plugin for a specific prim type. The main use for this functionality 
is to import a prim into Maya for editing, and then push the changes back into the current edit target within USD.

Assuming a plugin translator exists for a given prim type, you can import that prim into Maya by using the *ip* flag with one of more prim paths:


```c++
AL_usdmaya_TranslatePrim -ip "/MyPrim";  //< Run the Prim's translator's import
```
```c++
AL_usdmaya_TranslatePrim -ip "/MyPrim,/YourPrim";  //< Run the Prim's translator's import on multiple Prims
```

To avoid overloading Maya with data that could be retained in a USD proxy shape for performance reasons, some prims _(such as the Mesh)_ are not imported by default.  
In those cases, you will need to use the *-fi* flag to force import of that prim type, e.g. 

```c++
AL_usdmaya_TranslatePrim -fi -ip "/MyMesh";  //< Run the Prim's translator's import
```
Once you have made any modifications to the prim, you can push those changes back into the current edit target by using the *-tp* flag with one of more prim paths that you wish to tear down. e.g. 

```c++
AL_usdmaya_TranslatePrim -tp "/MyPrim";  //< Run the Prim's translator's tearDown
```
```c++
AL_usdmaya_TranslatePrim -tp "/MyPrim,/YourPrim";  //< Run the Prim's translator's tearDown on multiple Prims
```


