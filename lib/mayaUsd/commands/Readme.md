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
| SchemaCommand                  | mayaUsdSchema            | Manipulate prim schemas                |

Each base command class is documented in the following sections.


## `MayaUSDImportCommand`

### Command Flags

| Long flag                     | Short flag | Type           | Default                           | Description |
| ----------------------        | ---------- | -------------- | --------------------------------- | ----------- |
| `-apiSchema`                  | `-api`     | string (multi) | none                              | Imports the given API schemas' attributes as Maya custom attributes. This only recognizes API schemas that have been applied to prims on the stage. The attributes will properly round-trip if you re-export back to USD. |
| `-chaser`                     | `-chr`     | string(multi)  | none                              | Specify the import chasers to execute as part of the import. See "Import Chasers" below. |
| `-chaserArgs`                 | `-cha`     | string[3] multi| none                              | Pass argument names and values to import chasers. Each argument to `-chaserArgs` should be a triple of the form: (`<chaser name>`, `<argument name>`, `<argument value>`). See "Import Chasers" below. |
| `-excludePrimvar`             | `-epv`     | string (multi) | none                              | Excludes the named primvar(s) from being imported as color sets or UV sets. The primvar name should be the full name without the `primvars:` namespace prefix. |
| `-excludePrimvarNamespace`    | `-epv`     | string (multi) | none                              | Excludes the named primvar namespace(s) from being imported as color sets or UV sets. The namespance should be the name without the `primvars:` namespace prefix. Ex: `primvars:excludeNamespace:excludeMe:excludeMetoo` can be excluded by specifying `excludeNamespace` or `excludeNamespace:excludeMe`|
| `-excludeExportTypes`         | `-eet`     | string (multi) | none                              | Excludes the selected types of prims from being exported. The options are: Meshes, Cameras and Lights.|
| `-file`                       | `-f`       | string         | none                              | Name of the USD being loaded |
| `-frameRange`                 | `-fr`      | float float    | none                              | The frame range of animations to import |
| `-importInstances`            | `-ii`      | bool           | true                              | Import USD instanced geometries as Maya instanced shapes. Will flatten the scene otherwise. |
| `-jobContext`                 | `-jc`      | string (multi) | none                              | Specifies an additional import context to handle. These usually contains extra schemas, primitives, and materials that are to be imported for a specific task, a target renderer for example. |
| `-metadata`                   | `-md`      | string (multi) | `hidden`, `instanceable`, `kind`  | Imports the given USD metadata fields as Maya custom attributes (e.g. `USD_hidden`, `USD_kind`, etc.) if they're authored on the USD prim. The metadata will properly round-trip if you re-export back to USD. |
| `-parent`                     | `-p`       | string         | none                              | Name of the Maya scope that will be the parent of the imported data. |
| `-primPath`                   | `-pp`      | string         | none (defaultPrim)                | Name of the USD scope where traversing will being. The prim at the specified primPath (including the prim) will be imported. Specifying the pseudo-root (`/`) means you want to import everything in the file. If the passed prim path is empty, it will first try to import the defaultPrim for the rootLayer if it exists. Otherwise, it will behave as if the pseudo-root was passed in. |
| `-preferredMaterial`          | `-prm`     | string         | `lambert`                         | Indicate a preference towards a Maya native surface material for importers that can resolve to multiple Maya materials. Allowed values are `none` (prefer plugin nodes like pxrUsdPreviewSurface and aiStandardSurface) or one of `lambert`, `standardSurface`, `blinn`, `phong`. In displayColor shading mode, a value of `none` will default to `lambert`.
| `-primVariant`                | `-pv`      | string (multi) | none                              | Specifies variant choices to be imported on a prim. The variant specified will be the one to be imported, otherwise, the default variant will be imported. This flag is repeatable. Repeating the flag allows for extra prims and variant choices to be imported.| 
| `-readAnimData`               | `-ani`     | bool           | false                             | Read animation data from prims while importing the specified USD file. If the USD file being imported specifies `startTimeCode` and/or `endTimeCode`, Maya's MinTime and/or MaxTime will be expanded if necessary to include that frame range. **Note**: Only some types of animation are currently supported, for example: animated visibility, animated transforms, animated cameras, mesh and NURBS surface animation via blend shape deformers. Other types are not yet supported, for example: time-varying curve points, time-varying mesh points/normals, time-varying NURBS surface points |
| `-remapUVSetsTo`              | `-ruv`     | string[2](multi) | none                            | Specify UV sets by name to rename on import. Each argument should be a pair of the form: (`<from set name>`, `<to set name>`). |
| `-shadingMode`                | `-shd`     | string[2] multi| `useRegistry` `UsdPreviewSurface` | Ordered list of shading mode importers to try when importing materials. The search stops as soon as one valid material is found. Allowed values for the first parameter are: `none` (stop search immediately, must be used to signal no material import), `displayColor` (if there are bound materials in the USD, create corresponding Lambertian shaders and bind them to the appropriate Maya geometry nodes), `pxrRis` (attempt to reconstruct a Maya shading network from (presumed) Renderman RIS shading networks in the USD), `useRegistry` (attempt to reconstruct a Maya shading network from (presumed) UsdShade shading networks in the USD) the second item in the parameter pair is a convertMaterialFrom flag which allows specifying which one of the registered USD material sources to explore. The full list of registered USD material sources can be found via the `mayaUSDListShadingModesCommand` command. |
| `-useAsAnimationCache`        | `-uac`     | bool           | false                             | Imports geometry prims with time-sampled point data using a point-based deformer node that references the imported USD file. When this parameter is enabled, `MayaUSDImportCommand` will create a `pxrUsdStageNode` for the USD file that is being imported. Then for each geometry prim being imported that has time-sampled points, a `pxrUsdPointBasedDeformerNode` will be created that reads the points for that prim from USD and uses them to deform the imported Maya geometry. This provides better import and playback performance when importing time-sampled geometry from USD, and it should reduce the weight of the resulting Maya scene since it will bypass creating blend shape deformers with per-object, per-time sample geometry. Only point data from the geometry prim will be computed by the deformer from the referenced USD. Transform data from the geometry prim will still be imported into native Maya form on the Maya shape's transform node. **Note**: This means that a link is created between the resulting Maya scene and the USD file that was imported. With this parameter off (as is the default), the USD file that was imported can be freely changed or deleted post-import. With the parameter on, however, the Maya scene will have a dependency on that USD file, as well as other layers that it may reference. Currently, this functionality is only implemented for Mesh prims/Maya mesh nodes. |
| `-verbose`                    | `-v`       | noarg          | false                             | Make the command output more verbose. |
| `-variant`                    | `-var`     | string[2]      | none                              | Set variant key value pairs |
| `-importUSDZTextures`         | `-itx`     | bool           | false                             | Imports textures from USDZ archives during import to disk. Can be used in conjuction with `-importUSDZTexturesFilePath` to specify an explicit directory to write imported textures to. If not specified, requires a Maya project to be set in the current context.  |
| `-importUSDZTexturesFilePath` | `-itf`     | string         | none                              | Specifies an explicit directory to write imported textures to from a USDZ archive. Has no effect if `-importUSDZTextures` is not specified. |
| `-importRelativeTextures`     | `-rtx`     | string         | none                              | Selects how textures filenames are generated: absolute, relative, automatic or none. When automatic, the filename is relative if the source filename of the texture being imported is relative. When none, the file path is left alone, for backward compatible behavior. |
| `-upAxis`                     | `-upa`     | bool           | true                              | Enable changing axis on import. |
| `-unit`                       | `-unt`     | bool           | true                              | Enable changing units on import. |
| `-axisAndUnitMethod`          | `-aum`     | string         | rotateScale                       | Select how units and axis are handled during import. You can choose only one of the following options: *rotateScale*: Adjusts imported units and axis by rotating and scaling them to fit the scene. *addTransform*: Adds a parent transform to the imported object, aligning it with the scene while preserving original settings. *overwritePrefs*: Replaces the current scene's unit and axis preferences with those of the imported file. Note: Only one option can be selected at a time. |

### Return Value

`MayaUSDImportCommand` will return an array containing the fullDagPath
of the highest prim(s) imported. This is generally the fullDagPath
that corresponds to the imported primPath but could be multiple
paths if primPath="/".

### Repeatable flags
The `-primVar` flag can be used multiple times when used to import specific variants. In MEL, this simply means using the flag repeatedly for each prim that contains the chosen variants. In Python, the flag requires a string array to import. Importing multiple variant choices using python would for example use:

```python
cmds.mayaUSDImport(
    file='/tmp/test.usda',
    primVariant= [
("/primPath/prim_1", "VariantGroup", "Variant_1"),
("/primPath/prim_2", "VariantGroup", "Variant_2")])
```

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

PostImport function provides access to the SDF paths of USD objects and the DAG paths
of imported Maya objects. Input parameter `dagPaths` and `sdfPaths` represent the corresponding 
DAG and SDF paths for the top primitives. To access the mapping between SDF and DAG paths of 
all primitives, you can use the `GetSdfToDagMap()` function, which returns a 
`MSdfToDagMap` object with SDF path as the key and DAG path as the value. Input parameter `stage`
contains information of all primitives in the current stage. Use `TraverseAll()` to traverse the
primitives.

Import chasers may also be written to parse an array of 3 string arguments for
their own purposes, similar to the Alembic export chaser example.


## `MayaUSDExportCommand`

### Command Flags

| Long flag                        | Short flag | Type             | Default             | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
|----------------------------------|------------|------------------|---------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `-apiSchema`                     | `-api`     | string (multi)   | none                | Exports the given API schema. Requires registering schema exporters for the API.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| `-append`                        | `-a`       | bool             | false               | Appends into an existing USD file                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| `-chaser`                        | `-chr`     | string(multi)    | none                | Specify the export chasers to execute as part of the export. See "Export Chasers" below.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| `-chaserArgs`                    | `-cha`     | string[3](multi) | none                | Pass argument names and values to export chasers. Each argument to `-chaserArgs` should be a triple of the form: (`<chaser name>`, `<argument name>`, `<argument value>`). See "Export Chasers" below.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| `-convertMaterialsTo`            | `-cmt`     | string(multi)    | `UsdPreviewSurface` | Selects how to convert materials on export. The default value `UsdPreviewSurface` will export to a UsdPreviewSurface shading network. A plugin mechanism allows more conversions to be registered. Use the `mayaUSDListShadingModesCommand` command to explore the possible options.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| `-remapUVSetsTo`                 | `-ruv`     | string[2](multi) | none                | Specify UV sets by name to rename on export. Each argument should be a pair of the form: (`<from set name>`, `<to set name>`). This option takes priority over `-preserveUVSetNames` for any specified UV set.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| `-compatibility`                 | `-com`     | string           | none                | Specifies a compatibility profile when exporting the USD file. The compatibility profile may limit features in the exported USD file so that it is compatible with the limitations or requirements of third-party applications. Currently, there are only two profiles: `none` - Standard export with no compatibility options, `appleArKit` - Ensures that exported usdz packages are compatible with Apple's implementation (as of ARKit 2/iOS 12/macOS Mojave). Packages referencing multiple layers will be flattened into a single layer, and the first layer will have the extension `.usdc`. This compatibility profile only applies when exporting usdz packages; if you enable this profile and don't specify a file extension in the `-file` flag, the `.usdz` extension will be used instead.                                                                                                                                                                        |
| `-defaultCameras`                | `-dc`      | noarg            | false               | Export the four Maya default cameras                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| `-defaultMeshScheme`             | `-dms`     | string           | `catmullClark`      | Sets the default subdivision scheme for exported Maya meshes, if the `USD_ATTR_subdivisionScheme` attribute is not present on the Mesh. Valid values are: `none`, `catmullClark`, `loop`, `bilinear`                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| `-exportDisplayColor`            | `-dsp`     | bool             | false               | Export display color                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| `-jobContext`                    | `-jc`      | string (multi)   | none                | Specifies an additional export context to handle. These usually contains extra schemas, primitives, and materials that are to be exported for a specific task, a target renderer for example.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| `-defaultUSDFormat`              | `-duf`     | string           | `usdc`              | The exported USD file format, can be `usdc` for binary format or `usda` for ASCII format.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| `-exportBlendShapes`             | `-ebs`     | bool             | false               | Enable or disable export of blend shapes                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| `-exportCollectionBasedBindings` | `-cbb`     | bool             | false               | Enable or disable export of collection-based material assigments. If this option is enabled, export of material collections (`-mcs`) is also enabled, which causes collections representing sets of geometry with the same material binding to be exported. Materials are bound to the created collections on the prim at `materialCollectionsPath` (specfied via the `-mcp` option). Direct (or per-gprim) bindings are not authored when collection-based bindings are enabled.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| `-exportColorSets`               | `-cls`     | bool             | true                | Enable or disable the export of color sets                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| `-exportMaterials`               | `-mat`     | bool             | true                | Enable or disable the export of materials                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| `-exportAssignedMaterials`       | `-ama`     | bool             | true                | Export materials only if they are assigned to a mesh                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| `-legacyMaterialScope`           | `-lms`     | bool             | false               | Export materials under a scope determined using the same algorithm as MayaUSD circa 0.28                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| `-exportInstances`               | `-ein`     | bool             | true                | Enable or disable the export of instances                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| `-referenceObjectMode`           | `-rom`     | string           | `none`              | Determines how to export reference objects for meshes. The reference object's points are exported as a primvar on the mesh object; the primvar name is determined by querying `UsdUtilsGetPrefName()`, which defaults to `pref`. Valid values are: `none` - No reference objects are exported, `attributeOnly` - Only meshes set with a valid "referenceObject" attached will be exported, `defaultToMesh` - Meshes with no "referenceObject" attached will export their own points                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| `-exportRefsAsInstanceable`      | `-eri`     | bool             | false               | Will cause all references created by USD reference assembly nodes or explicitly tagged reference nodes to be set to be instanceable (`UsdPrim::SetInstanceable(true)`).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `-exportRoots`                   | `-ert`     | string           | none                | Multi-flag that allows export of any DAG subtree without including parents                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| `-exportSkels`                   | `-skl`     | string           | none                | Determines how to export skeletons. Valid values are: `none` - No skeleton are exported, `auto` - All skeletons will be exported, SkelRoots may be created, `explicit` - only those under SkelRoots                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| `-exportSkin`                    | `-skn`     | string           | none                | Determines how to export skinClusters via the UsdSkel schema. On any mesh where skin bindings are exported, the geometry data is the pre-deformation data. On any mesh where skin bindings are not exported, the geometry data is the final (post-deformation) data. Valid values are: `none` - No skinClusters are exported, `auto` - All skinClusters will be exported for non-root prims. The exporter errors on skinClusters on any root prims. The rootmost prim containing any skinned mesh will automatically be promoted into a SkelRoot, e.g. if `</Model/Mesh>` has skinning, then `</Model>` will be promoted to a SkelRoot, `explicit` - Only skinClusters under explicitly-tagged SkelRoot prims will be exported. The exporter errors if there are nested SkelRoots. To explicitly tag a prim as a SkelRoot, specify a `USD_typeName`attribute on a Maya node.                                                                                                    |
| `-exportUVs`                     | `-uvs`     | bool             | true                | Enable or disable the export of UV sets                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `-exportRelativeTextures`        | `-rtx`     | string           | automatic           | Selects how textures filenames are generated: absolute, relative or automatic. When automatic, the filename is relative if the source filename of the texture being exported is relative                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| `-exportVisibility`              | `-vis`     | bool             | true                | Export any state and animation on Maya `visibility` attributes                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| `-exportComponentTags`           | `-tag`     | bool             | true                | Export component tags                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| `-exportStagesAsRefs`            | `-sar`     | bool             | true                | Export USD stages as USD references                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| `-exportMaterialCollections`     | `-mcs`     | bool             | false               | Create collections representing sets of Maya geometry with the same material binding. These collections are created in the `material:` namespace on the prim at the specified `materialCollectionsPath` (see export option `-mcp`). These collections are encoded using the UsdCollectionAPI schema and are authored compactly using the API `UsdUtilsCreateCollections()`.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| `-eulerFilter`                   | `-ef`      | bool             | false               | Exports the euler angle filtering that was performed in Maya                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| `-filterTypes`                   | `-ft`      | string (multi)   | none                | Maya type names to exclude when exporting. If a type is excluded, all inherited types are also excluded, e.g. excluding `surfaceShape` will exclude `mesh` as well. When a node is excluded based on its type name, its subtree hierarchy will be pruned from the export, and its descendants will not be exported.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| `-file`                          | `-f`       | string           |                     | The name of the file being exported. The file format used for export is determined by the extension: `(none)`: By default, adds `.usd` extension and uses USD's crate (binary) format, `.usd`: usdc (binary) format, `.usda`: usda (ASCII), format, `.usdc`: usdc (binary) format, `.usdz`: usdz (packaged) format. This will also package asset dependencies, such as textures and other layers, into the usdz package. See `-compatibility` flag for more details.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| `-animationType`                 |`-at`       | string           | timesamples         | Sets the prefered animation type to be used on export. |
| `-frameRange`                    | `-fr`      | double[2]        | `[1, 1]`            | Sets the first and last frame for an anim export (inclusive).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| `-frameSample`                   | `-fs`      | double (multi)   | `0.0`               | Specifies sample times used to multi-sample frames during animation export, where `0.0` refers to the current time sample. **This is an advanced option**; chances are, you probably want to set the `frameStride` parameter instead. But if you really do need fine-grained control on multi-sampling frames, see "Frame Samples" below.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| `-frameStride`                   | `-fst`     | double           | `1.0`               | Specifies the increment between frames during animation export, e.g. a stride of `0.5` will give you twice as many time samples, whereas a stride of `2.0` will only give you time samples every other frame. The frame stride is computed before the frame samples are taken into account. **Note**: Depending on the frame stride, the last frame of the frame range may be skipped. For example, if your frame range is `[1.0, 3.0]` but you specify a stride of `0.3`, then the time samples in your USD file will be `1.0, 1.3, 1.6, 1.9, 2.2, 2.5, 2.8`, skipping the last frame time (`3.0`).                                                                                                                                                                                                                                                                                                                                                                            |
| `-ignoreWarnings`                | `-ign`     | bool             | false               | Ignore warnings, do not fail to export due to warnings                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| `-includeEmptyTransforms`        | `-iet`     | bool             | true                | Include Xform even if all they contain are other empty Xforms.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| `-kind`                          | `-k`       | string           | none                | Specifies the required USD kind for *root prims* in the scene. (Does not affect kind for non-root prims.) If this flag is non-empty, then the specified kind will be set on any root prims in the scene without a `USD_kind` attribute (see the "Maya Custom Attributes" table below). Furthermore, if there are any root prims in the scene that do have a `USD_kind` attribute, then their `USD_kind` values will be validated to ensure they are derived from the kind specified by the `-kind` flag. For example, if the `-kind` flag is set to `group` and a root prim has `USD_kind=assembly`, then this is allowed because `assembly` derives from `group`. However, if the root prim has `USD_kind=subcomponent` instead, then `MayaUSDExportCommand` would stop with an error, since `subcomponent` does not derive from `group`. The validation behavior understands custom kinds that are registered using the USD kind registry, in addition to the built-in kinds. |
| `-disableModelKindProcessor`     | `-dmk`     | bool             | false               | Disables the tagging of prim kinds based on the ModelKindProcessor.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| `-materialCollectionsPath`       | `-mcp`     | string           | none                | Path to the prim where material collections must be exported.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| `-melPerFrameCallback`           | `-mfc`     | string           | none                | Mel function called after each frame is exported                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| `-melPostCallback`               | `-mpc`     | string           | none                | Mel function called when the export is done                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| `-materialsScopeName`            | `-msn`     | string           | `Looks`             | Materials Scope Name                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| `-mergeTransformAndShape`        | `-mt`      | bool             | true                | Combine Maya transform and shape into a single USD prim that has transform and geometry, for all "geometric primitives" (gprims). This results in smaller and faster scenes. Gprims will be "unpacked" back into transform and shape nodes when imported into Maya from USD.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| `-writeDefaults`                 | `-wd`      | bool             | false               | Write default attribute values at the default USD time.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `-normalizeNurbs`                | `-nnu`     | bool             | false               | When setm the UV coordinates of nurbs are normalized to be between zero and one.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| `-preserveUVSetNames`            | `-puv`     | bool             | false               | Refrain from renaming UV sets additional to "map1" to "st1", "st2", etc. This option is overridden for any UV set specified in `-remapUVSetsTo`.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| `-pythonPerFrameCallback`        | `-pfc`     | string           | none                | Python function called after each frame is exported                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| `-pythonPostCallback`            | `-ppc`     | string           | none                | Python function called when the export is done                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| `-parentScope`                   | `-psc`     | string           | none                | Deprecated. Use `-rootPrim` instead. Name of the USD scope that is the parent of the exported data                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| `-defaultPrim`                   | `-dp`      | string           | none                | Set the default prim on export. If passed n empty string, then no default prim is used. If the flag is absent, the first root prim is set as the default prim.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| `-rootPrim`                      | `-rpm`     | string           | none                | Name of the USD root prim that is the parent of the exported data                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| `-rootPrimType`                  | `-rpt`     | string           | none                | Type of the USD root prim. Currently Scope and Xform are supported.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| `-renderableOnly`                | `-ro`      | noarg            |                     | When set, only renderable prims are exported to USD.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| `-renderLayerMode`               | `-rlm`     | string           | defaultLayer        | Specify which render layer(s) to use during export. Valid values are: `defaultLayer`: Makes the default render layer the current render layer before exporting, then switches back after. No layer switching is done if the default render layer is already the current render layer, `currentLayer`: The current render layer is used for export and no layer switching is done, `modelingVariant`: Generates a variant in the `modelingVariant` variantSet for each render layer in the scene. The default render layer is made the default variant selection.                                                                                                                                                                                                                                                                                                                                                                                                                |
| `-shadingMode`                   | `-shd`     | string           | `useRegistry`       | Set the shading schema to use. Valid values are: `none`: export no shading data to the USD, `pxrRis`: export the authored Maya shading networks, applying the same translations applied by RenderMan for Maya to the shader types, `useRegistry`: Use a registry based to export the Maya shading network to an equivalent UsdShade network.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| `-selection`                     | `-sl`      | noarg            | false               | When set, only selected nodes (and their descendants) will be exported                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| `-stripNamespaces`               | `-sn`      | bool             | false               | Remove namespaces during export. By default, namespaces are exported to the USD file in the following format: nameSpaceExample_pPlatonic1                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| `-hideSourceData`                | `-hsd`     | bool             | false               | Hide the Maya nodes that were used as the source.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| `-worldspace`                    | `-wsp`     | bool             | false               | Export all root prim using their full worldspace transform instead of their local transform                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| `-staticSingleSample`            | `-sss`     | bool             | false               | Converts animated values with a single time sample to be static instead                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `-geomSidedness`                 | `-gs`      | string           | derived             | Determines how geometry sidedness is defined. Valid values are: `derived` - Value is taken from the shapes doubleSided attribute, `single` - Export single sided, `double` - Export double sided                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| `-verbose`                       | `-v`       | noarg            | false               | Make the command output more verbose                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| `-customLayerData`               | `-cld`     | string[3](multi) | none                | Set the layers customLayerData metadata. Values are a list of three strings for key, value and data type                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| `-metersPerUnit`                 | `-mpu`     | double           | 0.0                 | (Evolving) Exports with the given metersPerUnit. Use with care, as only certain attributes have their dimensions converted.<br/><br/> The default value of 0 will continue to use the Maya internal units (cm) and a value of -1 will use the display units. Any other positive value will be taken as an explicit metersPerUnit value to be used.<br/><br/> Currently, the following prim types are supported: <br/><ul><li>Meshes</li><li>Transforms</li></ul>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| `-exportDistanceUnit`            | `-edu`     | bool             | false               | Use the metersPerUnit option specified above for the stage under its `metersPerUnit` attribute                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| `-upAxis`                        | `-upa`     | string           | mayaPrefs           | How the up-axis of the exported USD is controlled. "mayaPrefs" follows the current Maya Preferences. "none" does not author up-axis. "y" or "z" author that axis and convert data if the Maya preferences does not match.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| `-unit`                          | `-unt`     | string           | mayaPrefs           | How the measuring units of the exported USD is controlled. "mayaPrefs" follows the current Maya Preferences. "none" does not author up-axis. Explicit units (cm, inch, etc) author that and convert data if the Maya preferences does not match.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| `-accessibilityLabel`            | `-al`      | string           |                     | Adds an Accessibility label to the default prim. This should be short and concise.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| `-accessibilityDescription`      | `-ad`      | string           |                     | Adds an Accessibility description to the default prim. This can be more verbose and detailed.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |

Note: the -metersPerUnit and -exportDistanceUnit are one way to change the exported units, the -unit is another.
      We keep both to keep backward compatibility, but the -unit option is the favored way to handle the units.

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
| [1, 3]       | 2.0         | -3.0, 3.0      | -2.0, 0.0, 4.0, 6.0                             |

The last example is quite strange, and a warning will be issued.
This is how it will be processed:

* The current time starts at 1.0. We evaluate frame samples,
  giving us time samples -2.0 and 4.0.
* Then we advance the current time by the stride (2.0),
  making the new current time 3.0. We evaluate frame samples,
  giving us time samples 0.0 and 6.0.
* The time samples are sorted before exporting, so they are
  evaluated in the order -2.0, 0.0, 4.0, 6.0.


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

#### Material Scopes

The recommended way of selecting a global scope name for materials is to use the
materialsScopeName argument of MayaUSDExportCommand.

If that argument is not specified, we will use "mtl" as a globally defined default.
This follows the [Guidelines for Structuring USD Assets](https://wiki.aswf.io/display/WGUSD/Guidelines+for+Structuring+USD+Assets)
as defined by the USD Assets WG.

The `MAYAUSD_MATERIALS_SCOPE_NAME` environment variable can be used to change
that default on a global level. The value defined in that env var will take
precedence over any value passed in the materialsScopeName argument.

This environment variable was added in part to support legacy user of mayaUSD:
in past versions of the plugin, the material scope was named `Looks`. The change
to the new default `mtl` was done to be aligned with other tools generating USD.
As such, the environment variable provides the possibility to use the old `Looks`
name by setting `MAYAUSD_MATERIALS_SCOPE_NAME=Looks`.

Note that if a user sets both `MAYAUSD_MATERIALS_SCOPE_NAME` and the Pixar
`USD_FORCE_DEFAULT_MATERIALS_SCOPE_NAME` environment variable, then the
Pixar environment variable is used.


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
| `USD_EmitNormals`                         | bool   | true/false      | UsdMaya uses this attribute to determine if mesh normals should be emitted; by default, without the tag, UsdMaya will export mesh normals to USD. **Note**: Currently Maya reads/writes face varying normals. This is only valid when the mesh's subdivision scheme is `none` (regular poly mesh), and is ignored otherwise. |
| `USD_GeomSubsetInfo`                      | string   | JSON  | UsdMaya uses this attribute to provide roundtripping information for UsdGeomSubset data. |
| **Mesh nodes (UsdGeomMesh attributes)**: These attributes get converted into attributes of the UsdGeomMesh schema. You can use UsdMaya adaptors to author them. Note that these are only the common mesh attributes; you can export any known schema attribute using UsdMaya adaptors. |
| `USD_ATTR_faceVaryingLinearInterpolation` | string | `none`, `cornersOnly`, `cornersPlus1`, `cornersPlus2`, `boundaries`, `all` | Determines the Face-Varying Interpolation rule. Used for texture mapping/shading purpose. Defaults to `cornersPlus1`. See the [OpenSubdiv documentation](http://graphics.pixar.com/opensubdiv/docs/subdivision_surfaces.html#face-varying-interpolation-rules) for more detail. |
| `USD_ATTR_interpolateBoundary`            | string | `none`, `edgeAndCorner`, `edgeOnly` | Determines the Boundary Interpolation rule. Valid for Catmull-Clark and Loop subdivision surfaces. Defaults to `edgeAndCorner`. |
| `USD_ATTR_subdivisionScheme`              | string | `none`, `bilinear`, `catmullClark`, `loop` | Determines the Mesh subdivision scheme. Default can be configured using the `-defaultMeshScheme` export option for meshes without `USD_ATTR_subdivisionScheme` manually specified; we currently default to `catmullClark`. |


### Specifying Arbitrary Attributes for Export 

Attributes on a Maya DAG node that are not part of an existing schema or are otherwise unknown to USD can still be tagged for export.

Attributes of a node can be added to Maya attribute USD_UserExportedAttributesJson as a JSON dictionary. During export, this attribute is used to find the names of additional Maya attributes to export as USD attributes, as well as any additional metadata about how the attribute should be exported. Here is example of what the JSON in this attribute might look like after tagging:

```javascript 
{ "myMayaAttributeOne":
{ }, "myMayaAttributeTwo":

{ "usdAttrName": "my:namespace:attributeTwo" }

, "attributeAsPrimvar":

{ "usdAttrType": "primvar" }

, "attributeAsVertexInterpPrimvar":

{ "usdAttrType": "primvar", "interpolation": "vertex" }

, "attributeAsRibAttribute":

{ "usdAttrType": "usdRi" }

"doubleAttributeAsFloatAttribute":

{ "translateMayaDoubleToUsdSinglePrecision": true }
```

If the attribute metadata contains a value for usdAttrName, the attribute will be given that name in USD. Otherwise, the Maya attribute name will be used, and for regular USD attributes, that name will be prepended with the userProperties: namespace. Note that other types of attributes such as primvars and UsdRi attributes have specific namespacing schemes, so attributes of those types will follow those namespacing conventions. Maya attributes in the JSON will be processed in sorted order. Any USD attribute name collisions will be resolved by using the first attribute visited, and a warning will be issued about subsequent attribute tags for the same USD attribute. The attribute metadata can also contain a value for usdAttrType which can be set to primvar to create the attribute as a UsdGeomPrimvar, or to usdRi to create the attribute using UsdRiStatements::CreateRiAttribute(). Any other value for usdAttrType will result in a regular USD attribute. Attributes to be exported as primvars can also have their interpolation specified by providing a value for the interpolation key in the attribute metadata.

There is not always a direct mapping between Maya-native types and USD/Sdf types, and often it's desirable to intentionally use a single precision type when the extra precision is not needed to reduce size, I/O bandwidth, etc. For example, there is no native Maya attribute type to represent an array of float triples. To get an attribute with a VtVec3fArray type in USD, you can create a vectorArray data-typed attribute in Maya (which stores an array of MVectors, which contain doubles) and set the attribute metadata translateMayaDoubleToUsdSinglePrecision to true so that the data is cast to single precision on export. It will be up-cast back to double precision on re-import.


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
  "Plugins": [
    {
      "Info": {
        "UsdMaya": {
            "UsdExport": {
              "exportMaterialCollections": true,
              "chaser": ["alembic"],
              "chaserArgs": [
                  ["alembic", "primvarprefix", "ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:"],
                  ["alembic", "attrprefix", "ABC_,ABC2_=customPrefix_,ABC3_=,ABC4_=customNamespace:"]
              ]
          }
        }
      },
      "Name": "MySiteSpecificConfigPlugin",
      "Type": "resource"
    }
  ]
}
```

This also works for the `MayaUSDImportCommand` base command, for example
in the `mayaUSDImport` command and the "File > Import" menu item; use
the `UsdImport` key in the `plugInfo.json` file to configure your
site-specific defaults.


## `mayaUSDListShadingModesCommand`

The purpose of this command is to translate between nice-names, internal
names and annotations for various elements passed to the other commands.

### Command Flags

| Long flag              | Short flag | Type           | Description |
| ---------------------- | ---------- | -------------- | ----------- |
| `-useRegistryOnly`     | `-ur`      | noarg          | Modifier to limit all following options to useRegistry modes only |
| `-export`              | `-ex`      | noarg          | Retrieve the list of export shading mode nice names. |
| `-exportOptions`       | `-eo`      | string         | Retrieve the names necessary to be passed to the `shadingMode` and `convertMaterialsTo` flags of the export command. |
| `-exportAnnotation`    | `-ea`      | string         | Retrieve the description of the export shading mode option |
| `-findExportName`      | `-fen`     | string         | Retrieve the nice name of an export shading mode |
| `-findImportName`      | `-fin`     | string         | Retrieve the nice name of an import shading mode |
| `-import`              | `-im`      | noarg          | Retrieve the list of import shading mode nice names. |
| `-importOptions`       | `-io`      | string         | Retrieve the a pair of names that completely define a shading mode, as used by the import `shadingMode` option |
| `-importAnnotation`    | `-ia`      | string         | Retrieve the description of the import shading mode option |

## `mayaUSDListJobContexts`

The purpose of this command is to find the names and annotations for registered import and export job contexts.

### Command Flags

| Long flag              | Short flag | Type           | Description |
| ---------------------- | ---------- | -------------- | ----------- |
| `-export`              | `-ex`      | noarg          | Retrieve the list of export job context nice names. |
| `-exportAnnotation`    | `-ea`      | string         | Retrieve the description of the export job context option nice name passed as parameter. |
| `-exportArguments`     | `-eg`      | string         | Retrieve the export arguments affected by the export job context nice name passed as parameter |
| `-import`              | `-im`      | noarg          | Retrieve the list of import job context nice names. |
| `-importAnnotation`    | `-ia`      | string         | Retrieve the description of the import job context option nice name passed as parameter. |
| `-importArguments`     | `-ig`      | string         | Retrieve the import arguments affected by the import job context nice name passed as parameter |
| `-jobContext`          | `-jc`      | string         | Retrieve the job context name associated with a nice name. |

## `mayaUsdGetMaterialsFromRenderers`

The purpose of this command is to return the names of USD-compatible materials associated with the currently loaded renderers.  
As of this writing, the returned names are largely hard-coded, as is the selection of renderers: 

*  MaterialX
*  USDPreviewSurface
*  Arnold (if installed and loaded)

### Return Value

The command returns an array of strings in the format `Renderer Name/Material Label|MaterialIdentifier`, where the `MaterialIdentifier` is the internal name that may be used to instantiate the given material. 

## `mayaUsdGetMaterialsInStage`

The purpose of this command is to get a list of all materials contained in the USD stage of the given object.

### Arguments

Pass the path of the USD object to query for materials as an argument.

### Return Value

Returns an array of strings containing paths pointing to the materials in the given object's USD stage.

## `mayaUsdMaterialBindings`

The purpose of this command is to determine various material-related attributes of a given USD object. This includes:

*  Determining whether an object has a material assigned.
*  Determining whether an object is of a type where allowing material assignment would be sensible in a GUI context.  
Technically, USD allows binding of materials to any object type. This function however contains hard-coded filters intended to prevent material assignments in cases where it does not make sense from a usability perspective: For example, the function will return `false` for objects such as `UsdMediaSpatialAudio` or `UsdPhysicsScene`, as it may be reasonably assumed that the user does not intend to assign a material to these object types.

### Arguments

Pass the path of the USD object to query for materials as an argument.

### Command Flags

| Long flag      | Short flag | Type            | Default | Description |
| -------------- | ---------- | --------------- | --------| ----------- |
| `-canAssignMaterialToNodeType`  | `-ca` | bool | false | Determines whether the given object is of a type that accepts material assignments in a GUI context. |
| `-hasMaterialBinding `  | `-mb` | bool | false | Determines whether the given object is bound to a material. |

### Return Value

Returns `true` or `false` in response to the given query.

## `EditTargetCommand`

The purpose of this command is to set the current edit target.

### Command Flags

| Long flag      | Short flag | Type           | Description |
| -------------- | ---------- | -------------- | ----------- |
| `-edit`        | `-e`       | noarg          | Set the current edit target, chosen by naming it in the `-editTarget` flag |
| `-query`       | `-q`       | noarg          | Retrieve the current edit target |
| `-editTarget`  | `-et`      | string         | The name of the target to set with the `-edit` flag |

## `mayaUsdSchema`

The purpose of this command is to query or apply USD schemas to USD prims.
It takes as its main arguments a list of UFE paths.

### Command Flags

| Long flag                   | Short flag | Type           | Description                                  |
| --------------------------- | ---------- | -------------- | -------------------------------------------- |
| `-appliedSchemas`           | `-app`     | noarg          | Query which schemas the prims have in common |
| `-schema`                   | `-sch`     | string         | The schema type name to apply to the prims   |
| `-instanceName`             | `-in`      | string         | The instance name for multi-apply schema     |
| `-removeSchema`             | `-rem`     | noarg          | Remove the schema instead of applying it     |
| `-singleApplicationSchemas` | `-sas`     | noarg          | Query the list of known single-apply schemas |
| `-multiApplicationSchemas`  | `-mas`     | noarg          | Query the list of known multi-apply schemas  |

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
| `-lockLayer`        | `-lk`      | int int string | Lock, System-Lock or unlock a layer and its sublayers. `Lock Type`: `0` = Unlocked, `1` = Locked and `2` = System-Locked. `Include Sublayers` : `0` = Top Layer Only, `1` : Top and Sublayers |
| `-refreshSystemLock`| `-rl`      | string int     | Refreshes the lock status of the named layer. `0` = Only top layer, `1` = Include the sublayers|
| `-replaceSubPath`   | `-rp`      | string string  | Replaces a path in the layer stack       |
| `-removeSubPath`    | `-rs`      | int string     | Remove a sub layer at a given index      |


## `LayerEditorWindowCommand`

The purpose of this command is to control the layer editor window.

### Command Flags

| Long flag               | Short flag |  Mode  | Description                                   |
| ----------------------- | ---------- | ------ | --------------------------------------------- |
| `-edit`                 | `-e`       |        | Edit various aspects of the editor window     |
| `-query`                | `-q`       |        | Retrieve various aspects of the editor window |
| `-addAnonymousSublayer` | `-aa`      |  Edit  | Add an anonynous layer at the top of the stack, returns it |
| `-addParentLayer`       | `-ap`      |  Edit  | Add a parent layer                            |
| `-loadSubLayers`        | `-lo`      |  Edit  | Open a dialog to load sub-layers              |
| `-removeSubLayer`       | `-rs`      |  Edit  | Remove sub-layers                             |
| `-clearLayer`           | `-cl`      |  Edit  | Erase everything in a layer                   |
| `-discardEdits`         | `-de`      |  Edit  | Discard changes made on a layer               |
| `-layerHasSubLayers`    | `-ll`      | Query  | Query if the layer has sub-layers             |
| `-isAnonymousLayer`     | `-al`      | Query  | Query if the layer is anonymous               |
| `-isLayerDirty`         | `-dl`      | Query  | Query if the layer has been modified          |
| `-isInvalidLayer`       | `-il`      | Query  | Query if the layer is not found or invalid    |
| `-isSubLayer`           | `-su`      | Query  | Query if the layer is a sub-layer             |
| `-isIncomingLayer`      | `-in`      | Query  | Query if the layer is incoming (connection)   |
| `-layerAppearsMuted`    | `-am`      | Query  | Query if the layer or any parent is muted     |
| `-layerIsMuted`         | `-mu`      | Query  | Query if the layer itself is muted            |
| `-layerIsReadOnly`      | `-r`       | Query  | Query if the layer or any parent is read only |
| `-muteLayer`            | `-ml`      |  Edit  | Toggle the muting of a layer                  |
| `-layerAppearsLocked`   | `-al`      | Query  | Query if the layer's parent is locked         |
| `-layerIsLocked`        | `-lo`      | Query  | Query if the layer itself is locked           |
| `-layerAppearsSystemLocked` | `-as`  | Query  | Query if the layer's parent is system-locked  |
| `-layerIsSystemLocked`  | `-ls`      | Query  | Query if the layer itself is system-locked    |
| `-lockLayer`            | `-lk`      |  Edit  | Lock or unlock a layer.                       |
| `-lockLayerAndSubLayers`| `-la`      |  Edit  | Lock or unlocks a layer and its sublayers.    |
| `-layerNeedsSaving`     | `-ns`      | Query  | Query if the layer is dirty or anonymous      |
| `-printLayer`           | `-pl`      |  Edit  | Print the layer to the script editor output   |
| `-proxyShape`           | `-ps`      |Query<br>Create| Query the proxyShape path or (Create) sets the selected shape by its path. Takes the path as argument |                    |   
| `-reload`               | `-rl`      |Query<br>Create<br>Edit| Open or show the editor window                |
| `-selectionLength`      | `-se`      | Query  | Query the number of items selected            |
| `-isSessionLayer`       | `-sl`      | Query  | Query if the layer is a session layer         |
| `-selectPrimsWithSpec`  | `-sp`      |  Edit  | Select the prims with spec in a layer         |
| `-saveEdits`            | `-sv`      |  Edit  | Save the modifications                        |
| `-getSelectedLayers`    | `-gsl`     | Query | Query the selected layers in the layer editor (note this is different from the edit target). Returns layer ids of the selected layers.    |
| `-setSelectedLayers`    | `-ssl`     |  Edit  | Set the selected layers in the layer editor with a semicolon delimited string of layer ids. Eg: `-e -ssl "layer_id_1;layer_id_2"`   |

In order to get notifications on layer selection changes, you can use `mayaUsd.lib.registerUICallback` with the `onLayerEditorSelectionChanged` notification:
```
    import mayaUsd.lib
    def exampleCallback(context, callbackData):
        # Get the stage object path
        objectPath = context.get('objectPath')
        # Get the list of selected layers
        layerIds = callbackData.get('layerIds')
        for layerId in layerIds:
            print(layerIds)

    mayaUsd.lib.registerUICallback('onLayerEditorSelectionChanged', exampleCallback)
```
