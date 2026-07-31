# Design — USD render pass `cameraVisibility` in the Maya viewport

**Date:** 2026-07-30
**Status:** implemented and verified in Maya. **The Phase 0 assumption below was FALSE** — the
working mechanism is different; see [What actually worked](#what-actually-worked).
**Builds on:** [`../2026-07-30-render-pass-viewport-filter-overview.md`](../2026-07-30-render-pass-viewport-filter-overview.md)

## Goal

Emulate `cameraVisibility` in VP2: prims outside the collection stop being drawn to camera but
keep casting shadows.

## What VP2 can actually offer

`cameraVisibility` means invisible to camera rays, visible to secondary rays. **In VP2 the only
secondary-ray effect that exists is the shadow map** — there is no ray-traced reflection or GI to
participate in. So the achievable emulation is "invisible but still casts shadows". That is the
dominant visible effect in practice, but this is a partial emulation by construction, not a faithful
one.

Second caveat: **VP2 shadows are off by default.** They require enabling in the viewport lighting
menu plus actual lights in the scene. With shadows off, a `cameraVisibility`-hidden prim is
indistinguishable from a `renderVisibility`-hidden one.

## Phase 0 — the load-bearing assumption

**Does Maya's shadow pass include a render item whose `drawMode` matches no active display mode?**

VP2 exposes three relevant levers on `MRenderItem`: `enable(bool)`, `castsShadows(bool)` and
`setDrawMode(DrawMode)` — a four-bit mask (wireframe/shaded/textured/bbox, `MHWGeometry.h:132-145`)
with no explicit "none". Hiding an item from the beauty pass while keeping it in the shadow pass is
only possible if the shadow pass filters on `castsShadows() && isEnabled()` and **ignores**
`drawMode`.

This is undocumented. The devkit headers carry no doc comments on these methods and VP2 is closed
source, so unlike the USD-side questions in this spike it cannot be settled by reading. It needs an
experiment, and **no amount of design structure avoids it** — a dedicated shadow-only render item
still has to be invisible to camera, and `drawMode` is the only lever for that.

**The experiment:** in the demo scene with VP2 shadows enabled and a light present, hardcode one
group's shaded render item to `setDrawMode(MHWRender::MGeometry::DrawMode(0))` while leaving
`castsShadows(true)` and `enable(true)`. If the cubes vanish but their shadow still lands on the
ground plane, the design below is viable. If the shadow vanishes too, **this approach is dead** and
the fallback is to treat `cameraVisibility` as an authoring aid (a second flag colour, as done for
`matte`).

Build and run this before any of the plumbing below.

## What actually worked

**Phase 0 came back negative, and so did four more attempts.** Recorded here so nobody repeats them:

| Attempt | Result |
|---|---|
| `drawMode(0)` | cubes hidden, **shadow also gone** |
| `drawMode(kBoundingBox)` | cubes hidden, **shadow also gone** |
| `drawMode(kShaded\|kTextured)` | cubes visible, shadow present — the control that proved the item, its geometry and `castsShadows(true)` were all fine |
| shape-level `primaryVisibility` | **VP2 ignores it entirely** — it is a batch-render stat, not a viewport flag (`holdOut` likewise) |
| alpha-blended transparent shader | shadow gone; VP2 excludes transparent items from the shadow map |

The rule those establish: **an item must actually be drawn in the current display mode to enter the
shadow pass.** Every `drawMode`-based idea is therefore self-defeating, because invisible and
shadow-casting are the same bit.

**The mechanism that works is `MPxShaderOverride::handlesDraw()`** — the one pass-aware API in VP2.
Its documented contract is that returning `false` declines the pass and hands it back to Maya
(`MPxShaderOverride.h:167-176`), and `MPassContext::passSemantics()` identifies shadow passes via
`kShadowPassSemantic` / `kPointLightShadowPassSemantic` (`MDrawContext.h:259-316`). So the item keeps
a normal `kShaded|kTextured` draw mode — it *is* drawn, hence in the shadow pass — and the override
declines shadow passes while claiming every other one and drawing nothing.

`handlesDraw()` is only consulted for shaders that come from a shading node, and maya-usd assigns
`MShaderInstance` directly, so this required a node type: `ShadowOnlyShader` (`shadowOnlyShader.h`,
type id `0x580000A7`), registered from `plugin.cpp`, attached with `MRenderItem::setShaderFromNode2`.

Two threading constraints, both of which crashed Maya before being fixed:

- Render items are built on TBB worker threads during Hydra sync, so the node cannot be created
  there. `ensureSharedNode()` runs from `ProxyRenderDelegate::_InitRenderDelegate` on the main thread.
- `setShaderFromNode2` links to a DG node, so it is deferred to the commit phase via
  `EnqueueCommit`.

Supersede the `setDrawMode` row in the table below with `kShaded|kTextured`; everything else in the
VP2 side section still holds.

## Filter side

Restores `cameraVisExpr` / `cameraVisEval` / `DoesOverrideCameraVis` from the hdPrman/arnold-usd
fork and compiles the `cameraVisibility` collection.

Polarity matches `renderVisibility`, **not** `matte`: a prim that does *not* match the collection is
the one hidden from camera.

Unlike Arnold, `DoesOverrideCameraVis` does not consult an existing camera-visibility primvar —
Arnold reads back its own `primvars:arnold:visibility:camera`, and there is no VP2 equivalent to
respect. (Arnold's `_IsVisibleToCamera` also has a real bug worth not copying: `renderPassSIP.cpp:92-94`
tests `IsHolding<VtArray<bool>>()` then calls `UncheckedGet<bool>()`.)

For matching geometry prims, `GetPrim` overlays a constant primvar:

```
mayaUsd:cameraInvisible = true
```

It must **not** overlay `HdVisibilitySchema` visibility=0. That is `renderVisibility`'s mechanism and
it would defeat the whole design — see the next section.

## VP2 side

`HdVP2Mesh` caches the flag as `bool _isCameraInvisible`, read straight from the scene delegate under
`DirtyPrimvar`, exactly as `_isMatte` is (`_allRequiredPrimvars` would otherwise drop it).

**Why a dedicated render item rather than reusing the shaded one.** `mesh.cpp:2201` computes
`enable = drawItem->GetVisible() && ...` for every render item on the rprim, so Hydra visibility is
all-or-nothing across items. Hiding the prim the `renderVisibility` way would disable a shadow item
too. Mutating the existing shaded item's `drawMode` instead would work, but that value is
repr-dependent (`kTextured` for `smoothHull`, `kShaded` for `smoothHullUntextured`,
`mesh.cpp:2769-2776`) and would have to be saved and restored across repr switches, along with the
selection mask. A separate item keeps the beauty path untouched.

`_CreateShadowOnlyRenderItem`, modelled on `_CreateSmoothHullRenderItem` (`mesh.cpp:2767-2800`):

| Setting | Value | Why |
|---|---|---|
| type / primitive | `MaterialSceneItem`, `kTriangles` | same geometry as the shaded item |
| `setDrawMode` | `DrawMode(0)` | matches no display mode — the Phase 0 assumption |
| `castsShadows` | `true` | the entire point |
| `receivesShadows` | `false` | it is never seen, so shading it is wasted work |
| `setSelectionMask` | empty `MSelectionMask()` | not visible to camera should mean not pickable |
| `setExcludedFromPostEffects` | `true` | must not contribute to SSAO or similar |
| shader | cheap solid shader | only depth matters for the shadow map |

Enable logic in `_UpdateDrawItem`, in the block at `mesh.cpp:2196-2224`:

- shaded item: `enable = enable && !_isCameraInvisible`
- shadow-only item: `enable = enable && _isCameraInvisible`

Geometry: the shadow item shares the shaded item's position buffer and triangle index buffer via
`setGeometryForRenderItem`; no new buffers are generated.

## Invalidation

Same class of trap as `matte`, different dirty bit. The enable block is gated on
`DirtyVisibility | DirtyRenderTag | DirtyPoints | DirtyExtent | DirtySelectionHighlight`
(`mesh.cpp:2197-2200`) — **`DirtyPrimvar` is not in that list**. A primvar-only change would refresh
`_isCameraInvisible` and never re-evaluate `enable`.

So when cameraVisibility membership changes the filter emits both:

```cpp
locators.insert(HdPrimvarsSchema::GetDefaultLocator());     // refreshes _isCameraInvisible
locators.insert(HdVisibilitySchema::GetDefaultLocator());   // forces DirtyVisibility
```

Dirtying visibility without changing it is safe — `GetVisible()` returns the same value, and it is
what makes the enable block re-run. Note this is the third variant of the same problem: `matte`
needed `DirtyMaterialId`, this needs `DirtyVisibility`, and in both cases the failure mode is
silent and only shows when *clearing* the collection.

## Verification

Manual. The demo script gains a ground plane (shadows need a receiver), a light, and passes
exercising `cameraVisibility`.

The checks that matter, in order:

1. **Shadow survives** — the group vanishes, its shadow does not. This is Phase 0 and everything
   else is moot without it.
2. **Clearing restores** — switching to a pass without the collection brings the geometry back.
3. **Not pickable** — marquee-select over where the hidden geometry was and confirm nothing is
   selected.
4. **Distinct from `renderVisibility`** — a `renderVisibility`-hidden group must lose its shadow, a
   `cameraVisibility`-hidden one must keep it. If both behave identically, the feature is not
   working even if the viewport looks plausible.

## Risks beyond Phase 0

- Sharing geometry buffers across two render items may interact badly with consolidation
  (`VP2RenderDelegateConsolidationTest` exists for a reason) and with the GPU compute paths behind
  `HDVP2_ENABLE_GPU_COMPUTE`.
- Doubling render items for affected prims has a draw-call cost even though only one is enabled.
- Meshes only, as with `matte`. `basisCurves` and `points` already set `castsShadows(false)`
  unconditionally, so they have no shadow to preserve.
