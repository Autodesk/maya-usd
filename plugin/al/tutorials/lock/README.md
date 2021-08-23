# Lock
al_usdmaya_lock is a metadata used to determine lock state of a prim in Maya. If a prim is locked, its corresponding Maya transform object will have locked transform attributes (Translate, Roate and Scale) and disabled "pushToPrim" attribute. This prevents data being overwritten accidentally. By default, a prim is unlocked. 

## Demo
The following demo will show you how to make a prim locked or unlocked in Maya and a set of APIs to access lock state from a prim. 

Run the following commands in Maya's script editor with the AL_USDMaya plugin loaded.

***

Create a proxy shape pointing to our test file

```
cmds.AL_usdmaya_ProxyShapeImport(file="<PATH_TO_ASSETS_FOLDER>/lock_hierarchy.usda")
```

You should notice that "transA" and "camB" have their transform attributes locked and "pushToPrim" attributes disabled. Their descendants "transC" and "camD" are unlocked by "unlocked" metadata defined on "transC". And "transE" and "camF" are unlocked due to "inherited" by default.

![Lock](LockPrims.png)

***

Now let's lock "transC" and "camD" through ModelAPI.

``` 
from AL import usdmaya
from AL.usd.schemas import maya as maya_schemas
stageCache = usdmaya.StageCache.Get()
stage = stageCache.GetAllStages()[0]
transC = stage.GetPrimAtPath('/transA/camB/transC')
transCApi = maya_schemas.ModelAPI(transC)
transCApi.SetLock("inherited")
```
"tranC" and "camD" are locked now because they inherit lock from ancestors. 

***

Now let's unlock all nodes under "transA" hierarchy

```
transA = stage.GetPrimAtPath('/transA')
transAApi = maya_schemas.ModelAPI(transA)
transAApi.SetLock("unlocked")
```

***
To determine the lock status of a prim you can use ModelAPI::ComputeLock method

```
transCApi.ComputeLock()
```

To get direct metadata value of the prim, you can call ModelAPI::GetLock()

```
transCApi.GetLock()
```
