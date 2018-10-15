This is a table comparing some of the features of AL_USDMaya and Pixar's USDMaya plugin (Information is roughly correct at 15/10/2018). 
Thanks to Christopher Moore from Blue Sky Studios for making the first version of this


| Feature Type  | Feature Description | AL_USD | PxrUSD | Notes                  |
| ------------------ | ------------------ |------------- |------------- |---------------------------------------------------- |
|**Exporting**||||
| |**Animated Visibility**| yes | yes | |
||**Camera**| yes | yes ||
||**Filtering / Compressing**||||
||Duplicate Instances| yes| yes||
||Merging Transforms| yes| yes||
||Time Samples| yes| needs investigation |pxr seems to have no compression flags|
||UVs |yes| needs investigation| pxr seems to have no compression flags|
||**Frame Samples / Sub-Frame Export**| no* |yes| fileio/AnimationTranslator.cpp seems to have this hardcoded to 1.0, should be easy to add|
||**Instancing**| yes| yes||
||**Lights**|no |yes* |Pixar only supports RfM Lights|
||**Mesh**||||
||Topology and Point Positions| yes| yes||
||Indexed UVs| yes| yes||
||Color Sets (by default RGBA and faceVarying)| yes| yes| AL_USD requires colorSet to be called displayColor ( displayOpacity is also supported ) pxrUSD has the same displayColor set requirement, however it handled the missing 'flood data' elegantly. multiple colorSets have not been tested yet*|
||Subdivision Edge and Vertex Creases | yes | yes ||
||Dynamic Attributes| yes |yes|
||**Metadata / extra attrs**| yes| yes|
||**NURBS**|no |yes |I re-ran the export of a nurbsSphere and it *did* work in pxrUSD. However, adding support to AL_USD for this (if still required) should not be a big deal|
||**NurbsCurve** | yes*| yes* |While it's true AL_USD and pxrUSD supports NurbsCurve export, it's not the format I think we'll need (i.e : BasisCurves ). We'll need some form of exporter for this. Maybe another opportunity for Open Source Contribution? :) |
||**Particles** |no |no||
||**Shaders** |no |yes* |Pixar only supports RfM Shaders|
||**Skin/Joints** |no* |yes*|there seems to be a Export::exportIkChain() method, here : https://github.com/AnimalLogic/AL_USDMaya/blob/master/lib/AL_USDMaya/AL/usdmaya/fileio/Export.cpp ( maybe no* = yes for AL_USD, but it's not exposed by default in the export interface ? ). PxrUsd as a basic UsdSkel support out-of-the-box|
||**Transforms**| yes| yes||
|**Importing**||||
||**Lights**|no |yes*| Pixar only supports RfM Lights |
||**proxyShape** |yes| yes |Both have this implimented in similar ways ( at least from USD -> Hydra -> Viewport )|
||Supported Animation| yes| yes| through time1 connection|
||Load Part of Stage |yes| yes| yes, each plug-in has it's own way of importing 'fast' (ie: proxyShape), then diving down into transforms to interact with / update the usd stage (visible in the viewport )|
||Load / Switch Variants| yes*| yes*| AL_USD seems to require this through a usd outliner (ie: luma), or through commands. PxrUsd has a variantKey on the proxyShape, but that does not seem to work. However the pxrUsdReferenceAssembly nodes are sub-classes of kAssembly's and do offer the representation switching|
||**Shaders** |no |yes*| Pixar only supports RfM Shaders|
|**Selecting / Editing**||||
||**Constraint Based Workflows**| yes| limited | AL_USD has bi-directional constraint support, including access to constrain to individual elements of the USD Stage. PxrUSD has limitations due to the Assembly node and not having 'connect to prim' capability (out-of-the-box, might be possible through scripting and such )|
||**Interface with Native USD** |no* |no* |not natively, but through custom commands / plugins ( pxrUsdReferenceAssembly or AL_usdmaya_Transforms or a proxyShape node to each USD Stage )|
||**Layer Management**| yes| no |(not out-of-thebox) AL_USD serializes 'live layer data' so when artist re-opens maya session it opens the usd stage, then your local edits. Once task is complete, layer can be saved out and merged back to the usd stage/file ). PxrUSD seems to require full import / export from usd -> maya nodes, which can impact Maya performnace |
||**Partial Stage Selection**|yes| yes* |This is possible through commands, and with NGRL-527 through the viewport |
||**Variant Switching to Rigs**| yes| no*| AL_USD has a schema for MayaReferences to allow to switch out to rig representations. PxrUSD, while it has slightly better UI to switch model/shading variants (mainly due to being a sub-class of an assembly node), does not have built-in support for switching to a rig variant ( though I'm sure it's possible to extend ) |
||**VP2 Selection from Stage to Outliner** |yes* |under investigation | Pilar has this working w/ AL_USD, which has extra selection management going on, and hooks into it's ProxyShapeImportTransforms command. PxrUSD selection ( back to native dag nodes of some form) might not be possible this is being investigated |
||**VP2 Selection of Stage** | no/ip |no/ip | Pilar's working on this|
