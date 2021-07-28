# Common Plug-in Commands

This read-me documents the common commands that are shared by the various
USD plug-ins made by Autodesk and Pixar.

## Common Plug-in Contents

The common parts of Maya USD plug-in provides base commands to import and
export USD files from scripts. The name of the command as seen by the
end-user will vary for each specific plug-in. In the following table,
we give an example of what the final command name would be in Maya,
in this instance for the mayaUsd plug-in.

| Class Name                     | Example Real Cmd Name    | Purpose                                |
| ------------------------------ | ------------------------ | -------------------------------------- |
| MayaUSDImportCommand           | mayaUSDImport            | Command to import from a USD file      |
| MayaUSDExportCommand           | mayaUSDExport            | Command to export into a USD file      |
| MayaUSDListShadingModesCommand | mayaUSDListShadingModes  | Command to get available shading modes |
| EditTargetCommand              | mayaUsdEditTarget        | Command to set or get the edit target  |
| LayerEditorCommand             | mayaUsdLayerEditor       | Manipulate layers                      |
| LayerEditorWindowCommand       | mayaUsdLayerEditorWindow | Open or manipulate the layer window    |

Each base command class is documented in the following sections.


## `MayaUSDImportCommand`

### Command Flags

| Long flag                     | Short flag | Type           | Default                           | Description |
| ----------------------        | ---------- | -------------- | --------------------------------- | ----------- |
| `-apiSchema`                  | `-api`     | string (multi) | none                              | Imports the given API schemas' attributes as Maya custom attributes. This only recognizes API schemas that have been applied to prims on the stage. The attributes will properly round-trip if you re-export back to USD. |
| `-chaser`                     | `-chr`     | string(multi)  | none                              | Specify the import chasers to execute as part of the export. See "Import Chasers" below. |
| `-chaserArgs`                 | `-cha`     | string[3] multi| none                              | Pass argument names and values to import chasers. Each argument to `-chaserArgs` should be a triple of the form: (`<chaser name>`, `<argument name>`, `<argument value>`). See "Import Chasers" below. |
| `-excludePrimvar`             | `-epv`     | string (multi) | none                              | Excludes the named primvar(s) from being imported as color sets or UV sets. The primvar name should be the full name without the `primvars:` namespace prefix. |
| `-file`                       | `-f`       | string         | none                              | Name of the USD being loaded |
| `-frameRange`                 | `-fr`      | float float    | none                              | The frame range of animations to import |
| `-importInstances`            | `-ii`      | bool           | true                              | Import USD instanced geometries as Maya instanced shapes. Will flatten the scene otherwise. |
| `-metadata`                   | `-md`      | string (multi) | `hidden`, `instanceable`, `kind`  | Imports the given USD metadata fields as Maya custom attributes (e.g. `USD_hidden`, `USD_kind`, etc.) if they're authored on the USD prim. The metadata will properly round-trip if you re-export back to USD. |
| `-parent`                     | `-p`       | string         | none                              | Name of the Maya scope that will be the parent of the imported data. |
| `-primPath`                   | `-pp`      | string         | none (defaultPrim)                | Name of the USD scope where traversing will being. The prim at the specified primPath (including the prim) will be imported. Specifying the pseudo-root (`/`) means you want to import everything in the file. If the passed prim path is empty, it will first try to import the defaultPrim for the rootLayer if it exists. Otherwise, it will behave as if the pseudo-root was passed in. |
| `-preferredMaterial`          | `-prm`     | string         | `lambert`                         | Indicate a preference towards a Maya native surface material for importers that can resolve to multiple Maya materials. Allowed values are `none` (prefer plugin nodes like pxrUsdPreviewSurface and aiStandardSurface) or one of `lambert`, `standardSurface`, `blinn`, `phong`. In displayColor shading mode, a value of `none` will default to `lambert`.
| `-readAnimData`               | `-ani`     | bool           | false                             | Read animation data from prims while importing the specified USD file. If the USD file being imported specifies `startTimeCode` and/or `endTimeCode`, Maya's MinTime and/or MaxTime will be expanded if necessary to include that frame range. **Note**: Only some types of animation are currently supported, for example: animated visibility, animated transforms, animated cameras, mesh and NURBS surface animation via blend shape deformers. Other types are not yet supported, for example: time-varying curve points, time-varying mesh points/normals, time-varying NURBS surface points |
| `-shadingMode`                | `-shd`     | string[2] multi| `useRegistry` `UsdPreviewSurface` | Ordered list of shading mode importers to try when importing materials. The search stops as soon as one valid material is found. Allowed values for the first parameter are: `none` (stop search immediately, must be used to signal no material import), `displayColor` (if there are bound materials in the USD, create corresponding Lambertian shaders and bind them to the appropriate Maya geometry nodes), `pxrRis` (attempt to reconstruct a Maya shading network from (presumed) Renderman RIS shading networks in the USD), `useRegistry` (attempt to reconstruct a Maya shading network from (presumed) UsdShade shading networks in the USD) the second item in the parameter pair is a convertMaterialFrom flag which allows specifying which one of the registered USD material sources to explore. The full list of registered USD material sources can be found via the `mayaUSDListShadingModesCommand` command. |
| `-useAsAnimationCache`        | `-uac`     | bool           | false                             | Imports geometry prims with time-sampled point data using a point-based deformer node that references the imported USD file. When this parameter is enabled, `MayaUSDImportCommand` will create a `pxrUsdStageNode` for the USD file that is being imported. Then for each geometry prim being imported that has time-sampled points, a `pxrUsdPointBasedDeformerNode` will be created that reads the points for that prim from USD and uses them to deform the imported Maya geometry. This provides better import and playback performance when importing time-sampled geometry from USD, and it should reduce the weight of the resulting Maya scene since it will bypass creating blend shape deformers with per-object, per-time sample geometry. Only point data from the geometry prim will be computed by the deformer from the referenced USD. Transform data from the geometry prim will still be imported into native Maya form on the Maya shape's transform node. **Note**: This means that a link is created between the resulting Maya scene and the USD file that was imported. With this parameter off (as is the default), the USD file that was imported can be freely changed or deleted post-import. With the parameter on, however, the Maya scene will have a dependency on that USD file, as well as other layers that it may reference. Currently, this functionality is only implemented for Mesh prims/Maya mesh nodes. |
| `-verbose`                    | `-v`       | noarg          | false                             | Make the command output more verbose. |
| `-variant`                    | `-var`     | string[2]      | none                              | Set variant key value pairs |
| `-importUSDZTextures`         | `-itx`     | bool           | false                             | Imports textures from USDZ archives during import to disk. Can be used in conjuction with `-importUSDZTexturesFilePath` to specify an explicit directory to write imported textures to. If not specified, requires a Maya project to be set in the current context.  |
| `-importUSDZTexturesFilePath` | `-itf`     | string         | none                              | Specifies an explicit directory to write imported textures to from a USDZ archive. Has no effect if `-importUSDZTextures` is not specified.

### Return Value

`MayaUSDImportCommand` will return an array containing the fullDagPath
of the highest prim(s) imported. This is generally the fullDagPath
that corresponds to the imported primPath but could be multiple
paths if primPath="/".

#### Import behaviours

#### Import Chasers

Import chasers are plugins that run after the initial import process and can
implement post-processing on Maya nodes that executes right after the main
import operation is complete. This can be used, for example, to implement
pipeline-specific operations and/or early prototyping of features that might
otherwise not make sense to be part of the mainline codebase.

Chasers are registered with a particular name and can be passed argument
name/value pairs in an invocation of `mayaUSDImport`. There is no "plugin
discovery" method here – the developer/user is responsible for making sure the
chaser is registered via a call to the convenience macro
`USDMAYA_DEFINE_IMPORT_CHASER_FACTORY(name, ctx)`, where `name` is the name of
the chaser being created. Unlike export chasers, import chasers also have the
ability to define `Undo` and `Redo` methods in order to allow the
`mayaUSDImport` command to remain compliant with the Maya undo stack. It's not
necessary to compile your chaser plugin together with `mayaUsdPlugin` in order
to work; you can create a completely separate maya DLL that contains the
business logic of your chaser code, and just call the aforementioned
`USDMAYA_DEFINE_IMPORT_CHASER_FACTORY` to register it, as long as the
`mayaUsdPlugin` DLL is loaded first.

A sample import chaser, `infoImportChaser.cpp`, is provided to give an example
of how to write an import chaser. All it does is read any custom layer data in
the USD file on import, and create string attributes on the nodes created and
populate them with said string attribute. Invoking it during import is as simple
as calling:

```python
cmds.mayaUSDImport(
    file='/tmp/test.usda',
    chaser=['info'])
```

As mentioned, when writing an import chaser, you also have the chance to
implement undo/redo functionality for it in a way that will remain compatible
with the Maya undo stack. While you do not have to strictly do this, it is
recommended that you keep a record of edits you have made in your chaser and
implement the necessary undo/redo functionality where possible or risk
experiencing issues (i.e. the main import created a node while your import
chaser deleted it from the scene, and then invoking an undo causes a crash since
the main plugin's `Undo` code will no longer work correctly.) It is also highly
recommended that you be very mindful of the edits you are making to the scene
graph, and how multiple import chasers might work together in unexpected ways or
have inter-dependencies.

Import chasers may also be written to parse an array of 3 string arguments for
their own purposes, similar to the Alembic export chaser example.


## `MayaUSDExportCommand`

### Command Flags

| Long flag                        | Short flag | Type             | Default             | Description |
| -------------------------------- | ---------- | ---------------- | ------------------- | ----------- |
| `-append`                        | `-a`       | bool             | false               | Appends into an existing USD file |
| `-chaser`                        | `-chr`     | string(multi)    | none                | Specify the export chasers to execute as part of the export. See "Export Chasers" below. |
| `-chaserArgs`                    | `-cha`     | string[3](multi) | none                | Pass argument names and values to export chasers. Each argument to `-chaserArgs` should be a triple of the form: (`<chaser name>`, `<argument name>`, `<argument value>`). See "Export Chasers" below. |
| `-convertMaterialsTo`            | `-cmt`     | string           | `UsdPreviewSurface` | Selects how to convert materials on export. The default value `UsdPreviewSurface` will export to a UsdPreviewSurface shading network. A plugin mechanism allows more conversions to be registered. Use the `mayaUSDListShadingModesCommand` command to explore the possible options. |
| `-compatibility`                 | `-com`     | string           | none                | Specifies a compatibility profile when exporting the USD file. The compatibility profile may limit features in the exported USD file so that it is compatible with the limitations or requirements of third-party applications. Currently, there are only two profiles: `none` - Standard export with no compatibility options, `appleArKit` - Ensures that exported usdz packages are compatible with Apple's implementation (as of ARKit 2/iOS 12/macOS Mojave). Packages referencing multiple layers will be flattened into a single layer, and the first layer will have the extension `.usdc`. This compatibility profile only applies when exporting usdz packages; if you enable this profile and don't specify a file extension in the `-file` flag, the `.usdz` extension will be used instead. |
| `-defaultCameras`                | `-dc`      | noarg            | false               | Export the four Maya default cameras |
| `-defaultMeshScheme`             | `-dms`     | string           | `catmullClark`      | Sets the default subdivision scheme for exported Maya meshes, if the `USD_subdivisionScheme` attribute is not present on the Mesh. Valid values are: `none`, `catmullClark`, `loop`, `bilinear` |
| `-exportDisplayColor`            | `-dsp`     | bool             | false               | Export display color |
| `-defaultUSDFormat`              | `-duf`     | string           | `usdc`              | The exported USD file format, can be `usdc` for binary format or `usda` for ASCII format. |
| `-exportBlendShapes`             | `-ebs`     | bool             | false               | Enable or disable export of blend shapes |
| `-exportCollectionBasedBindings` | `-cbb`     | bool             | false               | Enable or disable export of collection-based material assigments. If this option is enabled, export of material collections (`-mcs`) is also enabled, which causes collections representing sets of geometry with the same material binding to be exported. Materials are bound to the created collections on the prim at `materialCollectionsPath` (specfied via the `-mcp` option). Direct (or per-gprim) bindings are not authored when collection-based bindings are enabled. |
| `-exportColorSets`               | `-cls`     | bool             | true                | Enable or disable the export of color sets |
| `-exportInstances`               | `-ein`     | bool             | true                | Enable or disable the export of instances |
| `-exportReferenceObjects`        | `-ero`     | bool             | false               | Whether to export reference objects for meshes. The reference object's points are exported as a primvar on the mesh object; the primvar name is determined by querying `UsdUtilsGetPrefName()`, which defaults to `pref`. |
| `-exportRefsAsInstanceable`      | `-eri`     | bool             | false               | Will cause all references created by USD reference assembly nodes or explicitly tagged reference nodes to be set to be instanceable (`UsdPrim::SetInstanceable(true)`). |
| `-exportRoots`                   | `-ert`     | string           | none                | Multi-flag that allows export of any DAG subtree without including parents |
| `-exportSkels`                   | `-skl`     | string           | none                | Determines how to export skeletons. Valid values are: `none` - No skeleton are exported, `auto` - All skeletons will be exported, SkelRoots may be created, `explicit` - only those under SkelRoots |
| `-exportSkin`                    | `-skn`     | string           | none                | Determines how to export skinClusters via the UsdSkel schema. On any mesh where skin bindings are exported, the geometry data is the pre-deformation data. On any mesh where skin bindings are not exported, the geometry data is the final (post-deformation) data. Valid values are: `none` - No skinClusters are exported, `auto` - All skinClusters will be exported for non-root prims. The exporter errors on skinClusters on any root prims. The rootmost prim containing any skinned mesh will automatically be promoted into a SkelRoot, e.g. if `</Model/Mesh>` has skinning, then `</Model>` will be promoted to a SkelRoot, `explicit` - Only skinClusters under explicitly-tagged SkelRoot prims will be exported. The exporter errors if there are nested SkelRoots. To explicitly tag a prim as a SkelRoot, specify a `USD_typeName`attribute on a Maya node. |
| `-exportUVs`                     | `-uvs`     | bool             | true                | Enable or disable the export of UV sets |
| `-exportVisibility`              | `-vis`     | bool             | true                | Export any state and animation on Maya `visibility` attributes |
| `-exportMaterialCollections`     | `-mcs`     | bool             | false               | Create collections representing sets of Maya geometry with the same material binding. These collections are created in the `material:` namespace on the prim at the specified `materialCollectionsPath` (see export option `-mcp`). These collections are encoded using the UsdCollectionAPI schema and are authored compactly using the API `UsdUtilsCreateCollections()`. |
| `-eulerFilter`                   | `-ef`      | bool             | false               | Exports the euler angle filtering that was performed in Maya |
| `-filterTypes`                   | `-ft`      | string (multi)   | none                | Maya type names to exclude when exporting. If a type is excluded, all inherited types are also excluded, e.g. excluding `surfaceShape` will exclude `mesh` as well. When a node is excluded based on its type name, its subtree hierarchy will be pruned from the export, and its descendants will not be exported. |
| `-file`                          | `-f`       | string           |                     | The name of the file being exported. The file format used for export is determined by the extension: `(none)`: By default, adds `.usd` extension and uses USD's crate (binary) format, `.usd`: usdc (binary) format, `.usda`: usda (ASCII), format, `.usdc`: usdc (binary) format, `.usdz`: usdz (packaged) format. This will also package asset dependencies, such as textures and other layers, into the usdz package. See `-compatibility` flag for more details. |
| `-frameRange`                    | `-fr`      | double[2]        | `[1, 1]`            | Sets the first and last frame for an anim export (inclusive). |
| `-frameSample`                   | `-fs`      | double (multi)   | `0.0`               | Specifies sample times used to multi-sample frames during animation export, where `0.0` refers to the current time sample. **This is an advanced option**; chances are, you probably want to set the `frameStride` parameter instead. But if you really do need fine-grained control on multi-sampling frames, see "Frame Samples" below. |
| `-frameStride`                   | `-fst`     | double           | `1.0`               | Specifies the increment between frames during animation export, e.g. a stride of `0.5` will give you twice as many time samples, whereas a stride of `2.0` will only give you time samples every other frame. The frame stride is computed before the frame samples are taken into account. **Note**: Depending on the frame stride, the last frame of the frame range may be skipped. For example, if your frame range is `[1.0, 3.0]` but you specify a stride of `0.3`, then the time samples in your USD file will be `1.0, 1.3, 1.6, 1.9, 2.2, 2.5, 2.8`, skipping the last frame time (`3.0`). |
| `-ignoreWarnings`                | `-ign`     | bool             | false               | Ignore warnings, do not fail to export due to warnings |
| `-kind`                          | `-k`       | string           | none                | Specifies the required USD kind for *root prims* in the scene. (Does not affect kind for non-root prims.) If this flag is non-empty, then the specified kind will be set on any root prims in the scene without a `USD_kind` attribute (see the "Maya Custom Attributes" table below). Furthermore, if there are any root prims in the scene that do have a `USD_kind` attribute, then their `USD_kind` values will be validated to ensure they are derived from the kind specified by the `-kind` flag. For example, if the `-kind` flag is set to `group` and a root prim has `USD_kind=assembly`, then this is allowed because `assembly` derives from `group`. However, if the root prim has `USD_kind=subcomponent` instead, then `MayaUSDExportCommand` would stop with an error, since `subcomponent` does not derive from `group`. The validation behavior understands custom kinds that are registered using the USD kind registry, in addition to the built-in kinds. |
| `-materialCollectionsPath`       | `-mcp`     | string           | none                | Path to the prim where material collections must be exported. |
| `-melPerFrameCallback`           | `-mfc`     | string           | none                | Mel function called after each frame is exported |
| `-melPostCallback`               | `-mpc`     | string           | none                | Mel function called when the export is done |
| `-materialsScopeName`            | `-msn`     | string           | `Looks`             | Materials Scope Name |
| `-mergeTransformAndShape`        | `-mt`      | bool             | true                | Combine Maya transform and shape into a single USD prim that has transform and geometry, for all "geometric primitives" (gprims). This results in smaller and faster scenes. Gprims will be "unpacked" back into transform and shape nodes when imported into Maya from USD. |
| `-normalizeNurbs`                | `-nnu`     | bool             | false               | When setm the UV coordinates of nurbs are normalized to be between zero and one. |
| `-pythonPerFrameCallback`        | `-pfc`     | string           | none                | Python function called after each frame is exported |
| `-pythonPostCallback`            | `-ppc`     | string           | none                | Python function called when the export is done |
| `-parentScope`                   | `-psc`     | string           | none                | Name of the USD scope that is the parent of the exported data |
| `-renderableOnly`                | `-ro`      | noarg            |                     | When set, only renderable prims are exported to USD. |
| `-renderLayerMode`               | `-rlm`     | string           | defaultLayer        | Specify which render layer(s) to use during export. Valid values are: `defaultLayer`: Makes the default render layer the current render layer before exporting, then switches back after. No layer switching is done if the default render layer is already the current render layer, `currentLayer`: The current render layer is used for export and no layer switching is done, `modelingVariant`: Generates a variant in the `modelingVariant` variantSet for each render layer in the scene. The default render layer is made the default variant selection. |
| `-shadingMode`                   | `-shd`     | string           | `displayColor`      | Set the shading schema to use. Valid values are: `none`: export no shading data to the USD, `displayColor`: unless there is a colorset named `displayColor` on a Mesh, export the diffuse color of its bound shader as `displayColor` primvar on the USD Mesh, `pxrRis`: export the authored Maya shading networks, applying the same translations applied by RenderMan for Maya to the shader types, `useRegistry`: Use a registry based to export the Maya shading network to an equivalent UsdShade network. |
| `-selection`                     | `-sl`      | noarg            | false               | When set, only selected nodes (and their descendants) will be exported |
| `-stripNamespaces`               | `-sn`      | bool             | false               | Remove namespaces during export. By default, namespaces are exported to the USD file in the following format: nameSpaceExample_pPlatonic1 |
| `-staticSingleSample`            | `-sss`     | bool             | false               | Converts animated values with a single time sample to be static instead |
| `-geomSidedness`                   | `-gs`     | string           | derived                | Determines how geometry sidedness is defined. Valid values are: `derived` - Value is taken from the shapes doubleSided attribute, `single` - Export single sided, `double` - Export double sided |

| `-verbose`                       | `-v`       | noarg            | false               | Make the command output more verbose |

#### Frame Samples

The frame samples are computed *after* the frame stride is taken
into account. If any of your frame samples falls outside the open
interval `(-frameStride, +frameStride)`, a warning will be issued,
but export will proceed normally.

**Note**: If you have frame samples > 0.0, additional frames may
          be generated outside your frame range.

Examples:

| frameRange   | frameStride | frameSample    | Time samples in exported USD file                |
| ------------ | ----------- | -------------- | ------------------------------------------------ |
| [1, 3]       | 1.0         | -0.1, 0.2      | 0.9, 1.2, 1.9, 2.2, 2.9, 3.2                     |
| [1, 3]       | 1.0         | -0.1, 0.0, 0.2 | 0.9, 1.0, 1.2, 1.9, 2.0, 2.2, 2.9, 3.0, 3.2      |
| [1, 3]       | 2.0         | -0.1, 0.2      | 0.9, 1.2, 2.9, 3.2                               |
| [1, 3]       | 0.5         | -0.1, 0.2      | 0.9, 1.2, 1.4, 1.7, 1.9, 2.2, 2.4, 2.7, 2.9, 3.2 |
| [1, 3]       | 2.0         | -3.0, 3.0      | -2.0, -1.0, 4.0, 5.0                             |

The last example is quite strange, and a warning will be issued.
This is how it will be processed:

* The current time starts at 1.0. We evaluate frame samples,
  giving us time samples -2.0 and 4.0.
* Then we advance the current time by the stride (2.0),
  making the new current time 3.0. We evaluate frame samples,
  giving us time samples -1.0 and 5.0.
* The time samples are sorted before exporting, so they are
  evaluated in the order -2.0, -1.0, 4.0, 5.0.


### Export Behaviors

#### Model Hierarchy and Kind

`MayaUSDExportCommand` currently attempts to make each root prim in the
exported USD file a valid model, and may author a computed `kind`
on one or more prims to do so. In the future, we plan to support
annotating desired `kind` in the Maya scenegraph, and possibly make
further `kind`/model inference optional. The current behavior is:

* We initially author the kinds on prims based on the `-kind` flag
  in `MayaUSDExportCommand` (see the `MayaUSDExportCommand` flags above) or the
  `USD_kind` attribute on individual Maya nodes (see the "Maya
  Custom Attributes" table below).

* For each root prim in the scene, we *validate* the kind if it
  has been specified. Otherwise, we *compute* a kind for the root prim:
  * If the root prim is specified to be an `assembly` (or type derived
    from `assembly`), then we check to make sure that Maya has not
    created any gprims (UsdGeomGprim-derived prim-type) below the
    root prim. If there are any gprims below the root prim, then
    `MayaUSDExportCommand` will halt with an error.
  * If the root prim has no kind, then we will compute a value
    for the kind to ensure that all root prims have a kind.
    We determine whether a root prim represents a `component`
    (i.e. leaf model) or `assembly` (i.e. aggregate model of
    other models, by reference) by determining whether Maya
    directly created any gprims (UsdGeomGprim-derived prim-type).
    If Maya has created gprims, model is a `component`, else
    it is an `assembly`.

* Once we have validated or set the kind on each root prim, we go
  through each root prim's sub-hierarchy to make sure that it is
  a valid model:
  * If model is a `component`, but also has references to other
    models contained (nested) within it, override the `kind` of
    the nested references to `subcomponent`, since `component`
    models cannot contain other models according to USD's
    model-hierarchy rules.
  * Else, if model is an `assembly`, ensure that all the intermediate
    prims between the root and the locations at which other assets
    are referenced in get `kind=group` so that there is a contiguous
    hierarchy of models from the root down to the "leaf" model references.


#### UV Set Names

Currently, for Mesh export (and similarly for NurbsPatch, also),
the UV set names will be renamed st, st1, st2,... based on ordering of the
primitive. The original name will be preserved in custom data for roundtripping.


### Custom Attributes and Tagging for USD

`MayaUSDExportCommand` has several ways to export user-defined "blind data"
(such as custom primvars) and USD-specific data (such as mesh
subdivision scheme).

#### Maya Custom Attributes that Guide USD Export

We reserve the prefix `USD_` for attributes that will be used by
the USD exporter. You can author most of these attributes in Maya
using the Python "adaptor" helper; see the section on
"Registered Metadata and Schema Attributes" below.

| Maya Custom Attribute Name                | Type   | Value           | Description |
| ----------------------------------------- | ------ | --------------- | ----------- |
| **All DAG nodes (internal for UsdMaya)**: Internal to Maya; cannot be set using adaptors. |
| `USD_UserExportedAttributesJson`          | string | JSON dictionary | Specifies additional arbitrary attributes on the Maya DAG node to be exported as USD attributes. You probably don't want to author this directly (but can if you need to). See "Specifying Arbitrary Attributes for Export" below. |
| **All DAG nodes (USD metadata)**: These attributes get converted into USD metadata. You can use UsdMaya adaptors to author them. Note that these are only the most common metadata keys; you can export any registered metadata key using UsdMaya adaptors.|
| `USD_hidden`                              | bool   | true/false      | Equivalent to calling `UsdPrim::SetHidden()` for the exported prim. **Note**: in USD, "hidden" is a GUI property intended to be meaningful only to hierarchy browsers, as a complexity management feature indicating whether prims and properties so-tagged should be displayed, similar to how Maya allows you to show/hide shape nodes in the Outliner. Maya's "Hide Selection" GUI operation will cause `UsdGeomImageable::CreateVisibilityAttr("invisible")` to be authored on export if the `-exportVisibility` option is specified to `MayaUSDExportCommand`. |
| `USD_instanceable`                        | bool   | true/false      | Equivalent to calling `UsdPrim::SetInstanceable()` with the given value for the exported prim corresponding to the node on which the attribute is authored, overriding the fallback behavior specified via the `-exportRefsAsInstanceable` export option. |
| `USD_kind`                                | string | e.g. `component`, `assembly`, or any other custom kind | Equivalent to calling `UsdModelAPI::SetKind()` with the given value for the exported prim corresponding to the node on which the attribute is authored. Note that setting the USD kind on root prims may trigger some additional model hierarchy validation. Please see the "Model Hierarchy and kind" section above. |
| `USD_typeName`                            | string | e.g. `SkelRoot` or any other USD type name | Equivalent to calling `UsdPrim::SetTypeName()` with the given value for the exported prim corresponding to the node on which the attribute is authored. |
| **All DAG nodes (UsdGeomImageable attributes)**: These attributes get converted into attributes of the UsdGeomImageable schema. You can use UsdMaya adaptors to author them. Note that these are only the common imageable attributes; you can export any known schema attribute using UsdMaya adaptors. |
| `USD_ATTR_purpose`                        | string | `default`, `render`, `proxy`, `guide` | Directly corresponds to UsdGeomImageable's purpose attribute for specifying context-sensitive and selectable scenegraph visibility. This attribute will be populated from an imported USD scene wherever it is explicitly authored, and wherever authored on a Maya dag node, will be exported to USD. |
| **Mesh nodes (internal for UsdMaya)**: Internal to Maya; cannot be set using adaptors. |
| `USD_EmitNormals`                         | bool   | true/false      | UsdMaya uses this attribute to determine if mesh normals should be emitted; by default, without the tag, UsdMaya won't export mesh normals to USD. **Note**: Currently Maya reads/writes face varying normals. This is only valid when the mesh's subdivision scheme is `none` (regular poly mesh), and is ignored otherwise. |
| **Mesh nodes (UsdGeomMesh attributes)**: These attributes get converted into attributes of the UsdGeomMesh schema. You can use UsdMaya adaptors to author them. Note that these are only the common mesh attributes; you can export any known schema attribute using UsdMaya adaptors. |
| `USD_ATTR_faceVaryingLinearInterpolation` | string | `none`, `cornersOnly`, `cornersPlus1`, `cornersPlus2`, `boundaries`, `all` | Determines the Face-Varying Interpolation rule. Used for texture mapping/shading purpose. Defaults to `cornersPlus1`. See the [OpenSubdiv documentation](http://graphics.pixar.com/opensubdiv/docs/subdivision_surfaces.html#face-varying-interpolation-rules) for more detail. |
| `USD_ATTR_interpolateBoundary`            | string | `none`, `edgeAndCorner`, `edgeOnly` | Determines the Boundary Interpolation rule. Valid for Catmull-Clark and Loop subdivision surfaces. Defaults to `edgeAndCorner`. |
| `USD_ATTR_subdivisionScheme`              | string | `none`, `bilinear`, `catmullClark`, `loop` | Determines the Mesh subdivision scheme. Default can be configured using the `-defaultMeshScheme` export option for meshes without `USD_ATTR_subdivisionScheme` manually specified; we currently default to `catmullClark`. |


#### Export Chasers (Advanced)

Export chasers are plugins that run as part of the export and can
implement prim post-processing that executes immediately after prims
are written (and/or after animation is written to a prim in time-based
exports). Chasers are registered with a particular name and can be
passed argument name/value pairs in an invocation of a concrete
`MayaUSDExportCommand` command. There is no "plugin discovery" method
here – the developer/user is responsible for making sure the chaser is
registered.

For example the pxr plug-in provides one such chaser plugin called
`AlembicChaser` to try to make integrating USD into Alembic-heavy
pipelines easier. One of its features is that it can export all Maya
attributes whose name matches a particular prefix (e.g. `ABC_`) as USD
attributes by using its `attrprefix` argument. Here is an example of
what that call to `mayaUSDExport` might look like:

```python
cmds.loadPlugin('pxrUsd')
cmds.mayaUSDExport(
    file=usdFilePath,
    chaser=['alembic'],
    chaserArgs=[
       ('alembic', 'attrprefix', 'ABC_'),
    ])
```

The export chasers to run are passed by name to the `-chaser` option,
and then an argument to a chaser is passed as a string triple to the
`-chaserArgs` option. Each chaser argument triple should be composed
of the name of the chaser to receive the argument, the name of the
argument, and the argument's value.


##### Alembic Chaser Arguments

| Argument        | Format                                                  | Description |
| --------------- | ------------------------------------------------------- | ----------- |
| `attrprefix`    | `mayaPrefix1[=usdPrefix1],mayaPrefix2[=usdPrefix2],...` | Exports any Maya custom attribute that begins with `mayaPrefix1`, `mayaPrefix2`, ... as a USD attribute. `usdPrefix#` specifies the substitution for `mayaPrefix#` when exporting the attribute to USD. The `usdPrefix` can contain namespaces, denoted by colons after the namespace names. It can also be empty after the equals sign to indicate no prefix. If the entire `[=usdPrefix]` component (including equals sign) is omitted for some `mayaPrefix`, the `usdPrefix` is assumed to be `userProperties:` by default. This is similar to the `userattrprefix` argument in Maya's `AbcExport` command, where the `userProperties:` namespace is USD's counterpart to the `userProperties` compound. See "Alembic Chaser `attrprefix`" below for examples. |
| `primvarprefix` | `mayaPrefix1[=usdPrefix1],mayaPrefix2[=usdPrefix2],...` | Exports any Maya custom attribute that begins with `mayaPrefix1`, `mayaPrefix2`, ... as a USD primvar. `usdPrefix#` specifies the substitution for `mayaPrefix#` when exporting the attribute to USD. The `usdPrefix` can contain namespaces, denoted by colons after the namespace names. If the `usdPrefix` is empty after the equals sign, or if the entire `[=usdPrefix]` term is omitted completely, then the `usdPrefix` is assumed to be blank ("") by default. This is similar to the `attrprefix` argument in Maya's `AbcExport` command, where the `primvars:` namespace is USD's counterpart to the `arbGeomParams` compound. See "Alembic Chaser `primvarprefix`" below for examples. |

###### Alembic Chaser `attrprefix`

If `attrprefix` = `ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:`,
then the following custom attributes will be exported:

| Maya name       | USD name                   |
| --------------- | -------------------------- |
| `ABC_attrName`  | `userProperties:attrName`  |
| `ABC2_attrName` | `customPrefix_attrName`    |
| `ABC3_attrName` | `attrName`                 |
| `ABC4_attrName` | `customNamespace:attrName` |

Example `mayaUSDExport` invocation with `attrprefix` option:

```python
cmds.loadPlugin('pxrUsd')
cmds.mayaUSDExport(
    file=usdFilePath,
    chaser=['alembic'],
    chaserArgs=[
       ('alembic', 'attrprefix', 'ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:'),
    ])
```


###### Alembic Chaser `primvarprefix`

If `primvarprefix` = `ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:`,
then the following custom attributes will be exported:

| Maya name       | USD name (with `primvars:` namespace) |
| --------------- | ------------------------------------- |
| `ABC_attrName`  | `primvars:attrName`                   |
| `ABC2_attrName` | `primvars:customPrefix_attrName`      |
| `ABC3_attrName` | `primvars:attrName`                   |
| `ABC4_attrName` | `primvars:customNamespace:attrName`   |

The interpolation of the primvar is based on the `_AbcGeomScope` attribute
corresponding to an attribute (e.g. `myCustomAttr_AbcGeomScope` for
`myCustomAttr`). Supported interpolations are `fvr` (face-varying),
`uni` (uniform), `vtx` (vertex), and `con` (constant). If there's no
sidecar `_AbcGeomScope`, the primvar gets exported without an authored
interpolation; the current fallback interpolation in USD is constant
interpolation.

The type of the primvar is automatically deduced from the type of
the Maya attribute. (We currently ignore `_AbcType` hint attributes.)

Example `mayaUSDExport` invocation with `primvarprefix` option:

```python
cmds.loadPlugin('pxrUsd')
cmds.mayaUSDExport(
    file=usdFilePath,
    chaser=['alembic'],
    chaserArgs=[
       ('alembic', 'primvarprefix', 'ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:'),
    ])
```


## Setting Site-Specific Defaults for MayaUSDImportCommand/MayaUSDExportCommand

Suppose that at your site you always want to export with the flags
`-exportMaterialCollections` and `-chaser alembic` and `-chaserArgs ...`,
even when exporting using the "File > Export All" menu item, where
you wouldn't normally be able to set some more advanced options like
`-chaser` that are only available via the specific command, for example
the `mayaUSDExport` command.

You can configure these options as the "site-specific" defaults for
your installation of the plug-in, for example the Autodesk Maya USD
plugins by creating a plugin and adding some information to its
`plugInfo.json` file (see the USD Plug library documentation for more
information).

For example, your `plugInfo.json` would contain these keys if you
wanted the default flags mentioned above:

```javascript
{
    "UsdMaya": {
        "mayaUSDExport": {
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

This also works for the `MayaUSDImportCommand` base command, for example
in the `mayaUSDImport` command and the "File > Import" menu item; use
the `mayaUSDImport` key in the `plugInfo.json` file to configure your
site-specific defaults.


## `mayaUSDListShadingModesCommand`

The purpose of this command is to translate between nice-names, internal
names and annotations for various elements passed to the other commands.

### Command Flags

| Long flag              | Short flag | Type           | Description |
| ---------------------- | ---------- | -------------- | ----------- |
| `-export`              | `-ex`      | noarg          | Retrieve the list of export shading mode nice names. |
| `-exportOptions`       | `-eo`      | string         | Retrieve the names necessary to be passed to the `shadingMode` and `convertMaterialsTo` flags of the export command. |
| `-exportAnnotation`    | `-ea`      | string         | Retrieve the description of the export shading mode option |
| `-findExportName`      | `-fen`     | string         | Retrieve the nice name of an export shading mode |
| `-findImportName`      | `-fin`     | string         | Retrieve the nice name of an import shading mode |
| `-import`              | `-im`      | noarg          | Retrieve the list of import shading mode nice names. |
| `-importOptions`       | `-io`      | string         | Retrieve the a pair of names that completely define a shading mode, as used by the import `shadingMode` option |
| `-importAnnotation`    | `-ia`      | string         | Retrieve the description of the import shading mode option |


## `EditTargetCommand`

The purpose of this command is to set the current edit target.

### Command Flags

| Long flag      | Short flag | Type           | Description |
| -------------- | ---------- | -------------- | ----------- |
| `-edit`        | `-e`       | noarg          | Set the current edit target, chosen by naming it in the `-editTarget` flag |
| `-query`       | `-q`       | noarg          | Retrieve the current edit target |
| `-editTarget`  | `-et`      | string         | The name of the target to set with the `-edit` flag |


## `LayerEditorCommand`

The purpose of this command is edit layers.

### Command Flags

| Long flag           | Short flag | Type           | Description                              |
| ------------------- | ---------- | -------------- | ---------------------------------------- |
| `-edit`             | `-e`       | noarg          | Edit various element of a layer          |
| `-clear`            | `-cl`      | noarg          | Erase everything in a layer              |
| `-discardEdits`     | `-de`      | noarg          | Discard changes made on a layer          |
| `-addAnonymous`     | `-aa`      | string         | Add an anonynous layer at the top of the stack, returns it |
| `-insertSubPath`    | `-is`      | int string     | Insert a sub layer path at a given index |
| `-muteLayer`        | `-mt`      | bool string    | Mute or unmute the named layer           |
| `-replaceSubPath`   | `-rp`      | string string  | Replaces a path in the layer stack       |
| `-removeSubPath`    | `-rs`      | int string     | Remove a sub layer at a given index      |


## `LayerEditorWindowCommand`

The purpose of this command is to control the layer editor window.

### Command Flags

| Long flag               | Short flag | Description                                   |
| ----------------------- | ---------- | --------------------------------------------- |
| `-edit`                 | `-e`       | Edit various aspects of the editor window     |
| `-query`                | `-q`       | Retrieve various aspects of the editor window |
| `-addAnonymousSublayer` | `-aa`      | Add an anonynous layer at the top of the stack, returns it |
| `-addParentLayer`       | `-ap`      | Add a parent layer                            |
| `-loadSubLayers`        | `-lo`      | Open a dialog to load sub-layers              |
| `-removeSubLayer`       | `-rs`      | Remove sub-layers                             |
| `-clearLayer`           | `-cl`      | Erase everything in a layer                   |
| `-discardEdits`         | `-de`      | Discard changes made on a layer               |
| `-isAnonymousLayer`     | `-al`      | Query if the layer is anonymous               |
| `-isLayerDirty`         | `-dl`      | Query if the layer has been modified          |
| `-isInvalidLayer`       | `-il`      | Query if the layer is not found or invalid    |
| `-isSubLayer`           | `-su`      | Query if the layer is a sub-layer             |
| `-isIncomingLayer`      | `-in`      | Query if the layer is incoming (connection)   |
| `-layerAppearsMuted`    | `-am`      | Query if the layer or any parent is muted     |
| `-layerIsMuted`         | `-mu`      | Query if the layer itself is muted            |
| `-layerIsReadOnly`      | `-r`       | Query if the layer or any parent is read only |
| `-muteLayer`            | `-mt`      | Toggle the muting of a layer                  |
| `-layerNeedsSaving`     | `-ns`      | Query if the layer is dirty or anonymous      |
| `-printLayer`           | `-pl`      | Print the layer to the script editor output   |
| `-proxyShape`           | `-ps`      | Query the proxyShape path or sets the selected shape by its path. Takes the path as argument |
| `-reload`               | `-rl`      | Open or show the editor window                |
| `-selectionLength`      | `-se`      | Query the number of items selected            |
| `-isSessionLayer`       | `-sl`      | Query if the layer is a session layer         |
| `-selectPrimsWithSpec`  | `-sp`      | Select the prims with spec in a layer         |
| `-saveEdits`            | `-sv`      | Save the modifications                        |
