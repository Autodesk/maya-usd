---
title: USD Scene and Layer Saving in Maya USD Plugin
numbersections: true
---

# USD Scene and Layer Saving in Maya USD Plugin

This page documents how the USD stage and its layers are saved within a Maya
scene. We also document some of the subtle complexity that code maintainers
should be aware of.

To re-iterate, this is only for USD stages where their layers are saved to the
Maya scene instead of on-disk.


## Necessary Preliminary Knowledge

Here are the topics that affects layer saving:

  * How USD find stages (Stage in the rest of the document)
  * How the Maya USD proxy shape works (Proxy Shape, in the rest of this
    document)
  * How the layer manager works and is saved in Maya USD (Layer Manager
    in the rest of this document)
  * How and when layers and other data get saved within the Maya scene
    save mechanism
  * How and when layers are loaded from the Maya scene into the USD
    stage a proxy shapes


### How USD Find Stages

USD finds stages in USD stage caches, which are represented by the USD class
`UsdStageCache`. The caches are set globally (per-thread) by declaring an
object that holds a stage cache: the USD-provided class `UsdStageCacheContext`.
In Maya USD, we have multiple stage cache to separate stages into categories:

  * Shared stages vs unshared stages
  * Load-all-payload vs unload-all-payload.

The API to retrieve a stage (UsdStage::Open, UsdStage::Find, etc)
receives information describing the stage and if the information
provided matches a stage in the cache, then that stage is returned.
Otherwise a new stage with that information is created, put in the
cache and returned. One particular important point is that cache
matching works with partial information. If a cache entry was created
with information A and B, then trying to retrieve a stage with only
information A will find the A+B entry.

The concrete case for Maya USD is that stages are searched using the root
layer pointer and optionally the session layer pointer. If both are
provided, then both must match. But if only the root layer is provided,
then any stage with that root will be returned, and thus the session
layer that was in that stage will be used by the Proxy Shape. One
particular important point is that it is the ID of the layers that are
compared. If a layer change ID, then it won't match anymore. This is
relevant when saving.


### How MayaUSD Proxy Shapes Work

The Proxy Shape provides access to the USD stage. It does *_not_*
initially keep:

  * The Stage nor a pointer to it
  * The layers nor pointers to them
  * The session layer nor a pointer to it

The stage uses the following data to compute the stage and its output root
and session layers:

  * The ID of the root layer.
  * The ID of the session layer.
  * A flag to know if the stage is shared.
  * A flag to know if the stage is actually provided as an input attribute.
  * The UUID of the layer manager that contains the stage layers.

The main difference between shared and unshared stages is that unshared
stages have their own unique session layer.

To find the Stage, so that it can implement its getStage() function,
the Proxy Shape computes to root layer pointer and session layer
pointer. It does this by checking if it holds a root ID and session ID,
respectively. If it does hold a valid ID, then it asks the Layer Manager
for the corresponding layer. If the ID are not set, then the compute for
these layer returns a null pointer. For the root layer, a null layer
triggers the creation of a new anonymous layer in the Proxy Shape. That
is how a newly-created stage is initialized with a root layer.

The proxy shape caches the resulting layer pointers. This is necessary because
the layer manager node exists only for the brief moment when the Maya scene is
being saved or when it is loading. The layer manager nodes do not exist outside
of the duration of saving or loading. More detail on this in the next section.


### How the Layer Manager Works

The layer manager is both a class with many static functions and a Maya node.
It is where the layers that are saved within a Maya scene instead of on-disk
are kept. It is used by the MayaUSD Proxy Shape to find its layers during
loading and to store its layers during saving to the Mayascene. The Layer
Manager UI also store some data in it.

It is important to note that the Layer Manager's Maya node is only briefly kept
alive. When a Maya scene is loaded, the Maya node contains the list of known
layer IDs and possibly string attributes with the content of these layers,
if they were not saved to external files. For each layer saved in the node,
four attributes are kept:

  * The layer ID
  * The type of file: usd, usda or sdf
  * The content of the layer, in text form
  * If the layer was anonymous

For example, all session layers are always saved within these text attributes.
These ID and content are used to re-open or re-create all these known layers.
The ID of the layers are set to the ID that were kept in the Layer Manager
node. Once this layer loading is done, the Maya node is destroyed. It will
only be recreated when the scene is about to be saved.

This processing to create layers from text attributes is done the first time
a layer is requested. It is done when LayerManager::findLayer() is called,
which is important in the saving process.

The layer is found on first access because the Proxy Shape uses the Layer
Manager to find its layers and the point at which the Proxy Shape compute()
function is called is unknown, so the Layer Manager needs to be ready to
provide them at any time. In particular, it could not assume that a post-
scene-load callback would be triggered early enough. For example, another
post-load-scene callback could access the Proxy Shape and would thus need
the Layer Manager to be ready to provide layers.

The reason we need this Layer Manager is that there is no such thing as a
layer cache in USD, unlike for USD stages. Sdf layers only exist if they are
referenced. The Layer Manager meets this requirement by holding a reference
to each layer. This preserves the layer lifetime between the time the Layer
Manager node is deserialized and the time the layers are requested by the
Proxy Shapes.

### How Saving Works

The USD Stage is a pure run-time object. The objects that are saved are
only the layers. Stages get re-created based on layers, as explained above.

Layers are saved just before the Maya scene is saved, in a Maya pre-save
callback. All layers that need to be saved to external files are saved,
then all layers that need to be saved inside the Maya file, including session
layers, are serialized into string attributes in the Layer Manager Maya node.

This layer-saving process is customizable. The USD plugin can provide
a callback to do some of the work of saving layers. If such a callback is
provided (and the Maya USD plugin does provide one), then it gets called
with a list of objects describing each stage that need to be saved. For each
stage, the following information is provided:

  * The DAG node paths pointing to the Proxy Shape
  * A pointer to the USD stage
  * A flag indicating if the stage is shared
  * A flag indicating if the stage was received by an input Maya plug
  
The callback can execute arbitrary code. It must return a flag indicating if
the saving of layers should be aborted, if the entire saving process has been
handled by the callback or if only a partial save occured. A partial save has
occured if not all dirty layers were saved by the callback. That is the case
in the Maya USD plugin; the provided callback does only a partial handling.
To handle such partial saves, the Layer Manager re-checks the list of layers
to find which ones are still dirty and still need to be saved. It then saves
these layers.

An important addition is that the load-state of USD payload are pure stage-
level information. That means they do not get saved in the layer data. In
Maya USD, we do save load state of all payload for each stage as an attribute
on the Proxy Shape. This is also done in another Maya pre-saved callback.

When a Proxy Shape is saved within the Layer Manager, information critical to
be able to reload the proxy shape are added to it. The unique ID (UUID) of the
Layer Manager is set in its "layerManager" attribute.

### How Loading Works

The Proxy Shape and Layer Manager node are normal Maya nodes. Maya reads them
from disk and recreates the nodes in memory. This poses a problem: the Proxy
Shape needs the Layer Manager to get its layers, but we do not know in which
order each node will be created.

One problem is that the proxy shape really does not know that a load occured.
The nodes are created when loading, but they are similarly created when a new
proxy shape is added to a Maya scene. Furthermore, loading works by creating
node and then setting its attributes incrementally. Again, the same things
happen when a new stage is created in Maya and the user modifies the stage.

In all cases, at some undefined point, the `compute` method of the node gets
called. For the Proxy Shape, that is when the USD stage gets created. The stage
needs to be provided with its root layer and session layer. When loading a stage
that was saved to the Maya scene, these layers need to be restored from the data
that was put in the Layer Manager.

So, in the Proxy Manager `compute` function, the information about the layer
IDS and the Layer Manager UUID is used to find a Layer Manager that contains
the desired layers. The exact logic is more complicated than that because
layers can come from different sources and thus be derived from different data.
For example, maybe the user provided an explicit on-disk USD file name. In the
case where the stage was saved to the Maya scene, as explained in the section
about saving, information about how to find layers in the Layer Manager is used.

The problem is that maybe when `compute` is called, the Layer Manager has not
been loaded yet. This is where the `recomputeLayers` attribute of the Proxy
Shape comes into play. At the end of the load, a callback increases the value
of the `recomputeLayers` attribute of each Proxy Shape. This ensures they will
be able to read any required layer from the Layer Manager and contain correct
data.

In order to avoid loading data from the Layer Manager multiple times, which
Proxy Shapes have already read their layers from the Layer Manager is kept.
We also protect against trying to read-back layers from the Layer Manager
while saving.


## Save Architecture Challenges

The main challenges that the save architecture must deal with are that:

  * The only persistent way to refer to a layer is by its ID.
  * Saving a freshly-created layer changes its ID.
  * (So does saving an existing layer under a new filename.)
  * Objects that keep a reference to a layer by through its ID
    must update their data when the ID changes.

At this time, Maya USD doesn't have a mechanism to notify of layer ID change.
The use of SdfNotice::LayerIdentifierDidChange should be investigated.


## FAQ

### Where are stage caches created in Maya USD?

*Answer*: The Maya USD UsdStageCache class is creating and holding the caches.


### How many pre-save callbacks are there? Are they dependent?

*Answer*: There are two pre-save callbacks, with no dependence between
them. They are:

  * The Layer Manager callback, that does the USD layers saving.
  * The Load State callback, which saves the USD payload state
    (loaded/unlaoded) in a proxy shape attribute.

If more clients require a pre-save callback, or if a dependence between
callbacks becomes required, the architecture should be re-worked to use
a single callback so that we can control the ordering of pre-save clients.


### Should caches be flushed when saving?

*Answer*: If saving a layer changes its ID, then the entry in the stage
cache(s) is no longer valid. We currently update the cache so that
already-loaded stages that have a root layer that has changed filename
(either because it went from anonymous to file-based or its file name
changed) have the correct payload load-state. The reason we do this is
that the proxy shape finds the stage by the root layer ID, and if we did
not pre-fill the stage cache with stages using the new root layer with
the correct load state, when the proxy shape is recomputed the load
state could change which is incorrect.


### Why do we pass only the root layer to find shared stages?

*Answer*: The goal is that if we create multiple shared stages with the same
root layer, they will share the session layer. For example, if the user creates
a new stage, all that the user specifies is the file-name of the root layer
(and a few flags not relevant for this discussion). In order to find a
previously-existing shared stage, only the root layer must be passed so that
USD will return the existing stage, thus implementing the desired sharing of
the stage and session layer. We could not possibly pass a session layer,
because we don't have it and we want to find a stage with the root layer and
existing session layer, if any are already loaded.


### Why is the Maya layer manager node destroyed and re-created?

*Answer*: The Layer manager uses the existence of its Maya
node as a flag to know that it has not yet executed the post-read
processing of layers. Also, during a Maya session, the Layer Manager
node is not required. The node exists solely to be able to save
information into the Maya scene file, it is not used to create DG graph
and connections.


### Why does the layer manager have a layer database?

The reason is that while USD knows about all loaded layers, it does not
have a layer cache. It only keeps and know about layers that are kept alive
by at least one smart pointer. So SdfLayer::Find() can only find layers that
are already in active use, in a USD Stage.

The post-read processing triggered by the first call to findLayer() does
call SdfLayer::Find(). This is in case a layer is shared by multiple Stages.
It also does special processing for USD layers saved inside the Maya scene
file. These layers are not on-disk and are de-serialized from string attributes
on the Layer Manager. The only prupose of findLayers() is to ensure that the
post-read processing has been executed.

In the future, we are thinking of replacing the findLayers() and the one-time
processing it does by an explicit Maya post-read callback that would compute
all Proxy Shape and get rid of the Layer Manager layer database after all Proxy
Shapes are computed.


### Why does the LayerManager::findLayer() trigger processing?

The default implementation of MayaUsdProxyShapeBase::computeSessionLayer()
calls LayerManager::findLayer().

It's to avoid order dependency during de-serialization and computation between
the Proxy Shape and Layer manager. The Layer Manager needs to hold onto the
layers until all Proxy Shape have been computed.



### Dealing with Root Layer ID Changes

When the root layer of a freshly created Stage gets saved, its ID
changes from no-name to file-name. To be able to find the layer again,
this new ID is set on the Proxy Shape. This allows the Proxy Shape to
find it again, but it also marks the Proxy Shape dirty, which mean it
will be re-computed when accessed again. This ID change is done in the
Layer Manager when it saves the layer. The Layer manager updates its
internal layer database, so its findLayer() will be able to find the
renamed root layer, in particular when the Proxy Shape asks for it.
When root layer ID changes, the Stage caches are updated.


### Dealing with Session Layer ID Changes

The Layer Manager saves the session ID in a string attribute on the Proxy
Shape. This is important because when a freshly-created stage is first saved,
the root layer changes ID. When a layer changes ID, it no longer matches any
stage in the stage cache. Thus, the next time a Proxy Shape asks the stage cache
for the stage, a new stage is created. To ensure that this new stage uses the
existing session layer, the session ID is kept in the Proxy Shape, and is
explicitly used to lookup and create the new stage.

### Protecting the Layer Manager Node

The load rules are saved as an attribute on the Proxy Shape in order to be able
to apply them again when we reload the Maya scene. The load rules are saved in
a pre-save callback, which modifies the Proxy Shape since the load rules are
set as an attribute on it. Because it accesses the Proxy shape to retrieve the
stage that holds the load rules, the Proxy Shape can potentially end-up calling
LayerManager::findLayer(). Normally, findLayers() is where the layer database
is initialized and normally it deletes the Layer Manager Maya node.

To protect the Layer Manager node from deletion during saving, an internal flag
is set at the start of the saving process and reset at the end. While the flag
is set, the Layer Manager node is never deleted. The reasoning being that while
in the middle of a save, calling findLayer() should not cause the node to be
deleted, since it must exist to be saved into the Maya scene.
