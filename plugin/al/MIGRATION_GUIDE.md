# Migration Guide from AL_USDMaya to ADSK_USDMaya

## Introduction

AL_USDMaya and ADSK_USDMaya[^1] have a lot of similarities [^2]:

- Both proxy shape nodes are inherited from the same base class `MayaUsdProxyShapeBase`
- Both of the plugins use `vp2RenderDelegate/proxyRenderDelegate` for sub-scene override

In most cases, ADSK_USDMaya could be a drop-in replacement for AL_USDMaya.

Please note that this documentation is not a complete feature comparison of AL_USDMaya and ADSK_USDMaya but mainly focuses on migration from one to another.

[^1] To distinguish Animal Logic's version and Autodesk's version, we call Autodesk's version "ADSK_USDMaya" for convenience.

[^2] This documentation is written based on the current latest release of `maya-usd-v0.27.0`.

## Proxy Shape

### Dependency Nodes

- `AL_usdmaya_ProxyShape`

An equivalent proxy shape type in ADSK_USDMaya is `mayaUsdProxyShape`.

- `AL_usdmaya_LayerManager`

The equivalent node type in ADSK_USDMaya is `mayaUsdLayerManager`.

- `AL_usdmaya_Layer`

Obsolete in AL_USDMaya, no equivalent in ADSK_USDMaya.

- `AL_usdmaya_RendererManager`

Obsolete in AL_USDMaya since moving to VP2 render delegate, no equivalent in ADSK_USDMaya.

- `AL_usdmaya_Scope`, `AL_usdmaya_Transform`, `AL_usdmaya_MeshAnimCreator` and `AL_usdmaya_MeshAnimDeformer`

There is no equivalent nodes like these to [drive transformation and animation from USD to Maya](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/docs/animation.md) in ADSK_USDMaya, however, the closest implementation is to use [proxyAccessor](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/nodes/proxyAccessor.h#L54-L75) for communication between USD and Maya. Find out more usage examples in this [unit test](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/test/lib/testMayaUsdProxyAccessor.py).

- `AL_usd_ProxyUsdGeomCamera`

This node works similar as MayaUsd ProxyAccessor but for camera, in ADSK_USDMaya, there is native support of USD camera via the [Ufe::Camera interface](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/usdUfe/ufe/UsdCamera.h#L30), USD camera (and derived schema cameras) could be interacted like other Maya cameras but with some limitations, for instance when viewing through a USD camera you cannot use the panning, zoom, and dolly.

### USD Stage Creation

#### From Maya UI menu

- AL_USDMaya:

  - USD > Proxy Shape > Import

- ADSK_USDMaya :

  - Create > Universal Scene Description > Stage with New Layer or Stage from File

#### From Command and Scripting

See ["Commands" section](#commands) below.

### Prim Locking

Both of proxy shapes support locking prims by inserting a special metadata but name of the metadata is different, your USD scene will need to change to the new metadata name, the difference is:

| | AL_USDMaya | ADSK_USDMaya |
| --- | --- | --- |
| Doc | [See here](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/docs/lock.md) and [here](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/tutorials/lock/README.md) | [See here](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/utils/plugInfo.json#L16) |
| Prim metadata | `al_usdmaya_lock` | `mayaLock` |
| Applies To | All xformOps of a prim | Individual xformOp of a prim |
| USD example | <br>def Xform "geo" (<br>&nbsp;&nbsp;&nbsp;&nbsp;al_usdmaya_lock = transform"<br>)<br>{<br>}<br>|def Xform "geo"<br>{<br>&nbsp;&nbsp;&nbsp;&nbsp;float3 xformOp:rotateXYZ (<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;mayaLock = "on"<br>&nbsp;&nbsp;&nbsp;&nbsp;)<br>&nbsp;&nbsp;&nbsp;&nbsp;double3 xformOp:translate (<br>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;mayaLock = "on"<br>&nbsp;&nbsp;&nbsp;&nbsp;)<br>}|
| Effectiveness | Affects the current prim and all its descendants | Affects the current prim and all its descendants |
| Note | | `mayaLock` in ADSK_USDMaya behaves like native Maya attributes, which requires setting the locking state explicitly at individual `xformOp`.<br><br>Toggling a parent prim’s attribute metadata only affects the parent prim itself.|

There is no exact equivalent approach in ADSK_USDMaya comparing with AL_USDMaya, the alternatives could be:

- Insert `mayaLock` metadata in all prims hierarchically;
- OR [Assign prims as Display Layers](https://help.autodesk.com/view/MAYAUL/2025/ENU/?guid=GUID-C52465E4-FA5F-46AF-A186-E20ABEDD9BF2)

### Prim Selectability

| | AL_USDMaya | ADSK_USDMaya |
| --- | --- | --- |
| Doc | [See here](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/tutorials/selectability/README.md) | |
| Control via | Prim metadata<br>`al_usdmaya_selectability` | Maya [Display Layer](https://help.autodesk.com/view/MAYAUL/2025/ENU/?guid=GUID-C52465E4-FA5F-46AF-A186-E20ABEDD9BF2) (available since **2023.2** or above) |
| Applies To | Individual Prim | Individual Prim |
| Example | def Xform "geo" (<br>&nbsp;&nbsp;&nbsp;&nbsp;al_usdmaya_selectability = "unselectable"<br>)<br>{<br>} | |
| Effectiveness | Affects the current prim and all its descendants | Affects the current prim and all its descendants |

Note that for instances, there is a metadata "mayaSelectability" to [control](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/utils/plugInfo.json#L7-L14) instances selection on viewport.

### Selection Mode (Pick Mode)

| | AL_USDMaya | ADSK_USDMaya |
| --- | --- | --- |
| Doc | | [See here](https://help.autodesk.com/view/MAYAUL/2025/ENU/?guid=GUID-532E99C9-F638-49E3-9483-800FDB9B65D7) |
| Maya `optionVar` | `AL_usdmaya_pickMode` (enum) | `mayaUsd_SelectionKind` (string) |
| Note | AL_USDMaya no longer supports this optionVar since moving forward to **VP2RenderDelegate** for sub-scene override.| |

`mayaUsd_SelectionKind` optionVar is the equivalent in ADSK_USDMaya.

### Push/pull workflow (Edit-As-Maya workflow)

AL_USDMaya has a custom Maya command `AL_usdmaya_TranslatePrim` (see [Commands](#commands) below) to "Push to Maya" and "Pull to USD" to edit geometries in Maya; the alternative in ADSK_USDMaya the new ["Edit As Maya"](https://help.autodesk.com/view/MAYAUL/2025/ENU/?guid=GUID-C1C08BA5-24EA-44FF-9497-71E0A6843744) workflow to author geometries (and materials).

### Translator Plugin System

There are two types of translators in AL_USDMaya, they both inherit the same base class, [`TranslatorBase`](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/lib/AL_USDMaya/AL/usdmaya/fileio/translators/TranslatorBase.h#L229), which implement the virtual `import()` and `exportObject()` methods (in C++ or Python):

1. [Schema Translator Plugins](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/translators/README.md)

    - Registered as "schematype" in [`TranslatorManufacture`](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/lib/AL_USDMaya/AL/usdmaya/fileio/translators/TranslatorBase.cpp#L47) when USD finds and loads the schema plugins
    - Static mapping of a custom schema type to Maya data type
    - Will be used when `AL_usdmaya_TranslatePrim` command is run to exchange data between Maya and USD
    - Will be used when import or export data from or to USD file in Maya

2. Python translators

    - Registered as "assettype" in `TranslatorManufacture` which usually needs to be registered explicitly at via `AL.usdmaya.TranslatorBase.registerTranslator()` (in your pipeline) and the USD prims must declare metadata `assettype = "your_custom_type"`, see [example API call here](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/plugin/AL_USDMayaTestPlugin/py/testTranslators.py) and [example USD content here](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/plugin/AL_USDMayaTestPlugin/test_data/inactivetest.usda#L15)
    - Metadata driven approach allowing the flexibility in pipeline to not statically maps to a specific schema type
    - Will be used when `AL_usdmaya_TranslatePrim` command is run to exchange data between Maya and USD

ADSK_USDMaya has a clear separation of:

- Exchange data between USD stage and Maya when using proxy shape

  - [`PrimUpdater` plugin](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/fileio/primUpdaterRegistry.h#L34-L60)
  - Static mapping of USD data type and Maya data type
  - Will be used for the ["Edit As Maya"](https://help.autodesk.com/view/MAYAUL/2025/ENU/?guid=GUID-C1C08BA5-24EA-44FF-9497-71E0A6843744) workflow

- Import data from USD to Maya

  - [`PrimReader` plugin](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/fileio/primReaderRegistry.h#L31-L51)
  - Static mapping of USD data type and Maya data type
  - Will be used when importing

- Export data from Maya to USD

  - [`PrimWriter` plugin](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/fileio/primWriterRegistry.h#L34-L64)
  - Static mapping of USD data type and Maya data type
  - Will be used when exporting

If your workflow has AL_USDMaya's schema translator plugins, porting to ADSK_USDMaya's `PrimReader`, `PrimWriter` and `PrimUpdater` would be straightforward; however, if you have workflow relies on "assettype" metadata (Python translators), there won't be anything equivalent in ADSK_USDMaya.

Since the Python translators are run during:

- Stage loading in proxy shape
- Exchange data between USD and Maya via `AL_usdmaya_TranslatePrim` command

To maintain the similar behavior, one possible solution would be:

- Register your own [`MayaUsdProxyStageSetNotice`](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/listeners/proxyShapeNotice.h#L54) to receive notification when a stage has been set in proxy shape
- OR explicitly run stage traversal function immediately after creating proxy shape
- Then traverse stage hierarchy, search prims with metadata "assettype" and kick off the `import()` or `exportObject()` manually

### USD Transaction

AL_USDMaya has USD transaction module integrated into the proxy shape but there is no equivalent module for ADSK_USDMaya, there is no batch [processing changed objects notices](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/lib/AL_USDMaya/AL/usdmaya/nodes/ProxyShape.cpp#L997) support in ADSK_USDMaya, [`ObjectsChanged` notice](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/nodes/proxyShapeBase.cpp#L2078) will be executed as they are being sent.

## Commands

### Import Command and Export Command

#### `AL_usdmaya_ImportCommand`

ADSK_USDMaya provided an equivalent command `mayaUSDImportCommand`, see [Import and Export](#import-and-export-static-io) below for more details.

#### `AL_usdmaya_ExportCommand`

ADSK_USDMaya provided an equivalent command `mayaUSDExportCommand`, see [Import and Export](#import-and-export-static-io) below for more details.

### Proxy Shape commands

#### `AL_usdmaya_ProxyShapeImport`

This command does not only create the proxy shape node but also loads the given USD stage; in ADSK_USDMaya, you will have to do it separately:

- Create the proxy shape node
- Set the `.filePath` attribute to point to your USD root layer, and set other attributes if necessary
- Connect `time1` to `.time` attribute if you need to read animation

Example MEL function `mayaUsd_createStageFromFilePath` bundled in ADSK_USDMaya can be found [here](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/adsk/scripts/mayaUsd_createStageFromFile.mel#L274C20-L274C51).

#### `AL_usdmaya_ProxyShapeResync`

[`AL_usdmaya_ProxyShapeResync`](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/lib/AL_USDMaya/AL/usdmaya/cmds/ProxyShapeCommands.h#L198) triggers AL's [`ProxyShape::resync()`](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/lib/AL_USDMaya/AL/usdmaya/nodes/ProxyShape.cpp#L806) which will then rerun the [translator system](#translator-plugin-system) to import/teardown/update associated Maya data according to the changes in USD stage, you will need to explicitly traverse stage hierarchy manually to achieve the same in ADSK_USDMaya (see [translator system](#translator-plugin-system) above for the details).

#### `AL_usdmaya_TranslatePrim`

ADSK_USDMaya introduced a new ["Edit As Maya"](https://help.autodesk.com/view/MAYAUL/2025/ENU/?guid=GUID-C1C08BA5-24EA-44FF-9497-71E0A6843744) workflow to exchange data between USD and Maya, there are two equivalent Maya commands to push / pull USD data to/from Maya:

- `mayaUsdEditAsMaya`
  - Equivalent implementation of AL's `AL_usdmaya_TranslatPrim` where "importPaths" or "updatePaths" or "forceImport" flag is being used, a.k.a "Push to Maya"

- `mayaUsdMergeToUsd`
  - Equivalent implementation of AL's `AL_usdmaya_TranslatPrim` where "teardownPaths" flag is being used, a.k.a "Pull to USD"

For more info about this new workflow in Maya, see the official ["Edit As Maya"](https://help.autodesk.com/view/MAYAUL/2025/ENU/?guid=GUID-C1C08BA5-24EA-44FF-9497-71E0A6843744) workflow doc.

If you have translators implemented for AL_USDMaya:

- Schema based translators:
  - You will need to update the translators to be [`PrimUpdater` plugin](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/fileio/primUpdaterRegistry.h#L34-L60)

- "assettype" metadata based translators (Python translators):
  - You will need to run them manually

See [translator system](#translator-plugin-system) above for more details.

#### `AL_usdmaya_ProxyShapeFindLoadable`

No equivalent command in ADSK_USDMaya, recommends finding layers with native USD API.

#### `AL_usdmaya_ListTranslators`

This command is part of the [Translator Plugin System](#translator-plugin-system), there is no equivalent command in ADSK_USDMaya.

#### `AL_usdmaya_ProxyShapeImportAllTransforms`

This command creates a Maya transform hierarchy next to the proxy shape node that mirrors the stage hierarchy. There is no equivalent command in ADSK_USDMaya, which recommends creating a transform chain with native Maya API.

#### `AL_usdmaya_ProxyShapeRemoveAllTransforms`

No equivalent command in ADSK_USDMaya, recommends removing the transform chain with native Maya API.

#### `AL_usdmaya_ProxyShapeImportPrimPathAsMaya`

No equivalent command in ADSK_USDMaya, recommends managing transform nodes with native Maya API.

### AL Event System and Callbacks Commands

AL_USDMaya implemented a custom event system (see [Core Event System](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/lib/AL_USDMaya/AL/docpages/docs_events.h) and [Maya Event System](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/lib/AL_USDMaya/AL/docpages/docs_mayaevents.h)) to group and prioritize global and per-node callback, there is no equivalent in ADSK_USDMaya, however, all of them can be easily reimplemented by native Maya [MMessage](https://help.autodesk.com/view/MAYADEV/2025/ENU/?guid=MAYA_API_REF_cpp_ref_class_m_message_html).

Commands below have no equivalent in ADSK_USDMaya, we recommend to reimplement with native Maya MMessage API and manage the callbacks and events separately (in your code):

- `AL_usdmaya_command_Callback`
- `AL_usdmaya_command_ListEvent`
- `AL_usdmaya_ListEvents`
- `AL_usdmaya_ListCallbacks`
- `AL_usdmaya_Callback`
- `AL_usdmaya_DeleteCallbacks`
- `AL_usdmaya_CallbackQuery`
- `AL_usdmaya_Event`
- `AL_usdmaya_EventQuery`
- `AL_usdmaya_EventLookup`
- `AL_usdmaya_TriggerEvent`

### Prim and Layer manipulation commands

#### `AL_usdmaya_LayerManager`

This command allows you to query dirty layers currently managed by LayerManager in AL_USDMaya, there is no equivalent command in ADSK_USDMaya, we recommend querying dirty layers via native USD API.

#### `AL_usdmaya_LayerCreateLayer`

This command opens a new Sdf layer from path (with `-o` flag) or creates a new anonymous layer (without `-o` flag), and inserts as a sub layer into the root layer (with `-s` flag).

Equivalent command and example usage in ADSK_USDMaya:

- Create and insert a new anonymous layer into existing layer:
  - **mayaUsdLayerEditor** -edit -addAnonymous  "new_anon_layer_id" "existing_parent_layer_id";
- Open and insert the new layer as the strongest layer (index `0`) into existing layer:
  - **mayaUsdLayerEditor** -edit -insertSubPath 0  "/path/to/layer/file.usd" "existing_parent_layer_id";

ADSK_USDMaya provides both an [UI interface](https://help.autodesk.com/view/MAYAUL/2025/ENU/?guid=GUID-4FAD73CA-E775-4009-9DCB-3BC6792C465E) and [command line interface](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/commands/layerEditorCommand.cpp#L831-L868) to manage USD layers, it has more features and options compared to AL_USDMaya, please refer to the links for other usages.

Recommend to use either native USD API or ADSK_USDMaya's **mayaUsdLayerEditor** command.

#### `AL_usdmaya_LayerCurrentEditTarget`

ADSK_USDMaya has similar command to get/set edit target but for **local layer** only:

- **mayaUsdEditTarget** -q -et "|usdTest|usdTestShape";
- **mayaUsdEditTarget** -e -et "target_layer_id" "|usdTest|usdTestShape";

`AL_usdmaya_LayerCurrentEditTarget` commands support setting edit targets to be a non-local layer, however, it is limited for references, it does not take into account sub layers in references and nested references.

We recommend using native USD API for non-local edit target layer cases until there is better support in ADSK_USDMaya.

Following commands are wrappers around USD APIs, there is no equivalent commands in ADSK_USDMaya, recommend switching over to native USD APIs:

- `AL_usdmaya_ChangeVariant`
- `AL_usdmaya_ActivatePrim`
- `AL_usdmaya_CreateUsdPrim`
- `AL_usdmaya_LayerGetLayers`
- `AL_usdmaya_LayerSave`
- `AL_usdmaya_LayerSaveMuted`

### Viewport and Renderer Command

#### AL_usdmaya_ManageRenderer

Obsoleted in AL_USDMaya and no equivalent command in ADSK_USDMaya, recommend using native Maya commands to manage viewport renderer.

### Debugging

### AL_usdmaya_UsdDebugCommand

No equivalent command in ADSK_USDMaya, recommends toggling debug symbols with native USD Tf debug APIs.

## Import and Export (Static IO)

Both of [AL_USDMaya](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/docs/importExport.md) and [ADSK_USDMaya](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/commands/Readme.md) work very similar, in most of the cases, it should be sufficient to just replace the command or file translator name, for example:

If in AL_USDMaya:

- **AL_usdmaya_ImportCommand** -f "<path/to/out/file.usd>" -primPath "/path/to/prim" ...options...
- file -force -typ **"AL usdmaya import"** -options "...." /path/to/out/file.usd
- **AL_usdmaya_ExportCommand** -f "<path/to/out/file.usd>" ...options...
- file -force -typ **"AL usdmaya export"** -options "...." /path/to/out/file.usd

Could be simply modified to be like this with ADSK_USDMaya:

- **mayaUSDImportCommand** -f "<path/to/out/file.usd>" -primPath "/path/to/prim" ...options...
- file -force -typ **"USD Import"** -options "...." /path/to/out/file.usd
- **mayaUSDExportCommand** -f "<path/to/out/file.usd>" ...options...
- file -force -typ **"USD Export"** -options "...." /path/to/out/file.usd

You probably need to tweak the option names a little bit and the USD content exported by these two plugins has many (minor) details differences regarding the structure and metadata, however, overall, the output USD stage looks mostly the same.

### Caveats and Minor Differences

Note: issues listed here are tested up to version maya-usd-**v0.27.0**.

#### Export joints from a parent hierarchy

`mayaUSDExportCommand` does not work well with `-exportSkels` and `-exportRoots` together, you won't be able to extract (and export) the joints under parent nodes

    - Details in [this issue](https://github.com/Autodesk/maya-usd/issues/3596)
    - Workaround:
        - Either leave the joints in worldspace
        - OR export with AL_USDMaya

#### Interpolation value control for UV and display color

AL_USDMaya has an [option "Compaction Level"](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/translators/CommonTranslatorOptions.h#L39) with choice of `None`, `Basic` and `Full` to control how UV and display color data being exported, this compaction level impacts **interpolation** value if it's [been set to `Basic` or `FuLL`](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/plugin/al/usdmayautils/AL/usdmaya/utils/DiffPrimVar.cpp#L755-L773).

For ADSK_USDMaya:
    - Compaction on display color is **on** automatically, user does not have a control how **interpolation** could be
    - UV (st) **interpolation** is either "constant" or "faceVarying", no compaction would be performed

If your workflow relies on a specific interpolation type, you might need to adjust your tool to take into account this minor difference.

#### Display color sets

ADSK_USDMaya imports display color (RGB) and display opacity (A) as two color sets (RGB + A) in Maya, whereas in AL_USDMaya if both display color (RGB) and display opacity (A) were presented, they will be combined as one color set (RGBA) in Maya.

#### Crease Sets

For subdivision meshes with creases, ADSK_USDMaya imports creases as "CreaseSet", unlike AL_USDMaya that "Creases" would be applied in-place for the meshes.

### Import Export Customization

ADSK_USDMaya introduced a few different approaches to custom the import / export to meet production requirements:

#### SchemaAPI

[SchemaAPI](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/fileio/doc/SchemaAPI_Import_Export_in_Python.md) provides a reasonable design to work with custom schema types, the [Rigid Body example](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/fileio/doc/SchemaAPI_Import_Export_in_Python.md#extending-to-nearby-maya-objects) is a good one to demo the capability of SchemaAPI.

The cons of using SchemaAPI is that it requires binding to an "applied schema" which is less flexible on arbitrary data types.

#### Job Contexts

[Job Contexts](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/fileio/doc/Managing_export_options_via_JobContext_in_Python.md) is best suited for managing predefined import / export options, especially useful for centralizing department specific options.

#### Chasers

[Import chasers](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/commands/Readme.md#import-chasers) and [Export chasers](https://github.com/Autodesk/maya-usd/blob/release/v0.27.0/lib/mayaUsd/commands/Readme.md#export-chasers-advanced) are the most flexible way to customize import and export behaviors, you could access the USD stage, prim, Maya object and import/export options in chasers, which you could pretty much any customization you like in chasers:

- Minor tweaks and postfix of the USD or Maya data
- Validation and sanitization
- Execute post processing steps (callbacks etc.)
