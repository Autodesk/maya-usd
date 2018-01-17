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

### HostDrivenTransform Node ###
see [here](https://github.com/AnimalLogic/AL_USDMaya/blob/master/lib/AL_USDMaya/AL/usdmaya/nodes/HostDrivenTransforms.h)
@todo needs documentation

