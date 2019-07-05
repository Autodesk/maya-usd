
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


