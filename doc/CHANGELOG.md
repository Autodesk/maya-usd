# Changelog

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
  - Ignored setting playback range if no time code found from USD [\#1686](https://github.com/Autodesk/maya-usd/pull/1686)
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
* Clear frameSamples set [#326](https://github.com/Autodesk/maya-usd/pull/326)
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

