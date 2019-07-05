# Animation - USD Driving Maya Attribute Values and Vice Versa

## Attribute Storage in Maya
### Transforms

AL_USD Maya has it's own Transform Node defined [here](https://github.com/AnimalLogic/AL_USDMaya/blob/master/lib/AL_USDMaya/AL/usdmaya/nodes/Transform.h) which allows USD to drive Maya Transforms and vice versa.
This inherits from Maya's MPxTransform node and passes every attribute change to USD, then delegates to the base class functionality to behave like a normal transform.
This allows us to drive Maya Transforms from USD, and vice-versa (there is a "pushToPrim" boolean attribute which will make maya the "driver" of the Transform Attribute. 

> As USD's transform stack representation is more flexible than maya's one, the current implementation of transformations has some
> limitations. A more robust implementation is needed to encompass as much use cases as possible and reject non translatable cases.
> @elron79 has submitted an interesting [solution](https://github.com/PixarAnimationStudios/USD/pull/287) for that.
 
### Implementing your own nodes

While transforms are special, you can implement your own nodes using the same "Maya wrapping USD" technique

## Attribute Storage in USD
Another approach (which is not possible with Transforms due to restrictions on how their attributes are stored) is to use the setInternalValueInContext/setInternalValueInContext API from [MpXNode](http://help.autodesk.com/view/MAYAUL/2016/ENU/?guid=__cpp_ref_class_m_px_node_html)
This allows us to not store any data in Maya, and delegate the ownership and serialisation to USD.
This technique is illustrated in [Layers.h](https://github.com/AnimalLogic/AL_USDMaya/blob/master/lib/AL_USDMaya/AL/usdmaya/nodes/Layer.h) (although not for the purpose of animation)
@todo: example where driving animation....


## Baking Attribute Values 
When implenting a translator of any kind, you can of course choose other ways of communicating data between USD and Maya - or not - for example, you are free to bake any USD data into a maya fcurve and maintain no live connection. 


## Other topics

### USD Driven Transform ####
This is a set of Array Values on the Proxy Shape Node that allow you to drive multiple transform nodes in maya, batching up the updates to improve performance.
@todo: deprecate unless it's useful to someone?



## Driving Animated Maya Meshes from USD

As of AL_USDMaya 0.30.5, we have initial work on a pair of nodes to drive animated mesh data from USD - further work is scheduled to better integrate the nodes from this PR with AL_USDMaya. This PR adds two custom maya nodes:

**AL_usdmaya_MeshAnimCreator**
This node acts as a polyCreator node which reads a mesh prim from a USD stage, and pipes the output into the outMesh attribute. In many cases, this may be preferable to simply importing a mesh prim (since the creator node approach will not insert additional polygonal data into the maya ascii/binary files when saved). This node is ideal for static meshes. Whilst it can be used for animated meshes, it might be advisable to use.... 

**AL_usdmaya_MeshAnimDeformer**
This node acts as a very simple deformer node, which takes an input mesh, applies the vertex and normal values from USD at the given time code, and passes the result through to the output mesh. In theory this node should be much faster to evaluate than the AL_usdmaya_MeshAnimCreator node (since it doesn't need to re-specify face indices, etc). The downside is that it can't handle animated topology changes (and currently animated primVars are not supported). Using a AL_usdmaya_MeshAnimCreator node as an input to this deformer will give you the best of both worlds (zero data added to the maya file, and fast deformation times)

Eventually we will build these nodes into the TranslatPrim operation on a mesh, but that requires a few internal changes before we can fully support that. In the meantime, there are two menu items that have been added to the USD->AnimatedGeometry  menu. 

To use:

1. translate a mesh prim from USD into maya. 
2. Select the mesh that has been translated
3. Choose either 'Static' or 'Animated' from the USD->AnimatedGeometry menu. 

The static option will simply create a AL_usdmaya_MeshAnimCreator node as an input to the mesh. Visually nothing will change, however the mesh data is now gathered directly from USD. 
The Animated option creates a AL_usdmaya_MeshAnimCreator which feeds into a AL_usdmaya_MeshAnimDeformer node. Scrubbing through the timeline should now show you an animated mesh. 
