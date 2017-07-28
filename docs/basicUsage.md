
# Basic Usage

### Load the plugin
```
from maya import cmds
cmds.loadPlugin('AL_USDMayaPlugin') 
```
The plugin registers a large number of maya commands and nodes. 
Once you load the plugin you will see the "USD" menu in maya which is an entry point to many of the commands

Most of the commands have help, so e.g from mel:
```
AL_usdmaya_ProxyShapeImport -help
```


### Using the Proxy Shape importer

```
AL_usdmaya_ProxyShapeImport -file "${AL_USDMAYA_SOURCE_LOCATION}/AL_USDMaya/src/samples/testchar_animallogo01_surfgeo_highres.usd";
```

Now:
- select the Proxy Shape
- if you click on any of the meshes/curves (Displayable objects) in the selected object (at the moment they dont highlight - you may need to use the Transform tool to see what's selected @todo) AL_USDMaya will create all of the transform hierarchy for that object(s) in Maya. 
- When you click off the selected object, they will disappear
- If you want to make these transforms permanent, from the menu, select "USD->Proxy Shape->Import Transforms as Transforms" (at the moment, you can only choose to create transforms for all of the objects in the scene, not just some of them.. it's a @todo). These transforms will be animated if the underlying USD data is animated



##Python bindings
There aren't many python bindings in AL_USDMaya - at the moment just the "StageCache" Singleton class which allows python access to the set of in-memory stages  


getting the USD stage (once you've created a proxy shape)
```python
from AL import usdmaya
stageCache = usdmaya.StageCache.Get()
stages = stageCache.GetAllStages()
if stages:
    print stages[0]
```
Note at the moment there's no way of easily knowing which stage corresponds to which proxy Shape. 
You could probably work it out by looking at something like the identifier/rootLayer of each stage and matching to something in the proxy shape..?

