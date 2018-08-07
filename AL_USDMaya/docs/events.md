# Reacting to USD Events

AL_USDMaya reacts to specific USD related events (known as "Notices" in USD parlance) and performs modifications to the maya scene based off these. The way this system works is currently not very nice, and we are looking for better ways to implement this functionality. 

## Implementation Details
We do not want to listen to all events as there are many, so currently we only support the ones listed below. We generally listen for one "high-level" event, change some internal state to indicate to AL_USDMaya that following finer-grained events follow from that change, and then wait for an indicator that the higher-level event has completed (@todo: check this)

An examples is variant switching:
+ We have a listener on the notice SdfNotice::LayersDidChange 
+ When it's triggered we check if the change is a variant switch (or any other event type we support)
+ If so, do some preTearDown of "stuff" in the scene
+ Set some global state to indicate that following fine-grained notices on the affected Prim or it's descendants are related to that variant change
+ React to those finer-grained changes - check what's changed - if a relevant change has occurred trigger e.g the appropriate schema translator

There are a few issues with this system:
+ Only allows a single Prim Hierachy to change at a time as that's the granularity of our state change
+ Easy to see how recursive changes e.g one event which triggers another etc could cause difficulties
+ In general - we would like to have a transaction-based system where we could be sure where each set of changes began and ended - at the moment we're guessing

For anyone interested in delving into this in more detail, useful USD references are:
+ [Explanation of Notice System](https://graphics.pixar.com/usd/docs/api/page_tf__notification.html)
+ [TfNotice diagram](https://graphics.pixar.com/usd/docs/api/class_tf_notice.html)

## Events
That said, here's a catalogue of what we have:

### Variant Switching 
AL_USDMaya listens for an event which indicates a Variant switch.
When we detect this, we listen for object changed events, and for each prim (recursively), we compare to see if that prim has changed, calling the appropriate schema translator plugin methods 

### Active/Inactive
A USD stage may contain prim hierarchies which are inactive initially (We use this for pruning e.g characters which we don’t want to import by default). 
We may however decide to import one or more of those characters later, which is why we listen for the “active” and “inactive” events. 
SchemaTranslatorPlugins can be implemented so they handle this - e.g with a MayaReference prim, if it's made inactive, we unload the reference in maya (but don't delete the node).
This is done in a similar way to the Variant Switching.

## Manual Sync Command
There is also a use case where we make changes to the USD stage which do not trigger events we can handle correctly, but we may notify AL_USDMaya by calling [ProxyShapeResync](proxyShape.md#al_usdmaya_proxyshaperesync-overview) before calling stage.Reload() and triggering a flood of fine-grained events.An example of this is Version Switching:

### Version Switching Workflow
In our Pipeline, we use URIs to reference assets (which end up as USD Layers). These URIs can optionally contain version information. When they don’t, there is an assumption that we want the latest version of an asset. This can change if a new version of an asset is checked in, and we reload the stage (which command?)






