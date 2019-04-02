## AL_USDMaya-0.32.13 (2019-04-02)
### Fixed
When importing meshes, primVar indices could end up being lost.
Correctly exclude instance paths when translating an instanced prim

## AL_USDMaya-0.32.12 (2019-04-02)
### Added
+ A node to act as a proxy between Maya and USD cameras to enable interactive updates to the USD scene see [docs](docs/cameraProxy.md) for more info
+ Added a new flag to the AL_usdmaya_TranslatePrim command to control whether pushToPrim is enabled on newly created transforms.
+ Ability to import nested instances into Maya from USD.

### Fixed
+ Potential problems when importing/exporting surface normals.
+ Bug where a transform chain would be incorrectly created for prims not being translated (When translating a prim into the maya scene, if the forceImport flag was disabled, then a transform chain would end up being created for a non existent prim (and could never be removed again). The PrimFilter now respects the forceImport flag.)
+ crash on AL_USDMaya Plugin Registration


## AL_USDMaya-0.32.11 (2019-03-26)
### Removed
+ DrivenTransformData MPxData type
+ DrivenTransforms.h/.cpp
+ DrivenTransformData.h/.cpp
+ test_DrivenTransforms.cpp
+ ProxyShape::computeDrivenAttributes method
+ related maya attribute on Proxy Shape etc



## AL_USDMaya-0.32.10 (2019-03-25)
### Fixed
* consolidate MayaReference update logic to make it more resilient: Combine the similar logic from update() and LoadMayaReference() to make the maya reference translator more resilient to imported references and stage changes [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/138) (#138 - @[nxkb](//github.com/nxkb))
+ Fixes a crash that happens when using AL_usdmayaProxyShapeSelect with pseudo-root "/" [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/145) (#145 - @[elrond79](//github.com/elrond79))
+ usd-0.19.3 fix: filesystem library needs system lib - need to explicitly link now that usd doesn't [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/146) (#146 - @[elrond79](//github.com/elrond79))
+ convert onAttributeChanged callbacks to setInternalValue, for performance [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/89) (#89 - @[elrond79](//github.com/elrond79))

## AL_USDMaya-0.32.9 (2019-03-14)
### Fixed
+ Fixed crash when modifying rotation attributes
+ Fixed erratic rotate tool

## AL_USDMaya-0.32.8 (2019-03-13)
### Fixed
+ Fixed potential crash on variant switch
+ Fixed problem where plugin translator options would not post the previous settings to the import/export GUI. 

## AL_USDMaya-0.32.7 (2019-03-11)
### Changed
+ Reverted the TransformReference struct to the state prior to 0.32.2 . This may re-introduce variant switch crashes.

### Fixed
+ No longer incorrectly expanding normals in the per-vertex case.

## AL_USDMaya-0.32.6 (2019-03-11)
### Changed
+ Refresh command call added at the end of ProxyShape::onObjectsChanged [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/144) (#144 - @[pilarmolinalopez](//github.com/pilarmolinalopez))
+ Some c++11 style foreach loops changed to iterate over refs, instead of values, to avoid copy [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/142) (#142 - @[elrond79](//github.com/elrond79))
+ Replaced removeVariantFromPath in ProxyShapeUI with standard SdfPath:: StripAllVariantSelections [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/142) (#142 - @[elrond79](//github.com/elrond79))
+ Removed an unused opIt var from TransformTranslator [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/142) (#142 - @[elrond79](//github.com/elrond79))
+ ProxyShapeSelection - avoided re-assignment of helper.m_paths = m_selectedPaths on every loop [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/142) (#142 - @[elrond79](//github.com/elrond79))
+ Removed unused if-branch in ProxyDrawOverride::userSelect [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/142) (#142 - @[elrond79](//github.com/elrond79))
+ FindUFE.cmake now detects version (so you can require specific version, and don't need to explicitly set it beforehand) [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/139) (#139 - @[elrond79](//github.com/elrond79))

### Fixed
+ Fixes bug where, in UFE mode, if had a USD hierarchy without any Xform nodes, it would select the
root anytime you clicked on anything [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/142) (#142 - @[elrond79](//github.com/elrond79))
+ Made sure SelectionEnded event was triggered in toggle mode, even if no changes made [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/142) (#142 - @[elrond79](//github.com/elrond79))
+ ufe selection: check for correct proxy shape before drawing sel highlight [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/141) (#141 - @[elrond79](//github.com/elrond79))
+ Fixed deselect mode of the proxy shape command [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/12) (#12 - @[elrond79](//github.com/elrond79))

## AL_USDMaya-0.32.5 (2019-03-04)
### Fixed
+ In Maya 2019, The File translator GUI's could end up failing due to an incorrect early return.
+ Bug when exporting normals, for cases where normalIDs need to be considered

## AL_USDMaya-0.32.4 (2019-03-04)
### Added
+ canBeOverridden method to the translator plugin to identify which plugins can be overridden

## AL_USDMaya-0.32.3 (2019-03-01)

### Changed

+ Default values are now set for file translator options.
+ Nurbs Curve: width was never being written
+ Nurbs Curve: Maya dynamic attribute written from Curve shape (consistent with mesh behaviour)

## AL_USDMaya-0.32.2 (2019-03-01)
### Added

+ 3 utility functions used by our internal translators to AL/maya/utils

### Changed

+ Fixed crashes during variant switches and prim activation/deactivation

## AL_USDMaya-0.32.0 (2019-02-25)
### Added

* added exportDescendants virtual method to TranslatorBase - this checks whether children of the exported node should be exported, and early outs if false (defaults to true)
* support for exporting custom Transforms by calling the appropriate registered translator (previously all transforms were exported with the default Transform Translator
* A system to allow for plugin translator options to be specified by a plugin
* Support for export of world space animation data
* Active/Inactive state on the translator plugins
* New command AL_usdmaya_ListTranslators
* New export/import options to specify the translator plugins that are enabled.

### Changed

* Some of the older options (specifically for mesh/camera/etc) have been changed from the hard coded variants, to plugin options.

### Removed

* Removed old (and no longer used) legacy translators

## AL_USDMaya-0.31.1 (2019-02-21)
### Added

* Maya directional light import/export [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/137) (#137 - @[wengn](//github.com/wengn))

## AL_USDMaya-0.31.0 (2019-02-07)
### Added

* Option for the exporter to output transforms in world space.

### Changed

* Avoid reconstruction of imaging engine on prim resync
* Updated to work with USD 0.19.01, see [PR](https://github.com/AnimalLogic/AL_USDMaya/pull/133/) (#133 - @[elrond79](//github.com/elrond79))

## AL_USDMaya-0.30.5 (2019-01-30)
### Added

* AL_usdmaya_MeshAnimCreator node. Used to read mesh data directly from USD into the Maya DG. Avoids the need to store mesh data (translated from USD) in the maya files on save.
* AL_usdmaya_MeshAnimDeformer node. Used to modify a mesh by updating the points/normals to the values from USD (at a given time)

## AL_USDMaya-0.30.4 (2019-01-18)
### Added

* Added a [AL_MayaTest docs](docs/testing.md) file explaining how to use the AL_MayaTest library, run AL_USDMaya tests etc

### Changed

* googletest updated to 1.8 - no code change

## AL_USDMaya-0.30.3 (2019-01-16)
### Added

* Strings can now be exported

## AL_USDMaya-0.30.2 (2019-01-14)
### Changed

* update root cmake file to not build new mayaTest library
* Bumping version to 0.30.2

### Fixed

* Extra Data Plugins not called on file import.

## AL_USDMaya-0.30.1 (2019-01-04)
### Added

* Created AL_MayaTest library to store reusable test utility and test "harness" code
* Extra Data Translation Plugins "An additional plugin mechanism to allow import/export of data which is not tied to a specific USD Schema (Typed or API) or Maya node type". See [docs](docs/extraDataTranslatorPlugins.md)

### Changed

* Removed openMayaFX as a dependency on AL_USDMaya lib
* Bumped version to 0.30.1

### Removed

* Test boilerplate from mayautils and AL_USDMayaTestPlugin

## AL_USDMaya-0.30.0 (2018-12-21)
### Changed

* Updated schema python module path to be AL.usd.schemas.maya

## AL_USDMaya-0.29.9 (2018-12-19)
### Added
* Autodesk UFE selection integration (Maya 2019+)
* When building with UFE is enabled, you will still get the default AL selection behavior. To enable UFE selection, you also need to set the env var MAYA_WANT_UFE_SELECTION. For now this is checked at run-time allowing you to enable/disable it to switch between the different selection modes. Note: this UFE selection is supported in the old VP1 code and also in VP2. When UFE selection is enabled, perform an extra wireframe draw to show selection.
* Implemented VP2 selection, so the env var MAYA_VP2_USE_VP1_SELECTION is no longer required.
* Set proper version in Maya plugin.

### Deprecated
* MAYA_VP2_USE_VP1_SELECTION is no longer required for Maya 2019+

### Fixed
* Some minor Windows build fixes.
* Update bounding box when UFE selection occurs.

## AL_USDMaya-0.29.8 (2018-12-18)
### Fixed

* Fixed incorrect export mode of polygon normals

## AL_USDMaya-0.29.7 (2018-12-16)
### Added

* Added "Duplicate_Mesh_Points_as_PRef" export flag to export first frame of points as "pref" attribute (a duplicate of the first sample of "P")

## AL_USDMaya-0.29.5 (2018-11-21)
### Fixed

* Ensure custom animation nodes being processed.

## AL_USDMaya-0.29.4 (2018-11-01)
### Added

* Built against USD-0.18.11
* support for displaying proxyShapes with default materials instead of full gl shaders for performance by toggling maya's "use default material" option (#121 - @[nxkb](//github.com/nxkb))
* uvs/meshUVS flag for AL_usdmaya_ExportCommand to turn on/off the export of mesh UV beside other mesh data.
* uvo/meshUVOnly flag for AL_usdmaya_ExportCommand to turn on/off the UV data only export mode.
* "Mesh_UV_Only=1" option for export translator for uvOnly export.
* An override to the plugin translator interface that can be used to export animation from nodes that don't have a one to one mapping between UsdAttributes, and MPlugs.
* Added `PickMode` enum
* Added Python wrapper for `PickMode` enum
* Camera::writePrim()

### Changed

* Tests data is generated in unique temporary folders
* Inserted some calls to `internal_pushMatrix(...)` to handle pushing matrix values to prim.
* Camera::initialize() is exposed as public API.
* Camera::updateAttributes() is exposed publicly and signature changed to be able work on different schemas.
* Multiple shapes test checks exported prim is a type inherit from UsdGeomCamera.

### Deprecated

N/A

### Removed

* [schemas] generated plugInfo.json from source tree
* VALIDATE_GENERATED_SCHEMAS build option
* Setting matrix values on prim without validation.
* muv/meshUV flag for AL_usdmaya_ExportCommand and replace with uvo/meshUVOnly to make things clear.

### Fixed

* FindMaya cmake module checking MAYA_DEVKIT_INC_DIR before using it (#127 - @[AlexSchwank](//github.com/AlexSchwank))
* Rendering of Wireframe selected objects
* Transform Import now respects single float rotations
* Transform orders that do not match maya's order are now correctly imported.
* Failing tests were crashing maya
* Ask DG iterator to traverse over world space dependents. Without this change, for a typical skin deformer, traversal ends at all upstream joints. And MAnimUtil::isAnimated failed to detect it is an animated joint, which is not too unexpected. After setTraversalOverWorldSpaceDependents(), those ctrl and srt nodes will be traversed and detected as animated.
* Clear the xformOps when copying attributes during exporting to avoid crashing Maya.
* Pick the first exportable shape to export and ignore the others when there are multiple shapes under a transform, and "mergeTransforms" flag is turned on on AL_usdmaya_ExportCommand.
* Fix the primvar normal mismatch warnings by setting interpolation to "vertex" for normals attribute.
Overs are no longer applied to prims by selecting them in the viewport.
* Animated caches were no longer animating in the viewport - fixed by setting the time in the draw method (rather than the now infrequently called prepareForDraw method)
* The mesh translator does not respect the internal exporting mesh params.

## AL_USDMaya-0.29.2 (2018-09-21)

### Added
* AL proxy shape will now probably respect the light decay properties (ie, linear, quadratic, none) of lights in the viewport (#116 - @[elrond79](//github.com/elrond79))
* Implementation of new ProxyDrawOverride::userSelect() provided with maya2019 (#118 - @[pilarmolinalopez](//github.com/pilarmolinalopez))
* Implementation of ProxyShape::getShapeSelectionMask(#118 - @[pilarmolinalopez](//github.com/pilarmolinalopez))

### Changed
* Improved performance when proxy shape isn't changing (ie, viewport tumble) (#93 - @[elrond79](//github.com/elrond79))

### Removed
* [schemas] generated plugInfo.json from source tree
* VALIDATE_GENERATED_SCHEMAS build option

### Fixed
* Transform Import now respects single float rotations
* Transform orders that do not match maya's order are now correctly imported.
* Rendering of Wireframe selected objects
* Don't create do-nothing xform ops (#112 - @[elrond79](//github.com/elrond79))
* Fixed setting of shearTweak (#114 - @[elrond79](//github.com/elrond79))
* check for whether scalePivotTranslate needs to be set enableReadAnimatedValues was checking if scalePivot was set, not scalePivotTranslate (#115 - @[elrond79](//github.com/elrond79))

## AL_USDMaya-0.29.1 (2018-08-28)

### Added

* Sub-sample animation export

### Changed

* Only perform UsdImagingGLHdEngine creation when it is Maya interactive mode.
* Issue error in console when some nodes cannot be created in the transform chain, at which case usdMaya will go and parent the chain created right under the proxy transform.
* Built against USD-0.18.9
* Plugins' metadata (`plugInfo.json`) have been moved to **lib/usd** to reflect USD's new layout.
* The initial edit target of proxyShapes has changed from the root layer to the session layer (#102 - @[nxkb](//github.com/nxkb))
* The mapping from prim to maya dag path is no longer stored on the session layer, but is stored on the proxyShape class (#102 - @[nxkb](//github.com/nxkb))
* Made translatorProxyShape into a completely separate plugin, AL_USDMayaPxrTranslators (#109 - @[elrond79](//github.com/elrond79))
* This sandboxes the main AL_USDMayaPlugin from needing to link against any pixar maya libraries (ie, usdMaya) (#109 - @[elrond79](//github.com/elrond79))
* Also added a cmake option to disable building of the plugin entirely, BUILD_USDMAYA_PXR_TRANSLATORS (#109 - @[elrond79](//github.com/elrond79))
* Updated FindUSD.cmake to enable finding of usdMaya (#109 - @[elrond79](//github.com/elrond79))
* Updated github contribution guidelines

### Fixed

* Fix for opensource tests
* Cameras not exported on file -> export
* Win32 build error
* Fixed some runtime linkage problems with the main AL_USDMayaPlugin.so and usdMaya.so (#109 - @[elrond79](//github.com/elrond79))
* Fixed warning about signed/unsigned comparsion (#110 - @[elrond79](//github.com/elrond79))
* Fixed warning about unused errorString (#110 - @[elrond79](//github.com/elrond79))

## AL_USDMaya-0.29.0 (2018-08-06)

### Added

* struct MayaFnTypeId to uniquely identify Maya object types.
* TranslatorAbstract::supportedMayaTypes() returns a vector of Maya object types supported by this translator.
* TranslatorAbstract::claimMayaObjectExporting() provides a place for translator to confirm it wants to export this Maya object.
* TranslatorAbstract::exportObject() interface for translator to pull data from Maya object and fill into Usd prim.
* overloading TranslatorManufacture::get() returns all translator candidates for a Maya object type.
* UsdMaya translator for AL_usdmaya_ProxyShape (#99 - @[nxkb](//github.com/nxkb))
* Support for files that are not on disk until they are resolved (#100 - @[nxkb](//github.com/nxkb))
* Added new Pre/PostDestroyProxyShape events (#104 - @[elrond79](//github.com/elrond79))

### Changed

* TranslatorAbstract::import(), a third parameter is added to pass out Maya object created in importing process. This interface will be used in both AL_usdmaya_Import and AL_usdmaya_ProxyShapeImport workflows and TranslatorContext need be checked before using.
* A third argument is added to macro AL_USDMAYA_DEFINE_TRANSLATOR. It must be an array of MayaFnTypeId which defines all Maya object types this translator might export.
* Selection(is stored preFileSave() in a MSelectionList, not storing AL_USD proxies (#78 - @[wnxntn](//github.com/wnxntn))
* Selection is restored postFileSave() (#78 - @[wnxntn](//github.com/wnxntn))

### Removed

* fileio/translators/MeshTranslator
* fileio/translators/NurbsCurveTranslator
* fileio/translators/CameraTranslator
* Removed some EXPORT macros from inline methods (no need to export)

### Fixed
* A number of compiler warnings fixed, so PXR_STRICT_BUILD_MODE may be used (#97 - @[elrond79](//github.com/elrond79))
* added missing triggers for Pre/PostStageLoaded events (#103 - @[elrond79](//github.com/elrond79))

## AL_USDMaya-0.28.5 (2018-07-10)

### Added

* New open-source ALFrameRange schema and translator, which sets up Maya time range and current frame during the translation.
* Tests for ALFrameRange translator.
* Tests for the fix for the over-decref issue in transform chain.

### Fixed

* Over-decref in transform chain during some prim tear-down.

## AL_USDMaya-0.28.4 (2018-07-05)

### Added

* New events to prevent USDGlimpse from rebuilding the scene when mapping meta data is inserted into the prims during selection.

### Changed

* Transform values that have not changed no longer get written back to USD.
* Only hook up the proxy shape time attribute to the transform nodes that are actually transforms (i.e. ignore prims that are effectively just organisational containers)
* Allowed named events to be able to have callbacks registered against them prior to the events being registered

### Removed

* Some export macros from inline methods

### Fixed

* Regression where transform values were no longer being correctly displayed when translating prims into maya.
* A number of build warnings in the OSS build
* Final two build warnings in the repo. The generated schema code needs to have a flag set in the build C4099 to hide the 'class defined as struct' warning (pixar issue, not ours)
* When you duplicate a proxy shape, you wont see anything in the viewport and it wont be fully initialize until you reopen maya or find a hack (like changing the usd path) to trigger a load stage. (#98 - @[nxkb](//github.com/nxkb))
* Add missing newlines to some TF_DEBUG statements in ProxyShapeUI (#92 - @[elrond79](//github.com/elrond79))

## AL_USDMaya-0.28.3 (2018-06-18)

### Added
* AL_usdmaya_CreateUsdPrim command added to insert a new prim into the UsdStage of a proxy shape.

### Changed
* The proxyShape outStageData is now connectable, and now longer hidden. Allows for manual DG node connections to be made.

### Fixed
* The internal mapping between a maya object and a prim now works correctly when specifying the -name flag of AL_usdmaya_ProxyShapeImport.

## AL_USDMaya-0.28.2 (2018-06-14)

### Added
* Viewport will continue to refresh to show updated progressive renderers (like embree) until converged (#91 - @[elrond79](//github.com/elrond79))

### Changed
* make getRenderAttris update showRender, as well as showGuides (#87 - @[elrond79](//github.com/elrond79))

### Fixed
* Tweaks to allow builds to work with just rpath. (#88 - @[elrond79](//github.com/elrond79))
* When importing left handed geometry that has no normals, compute the correct set of inverted normals.

## AL_USDMaya-0.28.1 (2018-06-06)

### Added
* Documentation for mesh export, interpolation modes, and diffing.
* "-fd" flag added to the import command and the translate command
* Support for prim var interpolation modes in the import/export code paths (currently UV sets only, colour sets & normals will be in a later PR).
* Support for diffing all variations of the interpolation modes on prim vars.
* Routines to determine the interpolation modes on vec2/vec3/vec4 types.
* AL_usdmaya_LayerManagerCommands to retrieve layers that have been modified and that have been the EditTarget.

### Changed
* Built using USD-0.8.5
* ProxyShapePostLoadProcess::createTranformChainsForSchemaPrims now checks all the prims' parents' metadata for their unmerged status. If found, the transform chain creation skips the current prim in place of its parent.
* ProxyShapePostLoadProcess::createSchemaPrims now checks all the prims' parents' metadata for their unmerged status. If found, the schema prim import will instead take place under the parent node.
* Mesh::import now checks the prim's parents' metadata for its unmerged status. If found, the created shape node will not have the appended "Shape" string.
* Mesh attributes queries use EarliestTime timecode
* Functions in DgNodeTranslator are moved to DgNodeHelper in AL_USDMayaUtils
* AttributeType.h/.cpp are moved to AL_USDMayaUtils
* LayerManager API has been updated so you can now inspect all layers that have been set as an EditTarget and are Dirty.
* Updated endToEndMaya tutorial to work with latest USD API changes.
* All hard coded /tmp paths in the unit tests, in favour of a linux/win32 solution.

### Fixed

* Fix for when tearing down a shape, that it's sibling shapes are left alone in Maya.
* time1.outTime is now connected to proxy shape nodes when transforms are selected during creation.
* Selected transforms are no longer renamed when importing a proxy shape.
* Tests build restored.
* Fix for Issue "meshTransform cannot execute twice" #82
* Bug in the import/export of mesh normals
* Normals export now correctly listen to the exporter options.
* AL_usdmaya_plugin can be unloaded correctly.
* Passing the time value into the nurbs, camera, and transform translators on export.

### Removed
* Removed the code that reversed polygon windings if the 'leftHanded' meta data flag was encountered. Instead, the 'opposite' flag is set on the maya mesh.

## AL_USDMaya-0.27.10 (2018-05-07)

### Added

* A new plugin translator (translator/NurbsCurve.cpp) to translate prim between USD and Maya, and write back editing result to USD prim.

### Changed

* Existing static nurbs curve translator re-uses common utils code.

### Fixed

* Windows build

## AL_USDMaya-0.27.8 (2018-05-02)

### Changed

* ExportCommand will now export time samples for the visibility attribute.
* ImportCommand will now correctly import time samples for the visibility attribute.

## AL_USDMaya-0.27.7 (2018-05-01)

### Added

* Experimental Windows support. It has been tested on Windows 7 with Visual Studio 2015 and Maya 2017.

### Removed

* Qt dependency (Thanks to Sebastien Dalgo from Autodesk)

## AL_USDMaya-0.27.6 (2018-04-13)

### Added

* New attribute "assetResolverConfig" on proxy shape
* Export creates master geometry prim thus each instance could reference. If transform and mesh nodes are separated by turning off "mergeTransform" option, transform prim will be marked as "instanceable" and fully benefits from usd instancing feature.
* Import detects shared master geometry prim, ensures maya transform nodes parenting the instancing shape.
* Layer manager is now serialised on file export

## AL_USDMaya-0.27.5 (2018-04-10)

### Added

* Add assetResolverConfig string attribute to ProxyShape

### Changed

* Built against USD-0.8.4
* Move initialisation of maya event handler from AL_USDMaya library to mayaUtils library

## AL_USDMaya-0.27.4 (2018-04-04)

### Added

* src/plugin/AL_USDMayaTestPlugin/AL/usdmaya/fileio/export_unmerged.cpp has been added, which tests the mergeTransforms parameter.

### Changed

* src/schemas/AL/usd/schemas/plugInfo.json.in had the metadata "al_usdmaya_mergedTransform" added.
* src/plugin/AL_USDMayaTestPlugin/AL/usdmaya/test_DiffPrimVar.cpp had the export parameter mergeTransforms changed to 1 to revert to previous test behaviour.
* fileio/Import.cpp will now check for the mergedTransform metadata on the parent transform, which prevents mesh import from creating the parent transform.
* fileio/Export.cpp has re-enabled functionality for "mergeTransforms", which will export the Xform prim and then the children mesh prims separately when set to '0'. The parent transform will have the metadata "al_usdmaya_mergedTransform = unmerged" added to tag it as an unmerged node.

## AL_USDMaya-0.27.3 (2018-03-29)

### Added

* Add -eac/-extensiveAnimationCheck option to AL_usdmaya_ExportCommand
* Add isAnimatedTransform() to AnimationTranslator to perform extensive animation check on transform nodes.

### Changed

* Updated Inactive/Active tests
* By default AL_usdmaya_ExportCommand will perform extensive animation check.
* Changed the signature of StageCache::Get(bool forcePopulate) to remove forcePopulate as it doesn't exist in the USDUtilsCache. Also affects Python bindings

### Fixed

* Mesh surface normals were not importing correctly

## AL_USDMaya-0.27.2 (2018-03-27)

### Added

* Internal AL build fixes
* Small fix to nurb width import

## AL_USDMaya-0.27.1 (2018-03-23)

### Added

* Updated existing command to add layer to the RootLayers sublayer stack
* Support relative path as usd file path, which will be resolved using current maya file path. It does nothing if current maya file path is none.
* Renamed glimpse subdivision related tokens (from glimpse_name to glimpse:subdiv:name)
* Nurb Curve widths now are imported and exported from AL_USDMaya's ImportCommand
* rendererPlugin attribute to ProxyShape
* Mesh Translation tutorial

### Changed

* Built using USD-0.8.3
* Library name AL_Utils -> AL_EventSystem
* Use custom data to store maya_associatedReferenceNode instead of attribute on the prim when import maya reference.
* AL_usdmaya_ProxyShapeImportPrimPathAsMaya, removed the parameters which weren't used.
* Allow for custom pxr namespace - https://github.com/AnimalLogic/AL_USDMaya/pull/68

### Fixed

* "Error : No proxyShape specified/selected " when attempting to add a sublayer via the UI
* The MPLug::source()  error in Maya 2016, the API is only available since Maya 2016 ex2.
* Failing unit test
* AL_usdmaya_TranslatePrim now ignores translation to an already translated prim
* Crash fix for MeshTranslator crash when variant switching
* Fixes several issues with selection in the maya viewport - https://github.com/AnimalLogic/AL_USDMaya/pull/42

## AL_USDMaya-0.27.0 (2018-03-12)

### Added
* Mesh Translation: Add support for glimpse user data attributes during import / export
* Added a Translate command "AL_usdmaya_TranslatePrim" that allows you to selectively run the Translator for a set of Prim Paths to either Import or Teardown the prim. Tutorial on the way!
* preTearDown writes Meshes Translated to Maya to EditTarget
* Library Refactor: Refactored to seperate code into multiple libraries: see change [DeveloperDocumentation](README.md#developer-documentation)


### Changed

* Tests: TranslatorContext.TranslatorContext was failing because there was an extra insert into the context in the MayaReference import. Also there was a filter on the tests which needed to be nuked!
* Docs: Small update to build.md
* Mesh Translation: updated glimpse subdivision attribute handling
* Store maya_associatedReferenceNode in custom data This is to stop USD from complaining about: "Error in 'pxrInternal_v0_8__pxrReserved__::UsdStage::_HandleLayersDidChange' at line 3355 in file stage.cpp : 'Detected usd threading violation.  Concurrent changes to layer(s) composed in stage 0x185f5a50 rooted at @usdfile.usda@.  (serial=6, lastSerial=12).'
* Updated AL_usdmaya_LayerCreateLayer command to add layers to Sublayers
* Proxy Shape and transform nodes names now match
* Update to USD-0.8.3


### Known Bugs/Limitations:
* Variant Switch with Maya Meshes causes a crash
* AL_usdmaya_TranslatePrim generates multiple copies of meshes
* AL_usdmaya_ProxyShapeImportPrimPathAsMaya not using new Mesh Translation functionality



## AL_USDMaya-0.26.1 (2018-03-02)

### Added

* Doxygen comments, tutorial and unit test for ModelAPI::GetLock(), ModelAPI::SetLock() and ModelAPI::ComputeLock().


### Changed

Change default lock behaviour of al_usdmaya_lock metadata. "transform" will lock current prim and all its children.
* GetSelectabilityValue method is now called GetSelectability
* Implementation of ComputeLock() is modified to reuse common code structure.
* Library name AL_Utils -> AL_EventSystem


### Fixed

* Issue with the ProxyShapeIimport command not loading payloads correctly.
* AL_USDMayaSchema's pixar plugin was not being loaded during it's tests
* Blocked the usd stage being reloaded when the filePath changes, when loading a maya file. This ensures all proxy shape attributes are initialised prior to reloading the stage.
* Crash when moving a locked prim
* Locking not working on maya hierarchies that appear and disappear on selection
* "Error : No proxyShape specified/selected " when attempting to add a sublayer via the UI

## AL_USDMaya-0.26.0 (2018-02-09)

### Added

* Overloads for the MPxTransform::isBounded() and MPxTransform::boundingBox() methods to provide the transforms with a bounding box to enable usage of frame selected
* ProxyShape::usdStage() to directly return the proxys shape stage without triggering a compute
* Extended Python bindings for ProxyShape class
* Flag to keep temporary test files generated by AL_USDMayaTestPlugin

### Changed

* Uses USD-0.8.2
* Reduced the overall number of pulls on the outStage data attribute
* Replaced the use of MMessage callbacks in favour of a scheduled system allowing control of maya events over a number of plugins and tools
* the -identifiers/-id flag is set.  This makes it more in line with all the other flags for the command (and more
  flexible), but is a change in behavior - previously, the muted layers would ALWAYS be returned as identifier
* Removed QT dependency in favour of Maya's keyboard getModifiers command
* Total revamp of how layers and edited layers are stored by AL_USDMaya

### Removed

* The insertFirst / insertLast concept in the maya events system

### Fixed

* Incorrect spot cone angles were being passed to Hydra in VP2
* Animated attributes could end up being added to the export list multiple times
* Ik chains needed to be inspected to ensure the animated values were correctly exported
* Animation caused via geometry constraints would not be exported
* regression that printed out various malformed sdfpath errors in unit tests
* Add AL pkgutil fallback for environments without pkg_resources
* ProxyShapeImport command now correctly takes `ul` parameter to direct if it should load payloads or not
* Bug where it was possible to select a transform outside the proxyshape using the 'up' key
* Bug with CommandGuiHelper where if it has errors building it would stay hidden, it now deletes and rebuilds itself
* Bug where addListOption would error if any list items had any characters that need to be quoted
* Crash when a prim is selected multiple times via command
* Crash if set xform values on an invalid prim
* Crash when pushToPrim is triggered but the prim is invalid
* Crash fix for when switching back + forth between a usd-camera and a maya-referenced camera

## AL_USDMaya-0.25.0 (2017-11-28)

### Added

* New metadata and logic to lock transform and disable pushToPrim.
* A boolean option "meshUV" or "muv" in AL_usdmaya_ExportCommand to indicate only export mesh uv data to a scene hierarchy with empty over prims.
* A boolean option "leftHandedUV" or "luv" in AL_usdmaya_ExportCommand to indicate whether to adjust uv indices orientation. This option only works with "meshUV"

### Changed

* Add special rules for some Maya-USD attribute type mismatches so the maya attribute values transfer correctly to USD prim, e.g. interpolateBoundary attribute on mesh.
* When translate, rotate, scale attributes are locked, TransformationMatrix blocks any changes of these attributes.
* In mesh import, leftHanded flag retrieved from USD will also decide if uv indices need be adjusted.

### Removed

* Host Driven Transforms

## AL_USDMaya-0.24.5 (2017-11-06)

### Fixed

* Remove leftover file

## AL_USDMaya-0.24.4 (2017-10-18)

### Changed

* Update to USD-0.8.1

## AL_USDMaya-0.24.3 (2017-10-18)

### Fixed

In same cases when there was geometry in VP2 behind some maya objects, some maya objects wouldn't be selected

## AL_USDMaya-0.24.2 (2017-10-17)

### Fixed

* Fix for [#24](https://github.com/AnimalLogic/AL_USDMaya/issues/24). This is really to avoid poping objects while selecting and does not represent a long term solution.
Selecting maya nodes that were outside of the proxyshape were causing a "Root path must be absolute (<>) " error.

## AL_USDMaya-0.24.1 (2017-10-10)

### Added

SelectionDB that stores all the paths that are selectable.

### Changed

Move all the selection commands into a different cpp file.

## AL_USDMaya-0.24.0 (2017-10-09)

### Fixed

* Update test_PrimFilter to not rely on external files

## AL_USDMaya-0.23.9 (2017-09-20)
### Added
* Added missing header files from nodes/proxy directory into released package
* AL_usdmaya_UsdDebugCommand to allow you to modify and query the TfDebug notifications
* Added a new Debug GUI that lets you enable/disable which TfDebug notices are output to the command prompt
* ProxyShape now calls ConfigureResolverForAsset with the USD file path that is being open, which is similar to what usdview does.

## AL_USDMaya-0.23.7 (2017-09-15)
### Fixed
* bug where enabling/disabling certain prims could cause an infinite loop.

## AL_USDMaya-0.23.6 (2017-09-12)
### Fixed
* Bug where if the variant selection is called twice before it gets processed

## AL_USDMaya-0.23.5
* Prevent viewport refresh at end of animation export that can cause a VP2 crash.

## AL_USDMaya-0.23.2
* Consolidated debug traces to use TF_DEBUG, see developers documentation for available flags.

## AL_USDMaya-0.23.1
* Bug Fix: Maya 2016 build.

## AL_USDMaya-0.22.1
* Bug Fix: Fixed playblast issue that caused drift in caches when using animated film offsets on the camera

## AL_USDMaya-0.22.0
* Bug Fix: Fixed crash in prim resync command when passed an invalid prim path
* Removed unused pxr directory
* Bug Fix: Schemas generation
* Bug Fix: the maya reference update code was double-loading references when switching paths.

## AL_USDMaya-0.21.0
* Removed legacy TRA Array attributes from the proxy shape
* Bug Fix: Fixed issue that could cause translator plugins to be uninitialised when proxy shape was first created
* Updated to USD 0.8.0
* Various cmake/build improvements

## AL_USDMaya-0.20.1
* Improvements to colour set export on mesh geometry
* Bug Fix: Force parallel evaluation when running unit tests
* Removed unused asset resolver config
* Bug Fix: Improve selection between maya geometry and usd geometry

## AL_USDMaya-0.20.0
* Hooked up display Guides + displayRenderGuides attributes to usdImaging
* Added MEL command to provide a simple selection mechanism in USD imaging layer
* Reduced the number of times the translators were being initialized to once per proxy shape instance.
* Added ability for the translators to say they support an inactive state (so nodes don't get deleted)
* Added reference counting for kRequired transforms.
* Bug Fix: If a prim changed type as a result of a variant switch, the type info was not updated in the translator context
* Bug Fix: Shapes, Transforms, and DG nodes now correctly deleted (without leaving transforms)
* Bug Fix: Fixed issue where we could end up with invalid transform references in some very rare edge cases
* Bug Fix: Fixed issue where invalid MObjects could be generated within transform reference generation.
* Bug Fix: Switching from a plugin prim type, to an ignored cache prim, could leave transforms in the scene
* Added the AL_usdmaya_ProxyShapeSelect command to select prims via a command (supports undo/redo).
* Promoted the CameraTranslator to a properly implemented translator plugin

## AL_USDMaya-0.19.0
* Added Open Source Licencing
* Documentation Refactor
* Addded End-to-End Tutorial
* Added MayaReference and HostDrivenTransformInfo schema (_the opensource repository is now deprecated_)
* Added MayaReference translator (_the opensource repository is now deprecated_)

## AL_USDMaya-0.18.1
**Change Log**
* Bug Fix: Fixed issue with custom transform types not being correctly serialised on file Save
* Bug Fix: Fixed issue with transforms with identical names not being correctly deserialised
* Added support for the Intel F16C half-float conversion intrinsics

## AL_USDMaya-0.18.0
**Change Log**
* AL_usdmaya_LayerCurrentEditTarget command can now take a layer identifier to specify the target layer
* New command AL_usdmaya_LayerCreateLayer added
* Updated docker config
* Bug Fix: Unchecked pointer access in SchemaNodeRefDB::removeEntries
* proxyShapeImport GUI now displays all usd file types
* Bug Fix: Colour sets now correctly applied on import
* Bug Fix: Maya 2018 compatibility changes

## AL_USDMaya-0.17.0
**Change Log**
* Built against usd-0.7.5
* Switching StageData to use a UsdStageWeakPtr to avoid keeping some stages alive forever.
* Copy the edit target instead and re-use it when setting it back.
* Add CHANGELOG.md
* Bug Fix: Fixes to make the unit tests run in mayabatch mode.
* Bug Fix: Use shared_ptr to manage driven transform data life time. [#263]
* Bug Fix: Fixing a possible crash with the curve importer.
* Bug Fix: Avoiding a crash when importing a camera.

## AL_USDMaya-0.16.9
**Change Log**
* Bug Fix: Previous selection crash-fix, that re-parented custom transforms under a temporary, would cause a change in the
           selection list, which resulted in a crash.
* Bug Fix: Fixed crash in removeUsdTransforms as a result of Maya deleting the parent of a custom transform
* Bug Fix: Hydra selection highlighting was being overwritten when performing Shift+Select
* The proxy shape now responds to changes of the Active state of plugin translator prims
* Maya 2016 now supported

## AL_USDMaya-0.16.8
**Change Log**
* Bug Fix: fixing a regression that would cause the transform hierarchy to be incorrect

## AL_USDMaya-0.16.7
**Change Log**
* Changes to the driven transforms on the proxy shape
* Ported from CPP unit to googletest, and moved all tests into a test plugin.
* Code now compiles against Maya 2018 beta77
* Bug Fix: session layer handle was incorrectly being wiped after a file load, which could cause a crash
* Bug Fix: Layer::getSubLayers would fail after the scene was reloaded.
* Bug Fix: Incorrectly warned of storable message attributes
* Bug Fix: Unitialised layer handles could cause a crash
* Bug Fix: Deleting AL_usdmaya_Transform nodes would cause parent transforms to be deleted.
* Bug Fix: Layer::getParentLayer returning invalid value
* Bug Fix: getAttr "layerNode.framePrecision" now works as expected
* Bug Fix: Chaning the USD edit target now correctly updates the 'hasBeenEditTarget' flag
* Bug Fix: Animated Shear
* Bug Fix: AL_usdmaya_TransformationMatrix::getTimeCode now correctly returns the animated time values
* Bug Fix: AL_usdmaya_TransformationMatrix::enablePushToPrim would incorrectly create a scalePivot transformation op
* Bug Fix: AL_usdmaya_TransformationMatrix could fail to update if frame 0 was the first animation frame in a sequence.
* Bug Fix: Selecting a parent of a selected transform, would cause a crash in Maya.

## AL_USDMaya-0.16.6
**Change Log**
* Bug Fix: Matrix driven transform node could write an invalid key into the session layer, nuking animation cache data.
* Bug Fix: Excluded geometry became visible on reload.
* Bug Fix: Removed option box from Import Proxy Shape (was causing a crash).
* Improvement: Proxy Shape now runs the post-load process immediately, rather than waiting on a defferred MEL call.

## AL_USDMaya-0.16.5
**Change Log**
* Ability to set Edit Targets with a map function
* Bug Fix: Edit Targets not correctly preserved during selection changes
* Bug Fix: Export command strips namespaces to avoid crash
* Bug Fix: Camera transforms now correctly animate
* readFromTimeline attribute added to AL_usdmaya_Transform to control when transforms display custom or animated values.

## AL_USDMaya-0.16.4
**Change Log**
* Bug Fix: playblasts were coming out black
* Bug Fix: OpenGL state not preserved in VP1

## AL_USDMaya-0.16.3
**Change Log**
* Bug Fix: excluded objects not hidden after file load
* Bug Fix: Prevented crash within draw override.

## AL_USDMaya-0.16.2
**Change Log**
* Bug Fix: excluded objects not hidden after variant switch

## AL_USDMaya-0.16.1
**Change Log**
* Bug Fix: Fixed incorrect depth settings when rendering in Hydra
* Bug Fix: Fixed complexity issue that caused warnings to be spammed into the command prompt.

## AL_USDMaya-0.16.0
**Change Log**
* Switched code over to use UsdImaging rather than UsdMayaGL library
* Updated USD base to 0.7.4
* Selection highlighting now visible in the maya viewport.

## AL_USDMaya-0.12.1
**Change Log**
* Variant Switching now supported
* Minor Menu GUI improvements
* Dead code removal
* open sourcing prep work, and documentation

## AL_USDMaya-0.9.14
**Change Log**
* Updated to support ALMayaReferences

**Known Issues**
* Having objects that have a MayaReference to the same path on disk has been reported to cause problems.

## AL_USDMaya-0.9.13
**Change Log**
* Disabled Asset Resolver Configuration

## AL_USDMaya-0.9.12
**Change Log**
* "add AL_USDMaya library to python bindings linked libraries"
* Importing of animated attributes

## AL_USDMaya-0.9.10
**Change Log**
* Updated the AL/__init__.py to merge in with the existing AL module to avoid  UsdMaya from stomping on PythongLibs' AL module.

## AL_USDMaya-0.9.8
**Change Log**
* Bug Fix: Scenes no longer crash in parallel evaluation mode. http://github.al.com.au/rnd/usdMaya/issues/41
* Bug Fix: Maya no longer crashes when modifying the filepath attribute of a proxyshape node. http://github.al.com.au/rnd/usdMaya/issues/121
* Bug Fix: Frame range incorrectly set when opening a shot. http://github.al.com.au/rnd/usdMaya/issues/72
* Bug Fix: maya crash if rig ref missing. http://github.al.com.au/rnd/usdMaya/issues/84
* Massive rename of nodes, and commands. The previous names (e.g. alUsdProxyShape) have been standardised with the rest of AL, so now it's 'AL_usdmaya_ProxyShape'.
* Support for the export of animated cameras via the 'AL usdmaya Export' translator
* The AL usdmaya plugin has been divorced from the orignal PXR maya plugin
* Remaining python code has been moved, previously you had to import the module by 'from pxrUsd import UsdMaya', now you should use 'from AL import UsdMaya'.

**Known issues**
* Missing usdImport and usdExport command for animated data - http://github.al.com.au/rnd/usdMaya/issues/108
* TLS Problem in maya 2017 - you may have to switch off other plugins when working with our USD Plugin. See https://groups.google.com/forum/#!topic/usd-interest/wJr8c_iTO7k

## AL_USDMaya-0.9.6
**Change Log**
* Prim->MayaPath: Prim's that translate into a corresponding maya shape now point to the transform above the shape instead of ths shape
* Enabled backface culling in the proxyshapeUi
* Fixed "picaso" bug

**Known issues**
* Missing usdImport and usdExport command for animated data - http://github.al.com.au/rnd/usdMaya/issues/108
* TLS Problem in maya 2017 - you may have to switch off other plugins when working with our USD Plugin. See https://groups.google.com/forum/#!topic/usd-interest/wJr8c_iTO7k
* DG Parallel Eval: http://github.al.com.au/rnd/usdMaya/issues/41
* Frame Range http://github.al.com.au/rnd/usdMaya/issues/72
* Crash if ref rig missing http://github.al.com.au/rnd/usdMaya/issues/84

## AL_USDMaya-0.9.5
**Change Log**
* Simple profiler
* Global Command Executor problems
* Removed pixar mayaUsd source that isn't being used

**Known issues**
* Missing usdImport and usdExport command for animated data - http://github.al.com.au/rnd/usdMaya/issues/108
* TLS Problem in maya 2017 - you may have to switch off other plugins when working with our USD Plugin. See https://groups.google.com/forum/#!topic/usd-interest/wJr8c_iTO7k
* DG Parallel Eval: http://github.al.com.au/rnd/usdMaya/issues/41
* Frame Range http://github.al.com.au/rnd/usdMaya/issues/72
* Crash if ref rig missing http://github.al.com.au/rnd/usdMaya/issues/84

## AL_USDMaya-0.9.4
**Change Log**
* New patch for Maya PR71

**Known issues**
* TLS Problem in maya 2017 - you may have to switch off other plugins when working with our USD Plugin. See https://groups.google.com/forum/#!topic/usd-interest/wJr8c_iTO7k
* DG Parallel Eval: http://github.al.com.au/rnd/usdMaya/issues/41
* Frame Range http://github.al.com.au/rnd/usdMaya/issues/72
* Crash if ref rig missing http://github.al.com.au/rnd/usdMaya/issues/84

## AL_USDMaya-0.9.3
**Change Log**
* Added all_tests target which will be the target ran by the usdMaya_BUILD jenkins job.
* BugFix: Fixed MayaCache: http://github.al.com.au/rnd/usdMaya/issues/37

**Known issues**
* TLS Problem in maya 2017 - you may have to switch off other plugins when working with our USD Plugin. See https://groups.google.com/forum/#!topic/usd-interest/wJr8c_iTO7k
* DG Parallel Eval: http://github.al.com.au/rnd/usdMaya/issues/41
* Frame Range http://github.al.com.au/rnd/usdMaya/issues/72
* Crash if ref rig missing http://github.al.com.au/rnd/usdMaya/issues/84

## AL_USDMaya-0.9.2
**Documentation**
http://github.al.com.au/rnd/usdMaya/wiki

**Change Log**
* Initial UsdMaya unit tests.
* Fix for rabbit vertex windings, rabbit now shows correctly as white.
* Fix for UsdPrim->usdMaya path.
* Fix for crash when burnin can't find parent path.
* Fix for multiple rabbits, now correctly shows one rabbit

**Known issues**
* TLS Problem in maya 2017 - you may have to switch off other plugins when working with our USD Plugin. See https://groups.google.com/forum/#!topic/usd-interest/wJr8c_iTO7k
* MayaCache: http://github.al.com.au/rnd/usdMaya/issues/37
* DG Parallel Eval: http://github.al.com.au/rnd/usdMaya/issues/41
* Frame Range http://github.al.com.au/rnd/usdMaya/issues/72
* Crash if ref rig missing http://github.al.com.au/rnd/usdMaya/issues/84
