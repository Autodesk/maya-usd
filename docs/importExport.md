# Import and Export functionality

Supported Types
- Mesh 
- NurbsCurve
- Transforms (always exported)
- Camera (no special flags needed - will auto-detect cameras and export correctly)


## Import 

You can either use:
+ the command (AL_usdmaya_ImportCommand)
+ the maya "file" interface

Usage is as below (see Example Usages for Export)

Full set of supported import options are described [here](https://animallogic.github.io/AL_USDMaya/structAL_1_1usdmaya_1_1fileio_1_1ImporterParams.html)


## Export
Will export a part of your maya scene into the USD format for selected types

You can either use:
+ the command (AL_usdmaya_ExportCommand)
+ the maya "file" interface

If you want the export to happen from a certain point in the hierarchy then select the node in maya and pass the parameter selected=True, otherwise it will export from the root of the scene.

Full set of supported export options are described [here](https://animallogic.github.io/AL_USDMaya/structAL_1_1usdmaya_1_1fileio_1_1ExporterParams.html)


### Example Usage

Using File interface (also available from File->Export menu - File type is "al_usdmaya export"
```
file -force -options "Dynamic_Attributes=1;Meshes=1;Nurbs_Curves=1;Duplicate_Instances=1;Use_Animal_Schema=1;Merge_Transforms=1;Animation=1;Use_Timeline_Range=1;Frame_Min=1;Frame_Max=1;Filter_Sample=0;" -typ "AL usdmaya export" -es "/var/tmp/vv.usdc";
```

Command Examples
If you want to export everything keeping the time sampled data, you can do so by passing:
```
AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -animation
```

Exporting attributes that are dynamic attributes can be done by:
```
AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -dynamic
```

Exporting samples over a framerange can be done a few ways:
```
AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -frameRange 0 24
AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -ani
```

Nurbs curves can be exported by passing the corresponding parameters:
```
AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -nc
```
  
The exporter can remove samples that contain the same data for adjacent samples
```
AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -fs
```


## Mesh Export
For meshes normally we export:
1. Topology and Point Positions
2. Indexed UVs 
3. Colour Sets (by default RGBA and faceVarying)
4. Subdivision Edge and Vertex Creases
5. Dynamic Attributes

An option "meshUV" indicates AL_usdmaya_ExportCommand to only export Indexed UVs and leave other part of USD hierarchy as empty "over" prims. Under this mode, "leftHandedUV" is used to adjust UV indices orientation.

**Use Animal Schema**
The "Use_Animal_Schema" option when exporting was originally intended to export mesh data in a way which was easier for our inhouse Renderer to consume.
The salient differences are:
+ Export Indexed Subdiv Crease Data
+ Imports/Exports Subdivision-related Flags used by our Renderer as attributes
+ When exporting Colour Sets, names them prefixed with "alusd_colour_" to avoid clashes with Pixar Schemas, and force to RGBA and per Face

**Note: This special schema needs to be revisited, possibly removed/made more generic (@todo Reference Github issue)**
**The colour handling in particular is a bit of a mess at the moment, and is hardcoded to support some very specific use cases. Until we fix all this, might be worth using the Pixar Plugin for mesh import/export**


# Tech Note: Colours on Geometry
USD has an convention enforced in a Schema related to having a Colour Set on your mesh called "primvars:displayColor" [see here](https://github.com/PixarAnimationStudios/USD/blob/4a3f61e50fd862bfaa57b9a06b5f3d3c05c3bb09/pxr/usd/lib/usdGeom/gprim.h#L131).
This will be used as a fallback by Hydra and other viewers if there is no other shading information (There is also a matching displayOpacity)

To have geometry colours displayed by Hydra your data needs to look something like one of the 3 meshes in [this](../samples/colours/meshColours.usda) usd file.
Colour Data can be exported:
1. Per Vert
2. Per Face
3. Per Vert per Face (FaceVarying) Not 100% sure that this is either working or supported by Hydra

The exporter will export the data as it is stored in Maya - non-indexed


### How do we export in AL_USDMaya so we have colours?
1. Make sure that one of the colour sets in your mesh is called "displayColor" (there is an example maya file [here](../samples/colours/faceColours.ma))
2. If we don't use the "Animal_Schema" when exporting the names of Colour Sets will be preserved (but exported as faceVarying)


### A note on Alembic+Maya+USD 

If I export to alembic from maya e.g
```
AbcExport -j "-root |pSphere1 -writeColorSets -f /var/tmp/colouredVerts.abc";
```
...where -wcs is "Write all color sets on MFnMeshes as color 3 or color 4 indexed geometry parameters with face varying scope".

Then I re-import I see the colours again in maya - great!

Looking at the alembic with abctree:
```
:--.childBnds
 :--statistics
 `--pSphere1
     | `--.xform
     `--pSphereShape1
         `--.geom
             :--.selfBnds
             :--P
             :--.faceIndices
             :--.faceCounts
             `--.arbGeomParams
                 `--colorSet1
                     :--.vals
                     `--.indices
```
shows there is an indexed colour set..

In the current version of the USD ABC Plugin(0.7.6) it looks like these indexed UV sets are correctly converted to Face-varying data...and display properly in the viewport (this was not working in older versions of USD)












