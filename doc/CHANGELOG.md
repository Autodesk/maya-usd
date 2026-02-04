# Changelog

## [v0.34.0] - 2025-11-12

**Build:**
* Fix custom rig test [#4332](https://github.com/Autodesk/maya-usd/pull/4332)
* Support using python 3.13+ to run build.py [#4331](https://github.com/Autodesk/maya-usd/pull/4331)
* Add tests for generic getter and setter of Ufe::Atribute [#4322](https://github.com/Autodesk/maya-usd/pull/4322)
* Define static class member in impl file [#4302](https://github.com/Autodesk/maya-usd/pull/4302)
* Vp2RenderDelegate: Ensure the intended TBB version is picked [#4294](https://github.com/Autodesk/maya-usd/pull/4294)
* Remove Boost [#4286](https://github.com/Autodesk/maya-usd/pull/4286) [#4250](https://github.com/Autodesk/maya-usd/pull/4250)
* MayaUsd: Maya blessing broken after Qt 6.8.3 update [#4271](https://github.com/Autodesk/maya-usd/pull/4271)
* Updates for MaterialX 1.39.4 [#4268](https://github.com/Autodesk/maya-usd/pull/4268)
* Fix define typo [#4258](https://github.com/Autodesk/maya-usd/pull/4258)
* Future proof fileformat usage [#4220](https://github.com/Autodesk/maya-usd/pull/4220)
* Fixes for building with QT_DISABLE_DEPRECATED_BEFORE [#4219](https://github.com/Autodesk/maya-usd/pull/4219)
* Move deprecated Usd ZipFile-related codesites [#4201](https://github.com/Autodesk/maya-usd/pull/4201)
* Move deprecated Usd CreateInfo and file format codesites [#4200](https://github.com/Autodesk/maya-usd/pull/4200)
* Update Ndr callsite in test [#4199](https://github.com/Autodesk/maya-usd/pull/4199)

**Translation Framework:**
* Fix the issue that animated F-stop attribute is not importing when using spline [#4338](https://github.com/Autodesk/maya-usd/pull/4338)
* Fix the unknown warning when selecting USD export option [#4336](https://github.com/Autodesk/maya-usd/pull/4336)
* "Axis and Units Conversion" tab formatting does not adjust to window size [#4334](https://github.com/Autodesk/maya-usd/pull/4334)
* Adds Apply Euler Filter option to Import UI [#4327](https://github.com/Autodesk/maya-usd/pull/4327)
* Changed export material default option to MaterialX [#4315](https://github.com/Autodesk/maya-usd/pull/4315)
* Import support for left handed polygon meshes [#4305](https://github.com/Autodesk/maya-usd/pull/4305)
* Batched export and merge to USD [#4279](https://github.com/Autodesk/maya-usd/pull/4279)
* Add a hide-source-data flag to the MayaUSD export [#4261](https://github.com/Autodesk/maya-usd/pull/4261)
* Anim spline, fix vertical aperture incorrect unit [#4254](https://github.com/Autodesk/maya-usd/pull/4254)
* Animation Rotation Mismatch After USD to Maya Data Conversion [#4253](https://github.com/Autodesk/maya-usd/pull/4253)
* Some tangent are not translated correctly on export [#4248](https://github.com/Autodesk/maya-usd/pull/4248) [#4241](https://github.com/Autodesk/maya-usd/pull/4241)
* Rotation is incorrectly written on export to USD when using splines [#4240](https://github.com/Autodesk/maya-usd/pull/4240)
* Transforms are lost if objects contain transform data and Animation Curves is set on export [#4236](https://github.com/Autodesk/maya-usd/pull/4236)
* Texture paths are not saved as relative when exporting an object with MaterialX Document [#4234](https://github.com/Autodesk/maya-usd/pull/4234)
* Import Xform transformations with Animation Curves [#4232](https://github.com/Autodesk/maya-usd/pull/4232)
* Default Camera Attributes are not exported when using Animation Curve option [#4231](https://github.com/Autodesk/maya-usd/pull/4231)
* Crash if the file port is promoted to the top level of the graph [#4230](https://github.com/Autodesk/maya-usd/pull/4230)
* Export Xform transformations with Animation Curves [#4229](https://github.com/Autodesk/maya-usd/pull/4229)
* Support Import for spline attribute of USD Lights [#4228](https://github.com/Autodesk/maya-usd/pull/4228)
* Ability to export light animation curves [#4225](https://github.com/Autodesk/maya-usd/pull/4225)
* Mtlx-USD export should add ND_geompropvalue node to the image node during convertion [#4217](https://github.com/Autodesk/maya-usd/pull/4217)
* Add internal Maya light shading to USD lights [#4222](https://github.com/Autodesk/maya-usd/pull/4222)
* Support import camera animation curves [#4216](https://github.com/Autodesk/maya-usd/pull/4216)
* Wrong/Missing info on Camera spline exporting properties [#4214](https://github.com/Autodesk/maya-usd/pull/4214)

**Workflow:**
* Layer Editor data window performance improvements [#4335](https://github.com/Autodesk/maya-usd/pull/4335)
* Update styling in our layer content view [#4333](https://github.com/Autodesk/maya-usd/pull/4333)
* Add Variable Expression Support to Layer editor [#4321](https://github.com/Autodesk/maya-usd/pull/4321)
* Adds an envionment variable to disable layer auto targeting [#4308](https://github.com/Autodesk/maya-usd/pull/4308)
* Ufe
    * Extract UsdSceneItemOps to the USDUFE lib [#4307](https://github.com/Autodesk/maya-usd/pull/4307)
    * Adds UFE Light2 interface support for Area Lights [#4297](https://github.com/Autodesk/maya-usd/pull/4297)
    * Echo UFE Commands to Script Editor [#4290](https://github.com/Autodesk/maya-usd/pull/4290) [#4289](https://github.com/Autodesk/maya-usd/pull/4289)
    * UsdUfe: move delete and rename commands from MayaUsd [#4266](https://github.com/Autodesk/maya-usd/pull/4266)
    * `UsdAttributeUInt`: unsigned int ufe type support [#4211](https://github.com/Autodesk/maya-usd/pull/4211)
* Implement dstParentPath duplicate option of BatchOpsHandler [#4296](https://github.com/Autodesk/maya-usd/pull/4296)
* Fix reparent in the session layer [#4292](https://github.com/Autodesk/maya-usd/pull/4292)
* Show Layer Data Window [#4291](https://github.com/Autodesk/maya-usd/pull/4291)
* Pulling then Pushing changes to transform curves won't change back usd data [#4287](https://github.com/Autodesk/maya-usd/pull/4287)
* Fix duplicate not duplicating connections to NodeGraph outputs [#4280](https://github.com/Autodesk/maya-usd/pull/4280)
* Keep session data in the session layer [#4278](https://github.com/Autodesk/maya-usd/pull/4278)
* Performance is slow with variants on certain scene assets [#4275](https://github.com/Autodesk/maya-usd/pull/4275)
* ProgressBar: Prevent premature completion and warnings [#4272](https://github.com/Autodesk/maya-usd/pull/4272)
* Fixes layer auto switching when the current target isn't locked [#4269](https://github.com/Autodesk/maya-usd/pull/4269)
* Add asset info section to the attribute editor [#4265](https://github.com/Autodesk/maya-usd/pull/4265)
* Apply color management file rules on setValue [#4262](https://github.com/Autodesk/maya-usd/pull/4262)
* Some tangent usage are not translated correctly in 25.05 on Stage Creation [#4252](https://github.com/Autodesk/maya-usd/pull/4252)
* Outliner icons and add class prim contexts menu [#4260](https://github.com/Autodesk/maya-usd/pull/4260)
* Material prims break AE on Maya 2023 / 2022 [#4243](https://github.com/Autodesk/maya-usd/pull/4243)

**Render:**
* Provide nice names to top level NodeDef classifications [#4323](https://github.com/Autodesk/maya-usd/pull/4323)
* Initialize PxrGL HdCamera attr value cache [#4298](https://github.com/Autodesk/maya-usd/pull/4298)
* Bracket code backported from MaterialX 1.39.X [#4274](https://github.com/Autodesk/maya-usd/pull/4274)
* Move ProxyShapeLookdevHandler to MayaUSD [#4244](https://github.com/Autodesk/maya-usd/pull/4244)
* Added support for USD Domelight_1 [#4227](https://github.com/Autodesk/maya-usd/pull/4227)
* Use new wireframe draw modes [#4218](https://github.com/Autodesk/maya-usd/pull/4218)

**Shared Components:**
* Asset Resolver
    * Asset Resolver Search Path preferences UI [#4337](https://github.com/Autodesk/maya-usd/pull/4337)
    * Asset Resolver Mapping file preference UI [#4318](https://github.com/Autodesk/maya-usd/pull/4318)
    * Register Maya Project Tokens to USD AssetResolver and Asset Resolver Presets [#4282](https://github.com/Autodesk/maya-usd/pull/4282)

**Miscellaneous:**
* Temporarily use default ar for export pacakge test [#4339](https://github.com/Autodesk/maya-usd/pull/4339)
* Fix AE template that Usd v22.11 Usd.PrimDefinition doesn't have method called GetAttributeDefinition [#4329](https://github.com/Autodesk/maya-usd/pull/4329)
* Migrate deprecated pxr/usd/usd file format utils [#4319](https://github.com/Autodesk/maya-usd/pull/4319)
* Always enable testUsdExportRenderLayerMode [#4300](https://github.com/Autodesk/maya-usd/pull/4300)
* API doc is no longer included schema's "doc" field [#4256](https://github.com/Autodesk/maya-usd/pull/4256)
* Recognize new default sRGB color space name [#4239](https://github.com/Autodesk/maya-usd/pull/4239)

## [v0.33.0] - 2025-08-06

**Build:**
* Fix define typo [#4258](https://github.com/Autodesk/maya-usd/pull/4258)
* API doc is no longer included schema's "doc" field [#4256](https://github.com/Autodesk/maya-usd/pull/4256)
* Recognize new default sRGB color space name [#4239](https://github.com/Autodesk/maya-usd/pull/4239)
* Future proof fileformat usage [#4220](https://github.com/Autodesk/maya-usd/pull/4220)
* Fixes for building with QT_DISABLE_DEPRECATED_BEFORE [#4219](https://github.com/Autodesk/maya-usd/pull/4219)
* Move deprecated Usd ZipFile-related codesites [#4201](https://github.com/Autodesk/maya-usd/pull/4201)
* Move deprecated Usd CreateInfo and file format codesites [#4200](https://github.com/Autodesk/maya-usd/pull/4200)
* Update Ndr callsite in test [#4199](https://github.com/Autodesk/maya-usd/pull/4199)

**Translation Framework:**
* Anim spline, fix vertical aperture incorrect unit [#4254](https://github.com/Autodesk/maya-usd/pull/4254)
* Some tangent are not translated correctly on export [#4248](https://github.com/Autodesk/maya-usd/pull/4248)
* Some tangent usage are not translated correctly in 25.05 on Stage Creation [#4241](https://github.com/Autodesk/maya-usd/pull/4241)
* Rotation is incorrectly written on export to USD when using splines [#4240](https://github.com/Autodesk/maya-usd/pull/4240)
* Transforms are lost If objects contain transform data and Animation Curves is set on export [#4236](https://github.com/Autodesk/maya-usd/pull/4236)
* Texture paths are not saved as relative when exporting an object with MaterialX Document [#4234](https://github.com/Autodesk/maya-usd/pull/4234)
* Import Xform transformations with Animation Curves [#4232](https://github.com/Autodesk/maya-usd/pull/4232)
* Default Camera Attributes are not exported when using Animation Curve option [#4231](https://github.com/Autodesk/maya-usd/pull/4231)
* Crash if the file port is promoted to the top level of the graph [#4230](https://github.com/Autodesk/maya-usd/pull/4230)
* Export Xform transformations with Animation Curves [#4229](https://github.com/Autodesk/maya-usd/pull/4229)
* Support Import for spline attribute of USD Lights [#4228](https://github.com/Autodesk/maya-usd/pull/4228)
* Ability to export light animation curves [#4225](https://github.com/Autodesk/maya-usd/pull/4225)
* Mtlx-USD export should add ND_geompropvalue node to the image node during convertion [#4217](https://github.com/Autodesk/maya-usd/pull/4217)
* Support import camera animation curves [#4216](https://github.com/Autodesk/maya-usd/pull/4216)
* Fix Wrong/Missing info on Camera spline exporting properties [#4214](https://github.com/Autodesk/maya-usd/pull/4214)

**Workflow:**
* Material prims break AE on Maya 2023 / 2022 [#4243](https://github.com/Autodesk/maya-usd/pull/4243)
* `UsdAttributeUInt`: unsigned int ufe type support [#4211](https://github.com/Autodesk/maya-usd/pull/4211)

**Render:**
* Move ProxyShapeLookdevHandler to MayaUSD [#4244](https://github.com/Autodesk/maya-usd/pull/4244) [#4238](https://github.com/Autodesk/maya-usd/pull/4238)
* Added support for USD Domelight_1 [#4227](https://github.com/Autodesk/maya-usd/pull/4227)
* Add internal Maya light shading to USD lights [#4222](https://github.com/Autodesk/maya-usd/pull/4222)
* Use new wireframe draw modes [#4218](https://github.com/Autodesk/maya-usd/pull/4218)

## [v0.32.0] - 2025-06-04

**Build:**
* Update diffCore.cpp [#4188](https://github.com/Autodesk/maya-usd/pull/4188)
* Add ABI checker to MayaUsd preflight to verify MayaUsdAPI [#4169](https://github.com/Autodesk/maya-usd/pull/4169)
* Use new Sdr API [#4141](https://github.com/Autodesk/maya-usd/pull/4141)
* Add LookdevXUsd unit tests [#4113](https://github.com/Autodesk/maya-usd/pull/4113)
* MayaUsd: rerun failed tests to make random failures more stable [#4090](https://github.com/Autodesk/maya-usd/pull/4090)
* Remove all the UFE_PREVIEW_VERSION_NUM v5/v6 checks [#4104](https://github.com/Autodesk/maya-usd/pull/4104)

**Translation Framework:**
* Hidden LookdevX nodes are exposed when exporting Mtlx to USD stage [#4191](https://github.com/Autodesk/maya-usd/pull/4191)
* Added export splines anim for camera attributes [#4164](https://github.com/Autodesk/maya-usd/pull/4164)
* Removing UsdLuxGeometryLight reader [#4147](https://github.com/Autodesk/maya-usd/pull/4147)
* Avoid crash when shader has no uvLink [#4136](https://github.com/Autodesk/maya-usd/pull/4136)
* Export instances with exportRoots [#4118](https://github.com/Autodesk/maya-usd/pull/4118)
* Support MaterialX document in USD Exporter [#4107](https://github.com/Autodesk/maya-usd/pull/4107)
* ShadingModeExporter: Fix potential crash when exporting collection-based material bindings [#4106](https://github.com/Autodesk/maya-usd/pull/4106)

**Workflow:**
* Layer Editor
    * Disables cache rebuild due to layer editor [#4202](https://github.com/Autodesk/maya-usd/pull/4202)
    * Fix: Clear Layer not undoable after save on not dirty layer. Wrong focus button for Reload Layer dialog. Layer editor pin stage not respected randomly when list changes [#4196](https://github.com/Autodesk/maya-usd/pull/4196)
    * Fixed a bug that Layer Editor Won't Launch [#4195](https://github.com/Autodesk/maya-usd/pull/4195)
    * Avoids rebuilding cache when trying to getProxyShape [#4190](https://github.com/Autodesk/maya-usd/pull/4190)
    * Remember expand/collapse state of layers. Middle Truncation for Layer Names. New Pin Tooltip. Query Selected Layers. [#4168](https://github.com/Autodesk/maya-usd/pull/4168)

* Properly handle deleted attributes [#4186](https://github.com/Autodesk/maya-usd/pull/4186)
* Add logic to distinguish between system and regular locks for layer editing error message [#4185](https://github.com/Autodesk/maya-usd/pull/4185)
* Check that the stagePath points to a valid Ufe item [#4184](https://github.com/Autodesk/maya-usd/pull/4184)
* Creating or parenting a camera under a USD def will crash Maya [#4183](https://github.com/Autodesk/maya-usd/pull/4183)
* Show Def in AE [#4081](https://github.com/Autodesk/maya-usd/pull/4081)
* Duplicating a ProxyStage and hiding the dup causes crash [#4180](https://github.com/Autodesk/maya-usd/pull/4180)
* Add Outliner icons for NodeGraphs [#4172](https://github.com/Autodesk/maya-usd/pull/4172)
* Port over recent layer editor changes [#4168](https://github.com/Autodesk/maya-usd/pull/4168)
* Fixes a performance issue regarding findGatwayItems() / cameras [#4158](https://github.com/Autodesk/maya-usd/pull/4158)
* UsdUfe.registerUICallback can crash with python lambdas [#4155](https://github.com/Autodesk/maya-usd/pull/4155)
* PrimUpdaterManager: Avoid spurious `allowTopologyMod` reference edits [#4153](https://github.com/Autodesk/maya-usd/pull/4153)
* Adding or removing of items in collections should respect the edit restrictions as other attributes [#4143](https://github.com/Autodesk/maya-usd/pull/4143)
* Adds a cutom gizmo shape and geometry [#4140](https://github.com/Autodesk/maya-usd/pull/4140)
* Make materialX bindings accessible in Maya [#4137](https://github.com/Autodesk/maya-usd/pull/4137)
* Fix schemas in AE [#4131](https://github.com/Autodesk/maya-usd/pull/4131)
* Duplicate material to USD with Arnold [#4124](https://github.com/Autodesk/maya-usd/pull/4124)
* Honor the target-session option var [#4120](https://github.com/Autodesk/maya-usd/pull/4120)
* Applied schemas in AE [#4114](https://github.com/Autodesk/maya-usd/pull/4114)
* Make USD camera invisible when the parents turn invisible [#4110](https://github.com/Autodesk/maya-usd/pull/4110)
* Support grouped instance in dup to USD [#4108](https://github.com/Autodesk/maya-usd/pull/4108)
* Disable edit as maya for geometry light [#4056](https://github.com/Autodesk/maya-usd/pull/4056)

**Render:**
* Fixes missing color settings [#4189](https://github.com/Autodesk/maya-usd/pull/4189)
* Fixed the bug that Selectable imaging node caused issues with XRay shading [#4146](https://github.com/Autodesk/maya-usd/pull/4146)
* Update to MaterialX 1.39.3 [#4160](https://github.com/Autodesk/maya-usd/pull/4160)
* Fixes viewport filter for USD lights [#4159](https://github.com/Autodesk/maya-usd/pull/4159)
* Light Gizmo workaround for Rect Light / Portal Light [#4152](https://github.com/Autodesk/maya-usd/pull/4152)
* Improve reliability of OpenPBR VP2 test [#4138](https://github.com/Autodesk/maya-usd/pull/4138)
* Deprecating enableIdRender UsdImagingGLRenderParams [#4109](https://github.com/Autodesk/maya-usd/pull/4109)
* Fix double texture loading when using Hydra [#4094](https://github.com/Autodesk/maya-usd/pull/4094)

**Shared Components:**
* Collections:
    * "Remove selected" also deselects items from the opposite list [#4187](https://github.com/Autodesk/maya-usd/pull/4187)
    * Help link to point to training docs for Collections [#4171](https://github.com/Autodesk/maya-usd/pull/4171)
    * Fix multiple collection UI issues [#4130](https://github.com/Autodesk/maya-usd/pull/4130)
    * Support the collection UI in Maya 2023 [#4101](https://github.com/Autodesk/maya-usd/pull/4101)
    * Print collection membership [#4100](https://github.com/Autodesk/maya-usd/pull/4100)
    * Save collection list view size [#4086](https://github.com/Autodesk/maya-usd/pull/4086)
    * Adjust collection UI spacing [#4080](https://github.com/Autodesk/maya-usd/pull/4080)
    * Add the selection to a collection [#4076](https://github.com/Autodesk/maya-usd/pull/4076)

* Adjust some sizes of the light linking widget to better match the design [#4154](https://github.com/Autodesk/maya-usd/pull/4154)
* Add remove from both menu item [#4132](https://github.com/Autodesk/maya-usd/pull/4132)
* Set status tip when setting tool tips [#4103](https://github.com/Autodesk/maya-usd/pull/4103)
* Bringing code back from shared repo [#4102](https://github.com/Autodesk/maya-usd/pull/4102)
* Fix warning icon not disappearing [#4099](https://github.com/Autodesk/maya-usd/pull/4099)
* Fix collections for Maya 2023 [#4098](https://github.com/Autodesk/maya-usd/pull/4098)
* Sync drag and drop from max [#4097](https://github.com/Autodesk/maya-usd/pull/4097)
* Detect data conflicts [#4093](https://github.com/Autodesk/maya-usd/pull/4093)
* Clear include/exclude opinions [#4091](https://github.com/Autodesk/maya-usd/pull/4091)
* Sync internal Include/Exclude editor [#4089](https://github.com/Autodesk/maya-usd/pull/4089)
* Select include/exclude or expression items [#4088](https://github.com/Autodesk/maya-usd/pull/4088)
* Multiple small UI fixes [#4083](https://github.com/Autodesk/maya-usd/pull/4083)

**Documentation:**
* Update README.md [#4179](https://github.com/Autodesk/maya-usd/pull/4179) [#4161](https://github.com/Autodesk/maya-usd/pull/4161) [#4161](https://github.com/Autodesk/maya-usd/pull/4161)
* Publish USD v24.11 branch [#4170](https://github.com/Autodesk/maya-usd/pull/4170)
* Add details to the axis and units import flag doc [#4149](https://github.com/Autodesk/maya-usd/pull/4149)
* Add help for create stage option box [#4111](https://github.com/Autodesk/maya-usd/pull/4111)

## [v0.31.0] - 2025-03-07

**Build:**
* Update reference images for a fix in OpenPBR native [#4051](https://github.com/Autodesk/maya-usd/pull/4051)
* Ignore Clang Format for LookdevXUsd folder [#4042](https://github.com/Autodesk/maya-usd/pull/4042)
* Re-enable light test for USD 24.11 [#4038](https://github.com/Autodesk/maya-usd/pull/4038)
* Removing deprecated renderman attributes from test .ma file [#4035](https://github.com/Autodesk/maya-usd/pull/4035)
* Disable new light test on USD 24.11 [#4033](https://github.com/Autodesk/maya-usd/pull/4033)
* Support for USD v24.11
  * Initial support for USD v24.11 [#4024](https://github.com/Autodesk/maya-usd/pull/4024) [#4021](https://github.com/Autodesk/maya-usd/pull/4021) [#3992](https://github.com/Autodesk/maya-usd/pull/3992)
* Fix IOR image test [#4019](https://github.com/Autodesk/maya-usd/pull/4019)
* Fixes an AL Plugin test due to a node name change [#3977](https://github.com/Autodesk/maya-usd/pull/3977)
* Make unit test scene file names unique [#3970](https://github.com/Autodesk/maya-usd/pull/3970)
* Fix bad merge in string resources [#3964](https://github.com/Autodesk/maya-usd/pull/3964)
* Fix Cmake configure overwriting plugInfo.json [#3958](https://github.com/Autodesk/maya-usd/pull/3958)

**Translation Framework:**
* Removing some readers for Renderman-for-Maya lights [#4049](https://github.com/Autodesk/maya-usd/pull/4049)
* Expose export job args to Python [#4041](https://github.com/Autodesk/maya-usd/pull/4041)
* Fix export roots when no default prim is specified [#3993](https://github.com/Autodesk/maya-usd/pull/3993)
* Implement OpenPBR import/export [#3990](https://github.com/Autodesk/maya-usd/pull/3990)
* Namespace in USD default prim export argument [#3989](https://github.com/Autodesk/maya-usd/pull/3989)
* No save during export [#3982](https://github.com/Autodesk/maya-usd/pull/3982)
* Add support for decimeters in export [#3976](https://github.com/Autodesk/maya-usd/pull/3976)
* Automatically close prefs when changing units [#3960](https://github.com/Autodesk/maya-usd/pull/3960)
* Units prefs during import [#3956](https://github.com/Autodesk/maya-usd/pull/3956)

**Workflow:**
* Make job context UI registeration be order-neutral [#4054](https://github.com/Autodesk/maya-usd/pull/4054)
* Better support for grouping prims with references [#4048](https://github.com/Autodesk/maya-usd/pull/4048)
* EMSUSD_1917 lock layer should preserve the selection [#4044](https://github.com/Autodesk/maya-usd/pull/4044)
* Fix UsdMayaPrimWriter and UsdMayaPrimReader Python Wrappings [#4037](https://github.com/Autodesk/maya-usd/pull/4037)
* Update change processing code [#4036](https://github.com/Autodesk/maya-usd/pull/4036)
* Explicitly add required export arg [#4034](https://github.com/Autodesk/maya-usd/pull/4034)
* Add textured mode handling to topo grapher [#4031](https://github.com/Autodesk/maya-usd/pull/4031)
* Shorter nice names [#4029](https://github.com/Autodesk/maya-usd/pull/4029)
* Don't dirty stage when loading [#4028](https://github.com/Autodesk/maya-usd/pull/4028)
* LayerManager: Prevent unintended overwriting of unmodified USD layers at Maya save [#4027](https://github.com/Autodesk/maya-usd/pull/4027)
* Fix crashes when saving layers to USD files in specific edge cases [#4018](https://github.com/Autodesk/maya-usd/pull/4018)
* Make the collection widget support all collections [#4017](https://github.com/Autodesk/maya-usd/pull/4017)
* Disable EditAsMaya for cylinder, disk and dome light [#4011](https://github.com/Autodesk/maya-usd/pull/4011)
* Make group names more unique [#4010](https://github.com/Autodesk/maya-usd/pull/4010)
* Improve stage edit target updates with refreshSystemLock callbacks [#4009](https://github.com/Autodesk/maya-usd/pull/4009)
* Remove schema from prims [#4008](https://github.com/Autodesk/maya-usd/pull/4008)
* Shader parameter sections should be expanded [#4006](https://github.com/Autodesk/maya-usd/pull/4006)
* Manipulate prim schemas [#4004](https://github.com/Autodesk/maya-usd/pull/4004)
* Add a class-prims filter to USD UFE [#3987](https://github.com/Autodesk/maya-usd/pull/3987)
* Camera Performance Fix Backport [#3986](https://github.com/Autodesk/maya-usd/pull/3986)
* Fix node origin detection [#3984](https://github.com/Autodesk/maya-usd/pull/3984)
* Correct matrix manipulation [#3966](https://github.com/Autodesk/maya-usd/pull/3966)
* Find the correct layer manager node [#3965](https://github.com/Autodesk/maya-usd/pull/3965)
* No metadata on read-only layers [#3962](https://github.com/Autodesk/maya-usd/pull/3962)
* Attribute Editor
  * Fix AE extra attribute section [#4025](https://github.com/Autodesk/maya-usd/pull/4025)
  * Fix custom AE callbacks [#4002](https://github.com/Autodesk/maya-usd/pull/4002)
  * Fix crash in Attribute Editor when renaming a prim with pulled descendants [#3979](https://github.com/Autodesk/maya-usd/pull/3979)
  * Modify AE section order [#3975](https://github.com/Autodesk/maya-usd/pull/3975)
  * Support custom callback for primitives AE template [#3972](https://github.com/Autodesk/maya-usd/pull/3972)
  * Refactor AE template [#3969](https://github.com/Autodesk/maya-usd/pull/3969)
* Axis and Units
  * Adjust axis and units section indent [#3983](https://github.com/Autodesk/maya-usd/pull/3983)
  * Author metadata only on anonymous root layers [#3974](https://github.com/Autodesk/maya-usd/pull/3974)

**Render:**
* Do not contaminate HdStorm libraries with lobe pruning [#4066](https://github.com/Autodesk/maya-usd/pull/4066)
* Lobe pruning shader optimization [#4016](https://github.com/Autodesk/maya-usd/pull/4016)
* Add option to fallback to mayaDefaultStandardSurface shader fragment [#3957](https://github.com/Autodesk/maya-usd/pull/3957)
* Optimize VP2 rendering of USD objects with geometric cut-outs [#3952](https://github.com/Autodesk/maya-usd/pull/3952)
* Fix VP2 rendering of UsdPreviewSurface with opacityThreshold [#3947](https://github.com/Autodesk/maya-usd/pull/3947)
* Material
  * Optimize material libraries [#4055](https://github.com/Autodesk/maya-usd/pull/4055)
  * Adjust test following renaming of the default OpenPBR material [#4005](https://github.com/Autodesk/maya-usd/pull/4005)
  * Copies nodegraph input defaultgeomprop values during TopoNeutralGraph construction [#3980](https://github.com/Autodesk/maya-usd/pull/3980)
  * Adds support for defaultgeomprop values on nodegraph inputs [#3968](https://github.com/Autodesk/maya-usd/pull/3968)

**Shared Components:**
* Implement theme scale in Maya Host [#4075](https://github.com/Autodesk/maya-usd/pull/4075)
* Report error for invalid include and exclude [#4074](https://github.com/Autodesk/maya-usd/pull/4074)
* Collection tab look [#4072](https://github.com/Autodesk/maya-usd/pull/4072)
* Bring light linking changes from the shared repo [#4071](https://github.com/Autodesk/maya-usd/pull/4071)
* Light linking undo and redo [#4067](https://github.com/Autodesk/maya-usd/pull/4067)
* Fix expression UI [#4065](https://github.com/Autodesk/maya-usd/pull/4065)
* Bring changes back from other light linking repo [#4064](https://github.com/Autodesk/maya-usd/pull/4064)
* Collections: add option to remove all includes and excludes [#4058](https://github.com/Autodesk/maya-usd/pull/4058)
* Refactor collection UI [#4053](https://github.com/Autodesk/maya-usd/pull/4053)
* Fix minimum size and mouse scroll issue [#4052](https://github.com/Autodesk/maya-usd/pull/4052)
* Light linking sync [#4046](https://github.com/Autodesk/maya-usd/pull/4046)
* Drag and drop one or more scene objects from the outliner into the include or exclude list [#4030](https://github.com/Autodesk/maya-usd/pull/4030)
* Add filtering search to the collection widget [#4023](https://github.com/Autodesk/maya-usd/pull/4023)
* Support viewing and set the expansion rule of the collection [#4022](https://github.com/Autodesk/maya-usd/pull/4022)
* Determine whether a collection includes all objects by default or not [#3988](https://github.com/Autodesk/maya-usd/pull/3988)

**Documentation:**
* Document how to get nice undo labels [#4070](https://github.com/Autodesk/maya-usd/pull/4070)
* Update README.md [#4032](https://github.com/Autodesk/maya-usd/pull/4032)
* Update maya-usd Readme.md [#4026](https://github.com/Autodesk/maya-usd/pull/4026)

**Miscellaneous:**
* Fix fatal error exit [#4043](https://github.com/Autodesk/maya-usd/pull/4043)
* Adds LookdevXUsd Extension [#4040](https://github.com/Autodesk/maya-usd/pull/4040)
* Fix warnings about the layer manager attribute [#3994](https://github.com/Autodesk/maya-usd/pull/3994)
* Prevent warnings when reloading the plugin [#3959](https://github.com/Autodesk/maya-usd/pull/3959)

## [v0.30.0] - 2024-10-16

**Build:**
* Trim down the UsdUfe linking to only what is needed [#3909](https://github.com/Autodesk/maya-usd/pull/3909)
* Preparing for USD v24.08 build (with OneTBB)
  * Build USD v24.08 (with oneTBB) [#3908](https://github.com/Autodesk/maya-usd/pull/3908)
  * Build with USD v24.08 [#3891](https://github.com/Autodesk/maya-usd/pull/3891)
* Unit test for multi export chasers with multi job contexts [#3897](https://github.com/Autodesk/maya-usd/pull/3897)
* Updating callsites to use new NdrSdfTypeIndicator class [#3890](https://github.com/Autodesk/maya-usd/pull/3890)
* Add required includes explicitly [#3862](https://github.com/Autodesk/maya-usd/pull/3862)
* Remove unused ghc include [#3848](https://github.com/Autodesk/maya-usd/pull/3848)
* Explicitly setting legacyMaterialScope arg in tests [#3819](https://github.com/Autodesk/maya-usd/pull/3819)

**Translation Framework:**
* Support relative references when export stages [#3900](https://github.com/Autodesk/maya-usd/pull/3900)
* USD Support for Maya Dual Quaternion skinned shapes [#3886](https://github.com/Autodesk/maya-usd/pull/3886)
* Generate material scope to be under the default prim that gets automatically determined [#3867](https://github.com/Autodesk/maya-usd/pull/3867)
* Improve curve import sanity checking [#3858](https://github.com/Autodesk/maya-usd/pull/3858)
* Fix the bug that default prim does not retain [#3849](https://github.com/Autodesk/maya-usd/pull/3849)
* Fixed accessing JobExportArgs TfToken arguments in Python [#3845](https://github.com/Autodesk/maya-usd/pull/3845)
* Save before export [#3843](https://github.com/Autodesk/maya-usd/pull/3843)
* Export stages as references [#3839](https://github.com/Autodesk/maya-usd/pull/3839)
* Export of Maya skeleton as USD comes in as a stage in the wrong place [#3820](https://github.com/Autodesk/maya-usd/pull/3820)

**Workflow:**
* Fix copying grouped meshes [#3906](https://github.com/Autodesk/maya-usd/pull/3906)
* Save and restore non local edit target layer and anonymous layer  [#3904](https://github.com/Autodesk/maya-usd/pull/3904)
* Update MaterialX node UInames [#3903](https://github.com/Autodesk/maya-usd/pull/3903)
* Merge job context arguments with user arguments [#3899](https://github.com/Autodesk/maya-usd/pull/3899)
* USD Lights:
  * Add more light shapes supports in the viewport [#3896](https://github.com/Autodesk/maya-usd/pull/3896)
  * Add fallback light type for unsupported lights [#3883](https://github.com/Autodesk/maya-usd/pull/3883)
* Update AE interface for Pxr Reference Assembly nodes [#3889](https://github.com/Autodesk/maya-usd/pull/3889)
* Support selection in duplicate to USD [#3884](https://github.com/Autodesk/maya-usd/pull/3884)
* Duplicate materials without meshes [#3876](https://github.com/Autodesk/maya-usd/pull/3876)
* Fix multi-layer parenting [#3874](https://github.com/Autodesk/maya-usd/pull/3874)
* Improve UI name generation [#3871](https://github.com/Autodesk/maya-usd/pull/3871)
* Don't set kind when editing as Maya [#3868](https://github.com/Autodesk/maya-usd/pull/3868)
* Fix outliner set text colour on prims [#3853](https://github.com/Autodesk/maya-usd/pull/3853)
* Support users naming node "world"
  * Nothing prevents users from naming nodes "world" [#3852](https://github.com/Autodesk/maya-usd/pull/3852)
  * Adapt to Maya Ufe world node change [#3842](https://github.com/Autodesk/maya-usd/pull/3842) 
* Improve Maya pivot support [#3851](https://github.com/Autodesk/maya-usd/pull/3851)
* Add Reload References Command [#3844](https://github.com/Autodesk/maya-usd/pull/3844)
* Write Autodesk metadata to the session layer [#3841](https://github.com/Autodesk/maya-usd/pull/3841)
* API implementation of UFE SceneSegmentHandler getter method for the dcc root path [#3834](https://github.com/Autodesk/maya-usd/pull/3834)
* Push/pull: pushing skel data back to USD can put objects in wrong position [#3826](https://github.com/Autodesk/maya-usd/pull/3826)
* Fix display layer loss when merging/duplicating to USD [#3807](https://github.com/Autodesk/maya-usd/pull/3807)

**Render:**
* Fix transmission issues [#3857](https://github.com/Autodesk/maya-usd/pull/3857)
* Fixing renamed parameters [#3833](https://github.com/Autodesk/maya-usd/pull/3833)
* Fix diffuse lighting with aiAreaLight [#3830](https://github.com/Autodesk/maya-usd/pull/3830)

**Miscellaneous:**
* Fix materials options retention [#3894](https://github.com/Autodesk/maya-usd/pull/3894)
* Remove matrix multiply operator overloads that potentially lose [#3888](https://github.com/Autodesk/maya-usd/pull/3888)
* Changed tsverify to manual nullptr check [#3875](https://github.com/Autodesk/maya-usd/pull/3875)
* MayaUsdAPI - enhance with missing functions required for LookdevX [#3869](https://github.com/Autodesk/maya-usd/pull/3869)
* Default prim tooltip [#3863](https://github.com/Autodesk/maya-usd/pull/3863)
* Same-name Python edit routers [#3856](https://github.com/Autodesk/maya-usd/pull/3856)
* UsdUfe: Move Ufe::Transform3d interface implementation [#3828](https://github.com/Autodesk/maya-usd/pull/3828)

## [v0.29.0] - 2024-07-31

**Build:**
* MayaUsd: Code cleanup - No pragma once [#3811](https://github.com/Autodesk/maya-usd/pull/3811)
* Fix tests that are modifying source folder [#3802](https://github.com/Autodesk/maya-usd/pull/3802)
* Update googletest & gulrak (filesystem) [#3800](https://github.com/Autodesk/maya-usd/pull/3800)
* Remove "using namespace UsdUfe" from api.h [#3770](https://github.com/Autodesk/maya-usd/pull/3770)
* Fix all test that relied on syncolor [#3763](https://github.com/Autodesk/maya-usd/pull/3763)
* UsdUfe: create standalone builds for MaxUsd [#3757](https://github.com/Autodesk/maya-usd/pull/3757)
* Fix Pixar plugin for USD v24.03 [#3689](https://github.com/Autodesk/maya-usd/pull/3689)

**Translation Framework:**
* Export Material:
  * Suport legacy metarial scope mode [#3810](https://github.com/Autodesk/maya-usd/pull/3810)
  * Material scope as default prim [#3801](https://github.com/Autodesk/maya-usd/pull/3801)
  * Export assigned materials [#3786](https://github.com/Autodesk/maya-usd/pull/3786)
  * Export materials without meshes [#3785](https://github.com/Autodesk/maya-usd/pull/3785)
  * Export material scope [#3774](https://github.com/Autodesk/maya-usd/pull/3774)
  * Export materials checkbox [#3768](https://github.com/Autodesk/maya-usd/pull/3768)
* Add remapUVSetsTo option to UsdImport [#3776](https://github.com/Autodesk/maya-usd/pull/3776)
* Add support to import Usd Blendshape as Maya blendshapes [#3775](https://github.com/Autodesk/maya-usd/pull/3775)
* Add support for env var expansions in file node writers [#3769](https://github.com/Autodesk/maya-usd/pull/3769)
* Only contain rootprim in defaultprim list if its available [#3759](https://github.com/Autodesk/maya-usd/pull/3759)
* Add flag to export empty transform [#3741](https://github.com/Autodesk/maya-usd/pull/3741)
* Fixed an issue exporting USD skel with new root option [#3729](https://github.com/Autodesk/maya-usd/pull/3729)
* Fix blendshape names by using parent node instead of mesh shape[#3703](https://github.com/Autodesk/maya-usd/pull/3703)

**Workflow:**
* Copy Paste: 
  * Cut a prim and not have it paste if the cut is restricted [#3793](https://github.com/Autodesk/maya-usd/pull/3793)
  * Support copy and then paste a prim as a sibling [#3791](https://github.com/Autodesk/maya-usd/pull/3791)
  * Pasted prims should be selected [#3777](https://github.com/Autodesk/maya-usd/pull/3777)
  * MayaUSD clipboard error messages when opening up preferences window [#3758](https://github.com/Autodesk/maya-usd/pull/3758)
  * Implement Copy\paste capabilities on USD Data [#3661](https://github.com/Autodesk/maya-usd/pull/3661)
* Fix rotation when prim already single-axis rotation [#3756](https://github.com/Autodesk/maya-usd/pull/3756)
* Fix center pivot command [#3755](https://github.com/Autodesk/maya-usd/pull/3755)
* Fast Python routing [#3753](https://github.com/Autodesk/maya-usd/pull/3753)
* Fix the position of the Maya manipulators when USD pivot are used instead of Maya pivots. [#3752](https://github.com/Autodesk/maya-usd/pull/3752)
* Extend the delete cmd edit routing [#3747](https://github.com/Autodesk/maya-usd/pull/3747)
* Edit routing for transform commands [#3746](https://github.com/Autodesk/maya-usd/pull/3746)
* Material binding strength in AE [#3736](https://github.com/Autodesk/maya-usd/pull/3736)
* Material custom control for AE [#3731](https://github.com/Autodesk/maya-usd/pull/3731)
* Repaint layer editor when layer muting changes [#3725](https://github.com/Autodesk/maya-usd/pull/3725)
* Fix layer editor refresh when reloading from AE [#3722](https://github.com/Autodesk/maya-usd/pull/3722)
* Add default value concept to USD datamodel for UFE [#3721](https://github.com/Autodesk/maya-usd/pull/3721)
* Filter items hidden in the outliner [#3719](https://github.com/Autodesk/maya-usd/pull/3719)
* Payload command undo to restore payload rules [#3714](https://github.com/Autodesk/maya-usd/pull/3714)
* Support EditRouter in delete command [#3695](https://github.com/Autodesk/maya-usd/pull/3695)

**Render:**
* Backport MaterialX 1.39 fix for source code node [#3795](https://github.com/Autodesk/maya-usd/pull/3795)
* Fix UDIMs for custom textures [#3787](https://github.com/Autodesk/maya-usd/pull/3787)
* Remove down cast of world space hit point in HdxPickHit [#3754](https://github.com/Autodesk/maya-usd/pull/3754)
* Instance selection highlight [#3744](https://github.com/Autodesk/maya-usd/pull/3744)

**Documentation:**
* Create SECURITY.md [#3806](https://github.com/Autodesk/maya-usd/pull/3806)
* Improve the undo/redo documentation [#3764](https://github.com/Autodesk/maya-usd/pull/3764)
* Minor fixes on AL migration guide, updated the Maya doc links to be 2025 [#3738](https://github.com/Autodesk/maya-usd/pull/3738)
* Migration guide from AL_USDMaya to Autodesk's MayaUSD [#3735](https://github.com/Autodesk/maya-usd/pull/3735)

**Miscellaneous:**
* Hides ramp attributes in AE Template [#3812](https://github.com/Autodesk/maya-usd/pull/3812)
* Disable material in sync with meshes when duplicating [#3805](https://github.com/Autodesk/maya-usd/pull/3805)
* Don't depend on MayaUSD if not using it [#3794](https://github.com/Autodesk/maya-usd/pull/3794)
* Add tooltip for lookdevx button in material section in AE [#3788](https://github.com/Autodesk/maya-usd/pull/3788)
* Update for a change in MaterialX 1.38.10 [#3771](https://github.com/Autodesk/maya-usd/pull/3771)
* Graph the material in LookdevX [#3740](https://github.com/Autodesk/maya-usd/pull/3740)
* UsdUfe: Move Ufe::Attributes interface implementation [#3737](https://github.com/Autodesk/maya-usd/pull/3737)
* Early out of fn if the required 'pxrUsd' plugin isn't loaded [#3724](https://github.com/Autodesk/maya-usd/pull/3724)
* Wrap the 'unilinearizeColors' parameter [#3723](https://github.com/Autodesk/maya-usd/pull/3723)
* Fix incorrect version check in test [#3720](https://github.com/Autodesk/maya-usd/pull/3720)

## [v0.28.0] - 2024-05-23

**Build:**
* Update for USD v24.03 [#3684](https://github.com/Autodesk/maya-usd/pull/3684)
* Fixup UFE_PREVIEW_NUM to use better Ufe cmake checks [#3673](https://github.com/Autodesk/maya-usd/pull/3673)
* Update testMayaUsdInfoCommand.py [#3670](https://github.com/Autodesk/maya-usd/pull/3670)
* Stage loading problems using MEL proc [#3667](https://github.com/Autodesk/maya-usd/pull/3667)
* Use the Using Directive macro [#3628](https://github.com/Autodesk/maya-usd/pull/3628)
* Minimal API needed by LookdevX to decouple from mayaUsd [#3608](https://github.com/Autodesk/maya-usd/pull/3608)
* Link against mayaUsdAPI to preload it for other plugins [#3606](https://github.com/Autodesk/maya-usd/pull/3606)
* Maya is no longer using a Preview Release number [#3602](https://github.com/Autodesk/maya-usd/pull/3602)
* Enable "Implicit Namespace Packages" feature [#3581](https://github.com/Autodesk/maya-usd/pull/3581)

**Translation Framework:**
* Root Prim and Root Prim Type:
    * Add the feature to set the root prim type on export [#3630](https://github.com/Autodesk/maya-usd/pull/3630)
    * Disable root prim type UI when no root prim entered [#3708](https://github.com/Autodesk/maya-usd/pull/3708)
    * Make rootPrimType persistent through exports [#3699](https://github.com/Autodesk/maya-usd/pull/3699)
    * Make root prim persistent and update default prim accordingly [#3641](https://github.com/Autodesk/maya-usd/pull/3641)
* Default Prim:
    * Persist the default prim selection if its available [#3705](https://github.com/Autodesk/maya-usd/pull/3705)
    * Expose all export options to chasers [#3698](https://github.com/Autodesk/maya-usd/pull/3698)
    * Keep rootprim in default prim list when export options change [#3692](https://github.com/Autodesk/maya-usd/pull/3692)
* Import/Export Plugin Configuration
    * Hide unwanted export plugin options [#3710](https://github.com/Autodesk/maya-usd/pull/3710)
    * Created import plugin config UI [#3676](https://github.com/Autodesk/maya-usd/pull/3676)
    * Updated the export plugin configuration UI [#3655](https://github.com/Autodesk/maya-usd/pull/3655)
* Support exporting a joint hierarchy inside a node tree to the root level [#3681](https://github.com/Autodesk/maya-usd/pull/3681)
* Fix the issue caused duplicate as USD failed [#3601](https://github.com/Autodesk/maya-usd/pull/3601)
* Call CanExport() per object instead of per export [#3592](https://github.com/Autodesk/maya-usd/pull/3592)

**Workflow:**
* Layer Locking:
    * Prevent unlocking system-locked layer when unlocking parents [#3706](https://github.com/Autodesk/maya-usd/pull/3706)
    * Update layer editor lock status after adding a layer [#3704](https://github.com/Autodesk/maya-usd/pull/3704)
    * Adds a callback for RefreshSystemLock when a layer's lock status is changed [#3671](https://github.com/Autodesk/maya-usd/pull/3671)
    * Fixed saving vs locked layers [#3669](https://github.com/Autodesk/maya-usd/pull/3669)
    * Un-shared stage and its sublayers will appear as system lock [#3654](https://github.com/Autodesk/maya-usd/pull/3654)
    * Fixes the issue where the lock state isn't applied from Maya file when the layer is both locked and muted [#3650](https://github.com/Autodesk/maya-usd/pull/3650)
    * Adds the ability to lock layers and sublayers at once [#3646](https://github.com/Autodesk/maya-usd/pull/3646)
    * Locked layer status persists between Maya sessions [#3638](https://github.com/Autodesk/maya-usd/pull/3638)
    * Adds write permission checks to layer system-locking [#3626](https://github.com/Autodesk/maya-usd/pull/3626)
    * Adds the ability to System-lock a layer [#3619](https://github.com/Autodesk/maya-usd/pull/3619)
    * Added Locking mechanism to layers [#3595](https://github.com/Autodesk/maya-usd/pull/3595)
* Allow deleting prims carrying a loaded payload [#3712](https://github.com/Autodesk/maya-usd/pull/3712)
* When opening a file dependency in the AE, the dialog doesn't open to that path [#3709](https://github.com/Autodesk/maya-usd/pull/3709)
* Layer Editor UI:
    * Updates Layer Editor color visuals [#3685](https://github.com/Autodesk/maya-usd/pull/3685)
    * Improve Layer Eidtor UI [#3649](https://github.com/Autodesk/maya-usd/pull/3649)
    * Update the look of the Layer Editor [#3643](https://github.com/Autodesk/maya-usd/pull/3643)
* Implement Ufe::Camera::renderable() for Maya Master (2026) [#3683](https://github.com/Autodesk/maya-usd/pull/3683)
* Clear the selected stage layer database attribute when no stage selected [#3680](https://github.com/Autodesk/maya-usd/pull/3680)
* Remove lingering invalid prims after adding prim under prim that is deactivated [#3679](https://github.com/Autodesk/maya-usd/pull/3679)
* Targets the session layer when no other layers are modifiable [#3665](https://github.com/Autodesk/maya-usd/pull/3665)
* Add support for getting native type via metadata [#3648](https://github.com/Autodesk/maya-usd/pull/3648)
* Restrict UFE nodes according to proxyNode's primPath [#3640](https://github.com/Autodesk/maya-usd/pull/3640)
* Better universal manipulator undo redo [#3615](https://github.com/Autodesk/maya-usd/pull/3615)
* Keep Maya Ref valid after saving [#3609](https://github.com/Autodesk/maya-usd/pull/3609)
* Fix the crash when editing as Maya an instanced object and then undoing the edit-as-Maya [#3593](https://github.com/Autodesk/maya-usd/pull/3593)

**Render:**
* Fix crash due to invalid shader [#3702](https://github.com/Autodesk/maya-usd/pull/3702)
* Fix reading past the end of the buffer causing incorrect rendering and crash later on [#3663](https://github.com/Autodesk/maya-usd/pull/3663)
* Add support for doubleSided attribute on USD prims in VP2 delegate [#3656](https://github.com/Autodesk/maya-usd/pull/3656)
* Remember unknown color spaces [#3652](https://github.com/Autodesk/maya-usd/pull/3652)
* Fix performance of instanceable prims [#3607](https://github.com/Autodesk/maya-usd/pull/3607)
* Allowing meshes with insufficient primvars data size [#3586](https://github.com/Autodesk/maya-usd/pull/3586)

**Documentation:**
* Adding a C++ export plugin example [#3688](https://github.com/Autodesk/maya-usd/pull/3688)
* Publish USD Branches [#3645](https://github.com/Autodesk/maya-usd/pull/3645)
* Improve the undo/redo documentation [#3644](https://github.com/Autodesk/maya-usd/pull/3644)
* Update codingGuidelines.md [#3633](https://github.com/Autodesk/maya-usd/pull/3633)

**Miscellaneous:**
* Layer editor icon scaling [#3715](https://github.com/Autodesk/maya-usd/pull/3715)
* Fix the crash when attempting to save a USD file from USD Layer Editor [#3697](https://github.com/Autodesk/maya-usd/pull/3697)
* Author references but do not have the UsdStage compose [#3696](https://github.com/Autodesk/maya-usd/pull/3696)
* Fix image file attribute detection [#3693](https://github.com/Autodesk/maya-usd/pull/3693)
* Dope Sheet : Error when right-click on channel set if USD plugin installed [#3691](https://github.com/Autodesk/maya-usd/pull/3691)
* Fix conflict in shaders with MaterialX 1.38.9 [#3678](https://github.com/Autodesk/maya-usd/pull/3678)
* Fix mistake in command name [#3668](https://github.com/Autodesk/maya-usd/pull/3668)
* Allow Raw colorspace aliases in file texture validation [#3653](https://github.com/Autodesk/maya-usd/pull/3653)

## [v0.27.0] - 2024-02-06

**Build:**
* Refactor duplication code [#3573](https://github.com/Autodesk/maya-usd/pull/3573)
* Fix broken CXX ABI detection [#3562](https://github.com/Autodesk/maya-usd/pull/3562)
* Add Unit test for parent to selection [#3558](https://github.com/Autodesk/maya-usd/pull/3558)
* Updated workflow for running preflight [#3545](https://github.com/Autodesk/maya-usd/pull/3545)
* Add explicit boost/optional include [#3540](https://github.com/Autodesk/maya-usd/pull/3540)
* Restore OpenEXR test [#3532](https://github.com/Autodesk/maya-usd/pull/3532)
* Deprecate boost optional for sdf copy spec [#3522](https://github.com/Autodesk/maya-usd/pull/3522)
* Update usage of the Hydra API [#3521](https://github.com/Autodesk/maya-usd/pull/3521)
* Add versioning comments [#3520](https://github.com/Autodesk/maya-usd/pull/3520)
* MayaUSD : Bump UFE version to v5.0 [#3514](https://github.com/Autodesk/maya-usd/pull/3514)
* Allow specializing the topo neutral graph generator [#3503](https://github.com/Autodesk/maya-usd/pull/3503)
* Run more of the Pixar plugin tests [#3479](https://github.com/Autodesk/maya-usd/pull/3479)
* Build USD v23.11 with Python 3.11/OSD 3.6 and update ecg-maya-usd [#3459](https://github.com/Autodesk/maya-usd/pull/3459)
* Add missing include [#3455](https://github.com/Autodesk/maya-usd/pull/3455)
* Support for USD v23.11 [#3447](https://github.com/Autodesk/maya-usd/pull/3447)

**Translation Framework:**
* Set the default prim on export [#3572](https://github.com/Autodesk/maya-usd/pull/3572)
* Export MaterialX path for extra nodes [#3516](https://github.com/Autodesk/maya-usd/pull/3516)
* Fix copying the proxy shape node [#3489](https://github.com/Autodesk/maya-usd/pull/3489)
* Provide access to the Export Selected options [#3488](https://github.com/Autodesk/maya-usd/pull/3488)
* Register USD data in the Maya File Path Editor [#3482](https://github.com/Autodesk/maya-usd/pull/3482)
* Fix errors when trying to cache a rig with merge transforms ON and namespaces OFF [#3474](https://github.com/Autodesk/maya-usd/pull/3474)
* Determine how to read UsdLux prims with an envvar [#3385](https://github.com/Autodesk/maya-usd/pull/3385)

**Workflow:**
* Attribute Editor:
    * Fix AE flashing when adding many attributes [#3579](https://github.com/Autodesk/maya-usd/pull/3579)
    * Fix AE tooltip and status bar message [#3571](https://github.com/Autodesk/maya-usd/pull/3571)
    * Tooltips are not formatted correctly [#3536](https://github.com/Autodesk/maya-usd/pull/3536)
* Add unit test for circular relationships [#3574](https://github.com/Autodesk/maya-usd/pull/3574)
* Recognize NodeGraph EnumString attributes [#3568](https://github.com/Autodesk/maya-usd/pull/3568)
* Fix copying node with Arnold material [#3566](https://github.com/Autodesk/maya-usd/pull/3566)
* Layer Editor's "Revert to File" menu item is renamed to "Reload" [#3565](https://github.com/Autodesk/maya-usd/pull/3565)
* Allow creating custom types at NodeGraph boundaries [#3564](https://github.com/Autodesk/maya-usd/pull/3564)
* Fix prettify name routine when all caps name has a number at the end [#3557](https://github.com/Autodesk/maya-usd/pull/3557)
* Fixes a bug with UsdAttributeEnumString that have token values [#3550](https://github.com/Autodesk/maya-usd/pull/3550)
* Duplicate-to-USD support relationship targets [#3537](https://github.com/Autodesk/maya-usd/pull/3537)
* Don't show set-as-default prim when already default [#3533](https://github.com/Autodesk/maya-usd/pull/3533)
* Use uimin if uisoftmin is missing [#3531](https://github.com/Autodesk/maya-usd/pull/3531)
* Bulk Editing:
    * Make the bulk menu look modern and up to date with VXD [#3528](https://github.com/Autodesk/maya-usd/pull/3528)
    * Add bulk editing support for unload and load with descendants [#3496](https://github.com/Autodesk/maya-usd/pull/3496)
* Allow parenting under a stronger layer [#3526](https://github.com/Autodesk/maya-usd/pull/3526)
* Fixes an out of range crash that only occurs in Debug mode [#3519](https://github.com/Autodesk/maya-usd/pull/3519)
* Use proper API to set color space in USD [#3515](https://github.com/Autodesk/maya-usd/pull/3515)
* Fix edit target after layer clear [#3508](https://github.com/Autodesk/maya-usd/pull/3508)
* From LookdevX bring over UINodeGraphNode new virtual functions [#3506](https://github.com/Autodesk/maya-usd/pull/3506)
* Allow saving locked layers [#3504](https://github.com/Autodesk/maya-usd/pull/3504)
* Fix crash when duplicating a proxy shape [#3449](https://github.com/Autodesk/maya-usd/pull/3449)
* Fix finding strongest layer [#3498](https://github.com/Autodesk/maya-usd/pull/3498)
* Support relative cache to USD in anon layers [#3495](https://github.com/Autodesk/maya-usd/pull/3495)
* Add a third party naming convention for shader outliner icons [#3494](https://github.com/Autodesk/maya-usd/pull/3494)
* Send subtree invalidate on undo mark instanceable [#3484](https://github.com/Autodesk/maya-usd/pull/3484)
* Apply restrictions for activation and instanceable [#3463](https://github.com/Autodesk/maya-usd/pull/3463)
* Fixes Edit As Maya Options Convert Instances Not Retaining From Session to Session [#3443](https://github.com/Autodesk/maya-usd/pull/3443)

**Render:**
* Fix crash in topo handler [#3570](https://github.com/Autodesk/maya-usd/pull/3570)
* Wait cursor during long VP2 updates [#3563](https://github.com/Autodesk/maya-usd/pull/3563)
* Fix incorrect tangent fixup [#3559](https://github.com/Autodesk/maya-usd/pull/3559)
* Fix the transform nodes crash [#3552](https://github.com/Autodesk/maya-usd/pull/3552)
* Do not bail out after a failed validate call [#3546](https://github.com/Autodesk/maya-usd/pull/3546)
* Fix instanceable prims not rendered in Hydra [#3534](https://github.com/Autodesk/maya-usd/pull/3534)
* Fix multiple NodeGraph connections [#3509](https://github.com/Autodesk/maya-usd/pull/3509)
* Update OCIO code to handle new Hydra colorspace info [#3507](https://github.com/Autodesk/maya-usd/pull/3507)
* Use new Hd MtlxStdLibraries API [#3493](https://github.com/Autodesk/maya-usd/pull/3493)
* Fix legacy CM nodes affected by MaterialX 1.38.8 [#3481](https://github.com/Autodesk/maya-usd/pull/3481)
* Get watch list of traversed nodes [#3462](https://github.com/Autodesk/maya-usd/pull/3462)
* Implemented MaterialX Topo Handler [#3445](https://github.com/Autodesk/maya-usd/pull/3445)

**Documentation:**
* Update CONTRIBUTING.md to point to coding guidelines [#3556](https://github.com/Autodesk/maya-usd/pull/3556)
* Update README.md with a link to the release page [#3524](https://github.com/Autodesk/maya-usd/pull/3524)
* Update helpTableMayaUSD [#3453](https://github.com/Autodesk/maya-usd/pull/3453)
* Update USD links [#3452](https://github.com/Autodesk/maya-usd/pull/3452)

**Miscellaneous:**
* Fix slow scene load performance [#3576](https://github.com/Autodesk/maya-usd/pull/3576)
* Resource Identifier warning on first launch of MayaUSD [#3561](https://github.com/Autodesk/maya-usd/pull/3561)
* USD Prefs: "Use Display Color" preferences are maintained when user cancels change [#3553](https://github.com/Autodesk/maya-usd/pull/3553)
* Make New layers collapsed [#3549](https://github.com/Autodesk/maya-usd/pull/3549)
* Expand and collapse all layer items [#3544](https://github.com/Autodesk/maya-usd/pull/3544)
* Fixes USDZ extension not being accepted in prim hierarchy view [#3530](https://github.com/Autodesk/maya-usd/pull/3530)
* Remove the load payloads proxy shape attribute [#3523](https://github.com/Autodesk/maya-usd/pull/3523)
* Fix reloading scene with layers saved in the Maya scene [#3518](https://github.com/Autodesk/maya-usd/pull/3518)
* Fix Linux icon paths [#3517](https://github.com/Autodesk/maya-usd/pull/3517)
* Use wait cursor for payload commands [#3513](https://github.com/Autodesk/maya-usd/pull/3513)
* Integrated UsdSceneItemMetaData Into UsdSceneItem [#3505](https://github.com/Autodesk/maya-usd/pull/3505)
* Support custom display name for USD attributes [#3499](https://github.com/Autodesk/maya-usd/pull/3499)
* Pipe UsdStage data and Complexity to proxy nodes [#3492](https://github.com/Autodesk/maya-usd/pull/3492)

## [v0.26.0] - 2023-11-21

**Build:**
* Cleanup old unsupported versions from unit tests [#3411](https://github.com/Autodesk/maya-usd/pull/3411)
* Explicitly specify all required export options [#3409](https://github.com/Autodesk/maya-usd/pull/3409)
* Fixing import path for AETemplate [#3408](https://github.com/Autodesk/maya-usd/pull/3408)
* Add test cases for material fanned out file nodes [#3403](https://github.com/Autodesk/maya-usd/pull/3403)
* Explicitly set UpAxis in tests [#3386](https://github.com/Autodesk/maya-usd/pull/3386)
* Remove a print 999 from one unit test [#3381](https://github.com/Autodesk/maya-usd/pull/3381)
* Qt6 archive in Maya devkit was renamed [#3359](https://github.com/Autodesk/maya-usd/pull/3359)
* Add versioning to tests that use UsdSkelPackedJointAnimation [#3318](https://github.com/Autodesk/maya-usd/pull/3318)
* Set purpose on test meshes [#3317](https://github.com/Autodesk/maya-usd/pull/3317)
* MayaUsd: build with Qt directly from Maya devkit/runtime [#3298](https://github.com/Autodesk/maya-usd/pull/3298)
* Adding unit test for AETemplate [#3297](https://github.com/Autodesk/maya-usd/pull/3297)
* Update Scene Indices chain code in maya-usd [#3293](https://github.com/Autodesk/maya-usd/pull/3293)
* Enable MaterialX support in CMake [#3289](https://github.com/Autodesk/maya-usd/pull/3289)

**Translation Framework:**
* Properly reset the select prim dialog [#3441](https://github.com/Autodesk/maya-usd/pull/3441)
* Make USD Export Options consistent with previous export [#3424](https://github.com/Autodesk/maya-usd/pull/3424)
* Disable Material section when Mesh type is excluded from export [#3421](https://github.com/Autodesk/maya-usd/pull/3421)
* Edit as Maya:
    * Fixes Edit As Maya Options Menu Not Showing [#3444](https://github.com/Autodesk/maya-usd/pull/3444)
    * Fixes Edit-As-Maya Option Dialog Not Executing Action [#3422](https://github.com/Autodesk/maya-usd/pull/3422)
    * Hide frame range in edit-as-Maya options [#3390](https://github.com/Autodesk/maya-usd/pull/3390)
    * Fixes Edit As Maya Data Apply Closes OptionDialog [#3382](https://github.com/Autodesk/maya-usd/pull/3382)
    * Edit as Maya multiple variants [#3350](https://github.com/Autodesk/maya-usd/pull/3350)
    * Fix the issue that "Edit as Maya Data" options dialog overwrites import UI [#3276](https://github.com/Autodesk/maya-usd/pull/3276)
    * Added an option box for outliner on edit as Maya Data [#3269](https://github.com/Autodesk/maya-usd/pull/3269)
* Better UI when selecting the proxy shape path [#3383](https://github.com/Autodesk/maya-usd/pull/3383)
* Always use the same folder to load layers [#3380](https://github.com/Autodesk/maya-usd/pull/3380)
* Allow selecting the referenced prim [#3372](https://github.com/Autodesk/maya-usd/pull/3372)
* Fixes unneeded nodes getting exported on materials [#3366](https://github.com/Autodesk/maya-usd/pull/3366)
* Write non-connected uvSet names [#3339](https://github.com/Autodesk/maya-usd/pull/3339)
* Control the Types of Objects to Export [#3338](https://github.com/Autodesk/maya-usd/pull/3338)
* Avoid creating an extra transform when in a prototype [#3315](https://github.com/Autodesk/maya-usd/pull/3315)
* Fix load sub-layer file preview [#3305](https://github.com/Autodesk/maya-usd/pull/3305)
* Fix default prepend/append when no prefs are saved [#3302](https://github.com/Autodesk/maya-usd/pull/3302)

**Workflow:**
* Fix how stages are updated when saved [#3432](https://github.com/Autodesk/maya-usd/pull/3432)
* Fixes Redo Visibility Visual Update in Outliner [#3428](https://github.com/Autodesk/maya-usd/pull/3428)
* Default Prim:
    * Disable the default prim auto-filling [#3427](https://github.com/Autodesk/maya-usd/pull/3427)
    * Set a prim as defaultPrim in the stage [#3394](https://github.com/Autodesk/maya-usd/pull/3394)
* Dirty layers don't get saved in Maya files [#3418](https://github.com/Autodesk/maya-usd/pull/3418) [#3402](https://github.com/Autodesk/maya-usd/pull/3402)
* Add support to toggle visibility of multiple prims [#3417](https://github.com/Autodesk/maya-usd/pull/3417)
* Use the same saved folder location as when loading a stage [#3401](https://github.com/Autodesk/maya-usd/pull/3401)
* Prevent root metadata changes when not targeting root [#3398](https://github.com/Autodesk/maya-usd/pull/3398)
* Labeling of Relative settings may need reworded [#3397](https://github.com/Autodesk/maya-usd/pull/3397)
* Fix errors when a USD object has an !invert! xform attribute [#3393](https://github.com/Autodesk/maya-usd/pull/3393)
* Add support to add a relative file dependency on a anonymous layer [#3392](https://github.com/Autodesk/maya-usd/pull/3392)
* Use the outliner menu in the viewport [#3375](https://github.com/Autodesk/maya-usd/pull/3375)
* Enabled Load Payload By Default On Add Payload UI [#3371](https://github.com/Autodesk/maya-usd/pull/3371)
* Add support to add a relative payload/ref on a anonymous layer [#3370](https://github.com/Autodesk/maya-usd/pull/3370)
* Fix reparenting crash [#3364](https://github.com/Autodesk/maya-usd/pull/3364)
* Add Untextured mode selections to user pref [#3361](https://github.com/Autodesk/maya-usd/pull/3361)
* Add support to add a relative sublayer on a anonymous layer [#3353](https://github.com/Autodesk/maya-usd/pull/3353)
* Fixes outliner not refreshing when performing a UsdUndoVisibleCommand [#3345](https://github.com/Autodesk/maya-usd/pull/3345)
* Setup preference section [#3343](https://github.com/Autodesk/maya-usd/pull/3343)
* Properly read the ignore variants flag [#3334](https://github.com/Autodesk/maya-usd/pull/3334)
* Append 1 to new prim name only if not ending with digit [#3331](https://github.com/Autodesk/maya-usd/pull/3331)
* Fix the issue that material assignment disconnected when duplicate count reaches 10 [#3329](https://github.com/Autodesk/maya-usd/pull/3329)
* Support ignoring variants when merging to USD [#3324](https://github.com/Autodesk/maya-usd/pull/3324)
* Fix lost muted anonymous layers [#3320](https://github.com/Autodesk/maya-usd/pull/3320)
* Incremental naming doesn't take zero into consideration [#3309](https://github.com/Autodesk/maya-usd/pull/3309)
* Report correct default soft min/max per type [#3301](https://github.com/Autodesk/maya-usd/pull/3301)
* Make Reference the default for Composition Arc instead of Payload [#3280](https://github.com/Autodesk/maya-usd/pull/3280) [#3327](https://github.com/Autodesk/maya-usd/pull/3327)
* Fixed bad USD/UFE notifications [#3278](https://github.com/Autodesk/maya-usd/pull/3278)

**Render:**
* Fix GLSL error with unlit surfaces [#3434](https://github.com/Autodesk/maya-usd/pull/3434)
* Renaming internal instancer primvars [#3347](https://github.com/Autodesk/maya-usd/pull/3347)
* Fix for crash on new scene with a USD Stage inside the Maya scene [#3325](https://github.com/Autodesk/maya-usd/pull/3325)
* Add support for MaterialX 1.38.8 [#3323](https://github.com/Autodesk/maya-usd/pull/3323)
* Update topo node list [#3322](https://github.com/Autodesk/maya-usd/pull/3322)
* Fix broken normal maps [#3307](https://github.com/Autodesk/maya-usd/pull/3307)
* Connect Maya Time to USD Imaging SceneIndex [#3296](https://github.com/Autodesk/maya-usd/pull/3296)
* Fix rendering of gltf texture nodes [#3295](https://github.com/Autodesk/maya-usd/pull/3295)
* Use display color when texture mode is disabled [#3272](https://github.com/Autodesk/maya-usd/pull/3272)

**Documentation:**
* Add documentation for excludeExportTypes [#3420](https://github.com/Autodesk/maya-usd/pull/3420)
* Build.md update for VisualStudio and Ufe versions [#3290](https://github.com/Autodesk/maya-usd/pull/3290)

**Miscellaneous:**
* Use project folder by default [#3333](https://github.com/Autodesk/maya-usd/pull/3333)
* Make error messages more clear [#3310](https://github.com/Autodesk/maya-usd/pull/3310)
* AE: broken templates on older maya versions [#3306](https://github.com/Autodesk/maya-usd/pull/3306)

## [v0.25.0] - 2023-10-04

**Build:**
* Use updated name for Hd task param [#3242](https://github.com/Autodesk/maya-usd/pull/3242)
* Include pxr header [#3230](https://github.com/Autodesk/maya-usd/pull/3230)
* Use Pixar's directive for namespacing [#3220](https://github.com/Autodesk/maya-usd/pull/3220)
* Make building with Ufe mandatory and remove v1 support [#3217](https://github.com/Autodesk/maya-usd/pull/3217)
* Use the updated HW Token in MaterialX 1.38.7 [#3211](https://github.com/Autodesk/maya-usd/pull/3211)
* MayaUsd: drop support for Maya 2018/2019/2020 and Qt 5.6.1 & 5.12.5 [#3165](https://github.com/Autodesk/maya-usd/pull/3165)
* Remove usage of macros leading to fatal exits [#3081](https://github.com/Autodesk/maya-usd/pull/3081)

**Translation Framework:**
* Bug fix for export relative texture [#3294](https://github.com/Autodesk/maya-usd/pull/3294)
* Fix texture export creating wrong type for uvCoord attribute [#3273](https://github.com/Autodesk/maya-usd/pull/3273)
* Import relative textures [#3259](https://github.com/Autodesk/maya-usd/pull/3259) [#3260](https://github.com/Autodesk/maya-usd/pull/3260)
* Export relative USD textures [#3250](https://github.com/Autodesk/maya-usd/pull/3250) [#3256](https://github.com/Autodesk/maya-usd/pull/3256)
* Ability to exclude primvars with specific Namespace [#3223](https://github.com/Autodesk/maya-usd/pull/3223)
* Referenced shader attributes issue [#3214](https://github.com/Autodesk/maya-usd/pull/3214)
* Refactors relative path dialog options to be more clear [#3195](https://github.com/Autodesk/maya-usd/pull/3195)
* Override primWriter [#3189](https://github.com/Autodesk/maya-usd/pull/3189) and PrimReader [#3164](https://github.com/Autodesk/maya-usd/pull/3164)
* Allow textures to be relative [#3183](https://github.com/Autodesk/maya-usd/pull/3183)
* Updating AL USDMaya's NurbsCurve static import export to use USD's curve spec [#2957](https://github.com/Autodesk/maya-usd/pull/2957)

**Workflow:**
* AE: broken templates on older maya versions [#3306](https://github.com/Autodesk/maya-usd/pull/3306)
* Fixed a bug with redoing a UsdUndoCreateFromNodeDefCommand [#3279](https://github.com/Autodesk/maya-usd/pull/3279)
* Changed toggle visibility to used computed visibile [#3266](https://github.com/Autodesk/maya-usd/pull/3266)
* USD reference/Payload: "Load Payload" on the browser dialog should have a tooltip [#3265](https://github.com/Autodesk/maya-usd/pull/3265)
* Don't use a random default prim [#3253](https://github.com/Autodesk/maya-usd/pull/3253)
* UsdUfe - Move UFE to its own Project [#3252](https://github.com/Autodesk/maya-usd/pull/3252) [#3248](https://github.com/Autodesk/maya-usd/pull/3248)
* Change the export options to be consistent with import options [#3251](https://github.com/Autodesk/maya-usd/pull/3251)
* Embed import options in subheadings [#3249](https://github.com/Autodesk/maya-usd/pull/3249)
* Load relative sub-layer [#3245](https://github.com/Autodesk/maya-usd/pull/3245)
* Relative Maya reference cache [#3239](https://github.com/Autodesk/maya-usd/pull/3239)
* Allow Maya reference to be relative to the layer [#3236](https://github.com/Autodesk/maya-usd/pull/3236)
* Return prim readers that are loaded in find [#3235](https://github.com/Autodesk/maya-usd/pull/3235)
* Newly created USD Lights can't cast shadows [#3233](https://github.com/Autodesk/maya-usd/pull/3233)
* Allow introspecting UsdAttribute UFE class [#3232](https://github.com/Autodesk/maya-usd/pull/3232)
* Fix root layer attribute in AE [#3231](https://github.com/Autodesk/maya-usd/pull/3231)
* Relative payload preview [#3229](https://github.com/Autodesk/maya-usd/pull/3229)
* Detect invalid use of undo item [#3227](https://github.com/Autodesk/maya-usd/pull/3227)
* Fixes an issue in Layer Editor where Save All icon was not functioning properly [#3222](https://github.com/Autodesk/maya-usd/pull/3222)
* Fix undo of the stage creation command [#3215](https://github.com/Autodesk/maya-usd/pull/3215)
* Build the layer manager model contents when created [#3213](https://github.com/Autodesk/maya-usd/pull/3213)
* Reduce the reference warning [#3210](https://github.com/Autodesk/maya-usd/pull/3210)
* Proxy shape listener nodes should not be persisted [#3207](https://github.com/Autodesk/maya-usd/pull/3207)
* Fix lost notification listeners [#3202](https://github.com/Autodesk/maya-usd/pull/3202)
* Fix for lights after "Edit as Maya Data" [#3198](https://github.com/Autodesk/maya-usd/pull/3198)
* Release all refs held by session state on reset [#3196](https://github.com/Autodesk/maya-usd/pull/3196)
* Clean up references, targets, and connections on delete [#3194](https://github.com/Autodesk/maya-usd/pull/3194)
* Load reference and payload without default prim [#3190](https://github.com/Autodesk/maya-usd/pull/3190)
* Options for "Edit as Maya Data" [#3188](https://github.com/Autodesk/maya-usd/pull/3188)
* Prevent editing instance proxy [#3187](https://github.com/Autodesk/maya-usd/pull/3187)
* UI cleanup for bulk save [#3185](https://github.com/Autodesk/maya-usd/pull/3185)
* Move commands out of context op implementation [#3179](https://github.com/Autodesk/maya-usd/pull/3179)
* Clear payloads [#3170](https://github.com/Autodesk/maya-usd/pull/3170)

**Render:**
* Editing purpose on a prim doesn't update in the viewport until prim is deselected [#3246](https://github.com/Autodesk/maya-usd/pull/3246)
* Fix extra Arnold refresh when moving nodes [#3241](https://github.com/Autodesk/maya-usd/pull/3241)
* Use new HdFlattenedDataSourceProvider [#3212](https://github.com/Autodesk/maya-usd/pull/3212)
* Fix inactive sourceColorSpace in UsdUVTexture [#3192](https://github.com/Autodesk/maya-usd/pull/3192)
* Fix refresh issues with textured shading [#3181](https://github.com/Autodesk/maya-usd/pull/3181)
* Add new camera parameters to the pxr gl renderer [#3162](https://github.com/Autodesk/maya-usd/pull/3162)

**Documentation:**
* Update build.md to include USD v23.08 [#3282](https://github.com/Autodesk/maya-usd/pull/3282)
* Update docs with `-primVariant` flag info and example [#3268](https://github.com/Autodesk/maya-usd/pull/3268)
* Refactored MayaUSD help content ids [#3255](https://github.com/Autodesk/maya-usd/pull/3255)
* Add `excludePrimvarNamespace` flag to readme [#3247](https://github.com/Autodesk/maya-usd/pull/3247)
* Fix link betwwen documents [#3221](https://github.com/Autodesk/maya-usd/pull/3221)

## [v0.24.0] - 2023-07-07

**Build:**
* Add options to UsdExport in new test [#3158](https://github.com/Autodesk/maya-usd/pull/3158)
* Use new HdMaterialBindings Schema and API [#3152](https://github.com/Autodesk/maya-usd/pull/3152)
* Qt 6:
  * Update Maya USD to use MacOS Frameworks for Qt 6 instead of using loose libraries [#3150](https://github.com/Autodesk/maya-usd/pull/3150)
  * Adds support to compile against Qt 6.5 [#3116](https://github.com/Autodesk/maya-usd/pull/3116)
* Add test for animated cameras and lights [#3149](https://github.com/Autodesk/maya-usd/pull/3149)
* Eliminate spaces from doc filenames [#3139](https://github.com/Autodesk/maya-usd/pull/3139)
* Add UFE selection notification tests [#3132](https://github.com/Autodesk/maya-usd/pull/3132)
* Usd Versioning:
  * Remove support for USD < v21.11 [#3128](https://github.com/Autodesk/maya-usd/pull/3128)
  * Use new UsdPrimDefintionAPI with latest USD [#3098](https://github.com/Autodesk/maya-usd/pull/3098)
  * Update FindUSD.cmake for USD version patch [#3064](https://github.com/Autodesk/maya-usd/pull/3064)
  * USD 23.5 compatibility: Fix USD version check [#3055](https://github.com/Autodesk/maya-usd/pull/3055)
  * testVP2RenderDelegateDrawModes fails with USD v23.02 [#3043](https://github.com/Autodesk/maya-usd/pull/3043)
  * Update for USD v23.02 [#3025](https://github.com/Autodesk/maya-usd/pull/3025)
* Fix tests to write output in testing temp dir (not source tree) [#3127](https://github.com/Autodesk/maya-usd/pull/3127)
* Adhere to coding standard and use the fully qualified namespace name [#3126](https://github.com/Autodesk/maya-usd/pull/3126) [#2976](https://github.com/Autodesk/maya-usd/pull/2976)
* Add rotation unit test [#3112](https://github.com/Autodesk/maya-usd/pull/3112)
* Add missing ARCHIVE DESTINATION [#3100](https://github.com/Autodesk/maya-usd/pull/3100)
* Fix out-of-date UsdSkel assets [#3090](https://github.com/Autodesk/maya-usd/pull/3090)
* Update include paths for Hd files that have moved [#3089](https://github.com/Autodesk/maya-usd/pull/3089)
* Material api: hasMaterial UFE version check [#3066](https://github.com/Autodesk/maya-usd/pull/3066)
* UsdLight*.cpp: fix building with non-pxr namespace [#3032](https://github.com/Autodesk/maya-usd/pull/3032)
* Ufe 4.x:
  * Update FindUFE.cmake for Ufe v4.2 [#3134](https://github.com/Autodesk/maya-usd/pull/3134)
  * Remove/replace all UFE_PREVIEW_VERSION_NUM [#3020](https://github.com/Autodesk/maya-usd/pull/3020)
  * Update UFE_PREVIEW_VERSION_NUM for Ufe 4.1 [#3019](https://github.com/Autodesk/maya-usd/pull/3019)
  * Preparation for Ufe v4.1 [#2960](https://github.com/Autodesk/maya-usd/pull/2960)
* Fix UINodeGraphNode test [#3009](https://github.com/Autodesk/maya-usd/pull/3009)
* Update imaging baselines [#2982](https://github.com/Autodesk/maya-usd/pull/2982)
* Expose MaterialX feature in build.py [#2979](https://github.com/Autodesk/maya-usd/pull/2979)

**Translation Framework:**
* Improve roundtripping of GeomSubsets [#3135](https://github.com/Autodesk/maya-usd/pull/3135)
* Fix export roots behaviour [#3131](https://github.com/Autodesk/maya-usd/pull/3131)
* Imported displayOpacity are not interpreted correctly [#3071](https://github.com/Autodesk/maya-usd/pull/3071)
* Add metersPerUnit scaling [#2954](https://github.com/Autodesk/maya-usd/pull/2954)
* Added applyEulerFilter to import [#2942](https://github.com/Autodesk/maya-usd/pull/2942)

**Workflow:**
* Smart Signaling:
  * Reintroduce legacy smart signaling in 2024 [#3177](https://github.com/Autodesk/maya-usd/pull/3177)
  * Smart signaling must emit MNodeMessages [#3015](https://github.com/Autodesk/maya-usd/pull/3015)
  * Add smart signaling for renderers [#2972](https://github.com/Autodesk/maya-usd/pull/2972)
* Merge as USD edits to be specified as overs when authoring to a stronger layer [#3173](https://github.com/Autodesk/maya-usd/pull/3173)
* UsdUfe - Move UFE to its own Project [#3161](https://github.com/Autodesk/maya-usd/pull/3161) [#3145](https://github.com/Autodesk/maya-usd/pull/3145) [#3109](https://github.com/Autodesk/maya-usd/pull/3109)
* Merge default values [#3159](https://github.com/Autodesk/maya-usd/pull/3159)
* Fix context menu when in a set [#3157](https://github.com/Autodesk/maya-usd/pull/3157)
* Missing discard edits [#3154](https://github.com/Autodesk/maya-usd/pull/3154)
* New feature: "Add payload" [#3151](https://github.com/Autodesk/maya-usd/pull/3151)
* Fix for prims not being added to empty USD Stage [#3144](https://github.com/Autodesk/maya-usd/pull/3144)
* Fix layer parenting of relative layers [#3141](https://github.com/Autodesk/maya-usd/pull/3141)
* Relative path error fix [#3137](https://github.com/Autodesk/maya-usd/pull/3137)
* Quick fix for conversion of rotations [#3124](https://github.com/Autodesk/maya-usd/pull/3124)
* Frame both USD and Maya data when edited as Maya [#3123](https://github.com/Autodesk/maya-usd/pull/3123)
* Fix geom instances being at the wrong place when using mayaHydra in the viewport [#3120](https://github.com/Autodesk/maya-usd/pull/3120)
* Save all my USD files as relative on the Bulk Save dialog [#3117](https://github.com/Autodesk/maya-usd/pull/3117)
* Import Chaser doesn't receive information on how to match USD prim to DAG paths [#3114](https://github.com/Autodesk/maya-usd/pull/3114)
* Fixing pivot command [#3110](https://github.com/Autodesk/maya-usd/pull/3110)
* Set my anonymous layers as relative in Bulk Save's 'Save As' dialog [#3104](https://github.com/Autodesk/maya-usd/pull/3104)
* Implement code wrapper handler [#3102](https://github.com/Autodesk/maya-usd/pull/3102)
* Fix a bug where `UsdUINodeGraphNode::hasPosOrSize()` returned the wrong value [#3099](https://github.com/Autodesk/maya-usd/pull/3099)
* Remove duplicate for Maya Ref [#3095](https://github.com/Autodesk/maya-usd/pull/3095)
* Remove error message on deleting a stage [#3093](https://github.com/Autodesk/maya-usd/pull/3093)
* Save and restore selected stage [#3092](https://github.com/Autodesk/maya-usd/pull/3092)
* Fixes broken prim creation caused by enum change [#3087](https://github.com/Autodesk/maya-usd/pull/3087)
* Relative USD root file to appear as relative before my scene file is saved [#3083](https://github.com/Autodesk/maya-usd/pull/3083)
* Fix batch saving of layers [#3080](https://github.com/Autodesk/maya-usd/pull/3080)
* Correctly duplicate prim [#3079](https://github.com/Autodesk/maya-usd/pull/3079)
* Allow editing prims directly under the stage [#3069](https://github.com/Autodesk/maya-usd/pull/3069)
* Lookdevx: attribute enum support [#3068](https://github.com/Autodesk/maya-usd/pull/3068)
* Improve the behavior of duplication vs payload [#3065](https://github.com/Autodesk/maya-usd/pull/3065)
* Refactor material commands [#3056](https://github.com/Autodesk/maya-usd/pull/3056)
* Correct layer order in batch save dialog [#3049](https://github.com/Autodesk/maya-usd/pull/3049)
* Add command to create stages [#3045](https://github.com/Autodesk/maya-usd/pull/3045)
* Material api: hasMaterial implementation [#3042](https://github.com/Autodesk/maya-usd/pull/3042)
* Added save/restore to preserve object selection after Undoing Remove SubLayer [#3037](https://github.com/Autodesk/maya-usd/pull/3037)
* Fix crash when deleting a stage [#3031](https://github.com/Autodesk/maya-usd/pull/3031)
* Update proxy shape scene index to support interpret pick result [#3028](https://github.com/Autodesk/maya-usd/pull/3028)
* Updating tooltip to fix title on composition arcs [#3024](https://github.com/Autodesk/maya-usd/pull/3024)
* Load my USD root file as relative before I save my scene file [#3022](https://github.com/Autodesk/maya-usd/pull/3022)
* Make stage set by ID reloadable [#3010](https://github.com/Autodesk/maya-usd/pull/3010)
* Improvements for surface shader querying and handling [#3008](https://github.com/Autodesk/maya-usd/pull/3008)
* Fix error on save [#3004](https://github.com/Autodesk/maya-usd/pull/3004)
* Drag and drop a sublayer that has a relative path [#3002](https://github.com/Autodesk/maya-usd/pull/3002)
* Edit routing of composite commands [#2997](https://github.com/Autodesk/maya-usd/pull/2997)
* Recognize string properties with IsAssetIdentifier as being filename [#2994](https://github.com/Autodesk/maya-usd/pull/2994)
* Updated sub layer dialog to display the correct relative path when enabled [#2990](https://github.com/Autodesk/maya-usd/pull/2990)
* Improve preventing edits [#2986](https://github.com/Autodesk/maya-usd/pull/2986)
* Restrict namespace edits to current stage [#2977](https://github.com/Autodesk/maya-usd/pull/2977)
* Check skin cluster weights [#2971](https://github.com/Autodesk/maya-usd/pull/2971)
* Added size functions in UINodeGraphNode [#2967](https://github.com/Autodesk/maya-usd/pull/2967)
* Pulled Maya object to inherit visibility [#2964](https://github.com/Autodesk/maya-usd/pull/2964)
* Assign to component tags [#2963](https://github.com/Autodesk/maya-usd/pull/2963)
* Return nullptr from transform command instead of throwing [#2959](https://github.com/Autodesk/maya-usd/pull/2959)
* Allow material-related functions only for specific node types [#2958](https://github.com/Autodesk/maya-usd/pull/2958)
* Fix edit-target loops in proxy shape compute [#2955](https://github.com/Autodesk/maya-usd/pull/2955)
* Add edit routing for attributes [#2951](https://github.com/Autodesk/maya-usd/pull/2951)
* Fixing multiple colour sets export from Maya to USD [#2927](https://github.com/Autodesk/maya-usd/pull/2927)

**Render:**
* Use experimental OCIO hooks for MaterialX shading [#3171](https://github.com/Autodesk/maya-usd/pull/3171)
* Implement experimental OCIO support [#3153](https://github.com/Autodesk/maya-usd/pull/3153)
* Show|Selection Highlighting in the Viewport doesn't work with USD Proxyshape [#3142](https://github.com/Autodesk/maya-usd/pull/3142)
* Prevent primInfo errors [#3085](https://github.com/Autodesk/maya-usd/pull/3085)
* Maya crashes if USD instanceable is used together with varying visibility [#3059](https://github.com/Autodesk/maya-usd/pull/3059)
* Use Hd_VertexAdjacency API directly [#3046](https://github.com/Autodesk/maya-usd/pull/3046)
* Fix broken UDIMs in VP2 and add unit test [#3044](https://github.com/Autodesk/maya-usd/pull/3044)
* Legacy MtoH:
  * Separate mtoh .mod file [#3038](https://github.com/Autodesk/maya-usd/pull/3038)
  * Turned off legacy mtoh build [#2989](https://github.com/Autodesk/maya-usd/pull/2989)
* Single channel should show as black and white in viewport [#2952](https://github.com/Autodesk/maya-usd/pull/2952)
* Add getter/setter for enableMaterials in HdMayaSceneDelegate [#2938](https://github.com/Autodesk/maya-usd/pull/2938)

**Documentation:**
* Add documentation for import chaser [#3160](https://github.com/Autodesk/maya-usd/pull/3160)
* Update Readme.md to include Arbitrary Attributes to export options [#3118](https://github.com/Autodesk/maya-usd/pull/3118)
* Update EditRouting.md to add example [#3103](https://github.com/Autodesk/maya-usd/pull/3103)
* Docs on turning off edit routing [#3014](https://github.com/Autodesk/maya-usd/pull/3014)
* Updating docs with Maya 2024 [#2998](https://github.com/Autodesk/maya-usd/pull/2998)

**Miscellaneous:**
* Fixing saving the pinned stage [#3130](https://github.com/Autodesk/maya-usd/pull/3130)
* Mark USD and MayaUSD Python bindings as safe [#3119](https://github.com/Autodesk/maya-usd/pull/3119)
* Clearer layer file names [#3073](https://github.com/Autodesk/maya-usd/pull/3073)
* Fix shiboken crash [#3057](https://github.com/Autodesk/maya-usd/pull/3057)
* Pin stage in Layer Editor [#3052](https://github.com/Autodesk/maya-usd/pull/3052)
* Fixes a stage leak from USD caches [#3035](https://github.com/Autodesk/maya-usd/pull/3035)
* Fix Attribute Editor refresh [#3034](https://github.com/Autodesk/maya-usd/pull/3034)
* Prevent right click-material assignment for physics-related nodes [#3026](https://github.com/Autodesk/maya-usd/pull/3026)
* Fix MEL warning in layer load dialog [#3011](https://github.com/Autodesk/maya-usd/pull/3011)
* Remove explicit call to sceneIndex::Populate [#2983](https://github.com/Autodesk/maya-usd/pull/2983)
* Undoing rename of prim causes prim's icon to change to def [#2965](https://github.com/Autodesk/maya-usd/pull/2965)

## [v0.23.0] - 2023-03-29

**Build:**
* Add cmake test fixes to go with .mod file changes made recently [#2931](https://github.com/Autodesk/maya-usd/pull/2931)
* Create a unit test for display layers with instanced geometry [#2913](https://github.com/Autodesk/maya-usd/pull/2913)
* Restore disabled test [#2889](https://github.com/Autodesk/maya-usd/pull/2889)
* Reduce testUsdExportPackage failures [#2888](https://github.com/Autodesk/maya-usd/pull/2888)
* Add PXR_USD_WINDOWS_DLL_PATH to all of the windows .mod templates [#2867](https://github.com/Autodesk/maya-usd/pull/2867)
* Create a more extensive unit test for display layer support during USD to USD duplication [#2866](https://github.com/Autodesk/maya-usd/pull/2866)
* Create an test for USD root layer having a relative path [#2861](https://github.com/Autodesk/maya-usd/pull/2861)

**Translation Framework:**
* Use "mtl" as default scope name on export [#2903](https://github.com/Autodesk/maya-usd/pull/2903)
* Keep hierarchy window on top of import window [#2892](https://github.com/Autodesk/maya-usd/pull/2892)
* Import meshes with single samples as static geometry [#2860](https://github.com/Autodesk/maya-usd/pull/2860)
* Write sourceColorSpace=raw when using Utility - Raw [#2782](https://github.com/Autodesk/maya-usd/pull/2782)
* Add Custom Layer Data export flag [#2754](https://github.com/Autodesk/maya-usd/pull/2754)

**Workflow:**
* Outliner: Renaming prims changes icon to def [#2940](https://github.com/Autodesk/maya-usd/pull/2940)
* Block command when layers are muted [#2934](https://github.com/Autodesk/maya-usd/pull/2934)
* Retrieve Ufe runtime id by data source integer reference [#2933](https://github.com/Autodesk/maya-usd/pull/2933)
* Error: Failed verification: on File > New [#2929](https://github.com/Autodesk/maya-usd/pull/2929)
* Transform command error reporting [#2928](https://github.com/Autodesk/maya-usd/pull/2928)
* Sync session layer for various commands [#2922](https://github.com/Autodesk/maya-usd/pull/2922)
* isDisconnected local variable treated as error if pxr > v0.23 [#2918](https://github.com/Autodesk/maya-usd/pull/2918)
* clearMetadata doesn't work for UsdShadeNodeGraph attribute [#2912](https://github.com/Autodesk/maya-usd/pull/2912)
* Improve duplicate command [#2895](https://github.com/Autodesk/maya-usd/pull/2895)
* Deleting a connected compound attribute does not delete its connection [#2894](https://github.com/Autodesk/maya-usd/pull/2894)
* Add a version upper bound on Hydra workaround [#2853](https://github.com/Autodesk/maya-usd/pull/2853)
* Deleting a connection does not delete source and destination properties [#2852](https://github.com/Autodesk/maya-usd/pull/2852)
* Missing metadata notification [#2846](https://github.com/Autodesk/maya-usd/pull/2846)
* Deleting a node does not delete its connections [#2837](https://github.com/Autodesk/maya-usd/pull/2837)

**Render:**
* Add prototypes to display layers [#2923](https://github.com/Autodesk/maya-usd/pull/2923)
* Add a getter for usdimaging delegate in proxy adapter [#2915](https://github.com/Autodesk/maya-usd/pull/2915)
* Add env var MAYAUSD_VP2_USE_ONLY_PREVIEWSURFACE [#2908](https://github.com/Autodesk/maya-usd/pull/2908)
* Display Layer:
  * Implement color override for instanced geometry [#2907](https://github.com/Autodesk/maya-usd/pull/2907)
  * Implement untextured mode for instanced geometry [#2897](https://github.com/Autodesk/maya-usd/pull/2897)
  * Implement wireframe mode for instanced geometry [#2883](https://github.com/Autodesk/maya-usd/pull/2883)
  * Implement template and reference modes for instanced geometry [#2875](https://github.com/Autodesk/maya-usd/pull/2875)
* Reset unauthored attributes to default on Sync [#2898](https://github.com/Autodesk/maya-usd/pull/2898)
* Extend Viewport and Outliner menus to allowing assigning new or existing materials [#2896](https://github.com/Autodesk/maya-usd/pull/2896)
* Correctly handle MaterialX filename inputs [#2891](https://github.com/Autodesk/maya-usd/pull/2891)
* Avoid diving into material when user selects "Show in LookdevX" [#2873](https://github.com/Autodesk/maya-usd/pull/2873)
* Fix untextured rendering of ND_surface [#2870](https://github.com/Autodesk/maya-usd/pull/2870)

**Miscellaneous:**
* Fix SceneIndex library path (linux/mac need lib prefix) [#2948](https://github.com/Autodesk/maya-usd/pull/2948)
* Detect if USD was built with extra security patches [#2945](https://github.com/Autodesk/maya-usd/pull/2945)
* USD Layer Editor in the Windows menu [#2935](https://github.com/Autodesk/maya-usd/pull/2935)
* Removed DisplayColor from ShadingMode export command [#2930](https://github.com/Autodesk/maya-usd/pull/2930)
* Update title of Duplicate As Usd Data Option Dialog to match [#2916](https://github.com/Autodesk/maya-usd/pull/2916)
* Add "Unassign Material" function to viewport context menu [#2910](https://github.com/Autodesk/maya-usd/pull/2910)
* Avoid crash when world node has an unexpected name [#2906](https://github.com/Autodesk/maya-usd/pull/2906)
* Preview of relative path (resolved and unresolved) [#2905](https://github.com/Autodesk/maya-usd/pull/2905)
* Fix rename when staged multiple times [#2904](https://github.com/Autodesk/maya-usd/pull/2904)
* Allow loading relative sub layers [#2902](https://github.com/Autodesk/maya-usd/pull/2902)
* Prim AE template cleanup Xformable section [#2901](https://github.com/Autodesk/maya-usd/pull/2901)
* Removed an artifical crash in transform operations and replaced it with assert [#2900](https://github.com/Autodesk/maya-usd/pull/2900)
* Use Load/Unload for payload [#2899](https://github.com/Autodesk/maya-usd/pull/2899)
* Update edit target when saving an anonymous layer [#2890](https://github.com/Autodesk/maya-usd/pull/2890)
* Sub-layers relative to their parent [#2885](https://github.com/Autodesk/maya-usd/pull/2885)
* Save the target layer [#2884](https://github.com/Autodesk/maya-usd/pull/2884)
* Release the memory a data handle is holding onto when exporting meshes [#2882](https://github.com/Autodesk/maya-usd/pull/2882)
* Improve error messages [#2876](https://github.com/Autodesk/maya-usd/pull/2876)
* Implement scale pivot for common API [#2868](https://github.com/Autodesk/maya-usd/pull/2868)
* Fix bulk save of root layer filename [#2864](https://github.com/Autodesk/maya-usd/pull/2864)
* Scene hierarchies need flattening scene index filter [#2862](https://github.com/Autodesk/maya-usd/pull/2862)
* Allow USD ref to be relative [#2855](https://github.com/Autodesk/maya-usd/pull/2855)
* Flush the diagnostic when Maya exits [#2830](https://github.com/Autodesk/maya-usd/pull/2830)


## [v0.22.0] - 2023-02-15

**Build:**
* Skip test if UFE attribute metadata not available [#2834](https://github.com/Autodesk/maya-usd/pull/2834)
* Use the PXR namespace directive [#2827](https://github.com/Autodesk/maya-usd/pull/2827)
* Bump UFE version to 4.0 [#2819](https://github.com/Autodesk/maya-usd/pull/2819)
* Fix VP2 test [#2787](https://github.com/Autodesk/maya-usd/pull/2787)
* Fix empty values for remapUVSetsTo [#2784](https://github.com/Autodesk/maya-usd/pull/2784)
* Fix USD Min build after outliner rework [#2781](https://github.com/Autodesk/maya-usd/pull/2781)

**Translation Framework:**
* Improve NURBS export [#2835](https://github.com/Autodesk/maya-usd/pull/2835)
* Import as anim cache [#2829](https://github.com/Autodesk/maya-usd/pull/2829)
* Token updates in preparation for Schema Versioning [#2826](https://github.com/Autodesk/maya-usd/pull/2826)
* Invalid beziers [#2813](https://github.com/Autodesk/maya-usd/pull/2813)
* Fix material export when preserving UVset names [#2776](https://github.com/Autodesk/maya-usd/pull/2776)
* Export Normals by default and modify docs to match code behaviour [#2768](https://github.com/Autodesk/maya-usd/pull/2768)

**Workflow:**
* Relative File Paths:
  * Save USD root file relative to scene file [#2854](https://github.com/Autodesk/maya-usd/pull/2854) [#2839](https://github.com/Autodesk/maya-usd/pull/2839)
  * Load my USD root file as relative to the scene file [#2848](https://github.com/Autodesk/maya-usd/pull/2848)
* Invalid extension in Layer editor would cause crash [#2847](https://github.com/Autodesk/maya-usd/pull/2847) [#2845](https://github.com/Autodesk/maya-usd/pull/2845)
* Clean options of materials scope name [#2840](https://github.com/Autodesk/maya-usd/pull/2840)
* Preserve NodeGraph boundaries on duplicate [#2836](https://github.com/Autodesk/maya-usd/pull/2836)
* Prevents a hang that can occur when forming certain connections [#2828](https://github.com/Autodesk/maya-usd/pull/2828)
* Prettify: Add space on lower to upper transition [#2824](https://github.com/Autodesk/maya-usd/pull/2824)
* Add Ufe log on console to notify shader attribute without default value [#2823](https://github.com/Autodesk/maya-usd/pull/2823)
* Prevent string conversions to throw exceptions with empty value [#2822](https://github.com/Autodesk/maya-usd/pull/2822)
* Fix plugInfo to use relative path [#2821](https://github.com/Autodesk/maya-usd/pull/2821)
* Fix USD export curve options [#2820](https://github.com/Autodesk/maya-usd/pull/2820)
* Shorten nice names by trimming schema name [#2818](https://github.com/Autodesk/maya-usd/pull/2818)
* Add reparenting rules for Shaders, NodeGraphs and Materials [#2811](https://github.com/Autodesk/maya-usd/pull/2811)
* Allow native file dialog when loading layers [#2807](https://github.com/Autodesk/maya-usd/pull/2807)
* Fix crash when editing two prims [#2806](https://github.com/Autodesk/maya-usd/pull/2806)
* Add scene index plugin code [#2805](https://github.com/Autodesk/maya-usd/pull/2805)
* Correctly identify orphaned edited prims [#2803](https://github.com/Autodesk/maya-usd/pull/2803)
* mayaUsd.ufe.stagePath() returns incorrect value [#2801](https://github.com/Autodesk/maya-usd/pull/2801)
* Delete compound attributes does not remove their connections [#2800](https://github.com/Autodesk/maya-usd/pull/2800)
* Improve diagnostics [#2799](https://github.com/Autodesk/maya-usd/pull/2799)
* Interaction between deactivation and cancel edit [#2794](https://github.com/Autodesk/maya-usd/pull/2794)
* Merge top UFE classification for glsfx and USD shader [#2791](https://github.com/Autodesk/maya-usd/pull/2791)
* UFE USD camera commands [#2789](https://github.com/Autodesk/maya-usd/pull/2789)
* Correctly expose Material and NodeGraph attributes in Maya Attribute Editor [#2788](https://github.com/Autodesk/maya-usd/pull/2788)
* Fix a bug where materials got created in the wrong scope [#2783](https://github.com/Autodesk/maya-usd/pull/2783)
* Outliner rework of Material items [#2775](https://github.com/Autodesk/maya-usd/pull/2775)
* Correctly disable merge to USD [#2769](https://github.com/Autodesk/maya-usd/pull/2769)
* merge-to-USD into variants [#2764](https://github.com/Autodesk/maya-usd/pull/2764)
* Auto re-edit when merging a Maya Reference [#2762](https://github.com/Autodesk/maya-usd/pull/2762)

**Render:**
* Adding a MaterialX v1.38.5 update for closures [#2858](https://github.com/Autodesk/maya-usd/pull/2858)
* Enable instances to determine non empty stage in preparation for USD v0.23.02 [#2844](https://github.com/Autodesk/maya-usd/pull/2844)
* UDIM: Use default LOD values in anisotropic mode [#2816](https://github.com/Autodesk/maya-usd/pull/2816)
* Remove frequent warning in Arnold/RenderMan shading workflows [#2814](https://github.com/Autodesk/maya-usd/pull/2814)
* Fix opacity computations for MaterialX and USD materials [#2790](https://github.com/Autodesk/maya-usd/pull/2790)
* Implement display layer's hide-on-playback flag for instanced geometry [#2780](https://github.com/Autodesk/maya-usd/pull/2780)

**Documentation:**
* Update documentation of the export command [#2808](https://github.com/Autodesk/maya-usd/pull/2808)


## [v0.21.0] - 2022-12-13

**Build:**
* Update deprecated Python 3.7 functions [#2752](https://github.com/Autodesk/maya-usd/pull/2752)
* Add includes that were being pulled transitively [#2732](https://github.com/Autodesk/maya-usd/pull/2732)
* Adapt testConnections for USD 22.11 [#2726](https://github.com/Autodesk/maya-usd/pull/2726)
* Remove reference to vaccine python code [#2715](https://github.com/Autodesk/maya-usd/pull/2715)
* Build MayaUsd on RHEL8 with new ABI [#2675](https://github.com/Autodesk/maya-usd/pull/2675)
* Removing unnecessary explicit 'pxr' namespacing [#2654](https://github.com/Autodesk/maya-usd/pull/2654)
* Create an auto-test for Display Layer attributes [#2647](https://github.com/Autodesk/maya-usd/pull/2647)
* Added test for getAllStages() versus delete [#2645](https://github.com/Autodesk/maya-usd/pull/2645)

**Translation Framework:**
* Issue error message for invalid variant selection [#2743](https://github.com/Autodesk/maya-usd/pull/2743)
* Make import __not__ use the shared stage cache [#2734](https://github.com/Autodesk/maya-usd/pull/2734)
* Export parent offset matrix [#2731](https://github.com/Autodesk/maya-usd/pull/2731)
* Anim channel code clarification [#2730](https://github.com/Autodesk/maya-usd/pull/2730)
* Fix default export logic if subdivScheme is none [#2713](https://github.com/Autodesk/maya-usd/pull/2713)
* Worldspace flag for Maya USD export [#2708](https://github.com/Autodesk/maya-usd/pull/2708)
* USD import command support prim variants [#2703](https://github.com/Autodesk/maya-usd/pull/2703)
* Handle timecodes-per-second when flipping shared state [#2701](https://github.com/Autodesk/maya-usd/pull/2701)
* Support duplicate xform ops [#2695](https://github.com/Autodesk/maya-usd/pull/2695)
* Transfer state from root layer when toggling shareable stage [#2691](https://github.com/Autodesk/maya-usd/pull/2691)
* Handle rename and reparent in the orphaned nodes manager [#2690](https://github.com/Autodesk/maya-usd/pull/2690)
* Wipe and restore orphaned session data [#2684](https://github.com/Autodesk/maya-usd/pull/2684)
* Serialization of pull variant info into JSON [#2682](https://github.com/Autodesk/maya-usd/pull/2682)
* Fix nurbs curves UI when using export job context [#2664](https://github.com/Autodesk/maya-usd/pull/2664)

**Workflow:**
* Connections now handled internally [#2766](https://github.com/Autodesk/maya-usd/pull/2766)
* Adds default UsdShaderAttributeDef uisoftmin value of 0.0 [#2760](https://github.com/Autodesk/maya-usd/pull/2760)
* Allow retrieving Arnold "uimin" metadata [#2755](https://github.com/Autodesk/maya-usd/pull/2755)
* Duplicated prim to be on the Display Layer that the source prim was on [#2747](https://github.com/Autodesk/maya-usd/pull/2747) [#2700](https://github.com/Autodesk/maya-usd/pull/2700)
* Fix duplication of homonyms [#2746](https://github.com/Autodesk/maya-usd/pull/2746)
* Improve usability of NoExec commands [#2737](https://github.com/Autodesk/maya-usd/pull/2737)
* More precise edit restrictions for metadata and variants [#2736](https://github.com/Autodesk/maya-usd/pull/2736)
* Allow connecting filename attributes [#2733](https://github.com/Autodesk/maya-usd/pull/2733)
* Fix lost connection for output port with parent [#2729](https://github.com/Autodesk/maya-usd/pull/2729)
* Test if visibility edit is valid before returning command [#2725](https://github.com/Autodesk/maya-usd/pull/2725)
* Allow creating prims in weaker layers [#2720](https://github.com/Autodesk/maya-usd/pull/2720)
* Preserve the Layer Manager expanded layers [#2718](https://github.com/Autodesk/maya-usd/pull/2718)
* Triggers attribute metadata changed notification [#2704](https://github.com/Autodesk/maya-usd/pull/2704)
* Unable to switch variants when Session Layer is targeted [#2698](https://github.com/Autodesk/maya-usd/pull/2698)
* Implementation batched duplication [#2687](https://github.com/Autodesk/maya-usd/pull/2687)
* Improve undo/redo capabilities [#2686](https://github.com/Autodesk/maya-usd/pull/2686)
* Implementation of UFE Material Interface [#2674](https://github.com/Autodesk/maya-usd/pull/2674)
* Bringing back the Display Layers in EditAsMaya support by fixing the crash [#2673](https://github.com/Autodesk/maya-usd/pull/2673)
* Allow custom UI hooks for mayaUsdProxyShape [#2671](https://github.com/Autodesk/maya-usd/pull/2671)
* Fix Edit-As-Maya on fresh Maya session [#2670](https://github.com/Autodesk/maya-usd/pull/2670)
* Implement filtered version of findGatewayItems() [#2666](https://github.com/Autodesk/maya-usd/pull/2666)
* Properly track usage of shared render items [#2663](https://github.com/Autodesk/maya-usd/pull/2663)
* Makes use of Materials env vars for generating scope name for assigned materials [#2658](https://github.com/Autodesk/maya-usd/pull/2658)
* Rename ports [#2644](https://github.com/Autodesk/maya-usd/pull/2644)
* Assign new material to multiple objects [#2641](https://github.com/Autodesk/maya-usd/pull/2641)
* Clearing out stage listeners before creating new ones [#2635](https://github.com/Autodesk/maya-usd/pull/2635)

**Render:**
* Performance Issue: AntiquesMall Scene now takes 20+ minutes to load in Maya [#2751](https://github.com/Autodesk/maya-usd/pull/2751)
* Fix rendering crash with MtoH [#2742](https://github.com/Autodesk/maya-usd/pull/2742)
* Point instancer: performance drop on point instanced selection [#2728](https://github.com/Autodesk/maya-usd/pull/2728)
* Implement display layer's bbox mode for instanced geometry [#2709](https://github.com/Autodesk/maya-usd/pull/2709)
* Implement display layer's visibility for instanced geometry [#2688](https://github.com/Autodesk/maya-usd/pull/2688)
* Use USD preview surface for cards draw mode [#2680](https://github.com/Autodesk/maya-usd/pull/2680)
* Update USD render when color prefs change [#2648](https://github.com/Autodesk/maya-usd/pull/2648)


## [v0.20.0] - 2022-10-25

**Build:**
- Fixes for building with gcc 11 (still using old ABI) [#2633](https://github.com/Autodesk/maya-usd/pull/2633)
- Fix for building with latest Maya PR and USD v21.8 [#2632](https://github.com/Autodesk/maya-usd/pull/2632)
- Use Pixar's namespace directive macro [#2620](https://github.com/Autodesk/maya-usd/pull/2620) [#2536](https://github.com/Autodesk/maya-usd/pull/2536)
- Broken AL_USDMaya and AL_USDTransaction builds (AL) [#2598](https://github.com/Autodesk/maya-usd/pull/2598)
- Add the python support for DagToUsdMap [#2584](https://github.com/Autodesk/maya-usd/pull/2584)
- Change how the active 3D view is retrieved in the test [#2581](https://github.com/Autodesk/maya-usd/pull/2581)
- Fix MaterialX compilation on M1 Macs [#2579](https://github.com/Autodesk/maya-usd/pull/2579)
- Build as Universal Binary 2 (x86_64 + arm64) [#2575](https://github.com/Autodesk/maya-usd/pull/2575)
- Fix configuration error when GULRAK_SOURCE_DIR is specified on Windows [#2551](https://github.com/Autodesk/maya-usd/pull/2551)
- Update to materialx 1.38.5 [#2528](https://github.com/Autodesk/maya-usd/pull/2528)

**Translation Framework:**
- Use Adapter to author MeshLightAPI for RfM's PxrMeshLight [#2613](https://github.com/Autodesk/maya-usd/pull/2613)
- Update exporters for string varnames [#2601](https://github.com/Autodesk/maya-usd/pull/2601)
- Fix export when exportRoots and stripNamespaces are combined [#2590](https://github.com/Autodesk/maya-usd/pull/2590)
- Preserve timeline during Edit-As-Maya [#2583](https://github.com/Autodesk/maya-usd/pull/2583)
- Always write color space to usd texture prims [#2548](https://github.com/Autodesk/maya-usd/pull/2548)
- Include Extra Knots for NurbsCurves Export [#2531](https://github.com/Autodesk/maya-usd/pull/2531)

**Workflow:**
- Make adding a pulled prim to the orphan manager undoable [#2662](https://github.com/Autodesk/maya-usd/pull/2662)
- Fix orphaned node discard edits with unloaded ancestor [#2638](https://github.com/Autodesk/maya-usd/pull/2638)
- Display Layers:
  - Temporarily disable support for display layers in EditAsMaya workflows [#2667](https://github.com/Autodesk/maya-usd/pull/2667)
  - Correct handling of Display List vs Variants [#2634](https://github.com/Autodesk/maya-usd/pull/2634)
  - Display Layer membership should survive the 'Edit As Maya Data' workflow [#2628](https://github.com/Autodesk/maya-usd/pull/2628)
  - Test prim undo delete vs Display Layer [#2625](https://github.com/Autodesk/maya-usd/pull/2625)
- Handle automatic creation of attribute unique name [#2622](https://github.com/Autodesk/maya-usd/pull/2622)
- Duplicate as USD Data timeline options don't show what is currently applied [#2615](https://github.com/Autodesk/maya-usd/pull/2615)
- Prevent crash when discarding deactivated Maya Ref [#2612](https://github.com/Autodesk/maya-usd/pull/2612)
- Fix namespace renaming [#2610](https://github.com/Autodesk/maya-usd/pull/2610)
- Handle trailing # in rename [#2604](https://github.com/Autodesk/maya-usd/pull/2604)
- Do not change suffix if renamed to the same name [#2600](https://github.com/Autodesk/maya-usd/pull/2600)
- Add support for Generic Shader Attributes [#2595](https://github.com/Autodesk/maya-usd/pull/2595)
- Adds support for converting strings with options into enum strings [#2588](https://github.com/Autodesk/maya-usd/pull/2588)
- Add layer muting serialization to the proxy shape [#2580](https://github.com/Autodesk/maya-usd/pull/2580)
- Initial implementation of AETemplate control creators [#2578](https://github.com/Autodesk/maya-usd/pull/2578)
- Hide orphaned pulled nodes on ancestor structure change [#2576](https://github.com/Autodesk/maya-usd/pull/2576)
- Update relationship targets and connection sources on prim rename [#2574](https://github.com/Autodesk/maya-usd/pull/2574)
- Workaround for USD Issue 2013 [#2569](https://github.com/Autodesk/maya-usd/pull/2569)
- Cached to USD dialog: Label gets cut off on parent prim info message [#2566](https://github.com/Autodesk/maya-usd/pull/2566)
- When adding an Xform to a USD prim two entries are shown in the Channel Box [#2565](https://github.com/Autodesk/maya-usd/pull/2565)
- Set icon disabled when Maya object is orphaned [#2561](https://github.com/Autodesk/maya-usd/pull/2561)
- Add support for doubles to GetDictionaryFromArgDatabase [#2547](https://github.com/Autodesk/maya-usd/pull/2547)
- Route visibility edits [#2546](https://github.com/Autodesk/maya-usd/pull/2546)
- Fix issues deleting attributes on NodeGraphs [#2544](https://github.com/Autodesk/maya-usd/pull/2544)
- Disconnection code improvements [#2539](https://github.com/Autodesk/maya-usd/pull/2539)
- Improve connecting to surface experience [#2534](https://github.com/Autodesk/maya-usd/pull/2534)
- Disallow switching variants in a weaker layer [#2533](https://github.com/Autodesk/maya-usd/pull/2533)
- Handle scope for read-only transform [#2530](https://github.com/Autodesk/maya-usd/pull/2530)
- Fix removing multiple layers at the same time [#2523](https://github.com/Autodesk/maya-usd/pull/2523)
- Allowing a new prim spec to be written in edit target layers [#2390](https://github.com/Autodesk/maya-usd/pull/2390)

**Render:**
- Disable Edit as Maya for materials [#2631](https://github.com/Autodesk/maya-usd/pull/2631)
- Defaults materials scope name to mtl and allows setting [#2624](https://github.com/Autodesk/maya-usd/pull/2624)
- Display Layers:
  - Prims wireframes should have a specific color and opacity when in a Display Layer [#2618](https://github.com/Autodesk/maya-usd/pull/2618)
  - When "Enable Overrides" is unchecked for a Display Layer USD Data still uses Bounding Box [#2611](https://github.com/Autodesk/maya-usd/pull/2611)
  - Prims should be in untextured mode when in a Display Layer [#2599](https://github.com/Autodesk/maya-usd/pull/2599)
  - Prims should be in bounding box mode when in a Display Layer [#2542](https://github.com/Autodesk/maya-usd/pull/2542)
  - Prims should be in unshaded mode when in a Display Layer [#2562](https://github.com/Autodesk/maya-usd/pull/2562)
- Show correct render backend name [#2617](https://github.com/Autodesk/maya-usd/pull/2617)
- USD needs to support untextured mode like Maya does [#2570](https://github.com/Autodesk/maya-usd/pull/2570)
- Use hydra material id fix [#2568](https://github.com/Autodesk/maya-usd/pull/2568)
- Mark primitives as dirty if the shader has been rebuilt [#2564](https://github.com/Autodesk/maya-usd/pull/2564)
- Fix MaterialX conversion crash with invalid graphs [#2543](https://github.com/Autodesk/maya-usd/pull/2543)
- Update callsite to use new Hydra API [#2538](https://github.com/Autodesk/maya-usd/pull/2538)
- Flush all resource caches on exit [#2529](https://github.com/Autodesk/maya-usd/pull/2529)

**Documentation:**
- Update build.md with Ufe version information [#2616](https://github.com/Autodesk/maya-usd/pull/2616)
- Update build.md with USD v22.08 support [#2573](https://github.com/Autodesk/maya-usd/pull/2573)
- Update CLA [#2535](https://github.com/Autodesk/maya-usd/pull/2535)
- Document the Maya Ref Edit router [#2526](https://github.com/Autodesk/maya-usd/pull/2526)

## [v0.19.0] - 2022-08-19

**Build:**
- Added test for group pivot options Maya fix [#2525](https://github.com/Autodesk/maya-usd/pull/2525)
- Display Layers Tests
  - editDisplayLayerMembers command is now "opt-in" for Ufe [#2524](https://github.com/Autodesk/maya-usd/pull/2524)
  - Changing a prim's metadata removes it from display layer [#2494](https://github.com/Autodesk/maya-usd/pull/2494)
  - Empty Display Layer command leaves invalid Ufe paths fUfeMembers [#2476](https://github.com/Autodesk/maya-usd/pull/2476)
  - Reparent/Rename/Delete test [#2459](https://github.com/Autodesk/maya-usd/pull/2459)
  - Save/restore with scene [#2455](https://github.com/Autodesk/maya-usd/pull/2455)
- Add test for xform -cpc support [#2513](https://github.com/Autodesk/maya-usd/pull/2513)
- RfM Translator Test Improvements [#2510](https://github.com/Autodesk/maya-usd/pull/2510)
- Detect Ufe SceneSegmentHandler support directly [#2467](https://github.com/Autodesk/maya-usd/pull/2467)
- MacOS: mayausd doesn't load on older Mac systems (10.15.7) [#2453](https://github.com/Autodesk/maya-usd/pull/2453)
- Run gizmo tests in 2023.2 [#2445](https://github.com/Autodesk/maya-usd/pull/2445)
- Test Maya fixes for center pivot [#2435](https://github.com/Autodesk/maya-usd/pull/2435)
- Re-enable testUsdChangeProcessingProxy [#2434](https://github.com/Autodesk/maya-usd/pull/2434)
- Provide explicit export options in tests [#2431](https://github.com/Autodesk/maya-usd/pull/2431)
- Set the color and specular roughness on the default standard surface [#2427](https://github.com/Autodesk/maya-usd/pull/2427)
- Test pixelMove UFE support [#2412](https://github.com/Autodesk/maya-usd/pull/2412)
- PluginLight needs to use the Light Icon instead of falling back to Xform icon [#2406](https://github.com/Autodesk/maya-usd/pull/2406)
- Force Y-up axis for image compare tests [#2379](https://github.com/Autodesk/maya-usd/pull/2379)
- Creating a testVP2RenderDelegateLight test [#2378](https://github.com/Autodesk/maya-usd/pull/2378)
- Fixing autotest failures that arise after enabling UFE Proxy Lights in Maya [#2373](https://github.com/Autodesk/maya-usd/pull/2373)
- Fix capitalization error on include file [#2362](https://github.com/Autodesk/maya-usd/pull/2362)
- Use UsdGeomPrimvarAPI in UVExport Tests [#2361](https://github.com/Autodesk/maya-usd/pull/2361)
- Replace UsdGeomImageable primvar methods [#2327](https://github.com/Autodesk/maya-usd/pull/2327)

**Translation Framework:**
- Use Sdr for RfM light translation [#2511](https://github.com/Autodesk/maya-usd/pull/2511)
- Author normalization attribute when exporting RfM Lights [#2496](https://github.com/Autodesk/maya-usd/pull/2496)
- Skip Material export if no conversion was selected [#2488](https://github.com/Autodesk/maya-usd/pull/2488)
- Support Maya reference prim transform round trip [#2462](https://github.com/Autodesk/maya-usd/pull/2462)
- Allow resetting material settings [#2451](https://github.com/Autodesk/maya-usd/pull/2451)
- Fix handling of empty materialCollectionsPath [#2442](https://github.com/Autodesk/maya-usd/pull/2442)
- Static caching still caches animation [#2416](https://github.com/Autodesk/maya-usd/pull/2416)
- USDExport: using "parentScope" and "exportRoots" flags together returns an error [#2400](https://github.com/Autodesk/maya-usd/pull/2400)
- Respect prim writer "ShouldPruneChildren" when shading mode is useRegistry [#2385](https://github.com/Autodesk/maya-usd/pull/2385)
- Updated NURBS curve export for widths and knots attributes [#2326](https://github.com/Autodesk/maya-usd/pull/2326)
- Fixed up shading mode export options and display colour fallback behaviour [#2324](https://github.com/Autodesk/maya-usd/pull/2324)
- Add namespace support to USD import file translators [#2308](https://github.com/Autodesk/maya-usd/pull/2308)
- The geomBindTransform should not be added to the mesh's transform [#2277](https://github.com/Autodesk/maya-usd/pull/2277)
- Add support for default rgba values and color threshold for exporting [#2250](https://github.com/Autodesk/maya-usd/pull/2250)

**Workflow:**
- Implement UI node graph node [#2520](https://github.com/Autodesk/maya-usd/pull/2520)
- MayaUSD implementation of the AddAttribute API [#2514](https://github.com/Autodesk/maya-usd/pull/2514)
- Undo of delete must update stage cache [#2503](https://github.com/Autodesk/maya-usd/pull/2503)
- Support Normal3f and Point3f USD type [#2501](https://github.com/Autodesk/maya-usd/pull/2501)
- Copy transform to the correct variant [#2497](https://github.com/Autodesk/maya-usd/pull/2497)
- Set a default prim when caching to USD [#2492](https://github.com/Autodesk/maya-usd/pull/2492)
- Default component tag setting (enabled) bloats USD file export [#2489](https://github.com/Autodesk/maya-usd/pull/2489)
- Add Outliner icons for shading types [#2485](https://github.com/Autodesk/maya-usd/pull/2485)
- Adds a nullptr check to UsdHierachyHandler::hierarchy() [#2478](https://github.com/Autodesk/maya-usd/pull/2478)
- Fix attribute creation on connect [#2472](https://github.com/Autodesk/maya-usd/pull/2472)
- Add contextual menu to create material nodes [#2466](https://github.com/Autodesk/maya-usd/pull/2466)
- Fix discovery and creation of namespaced nodes [#2457](https://github.com/Autodesk/maya-usd/pull/2457)
- Preserve session layer on save [#2447](https://github.com/Autodesk/maya-usd/pull/2447)
- Propagate MaterialX metadata info [#2441](https://github.com/Autodesk/maya-usd/pull/2441)
- Reset all merge and duplicate options [#2439](https://github.com/Autodesk/maya-usd/pull/2439)
- Prepend reference/payload doesn't work in Caching Maya Ref [#2436](https://github.com/Autodesk/maya-usd/pull/2436)
- Add list/create/delete connections  [#2433](https://github.com/Autodesk/maya-usd/pull/2433)
- Assign material to selection [#2429](https://github.com/Autodesk/maya-usd/pull/2429)
- Preserve unsaved root layer [#2428](https://github.com/Autodesk/maya-usd/pull/2428)
- Progress bar for import/export/push/pull [#2421](https://github.com/Autodesk/maya-usd/pull/2421)
- Reparent on deactivated [#2419](https://github.com/Autodesk/maya-usd/pull/2419)
- UFE light support in versions below UFE 4 (particularly UFE 3.1) [#2417](https://github.com/Autodesk/maya-usd/pull/2417)
- Unauthored attrs changes [#2414](https://github.com/Autodesk/maya-usd/pull/2414)
- Fix unshared stage toggling and loading [#2410](https://github.com/Autodesk/maya-usd/pull/2410)
- Set options when Duplicate to USD Data [#2407](https://github.com/Autodesk/maya-usd/pull/2407)
- Remember shared/unshared load state [#2398](https://github.com/Autodesk/maya-usd/pull/2398)
- The set "usdEditAsMaya" doesn't disappear when cancelling editing as Maya [#2397](https://github.com/Autodesk/maya-usd/pull/2397) [#2386](https://github.com/Autodesk/maya-usd/pull/2386)
- Only save the user-selected filter type when the dialog is confirmed [#2395](https://github.com/Autodesk/maya-usd/pull/2395)
- Fix crash when deleting a stage [#2393](https://github.com/Autodesk/maya-usd/pull/2393)
- Fix issues between two versions of standard_surface [#2388](https://github.com/Autodesk/maya-usd/pull/2388)
- Fix merging of normals [#2387](https://github.com/Autodesk/maya-usd/pull/2387)
- Reset auto-edit in all variants [#2377](https://github.com/Autodesk/maya-usd/pull/2377)
- Add execution validation during pull/push [#2374](https://github.com/Autodesk/maya-usd/pull/2374)
- Turn on Maya Reference for users [#2369](https://github.com/Autodesk/maya-usd/pull/2369)
- Applied schemas are broken in the AE template with USD v22.05 [#2368](https://github.com/Autodesk/maya-usd/pull/2368)
- Improved USD type support [#2364](https://github.com/Autodesk/maya-usd/pull/2364)
- Allow topological modifications [#2363](https://github.com/Autodesk/maya-usd/pull/2363)
- Turn off auto-edit when caching [#2360](https://github.com/Autodesk/maya-usd/pull/2360)
- Selecting a camera and framing it with "f" shows the origin instead of the camera [#2359](https://github.com/Autodesk/maya-usd/pull/2359)
- Catch exceptions during notifications [#2358](https://github.com/Autodesk/maya-usd/pull/2358)
- Do not offer USDZ as a file format to cache to USD [#2356](https://github.com/Autodesk/maya-usd/pull/2356)
- Maya reference node naming [#2350](https://github.com/Autodesk/maya-usd/pull/2350)
- Lights Outliner icon is not getting picked up [#2349](https://github.com/Autodesk/maya-usd/pull/2349)
- Merge to USD dialog window height - cannot scroll down [#2348](https://github.com/Autodesk/maya-usd/pull/2348)
- Cache to USD dialog: empty space below options [#2347](https://github.com/Autodesk/maya-usd/pull/2347)
- Allow creating prim in stronger layer [#2337](https://github.com/Autodesk/maya-usd/pull/2337)
- Avoid auto-edit check on invalid prim [#2336](https://github.com/Autodesk/maya-usd/pull/2336)
- Obey file-format choice in Layer Editor [#2334](https://github.com/Autodesk/maya-usd/pull/2334)
- cache-to-USD file type persistence [#2316](https://github.com/Autodesk/maya-usd/pull/2316)
- Add classifications and metadata to Node/Attribute def [#2315](https://github.com/Autodesk/maya-usd/pull/2315)
- ProxyShapeSceneSegmentHandler and UsdCameraHandler::findCameras [#2307](https://github.com/Autodesk/maya-usd/pull/2307)
- Implemented Outliner orphaned nodes, pulled node tooltips and italics [#2285](https://github.com/Autodesk/maya-usd/pull/2285)

**Render:**
- Display Layers:
  - I'd like my prims to be referenced in a Display Layer [#2519](https://github.com/Autodesk/maya-usd/pull/2519)
  - I'd like my prims to be templated in a Display Layer [#2508](https://github.com/Autodesk/maya-usd/pull/2508)
  - Fixing a visibility update issue when USD data and Display Layer is saved to Maya file [#2493](https://github.com/Autodesk/maya-usd/pull/2493)
  - I'd like my prims to hide with the Display Layer on playback [#2475](https://github.com/Autodesk/maya-usd/pull/2475)
  - Modifying Display Layer membership or drawOverride attr for MayaUsd prims is very slow [#2458](https://github.com/Autodesk/maya-usd/pull/2458)
  - Respect Display Layer visibility [#2418](https://github.com/Autodesk/maya-usd/pull/2418)
- Cleanup UsdImagingGL legacy engine [#2512](https://github.com/Autodesk/maya-usd/pull/2512)
- Hide on playback option doesn't work on re-open [#2509](https://github.com/Autodesk/maya-usd/pull/2509)
- Prevent skeleton moving when underlying base mesh is moved [#2491](https://github.com/Autodesk/maya-usd/pull/2491)
- Check for invalid prim [#2490](https://github.com/Autodesk/maya-usd/pull/2490)
- Improve classifications of Arnold shaders [#2484](https://github.com/Autodesk/maya-usd/pull/2484)
- Explicitly set specular rougness to 0.4 [#2482](https://github.com/Autodesk/maya-usd/pull/2482)
- Show bound surface materials as extra AE tabs [#2470](https://github.com/Autodesk/maya-usd/pull/2470)
- Implementing testVP2RenderDelegateCameras autotest [#2383](https://github.com/Autodesk/maya-usd/pull/2383)
- Fix hydra crash with skydome [#2331](https://github.com/Autodesk/maya-usd/pull/2331)
- Suppress UFE-proxy lights in Maya Scene Delegate to avoid lights duplication [#2328](https://github.com/Autodesk/maya-usd/pull/2328)
- Use getMemberPaths for material export when it is available [#2322](https://github.com/Autodesk/maya-usd/pull/2322)
- Add SdfAssetPath support to renderGlobals [#2314](https://github.com/Autodesk/maya-usd/pull/2314)
- Show nested subcomponents in expanded representation [#2289](https://github.com/Autodesk/maya-usd/pull/2289)

**Documentation:**
- Add docs about saving layers [#2483](https://github.com/Autodesk/maya-usd/pull/2483)
- Updated MaterialX documentation for recent changes [#2454](https://github.com/Autodesk/maya-usd/pull/2454)
- Update build.md with Maya 2023 info [#2450](https://github.com/Autodesk/maya-usd/pull/2450)
- Fixed documentation for subdivision scheme [#2423](https://github.com/Autodesk/maya-usd/pull/2423)
- Update build.md with supported USD versions [#2401](https://github.com/Autodesk/maya-usd/pull/2401)

## [v0.18.0] - 2022-05-10

**Build:**
- Installer script doesn't install the MaterialX libs for Maya 2022 [#2302](https://github.com/Autodesk/maya-usd/pull/2302)
- Update to MaterialX 1.38.4 [#2299](https://github.com/Autodesk/maya-usd/pull/2299)
- Use pixar using directive [#2293](https://github.com/Autodesk/maya-usd/pull/2293)
- Fix build for USD versions past 21.11 [#2288](https://github.com/Autodesk/maya-usd/pull/2288)
- Add MaterialX search path for tests [#2282](https://github.com/Autodesk/maya-usd/pull/2282)
- Added support for Universal Binary 2 (x86_64 + arm64) [#2280](https://github.com/Autodesk/maya-usd/pull/2280)
- Build with gcc 9 [#2274](https://github.com/Autodesk/maya-usd/pull/2274)
- Fix cotd preflights image comparison failures [#2241](https://github.com/Autodesk/maya-usd/pull/2241)
- Add unit test for select -addFirst [#2195](https://github.com/Autodesk/maya-usd/pull/2195)

**Translation Framework:**
- Added support for importing animated bool/int type attributes [#2318](https://github.com/Autodesk/maya-usd/pull/2318)
- Enforce cache file format [#2311](https://github.com/Autodesk/maya-usd/pull/2311)
- EditAsMaya set should be a locked node [#2290](https://github.com/Autodesk/maya-usd/pull/2290)
- Added mesh export options to preserve and remap UV set names [#2283](https://github.com/Autodesk/maya-usd/pull/2283)
- The true bindPose for a skin cluster comes from it's bindPreMatrix [#2270](https://github.com/Autodesk/maya-usd/pull/2270)
- Fix missing edit after merge [#2245](https://github.com/Autodesk/maya-usd/pull/2245)
- Target layer moves & stays on session layer when editing as Maya data [#2242](https://github.com/Autodesk/maya-usd/pull/2242)
- Prevent recursive edit as Maya [#2229](https://github.com/Autodesk/maya-usd/pull/2229)
- Enable readAnimatedValues attribute on Camera translator import [#2224](https://github.com/Autodesk/maya-usd/pull/2224)
- Enable proxyAccessor in AL_USDMaya [#2222](https://github.com/Autodesk/maya-usd/pull/2222)
- Added export option to merge offset parent matrix [#2218](https://github.com/Autodesk/maya-usd/pull/2218)
- Fixed uninitialized variable that caused NaN xform values after importing [#2217](https://github.com/Autodesk/maya-usd/pull/2217)
- MaterialX subcomponent connections export [#2211](https://github.com/Autodesk/maya-usd/pull/2211)
- USDExport: using <i>parentScope</i> & <i>exportRoots</i> flags together returns an error [#2204](https://github.com/Autodesk/maya-usd/pull/2204)
- Added stage export option to disable authoring of distance unit [#2203](https://github.com/Autodesk/maya-usd/pull/2203)
- Update the MaterialNetwork conversion function [#2184](https://github.com/Autodesk/maya-usd/pull/2184)
- Final part of the code refactoring that merges more common methods into MayaUsdRPrim class [#2086](https://github.com/Autodesk/maya-usd/pull/2086)

**Workflow:**
- Implement pivot accessors for wrapped matrix ops [#2319](https://github.com/Autodesk/maya-usd/pull/2319)
- NURBS default to off when caching [#2309](https://github.com/Autodesk/maya-usd/pull/2309)
- Added a user proc in USDMenuProc [#2305](https://github.com/Autodesk/maya-usd/pull/2305)
- Minor UI fixes
  - Merge maya edits to USD option box: "cancel" button should be "close" [#2300](https://github.com/Autodesk/maya-usd/pull/2300)
  - Merge to USD optionbox width is not the same as other option boxes [#2300](https://github.com/Autodesk/maya-usd/pull/2300)
  - Cache to USD: tooltips are missing word in bold [#2300](https://github.com/Autodesk/maya-usd/pull/2300)
  - Make "Define In" options enabled [#2298](https://github.com/Autodesk/maya-usd/pull/2298)
  - Add Layer Editor menu item to the context menu of edited-as-Maya objects [#2198](https://github.com/Autodesk/maya-usd/pull/2198)
  - Add contextual menu to bind materials [#2287](https://github.com/Autodesk/maya-usd/pull/2287)
- Removed proxy shape pre/post selection events notification [#2294](https://github.com/Autodesk/maya-usd/pull/2294)
- Refactored and simplified AL_USDMaya primvar interpolation type detection [#2284](https://github.com/Autodesk/maya-usd/pull/2284)
- Fix crash with deactivate and console [#2268](https://github.com/Autodesk/maya-usd/pull/2268)
- Support UFE lights [#2266](https://github.com/Autodesk/maya-usd/pull/2266)
- For for showing deactivated children [#2265](https://github.com/Autodesk/maya-usd/pull/2265)
- Use correct way to query attribute [#2260](https://github.com/Autodesk/maya-usd/pull/2260)
- Add validation on custom transform node before accessing [#2252](https://github.com/Autodesk/maya-usd/pull/2252)
- Mark outTime attribute dirty to force updating Maya bbox [#2251](https://github.com/Autodesk/maya-usd/pull/2251)
- USD shader node def [#2243](https://github.com/Autodesk/maya-usd/pull/2243)
- Avoid errors in marking menu [#2238](https://github.com/Autodesk/maya-usd/pull/2238)
- Maya Reference auto-edit handling [#2235](https://github.com/Autodesk/maya-usd/pull/2235)
- Persist USD payload state [#2226](https://github.com/Autodesk/maya-usd/pull/2226)
- Fixed dead loop issue when deleting node [#2219](https://github.com/Autodesk/maya-usd/pull/2219)
- Reduce floating point comparison precision to avoid making edit layer dirty [#2216](https://github.com/Autodesk/maya-usd/pull/2216)
- Bad variant cache with empty pref [#2200](https://github.com/Autodesk/maya-usd/pull/2200)
- Fix for attribute editor in 2022 due to changes to callcustom in editorTemplate [#2182](https://github.com/Autodesk/maya-usd/pull/2182)

**Render:**
- Can no longer pass ALL_INSTANCES into GetScenePrimPath [#2317](https://github.com/Autodesk/maya-usd/pull/2317)
- MaterialX doesn't display in the Viewport on MacOS [#2296](https://github.com/Autodesk/maya-usd/pull/2296)
- Set the default light intensity the new way  [#2291](https://github.com/Autodesk/maya-usd/pull/2291)
- Active Selection Highlighting color doesn't work in Maya 2022 with USD [#2279](https://github.com/Autodesk/maya-usd/pull/2279)
- Remove assertion with incorrect expression [#2278](https://github.com/Autodesk/maya-usd/pull/2278)
- SetParameter crash from a worker thread [#2275](https://github.com/Autodesk/maya-usd/pull/2275)
- Add optionVars for controlling specular lighting options [#2261](https://github.com/Autodesk/maya-usd/pull/2261)
- Use USDLuxTokens for spot light [#2253](https://github.com/Autodesk/maya-usd/pull/2253)
- Fix MaterialX not working with AMD material lib [#2236](https://github.com/Autodesk/maya-usd/pull/2236)
- Fix textures disappearing when manipulating a prim when async texture loading is enabled [#2228](https://github.com/Autodesk/maya-usd/pull/2228)
- Flat shading using V3 lighting API [#2223](https://github.com/Autodesk/maya-usd/pull/2223)
- Expose colorCorrect node to MaterialX [#2207](https://github.com/Autodesk/maya-usd/pull/2207)
- Points schema support in HdVP2 render delegate [#2190](https://github.com/Autodesk/maya-usd/pull/2190)

## [v0.17.0] - 2022-04-04

**Build:**
- Created cmake variables which tell if the API is present [#2167](https://github.com/Autodesk/maya-usd/pull/2167)
- Change _GetLocalTransformForDagPoseMember check from maya api version to diffrence_type iterator trait support [#2136](https://github.com/Autodesk/maya-usd/pull/2136)
- Remove/replace all UFE_PREVIEW_VERSION_NUM now that latest MayaPR has UFE v3 [#2096](https://github.com/Autodesk/maya-usd/pull/2096)
- MayaUsd test failures with USD built with AssetResolver 2 [#2092](https://github.com/Autodesk/maya-usd/pull/2092)
- Initial cache to USD test [#2081](https://github.com/Autodesk/maya-usd/pull/2081)

**Translation Framework:**
- Create usdGeomNode to extract usd geometry on compute [#2173](https://github.com/Autodesk/maya-usd/pull/2173)
- Import code for place2dTexture [#2170](https://github.com/Autodesk/maya-usd/pull/2170)
- Support duplicate to Maya and USD to non root [#2166](https://github.com/Autodesk/maya-usd/pull/2166) [#2156](https://github.com/Autodesk/maya-usd/pull/2156)
- Export of texture and placement [#2165](https://github.com/Autodesk/maya-usd/pull/2165)
- Add curve filtering option in UI [#2161](https://github.com/Autodesk/maya-usd/pull/2161)
- Fixed incorrect export flag for translator in test plugin [#2148](https://github.com/Autodesk/maya-usd/pull/2148)
- Preserve load state of prims [#2141](https://github.com/Autodesk/maya-usd/pull/2141)
- Added mesh export options for interpolation type and reference object mode [#2128](https://github.com/Autodesk/maya-usd/pull/2128)
- Add UV tangents directly in MaterialX [#2113](https://github.com/Autodesk/maya-usd/pull/2113)
- Import of usd lux light prims [#2097](https://github.com/Autodesk/maya-usd/pull/2097)
- Fixed recursive calls to bound Python translators under Python 3 [#2094](https://github.com/Autodesk/maya-usd/pull/2094)
- Error when attempting to add a Maya reference to a variant [#2091](https://github.com/Autodesk/maya-usd/pull/2091)

**Workflow:**
- Maya Reference:
  - Added check in context ops to avoid Add Maya Reference on Maya reference prim [#2187](https://github.com/Autodesk/maya-usd/pull/2187)
  - Cancel edit on Maya Reference under an xform [#2186](https://github.com/Autodesk/maya-usd/pull/2186)
  - Crash when editing as Maya [#2183](https://github.com/Autodesk/maya-usd/pull/2183)
  - Locked attributes on Maya reference prim transform stand-in [#2175](https://github.com/Autodesk/maya-usd/pull/2175)
  - Clean up 'Cache to USD' UI options [#2144](https://github.com/Autodesk/maya-usd/pull/2144)
  - Change add Maya reference edit state default [#2131](https://github.com/Autodesk/maya-usd/pull/2131)
  - Proper selection for edit as Maya, merge to USD, and discard Maya edits [#2125](https://github.com/Autodesk/maya-usd/pull/2125)
  - No icon for Maya Reference in USD on Linux [#2090](https://github.com/Autodesk/maya-usd/pull/2090)
  - Add undo support to add Maya reference [#2079](https://github.com/Autodesk/maya-usd/pull/2079)
- Remove Load/Unload payload from AE [#2171](https://github.com/Autodesk/maya-usd/pull/2171)
- Renaming 2 prims with illegal chars will crash Maya [#2151](https://github.com/Autodesk/maya-usd/pull/2151)
- Exporting maya file using USD takes extremely long [#2158](https://github.com/Autodesk/maya-usd/pull/2158)
- Add glBindVertexArray to UsdMayaGLBatchRenderer::_TestIntersection [#2157](https://github.com/Autodesk/maya-usd/pull/2157)
- Automatically set the animation option [#2149](https://github.com/Autodesk/maya-usd/pull/2149)
- Fixed problem where BBox did not recalculate if prim visibility changed [#2147](https://github.com/Autodesk/maya-usd/pull/2147)
- Persist cache-to-USD options [#2123](https://github.com/Autodesk/maya-usd/pull/2123)
- Cache to USD: take out 'Parent Scope' from options [#2122](https://github.com/Autodesk/maya-usd/pull/2122)
- Improve the merge-to-USD UI [#2119](https://github.com/Autodesk/maya-usd/pull/2119)
- Added export option to disable the model kinds processor [#2106](https://github.com/Autodesk/maya-usd/pull/2106)
- Refresh issue with deactivation of USD Prims [#2104](https://github.com/Autodesk/maya-usd/pull/2104)
- Ufe v1: Crash when interacting with Outliner after unloading payload [#2103](https://github.com/Autodesk/maya-usd/pull/2103)
- Removed redundant code in proxy shape transform validation [#2100](https://github.com/Autodesk/maya-usd/pull/2100)
- Support for orphaned objects [#2095](https://github.com/Autodesk/maya-usd/pull/2095)
- Removed tracking selectability and lock metadata [#2075](https://github.com/Autodesk/maya-usd/pull/2075)
- Switch delete command behavior to delete prims [#1895](https://github.com/Autodesk/maya-usd/pull/1895)

**Render:**
- When instance colors is nullptr don't get the length [#2230](https://github.com/Autodesk/maya-usd/pull/2230)
- MaterialX image improvements [#2178](https://github.com/Autodesk/maya-usd/pull/2178)
- Share cached HdVP2TextureInfo between all materials [#2145](https://github.com/Autodesk/maya-usd/pull/2145)
- If USD reports that the topology is dirty compare the new topology to the old [#2143](https://github.com/Autodesk/maya-usd/pull/2143)
- Warn the user that mtoh is experimental [#2138](https://github.com/Autodesk/maya-usd/pull/2138)
- Get the display style of all viewports rather than just the current [#2134](https://github.com/Autodesk/maya-usd/pull/2134)
- Use the class specific namespace so we don't have to check Maya's version [#2129](https://github.com/Autodesk/maya-usd/pull/2129)
- USD 22.03 integration: API change for MaterialX [#2126](https://github.com/Autodesk/maya-usd/pull/2126)
- Setting the instance colors or transforms into Maya is expensive [#2110](https://github.com/Autodesk/maya-usd/pull/2110)
- Varying display colour bug [#2107](https://github.com/Autodesk/maya-usd/pull/2107)
- Use the latest MPxSubSceneOverride API [#2101](https://github.com/Autodesk/maya-usd/pull/2101)
- Improved texture loading by running file reading asynchronously and only for active render purpose [#1980](https://github.com/Autodesk/maya-usd/pull/1980)

**Documentation:**
- Updated documentation + unit test [#2105](https://github.com/Autodesk/maya-usd/pull/2105)

## [v0.16.0] - 2022-02-08

**Build:**
- Fix edit router parent test [#2068](https://github.com/Autodesk/maya-usd/pull/2068)
- Add custom rig codeless schema test [#2057](https://github.com/Autodesk/maya-usd/pull/2057) [#1997](https://github.com/Autodesk/maya-usd/pull/1997)
- Rename "gulark" to "gulrak" [#2048](https://github.com/Autodesk/maya-usd/pull/2048)
- Removed AL_usdmaya_ProxyShapeSelect command and related functionality [#2009](https://github.com/Autodesk/maya-usd/pull/2009)
- Allow building Maya 2020 against a MaterialX-enabled USD [#1985](https://github.com/Autodesk/maya-usd/pull/1985)
- Fix TestALUSDMayaPython_LayerManager random fails [#1945](https://github.com/Autodesk/maya-usd/pull/1945)
- Updated AL Python tests to avoid tempfiles [#1923](https://github.com/Autodesk/maya-usd/pull/1923)

**Translation Framework:**
- Write sparsely when exporting mesh orientation [#2050](https://github.com/Autodesk/maya-usd/pull/2050)
- Allow updating registered Python class [#2044](https://github.com/Autodesk/maya-usd/pull/2044)
- Improve Python bindings [#2037](https://github.com/Autodesk/maya-usd/pull/2037)
- Disable multi-material options for legacy shading [#2035](https://github.com/Autodesk/maya-usd/pull/2035)
- Extended Python schema API wrappers [#2027](https://github.com/Autodesk/maya-usd/pull/2027)
- Added vertex colour utility node and translation for USD preview surface [#2016](https://github.com/Autodesk/maya-usd/pull/2016)
- Revisit Python shader export unit test [#1981](https://github.com/Autodesk/maya-usd/pull/1981)
- Fix MaterialX import of RGBA files [#1962](https://github.com/Autodesk/maya-usd/pull/1962)
- Make sure material export sanitizes names [#1950](https://github.com/Autodesk/maya-usd/pull/1950)
- Use correct default for shading mode [#1940](https://github.com/Autodesk/maya-usd/pull/1940)
- Properly detect animated compound plug [#1939](https://github.com/Autodesk/maya-usd/pull/1939)
- Support for ignoreColorSpaceFileRules when setting scalar textures [#1938](https://github.com/Autodesk/maya-usd/pull/1938)
- Fix correctness issues for material export [#1937](https://github.com/Autodesk/maya-usd/pull/1937)
- Fixed worldspace export flag behaviour to only bake xform at the top level prim [#1931](https://github.com/Autodesk/maya-usd/pull/1931)
- Validate normal maps for correctness [#1918](https://github.com/Autodesk/maya-usd/pull/1918)
- Unnest UV reader and optimize [#1915](https://github.com/Autodesk/maya-usd/pull/1915)
- Use USD syntax for Python callbacks [#1911](https://github.com/Autodesk/maya-usd/pull/1911)
- Only export transform rotate order if not default [#1713](https://github.com/Autodesk/maya-usd/pull/1713)

**Workflow:**
- Push/Pull workflow:
  - Cache to USD dialog for Maya reference prim [#2054](https://github.com/Autodesk/maya-usd/pull/2054)
  - See a waiting cursor when pulling [#2040](https://github.com/Autodesk/maya-usd/pull/2040)
  - Deactivate prim on pull instead of using exclude feature [#2036](https://github.com/Autodesk/maya-usd/pull/2036)
  - Group my Maya Reference on creation [#2025](https://github.com/Autodesk/maya-usd/pull/2025)
  - Improve edit as maya context menu [#2023](https://github.com/Autodesk/maya-usd/pull/2023)
  - Edit as Maya whole selected object [#2020](https://github.com/Autodesk/maya-usd/pull/2020)
  - MayaReference auto-edit support [#2015](https://github.com/Autodesk/maya-usd/pull/2015)
  - Always use Namespaces when creating a Maya Reference [#2004](https://github.com/Autodesk/maya-usd/pull/2004)
  - Name the MayaReference prim/transform node on creation [#2000](https://github.com/Autodesk/maya-usd/pull/2000)
  - Ensure duplicate-as USD creates unique prim names [#1989](https://github.com/Autodesk/maya-usd/pull/1989)
  - Define my Maya Reference in a variant on creation [#1988](https://github.com/Autodesk/maya-usd/pull/1988)
  - Support mergeNamespacesOnClash attribute on MayaReference prim [#1987](https://github.com/Autodesk/maya-usd/pull/1987)
  - Enable creating Maya Reference prim as child of proxy shape node [#1978](https://github.com/Autodesk/maya-usd/pull/1978)
  - Add a Maya Reference [#1973](https://github.com/Autodesk/maya-usd/pull/1973)
  - Add undo/redo to Edit-as-Maya workflows [#1969](https://github.com/Autodesk/maya-usd/pull/1969)
  - mergeToUsd/discardEdits should remove inert primSpecs from session layer[#1927](https://github.com/Autodesk/maya-usd/pull/1927)
  - 'Duplicate as Maya Data' only on prims that have import translators [#1924](https://github.com/Autodesk/maya-usd/pull/1924)
  - Get an edit on my sessionLayer instead of my editTarget layer [#1896](https://github.com/Autodesk/maya-usd/pull/1896)
- Fix group command not finding unique names [#2032](https://github.com/Autodesk/maya-usd/pull/2032)
- Outliner not updating with new prim defined in python [#1970](https://github.com/Autodesk/maya-usd/pull/1970)
- Expose python bindings for UsdMayaPrimUpdater [#1894](https://github.com/Autodesk/maya-usd/pull/1894)
- Add support for float and double types to the converter (and proxy accessor) [#1892](https://github.com/Autodesk/maya-usd/pull/1892)

**Render:**
- Removed proxyDrawOverride render delegate [#2063](https://github.com/Autodesk/maya-usd/pull/2063)
- MaterialX v1.38.3 [#2062](https://github.com/Autodesk/maya-usd/pull/2062)
- Introducing color cache to improve performance [#2059](https://github.com/Autodesk/maya-usd/pull/2059)
- Fix display color not matching the source Lambert materials [#2049](https://github.com/Autodesk/maya-usd/pull/2049)
- Trigger the populate selection when a new instancer is added [#2038](https://github.com/Autodesk/maya-usd/pull/2038)
- Disable the workaround of populating selection every frame [#2034](https://github.com/Autodesk/maya-usd/pull/2034)
  - Work around Pixar USD issue #1516 [#1897](https://github.com/Autodesk/maya-usd/pull/1897)
- USD selection highlighting doesn't take color settings into consideration [#2033](https://github.com/Autodesk/maya-usd/pull/2033)
- Don't dirty invalid paths [#2024](https://github.com/Autodesk/maya-usd/pull/2024)
- Interacting with heavy scenes is slow [#2019](https://github.com/Autodesk/maya-usd/pull/2019)
- Fix flipped normals [#2012](https://github.com/Autodesk/maya-usd/pull/2012)
- Work around USD race condition for UsdSkel [#2007](https://github.com/Autodesk/maya-usd/pull/2007)
- Don't try to get requiredVertextBuffers from a NULL shader [#1993](https://github.com/Autodesk/maya-usd/pull/1993)
- Curves drawing color is the exact same as the Maya viewport gray background [#1998](https://github.com/Autodesk/maya-usd/pull/1998)
- CPU UsdSkel support [#1992](https://github.com/Autodesk/maya-usd/pull/1992)
- Changing the primvar reader to a color reader can change the output [#1971](https://github.com/Autodesk/maya-usd/pull/1971)
- Test selection highlighting of USD objects in vp2RenderDelegate [#1965](https://github.com/Autodesk/maya-usd/pull/1965)
- UpdateSelectionStates may trigger an execute [#1957](https://github.com/Autodesk/maya-usd/pull/1957)
- Use cached textures [#1941](https://github.com/Autodesk/maya-usd/pull/1941)
- Fix exported normals not being used by VP2 [#1913](https://github.com/Autodesk/maya-usd/pull/1913)
- Safely test opacity values when iterpolation is uniform [#1906](https://github.com/Autodesk/maya-usd/pull/1906)
- Add required primvars corresponding to the Maya shader's required vertex buffers [#1903](https://github.com/Autodesk/maya-usd/pull/1903)
- Copy MaterialXGenOgsXml to MayaUSD [#1901](https://github.com/Autodesk/maya-usd/pull/1901)
- Mark everything dirty when display modes change [#1891](https://github.com/Autodesk/maya-usd/pull/1891)
- Make Enum UI control for TfToken type [#1874](https://github.com/Autodesk/maya-usd/pull/1874)

**Documentation:**
- Improve documentation discoverability [#2064](https://github.com/Autodesk/maya-usd/pull/2064)
- Schema adaptor documentation [#2061](https://github.com/Autodesk/maya-usd/pull/2061)

## [v0.15.0] - 2021-12-16

**Build:**
- Updated AL Python tests to use fixturesUtils [#1886](https://github.com/Autodesk/maya-usd/pull/1886)
- Added multi-format serialisation test for shared LayerManager [#1866](https://github.com/Autodesk/maya-usd/pull/1866)
- Use AbsPath for INPUT_PATH env var in converter tests [#1837](https://github.com/Autodesk/maya-usd/pull/1837)
- Compile MayaUSD correctly when mixing USD 20.05 and Maya 2022 [#1835](https://github.com/Autodesk/maya-usd/pull/1835)
- Test passing array to Maya commands [#1831](https://github.com/Autodesk/maya-usd/pull/1831)
- Use mayapy as Python interpreter on Mac [#1828](https://github.com/Autodesk/maya-usd/pull/1828)
- Update callsites to work with USD dev API changes [#1825](https://github.com/Autodesk/maya-usd/pull/1825)
- Fix UV export test [#1823](https://github.com/Autodesk/maya-usd/pull/1823)
- Add tests for auto load new objects, auto load selected and for instanced [#1820](https://github.com/Autodesk/maya-usd/pull/1820)

**Translation Framework:**
- Convert UsdMayaPrimUpdater::isAnimated() use of Python to C++ [#1878](https://github.com/Autodesk/maya-usd/pull/1878)
- Python bindings for chasers [#1864](https://github.com/Autodesk/maya-usd/pull/1864)
- Maya 2020 picked wrong MDGModifier.deleteNode [#1853](https://github.com/Autodesk/maya-usd/pull/1853)
- Job context UI for import/export dialogs [#1842](https://github.com/Autodesk/maya-usd/pull/1842)
- Fix support for normal map channel connections [#1833](https://github.com/Autodesk/maya-usd/pull/1833)
- Use Adaptors for custom schema import-export [#1814](https://github.com/Autodesk/maya-usd/pull/1814)
- Bugfix for exporting instances that have namespaces or other use invalid characters [#1708](https://github.com/Autodesk/maya-usd/pull/1708)

**Workflow:**
- Push/Pull workflow:
  - Remove edit-as-maya menus when not available [#1885](https://github.com/Autodesk/maya-usd/pull/1885)
  - Validate pull push capability [#1845](https://github.com/Autodesk/maya-usd/pull/1845)
  - Implement minimal merge of modified data when merging back to USD after editing in Maya [#1804](https://github.com/Autodesk/maya-usd/pull/1804)
- Adjust unit tests for UFE 3.0 [#1888](https://github.com/Autodesk/maya-usd/pull/1888)
- Unloaded prims should appear in the outliner [#1877](https://github.com/Autodesk/maya-usd/pull/1877)
- Add action in Outliner context menu to toggle instanceable state of prim [#1873](https://github.com/Autodesk/maya-usd/pull/1873)
- Made all dirty layers and anonymous sublayers serialisable [#1872](https://github.com/Autodesk/maya-usd/pull/1872)
- Selecting an instanced prim and then the stage shows incorrect selection hilite in viewport [#1868](https://github.com/Autodesk/maya-usd/pull/1868)
- Fixed exiting callback failing to deregister on UFE global [#1856](https://github.com/Autodesk/maya-usd/pull/1856)
- Added locking for instance proxy xforms [#1851](https://github.com/Autodesk/maya-usd/pull/1851)
- Prevent parenting under an instance [#1844](https://github.com/Autodesk/maya-usd/pull/1844)
- Manage the edit-ability of an attribute from Maya's Attribute Editor [#1816](https://github.com/Autodesk/maya-usd/pull/1816)
- Add the layer's file format as a serialisation attribute [#1792](https://github.com/Autodesk/maya-usd/pull/1792)

**Render:**
- Update for quaternion change in USD v21.11 [#1910](https://github.com/Autodesk/maya-usd/pull/1910) [#1902](https://github.com/Autodesk/maya-usd/pull/1902)
- Skip Ufe Identifier update when the identifiers are not in use [#1875](https://github.com/Autodesk/maya-usd/pull/1875)
- Fix for proxyShapes in VP2 to display shaders bound to "glslfx:surface" material output [#1870](https://github.com/Autodesk/maya-usd/pull/1870)
- Insert backup restore GL state render task around colorize-select-task [#1863](https://github.com/Autodesk/maya-usd/pull/1863)
- Performance optimization if there are no reprs selected [#1843](https://github.com/Autodesk/maya-usd/pull/1843)
- Selection Highlight doesn't update when changing selection with USD 21.11 [#1838](https://github.com/Autodesk/maya-usd/pull/1838)
- Fix OpenGL initialization to restore GPU Compute functionality [#1803](https://github.com/Autodesk/maya-usd/pull/1803)

## [0.14.0] - 2021-11-10

**Build:**
- Fixes to unit testing framework [#1798](https://github.com/Autodesk/maya-usd/pull/1798) [#1795](https://github.com/Autodesk/maya-usd/pull/1795) [#1793](https://github.com/Autodesk/maya-usd/pull/1793) [#1787](https://github.com/Autodesk/maya-usd/pull/1787) [#1772](https://github.com/Autodesk/maya-usd/pull/1772)
- Make shader reader/writer test self contained [#1794](https://github.com/Autodesk/maya-usd/pull/1794)
- Update image baseline for PreviewSurface test [#1786](https://github.com/Autodesk/maya-usd/pull/1786)
- Use new libusd library prefix when finding USD_LIBRARY [#1743](https://github.com/Autodesk/maya-usd/pull/1743)

**Translation Framework:**
- Python Bindings:
  - UsdMayaShadingModeRegistry [#1756](https://github.com/Autodesk/maya-usd/pull/1756)
  - Material translators [#1744](https://github.com/Autodesk/maya-usd/pull/1744)
- Multi-material export [#1788](https://github.com/Autodesk/maya-usd/pull/1788)
- USD export and import componentTags [#1755](https://github.com/Autodesk/maya-usd/pull/1755)
- Import basis curves [#1742](https://github.com/Autodesk/maya-usd/pull/1742)
- Fix translator context on nested proxy shapes [#1729](https://github.com/Autodesk/maya-usd/pull/1729)

**Workflow:**
- Hierarchy handler chain of responsibility for child filtering [#1810](https://github.com/Autodesk/maya-usd/pull/1810)
- Fixed tagged excluded prims to update correctly when changed [#1791](https://github.com/Autodesk/maya-usd/pull/1791)
- Crash when attempting to move Maya object and USD prim [#1790](https://github.com/Autodesk/maya-usd/pull/1790)
- Edit as Maya data / Merge back to USD [#1789](https://github.com/Autodesk/maya-usd/pull/1789)
- Implemented new metadata methods from Ufe::Attribute to support locking [#1785](https://github.com/Autodesk/maya-usd/pull/1785)
- Use HdRprim's render tag data for VP2 mesh shared data [#1784](https://github.com/Autodesk/maya-usd/pull/1784)
- Payload status is not respected when duplicating an object in USD [#1778](https://github.com/Autodesk/maya-usd/pull/1778) [#1735](https://github.com/Autodesk/maya-usd/pull/1735)
- Added intersection test command for ProxyDrawOverride [#1771](https://github.com/Autodesk/maya-usd/pull/1771)
- Layer Editor/Manager:
  - Updated manager to purge cached self on scene reset [#1760](https://github.com/Autodesk/maya-usd/pull/1760)
  - Loading sublayers with relative path checked throws error [#1748](https://github.com/Autodesk/maya-usd/pull/1748)
  - Added caching to LayerManager [#1734](https://github.com/Autodesk/maya-usd/pull/1734)
- Make TranslatePrim update behaviour consistent with Proxy Shape update behaviour [#1725](https://github.com/Autodesk/maya-usd/pull/1725)

**Render:**
- Fix textures missing on OSX with V2 Lighting [#1800](https://github.com/Autodesk/maya-usd/pull/1800)
- Maya freezes on playback when using a invalid stageCacheId [#1776](https://github.com/Autodesk/maya-usd/pull/1776)
- Updates to pxr hd renderer [#1774](https://github.com/Autodesk/maya-usd/pull/1774)
- Enable primvar filtering to match HdSt behavior [#1737](https://github.com/Autodesk/maya-usd/pull/1737)
- Added USD pick mode fixes for VP2 [#1731](https://github.com/Autodesk/maya-usd/pull/1731)

## [0.13.0] - 2021-10-12

**Build:**
- Fix compilation issue for schema, header and UFE linking [#1723](https://github.com/Autodesk/maya-usd/pull/1723)
- Use PXR_NAMESPACE_USING_DIRECTIVE to fix internal Pixar builds [#1717](https://github.com/Autodesk/maya-usd/pull/1717)
- Centos8 supprt [#1716](https://github.com/Autodesk/maya-usd/pull/1716)
- Python:
  - Prevent crash when accessing a stale attribute from Python [#1675](https://github.com/Autodesk/maya-usd/pull/1675)
  - Python 3.9 support [#1668](https://github.com/Autodesk/maya-usd/pull/1668)
  - Fix python 3.9 AL TransactionManager crash [#1698](https://github.com/Autodesk/maya-usd/pull/1698)
- Fix MaterialX code generation [#1696](https://github.com/Autodesk/maya-usd/pull/1696)
- Pixar plugin:
  - Removing references to the deprecated PxPointInstancer [#1694](https://github.com/Autodesk/maya-usd/pull/1694)
- Add tests for sets command and for isolate select in Vp2RenderDelegate [#1678](https://github.com/Autodesk/maya-usd/pull/1678)
- Maya could have some observers created when the test starts running [#1650](https://github.com/Autodesk/maya-usd/pull/1650)
- Update maya-usd for USD v0.21.08 [#1646](https://github.com/Autodesk/maya-usd/pull/1646)

**Translation Framework:**
- Python Bindings:
  - Chasers [#1722](https://github.com/Autodesk/maya-usd/pull/1722)
  - Un-registration of registered translators [#1709](https://github.com/Autodesk/maya-usd/pull/1709)
  - Schema adaptors [#1699](https://github.com/Autodesk/maya-usd/pull/1699)
  - Prim translators [#1670](https://github.com/Autodesk/maya-usd/pull/1670)
- Import/Export fixes:
  - Warning on blendShape export [#1684](https://github.com/Autodesk/maya-usd/pull/1684)
  - Standard surface export [#1671](https://github.com/Autodesk/maya-usd/pull/1671)
  - GeomNurbsCurve import [#1654](https://github.com/Autodesk/maya-usd/pull/1654)
- Animal Logic plugin:
  - Fill in missing export translator implementation [#1728](https://github.com/Autodesk/maya-usd/pull/1728)
  - Avoid opening duplicated usd stages on Maya file load [#1727](https://github.com/Autodesk/maya-usd/pull/1727)
  - Fix excluded prim paths [#1726](https://github.com/Autodesk/maya-usd/pull/1726)
  - Add duplicate check during translator context deserialise based on prim path match [#1710](https://github.com/Autodesk/maya-usd/pull/1710)
  - Ignored setting playback range if no time code found from USD [#1686](https://github.com/Autodesk/maya-usd/pull/1686)
- Use the API schema class UsdLuxLightAPI in place of USD typed class [#1701](https://github.com/Autodesk/maya-usd/pull/1701)
- Adopt use of references when passing ReaderContext when reading RfM [#1653](https://github.com/Autodesk/maya-usd/pull/1653)
- Update RfMLight translators to use codeless schemas [#1648](https://github.com/Autodesk/maya-usd/pull/1648)
- Allow usd reader registration for tf type tokens [#1619](https://github.com/Autodesk/maya-usd/pull/1619)

**Workflow:**
- Basic Channel Box support for USD prims [#1715](https://github.com/Autodesk/maya-usd/pull/1715)
- Manage editability of properties [#1700](https://github.com/Autodesk/maya-usd/pull/1700)
- Ability to select by point instancers, instances and prototypes [#1677](https://github.com/Autodesk/maya-usd/pull/1677)
- Ability to manage a prim's viewport selectability [#1673](https://github.com/Autodesk/maya-usd/pull/1673)
- Prevent saving layers with identical names [#1644](https://github.com/Autodesk/maya-usd/pull/1644)
- Improperly created scene item must not crash move tool [#1636](https://github.com/Autodesk/maya-usd/pull/1636)

**Render:**
- Prevent crash when nonexistant instance is selected [#1718](https://github.com/Autodesk/maya-usd/pull/1718)
- Animal Logic plugin:
  - Bail on proxyDrawOverride user select early if snapping is active [#1711](https://github.com/Autodesk/maya-usd/pull/1711)
- Properly use fallback color in vp2RenderDelegate [#1692](https://github.com/Autodesk/maya-usd/pull/1692)
- Fixing lighting issues with UsdPreviewSurface [#1682](https://github.com/Autodesk/maya-usd/pull/1682)
- Update primvars with instance interpolation when instancing is dirty [#1669](https://github.com/Autodesk/maya-usd/pull/1669)
- Set the Ufe Identifier on MRenderItems for isolate select support [#1662](https://github.com/Autodesk/maya-usd/pull/1662)
- Make sure that any prims whose purpose changed get updated [#1661](https://github.com/Autodesk/maya-usd/pull/1661)
- LightAPI v2 detection code was updated to detect fix in Maya [#1645](https://github.com/Autodesk/maya-usd/pull/1645)
- useSpecularWorkflow now works [#1638](https://github.com/Autodesk/maya-usd/pull/1638)
- Fixes incorrect Schlick Fresnel for IBL lighting [#1634](https://github.com/Autodesk/maya-usd/pull/1634)
- Minor performance improvement for loading scenes into Vp2RenderDelegate [#1633](https://github.com/Autodesk/maya-usd/pull/1633)
- Support for Hydra's new per-draw-item material tag [#1629](https://github.com/Autodesk/maya-usd/pull/1629)

## [0.12.0] - 2021-08-23

**Build:**
- Implement HdMayaSceneDelegate::GetInstancerPrototypes [#1604](https://github.com/Autodesk/maya-usd/pull/1604)
- Fix unit tests to run in Debug builds [#1593](https://github.com/Autodesk/maya-usd/pull/1593)

**Translation Framework:**
- Export opacity of standardSurface to usdPreviewSurface and back [#1611](https://github.com/Autodesk/maya-usd/pull/1611)
- Avoid TF_Verify message when USD attribute is empty [#1599](https://github.com/Autodesk/maya-usd/pull/1599)
- Export roots [#1592](https://github.com/Autodesk/maya-usd/pull/1592)
- Fix warnings from USDZ texture import [#1589](https://github.com/Autodesk/maya-usd/pull/1589)
- Add support for light translators [#1577](https://github.com/Autodesk/maya-usd/pull/1577)
- ProxyShape: add pauseUpdate attr to pause evaluation [#1511](https://github.com/Autodesk/maya-usd/pull/1511)

**Workflow:**
- No USD prompt during crash [#1601](https://github.com/Autodesk/maya-usd/pull/1601)
- Add grouping with absolute, relative, world flags [#1600](https://github.com/Autodesk/maya-usd/pull/1600)
- Fix crash on scene item creation [#1596](https://github.com/Autodesk/maya-usd/pull/1596)
- Matrix op, common API undo/redo [#1586](https://github.com/Autodesk/maya-usd/pull/1586)
- Make USD menu tear-off-able [#1575](https://github.com/Autodesk/maya-usd/pull/1575)

**Render:**
- Revert to using shading with the V1 lighting API  [#1626](https://github.com/Autodesk/maya-usd/pull/1626)
- Implement UsdTexture2d support in VP2 render delegate [#1617](https://github.com/Autodesk/maya-usd/pull/1617)
- Maya 2022.1 has the Light loop API V2 and is compatible with MaterialX [#1609](https://github.com/Autodesk/maya-usd/pull/1609)
- Fix rprim state assertions [#1607](https://github.com/Autodesk/maya-usd/pull/1607)
- Remove hardcode camera name string [#1598](https://github.com/Autodesk/maya-usd/pull/1598)
- Store the correct render item name in HdVP2DrawItem [#1597](https://github.com/Autodesk/maya-usd/pull/1597)

**Documentation:**
- Fix minor typos in "lock" and "selectability" tutorials [#1574](https://github.com/Autodesk/maya-usd/pull/1574)

## [0.11.0] - 2021-07-27

**Build:**
- Remove deprecated UsdRi schemas [#1585](https://github.com/Autodesk/maya-usd/pull/1585)
- Bump USD min version to 20.05 [#1568](https://github.com/Autodesk/maya-usd/pull/1568)
- Fixed two tests with the same name [#1562](https://github.com/Autodesk/maya-usd/pull/1562)
- Pxr tests will use build area for temporary files [#1443](https://github.com/Autodesk/maya-usd/pull/1443)

**Translation Framework:**
- Review all standard surface default values [#1560](https://github.com/Autodesk/maya-usd/pull/1560)
- Remove calls to configure resolver for asset under ar2 [#1530](https://github.com/Autodesk/maya-usd/pull/1530)
- UV set renamed to st, st1, st2 [#1528](https://github.com/Autodesk/maya-usd/pull/1528)
- Add geomSidedness flag to exporter [#1499](https://github.com/Autodesk/maya-usd/pull/1499)
- Fixed bug with visibility determination [#1479](https://github.com/Autodesk/maya-usd/pull/1479)

**Workflow:**
- Crash when transforming multiple objects with duplicate stage [#1554](https://github.com/Autodesk/maya-usd/pull/1554)
- Add restriction for grouping [#1550](https://github.com/Autodesk/maya-usd/pull/1550)
- Stage to be created without the proxyShape being capitalized [#1526](https://github.com/Autodesk/maya-usd/pull/1526)
- Handle multiple deferred notifs for single path [#1519](https://github.com/Autodesk/maya-usd/pull/1519)
- Use SdfComputeAssetPathRelativeToLayer to compute sublayer paths [#1515](https://github.com/Autodesk/maya-usd/pull/1515)
- Deleting assembly nodes that contained child assemblies crashes Maya [#1504](https://github.com/Autodesk/maya-usd/pull/1504)
- Add support for ungroup command [#1469](https://github.com/Autodesk/maya-usd/pull/1469)
- New unshareable stage workflow [#1455](https://github.com/Autodesk/maya-usd/pull/1455)
  - Disable save for unshared layer root [#1584](https://github.com/Autodesk/maya-usd/pull/1584)
  - Disable "Revert File" for read-only files [#1579](https://github.com/Autodesk/maya-usd/pull/1579)
- On drag and drop of a layer with an editTarget, the edit target moves to the root layer [#1444](https://github.com/Autodesk/maya-usd/pull/1444)

**Render:**
- MaterialX
  - Fix MaterialX import errors found by testing [#1553](https://github.com/Autodesk/maya-usd/pull/1553)
  - Handle UDIM in MaterialX shaders [#1582](https://github.com/Autodesk/maya-usd/pull/1582)
  - MaterialX UV0 defaultgeomprop to st primvar [#1533](https://github.com/Autodesk/maya-usd/pull/1533)
  - MaterialX build instructions [#1524](https://github.com/Autodesk/maya-usd/pull/1524)
  - MaterialX import export [#1478](https://github.com/Autodesk/maya-usd/pull/1478)
  - Show USD MaterialX in VP2 render delegate [#1433](https://github.com/Autodesk/maya-usd/pull/1433)
- Do not crash on invalid materials [#1573](https://github.com/Autodesk/maya-usd/pull/1573)
- Show patch curves as wireframe in default material mode [#1570](https://github.com/Autodesk/maya-usd/pull/1570)
- Don't normalize point instancer quaternions [#1563](https://github.com/Autodesk/maya-usd/pull/1563)
- Pixar fixed render tag issue in v20.08 so only enable env for earlier versions [#1561](https://github.com/Autodesk/maya-usd/pull/1561)
- Detect and save a pointer to the cardsUv primvar reader [#1558](https://github.com/Autodesk/maya-usd/pull/1558)
- Restore preview purpose material binding [#1555](https://github.com/Autodesk/maya-usd/pull/1555)
- Make sure we leave default material mode when the test ends [#1552](https://github.com/Autodesk/maya-usd/pull/1552)
- Build a proper index buffer for the default material render item [#1522](https://github.com/Autodesk/maya-usd/pull/1522)
- Point snapping to similar instances [#1521](https://github.com/Autodesk/maya-usd/pull/1521)
- UsdUVTexture works with Maya color management prefs [#1516](https://github.com/Autodesk/maya-usd/pull/1516)
- Handle source color space in Vp2 render delegate [#1508](https://github.com/Autodesk/maya-usd/pull/1508)
- Add kSelectPointsForGravity to all selected render items when we should be snapping to active objects [#1502](https://github.com/Autodesk/maya-usd/pull/1502)
- Add support to Vp2RenderDelegate for the "cards" drawing mode [#1463](https://github.com/Autodesk/maya-usd/pull/1463)

**Documentation:**
- Instruction for installing devtoolset-6 on CentOS [#1482](https://github.com/Autodesk/maya-usd/pull/1482)

## [0.10.0] - 2021-06-11

**Build:**
- Use new schemaregistry API [#1435](https://github.com/Autodesk/maya-usd/pull/1435)
- Update Pixar ProxyShape image tests affected from storm wireframe [#1414](https://github.com/Autodesk/maya-usd/pull/1414)
- Use new usdShade/material vector based API for USD versions beyond 21.05 [#1394](https://github.com/Autodesk/maya-usd/pull/1394)
- Support building Gulrak library locally [#1291](https://github.com/Autodesk/maya-usd/pull/1291)
- Center pivot command support test [#1207](https://github.com/Autodesk/maya-usd/pull/1207)

**Translation Framework:**
- Use Maya matrix decomposition for matrix op TRS accessors [#1428](https://github.com/Autodesk/maya-usd/pull/1428)
- Move Transform3d handler creation into core [#1409](https://github.com/Autodesk/maya-usd/pull/1409)
- Write correct IOR for metallic shaders [#1388](https://github.com/Autodesk/maya-usd/pull/1388)
- Remove namespace on material import [#1344](https://github.com/Autodesk/maya-usd/pull/1344)
- Configure resolver to generate resolver context [#1342](https://github.com/Autodesk/maya-usd/pull/1342)
- Referenced material non-default attributes not getting exported to USD [#1284](https://github.com/Autodesk/maya-usd/pull/1284)
- Don't write fStop when depthOfField is disabled [#1280](https://github.com/Autodesk/maya-usd/pull/1280)
- Spurious warning being raised about USDZ texture import [#1279](https://github.com/Autodesk/maya-usd/pull/1279)
- Guard against past-end iterator invalidation [#1247](https://github.com/Autodesk/maya-usd/pull/1247)

**Workflow:**
- Grouping a prim twice will crash Maya [#1453](https://github.com/Autodesk/maya-usd/pull/1453)
- AE missing information on materials and texture [#1446](https://github.com/Autodesk/maya-usd/pull/1446)
- Implement setMatrixCmd() for common transform API [#1445](https://github.com/Autodesk/maya-usd/pull/1445)
- Error messages showing up when opening the "Applied Schema" category [#1434](https://github.com/Autodesk/maya-usd/pull/1434)
- Replace callsites into deprecated "Master" API [#1430](https://github.com/Autodesk/maya-usd/pull/1430)
- Empty categories appear [#1427](https://github.com/Autodesk/maya-usd/pull/1427)
- See mayaUsdProxyShape icon in Node Editor [#1425](https://github.com/Autodesk/maya-usd/pull/1425)
- Mute layer doesn't work [#1423](https://github.com/Autodesk/maya-usd/pull/1423)
- USD files fail to load on mapped mounted volume [#1422](https://github.com/Autodesk/maya-usd/pull/1422)
- Send ufe notifications on undo and redo [#1412](https://github.com/Autodesk/maya-usd/pull/1412)
- Remove use of deprecated pxr namespace in favor of PXR_NS [#1411](https://github.com/Autodesk/maya-usd/pull/1411)
- On prim AE template, see connected attributes [#1410](https://github.com/Autodesk/maya-usd/pull/1410)
- Layer Editor save icon missing when Maya scaling is 125% [#1408](https://github.com/Autodesk/maya-usd/pull/1408)
- Title for Shaping API shows up incorrect in Attribute Editor [#1407](https://github.com/Autodesk/maya-usd/pull/1407)
- Crash on redo of creation and manipulation of prim [#1406](https://github.com/Autodesk/maya-usd/pull/1406)
- On the prim AE template see useful widgets for arrays [#1393](https://github.com/Autodesk/maya-usd/pull/1393) [#1374](https://github.com/Autodesk/maya-usd/pull/1374) [#1367](https://github.com/Autodesk/maya-usd/pull/1367)
- Importing USD file with USDZ Texture import option on causes import to fail [#1392](https://github.com/Autodesk/maya-usd/pull/1392)
- Check for empty paths [#1387](https://github.com/Autodesk/maya-usd/pull/1387)
- Attribute Editor: array attribute does not support nice/long name [#1380](https://github.com/Autodesk/maya-usd/pull/1380)
- Prim AE template: categories persist their expand/collapse state [#1372](https://github.com/Autodesk/maya-usd/pull/1372)
- Crash when editing layer that has permission to edit false (locked) [#1363](https://github.com/Autodesk/maya-usd/pull/1363)
- Clean up runtimeCommands created by the plugin [#1354](https://github.com/Autodesk/maya-usd/pull/1354)
- Point instance interactive batched position, orientation and scale changes [#1352](https://github.com/Autodesk/maya-usd/pull/1352)
- List all registered prim types for creation [#1350](https://github.com/Autodesk/maya-usd/pull/1350)
- On the prim AE template see applied schemas [#1333](https://github.com/Autodesk/maya-usd/pull/1333)
- USD load/unload payloads does not update viewport [#1332](https://github.com/Autodesk/maya-usd/pull/1332)
- Increase depth priority for point snapping items [#1329](https://github.com/Autodesk/maya-usd/pull/1329)
- Point snapping issue on USD instances [#1321](https://github.com/Autodesk/maya-usd/pull/1321)
- Send Ufe::CameraChanged when camera attributes are modified [#1315](https://github.com/Autodesk/maya-usd/pull/1315)
- Block attribute edits for the remaining transformation stacks [#1308](https://github.com/Autodesk/maya-usd/pull/1308)
- Test usd camera animated params and editing params [#1298](https://github.com/Autodesk/maya-usd/pull/1298)
- Block attribute authoring in weaker layers in legacy transform3d [#1294](https://github.com/Autodesk/maya-usd/pull/1294)
- No tooltip for prim path in Attribute Editor [#1288](https://github.com/Autodesk/maya-usd/pull/1288)
- Crash when token for "Info: id" is editied to invalide value [#1287](https://github.com/Autodesk/maya-usd/pull/1287)
- Ability to turn off file-backed Save confirmation in Layer Editor [#1277](https://github.com/Autodesk/maya-usd/pull/1277)

**Render:**
- Invisible prims don't get notifications on _PropagateDirtyBits or Sync [#1432](https://github.com/Autodesk/maya-usd/pull/1432)
- Integrate 21.05 UsdPreviewSurface shader into MayaUsd plugin [#1416](https://github.com/Autodesk/maya-usd/pull/1416)
- Store render item dirty bits with each render item [#1404](https://github.com/Autodesk/maya-usd/pull/1404)
- When the instance count changes we need to update everything related to instancer [#1398](https://github.com/Autodesk/maya-usd/pull/1398)
- Add a VP2RenderDelegate test for basis curves to ensure they draw correctly [#1390](https://github.com/Autodesk/maya-usd/pull/1390)
- Disable the material consolidation update workaround [#1359](https://github.com/Autodesk/maya-usd/pull/1359)
- Metallic IBL Reflection [#1356](https://github.com/Autodesk/maya-usd/pull/1356)
- Support default material shading using new SDK APIs [#1339](https://github.com/Autodesk/maya-usd/pull/1339)
- Faster point snapping [#1292](https://github.com/Autodesk/maya-usd/pull/1292)
- Add camera adapter to support physical camera parameters and configurable motion blur and depth-of-field [#1282](https://github.com/Autodesk/maya-usd/pull/1282)
- Hydra prim invalidation for hidden lights [#1281](https://github.com/Autodesk/maya-usd/pull/1281)

**Documentation:**
- Added getting started docs link to Detailed Documentation section [#1401](https://github.com/Autodesk/maya-usd/pull/1401)
- Keep the TF_ERROR message format consistence with TF_WARN and TF_STATUS [#1386](https://github.com/Autodesk/maya-usd/pull/1386)
- Update build.md for supported USD v21.05 version [#1365](https://github.com/Autodesk/maya-usd/pull/1365)

## [0.9.0] - 2021-04-01

**Build:**
* Added "/permissive-" flag for VS compiler [#1262](https://github.com/Autodesk/maya-usd/pull/1262)
* Explicitly case Ar path string to ArResolvedPath [#1261](https://github.com/Autodesk/maya-usd/pull/1261)
* Remove/replace all UFE_PREVIEW_VERSION_NUM [#1243](https://github.com/Autodesk/maya-usd/pull/1243)
* Namespace cleanup [#1214](https://github.com/Autodesk/maya-usd/pull/1214) [#1223](https://github.com/Autodesk/maya-usd/pull/1223) [#1231](https://github.com/Autodesk/maya-usd/pull/1231) [#1240](https://github.com/Autodesk/maya-usd/pull/1240)
* Allow ufeUtils.ufeFeatureSetVersion before Maya standalone initialize [#1237](https://github.com/Autodesk/maya-usd/pull/1237)
* Revert pxrUsdMayaGL tests to old color management [#1232](https://github.com/Autodesk/maya-usd/pull/1232)
* Fixed Maya 2018 compilation [#1225](https://github.com/Autodesk/maya-usd/pull/1225)
* Adapt tests to UsdShade GetInputs() and GetOutputs() API update with USD 21.05 [#1224](https://github.com/Autodesk/maya-usd/pull/1224)
* Fixed include order [#1211](https://github.com/Autodesk/maya-usd/pull/1211)
* Regression test for scene dirty when editing USD [#1205](https://github.com/Autodesk/maya-usd/pull/1205)
* Updates for building with latest post-21.02 dev branch commit of core USD [#1199](https://github.com/Autodesk/maya-usd/pull/1199)
* Remove dependency on boost
  - filesystem/system [#1182](https://github.com/Autodesk/maya-usd/pull/1182)
  - chrono and date_time [#1184](https://github.com/Autodesk/maya-usd/pull/1184)
  - hash_combine [#1183](https://github.com/Autodesk/maya-usd/pull/1183)
* Replaced usage of USD_VERSION_NUM with PXR_VERSION [#1181](https://github.com/Autodesk/maya-usd/pull/1181)

**Translation Framework:**
* Added support for indexed normals on import [#1250](https://github.com/Autodesk/maya-usd/pull/1250)
* Uvset names not written [#1188](https://github.com/Autodesk/maya-usd/pull/1188)
* Support duplicate blendshape target names [#1179](https://github.com/Autodesk/maya-usd/pull/1179)
* Maintain UV connectivity across export and re-import round trips [#1175](https://github.com/Autodesk/maya-usd/pull/1175)
* Added support import of textures from USDZ archives [#1151](https://github.com/Autodesk/maya-usd/pull/1151)
* Support import chaser plugins [#1104](https://github.com/Autodesk/maya-usd/pull/1104)

**Workflow:**
* Return error code for issues when saving [#1293](https://github.com/Autodesk/maya-usd/pull/1293)
* Fixed visibility attribute not blocked via outliner [#1275](https://github.com/Autodesk/maya-usd/pull/1275)
* Fixed crash when learing mutiple layers [#1274](https://github.com/Autodesk/maya-usd/pull/1274)
* Prim AE template:<br>
  - option to suppress array attributes [#1267](https://github.com/Autodesk/maya-usd/pull/1267)
  - show metadata [#1249](https://github.com/Autodesk/maya-usd/pull/1249)
* Undoing transform will not update manipulator [#1266](https://github.com/Autodesk/maya-usd/pull/1266)
* Don't convert the near and far clipping planes linear units to cm [#1265](https://github.com/Autodesk/maya-usd/pull/1265)
* Ability to select usd objects based on kind from the Select menu [#1260](https://github.com/Autodesk/maya-usd/pull/1260)
* Better return value (enum) from UI delegate [#1259](https://github.com/Autodesk/maya-usd/pull/1259)
* Part 1: Block attribute(s) authoring in weaker layer(s) [#1258](https://github.com/Autodesk/maya-usd/pull/1258)
* Added a camera test which relies on unimplemented methods [#1252](https://github.com/Autodesk/maya-usd/pull/1252)
* Minimize name-based lookup of proxy shape nodes [#1245](https://github.com/Autodesk/maya-usd/pull/1245)
* Added support for manipulating point instances via UFE [#1241](https://github.com/Autodesk/maya-usd/pull/1241)
* Stage AE template<br>
  - load/reload a file as stage source [#1218](https://github.com/Autodesk/maya-usd/pull/1218)
  - load/unload all payloads [#1203](https://github.com/Autodesk/maya-usd/pull/1203) [#1242](https://github.com/Autodesk/maya-usd/pull/1242)
  - show PrimPath/Purpose/Exlude Prim Path [#1189](https://github.com/Autodesk/maya-usd/pull/1189) [#1178](https://github.com/Autodesk/maya-usd/pull/1178) [#1176](https://github.com/Autodesk/maya-usd/pull/1176)
* No warning icon in USD Save Overwrite section [#1234](https://github.com/Autodesk/maya-usd/pull/1234)
* Fixed AE refresh problem when an Xformable is added [#1233](https://github.com/Autodesk/maya-usd/pull/1233)
* Undoable command base class [#1227](https://github.com/Autodesk/maya-usd/pull/1227)
* Separate callbacks for Save and Export [#1226](https://github.com/Autodesk/maya-usd/pull/1226)
* Multiple ProxyShapes with Same Stage [#1228](https://github.com/Autodesk/maya-usd/pull/1228)
* Null and anonymous layer checks [#1212](https://github.com/Autodesk/maya-usd/pull/1212)
* Multi matrix and fallback parent absolute [#1201](https://github.com/Autodesk/maya-usd/pull/1201)
* Fixed bbox computation for purposes [#1170](https://github.com/Autodesk/maya-usd/pull/1170)

**Render:**
* Added a basic test for Vp2RenderDelegate geom subset material binding support [#1255](https://github.com/Autodesk/maya-usd/pull/1255)
* Author a default value for color & opacity if none are present [#1254](https://github.com/Autodesk/maya-usd/pull/1254)
* Only assign the fallback material when no material is present [#1253](https://github.com/Autodesk/maya-usd/pull/1253)
* Added single-channel half-float EXR support [#1248](https://github.com/Autodesk/maya-usd/pull/1248)
* Added per instance inherited data support to Vp2RenderDelegate [#1220](https://github.com/Autodesk/maya-usd/pull/1220)
* Added support for 16-bits textures in VP2 renderer delegate [#1219](https://github.com/Autodesk/maya-usd/pull/1219)
* Fixed crash on failure to initialize a renderer [#1213](https://github.com/Autodesk/maya-usd/pull/1213)
* Use the nurbs curve selection mask for basis curves [#1208](https://github.com/Autodesk/maya-usd/pull/1208)

**Documentation:**
* Update comment about using universalRenderContext [#1271](https://github.com/Autodesk/maya-usd/pull/1271)
* Added namespace section to coding guidelines [#1239](https://github.com/Autodesk/maya-usd/pull/1239)
* Identify clang-format version 10.0.0 as the "standard" version [#1229](https://github.com/Autodesk/maya-usd/pull/1229)

## [0.8.0] - 2021-02-18

**Build:**
* Fixed UFEv2 test failures with USD 20.02 [#1171](https://github.com/Autodesk/maya-usd/pull/1171)
* Set MAYA_APP_DIR for each test, pointing to unique folders. [#1163](https://github.com/Autodesk/maya-usd/pull/1163)
* Disabled OCIOv2 in mtoh tests [#1160](https://github.com/Autodesk/maya-usd/pull/1160)
* Made tests in test/lib/ufe directly runnable and use fixturesUtils [#1144](https://github.com/Autodesk/maya-usd/pull/1144)
* Moved minimum core usd version to v20.02 [#1138](https://github.com/Autodesk/maya-usd/pull/1138)
* Made use of PXR_OVERRIDE_PLUGINPATH_NAME everywhere [#1108](https://github.com/Autodesk/maya-usd/pull/1108)
* Updates for building with latest post-21.02 dev branch commit of core USD [#1099](https://github.com/Autodesk/maya-usd/pull/1099)

**Translation Framework:**
* Support empty blendshape targets [#1149](https://github.com/Autodesk/maya-usd/pull/1149)
* Fixed load all prims in hierarchy view [#1140](https://github.com/Autodesk/maya-usd/pull/1140)
* Improved support for USD leftHanded mesh [#1139](https://github.com/Autodesk/maya-usd/pull/1139)
* Fixed export menu after expanding animation option [#1133](https://github.com/Autodesk/maya-usd/pull/1133)
* Added support for UV mirroring on UsdUVTexture [#1132](https://github.com/Autodesk/maya-usd/pull/1132)
* Imroved export of skeleton rest-xforms [#1130](https://github.com/Autodesk/maya-usd/pull/1130)
* Use earliest time when exporting without animation [#1127](https://github.com/Autodesk/maya-usd/pull/1127)
* Added error message if unrecognized shadingMode on export [#1110](https://github.com/Autodesk/maya-usd/pull/1110)
* Fixed anonymous layer import [#1086](https://github.com/Autodesk/maya-usd/pull/1086)

**Workflow:**
* Fixed crash when editing prim on a muted layer [#1172](https://github.com/Autodesk/maya-usd/pull/1172)
* Added workflows to save in-memory edits [#1152](https://github.com/Autodesk/maya-usd/pull/1152) [#1187](https://github.com/Autodesk/maya-usd/pull/1187)
* Removed attributes from AE stage view [#1145](https://github.com/Autodesk/maya-usd/pull/1145)
* Added support for duplicating a proxyShape. [#1142](https://github.com/Autodesk/maya-usd/pull/1142)
* Adopted data model undo framework for attribute undo [#1134](https://github.com/Autodesk/maya-usd/pull/1134)
* Prevent saving layers with anonymous sublayers [#1128](https://github.com/Autodesk/maya-usd/pull/1128)
* Added warning when saving layers on disk [#1121](https://github.com/Autodesk/maya-usd/pull/1121)
* Fixed crash caused by out of date stage map [#1116](https://github.com/Autodesk/maya-usd/pull/1116)
* Fixed crash when fallback stack was having unsupported ops added [#1105](https://github.com/Autodesk/maya-usd/pull/1105)
* Fixed parenting with single matrix xform op stacks [#1102](https://github.com/Autodesk/maya-usd/pull/1102)
* Author kind=group on the UsdPrim created by the group command when its parent is in the model hierarchy [#1094](https://github.com/Autodesk/maya-usd/pull/1094)
* Update edit target when layer removal made it no longer part of the stage [#1156](https://github.com/Autodesk/maya-usd/pull/1156)
* Fixed bounding box cache invalidation when editing USD stage [#1153](https://github.com/Autodesk/maya-usd/pull/1153)
* Handle selection changes during layer muting and variant switch [#1141](https://github.com/Autodesk/maya-usd/pull/1141)
* Fixed crash when removing an invalid layer [#1122](https://github.com/Autodesk/maya-usd/pull/1122)
* Added UFE path handling and utilities to support paths that identify point instances of a PointInstancer [#1027](https://github.com/Autodesk/maya-usd/pull/1027)

**Render:**
* Fixed crash after re-loading the mtoh plugin [#1159](https://github.com/Autodesk/maya-usd/pull/1159)
* Added compute extents for modified primitives [#1112](https://github.com/Autodesk/maya-usd/pull/1112)
* Fixed drawing of stale data when switching representations [#1100](https://github.com/Autodesk/maya-usd/pull/1100)
* Added hilight dirty state propagation [#1091](https://github.com/Autodesk/maya-usd/pull/1091)
* Implemented viewport selection highlighting of point instances and point instances pick modes [#1119](https://github.com/Autodesk/maya-usd/pull/1119)  [#1161](https://github.com/Autodesk/maya-usd/pull/1161)

**Documentation:**
* Added import/export cmd documentation [#1146](https://github.com/Autodesk/maya-usd/pull/1146)

## [0.7.0] - 2021-01-20

This release changes to highlight:
* Ufe cameras
* Blendshape export support
* Maya's scene will get modified by changes to USD data model 
* VP2RenderDelegate is enabled by default and doesn't require env variable flag anymore
* Maya-To-Hydra compilation is disabled for Maya 2019 with USD 20.08 or 20.11 

**Build:**
* Migrated more tests from plugin/pxr to core mayaUsd [#1042](https://github.com/Autodesk/maya-usd/pull/1042)
* Cleaned up UI folder [#1037](https://github.com/Autodesk/maya-usd/pull/1037)
* Cleaned up test utils [#1034](https://github.com/Autodesk/maya-usd/pull/1034)
* Updates for building with latest post-20.11 dev branch of core USD [#1025](https://github.com/Autodesk/maya-usd/pull/1025)
* Fixed  a build issue on Linux with maya < 2020 [#1013](https://github.com/Autodesk/maya-usd/pull/1013)
* Fixed interactive tests to ensure output streams are flushed before quitting [#1009](https://github.com/Autodesk/maya-usd/pull/1009)
* Added UsdMayaDiagnosticDelegate to adsk plugin to enable Tf errors reporting using Maya's routines [#1003](https://github.com/Autodesk/maya-usd/pull/1003)
* Added basic tests for HdMaya / mtoh [#915](https://github.com/Autodesk/maya-usd/pull/915) [#1006](https://github.com/Autodesk/maya-usd/pull/1006)

**Translation Framework:**
* Fixed issue where referenced scenes have incorrect UV set attribute name written out. [#1062](https://github.com/Autodesk/maya-usd/pull/1062)
* Fixed texture writer path resolver to work relative to final export path [#1028](https://github.com/Autodesk/maya-usd/pull/1028)
* Fixed incorrect `bindTransforms` being determined during export [#1024](https://github.com/Autodesk/maya-usd/pull/1024)
* Added Blendshape export functionality [#1016](https://github.com/Autodesk/maya-usd/pull/1016) 
* Added staticSingleSample flag, and optimize mesh attributes by default [#989](https://github.com/Autodesk/maya-usd/pull/989) [#1012](https://github.com/Autodesk/maya-usd/pull/1012)
* Added convert for USD timeSamples to Maya FPS [#970](https://github.com/Autodesk/maya-usd/pull/970) [#1010](https://github.com/Autodesk/maya-usd/pull/1010)

**Workflow:**
* Fixed camera frame for instance proxies. [#1082](https://github.com/Autodesk/maya-usd/pull/1082)
* Removed dependency between LayerEditor and HIK image resources [#1072](https://github.com/Autodesk/maya-usd/pull/1072)
* Fixed rename for inherits and specializes update [#1071](https://github.com/Autodesk/maya-usd/pull/1071)
* Added support for Ufe cameras [#1066](https://github.com/Autodesk/maya-usd/pull/1066)
* Editing USD should mark scene as dirty [#1061](https://github.com/Autodesk/maya-usd/pull/1061)
* Fixed crash when manipulating certain assets on Linux [#1060](https://github.com/Autodesk/maya-usd/pull/1060)
* Made visibility Undo/Redo correct [#1056](https://github.com/Autodesk/maya-usd/pull/1056)
* Fixed crash in LayerEditor when removing layer used as edit target [#1015](https://github.com/Autodesk/maya-usd/pull/1015)
* Fixed disappearing layer when moved under last layer in LayerEditor [#994](https://github.com/Autodesk/maya-usd/pull/994)

**Render:**
* Fixed false positive transparency [#1080](https://github.com/Autodesk/maya-usd/pull/1080)
* Made VP2RenderDelegate enabled by default [#1065](https://github.com/Autodesk/maya-usd/pull/1065)
* Fixed mtoh crash with DirectX [#1059](https://github.com/Autodesk/maya-usd/pull/1059)
* Optimized VP2RenderDelegate for many materials [#1043](https://github.com/Autodesk/maya-usd/pull/1043)  [#1048](https://github.com/Autodesk/maya-usd/pull/1048) [#1052](https://github.com/Autodesk/maya-usd/pull/1052)
* Disabled building of mtoh for Maya 2019 with USD 20.08 and 20.11 [#1023](https://github.com/Autodesk/maya-usd/pull/1023)

## [0.6.0] - 2020-12-14

This release includes many changes, like:
* Refactored UFE Manipulation support
* New Undo Manager
* LayerEditor
* Material import/export improvements (displacement, UDIMs, UVs) & bug fixes
* AE improvements
* Viewport optimization for many duplicated materials
* Undo support for UFEv2 selection in viewport
* Proxy accessor - handle recursive patterns
* Clang-format is applied to the entire repository

**Build:**
* Fixed compilation on windows if backslash in `Python_EXECUTABLE` [#968](https://github.com/Autodesk/maya-usd/pull/968)
* Enable USD version check for components distributed separately [#949](https://github.com/Autodesk/maya-usd/pull/949) [#846](https://github.com/Autodesk/maya-usd/pull/846)
* Fixed missing `ARCH_CONSTRUCTOR` code with VS2019 [#937](https://github.com/Autodesk/maya-usd/pull/937)
* Fixed current compiler warnings [#930](https://github.com/Autodesk/maya-usd/pull/930)
* Fixed compilation errors with latest post-20.11 dev branch of core USD  [#929](https://github.com/Autodesk/maya-usd/pull/929) [#889](https://github.com/Autodesk/maya-usd/pull/889)
* Added support for `Boost_NO_BOOST_CMAKE=O`N for Boost::python [#912](https://github.com/Autodesk/maya-usd/pull/912)
* Organized Gitignore, and add more editor,platform and build specific patterns [#908](https://github.com/Autodesk/maya-usd/pull/908)
* Applied clang-format [#890](https://github.com/Autodesk/maya-usd/pull/890) [#918](https://github.com/Autodesk/maya-usd/pull/918)  [#895](https://github.com/Autodesk/maya-usd/pull/895)
* Enabled initialization order warning on Windows. [#885](https://github.com/Autodesk/maya-usd/pull/885)
* Added mayaUtils.openTestScene and mayaUtils.getTestScene [#868](https://github.com/Autodesk/maya-usd/pull/868)
* Exposed stage reset notices to python [#861](https://github.com/Autodesk/maya-usd/pull/861)
* Clang-format fixes [#842](https://github.com/Autodesk/maya-usd/pull/842)
* Devtoolset-9 fixes [#841](https://github.com/Autodesk/maya-usd/pull/841)
* Made `WANT_UFE_BUILD` def public. [#793](https://github.com/Autodesk/maya-usd/pull/793)

**Translation Framework:**
* Fixed `Looks` scopes roundtrip [#985](https://github.com/Autodesk/maya-usd/pull/985)
* Added support for UV transform value exports [#954](https://github.com/Autodesk/maya-usd/pull/954)
* Allowed loading pure-USD shading mode plugins [#923](https://github.com/Autodesk/maya-usd/pull/923)
* Added opacity threshold to USD Preview Surface Shader [#917](https://github.com/Autodesk/maya-usd/pull/917)
* Fixed out of bound errors causing segfaults [#914](https://github.com/Autodesk/maya-usd/pull/914)
* Added FPS metadata to exported usd files if non static [#913](https://github.com/Autodesk/maya-usd/pull/913)
* Added proper support for importing UV set mappings [#902](https://github.com/Autodesk/maya-usd/pull/902)
* Fixed `map1` import [#892](https://github.com/Autodesk/maya-usd/pull/892)
* Fixed self-specialization bug with textureless materials [#891](https://github.com/Autodesk/maya-usd/pull/891)
* Added UDIM Import/Export [#883](https://github.com/Autodesk/maya-usd/pull/883)
* Added proper support for UV linkage on material export [#876](https://github.com/Autodesk/maya-usd/pull/876)
* Fix to keep USDZ texture paths relative [#865](https://github.com/Autodesk/maya-usd/pull/865)
* Added support for handling color space information on file nodes [#862](https://github.com/Autodesk/maya-usd/pull/862)
* Added import optimization to merge subsets connected to same material [#860](https://github.com/Autodesk/maya-usd/pull/860)
* Renamed InstanceSources to be Maya-specific [#859](https://github.com/Autodesk/maya-usd/pull/859)
* Added displacement export & import [#844](https://github.com/Autodesk/maya-usd/pull/844) [#851](https://github.com/Autodesk/maya-usd/pull/851)

**Workflow:**
* Added save layers dialog [#990](https://github.com/Autodesk/maya-usd/pull/990)
* Added Layer Editor [#988](https://github.com/Autodesk/maya-usd/pull/988)
* Fixed undo crash when switching between target layers in a stage [#965](https://github.com/Autodesk/maya-usd/pull/965)
* Made attributes categorized by schema in AE [#964](https://github.com/Autodesk/maya-usd/pull/964)
* Added new transform3d handlers to remove limitations in UFE manipulation support [#962](https://github.com/Autodesk/maya-usd/pull/962)
* Added Undo/Redo support for USD data model. [#942](https://github.com/Autodesk/maya-usd/pull/942) [#956](https://github.com/Autodesk/maya-usd/pull/956)
* Added connection to time when creating a new empty USD stage [#928](https://github.com/Autodesk/maya-usd/pull/928)
* Enabled recursive compute with proxy accessor [#926](https://github.com/Autodesk/maya-usd/pull/926)
* Cleaned up Proxy Shape AE template [#922](https://github.com/Autodesk/maya-usd/pull/922)  [#944](https://github.com/Autodesk/maya-usd/pull/944)
* Made select command UFE aware [#921](https://github.com/Autodesk/maya-usd/pull/921)
* Allowed usdPreviewSurface to exist without input connections [#907](https://github.com/Autodesk/maya-usd/pull/907)
* Fixed default edit target when root layer is not editable. Added ability to force session layer by default. [#899](https://github.com/Autodesk/maya-usd/pull/899) [#935](https://github.com/Autodesk/maya-usd/pull/935)
* Fixed outliner update when duplicating items [#880](https://github.com/Autodesk/maya-usd/pull/880)
* Added subtree invalidation support on StageProxy ( a.k.a pseudo-root ). [#864](https://github.com/Autodesk/maya-usd/pull/864)
* Added support None-destructive reorder ( move ) of prims relative to their siblings. [#867](https://github.com/Autodesk/maya-usd/pull/867)
* Made parenting to a Gprim not allowed [#852](https://github.com/Autodesk/maya-usd/pull/852)
* Improved accessor plug name and cleanup tests [#838](https://github.com/Autodesk/maya-usd/pull/838)

**Render:**
* Enabled color consolidation support for UsdPreviewSurface and UsdPrimvarReader_color [#979](https://github.com/Autodesk/maya-usd/pull/979)
* Fixed broken lighting fragment [#963](https://github.com/Autodesk/maya-usd/pull/963)
* Removed dependencies on old Hydra texture system from hdMaya for USD releases after 20.11 [#961](https://github.com/Autodesk/maya-usd/pull/961)
* Improved draw performance for Rprims without extent [#955](https://github.com/Autodesk/maya-usd/pull/955)
* Reuse shader effect for duplicate material networks [#950](https://github.com/Autodesk/maya-usd/pull/950)
* Added undo support for viewport selections of USD objects [#940](https://github.com/Autodesk/maya-usd/pull/940)
* Fixed transparency issues for pxrUsdPreviewSurface shader node [#903](https://github.com/Autodesk/maya-usd/pull/903)
* Fixed fallback shader (a.k.a "blue color") being incorrectly used in some cases [#882](https://github.com/Autodesk/maya-usd/pull/882)
* Added CPV support and curveBasis support for basiscurves complexity level 1 [#809](https://github.com/Autodesk/maya-usd/pull/809)

## [0.5.0] - 2020-10-20

**Build:**
* Added validation of exported USD in testUsdExportSkeleton for case without bind pose [#839](https://github.com/Autodesk/maya-usd/pull/839)
* Added support for core USD release 20.11 [#835](https://github.com/Autodesk/maya-usd/pull/835) [#808](https://github.com/Autodesk/maya-usd/pull/808) [#775](https://github.com/Autodesk/maya-usd/pull/775)
* Continue pxr tests migration and fixes [#828](https://github.com/Autodesk/maya-usd/pull/828) [#825](https://github.com/Autodesk/maya-usd/pull/825) [#815](https://github.com/Autodesk/maya-usd/pull/815) [#806](https://github.com/Autodesk/maya-usd/pull/806) [#779](https://github.com/Autodesk/maya-usd/pull/779)
* Cleaned up pxr CMake and remove irrelevant build options [#818](https://github.com/Autodesk/maya-usd/pull/818)
* Made necessary fixes required before we can apply clang format [#807](https://github.com/Autodesk/maya-usd/pull/807)
* Fixed Python 3 warnings [#783](https://github.com/Autodesk/maya-usd/pull/783)
* Fixed PROJECT_SOURCE_DIR use for nested projects. [#780](https://github.com/Autodesk/maya-usd/pull/780)
* Fixed namespace migration of QStringListModel between Maya 2019 and Maya 2020 in userExportedAttributesUI [#812](https://github.com/Autodesk/maya-usd/pull/812)

**Translation Framework:**
* Fixed import of "useSpecularWorkflow" input on UsdPreviewSurface shaders [#837](https://github.com/Autodesk/maya-usd/pull/837)
* Added translation support between lambert transparency and UsdPreviewSurface opacity 
* Material import options rework [#822](https://github.com/Autodesk/maya-usd/pull/822) [#831](https://github.com/Autodesk/maya-usd/pull/831) [#832](https://github.com/Autodesk/maya-usd/pull/832) [#833](https://github.com/Autodesk/maya-usd/pull/833) [#836](https://github.com/Autodesk/maya-usd/pull/836)
* Ensure UsdShadeMaterialBindingAPI schema is applied when authoring bindings during export [#804](https://github.com/Autodesk/maya-usd/pull/804)
* Updated custom converter test plugin to use symmetric writer [#803](https://github.com/Autodesk/maya-usd/pull/803)
* Added instance import support [#794](https://github.com/Autodesk/maya-usd/pull/794)
* Extracted RenderMan for Maya shading export from "pxrRis" shadingMode for use with "useRegistry" [#787](https://github.com/Autodesk/maya-usd/pull/787) [#799](https://github.com/Autodesk/maya-usd/pull/799)
* Fixed registry errors for preview surface readers and writers [#778](https://github.com/Autodesk/maya-usd/pull/778)

**Workflow:**
* Added prim type icons support in outliner [#782](https://github.com/Autodesk/maya-usd/pull/782)
* Made absence of scale pivot support is not an error in transform3d [#834](https://github.com/Autodesk/maya-usd/pull/834)
* Adapt to changes in UFE interface to support Maya transform stacks [#827](https://github.com/Autodesk/maya-usd/pull/827)
* Added UFE contextOps for working set management (loading and unloading) [#823](https://github.com/Autodesk/maya-usd/pull/823)
* Fixed crash after discarding edits on an anon layer [#817](https://github.com/Autodesk/maya-usd/pull/817)
* Added display for outliner to show which prims have a composition arc including a variant [#816](https://github.com/Autodesk/maya-usd/pull/816)
* Added support for automatic update of internal references when the path they are referencing has changed. [#797](https://github.com/Autodesk/maya-usd/pull/797)
* Rewrote the rename restriction algorithm from scratch to cover more edge cases. [#786](https://github.com/Autodesk/maya-usd/pull/786)
* Added Cache Id In/Out attributes to the Base Proxy Shape [#785](https://github.com/Autodesk/maya-usd/pull/785)

**Render:**
* Fixed refresh after change to excluded prims attribute [#821](https://github.com/Autodesk/maya-usd/pull/821)
* Fixed snapping to prevent snap to active objects when using point snapping [#819](https://github.com/Autodesk/maya-usd/pull/819)
* Always register and de-register the PxrMayaHdImagingShape and its draw override [#810](https://github.com/Autodesk/maya-usd/pull/810)
* Added selection and point snapping support for mtoh [#776](https://github.com/Autodesk/maya-usd/pull/776)
* Refactored settings / defaultRenderGlobals storage for mtoh [#758](https://github.com/Autodesk/maya-usd/pull/758)

## [0.4.0] - 2020-09-22

This release includes:
* Completed materials interop via PreviewSurface
* Improved rename and parenting workflows
* Improved Outliner support

**Build:**
* Fixed build script with python 3 where each line starting with 'b and ends with \n' making it hard to read the log. [PR #768](https://github.com/Autodesk/maya-usd/pull/768)
* Removed test dependencies on Python future past, and builtin.zip [PR #764](https://github.com/Autodesk/maya-usd/pull/764)
* Added test for setting rotate pivot through move command. [PR #763](https://github.com/Autodesk/maya-usd/pull/763)
* Migrated pxrUsdMayaGL and relevant pxrUsdTranslators tests from plugin/pxr/... to test/... [PR #762](https://github.com/Autodesk/maya-usd/pull/762) [PR #761](https://github.com/Autodesk/maya-usd/pull/761) [PR #759](https://github.com/Autodesk/maya-usd/pull/759) [PR #756](https://github.com/Autodesk/maya-usd/pull/756) [PR #741](https://github.com/Autodesk/maya-usd/pull/741) [PR #706](https://github.com/Autodesk/maya-usd/pull/706)
* Replaced #pragma once with #ifndef include guards [PR #750](https://github.com/Autodesk/maya-usd/pull/750)
* Removed old UFE condional compilation blocks [PR #746](https://github.com/Autodesk/maya-usd/pull/746)
* Removed code blocks for USD 19.07 [PR #734](https://github.com/Autodesk/maya-usd/pull/734)
* Split Maya module file into separate plugins [PR #731](https://github.com/Autodesk/maya-usd/pull/731)

**Translation Framework:**
* Fixed regression in displayColors shading mode to use the un-linearized lambert transparency to author displayOpacity [PR #751](https://github.com/Autodesk/maya-usd/pull/751)
* Renamed pxrUsdPreviewSurface to usdPreviewSurface [PR #744](https://github.com/Autodesk/maya-usd/pull/744)
* Fixed empty relative path handling for texture writer [PR #743](https://github.com/Autodesk/maya-usd/pull/743)
* Added support for multiple export converters to co-exist in useRegistry export [PR #721](https://github.com/Autodesk/maya-usd/pull/721)
* Cleaned-up export material UI [PR #709](https://github.com/Autodesk/maya-usd/pull/709)
* Added preview surface to Maya native shaders converstion [PR #707](https://github.com/Autodesk/maya-usd/pull/707)

**Workflow:**
* Added support in the outliner for displaying activate and deactivate prims [PR #773](https://github.com/Autodesk/maya-usd/pull/773)
* Fixed undo/redo re-parenting crash due to accessing a stale prim [PR #772](https://github.com/Autodesk/maya-usd/pull/772)
* Fixed matrix converstion when reading/writing to data handles [PR #765](https://github.com/Autodesk/maya-usd/pull/765)
* Fixed incorrect exception handling when setting variants in attribute editor for pxrUsdReferenceAssembly [PR #760](https://github.com/Autodesk/maya-usd/pull/760)
* Added undo support for multiple parented items in outliner [PR #755](https://github.com/Autodesk/maya-usd/pull/755)
* Fixed crash with undo redo in USD Layer Editor [PR #754](https://github.com/Autodesk/maya-usd/pull/754)
* Fixed `add new prim` after delete [PR #742](https://github.com/Autodesk/maya-usd/pull/742)
* Fixed crash after switching variant [PR #726](https://github.com/Autodesk/maya-usd/pull/726)
* Added support in the outliner for prim type icons [PR #712](https://github.com/Autodesk/maya-usd/pull/712)

**Render:**
* Added mtoh support for standardSurface [PR #749](https://github.com/Autodesk/maya-usd/pull/749)
* Fixed refresh with VP2RenderDelegate when the USD stage is cleared [PR #748](https://github.com/Autodesk/maya-usd/pull/748)
* Improved draw performance for dense mesh [PR #715](https://github.com/Autodesk/maya-usd/pull/715)

## [0.3.0] - 2020-08-15

### Build
* Fixed build script and test failures with python3 build [PR #700](https://github.com/Autodesk/maya-usd/pull/700)
* Fixed unused-but-set-variable on compiling with Maya 2018 earlier than 2018.7 [PR #699](https://github.com/Autodesk/maya-usd/pull/699)
* Cleaned up the status report when building USD with Python 3 [PR #688](https://github.com/Autodesk/maya-usd/pull/688)
* Removed unused python modules from tests. [PR #670](https://github.com/Autodesk/maya-usd/pull/670)
* Added support for core USD release 20.08 [PR #617](https://github.com/Autodesk/maya-usd/pull/617)

### Translation Framework
* Fixed import of non displayColors primvars [PR #703](https://github.com/Autodesk/maya-usd/pull/703)
* Fixed load Plug libraries when finding and loading in the registry [PR #694](https://github.com/Autodesk/maya-usd/pull/694)
* Fixed export of  texture file paths [PR #686](https://github.com/Autodesk/maya-usd/pull/686)
* Fixed import of UV sets [PR #685](https://github.com/Autodesk/maya-usd/pull/685)
* Added UI to display the number of selected prims and changed variants [PR #675](https://github.com/Autodesk/maya-usd/pull/675)

### Workflow
* Added Restriction Rules for Parenting [PR #673](https://github.com/Autodesk/maya-usd/pull/673) [PR #680](https://github.com/Autodesk/maya-usd/pull/680) [PR #687](https://github.com/Autodesk/maya-usd/pull/687) [PR #697](https://github.com/Autodesk/maya-usd/pull/697) [PR #682](https://github.com/Autodesk/maya-usd/pull/682)
* Fixed Outliner refresh after adding a reference [PR #689](https://github.com/Autodesk/maya-usd/pull/689)
* Fixed naming of stage and shape at creation to align with naming in Maya [PR #695](https://github.com/Autodesk/maya-usd/pull/695)
* Fixed Outliner refresh after rename [PR #677](https://github.com/Autodesk/maya-usd/pull/677)
* Changed default edit target from SessionLayer to RootLayer [PR #661](https://github.com/Autodesk/maya-usd/pull/661)
* Fixed the type when creating a new group [PR #672](https://github.com/Autodesk/maya-usd/pull/672)
* Added implementation for new UFE unparent interface [PR #665](https://github.com/Autodesk/maya-usd/pull/665)

### Render
* Fixed non-UFE selection code path when selecting instances in VP2RenderDelegate [PR #681](https://github.com/Autodesk/maya-usd/pull/681)
* Added selection highlight color for lead and active selection in VP2RenderDelegate [PR #671](https://github.com/Autodesk/maya-usd/pull/671)
* Fixed stage repopulation in mtoh [PR #650](https://github.com/Autodesk/maya-usd/pull/650)
* Added suppor for default material option in mtoh [PR #647](https://github.com/Autodesk/maya-usd/pull/647)
* Removed UseVp2 selection highlighting display from mtoh [PR #646](https://github.com/Autodesk/maya-usd/pull/646)

## [0.2.0] - 2020-07-22
This release includes support for Python 3 (requires USD 20.05). The goal of this beta/pre-release version is to validate:
* a new Maya 2020 installer
* basic import / export of simple Maya materials to UsdShade with PreviewSurface 
* viewport improvements

### Build
* Add python 3 support [PR #622](https://github.com/Autodesk/maya-usd/pull/622) [PR #602](https://github.com/Autodesk/maya-usd/pull/602) [PR #586](https://github.com/Autodesk/maya-usd/pull/586) [PR #573](https://github.com/Autodesk/maya-usd/pull/573) [PR #564](https://github.com/Autodesk/maya-usd/pull/564) [PR #555](https://github.com/Autodesk/maya-usd/pull/555) [PR #502](https://github.com/Autodesk/maya-usd/pull/502) [PR #637](https://github.com/Autodesk/maya-usd/pull/637)
* Split MayaUsd mod file into Windows and Linux/MacOS specific [PR #631](https://github.com/Autodesk/maya-usd/pull/631)
* Refactored import / export tests to run without dependency on Pixar plugin [PR #595](https://github.com/Autodesk/maya-usd/pull/595)
* Enabled GitHub Actions checks for maya-usd branches [PR #434](https://github.com/Autodesk/maya-usd/pull/434) [PR #533](https://github.com/Autodesk/maya-usd/pull/533) [PR #557](https://github.com/Autodesk/maya-usd/pull/557) [PR #565](https://github.com/Autodesk/maya-usd/pull/565) [PR #590](https://github.com/Autodesk/maya-usd/pull/590)
* Allowed parallel ctest execution with `ctest-args` build flag [PR #578](https://github.com/Autodesk/maya-usd/pull/578)
* Improved mayaUsd_copyDirectory routine to allow more control over copying dir/files. [PR #515](https://github.com/Autodesk/maya-usd/pull/515)
* Added override flag for setting WORKING_DIRECTORY. Enabled tests to run from build directory [PR #508](https://github.com/Autodesk/maya-usd/pull/508) [PR #616](https://github.com/Autodesk/maya-usd/pull/616)
* Removed support for Maya releases older than 2018 [PR #501](https://github.com/Autodesk/maya-usd/pull/501)
* Introduced `compiler_config.cmake` where all the compiler flags/options are applied. [PR #412](https://github.com/Autodesk/maya-usd/pull/412) [PR #443](https://github.com/Autodesk/maya-usd/pull/443) [PR #445](https://github.com/Autodesk/maya-usd/pull/445)
* Removed `cmake/defaults/Version.cmake` [PR #440](https://github.com/Autodesk/maya-usd/pull/440)
* Updated C++ standard version C++14 [PR #438](https://github.com/Autodesk/maya-usd/pull/438)
* Fixed the build when project is not created with Qt [PR #411](https://github.com/Autodesk/maya-usd/pull/411)
* Updated mayaUsd_add_test macro calls in AL plugin to add support for additional schema paths [PR #409](https://github.com/Autodesk/maya-usd/pull/409)
* Adopted coding guidelines (include directives and cmake) [PR #407](https://github.com/Autodesk/maya-usd/pull/407) [PR #408](https://github.com/Autodesk/maya-usd/pull/408) [PR #585](https://github.com/Autodesk/maya-usd/pull/585) [PR #544](https://github.com/Autodesk/maya-usd/pull/544) [PR #547](https://github.com/Autodesk/maya-usd/pull/547) [PR #653](https://github.com/Autodesk/maya-usd/pull/653)
* Various changes to comply with core USD dev branch commits (20.05 support + early 20.08) [PR #402](https://github.com/Autodesk/maya-usd/pull/402) [PR #449](https://github.com/Autodesk/maya-usd/pull/449) [PR #534](https://github.com/Autodesk/maya-usd/pull/534)
* Reorganized lib folder [PR #388](https://github.com/Autodesk/maya-usd/pull/388)
* Cleanded up build/usage requirements logics for cmake targets [PR #369](https://github.com/Autodesk/maya-usd/pull/369)
* Removed hard-coded "pxr" namespace in favor of PXR_NS in baseExportCommand [PR #540](https://github.com/Autodesk/maya-usd/pull/540)
* Fixed compilation error on gcc 9.1 [PR #441](https://github.com/Autodesk/maya-usd/pull/441)

### Translation Framework
* Added material import via registry [PR #621](https://github.com/Autodesk/maya-usd/pull/621)
* Filtered out objects to export by hierarchy [PR #657](https://github.com/Autodesk/maya-usd/pull/657)
* Added menu in Hierarchy View to reset the model and variant selections [PR #634](https://github.com/Autodesk/maya-usd/pull/634)
* Added option to save .usd files as binary or ascii [PR #630](https://github.com/Autodesk/maya-usd/pull/630)
* Fixed opacity vs. transparency issues with UsdPreviewSurface and pxrUsdPreviewSurface [PR #626](https://github.com/Autodesk/maya-usd/pull/626)
* Made export UV sets as USD Texture Coordinate value types by default [PR #618](https://github.com/Autodesk/maya-usd/pull/618)
* Filtered out wheel events on the variant set combo boxes [PR #600](https://github.com/Autodesk/maya-usd/pull/600)
* Added material export via registry [PR #574](https://github.com/Autodesk/maya-usd/pull/574)
* Moved mesh translator utilities to core library [PR #420](https://github.com/Autodesk/maya-usd/pull/420)
* Moved Pixar import / export commands into core lib library [PR #398](https://github.com/Autodesk/maya-usd/pull/398)
* Added .usdz extension to the identifyFile function [PR #396](https://github.com/Autodesk/maya-usd/pull/396)

### Workflow
* Fixed anonymous layer default name [PR #660](https://github.com/Autodesk/maya-usd/pull/660)
* Improved renaming support [PR #659](https://github.com/Autodesk/maya-usd/pull/659) [PR #639](https://github.com/Autodesk/maya-usd/pull/639) [PR #628](https://github.com/Autodesk/maya-usd/pull/628) [PR #627](https://github.com/Autodesk/maya-usd/pull/627) [PR #625](https://github.com/Autodesk/maya-usd/pull/625) [PR #483](https://github.com/Autodesk/maya-usd/pull/483)  [PR #475](https://github.com/Autodesk/maya-usd/pull/475)
* Added support for mixed data model compute [PR #594](https://github.com/Autodesk/maya-usd/pull/594)
* Updated UFE interface for Create Group [PR #593](https://github.com/Autodesk/maya-usd/pull/593)
* Reduced number of transform API conversions [PR #569](https://github.com/Autodesk/maya-usd/pull/569)
* Allowed tear-off of Create sub-menu [PR #549](https://github.com/Autodesk/maya-usd/pull/549)
* Added UFE interface for parenting [PR #455](https://github.com/Autodesk/maya-usd/pull/455) [PR #545](https://github.com/Autodesk/maya-usd/pull/545)
* Improved duplicate command to avoid name collisions [PR #541](https://github.com/Autodesk/maya-usd/pull/541)
* Fixed missing notice when newly created proxy shape is dirty [PR #536](https://github.com/Autodesk/maya-usd/pull/536)
* Removed all UFE interface data members from UFE handlers [PR #523](https://github.com/Autodesk/maya-usd/pull/523)
* Use separate UsdStageCache's for stages that loadAll or loadNone  [PR #519](https://github.com/Autodesk/maya-usd/pull/519)
* Added add/clear reference menu op's [PR #503](https://github.com/Autodesk/maya-usd/pull/503) [PR #521](https://github.com/Autodesk/maya-usd/pull/521)
* Added add prim menu op's [PR #489](https://github.com/Autodesk/maya-usd/pull/489)
* Added ability to create proxy shape with new in-memory root layer [PR #478](https://github.com/Autodesk/maya-usd/pull/478)
* UFE notification now watches for full xformOpOrder changes [PR #476](https://github.com/Autodesk/maya-usd/pull/476)
* Fixed object/sub-tree framing [PR #472](https://github.com/Autodesk/maya-usd/pull/472)
* Added clear stage cach on file new/open [PR #437](https://github.com/Autodesk/maya-usd/pull/437)
* Renamed 'Create Stage from Existing Layer' [PR #413](https://github.com/Autodesk/maya-usd/pull/413)
* Organized transform attributes in UFE AE [PR #383](https://github.com/Autodesk/maya-usd/pull/383)
* Updated UFE Attribute interface for USD to read attribute values for current time [PR #381](https://github.com/Autodesk/maya-usd/pull/381)
* Added UFE string path support [PR #368](https://github.com/Autodesk/maya-usd/pull/368)
* Removed MAYA_WANT_UFE_SELECTION from mod file [PR #367](https://github.com/Autodesk/maya-usd/pull/367)

### Render
* Added selection by kind [PR #641](https://github.com/Autodesk/maya-usd/pull/641)
* Fixed the regression for selecting single instance objects. [PR #620](https://github.com/Autodesk/maya-usd/pull/620)
* Added playblasting support to mtoh [PR #615](https://github.com/Autodesk/maya-usd/pull/615)
* Enabled level 1 complexity level for basisCurves [PR #598](https://github.com/Autodesk/maya-usd/pull/598)
* Enabled multi connections between shaders on Maya 2018 and 2019 [PR #597](https://github.com/Autodesk/maya-usd/pull/597)
* Support rendering purpose [PR #517](https://github.com/Autodesk/maya-usd/pull/517) [PR #558](https://github.com/Autodesk/maya-usd/pull/558) [PR #587](https://github.com/Autodesk/maya-usd/pull/587)
* Refactored and improved performance of Pixar batch renderer [PR #577](https://github.com/Autodesk/maya-usd/pull/577)
* Improved instance selection performance [PR #575](https://github.com/Autodesk/maya-usd/pull/575)
* Hide MRenderItems created for instancers with zero instances [PR #570](https://github.com/Autodesk/maya-usd/pull/570)
* Fixed crash with UsdSkel. Skinned mesh is not supported yet, only skeleton will be drawn [PR #559](https://github.com/Autodesk/maya-usd/pull/559)
* Added UDIM texture support [PR #538](https://github.com/Autodesk/maya-usd/pull/538)
* Fixed selection update on new objects [PR #537](https://github.com/Autodesk/maya-usd/pull/537)
* Split pre and post scene passes so camera guides go above Hydra in mtoh. [PR #526](https://github.com/Autodesk/maya-usd/pull/526)
* Fixed selection highlight disappears when switching variant [PR #524](https://github.com/Autodesk/maya-usd/pull/524)
* Fixing the crash when loading USDSkel [PR #514](https://github.com/Autodesk/maya-usd/pull/514)
* Removed UsdMayaGLHdRenderer [PR #482](https://github.com/Autodesk/maya-usd/pull/482)
* Fixed viewport refresh after changing the USD stage on proxy [PR #474](https://github.com/Autodesk/maya-usd/pull/474)
* Added support for outline selection hilighting and disabling color-quantization in mtoh [PR #469](https://github.com/Autodesk/maya-usd/pull/469)
* Added diffuse color multidraw consolidation [PR #468](https://github.com/Autodesk/maya-usd/pull/468)
* Removed Pixar batch renderer's support of MAYA_VP2_USE_VP1_SELECTION env var [PR #450](https://github.com/Autodesk/maya-usd/pull/450)
* Enabled consolidation for instanced render items [PR #436](https://github.com/Autodesk/maya-usd/pull/436)
* Fixed failure to create shader instance when running command-line render [PR #431](https://github.com/Autodesk/maya-usd/pull/431)
* Added a script to measure key performance information for MayaUsd Vp2RenderDelegate. [PR #415](https://github.com/Autodesk/maya-usd/pull/415)

### Documentation
* Updated build documentation regarding pip install [PR #638](https://github.com/Autodesk/maya-usd/pull/638)
* Updated build documentation regarding Catalina [PR #614](https://github.com/Autodesk/maya-usd/pull/614)
* Added Python, CMake guidelines and clang-format [PR #484](https://github.com/Autodesk/maya-usd/pull/484)
* Added Markdown version of Pixar Maya USD Plugin Doc [PR #384](https://github.com/Autodesk/maya-usd/pull/384)
* Added coding guidelines for maya-usd repository [PR #380](https://github.com/Autodesk/maya-usd/pull/380) [PR #400](https://github.com/Autodesk/maya-usd/pull/400)

## [0.1.0] - 2020-03-15
This release adds *all* changes made to refactoring_sandbox branch, including BaseProxyShape, Maya's Attribute Editor and Outliner support via Ufe-USD runtime plugin, VP2RenderDelegate and Maya To Hydra. Added Autodesk plugin with Import/Export workflows and initial support for stage creation.

Made several build improvements, including fixing regression tests execution on all supported platforms. 

### Build
* Fixed regression tests to run on all supported platforms [PR #198](https://github.com/Autodesk/maya-usd/pull/198) 
* Improved CMake code and changed minimum version [PR #323](https://github.com/Autodesk/maya-usd/pull/323)
* Number several build documentation [PR #208](https://github.com/Autodesk/maya-usd/pull/208) [PR #150](https://github.com/Autodesk/maya-usd/issues/150) [PR #346](https://github.com/Autodesk/maya-usd/pull/346)
* Improved configuration of “mayaUSD.mod" with build variables [Issue #21](https://github.com/Autodesk/maya-usd/issues/21) [PR #29](https://github.com/Autodesk/maya-usd/pull/29) 
* Enabled strict compilation mode for all platforms [PR #222](https://github.com/Autodesk/maya-usd/pull/222) [PR #275](https://github.com/Autodesk/maya-usd/pull/275) [PR #293](https://github.com/Autodesk/maya-usd/pull/293)
* Enabled C++11 for GoogleTest external project [#290](https://github.com/Autodesk/maya-usd/pull/290)
* Added UFE_PREVIEW_VERSION_NUM pre-processor support. [#289](https://github.com/Autodesk/maya-usd/pull/289)
* Qt support for older versions of Maya [PR #318](https://github.com/Autodesk/maya-usd/pull/318)
* Added a timer to measure the elapsed time to finish the build. [PR #307](https://github.com/Autodesk/maya-usd/pull/307)
* Added HEAD sha/date in the build.log [PR #229](https://github.com/Autodesk/maya-usd/pull/229)
* Fixed & closed several build issues

### Translation framework
* Refactored Pixar plugin to create one translation framework in mayaUsd core library. Moved Pixar translator plugin to build as a shared module across all studio plugins [PR #154](https://github.com/Autodesk/maya-usd/pull/154) 
* Refactored AL_USDUtils to create a new shared library under lib. Added support for C++ tests in lib [PR #288](https://github.com/Autodesk/maya-usd/pull/288)

### Workflows
* Added bounding box interface to UFE and fixed frame selection [PR #210](https://github.com/Autodesk/maya-usd/pull/210)
* Added Attribute Editor support  [PR #142](https://github.com/Autodesk/maya-usd/pull/142) 
* Added base proxy shape and UFE-USD runtime plugin  [PR #91](https://github.com/Autodesk/maya-usd/pull/91) 
* Added attribute observation to UFE and fixed viewport and AE refresh issues [PR #204](https://github.com/Autodesk/maya-usd/pull/204)
* Added contextual operations to UFE with basic UFE-USD implementation [PR #303](https://github.com/Autodesk/maya-usd/pull/303)
* Added MayaReference schema and reader implementation [#255](https://github.com/Autodesk/maya-usd/pull/255)
* Fixed duplicating proxy shape crash [Issue #69](https://github.com/Autodesk/maya-usd/issues/69)
* Fixed renaming of prims [Issue #75](https://github.com/Autodesk/maya-usd/issues/75) [PR #237](https://github.com/Autodesk/maya-usd/pull/237)
* Fixed duplicate prim [Issue #72](https://github.com/Autodesk/maya-usd/issues/72)

*Autodesk plugin*
* Added import UI [PR #304](https://github.com/Autodesk/maya-usd/pull/304)  [PR #206](https://github.com/Autodesk/maya-usd/pull/206)  [PR #276](https://github.com/Autodesk/maya-usd/pull/276)
* Clear frameSamples set [#326](https://github.com/Autodesk/maya-usd/pull/326)
* Added export UI [PR #216](https://github.com/Autodesk/maya-usd/pull/216)
* Enabled import of .usdz files  [PR #313](https://github.com/Autodesk/maya-usd/pull/313)
* Added "Create USD Stage" to enable proxy shape creation from Maya's UI [PR #306](https://github.com/Autodesk/maya-usd/pull/306) [PR #317](https://github.com/Autodesk/maya-usd/pull/317)

*Animal Logic plugin*
* Disabled push to prim to avoid generation of overs while updating the data [PR #295](https://github.com/Autodesk/maya-usd/pull/295)
* Added approximate equality checks to determine if new value needs to be saved [PR #294](https://github.com/Autodesk/maya-usd/pull/294)
* Fixed colour set export type to be Color3f/Color4f [PR #274](https://github.com/Autodesk/maya-usd/pull/274)
* Improved interfaces for BasicTransformationMatrix and TransformationMatrix [PR #251](https://github.com/Autodesk/maya-usd/pull/251)
* Improved ColorSet import/export to support USD displayOpacity primvar [PR #250](https://github.com/Autodesk/maya-usd/pull/250)
* Fixed crash using TranslatePrim non-recursively on a previously translated transform/scope  [PR #249](https://github.com/Autodesk/maya-usd/pull/249)
* Added generate unique key method to reduce translator updates to only dirty prims [PR #248](https://github.com/Autodesk/maya-usd/pull/248)
* Improved time to select objects in the viewport for ProxyDrawOverride [PR #247](https://github.com/Autodesk/maya-usd/pull/247)
* Improved push to prim to avoid generating overs [Issue PR #246](https://github.com/Autodesk/maya-usd/pull/246)
* Improved performance when processing prim meta data [PR #245](https://github.com/Autodesk/maya-usd/pull/245)
* Improved USDGeomScope import  [PR #232](https://github.com/Autodesk/maya-usd/pull/232)
* Added USDGeomScope support in Proxy Shape  [PR #185](https://github.com/Autodesk/maya-usd/pull/185) 
* Changed pushToPrim default to On [PR #149](https://github.com/Autodesk/maya-usd/pull/149)

*Pixar plugin*
* Fixed uniform buffer bindings during batch renderer selection [PR #291](https://github.com/Autodesk/maya-usd/pull/291)
* Added  profiler & performance tracing when using batch renderer [PR #260](https://github.com/Autodesk/maya-usd/pull/260)

### Render
* Added VP2RenderDelegate  [PR #91](https://github.com/Autodesk/maya-usd/pull/91) 
* Added support for wireframe and bounding box display [PR #112](https://github.com/Autodesk/maya-usd/pull/112) 
* Fixed selection highlight when selecting multiple proxy shapes [Issue #105](https://github.com/Autodesk/maya-usd/issues/105)
* Fixed moving proxy shape in VP2RenderDelegate [Issue #71](https://github.com/Autodesk/maya-usd/issues/71)
* Fixed hiding proxy shape  in VP2RenderDelegate [Issue #70](https://github.com/Autodesk/maya-usd/issues/70)
* Added a way for pxrUsdProxyShape to disable subpath selection with the VP2RenderDelegate [PR #315](https://github.com/Autodesk/maya-usd/pull/315)
* Added support for USD curves in VP2RenderDelegate  [PR #228](https://github.com/Autodesk/maya-usd/pull/228)

* Added Maya-to-Hydra (mtoh) plugin and scene delegate [PR #231](https://github.com/Autodesk/maya-usd/pull/231)
* Fixed material support in hdMaya with USD 20.2 [Issue #135](https://github.com/Autodesk/maya-usd/issues/135)
* Removed MtohGetDefaultRenderer [PR #311](https://github.com/Autodesk/maya-usd/pull/311)
* Fixed mtoh initialization to reject invalid delegates. [#292](https://github.com/Autodesk/maya-usd/pull/292)

## [v0.0.2] - 2019-10-30
This release has the last merge from original repositories. Brings AnimalLogic plugin to version 0.34.6 and Pixar plugin to version 19.11. Both plugins now continue to be developed only in maya-usd repository.

## [v0.0.1] - 2019-10-01
This is the first released version containing AnimalLogic (0.31.1) and Pixar (19.05) plugins.

