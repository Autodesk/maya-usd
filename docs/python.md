
# Python bindings
Python bindings are currently limited to what we either we or Luma Pictures have implemented internally based on our needs, and are by no means comprehensive

Currently all bindings are under the AL.usdmaya module
```
from AL import usdmaya
```

## StageCache
the "StageCache" Singleton class  allows python access to the set of in-memory stages 


Example: getting the USD stage (once you've created a proxy shape)
```python
from AL import usdmaya
stageCache = usdmaya.StageCache.Get()
stages = stageCache.GetAllStages()
if stages:
    print stages[0]
```


Method list is:
+ Get()
+ Clear()


## ProxyShape

The ProxyShape class wraps an instance of a maya proxy shape

You can get a specific shape in the scene with:
+ getByName

example:
```
from AL import usdmaya
myShape = usdmaya.ProxyShape.getByName('name_of_my_Proxy_Shape')
theStage = myShape.getUsdStage()
```

As well as a number of methods to manipulate a shape instance:
+ boundingBox
+ destroyTransformReferences
+ findRequiredPath
+ getUsdStage
+ isRequiredPath
+ makeUsdTransformChain
+ makeUsdTransforms
+ removeUsdTransformChain
+ removeUsdTransforms
+ resync


## LayerManager
Is a singleton which reflects the Layer Manager node in the maya scene. Once you've created or referenced the node via:
+ find
+ findOrCreate

there are  number of methods for manipulating the manager:
+ addLayer
+ findLayer
+ getLayerIdentifiers
+ removeLayer


Example:
```
from AL import usdmaya
theLayerMan = usdmaya.LayerManager.findOrCreate()
print theLayerMan.getLayerIdentifiers()
```
