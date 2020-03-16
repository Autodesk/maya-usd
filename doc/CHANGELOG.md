# Changelog

## [0.1.0] - 2020-03-15
This release adds *all* changes made to refactoring_sandbox branch, including BaseProxyShape, Maya’s Attribute Editor and Outliner support via Ufe-USD runtime plugin, VP2RenderDelegate and Maya To Hydra. Added Autodesk plugin with Import/Export workflows and initial support for stage creation.

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
* MAYA-103736 Clear frameSamples set [#326](https://github.com/Autodesk/maya-usd/pull/326)
* Added export UI [PR #216](https://github.com/Autodesk/maya-usd/pull/216)
* Enabled import of .usdz files  [PR #313](https://github.com/Autodesk/maya-usd/pull/313)
* Added “Create USD Stage” to enable proxy shape creation from Maya’s UI [PR #306](https://github.com/Autodesk/maya-usd/pull/306) [PR #317](https://github.com/Autodesk/maya-usd/pull/317)

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

