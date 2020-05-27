# Layers


AL_USDMaya has limited support via Maya commands for editing or retrieving layer contents. It does know which layers have been modified in memory by tracking which layers have been set as an EditTarget and that are dirtied via the LayerManager node.

Typically a user of AL_USDMaya will do modifications directly via the USD API and AL_USDMaya will react accordingly.

#### LayerManager
The LayerManager is a singleton that tracks all layers that have been set as an EditTarget and that are classified as "Dirty" by USD. The LayerManager can be retrieved via it's Python bindings or directly in C++, you can also retrieve it's tracked layer contents via the AL_usdmaya_LayerManager command.
The tracked layers will be serialised to the Maya scene when the scene is saved (via an OnSceneSaved callback) and reapplied when the scene is re-opened (via an equivalent OnSceneOpened callback)
To record edits to a layer, you need to set it as the current [Edit Target](https://graphics.pixar.com/usd/docs/USD-Glossary.html#USDGlossary-EditTarget). 

Normally, the default Edit Target in USD will be the Root Layer of the scene, but the proxyShape sets the [Session Layer](https://graphics.pixar.com/usd/docs/USD-Glossary.html#USDGlossary-SessionLayer) as the default Edit Target. This has several advantages such as: authoring edits as the highest strength opinion, concentrating edits in one place, and avoiding serializing heavy root layers.

##### Uses at AL
###### Modifications for exisiting layers
After doing in-memory edits to our USD scene changes(typically via Maya) we then translate our USD scene, which is a filepath to the root layer and the serialised content of all the modified in-memory layers that are tracked by the LayerManager, into our renderers scene description for rendering. 

###### Serialised anonymous layers
An anonymous root layer can be imported into the ProxyShape by passing the `stageId` into the AL_usdmaya_ProxyShapeImport command. This way the layer manager serialises changes that are made in the root layer without any backing `.usda` or `.usdc` file. As an example this follow script sets up a scene with a single sphere in an anonymous layer, and import that into a ProxyShape.

```python
from pxr import Usd, UsdUtils, UsdGeom
from maya import cmds

stageCache = UsdUtils.StageCache.Get()
with Usd.StageCacheContext(stageCache):
    stage = Usd.Stage.CreateInMemory()
    with Usd.EditContext(stage, Usd.EditContext(stage.GetRootLayer())):
        UsdGeom.Sphere.Define(stage, "/sphere").GetRadiusAttr().Set(1.0)

cmds.AL_usdmaya_ProxyShapeImport(
    stageId=stageCache.GetId(stage).ToLongInt(),
    name='anonymousShape'
```

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

#### Hydra Renderer Plugin

Layer manager defines Hydra renderer plugin that is used by all Proxy shapes for rendering. It can be set directly (`AL_usdmaya_LayerManger.rendererPluginName`) or with command (`AL_usdmaya_ManageRenderer -setPlugin "Glimpse"`).
List of available renderers is based on plugins discovered by USD. If there is more than one renderer plugin available a new menu entry USD > Renderer is added.

