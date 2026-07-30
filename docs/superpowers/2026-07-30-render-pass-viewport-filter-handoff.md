# Handoff — USD render pass include/exclude in the Maya viewport

**Date:** 2026-07-30
**Status:** implemented and **verified working in Maya**. The open decision below was resolved —
see [What was built](#what-was-built).

> For a reviewable summary, read
> [`2026-07-30-render-pass-viewport-filter-overview.md`](2026-07-30-render-pass-viewport-filter-overview.md)
> instead. This file is the long-form investigation record.

## Goal

Prototype filtering prims out of the Maya viewport based on a `UsdRenderPass` prim's
collections. A property on the USD proxy shape names the active render pass; a Hydra
filtering scene index in the pipeline feeding the VP2 render delegate applies that pass's
collection to prune prims.

## Decisions locked in

| Question | Decision |
|---|---|
| Spike goal | **Demo-able UX** — an attribute on the proxy shape, prims visibly disappear. Showable over architecturally pure. |
| Pass semantics | **`prune` first, `renderVisibility` after.** Pixar ships a prune filter; renderVisibility has no filter in USD 25.11 and is the piece worth upstreaming. |
| Pass provenance | **Pre-authored in the USD asset.** Hand-written `.usda` test asset. No Maya-side authoring UI in scope. |
| Proxy shape attribute | **String prim path attribute**, mirroring the existing `excludePrimPaths` precedent. No AE dropdown in scope. |

## What was built

The A/B decision below was overtaken by a finding: **`arnold-usd`'s
`plugins/scene_index/renderPassSIP.cpp` is a modified copy of hdPrman's render pass scene
index**, handling `prune`, `renderVisibility`, `cameraVisibility` and `matte` in one filter.
Its `prune` and `renderVisibility` paths are renderer-neutral — `renderVisibility` overlays a
standard `HdVisibilitySchema`, no Arnold tokens — so a `renderVisibility` filter did *not* need
writing, and it is not the novel upstreamable artifact the notes below assume. `cameraVisibility`
and `matte` are Arnold-specific (they write `primvars:arnold:*`) and were dropped: VP2 has no
renderer-neutral destination for them.

Chosen approach, a variant of B ("B1"): fork Arnold's filter **unmodified** in its
scene-globals-driven form, and synthesize the pass prim it expects rather than imaging the stage
twice. Per proxy shape:

```
emulation ─┐
           ├─► HdMergingSceneIndex ─► HdsiSceneGlobalsSceneIndex ─► filter
 retained ─┘
```

This is approach A's shape minus the two things that made A expensive: no second
`UsdImagingStageSceneIndex`, and no per-frame `ApplyPendingUpdates()` pump (`stageSceneIndex.h:83`
— it is app-driven, and VP2 has nowhere to host it).

### How the chain is attached — the append callback does NOT work

**The original plan used `RegisterSceneIndexForRenderer`'s append callback. That silently never
runs for VP2.** `renderIndex.cpp:218-227` gates it:

```cpp
const std::string &rendererDisplayName = renderDelegate->GetRendererDisplayName();
if (!rendererDisplayName.empty()) {                       // ← never true for VP2
    sceneIndex = ...AppendSceneIndicesForRenderer(...);
}
```

`_displayName` is only populated by `HdRendererPlugin::CreateDelegate` (`rendererPlugin.cpp:70-73`),
and `HdVP2RenderDelegate` is constructed directly, so it stays empty. The registry's "empty
`rendererDisplayName` applies to all renderers" rule governs *registration*, not *invocation* — an
easy and costly misreading. `_displayName` is private, `SetTerminalSceneIndex` is called after the
adapter delegate already exists (`renderIndex.cpp:239-245`), and passing a `terminalSceneIndex` to
`HdRenderIndex::New` sets `_SetDisableEmulationAPI(true)`, so none of those are ways in either.

**What works instead** (`MayaUsdRenderPassPublisher::Attach`), the pattern 3ds Max USD uses for its
light-gizmo filter: walk down from `GetTerminalSceneIndex()` to the first `HdMergingSceneIndex`,
take its single input (the legacy/emulation chain), **`RemoveSceneIndex` it**, then
`InsertSceneIndex` the filter that wraps it. Removing first is the essential step — inserting alone
makes the filter a *sibling* that cannot see legacy prims, which is why the original notes wrongly
dismissed `InsertSceneIndex` outright. Verified in 25.11: `InsertSceneIndex(scene, "/")` skips the
prefixing wrapper (`renderIndex.cpp:322-328`) and `RemoveSceneIndex` matches by pointer identity
against the merging index's inputs (`renderIndex.cpp:355-364`), including inputs the render index
added internally. This needs no renderer display name and no `instanceName`.

Attachment happens in `_InitRenderDelegate` right after `HdRenderIndex::New`, while the scene is
still empty — `_Populate()` does not run until later in `update()`.

Files:

- **`lib/mayaUsd/render/vp2RenderDelegate/renderPassSceneIndex.{h,cpp}`** —
  `MayaUsdRenderPassSceneIndex`, the fork. Pixar copyright retained. `_UpdateActiveRenderPassState`
  and the entry-filtering helpers are unchanged from the original; only `cameraVisibility`/`matte`
  were stripped.
- **`renderPassPublisher.{h,cpp}`** — `Attach()` performs the interposition above; `Publish()` reads
  the pass's collections off the USD stage, rebases the expressions, publishes them into an
  `HdRetainedSceneIndex`, and names the pass active via `HdsiSceneGlobalsSceneIndex`.
- **`proxyShapeBase.{h,cpp}`** — `activeRenderPass` / `arp` string attribute plus version counter,
  following the `excludePrimPaths` pattern exactly.
- **`proxyRenderDelegate.{h,cpp}`** — calls `Attach()` after render index creation and republishes
  from `_UpdateSceneDelegate` when the version changes.
- **`debugCodes.{h,cpp}`** — `HDVP2_DEBUG_RENDER_PASS`, tracing every boundary: attachment, publish
  (authored vs. rebased expressions), and filter state (active pass, compiled expressions, and
  counts of prims pruned / re-added / visibility-dirtied). Enable with
  `Tf.Debug.SetDebugSymbolsByName('HDVP2_DEBUG_RENDER_PASS', True)`; `show()` does this by default.
- **`scripts/renderPassSpikeDemo.py`** — authors a nine-cube / three-group stage with five render
  passes, loads it into a proxy shape, and opens a window for switching passes.

### The path-prefix problem (not in the original notes)

`UsdImagingDelegate::ConvertCachePathToIndexPath` (`delegate.h:453`) prefixes every emitted prim
with the delegate ID, so `/World/Sphere` is `/Proxy_pShape1_0xABCD/World/Sphere` in the Hydra
scene. Collection expressions authored against raw USD paths therefore match **nothing**.
`MayaUsdRenderPassPublisher` rebases them with `SdfPathExpression::ReplacePrefix`
(`pathExpression.h:260`). This also rules out approach A, which has no hook to rebase a stage
scene index's expressions.

`ReplacePrefix` behaviour on `//` descendant patterns is **still unverified** — no `pxr` Python in
the sandbox to check it. Its failure mode is silent (nothing filters), so
`renderPassSpikeDemo.checkReprefix()` tests it with the same API and runs automatically from
`show()`. **If nothing filters in the viewport, read that output before suspecting anything else.**

### Unrelated pre-existing breakage, temporarily patched

`lib/usd/ui/debugTools/CompositionEditorCmd.cpp` failed to compile before any of this work
(verified: `git diff --ignore-all-space` shows zero real changes). `executeInCmd`,
`loadPersistentData` and `savePersistentData` are wrapped in `#if 0` to unblock the build.

**The cause is a stale CMake cache, not a stale artifact.** In
`build_mayausd_2027_edit_forward/build/RelWithDebInfo/CMakeCache.txt`:

```
ADSK_USD_DEBUG_TOOLS_INCLUDE_DIR:PATH=.../adskusddebugtool/b190bae/include   ← stale
ADSK_USD_DEBUG_TOOLS_ROOT_DIR:UNINITIALIZED=.../adskusddebugtool/a503391     ← what build.py passes
```

`a503391/include/UsdDebugUI/ApplicationHost.h:67,76,83` declares all three virtuals with exactly
the signatures the `.cpp` uses; `b190bae`'s does not. `ROOT_DIR` is overridden per-invocation but
the cached `PATH`/`FILEPATH` find-results are not re-evaluated, so the compiler keeps reading
b190bae's header. **No artifact update is needed** — the fix is to clear
`ADSK_USD_DEBUG_TOOLS_INCLUDE_DIR`, `_CORE_LIBRARY` and `_UI_LIBRARY` from the cache (or wipe the
build dir) and reconfigure, then delete the `#if 0` guards.

Running the relay's `configure` does **not** currently do this: it aborts early with
`ModuleNotFoundError: No module named 'jinja2'` at `test/lib/mayaUsd/fileio/CMakeLists.txt:96`
(mayapy is missing jinja2). That is a separate pre-existing host issue worth fixing on its own —
configure is effectively unavailable until it is.

## Superseded: the original open decision

Kept for context. Approach A vs B (C is a fallback only). Both register the same way; they differ only in how
the prune expression reaches the filter.

**A — Pixar's filter, faithfully wired.** Append callback installs `UsdImagingStageSceneIndex`
over the same stage, narrowed to `/Render` via `HdsiPrefixPathPruningSceneIndex` so the pass
prim's collections are present without double-populating geometry → `HdsiSceneGlobalsSceneIndex`
to name the active pass → `HdsiRenderPassPruneSceneIndex`.
*For:* zero filter code, Hydra resolves collections correctly, survives VP2's eventual
Hydra 2.0 migration. *Against:* three collaborating scene indices before anything renders; a
failed smoke test won't say which link broke.

**B — thin custom prune filter (was the recommendation).** ~100-line
`HdSingleInputFilteringSceneIndexBase` subclass holding an `SdfPathExpression` +
`HdCollectionExpressionEvaluator`, with `SetPruneExpression()`. Maya side reads the pass's
prune collection via `UsdCollectionAPI` and pushes the expression in.
*For:* one new moving part instead of three; no scene globals or pass-prim plumbing; we own
invalidation; already the right shape for `renderVisibility` next. *Against:* resolving the
collection Maya-side may drift from what a real renderer computes; not "wire up Pixar's filter."

**C — fallback, no scene index.** Resolve the collection to an `SdfPathVector` and feed the
existing `excludePrimPaths` machinery. Demos almost certainly, validates nothing, can't express
wildcards. Only if A and B both die on the smoke test below.

## The load-bearing assumption — verify this first

**Does pruning at the terminal scene index actually remove rprims from VP2's tables?**
`HdSceneIndexAdapterSceneDelegate` is what populates them. This is believed true but
**unverified**, and every approach above depends on it.

Step one of implementation, regardless of approach: register an append callback that prunes a
single hardcoded prim path, and confirm that geometry disappears in the viewport. If it does
not, A/B/C all collapse and the work becomes migrating VP2 to a real scene index chain.

## Verified technical findings

All file:line refs verified during this session unless marked otherwise.

### VP2 does not have a scene index chain of its own

- `lib/mayaUsd/render/vp2RenderDelegate/proxyRenderDelegate.cpp:715` — news up a
  `UsdImagingDelegate`; `:818` populates via `_sceneDelegate->Populate(prim, excludePrimPaths)`.
- **Zero** `InsertSceneIndex` / `SetSceneIndex` / `HdSceneIndexAdapterSceneDelegate` call sites
  anywhere in the maya-usd tree.
- The chain in `lib/mayaUsd/sceneIndex/` feeds mayaHydra and UFE/selection, **not** VP2.
- No CMake flag, env var, or preprocessor guard switches VP2 to a scene-index path. Recent
  scene-index commits are all `HYDRA-` prefixed mayaHydra work; none touch `vp2RenderDelegate`.

### But front-end emulation gives us a real chain anyway

Chain order inside `HdRenderIndex` (from the 25.11 header plus a fetch of `renderIndex.cpp` —
not verified by stepping through our build):

```
_emulationSceneIndex (HdLegacyPrimSceneIndex)
  → HdLegacyGeomSubsetSceneIndex
  → _mergingSceneIndex (HdMergingSceneIndex)
  → HdSceneIndexPluginRegistry::AppendSceneIndicesForRenderer(...)   ← our hook
  → [HdCachingSceneIndex if enabled]
  → _terminalSceneIndex
  → HdSceneIndexAdapterSceneDelegate → render index tables → VP2
```

- `InsertSceneIndex` is **not** the hook we want — it calls
  `_mergingSceneIndex->AddInputScene(...)`, making the filter a *sibling* of emulation rather
  than wrapping it. It cannot filter legacy prims.
- There is no public setter for `_terminalSceneIndex`. Wrapping `GetTerminalSceneIndex()` and
  re-inserting would be cyclic.

### The interposition point

`pxr/imaging/hd/sceneIndexPluginRegistry.h` — the **callback** overload of
`RegisterSceneIndexForRenderer(rendererDisplayName, SceneIndexAppendCallback, inputArgs,
insertionPhase, insertionOrder)`. Its own doc note: *"This method should be invoked before
render index construction when Hydra scene index emulation is enabled."* No `plugInfo.json` or
`TfType` registration needed.

Why it reaches VP2:
- `HdRenderDelegate::GetRendererDisplayName()` (`renderDelegate.h:506`) returns `_displayName`,
  *"Populated when instantiated via the HdRendererPluginRegistry."* `HdVP2RenderDelegate` is
  constructed directly, so this is `""`.
- `AppendSceneIndicesForRenderer` docs: plugins registered with an empty `rendererDisplayName`
  apply to all renderers, added ahead of renderer-specific ones.

### Known gap: no per-proxy-shape scoping

`proxyRenderDelegate.cpp:690` — `HdRenderIndex::New(_renderDelegate.get(), HdDriverVector())`.
Both `instanceName` and `appName` default to `""`, so `renderInstanceId` is empty for *every*
proxy shape and the callback can't tell them apart.

- Fix: pass the proxy shape's path as `instanceName` — a one-line change.
- The empty `appName` would also have blocked auto-loading a `plugInfo`-declared library. Another
  reason to use the callback form.

### `prune` vs `renderVisibility` (`pxr/usd/usdRender/pass.h:63-116`)

- **`renderVisibility`** — `includeRoot=true`, so everything renders and you exclude from there
  (or set `includeRoot=false` and include explicitly, or use `membershipExpression`). Doc calls it
  *"a lightweight attribute that is relatively cheap to toggle during interactive workflows."*
  Cannot resurrect prims already invisible via `ComputeEffectiveVisibility()`. Applies only to
  renderable content, not render-settings objects.
- **`prune`** — removes objects from the scene before rendering. *"Greater runtime cost savings
  for batch rendering, with the tradeoff that interactively modifying the prune collection is
  likely to be more expensive than toggling visibility."* Guaranteed removal where a renderer
  doesn't support visibility for some object kind.
- Practical consequence in VP2: prune produces prim add/remove notices, so rprims are destroyed
  and rebuilt on every pass edit; `renderVisibility` would be a visibility dirty bit. On screen
  they look the same. Pruned prims also leave the Hydra scene entirely, so they disappear from
  viewport selection.
- Note the mismatch: "include/exclude list" is `renderVisibility`'s vocabulary. `prune` is
  one-directional — a list of things to remove.

### `HdsiRenderPassPruneSceneIndex`

`pxr/imaging/hdsi/renderPassPruneSceneIndex.h`, gated `PXR_VERSION >= 2408`. Ours is **2511**, so
available. Reads the active pass from `HdSceneGlobalsSchema`, builds an
`HdCollectionExpressionEvaluator` from the pass's prune expression, drops prims in `GetPrim` /
`GetChildPrimPaths`. Header note: *"assumes that the active render pass is a UsdRenderPass for
the purposes of collection naming conventions."*

Also confirmed present: `hdsi/sceneGlobalsSceneIndex.h`, `hdsi/prefixPathPruningSceneIndex.h`,
`hd/collectionsSchema.h`, `hd/collectionExpressionEvaluator.h`, `usd/usdRender/pass.h`.
**Not** present: any `UsdImagingRenderPassSceneIndex` / `usdImaging/renderPassSceneIndex.h`, and
no hdsi filter for `renderVisibility`, `cameraVisibility`, or `matte`.

### Proxy shape attribute precedent — `excludePrimPaths`

Copy this pattern for the new attribute:

- Declaration: `lib/mayaUsd/nodes/proxyShapeBase.h:105` — `static MObject excludePrimPathsAttr;`
- Registration: `proxyShapeBase.cpp:292-298` —
  `typedAttrFn.create("excludePrimPaths", "epp", MFnData::kString, ...)`, then
  `setInternal(true)`, `setAffectsAppearance(true)`, `addAttribute(...)`.
- Invalidation: `proxyShapeBase.cpp:1831` (`preEvaluation`, via
  `evaluationNode.dirtyPlugExists(...)`) and `:1889` (`setDependentsDirty`), both bumping a
  version counter and calling `MHWRender::MRenderer::setGeometryDrawDirty(thisMObject())`.
- Parsing: `proxyShapeBase.cpp:2160-2171` — comma-separated, `TfStringTokenize` + `TfStringTrim`.
- Consumer-side version caching: `proxyRenderDelegate.h:312-314`, `IsExcludePrimsUpToDate()` /
  `ExcludePrimsUpdated()` around `proxyRenderDelegate.cpp:2324-2328`.

### hydra-viewport-toolbox is a dead end for reuse

Ships exactly three scene index filters — `BoundingBoxSceneIndex`, `WireFrameSceneIndex`,
`DisplayStyleOverrideSceneIndex` (per `source/sceneIndex/CMakeLists.txt`). None prune, hide, or
isolate. No `UsdRenderPass`, no `UsdCollectionAPI`, no collection-expression evaluation anywhere.
Its only include/exclude is legacy `HdRprimCollection` root-path + exclude-paths via
`FramePassParams::collection` (see `test/howTos/howTo07_UseIncludeExclude.cpp`), which cannot
express collection expressions, `includeRoot`, or `expansionRule`.

Consequences for the plan:
- The wheel to reuse is **Pixar's, not the toolbox's**.
- This partly undercuts "move the filter to the toolbox later" — what's left to write is
  Maya-specific glue that wouldn't belong there. The one genuinely upstreamable artifact would be
  a **`renderVisibility`** filter, since USD ships none.
- If we do upstream: house pattern is deriving straight from `HdSingleInputFilteringSceneIndexBase`
  with a `virtual bool _IsExcluded(SdfPath const&)` hook, everything in `HVT_NS` / `HVT_API`.
  Apache 2.0, CLA required (`openusd.hydra@autodesk.com`), GoogleTest mandatory with the PR,
  `.clang-format` is `BasedOnStyle: Microsoft` / `ColumnLimit: 100`. HVT pins USD 26.5.
  Caveat: `CONTRIBUTING.md` links a `Doc/CodingStandards.md` that doesn't exist, and its
  "target the latest `contrib/vXX.XX` branch" wording looks like stale boilerplate — confirm the
  target branch with maintainers.

## Environment

- **Working repo:** `/d/repos/ecg-maya-usd/maya-usd` (submodule of `ecg-maya-usd`).
- **Branch:** `deboisj/render_pass_spike` **exists but is not checked out** — the tree is on
  `dev` at `983f96372`. Check out the spike branch before starting.
- Only remote is `origin`; there is no `upstream`.
- **USD:** 0.25.11, `PXR_VERSION 2511`. Headers at
  `/d/repos/ecg-maya-usd/Artifactory/Windows/USD/600bfb6/include/pxr/`.
- **Builds and tests run on the host via a relay** — see the repo `CLAUDE.md`. Do not run build
  or test commands directly in the sandbox. Skills `ecg-maya-usd-build` and `run-test` wrap this.
- Line endings: nearly every file reports as modified from the sandbox (CRLF). In the parent repo
  only 3 files differ ignoring whitespace. Use `git diff --ignore-all-space` to see real changes.
  Whether the submodule tree has real edits underneath the noise was never checked.
- The parent repo records submodule pointer `bc5498b3`, which is **not** in the local clone, so
  `git submodule status` reports the submodule as out-of-pointer until fetched.

## Next steps

1. Check out `deboisj/render_pass_spike` — the work was done on `dev` because git access was
   read-only; uncommitted changes carry across the switch.
2. Run the demo in Maya: `import renderPassSpikeDemo; renderPassSpikeDemo.show()`.
3. This *is* the load-bearing smoke test. If no pass filters anything, read the
   `HDVP2_DEBUG_RENDER_PASS` output; suspects in order are: (a) `checkReprefix()` output,
   (b) whether `Attach` found the merging scene index, (c) whether pruning reaches VP2's rprim
   tables — the assumption everything rests on. If (c) is the culprit, the work becomes migrating
   VP2 to a real scene index chain.
4. Clear the three stale `ADSK_USD_DEBUG_TOOLS_*` cache entries, reconfigure, and delete the
   `#if 0` guards in `CompositionEditorCmd.cpp`. Needs the mayapy `jinja2` gap fixed first.

Still not done, deliberately: no automated tests (manual demo was the agreed bar), no AE dropdown
for the attribute, no `cameraVisibility`/`matte`.

Worth reconsidering if the spike graduates: a GoogleTest over the filter and the reprefix, using
the harness at `test/lib/mayaUsd/utils/CMakeLists.txt:52-97` (plain gtest, links `mayaUsd`, no
viewport, already Windows-only). Image-diff tests are the house pattern for VP2
(`testVP2RenderDelegatePrimPath.py`) but are circular for new functionality — the baseline would be
generated by the code under test.
