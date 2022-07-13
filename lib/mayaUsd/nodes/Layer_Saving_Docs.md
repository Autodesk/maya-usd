---
title: USD Scene and Layer Saving in Maya USD Plugin
numbersections: true
---

# USD Scene and Layer Saving in Maya USD Plugin

This page documents how the USD stage and its layers are saved within a
Maya scene. We also document some of the subtle complexity that can lead
to problems that we discovered along the way.


## Necessary Preliminary Knowledge

To understand some of the problems we encountered, it is necessary to know:

  * How USD find stages (Stage in the rest of the document)
  * How the Maya USD proxy shape works (Proxy Shape, in the rest of this
    document)
  * How the layer manager works and is saved in Maya (Layer Manager
    inthe rest of this document)
  * How and when layers and other data get saved within the Maya scene
    save mechanism


### Stage

USD finds stage in USD stage caches. The caches are set globally
(per-thread) by declaring an object that holds a stage cache. In Maya,
we have multiple stage cache to separate stage into categories:

  * Shared stages vs unshared stages
  * Load-all-payload vs unload-all-payload.

The API to retrieve a stage (UsdStage::Open, UsdStage::Find, etc)
receives information describing the stage and if the information
provided matches a stage in the cache, then that stage is returned.
Otherwise a new stage with those information is created, put in the
cache and returned. One particular important point is that cache
matching works with partial information. If a cache entry was created
with information A and B, then trying to retrieve a stage with only
information A will find the entry A+B entry.

The concrete case for Maya is that stage are searched using the root
layer pointer and optionally the session layer pointer. If both are
provided, then both must match. But if only the root layer is provided,
then any stage with that root will be returned, and thus the session
layer that was in that stage will be used by the Proxy Shape. One
particular important point is that it is the ID of the layers that are
compared. If a layer change ID, then it won't match anymore. This is
relevant when saving...


### Proxy Shape

The Proxy Shape provides access to the USD stage. It does *_not_*
initially keep:

  * The Stage nor a pointer to it
  * The layers nor pointer to them
  * The session layer nor a pointer to them

So how does it find all of these? It *_optionally_* keeps:

  * The ID of the root layer.
  * The ID of the session layer.
  * A flag to know if the stage is shared.
  * A flag to know if the stage is actually provided as an input attribute.

The main difference between shared and unshared stage is that unshared
stages have their own unique session layer.

To find the Stage, so that it can implements its getStage() function,
the Proxy Shape computes to root layer pointer and session layer
pointer. It does this by checking if it holds a root ID and session ID,
respectively. If it does hol valid ID, then it asks the Layer Manager
for the corresponding layer. If the ID are not set, then the compute for
these layer returns a null pointer. For the root layer, a null layer
triggers the creation of a new anonymous layer in the Proxy Shape. That
is how a newly-created stage is initialized with a root layer.


### Layer Manager

The layer manager is both a class with many static functions and a Maya
node. It is where the known layers are kept in Maya. It is used both by
the Proxy Shape to find the layers and by the Layer Manager UI to show
the known layers to the user.

One peculiarity is that the Layer Manager's Maya node is only briefly
kept alive. When a Maya scene is loaded, the Maya node contains the list
of known layers ID. That is used to re-open or re-create all these known
layers. The ID of the layers are forced to be the ID that were kept in
the Layer Manager node. Once this layer loading is done, the Maya node
is destroyed. It will only be created when the scene is about to be
saved. Also, this processing to find all layers is done the first time a
layer is requested. It is done when LayerManager::findLayer() is called.
This will be important when we discuss the problems we encountered in
the saving process.

Why were the layer found on first access? I don't know but I would think
it would be because the Proxy Shape uses the Layer Manager to find its
layers and when the Proxy Shape will be computed is ill-defined, so the
Layer Manager needs to be ready to provide them at any time. In
particular, it probably could not assume that a post-scene-load callback
would be triggered early enough. It might happen that the Proxy Shape is
accessed earlier and would thus need the Layer Manager to be ready
earlier? This is speculation, of course. (Tim Fowler might remember.)


### Layer and Load Rules Saving

The USD Stage is a pure run-time object. The things that are saved are
only the layers. Stages get re-created based on layers, as explained above.

Layers are saved just before the Maya is saved. Concretely, they get
saved in a Maya pre-save callback. So the process is that all layers
that needs to be saved to external files are saved, then all layers that
need to be saved inside Maya, in particular the session layers, are
serialized into text attributes in the Layer Manager Maya node.

This layer-saving process is not monolithic. We allow the concrete USD
plugin to provide a callback to do some of the work of saving layers. If
such a callback is provided (and the Maya USD plugin does provide one),
then it was being called (this changed in one of the fix described
below) with a list of DAG node paths pointing to each Proxy Shape. The
callback could do whatever it wanted. It would then return if the save
should be aborted or if the entire saving process was handled by the
callback. If it was *_not_*, aborted but not entirely handled either it
is a what is called a partial save. In the Maya USD plugin that is the
case, the callback does only a partial handling. Then, to handle that
partial save, the Layer Manager would re-check the list of layers to
find which ones were still dirty and still needed to be saved. It would
then do the saving of these layers itself.

An important detail, is that to find the list of dirty layers
post-callback, the Layer Manager would ask the Proxy Shape again for its
stage. In particular, if the stage had been dirtied for any reason, then
it would be re-computed. That ended-up causing problem as we will see below.

An important addition is that the load-state of USD payload are pure
stage-level information. That means they would not get saved at all. In
Maya, we work around this by saving the current load state of all
payload for each stage as an attribute on the Proxy Shape. Importantly,
this is also done in a Maya pre-saved callback. This will come up in the
problems we encountered when saving layers, below.


## Possible Improvements

Many of the problems that happened were due to the facts that:

  * The only persistent way to refer to a layer is by its ID.
  * Saving a freshly-created layer changes its ID.
  * (So does saving an existing layer under a new filename.)
  * So everything that kept a reference to a layer by keeping this ID
    need to update their data when the ID changes.
  * We don't have a mechanism to notify of layer ID change.

There is already a layer-ID-change notification mechanism in USD:
SdfNotice::LayerIdentifierDidChange.


## FAQ

Various questions raised by this investigation. For these questions, we
believe we have a valid answer.

### What stages caches do we have?

*Answer*: See the Stage section above.


### Is USD creating them for us? Are we (maya-usd) creating them?

*Answer*: The Autodesk Maya USD plugin (we) are creating them. Our own
code in the UsdStageCache class is creating and holding the caches.


### How many pre-save callbacks are there? Are they dependent?

*Answer*: There are two pre-save callbacks, with no dependence between
them. They are:

  * The Layer Manager callback, that does the USD layers saving.'
  * The Load State callback, which saves the USD payload state
    (loaded/unlaoded) in a proxy shape attribute.

Pierre T. suggests having a single pre-save callback, so that we can
control the ordering. OTOH, currently they do independent work and their
ordering does no longer matters since we fixed the bug that caused the
Layer Manager node to be deleted. Pierre T final assesment isthat no
code changes are needed to better structure this, but we should still
document their existence in a central place.


### Should we flush caches when saving?

*Answer*: If saving a layer changes its ID, then the entry in the stage
cache(s) is no longer valid. We currently update the cache so that
already-loaded stages that have a root layer that has changed filename
(either becasue it went from anonymous to file-based or its file name
changed) have the correct payload load-state. The reason we do this is
that the proxy shape finds the stage by the root layer ID, and if we did
not pre-fill the stage cache with stages usings the new root layer with
the correct load state, when teh proxy shape is recomputed the load
state could change which would surprise and annoy the user.


### Why creating a new stage used to lose the session layer.

*Answer*: The creation does not lose the session layer. It was teh
saving of a freshl-created stage that lost the session layer. The reason
being that the root layer woudl change ID and the Proxy Shape would ask
USD for a stage for that new root layer and would get a stage with a
different session layer since the root layer ID had changed. All of this
is fixed with the proposed code change.


### Why do we pass only the root layer to find shared stages?

Normally, for shared stages, only the root layer is given to
UsdStage::Open(): I don't understand this.  If you pass partial
information,  you will get an existing stage.  This is wrong.

*Answer*: no, it is right! The goal is that if we create multiple shared
stages with the same root layer, they will share the session layer. For
example, if the user creates a new stage, all that the user specifies is
the file-name of the root layer (and a few flags not relevant for this
discussion). In order to find a previously-existing shared stage, only
the root layer must be passed so that USD will return the existing
stage, thus implementing the desired sharing of the stage and session
layer. We could not possibly pass a session layer, because we don't have
it and actually want ot find a stage wirth the root layer and existing
session layer, if any are already loaded.


### Why do we need the Maya layer manager node to be destroyed and re-created?

*Speculative answer*: The Layer manager uses the existence of its Maya
node as a flag to know that it has not yet executed the post-read
processing of layers. Also, during a Maya session, the Layer Manager
node is not required. The node exists solely to be able to save
information into the Maya scene file, it is not used to create DG graph
and connections.


### Why does the layer manager have a layer database?

Why is it not solely responsible for serialization? Doesn't USD have a
layer database, which we  could use as the Single Point of Truth?  Why
can't we use SdfLayer::GetLoadedLayers()?

The post-read processing triggered by the first call to findLayer() does
call SdfLayer::Find(). It also does special processing for USD layers
saved inside the Maya scene file. These layers are not on-disk and are
de-serialized from text attributes on the Layer Manager. This does not
explain why SdfLayer::Find() could not be called by others instead of
having findLayers(). It seems findLayers() only prupose currently is to
ensure that the post-read processing has been executed.

*Speculative answer*: it serve no other purpose than ensuring the
post-read processing has been executed. That could potentially be done
in a true post-read callback? Why was it not done that way? Maybe Tim
Fowler remembers.


### Why does LayerManager::findLayer() triggers processing?

The default implementation of
MayaUsdProxyShapeBase::computeSessionLayer() calls
LayerManager::findLayer()!

*Speculative answer*: to avoid order dependency or de-serialization and
computation between the Proxy Shape and Layer manager. Maybe Tim Fowler
would remember the reason.


### Why Layer Manager ask the Proxy Shape for the stage again after calling the optional save callback?

Why doesn't the layer manager have the list of its layers, and can
access it directly, at this point?

*Partial answer*: with the proposed code change, the Layer Manager does
keep the list of layers.

*Speculative answer*: maybe it did not want to assume what the
save-stage callback did. It wanted to allow the callback to create new
layers or even new stages?


### What is the session layer ID problem?

It is never saved to disk, therefore never changes ID, no?

*Bug-related answer*: the Proxy Shape had no session layer ID attribute
set. It would fail to find the same sessin layer if teh root layer ID
changed.

*Partial answer*: the session layer gets serialized to a text buffer.
Maybe that affects its ID? Or maybe freshly-created layers don't have an ID?


### What are the responsibilites of the various objects?

We need to write more technical documentation to answer this.


## Session Layer Loss

At one point, we started having multiple bugs and problems which turned
to all be related to the session layer being lost when saving.

There were multiple reasons why the session layer was lost. They either
interacted with each others, causing a domino effect or were piling up
on each other, making fixing harder since there was not a single cause.
We will go over each problem and how they were fixed. Note that some of
the fixes overlap each other in some case. We still keep all fixes so to
have a defense-in-depth approach where multiple things will need to be
wrong instead of having a single point of failure. (For example,
removing some fixes would mean the Proxy Shape would need to not be
modified by any code or would need to not be re-computed. The fixes both
make the re-compute sturdier *and* avoid re-computing the Proxy Shape
mid-save.)


### The Root Layer ID Problem

When the root layer of a freshly created Stage gets saved, its ID
changes from no-name to file-name. To be able to find the layer again,
this new ID is set on the Proxy Shape. This allows the Proxy Shape to
find it again, but it also marks the Proxy Shape dirty, which mean it
will be re-computed when accessed again. This ID change is done in the
Layer Manager when it saves the layer. The Layer manager updates its
internal layer database, so its findLayer() will be able to find the
renamed root layer, in particular when the Proxy Shape asks for it. The
problem was that since the root layer had changed ID, it would not be
found in the Stage caches.

*Fix*: update the Stage caches when the root layer ID changes.


### The Session Layer ID Problem

The Layer Manager also does this for the session layer. Except there was
a bug: it would only set the session layer ID attribute on the Proxy
Shape when saving *_all_* layers inside the Maya scene file. If
non-session layers were saved to external USD files, then the session
layer ID would _*not*_ get set on the Proxy Shape. This would lead the
Proxy Shape to not find the session layer. The reason being that the
root layer would change ID, thus would not be found in the cache, thus a
new stage would be created with a new session layer.

*Fix:* set the session layer ID on the Proxy Shape when saving to
external USD files.


### The Session Layer ID Timing Problem

When the Layer Manager saved the root layer of a freshly-created stage,
the layer ID changes and the attribute that keeps the root layer ID on
the Proxy Shape would be set. Afterwards, the Layer Manager would
retrieve the stage from the Proxy Shape. This would trigger a re-compute
which would end-up creating a new stage with the root layer (since it
changed ID), which would lose the session layer.

The reason it loses the session layer is that:
- USD stages are found in the cache using all informations passed in
UsdStage::Open()
- UsdStage::Open() receives the root layer of the desired stage.
- UsdStage::Open() can optionally receive the session layer.
- The session layer is only set on the proxy shape once it gets saved.
- The session layer is saved after the other layers are saved.
- Normally, for shared stages, only the root layer is given to
UsdStage::Open().
- The stage cache will thus find the same previously-created stage again
and again... and thus the same session layer.
- ... except mid-save, when a previously unsaved root layer is saved,
its ID changed and a new stage gets created, with a new session layer.
- ... so we lose the session layer.

An issue here is that we cannot fully ensure that no code modifies the
Proxy Shape or that no code accesses the Proxy Shape and triggers a
re-compute. It is very easy to overlook one code path or have a rare
notification be sent mid-way through the save. The best approach in the
short term is to try to make the current behaviour sturdy and in
particular avoid accessing the Proxy Shape multiple times during the
saving of layer and the Maya scene.

*Fix*:
- Make sure we get the same stage and the same session layer after the
callback.
- The way the fix does this is by not accessing the proxy shape again
after the callback has run.
- Instead, we retrieve all information we will need before-hand, before
any other code runs, including the callback.
- This ensure we don't access the proxy shape in the middle, between the
callback and the rest of the USD layer saving code.
- Since we retrieve the information, we might as well give it to the
callback to avoid doing the same retrieval multiple times.
- It also reduces the probability that the callback would accidentally
cause a similar bug where it would modify the proxy shape
  in one part of its code then access it again, triggering a recompute
that would lose the session.


### The Disappearing Layer Manager

After the previous fixes, we would lose the Layer Manager Maya node. It
would not get saved in the Maya scene. It was created during the USD
layers saving process, but was gone by the time the Maya was saving the
scene. Why?

It turned out that we save the load rules as an attribute on the Proxy
Shape in order to be able to apply them again when we reload the Maya
scene as explained in the layer and load rule saving section above. The
load rules are saved in a pre-save callback, which modifies the Proxy
Shape since the load rules are set as an attribute on it. Because it
accesses the proxy shape to retrieve the stage, in order to get the load
rules, the Proxy Shape ends-up calling LayerManager::findLayer().
Unfortunately, findLayers() is where the layer database is initialized
and once initialized, it deletes the Layer Manager Maya node! So the
Layer Manager Maya node would get deleted in a pre-save callback. When
the Maya scene was saved, the node would already be gone and would not
get saved.

*Fix*: we now no longer delete the Layer Manager Maya node when we are
in the middle of a Maya scene save. The reasoning being that while in
the middle of a save, code will have created the Layer Manager node and
nothing that would accidentally call findLayer() should cause the node
to be deleted, since we need to have it to be able to save it into the
Maya scene, since the layer manager node is how we save layers in Maya.
(The layers are attributes on the layer manager node.)
