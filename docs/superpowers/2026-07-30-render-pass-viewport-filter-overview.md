# USD render pass filtering in the Maya viewport

**Status:** working spike, verified by hand in Maya. No automated tests.
**Branch:** intended for `deboisj/render_pass_spike`.

Type a `UsdRenderPass` prim path into a proxy shape's new `activeRenderPass` attribute and the
pass's `prune`, `renderVisibility` and `matte` collections change what VP2 draws.

Try it:

```python
import sys; sys.path.append(r'D:\repos\ecg-maya-usd\maya-usd\scripts')
import renderPassSpikeDemo; renderPassSpikeDemo.show()
```

Nine cubes in three colour-coded groups, five passes, each removing a different group. Note it
calls `cmds.file(new=True, force=True)`.

## The scene index chain

VP2 populates through `UsdImagingDelegate`, a Hydra 1.0 scene *delegate*. Hydra wraps that in
"scene index emulation," which gives us a real filtering chain to hook into:

```
UsdImagingDelegate ─► emulation (HdLegacyPrimSceneIndex ─► HdLegacyGeomSubsetSceneIndex)
                          │
                          ├──────────────────┐
                          │                  ├─► our merging ─► HdsiSceneGlobalsSceneIndex
   retained (pass prim) ──┘                  │        └─► MayaUsdRenderPassSceneIndex
                                             │                   │
                          render index's merging ◄───────────────┘
                                             └─► terminal ─► adapter delegate ─► VP2
```

Four moving parts:

| Part | Job |
|---|---|
| `renderPassSceneIndex.{h,cpp}` | The filter. Drops pruned prims from `GetPrim`/`GetChildPrimPaths`; overlays `visibility=0` on prims outside `renderVisibility`; overlays a constant `mayaUsd:matte` primvar on prims inside `matte`. |
| `renderPassPublisher.{h,cpp}` | `Attach()` splices the filter into the render index. `Publish()` reads the pass's collections off the USD stage and feeds them in. |
| `mesh.{h,cpp}` | Reads the matte flag and overrides the shader with a flat unlit colour. |
| `proxyShapeBase.{h,cpp}` | The `activeRenderPass` / `arp` string attribute and its version counter. |
| `proxyRenderDelegate.{h,cpp}` | Calls `Attach()` after render index creation; re-publishes when the version changes. |

**Button press → pixels:** `setAttr` → `setDependentsDirty` bumps a version counter and calls
`setGeometryDrawDirty` → next `update()` sees the version mismatch → `Publish()` re-reads the
collections and updates the retained prim → the filter recomputes and emits prim add/remove and
visibility-dirty notices → adapter delegate → VP2's rprim tables.

## Two things worth knowing

**1. How the filter is attached — this is the non-obvious part.**

The documented hook, `RegisterSceneIndexForRenderer`'s append callback, **silently never runs for
VP2**. `renderIndex.cpp:218-227` gates it on a non-empty renderer display name, and that name is
only set by `HdRendererPlugin::CreateDelegate`. `HdVP2RenderDelegate` is constructed directly, so
it's `""` and Hydra skips the call entirely. The registry's "empty name applies to all renderers"
rule governs *registration*, not *invocation*.

So `Attach()` instead does what 3ds Max USD does for its light gizmos: walk down from
`GetTerminalSceneIndex()` to the first `HdMergingSceneIndex`, take its single input (the emulation
chain), **remove it**, then re-insert the filter wrapping it. Removing first is the whole trick —
`InsertSceneIndex` alone makes the filter a *sibling* of emulation, which can't see legacy prims.
It runs while the scene is still empty (before `_Populate()`), so nothing needs re-notifying.

**2. Collection expressions are rebased onto the delegate prefix.**

`UsdImagingDelegate` prefixes every prim it emits with its delegate ID, so `/World/Sphere` is
`/Proxy_pShape1_0xABCD/World/Sphere` in the Hydra scene. Expressions authored against raw USD paths
match *nothing* without `SdfPathExpression::ReplacePrefix`. The failure mode is silent — the
viewport just looks unfiltered — so `renderPassSpikeDemo.checkReprefix()` tests it independently.

## Provenance and scope

The filter is a fork of `arnold-usd`'s `renderPassSIP.cpp`, which is itself a modified copy of
**hdPrman's** render pass scene index. Pixar copyright, OpenUSD license, compatible with maya-usd.
The valuable inherited part is `_UpdateActiveRenderPassState` — the old-vs-new expression diff and
the traversal that emits the right invalidation. It is unmodified.

This corrects an assumption in the original notes: a `renderVisibility` filter was expected to be
the novel, upstreamable artifact because USD ships none in `hdsi`. hdPrman has had one all along
and it writes standard `HdVisibilitySchema`, not renderer-specific data. Nothing here is novel
enough to upstream.

Dropped: `cameraVisibility`. Arnold implements it by writing `primvars:arnold:visibility:camera`;
there is no renderer-neutral Hydra schema and no VP2 concept to map it onto.

`matte` is supported, but not the way Arnold does it. Arnold writes `primvars:arnold:matte` and
lets the renderer produce a zero-alpha holdout. VP2 has no such concept and no alpha channel, so
instead the filter flags matte geometry with a constant `mayaUsd:matte` primvar and `HdVP2Mesh`
shades it flat magenta. **This is an authoring aid, not a render preview** — it shows which prims
the pass mattes, and deliberately makes no claim about the final image. See
[`specs/2026-07-30-render-pass-matte-design.md`](specs/2026-07-30-render-pass-matte-design.md).

## Caveats

- **Only `membershipExpression` collections work.** `HdCollectionSchema` carries nothing else, so
  `includes`/`excludes` are ignored. Same limitation as hdPrman's filter.
- **Matte applies to meshes only.** `basisCurves` and `points` silently ignore it; the override
  lives in `HdVP2Mesh::_UpdateDrawItem` and would have to be repeated in each.
- **Adding a collection means touching three places.** The publisher must read it from USD, the
  filter must compile and apply it, and (for matte) VP2 must act on it. Missing the publisher is
  what broke matte on the first attempt, and it fails silently: a pass whose only collection is
  unpublished ends up with no data source at all and is treated as no active pass.
- **`prune` and `renderVisibility` look identical on screen but aren't.** Prune destroys and
  rebuilds rprims on every pass edit and removes prims from viewport selection entirely;
  `renderVisibility` is just a dirty bit. The demo passes are arranged so each removes a *different*
  group, which is the only reliable way to tell them apart visually.
- **Every pass change walks the whole scene** (`HdSceneIndexPrimView` over all input prims), and
  switching between two passes currently does it twice — once when the old pass prim is removed,
  once when the new one is activated. Fine at demo scale, worth revisiting for real scenes.
- **`lib/usd/ui/debugTools/CompositionEditorCmd.cpp` has three methods wrapped in `#if 0`.**
  Unrelated to this work and pre-existing. The cause is a stale CMake cache pointing
  `ADSK_USD_DEBUG_TOOLS_INCLUDE_DIR` at `adskusddebugtool/b190bae` while the build passes
  `a503391`; the newer artifact declares the virtuals and the source is correct. Fixing it needs a
  clean reconfigure, currently blocked by mayapy missing `jinja2`. **Revert those guards** once
  that's sorted.
- **No tests.** Manual demo was the agreed bar. If this graduates, the two things worth testing are
  the expression rebasing and the filter's invalidation diff — both are provable with plain
  GoogleTest, no Maya or GPU, using the harness at `test/lib/mayaUsd/utils/CMakeLists.txt:52-97`.
  Image-diff tests are the house pattern for VP2 but are circular for new functionality: the
  baseline would be generated by the code under test.

## Debugging

```python
from pxr import Tf
Tf.Debug.SetDebugSymbolsByName('HDVP2_DEBUG_RENDER_PASS', True)   # before the first draw
```

Traces attachment, publish (**authored vs. rebased** expressions), and filter state (active pass,
compiled expressions, counts of prims pruned / re-added / visibility-dirtied).

## What this validated

The spike's load-bearing question was whether pruning at a scene index actually removes rprims from
VP2's tables, given VP2 has no scene index chain of its own. **It does.** Emulation is sufficient —
VP2 does not need migrating to Hydra 2.0 for this class of feature.

Still out of scope: authoring UI for the attribute (no AE dropdown), per-proxy-shape behaviour with
multiple stages beyond the single-shape demo, and animation/time-varying collections.
