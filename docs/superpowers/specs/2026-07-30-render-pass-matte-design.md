# Design — USD render pass `matte` in the Maya viewport

**Date:** 2026-07-30
**Status:** design, approved in outline. Not implemented.
**Builds on:** [`../2026-07-30-render-pass-viewport-filter-overview.md`](../2026-07-30-render-pass-viewport-filter-overview.md)

## Goal

Shade prims in a `UsdRenderPass`'s `matte` collection with a flat, unmistakable flag colour in the
VP2 viewport, so an artist can see which prims the pass mattes out.

This is an **authoring aid, not a render preview**. Matte at render time means zero alpha — a
holdout. The viewport has no alpha channel, so any attempt at fidelity would be a flat
background-coloured fill that reads as a broken material. A deliberate flag colour makes no claim
about the final image and is unambiguous.

## Decisions

| Question | Decision |
|---|---|
| Mechanism | **VP2-side flag.** Filter marks the prim; `HdVP2Mesh` overrides the shader. |
| Appearance | **Solid unlit flag colour** (magenta). |
| Scope | **Meshes only.** `basisCurves` and `points` will ignore matte. |
| Colour source | **Hardcoded `kMatteColor` constant.** No optionVar, no attribute. |

Rejected: overlaying `displayColor` (only consulted when no material is bound — `mesh.cpp:1778-1860`
— so it would silently do nothing on any shaded asset), and overriding the material binding to a
synthetic matte material (works, but a whole material network to author for a flat colour, and it
cannot later express holdout or always-on-top behaviour).

## Filter side

Restores what was stripped from the hdPrman/arnold-usd fork. `_RenderPassState` regains `matteExpr`,
`matteEval` and `DoesOverrideMatte`; `_UpdateActiveRenderPassState` compiles the `matte` collection
alongside `prune` and `renderVisibility`. This is close to a straight un-delete.

**Polarity gotcha:** matte is an *include* list — `matteEval->Match(path)` means "this prim **is**
matte". `renderVisibility` is the opposite: matching means visible, non-matching gets hidden. Easy
to get backwards.

For matching geometry prims, `GetPrim` overlays a constant primvar:

```
mayaUsd:matte = true        // "primvars:mayaUsd:matte" in USD terms
```

built with `HdPrimvarSchema::Builder().SetPrimvarValue(...).SetInterpolation(constant)`, overlaid
onto `HdPrimvarsSchema` — the same overlay the `renderVisibility` path already uses for visibility.

Gated by `_IsGeometryType(prim.primType)`, matching Arnold: matte applies to gprims only, not lights
or light filters.

## VP2 side

`HdVP2Mesh` caches the flag during `Sync`:

```cpp
bool _isMatte = false;   // refreshed when DirtyPrimvar is set
```

It must be read **directly from the scene delegate** (`sceneDelegate->Get(GetId(), "mayaUsd:matte")`)
rather than from `_meshSharedData->_primvarInfo`. `_UpdatePrimvarSources` only stores primvars that
pass `_PrimvarIsRequired` (`mesh.cpp:781-789`), which tests membership of `_allRequiredPrimvars` —
a list built from material and repr requirements. A custom primvar is not in it and would be
silently dropped.

The override goes at the **end** of the `desc.geomStyle == HdMeshGeomStyleHull &&
desc.shadingTerminal == HdMeshReprDescTokens->surfaceShader` block in `_UpdateDrawItem`, after both
the material and fallback paths have chosen a shader:

```cpp
// Matte prims are flagged with a flat unlit colour, overriding whatever the
// material or fallback path selected.
if (_isMatte) {
    MHWRender::MShaderInstance* shader = _delegate->Get3dSolidShader(kMatteColor);
    if (shader != nullptr && shader != drawItemData._shader) {
        drawItemData._shader = shader;
        stateToCommit._shader = shader;
        stateToCommit._isTransparent = false;
    }
}
```

Placing it last is what makes it beat a bound material. `Get3dSolidShader` returns the stock
`k3dSolidShader` (`renderDelegate.cpp:349`), unlit flat colour, already used for selection highlight
on hull render items (`mesh.cpp:2159-2168`) — a proven path for flat colour on shaded geometry.

## Invalidation — the risky part

Shader selection sits behind `if (dirtyMaterialId)` at `mesh.cpp:1779`. A matte change that dirties
only primvars would refresh `_isMatte` but never re-pick the shader. Worse, turning matte **off**
must restore the original material shader, which requires that block to re-run.

So when a prim's matte state changes, the filter emits a dirty entry carrying **both** locators:

```cpp
HdDataSourceLocatorSet locators;
locators.insert(HdPrimvarsSchema::GetDefaultLocator());
locators.insert(HdMaterialBindingsSchema::GetDefaultLocator());   // forces DirtyMaterialId
dirtyEntries->push_back({ path, locators });
```

Dirtying material bindings we did not actually change is slightly impure, but it is the least
invasive way to make emulation raise `DirtyMaterialId`. If it misbehaves, the fallback is to
restructure `_UpdateDrawItem` so the matte check runs outside the `dirtyMaterialId` gate with its
own dirty tracking.

This mirrors the existing `visExprDidChange` branch in `_UpdateActiveRenderPassState`, which already
computes per-prim old-vs-new state and emits targeted dirty entries.

## Verification

Manual, matching the bar set for the rest of the spike. `renderPassSpikeDemo.py` gains a matte
collection on a new pass, e.g. `MatteGreen` mattes `/World/GreenGroup//`.

**Toggling matte off is the test that matters**, not toggling it on. Turning it on and seeing
magenta only proves half the path; the silent-no-op risk is that switching to a pass without a matte
collection leaves the geometry stuck magenta. Check both directions, and check a prim that has a
real material bound — the whole point of choosing this mechanism over the `displayColor` overlay.

`HDVP2_DEBUG_RENDER_PASS` gains the compiled matte expression and a count of matte-dirtied prims.

## Out of scope

`basisCurves` and `points` (will ignore matte — worth a line in the overview doc), configurable
colour, holdout/alpha fidelity, and `cameraVisibility`, which still has no renderer-neutral
destination in VP2.
