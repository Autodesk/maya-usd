# USD render pass collections in the Maya viewport — summary

**Status:** working spike, all four collections verified by hand in Maya. No automated tests.
**Branch:** `deboisj/render_pass_spike`

Type a `UsdRenderPass` prim path into the proxy shape's new `activeRenderPass` attribute and its
`prune`, `renderVisibility`, `matte` and `cameraVisibility` collections change what VP2 draws.

```python
import sys; sys.path.append(r'D:\repos\ecg-maya-usd\maya-usd\scripts')
import renderPassSpikeDemo; renderPassSpikeDemo.show()
```

## Architecture

VP2 populates through `UsdImagingDelegate`, a Hydra 1.0 scene *delegate*. Hydra's scene index
emulation gives us a real filtering chain to splice into:

```
UsdImagingDelegate ─► emulation ─┐
                                 ├─► merging ─► sceneGlobals ─► MayaUsdRenderPassSceneIndex
    retained (synthetic pass) ───┘                                      │
                                 render index's merging ◄───────────────┘
                                      └─► terminal ─► adapter delegate ─► VP2
```

| Component | Job |
|---|---|
| `renderPassSceneIndex.{h,cpp}` | The filter. Fork of `arnold-usd`'s `renderPassSIP.cpp`, itself a copy of **hdPrman's**. Invalidation logic unmodified. |
| `renderPassPublisher.{h,cpp}` | `Attach()` splices the filter in; `Publish()` reads the pass's collections off the USD stage and feeds them in. |
| `mesh.{h,cpp}`, `drawItem.h` | Consumes the `matte` and `cameraVisibility` flags. |
| `shadowOnlyShader.{h,cpp}` | Shading node + `MPxShaderOverride` backing `cameraVisibility`. |
| `proxyShapeBase.{h,cpp}` | The `activeRenderPass` / `arp` attribute and version counter. |

## The four collections

| Collection | Polarity | Mechanism | Looks like |
|---|---|---|---|
| `prune` | matching = removed | Dropped from `GetPrim` / `GetChildPrimPaths` | Gone, and out of the Hydra scene entirely — also unselectable |
| `renderVisibility` | **non**-matching = hidden | Overlay `HdVisibilitySchema` `visibility=0` | Gone, shadow gone too |
| `matte` | matching = matte | Overlay constant `mayaUsd:matte`; `HdVP2Mesh` swaps in `Get3dSolidShader` | Flat magenta |
| `cameraVisibility` | **non**-matching = hidden | Overlay `mayaUsd:cameraInvisible`; shadow-only render item | Gone, **shadow remains** |

`matte`'s polarity is inverted relative to the two visibility collections — easy to get backwards in
near-copy-paste code.

`matte` is an **authoring aid, not a render preview**: matte means zero alpha at render time and the
viewport has no alpha channel, so a deliberate flag colour beats a fake holdout.

## Three non-obvious mechanisms

**1. How the filter attaches.** The documented hook — the append callback from
`RegisterSceneIndexForRenderer` — *silently never runs for VP2*. `renderIndex.cpp:218-227` gates it
on a non-empty renderer display name, which is only set for delegates built by
`HdRendererPluginRegistry`; `HdVP2RenderDelegate` is constructed directly. Instead `Attach()` walks
from `GetTerminalSceneIndex()` to the first `HdMergingSceneIndex`, **removes** its emulation input,
and re-inserts that input wrapped in the filter. Removing first is essential — inserting alone makes
the filter a *sibling* that cannot see legacy prims. Same pattern 3ds Max USD uses.

**2. Collection expressions are rebased.** `UsdImagingDelegate` prefixes every prim with its
delegate ID, so `/World/Sphere` is `/Proxy_pShape1_0xABCD/World/Sphere` in Hydra. Raw USD-path
expressions match **nothing** without `SdfPathExpression::ReplacePrefix`. The failure is silent —
`renderPassSpikeDemo.checkReprefix()` tests it independently.

**3. Each collection needs a different dirty bit.** Three variants of the same trap, all of which
fail *silently* and only when **clearing** a collection:

| Collection | Must dirty | Why |
|---|---|---|
| `prune` / `renderVisibility` | its own locator | normal path |
| `matte` | + `HdMaterialBindingsSchema` | shader selection is gated on `DirtyMaterialId` |
| `cameraVisibility` | + `HdVisibilitySchema` | the enable block keys off `DirtyVisibility`, not `DirtyPrimvar` |

## cameraVisibility: what does *not* work

VP2 has no per-render-item "invisible to camera" flag. Five candidates were tried and rejected:

| Attempt | Result |
|---|---|
| `drawMode(0)` | hidden, **shadow also gone** |
| `drawMode(kBoundingBox)` | hidden, **shadow also gone** |
| `drawMode(kShaded\|kTextured)` | visible + shadow — the control proving the item and `castsShadows` were fine |
| `primaryVisibility`, `holdOut` | **VP2 ignores both** — batch-render stats, not viewport flags |
| alpha-blended transparent shader | shadow gone; VP2 excludes transparent items from the shadow map |

The rule: **an item must be drawn in the current display mode to enter the shadow pass**, so every
`drawMode`-based idea is self-defeating.

**What works:** `MPxShaderOverride::handlesDraw()`, the one pass-aware API — returning `false`
declines a pass and hands it back to Maya (`MPxShaderOverride.h:167-176`), and
`MPassContext::passSemantics()` identifies shadow passes. So the shadow-only item keeps a normal
shaded draw mode (it *is* drawn, hence in the shadow pass) and its override declines shadow
semantics while claiming every other pass and drawing nothing. `handlesDraw` is only consulted for
shaders assigned from a shading node, hence `ShadowOnlyShader` (type id `0x580000A7`) and
`setShaderFromNode2`.

Two threading constraints, both of which crashed Maya first: render items are built on TBB worker
threads during Hydra sync, so the node is created from `_InitRenderDelegate` on the main thread, and
`setShaderFromNode2` links to a DG node so it is deferred to the commit phase.

## Caveats

- **Only `membershipExpression` collections work.** `HdCollectionSchema` carries nothing else, so
  `includes`/`excludes` are silently ignored. Same limitation as hdPrman's filter.
- **`matte` and `cameraVisibility` are meshes only.** `basisCurves`/`points` ignore them.
- **VP2 shadows are off by default** — with them off, `cameraVisibility` looks like
  `renderVisibility`.
- **Adding a collection means touching three places** — publisher, filter, and (for the flag-based
  ones) VP2. Missing the publisher is what broke `matte` first time, and it fails silently: a pass
  whose only collection is unpublished gets no data source and reads as "no active pass".
- Every pass change walks the whole scene, twice when switching between two passes.
- Nothing here is novel enough to upstream: `renderVisibility` was expected to be the contribution,
  but hdPrman has shipped one all along that writes standard `HdVisibilitySchema`.

## Temporary changes to revert

1. **`CompositionEditorCmd.cpp`** — three methods in `#if 0`. Pre-existing breakage: the CMake cache
   has a stale `ADSK_USD_DEBUG_TOOLS_INCLUDE_DIR` pointing at `adskusddebugtool/b190bae` while the
   build passes `a503391`. Confirmed still stale after a clean reconfigure.
2. **`lib/usd/schemas/CMakeLists.txt`** — `FATAL_ERROR` → `WARNING`. **Only safe because the
   generated sources already exist in this build tree**; a clean tree will fail at compile.

Both trace to `usdGenSchema` needing `jinja2`, absent from this mayapy. One command fixes both:
`mayapy.exe -m pip install jinja2`. (The matching change in `test/lib/mayaUsd/fileio` is a genuine
improvement worth keeping — a missing *test* dependency should skip that test, not fail configure.)

## Debugging

```python
from pxr import Tf
Tf.Debug.SetDebugSymbolsByName('HDVP2_DEBUG_RENDER_PASS', True)   # before first draw
```

Traces attachment, publish (**authored vs. rebased** expressions), filter state, per-prim dirty
counts, and `handlesDraw` pass semantics. On Windows this goes to the **Output Window**, not the
Script Editor.

## Commits

```
4244b840e  Support render pass cameraVisibility in the viewport
caa34077f  Do not fail configure when usdGenSchema is unavailable
2e84cf587  Design for cameraVisibility
9b0f94b48  Flag render pass matte prims with a solid colour
ea35055a5  Design for matte
626e875b0  Temporarily disable CompositionEditorCmd overrides
a8c5cf785  Add USD render pass filtering to the VP2 viewport
```

Longer records: [`2026-07-30-render-pass-viewport-filter-overview.md`](2026-07-30-render-pass-viewport-filter-overview.md),
the handoff, and the two specs under `specs/`.
