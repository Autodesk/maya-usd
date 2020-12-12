# Changelog

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

