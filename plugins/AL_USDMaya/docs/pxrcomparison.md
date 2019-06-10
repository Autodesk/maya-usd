This is a table comparing some of the features (mostly import/export functionality) of AL_USDMaya and Pixar's USDMaya plugin (roughly up to date at 15/10/2018). 
Thanks to Christopher Moore from Blue Sky Studios for making the first version of this


| Feature Type  | Feature Description | AL_USD | PxrUSD | Notes                  |
| ---------------- | ------------------ |------------- |------------- |---------------------------------------------------- |
|**Exporting**||||
| |**Animated Visibility**| yes | yes | |
||**Camera**| yes | yes ||
||**Filtering / Compressing**||||
||Duplicate Instances| yes| yes|  see [AL_USDMaya docs](importExport.md#transform-merging-instancing)|
||Merging Transforms| yes| yes| see [AL_USDMaya docs](importExport.md#transform-merging-instancing) |
||Time Samples| yes| needs investigation |pxr seems to have no compression flags|
||UVs |yes| needs investigation| pxr seems to have no compression flags|
||**Frame Samples / Sub-Frame Export**| yes |yes| see [AL_USDMaya docs](importExport.md#subsample-export)|
||**Instancing**| yes| yes||
||**Lights**|no |yes |Pixar only supports RfM Lights. AL todo: Include Reference Translator for Maya Lights|
||**Mesh**||||
||Topology and Point Positions| yes| yes||
||Varying Topology| no| yes||
||Indexed UVs| yes| yes||
||Color Sets (by default RGBA and faceVarying)| yes| yes| AL_USD requires colorSet to be called displayColor ( displayOpacity is also supported ). See [AL_USDMaya docs](importExport.md#tech-note-colours-on-geometry). pxrUSD has the same displayColor set requirement, however it handled the missing 'flood data' elegantly. multiple colorSets have not been tested yet*|
||Subdivision Edge and Vertex Creases | yes | yes ||
||Dynamic Attributes| yes |yes||
||**Metadata / extra attrs**| yes| yes||
||**NurbsSurface**|no |yes ||
||**NurbsCurve** | yes| yes | Note: Not currently renderable in Hydra|
||**BasisCurve** | no| no | Note: Renderable in Hydra|
||**Particles** |no |yes||
||**Shaders** |no |yes |PxrUsd only supports RfM Shaders|
||**Skin/Joints** |no |yes| PxrUsd has basic USDSkel support |
||**Transforms**| yes| yes||
|**Importing**||||
||**Lights**|no |yes| PxrUsd only supports RfM Lights |
||**proxyShape** |yes| yes |Both have this implemented in similar ways ( at least from USD -> Hydra -> Viewport )|
||Supported Animation| yes| yes| through time1 connection|
||Load Part of Stage |yes| yes| yes, each plug-in has it's own way of importing 'fast' (ie: proxyShape), then diving down into transforms to interact with / update the usd stage (visible in the viewport )|
||Load / Switch Variants| yes| yes| AL_USD supports this through listening to variant switches in the USD Stage. PxrUsd has a variantKey on the proxyShape, but that does not seem to work. However the pxrUsdReferenceAssembly nodes are sub-classes of kAssembly's and do offer the representation switching|
||**Shaders** |no |yes| PxrUsd only supports RfM Shaders|
|**Selecting / Editing**||||
||**Constraint Based Workflows**| yes| limited | AL_USD has bi-directional constraint support, including access to constrain to individual elements of the USD Stage. PxrUSD has limitations due to the Assembly node and not having 'connect to prim' capability (out-of-the-box, might be possible through scripting and such )|
||**Interface with Native USD** |no* |no* |not natively, but through custom commands / plugins ( pxrUsdReferenceAssembly or AL_usdmaya_Transforms or a proxyShape node to each USD Stage )|
||**Layer Management**| yes| no |(not out-of-thebox) AL_USD serializes 'live layer data' so when artist re-opens maya session it opens the usd stage, then your local edits. Once task is complete, layer can be saved out and merged back to the usd stage/file ). PxrUSD seems to require full import / export from usd -> maya nodes, which can impact Maya performance |
||**Partial Stage Selection**|yes| yes ||
||**Variant Switching to Rigs**| yes| no| AL_USD has a schema for MayaReferences to allow to switch out to rig representations. PxrUSD, while it has slightly better UI to switch model/shading variants (mainly due to being a sub-class of an assembly node), does not have built-in support for switching to a rig variant |
||**VP2 Selection from Stage to Outliner** |yes |under investigation | AL_USD has extra selection management going on, and hooks into it's ProxyShapeImportTransforms command. PxrUSD selection ( back to native dag nodes of some form) might not be possible this is being investigated |
||**VP2 Selection of Stage** | yes |no/ip | see [AL_USDMaya PR](https://github.com/AnimalLogic/AL_USDMaya/pull/118)  |
