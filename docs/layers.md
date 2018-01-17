# Layers

AL_USDMaya allows you to edit any of the layers used in a USD Stage as well as creating and modfiying them.

Layers can be modified using the USD API 

#### Serialising Modified Layers
Any layer changes you make will be serialised to the maya scene when the scene is saved, and reapplied when the scene is opened.
To record edits to a layer, you need to set it as the current [Edit Target](https://graphics.pixar.com/usd/docs/USD-Glossary.html#USDGlossary-EditTarget). 
AL_USDMaya will record which layers have been set as the edit target during a session, and when the scene is saved (via an OnSceneSaved callback) will serialise their content into the maya scene. Those layers will be deserialised into the live USD model after the scene has been opened again via an equivalent OnSceneOpened callback).
Normally, the default Edit Target in USD will be the Root Layer of the scene, although using the [Session Layer](https://graphics.pixar.com/usd/docs/USD-Glossary.html#USDGlossary-SessionLayer) is something we should consider.


#### Commands 
There are a number of layer-related commands available, all are available via MEL, and some from the USD->Layers dropdow in the UI.
+ AL_usdmaya_LayerSave
+ AL_usdmaya_LayerCreateLayer
+ AL_usdmaya_LayerCurrentEditTarget
+ AL_usdmaya_LayerGetLayers
+ AL_usdmaya_LayerSetMuted

These are documented in detail via their respective command help strings (-h/-help)

## Examples

This example shows how we record changes to the current Edit Target Layer, and output them to a string (can of course be saved to file 
also)
```python
#set this to the root of your source distribution
AL_USDMayaRootPath = 'MYSOURCEROOT'

from maya import cmds
#Import very simple ball - everything will be displayed in hydra, so this should create a single transform and shape
maya.cmds.AL_usdmaya_ProxyShapeImport(f=AL_USDMayaRoot + '/extras/usd/tutorials/endToEndMaya/assets/Ball/Ball.usd', name='shot')
#Create a maya transform (which will drive the USD values) for each prim in USD

#Create maya transforms for all of the prim paths, and move the ball  
cmds.select("AL_usdmaya_ProxyShape1")
maya.cmds.AL_usdmaya_ProxyShapeImportAllTransforms(pushToPrim=True)
maya.cmds.setAttr("shot|Ball|mesh.translateY",10) 

#Export the root layer to a string
from AL import usdmaya
theStageCache = usdmaya.StageCache.Get()
theStage = theStageCache.GetAllStages()[0]
print theStage.GetRootLayer().ExportToString() #making an assumption about root layer being current layer
```

You can see that this will be serialised in the maya scene if you find the appropriate layer, and print the contents of its "serialised"(szd) attribute:
```python
print cmds.getAttr("Ball_usd.szd")
```    




#### Session Layer

+ Session Layer support
+ Layer Creation Commands
+ Serialisation etc? Storing Layer Contents on the proxy shape
+ Animation

At Animal Logic we use this set of tools for our Shot Set Editing workflow

@todo Baz!


We currently create a layer DG node for each layer in the root layerstack. 
How do we know if this has changed?
We serialise this into the relevant DG node when we save the maya scene
A layer is either owned by USDMaya or not - if it’s owned by USDMaya we allow you to make any changes to that layer you want, and that layer is serialised on scene save, and deserialised on scene open

Creating temporary layer, checking in etc .. how does it work?
Working from an existing layer vs creating one from scratch

We use this system to implement our Shotbased-Set Overrides workflow - we import a shot which contains a set (sublayer or reference?). We want to be able to override transforms and visibility in that set and save the changes as an override layer which is passed down our pipeline
