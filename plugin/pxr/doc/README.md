# Pixar Maya USD Plugins

## Configure Environment

Set the following environment variables so that Maya can find the plugin files. We'll refer to the install location of your maya-usd build with `MAYAUSD_INSTALL_ROOT`:

| Name | Meaning | Value |
| ---- | ------- | ----- |
| `MAYA_PLUG_IN_PATH` | This is a path list which Maya uses to find plugins. | `$MAYA_PLUG_IN_PATH:MAYAUSD_INSTALL_ROOT/plugin/pxr/maya/plugin/` |
| `MAYA_SCRIPT_PATH` | This is a path list which Maya uses to find Mel scripts. | `$MAYA_SCRIPT_PATH:MAYAUSD_INSTALL_ROOT/plugin/pxr/maya/lib/usd/usdMaya/resources/:MAYAUSD_INSTALL_ROOT/plugin/pxr/maya/plugin/pxrUsdPreviewSurface/resources/` |
| `PYTHONPATH` | This is a path list which Python uses to find modules. | `$PYTHONPATH:MAYAUSD_INSTALL_ROOT/plugin/pxr/lib/python` |
| `XBMLANGPATH` | This is a path list which Maya uses to find icon images. | `$XBMLANGPATH:MAYAUSD_INSTALL_ROOT/plugin/pxr/maya/lib/usd/usdMaya/resources/` |
| `PATH` | Windows Only: This is a path list which Windows uses to find all the required dlls. | `%PATH%;MAYAUSD_INSTALL_ROOT\lib;MAYAUSD_INSTALL_ROOT\plugin\pxr\maya\lib` |

---


## Loading the Maya USD plugins

To import, export, and reference USD files in Maya we provide a plugin called `pxrUsd`. The plugin can be loaded through Maya's Plug-in Manager, or by using the Mel command `loadPlugin pxrUsd` in the Command Line/Script Editor.

The plugin creates two commands: `usdImport` and `usdExport`, and will also register USD import and export as `MPxFileTranslator`s, so that USD will appear as an option in Maya's "*File > Export...*" and "*File > Import...*" menu commands.

---


## `usdImport`

### Command Flags

| Short flag | Long flag | Type | Default | Description |
| ---------- | --------- | ---- | ------- | ----------- |
| `-api` | `-apiSchema` | string (multi)| none | Imports the given API schemas' attributes as Maya custom attributes. This only recognizes API schemas that have been applied to prims on the stage. The attributes will properly round-trip if you re-export back to USD. |
| `-ar` | `-assemblyRep` | string | `Collapsed` | If the import results in the creation of assembly nodes, this value specifies the assembly representation that will be activated after creation. If empty, no representation will be activated. Valid values are: `Collapsed`, `Expanded`, `Full`, `Import`, `(empty)`. `Import`: No USD reference assembly nodes will be created, and the geometry will be imported directly. `(empty)`: The assembly is created but is left unloaded after creation. See "Importing as Assemblies" below for more detail on assembly node creation. |
| `-epv` | `-excludePrimvar` | string (multi) | none | Excludes the named primvar(s) from being imported as color sets or UV sets. The primvar name should be the full name without the `primvars:` namespace prefix. |
| `-f` | `-file` | string | none | Name of the USD being loaded |
| `-md` | `-metadata` | string (multi) | `hidden`, `instanceable`, `kind` | Imports the given USD metadata fields as Maya custom attributes (e.g. `USD_hidden`, `USD_kind`, etc.) if they're authored on the USD prim. The metadata will properly round-trip if you re-export back to USD. |
| `-p` | `-parent` | string | none | Name of the Maya scope that will be the parent of the imported data. |
| `-pp` | `-primPath` | string | none (defaultPrim) | Name of the USD scope where traversing will being. The prim at the specified primPath (including the prim) will be imported. Specifying the pseudo-root (`/`) means you want to import everything in the file. If the passed prim path is empty, it will first try to import the defaultPrim for the rootLayer if it exists. Otherwise, it will behave as if the pseudo-root was passed in. |
| `-ani` | `-readAnimData` | bool | false | Read animation data from prims while importing the specified USD file. If the USD file being imported specifies `startTimeCode` and/or `endTimeCode`, Maya's MinTime and/or MaxTime will be expanded if necessary to include that frame range. **Note**: Only some types of animation are currently supported, for example: animated visibility, animated transforms, animated cameras, mesh and NURBS surface animation via blend shape deformers. Other types are not yet supported, for example: time-varying curve points, time-varying mesh points/normals, time-varying NURBS surface points |
| `-shd` | `-shadingMode` | string[2] multi | `useRegistry` `UsdPreviewSurface` | Ordered list of shading mode importers to try when importing materials. The search stops as soon as one valid material is found. Allowed values for the first parameter are: `none` (stop search immediately, must be used to signal no material import), `displayColor` (if there are bound materials in the USD, create corresponding Lambertian shaders and bind them to the appropriate Maya geometry nodes), `pxrRis` (attempt to reconstruct a Maya shading network from (presumed) Renderman RIS shading networks in the USD), `useRegistry` (attempt to reconstruct a Maya shading network from (presumed) UsdShade shading networks in the USD) the second item in the parameter pair is a convertMaterialFrom flag which allows specifying which one of the registered USD material sources to explore. The full list of registered USD material sources can be found via the `mayaUSDListShadingModes` command. |
| `-prm` | `-preferredMaterial` | string | `lambert` | Indicate a preference towards a Maya native surface material for importers that can resolve to multiple Maya materials. Allowed values are `none` (prefer plugin nodes like pxrUsdPreviewSurface and aiStandardSurface) or one of `lambert`, `standardSurface`, `blinn`, `phong`. In displayColor shading mode, a value of `none` will default to `lambert`.
| `-uac` | `-useAsAnimationCache` | bool | false | Imports geometry prims with time-sampled point data using a point-based deformer node that references the imported USD file. When this parameter is enabled, `usdImport` will create a `pxrUsdStageNode` for the USD file that is being imported. Then for each geometry prim being imported that has time-sampled points, a `pxrUsdPointBasedDeformerNode` will be created that reads the points for that prim from USD and uses them to deform the imported Maya geometry. This provides better import and playback performance when importing time-sampled geometry from USD, and it should reduce the weight of the resulting Maya scene since it will bypass creating blend shape deformers with per-object, per-time sample geometry. Only point data from the geometry prim will be computed by the deformer from the referenced USD. Transform data from the geometry prim will still be imported into native Maya form on the Maya shape's transform node. **Note**: This means that a link is created between the resulting Maya scene and the USD file that was imported. With this parameter off (as is the default), the USD file that was imported can be freely changed or deleted post-import. With the parameter on, however, the Maya scene will have a dependency on that USD file, as well as other layers that it may reference. Currently, this functionality is only implemented for Mesh prims/Maya mesh nodes. |
| `-var` | `-variant` | string[2] | none | Set variant key value pairs |


### Return Value
`usdImport` will return an array containing the fullDagPath of the highest prim(s) imported. This is generally the fullDagPath that corresponds to the imported primPath but could be multiple paths if primPath="/".


### Import Behaviors

#### Importing as Assemblies

If you use USD to construct aggregates of other models, then `usdImport` can preserve your referencing structure as assemblies. Typically, we want to avoid reasoning about the precise referencing structure of a scene. Instead, we use model hierarchy and `kind` to decide what can be imported, and `assetInfo` to determine what to import. Your scene must have a proper model hierarchy for the importer to import a particular prim as a reference assembly: it is insufficient for the prim `</World/anim/chars/Bob>` to have `kind=component` - additionally, all of its ancestor prims must have `kind` that `IsA` `group`. We plan to make the import behavior customizable via plugin, but for now, it behaves as follows:
* If the `assemblyRep` parameter is given a value of `Import`, we do not create any USD reference assembly nodes.
* All USD assembly nodes created by an import will have their file path set to the file being imported. Their prim paths will be set accordingly based on their path in that file. This ensures that opinions in stronger layers doing the referencing will retain their effect on prims in weaker layers being referenced.
* If `kind` `IsA` `model` and we have `assetInfo['identifier']`, we generate a new USD assembly node.
* If `kind` `IsA` `model` and we do not have `assetInfo['identifier']`, we fall back to checking for any references on the prim. If a reference is found, we generate a new USD assembly node.
* In all other cases, we continue to import the scene description as if there was no kind specified.


#### Import and Playback of Animation with USD Assemblies

USD assembly and proxy nodes have a `time` attribute to support referencing USD that contains animation. When a representation of an assembly is activated, we create a connection from the assembly node's time attribute to the time attribute(s) on any/all of its immediate proxies. We do **not**, however, create a connection between Maya's global time and the assembly's time. While doing so is necessary for animation to be displayed with assemblies in `Collapsed` and `Expanded` representations, having large numbers of assembly nodes with connections to Maya's global time incurs significant overhead within Maya. As a result, these connections should be created manually if animation is desired for `Collapsed` or `Expanded` representations. The following bit of Python code provides an example of how these connections can be made:

```python
# Connecting Maya's global time to all assembly nodes in a scene 
assemblyNodes = cmds.ls(type='pxrUsdReferenceAssembly')
for assemblyNode in assemblyNodes:
    cmds.connectAttr('time1.outTime', '%s.time' % assemblyNode)
```

Currently, edits on reference models do not get imported into Maya as assembly edits.

---


## `usdExport`

### Command Flags

Short flag | Long flag | Type | Default | Description
---------- | --------- | ---- | ------- | -----------
`-a` | `-append` | bool | false | Appends into an existing USD file
`-chr` | `-chaser` | string(multi) | none | Specify the export chasers to execute as part of the export. See "Export Chasers" below.
`-cha` | `-chaserArgs` | string[3](multi) | none | Pass argument names and values to export chasers. Each argument to `-chaserArgs` should be a triple of the form: (`<chaser name>`, `<argument name>`, `<argument value>`). See "Export Chasers" below.
`-com` | `-compatibility` | string | none | Specifies a compatibility profile when exporting the USD file. The compatibility profile may limit features in the exported USD file so that it is compatible with the limitations or requirements of third-party applications. Currently, there are only two profiles: `none` - Standard export with no compatibility options, `appleArKit` - Ensures that exported usdz packages are compatible with Apple's implementation (as of ARKit 2/iOS 12/macOS Mojave). Packages referencing multiple layers will be flattened into a single layer, and the first layer will have the extension `.usdc`. This compatibility profile only applies when exporting usdz packages; if you enable this profile and don't specify a file extension in the `-file` flag, the `.usdz` extension will be used instead.
`-dc` | `-defaultCameras` | noarg | false | Export the four Maya default cameras
`-dms` | `-defaultMeshScheme` | string | `catmullClark` | Sets the default subdivision scheme for exported Maya meshes, if the `USD_subdivisionScheme` attribute is not present on the Mesh. Valid values are: `none`, `catmullClark`, `loop`, `bilinear`
`-cls` | `-exportColorSets` | bool | true | Enable or disable the export of color sets
`-ero` | `-exportReferenceObjects` | bool | false | Whether to export reference objects for meshes. The reference object's points are exported as a primvar on the mesh object; the primvar name is determined by querying `UsdUtilsGetPrefName()`, which defaults to `pref`.
`-eri` | `-exportRefsAsInstanceable` | bool | false | Will cause all references created by USD reference assembly nodes or explicitly tagged reference nodes to be set to be instanceable (`UsdPrim::SetInstanceable(true)`).
`-skn` | `-exportSkin` | string | none | Determines how to export skinClusters via the UsdSkel schema. On any mesh where skin bindings are exported, the geometry data is the pre-deformation data. On any mesh where skin bindings are not exported, the geometry data is the final (post-deformation) data. Valid values are: `none` - No skinClusters are exported, `auto` - All skinClusters will be exported for non-root prims. The exporter errors on skinClusters on any root prims. The rootmost prim containing any skinned mesh will automatically be promoted into a SkelRoot, e.g. if `</Model/Mesh>` has skinning, then `</Model>` will be promoted to a SkelRoot, `explicit` - Only skinClusters under explicitly-tagged SkelRoot prims will be exported. The exporter errors if there are nested SkelRoots. To explicitly tag a prim as a SkelRoot, specify a `USD_typeName`attribute on a Maya node.
`-uvs` | `-exportUVs` | bool | true | Enable or disable the export of UV sets
`-vis` | `-exportVisibility` | bool | true | Export any state and animation on Maya `visibility` attributes
`-mcs` | `-exportMaterialCollections` | bool | false | Create collections representing sets of Maya geometry with the same material binding. These collections are created in the `material:` namespace on the prim at the specified `materialCollectionsPath` (see export option `-mcp`). These collections are encoded using the UsdCollectionAPI schema and are authored compactly using the API `UsdUtilsCreateCollections()`.
`-ft` | `-filterTypes` | string (multi) | none | Maya type names to exclude when exporting. If a type is excluded, all inherited types are also excluded, e.g. excluding `surfaceShape` will exclude `mesh` as well. When a node is excluded based on its type name, its subtree hierarchy will be pruned from the export, and its descendants will not be exported.
`-mcp` | `-materialCollectionsPath` | string | none | Path to the prim where material collections must be exported.
`-cbb` | `-exportCollectionBasedBindings` | bool | false | Enable or disable export of collection-based material assigments. If this option is enabled, export of material collections (`-mcs`) is also enabled, which causes collections representing sets of geometry with the same material binding to be exported. Materials are bound to the created collections on the prim at `materialCollectionsPath` (specfied via the `-mcp` option). Direct (or per-gprim) bindings are not authored when collection-based bindings are enabled.
`-f` | `-file` | string |  | The name of the file being exported. The file format used for export is determined by the extension: `(none)`: By default, adds `.usd` extension and uses USD's crate (binary) format, `.usd`: usdc (binary) format, `.usda`: usda (ASCII), format, `.usdc`: usdc (binary) format, `.usdz`: usdz (packaged) format. This will also package asset dependencies, such as textures and other layers, into the usdz package. See `-compatibility` flag for more details.
`-fr` | `-frameRange` | double[2] | `[1, 1]` | Sets the first and last frame for an anim export (inclusive).
`-fs` | `-frameSample` | double (multi) | `0.0` | Specifies sample times used to multi-sample frames during animation export, where `0.0` refers to the current time sample. **This is an advanced option**; chances are, you probably want to set the `frameStride` parameter instead. But if you really do need fine-grained control on multi-sampling frames, see "Frame Samples" below.
`-ft` | `-frameStride` | double| `1.0` | Specifies the increment between frames during animation export, e.g. a stride of `0.5` will give you twice as many time samples, whereas a stride of `2.0` will only give you time samples every other frame. The frame stride is computed before the frame samples are taken into account. **Note**: Depending on the frame stride, the last frame of the frame range may be skipped. For example, if your frame range is `[1.0, 3.0]` but you specify a stride of `0.3`, then the time samples in your USD file will be `1.0, 1.3, 1.6, 1.9, 2.2, 2.5, 2.8`, skipping the last frame time (`3.0`).
`-k` | `-kind` | string | none | Specifies the required USD kind for *root prims* in the scene. (Does not affect kind for non-root prims.) If this flag is non-empty, then the specified kind will be set on any root prims in the scene without a `USD_kind` attribute (see the "Maya Custom Attributes" table below). Furthermore, if there are any root prims in the scene that do have a `USD_kind` attribute, then their `USD_kind` values will be validated to ensure they are derived from the kind specified by the `-kind` flag. For example, if the `-kind` flag is set to `group` and a root prim has `USD_kind=assembly`, then this is allowed because `assembly` derives from `group`. However, if the root prim has `USD_kind=subcomponent` instead, then `usdExport` would stop with an error, since `subcomponent` does not derive from `group`. The validation behavior understands custom kinds that are registered using the USD kind registry, in addition to the built-in kinds.
`-mt` | `-mergeTransformAndShape` | bool | true | Combine Maya transform and shape into a single USD prim that has transform and geometry, for all "geometric primitives" (gprims). This results in smaller and faster scenes. Gprims will be "unpacked" back into transform and shape nodes when imported into Maya from USD.
`-ro` | `-renderableOnly` | noarg |  | When set, only renderable prims are exported to USD.
`-rlm` | `-renderLayerMode` | string | defaultLayer | Specify which render layer(s) to use during export. Valid values are: `defaultLayer`: Makes the default render layer the current render layer before exporting, then switches back after. No layer switching is done if the default render layer is already the current render layer, `currentLayer`: The current render layer is used for export and no layer switching is done, `modelingVariant`: Generates a variant in the `modelingVariant` variantSet for each render layer in the scene. The default render layer is made the default variant selection.
`-shd` | `-shadingMode` | string | `displayColor` | Set the shading schema to use. Valid values are: `none`: export no shading data to the USD, `displayColor`: unless there is a colorset named `displayColor` on a Mesh, export the diffuse color of its bound shader as `displayColor` primvar on the USD Mesh, `pxrRis`: export the authored Maya shading networks, applying the same translations applied by RenderMan for Maya to the shader types, `useRegistry`: Use a registry based to export the Maya shading network to an equivalent UsdShade network.
`-cmt` | `-convertMaterialsTo` | string | `UsdPreviewSurface` | Selects how to convert materials on export. The default value `UsdPreviewSurface` will export to a UsdPreviewSurface shading network. A plugin mechanism allows more conversions to be registered. Use the `mayaUSDListShadingModes` command to explore the possible options.
`-sl` | `-selection` | noarg | false | When set, only selected nodes (and their descendants) will be exported

#### Frame Samples

The frame samples are computed *after* the frame stride is taken into account. If any of your frame samples falls outside the open interval `(-frameStride, +frameStride)`, a warning will be issued, but export will proceed normally. **Note**: If you have frame samples > 0.0, additional frames may be generated outside your frame range.

Examples:

| frameRange | frameStride | frameSample | Time samples in exported USD file |
| ---------- | ----------- | ----------- | --------------------------------- |
| [1, 3] | 1.0 | -0.1, 0.2 | 0.9, 1.2, 1.9, 2.2, 2.9, 3.2 |
| [1, 3] | 1.0 | -0.1, 0.0, 0.2 | 0.9, 1.0, 1.2, 1.9, 2.0, 2.2, 2.9, 3.0, 3.2 |
| [1, 3] | 2.0 | -0.1, 0.2 | 0.9, 1.2, 2.9, 3.2 |
| [1, 3] | 0.5 | -0.1, 0.2 | 0.9, 1.2, 1.4, 1.7, 1.9, 2.2, 2.4, 2.7, 2.9, 3.2 |
| [1, 3] | 2.0 | -3.0, 3.0 | -2.0, -1.0, 4.0, 5.0 |

The last example is quite strange, and a warning will be issued. This is how it will be processed:
* The current time starts at 1.0. We evaluate frame samples, giving us time samples -2.0 and 4.0.
* Then we advance the current time by the stride (2.0), making the new current time 3.0. We evaluate frame samples, giving us time samples -1.0 and 5.0.
* The time samples are sorted before exporting, so they are evaluated in the order -2.0, -1.0, 4.0, 5.0.


### Export Behaviors

#### Model Hierarchy and Kind

`usdExport` currently attempts to make each root prim in the exported USD file a valid model, and may author a computed `kind` on one or more prims to do so. In the future, we plan to support annotating desired `kind` in the Maya scenegraph, and possibly make further `kind`/model inference optional. The current behavior is:
* We initially author the kinds on prims based on the `-kind` flag in `usdExport` (see the `usdExport` flags above) or the `USD_kind` attribute on individual Maya nodes (see the "Maya Custom Attributes" table below).
* For each root prim in the scene, we *validate* the kind if it has been specified. Otherwise, we *compute* a kind for the root prim:
  * If the root prim is specified to be an `assembly` (or type derived from `assembly`), then we check to make sure that Maya has not created any gprims (UsdGeomGprim-derived prim-type) below the root prim. If there are any gprims below the root prim, then `usdExport` will halt with an error.
  * If the root prim has no kind, then we will compute a value for the kind to ensure that all root prims have a kind. We determine whether a root prim represents a `component` (i.e. leaf model) or `assembly` (i.e. aggregate model of other models, by reference) by determining whether Maya directly created any gprims (UsdGeomGprim-derived prim-type). If Maya has created gprims, model is a `component`, else it is an `assembly`.
* Once we have validated or set the kind on each root prim, we go through each root prim's sub-hierarchy to make sure that it is a valid model:
  * If model is a `component`, but also has references to other models contained (nested) within it, override the `kind` of the nested references to `subcomponent`, since `component` models cannot contain other models according to USD's model-hierarchy rules.
  * Else, if model is an `assembly`, ensure that all the intermediate prims between the root and the locations at which other assets are referenced in get `kind=group` so that there is a contiguous hierarchy of models from the root down to the "leaf" model references.


#### UV Name Translation: `map1` becomes `st`

Currently, for Mesh export (and similarly for NurbsPatch, also), we rename the UV set `map1` to `st`. `st` is the primary UV set for RenderMan, so this is very convenient for a Renderman-based pipeline. We understand this behavior is not desirable for all, and do plan to allow this translation (or none) to be specifiable. The translation is reversed on importing USD into Maya.


### Custom Attributes and Tagging for USD

`usdExport` has several ways to export user-defined "blind data" (such as custom primvars) and USD-specific data (such as mesh subdivision scheme).

#### Maya Custom Attributes that Guide USD Export

We reserve the prefix `USD_` for attributes that will be used by the USD exporter. You can author most of these attributes in Maya using the Python "adaptor" helper; see the section on "Registered Metadata and Schema Attributes" below.

| Maya Custom Attribute Name | Type | Value | Description |
| -------------------------- | ---- | ----- | ----------- |
| **All DAG nodes (internal for UsdMaya)**: Internal to Maya; cannot be set using adaptors. |
| `USD_UserExportedAttributesJson` | string | JSON dictionary | Specifies additional arbitrary attributes on the Maya DAG node to be exported as USD attributes. You probably don't want to author this directly (but can if you need to). See "Specifying Arbitrary Attributes for Export" below. |
| **All DAG nodes (USD metadata)**: These attributes get converted into USD metadata. You can use UsdMaya adaptors to author them. Note that these are only the most common metadata keys; you can export any registered metadata key using UsdMaya adaptors.|
| `USD_hidden` | bool | true/false | Equivalent to calling `UsdPrim::SetHidden()` for the exported prim. **Note**: in USD, "hidden" is a GUI property intended to be meaningful only to hierarchy browsers, as a complexity management feature indicating whether prims and properties so-tagged should be displayed, similar to how Maya allows you to show/hide shape nodes in the Outliner. Maya's "Hide Selection" GUI operation will cause `UsdGeomImageable::CreateVisibilityAttr("invisible")` to be authored on export if the `-exportVisibility` option is specified to `usdExport`. |
| `USD_instanceable` | bool | true/false | Equivalent to calling `UsdPrim::SetInstanceable()` with the given value for the exported prim corresponding to the node on which the attribute is authored, overriding the fallback behavior specified via the `-exportRefsAsInstanceable` export option. |
| `USD_kind` | string | e.g. `component`, `assembly`, or any other custom kind | Equivalent to calling `UsdModelAPI::SetKind()` with the given value for the exported prim corresponding to the node on which the attribute is authored. Note that setting the USD kind on root prims may trigger some additional model hierarchy validation. Please see the "Model Hierarchy and kind" section above. |
| `USD_typeName` | string | e.g. `SkelRoot` or any other USD type name | Equivalent to calling `UsdPrim::SetTypeName()` with the given value for the exported prim corresponding to the node on which the attribute is authored. |
| **All DAG nodes (UsdGeomImageable attributes)**: These attributes get converted into attributes of the UsdGeomImageable schema. You can use UsdMaya adaptors to author them. Note that these are only the common imageable attributes; you can export any known schema attribute using UsdMaya adaptors. |
| `USD_ATTR_purpose` | string | `default`, `render`, `proxy`, `guide` | Directly corresponds to UsdGeomImageable's purpose attribute for specifying context-sensitive and selectable scenegraph visibility. This attribute will be populated from an imported USD scene wherever it is explicitly authored, and wherever authored on a Maya dag node, will be exported to USD. |
| **Mesh nodes (internal for UsdMaya)**: Internal to Maya; cannot be set using adaptors. |
| `USD_EmitNormals` | bool | true/false | UsdMaya uses this attribute to determine if mesh normals should be emitted; by default, without the tag, UsdMaya won't export mesh normals to USD. **Note**: Currently Maya reads/writes face varying normals. This is only valid when the mesh's subdivision scheme is `none` (regular poly mesh), and is ignored otherwise. |
| **Mesh nodes (UsdGeomMesh attributes)**: These attributes get converted into attributes of the UsdGeomMesh schema. You can use UsdMaya adaptors to author them. Note that these are only the common mesh attributes; you can export any known schema attribute using UsdMaya adaptors. |
| `USD_ATTR_faceVaryingLinearInterpolation` | string | `none`, `cornersOnly`, `cornersPlus1`, `cornersPlus2`, `boundaries`, `all` | Determines the Face-Varying Interpolation rule. Used for texture mapping/shading purpose. Defaults to `cornersPlus1`. See the [OpenSubdiv documentation](http://graphics.pixar.com/opensubdiv/docs/subdivision_surfaces.html#face-varying-interpolation-rules) for more detail. |
| `USD_ATTR_interpolateBoundary` | string | `none`, `edgeAndCorner`, `edgeOnly` | Determines the Boundary Interpolation rule. Valid for Catmull-Clark and Loop subdivision surfaces. Defaults to `edgeAndCorner`. |
| `USD_ATTR_subdivisionScheme` | string | `none`, `bilinear`, `catmullClark`, `loop` | Determines the Mesh subdivision scheme. Default can be configured using the `-defaultMeshScheme` export option for meshes without `USD_ATTR_subdivisionScheme` manually specified; we currently default to `catmullClark`. |


#### Registered Metadata and Schema Attributes

You can set the above attributes by hand, but an easier way is to use the Python "adaptor" helpers provided with UsdMaya. They provide an API that is similar to working with the USD API (but note that it's not 100% the same). For example, to set the `kind` metadata on a node in Maya, you can do the following:

```python
from pxr import UsdMaya
UsdMaya.Adaptor("|my|dag|path").SetMetadata("kind", "group") # set kind=group
UsdMaya.Adaptor("|my|dag|path").ClearMetadata("kind") # clear kind
```

And you can set USD attributes for a mesh like this:

```python
from pxr import UsdGeom, UsdMaya
UsdMaya.Adaptor("|my|mesh|shape").GetSchema(UsdGeom.Mesh).CreateAttribute(UsdGeom.Tokens.subdivisionScheme).Set(UsdGeom.Tokens.loop) # set subdivisionScheme=loop
UsdMaya.Adaptor("|my|mesh|shape").GetSchema(UsdGeom.Mesh).RemoveAttribute(UsdGeom.Tokens.subdivisionScheme) # unauthor subdivisionScheme
UsdMaya.Adaptor("|my|mesh|shape").GetSchema(UsdGeom.Mesh).CreateAttribute(UsdGeom.Tokens.purpose).Set(UsdGeom.Tokens.proxy) # this works because a Mesh is an Imageable
```

#### Specifying Arbitrary Attributes for Export

Attributes on a Maya DAG node that are not part of an existing schema or are otherwise unknown to USD can still be tagged for export using the User Exported Attributes Editor UI.

Here is an example of how to launch the User Exported Attributes UI in a style that looks like Maya's channel box:

```python
from pxr.UsdMaya import userExportedAttributesUI
userExportedAttributesUI.UserExportedAttributeWidget().show(dockable=True, area='right', floating=False)
```

Attributes tagged using this UI will be added to the Maya attribute `USD_UserExportedAttributesJson` as a JSON dictionary. During export, this attribute is used to find the names of additional Maya attributes to export as USD attributes, as well as any additional metadata about how the attribute should be exported. Here is example of what the JSON in this attribute might look like after tagging:

```javascript
{
    "myMayaAttributeOne": {
    },
    "myMayaAttributeTwo": {
        "usdAttrName": "my:namespace:attributeTwo"
    },
    "attributeAsPrimvar": {
        "usdAttrType": "primvar"
    },
    "attributeAsVertexInterpPrimvar": {
        "usdAttrType": "primvar",
        "interpolation": "vertex"
    },
    "attributeAsRibAttribute": {
        "usdAttrType": "usdRi"
    }
    "doubleAttributeAsFloatAttribute": {
        "translateMayaDoubleToUsdSinglePrecision": true
    }
}
```

If the attribute metadata contains a value for `usdAttrName`, the attribute will be given that name in USD. Otherwise, the Maya attribute name will be used, and for regular USD attributes, that name will be prepended with the `userProperties:` namespace. Note that other types of attributes such as primvars and UsdRi attributes have specific namespacing schemes, so attributes of those types will follow those namespacing conventions. Maya attributes in the JSON will be processed in sorted order. Any USD attribute name collisions will be resolved by using the first attribute visited, and a warning will be issued about subsequent attribute tags for the same USD attribute. The attribute metadata can also contain a value for `usdAttrType` which can be set to `primvar` to create the attribute as a UsdGeomPrimvar, or to `usdRi` to create the attribute using `UsdRiStatements::CreateRiAttribute()`. Any other value for `usdAttrType` will result in a regular USD attribute. Attributes to be exported as primvars can also have their interpolation specified by providing a value for the `interpolation` key in the attribute metadata.

There is not always a direct mapping between Maya-native types and USD/Sdf types, and often it's desirable to intentionally use a single precision type when the extra precision is not needed to reduce size, I/O bandwidth, etc. For example, there is no native Maya attribute type to represent an array of float triples. To get an attribute with a `VtVec3fArray` type in USD, you can create a `vectorArray` data-typed attribute in Maya (which stores an array of `MVector`s, which contain doubles) and set the attribute metadata `translateMayaDoubleToUsdSinglePrecision` to true so that the data is cast to single precision on export. It will be up-cast back to double precision on re-import.


#### Export Chasers (Advanced)

Export chasers are plugins that run as part of the export and can implement prim post-processing that executes immediately after prims are written (and/or after animation is written to a prim in time-based exports). Chasers are registered with a particular name and can be passed argument name/value pairs in an invocation of `usdExport`. There is no "plugin discovery" method here – the developer/user is responsible for making sure the chaser is registered.

We provide one such chaser plugin called `AlembicChaser` to try to make integrating USD into Alembic-heavy pipelines easier. One of its features is that it can export all Maya attributes whose name matches a particular prefix (e.g. `ABC_`) as USD attributes by using its `attrprefix` argument. Here is an example of what that call to `usdExport` might look like:

```python
cmds.usdExport(
    file=usdFilePath,
    chaser=['alembic'],
    chaserArgs=[
       ('alembic', 'attrprefix', 'ABC_'),
    ])
```

The export chasers to run are passed by name to the `-chaser` option, and then an argument to a chaser is passed as a string triple to the `-chaserArgs` option. Each chaser argument triple should be composed of the name of the chaser to receive the argument, the name of the argument, and the argument's value.

##### Alembic Chaser Arguments

| Argument | Format | Description |
| -------- | ------ | ----------- |
| `attrprefix` | `mayaPrefix1[=usdPrefix1],mayaPrefix2[=usdPrefix2],...` | Exports any Maya custom attribute that begins with `mayaPrefix1`, `mayaPrefix2`, ... as a USD attribute. `usdPrefix#` specifies the substitution for `mayaPrefix#` when exporting the attribute to USD. The `usdPrefix` can contain namespaces, denoted by colons after the namespace names. It can also be empty after the equals sign to indicate no prefix. If the entire `[=usdPrefix]` component (including equals sign) is omitted for some `mayaPrefix`, the `usdPrefix` is assumed to be `userProperties:` by default. This is similar to the `userattrprefix` argument in Maya's `AbcExport` command, where the `userProperties:` namespace is USD's counterpart to the `userProperties` compound. See "Alembic Chaser `attrprefix`" below for examples. |
| `primvarprefix` | `mayaPrefix1[=usdPrefix1],mayaPrefix2[=usdPrefix2],...` | Exports any Maya custom attribute that begins with `mayaPrefix1`, `mayaPrefix2`, ... as a USD primvar. `usdPrefix#` specifies the substitution for `mayaPrefix#` when exporting the attribute to USD. The `usdPrefix` can contain namespaces, denoted by colons after the namespace names. If the `usdPrefix` is empty after the equals sign, or if the entire `[=usdPrefix]` term is omitted completely, then the `usdPrefix` is assumed to be blank ("") by default. This is similar to the `attrprefix` argument in Maya's `AbcExport` command, where the `primvars:` namespace is USD's counterpart to the `arbGeomParams` compound. See "Alembic Chaser `primvarprefix`" below for examples. |

###### Alembic Chaser `attrprefix`

If `attrprefix` = `ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:`, then the following custom attributes will be exported:

| Maya name | USD name |
| --------- | -------- |
| `ABC_attrName` | `userProperties:attrName` |
| `ABC2_attrName` | `customPrefix_attrName` |
| `ABC3_attrName` | `attrName` |
| `ABC4_attrName` | `customNamespace:attrName` |

Example `usdExport` invocation with `attrprefix` option:

```python
cmds.usdExport(
    file=usdFilePath,
    chaser=['alembic'],
    chaserArgs=[
       ('alembic', 'attrprefix', 'ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:'),
    ])
```


###### Alembic Chaser `primvarprefix`

If `primvarprefix` = `ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:`, then the following custom attributes will be exported:

| Maya name | USD name (with `primvars:` namespace) |
| --------- | ------------------------------------- |
| `ABC_attrName` | `primvars:attrName` |
| `ABC2_attrName` | `primvars:customPrefix_attrName` |
| `ABC3_attrName` | `primvars:attrName` |
| `ABC4_attrName` | `primvars:customNamespace:attrName` |

The interpolation of the primvar is based on the `_AbcGeomScope` attribute corresponding to an attribute (e.g. `myCustomAttr_AbcGeomScope` for `myCustomAttr`). Supported interpolations are `fvr` (face-varying), `uni` (uniform), `vtx` (vertex), and `con` (constant). If there's no sidecar `_AbcGeomScope`, the primvar gets exported without an authored interpolation; the current fallback interpolation in USD is constant interpolation.

The type of the primvar is automatically deduced from the type of the Maya attribute. (We currently ignore `_AbcType` hint attributes.)

Example `usdExport` invocation with `primvarprefix` option:

```python
cmds.usdExport(
    file=usdFilePath,
    chaser=['alembic'],
    chaserArgs=[
       ('alembic', 'primvarprefix', 'ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:'),
    ])
```

---


## Setting Site-Specific Defaults for usdImport/usdExport

Suppose that at your site you always want to export with the flags `-exportMaterialCollections` and `-chaser alembic` and `-chaserArgs ...`, even when exporting using the "File > Export All > pxrUsdExport" menu item, where you wouldn't normally be able to set some more advanced options like `-chaser` that are only available via the `usdExport` command. You can configure these options as the "site-specific" defaults for your installation of the Maya USD plugins by creating a plugin and adding some information to its `plugInfo.json` file (see the USD Plug library documentation for more information).

For example, your `plugInfo.json` would contain these keys if you wanted the default flags mentioned above:

```javascript
{
    "UsdMaya": {
        "UsdExport": {
            "exportMaterialCollections": true,
            "chaser": "alembic",
            "chaserArgs": [
                ["alembic", "primvarprefix", "ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:"],
                ["alembic", "attrprefix", "ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:"]
            ]
        }
    }
}
```

This also works for the `usdImport` command and the "File > Import" menu item; use the `UsdImport` key in the `plugInfo.json` file to configure your site-specific defaults.
