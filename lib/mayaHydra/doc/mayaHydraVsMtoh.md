# Difference between MayaHydra and Mtoh

The Mtoh plugin, contributed by Luma Pictures represents the foundation for the work currently being done on MayaHydra viewport for Maya. The plugin faced complex challenges to add support for the remaining full range of Maya features. 

## Mtoh (Luma Pictures)

This code is an MRenderOverride Maya plugin, which binds the plugin to a window, adds an entry in the Renderer menu, and allows control of overall viewport rendering.

### *Data Access and Interpretation*

The plugin gets scene data out of Maya using the MFnMesh API which was available at the time. Many of those APIs were not designed for real-time rendering use. This could be fine for something like a one-off export of scene data, but can sometimes be cumbersome to interpret and too slow for satisfactory interactive use. Maya was missing any clear system to extract an abstract description of what should be rendered for a given node type with given attributes.

Maya has a wide assortment of systems that control whether an object displays in a viewport:  The visibility attribute, inheriting the effect of the visibility attribute of a DAG parent, the drawing override flag to override the inherited visibility status, the hide on playback attribute, the LOD visibly attribute, the viewport show filter, isolate select status, and more. A plugin can query these raw status values but then it's left up to the plugin to figure out how to combine them and how to efficiently manage changes.

### *Change Notifications*

Change notifications were received one attribute at a time, through various MDagMessage and MNodeMessage registrations.  This is too granular to scale well for animation or interactive changes to a large scene.  It works poorly with the Evaluation Manager (and related EM caching system) because these notifications are tied to the original DG dirty propagation system and bring along the associated main thread constraints.

## MayaHydra

To resolve these issues, we are adding a new "DataServer" API that lets the plugin code cleanly tie in to related viewport 2.0 code.  We can serve data to the plugin in a format more optimized for interactive rendering.  The plugin will feed that data into Hydra to render. Very roughly speaking, in terms of internal Maya architecture, we keep the code that translates Maya objects into renderable data and cut out the renderer-oriented code in vp2 and replace it with Hydra.

### *Data Access and Interpretation*

Data Access and Interpretation
Instead of always extracting state from the raw attributes of a Maya node, the DataServer API provides a more abstract view of things to be rendered via the (pre-existing) MRenderItem API.  A render item is similar to a Hydra RPrim and roughly corresponds to a traditional 3D "draw call" in OpenGL or DirectX, even if the data ends up in a batch and will generate no single draw call. It is an association of geometry buffers with an index buffer, a primitive type (triangles, lines, points), a shader, and shader parameters (transform matrices, colors, textures, etc).  MRenderItem was originally designed for plugin shapes to describe to vp2 how to render an object and for vp2 to describe an object to plugin shaders. The MRenderItem wraps an internal renderable item that tells VP2 what to render. Any future replacement of MRenderItem would be a similar abstraction.

One effect of the MRenderItem approach is that the plugin no longer needs to care about the details of most Maya node types.  For something like an ikHandle, the original mtoh approach involved writing an adapter class that manages all the node attributes for the type.  In the MRenderItem approach, the plugin doesn't necessarily need any ikHandle-specific code.  It receives an MRenderItem with the line geometry and color information, which is enough to render and support selection.  In this way, we leverage the existing Maya viewport code that implements the "business logic" that decides what an ikHandle looks like.

The MRenderItem approach might not make sense in all circumstances and can be optional.  The lead case is that mesh geometry buffers might work better using the MFnMesh interface that the original mtoh code uses or or some new interface with similar properties.  The default geometry format that the Maya viewport translation code produces has been triangulated and re-indexed so that all geometry streams share a single index buffer.  This triangulated, singly-indexed geometry is what the gpu eventually needs.  But Hydra, and especially Storm, prefer untriangulated face data.  It uses OpenSubDiv geometry shaders to triangulate meshes and natively prefers to generate normal buffers on the gpu.  So for the simple default case of drawing a mesh, possibly with its full wireframe displayed (and nothing else), it's unclear what value the render item system provides.  We have the option of bypassing the render items and providing raw face data, skipping normal buffer generation and re-indexing.  This option is likely to be simpler and more efficient for the default case for meshes.  When MRenderItems are needed for meshes using extra display modes, we can combine those approaches.  We expect animation workflows to fit better with the raw face data approach and modelling workflows to require the MRenderItem approach.  This requires more experimentation to validate these assumptions.

We have flexibility to handle other object types outside the MRenderItem system if we find a need.

Regarding copies of heavy data buffers (mainly mesh geometry buffers), current prototype code involves one redundant copy of the buffer.  The mtoh adapter class holds a persistent copy of the data in a VtArray.  Theoretically, this copy can be removed by holding only a span of the source buffer.  This may require some changes to manage buffer lifetime safely, but may be worth it if we have benchmarking that demonstrates the value.  Early benchmarks show this extra buffer copy costing 2% of time for a deforming medium density (160k triangle) mesh.

### *Change Notifications*

Instead of using low-level DAG and attribute change notifications, the maya-hydra plugin can use the internal change notification system used by vp2.  This system boils down different attributes into simpler categories that better align with Hydra HdDirtyBits.

#### *Adding and Removing primitives. Changing transforms, geometry and topology*

These operations are what Hydra needs and matches what we already have internally in vp2.
Maya has some complexities with these notifications where sometimes they are not delivered when objects are hidden, because they are expected to not be needed and can be expensive in a large scene.  By handling more of this internally we can hide some complexity and leave more optimization flexibility in the future.
This internal change notification system is efficiently and thread-safely integrated with the Maya Evaluation Manager and the EM Caching system.
Hybrid Rendering
The UI elements currently drawing in hybrid mode with vp2 can be adapted to draw natively using Hydra.  If we adapt this to the MRenderItem DataServer model, it should be low risk.  Eventually, we would need to do this to support a pure Metal system on Macs or any other hypothetical render delegate with limitations that prevent mixing with vp2 OpenGL code.  We may still need some form of hybrid rendering in the future if some render delegates are lacking features to support all UI elements.  We may need to draw a layer with Storm and composite over the "main" render delegate.  MRenderOverride will be useful to control that.
