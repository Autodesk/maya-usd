
# Animated Geometry Importer

In this folder there is a sample script that shows how to implement an "AlembicMaya" like import workflow in AL_USDMaya - i.e we:
+ import a Proxy shape that contains some models that contain animated Geometry (typically...a character or prop)
+ We want to convert that USD geometry to animated geometry in maya which is driven "live" from the Proxy Shape and animates as we scrub the timeline

This functionality is supported by AL_USDMaya, but is not exposed in any easy to use way - this script is an attempt to make that easier

```
# Import the animatedMeshImport class in your preferred way 
proxyShape  = cmds.AL_usdmaya_ProxyShapeImport(file='/your/usd/file.usdc')
shape = usdmaya.ProxyShape.getByName(proxyShape[0])
prims = Usd.PrimRange.Stage(shape.getUsdStage())
primList = [prim for prim in prims][:1]
parents = cmds.listRelatives(proxyShape[0], ap=True)
animatedTranslator = TranslateUSDMeshesToMaya(parents[0], primList)
```
