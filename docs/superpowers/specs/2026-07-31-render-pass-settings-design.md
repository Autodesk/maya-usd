# Design — render pass camera in the viewport

**Date:** 2026-07-31
**Status:** camera implemented in the demo script (Python only, no C++).
**`includedPurposes` was considered and declined — see below.**
**Builds on:** [`../2026-07-31-render-pass-summary.md`](../2026-07-31-render-pass-summary.md)

## Goal

Look through the camera a `UsdRenderPass` renders from.

## Not doing: `includedPurposes`

Originally specced here alongside the camera, and **dropped as unwanted for the viewport.** The
design is left below for the record rather than deleted, since the analysis stands if anyone
revisits it.

Two things worth keeping from it either way. First, `includedPurposes` defaults to
`["default", "render"]`, so a naive implementation would silently switch proxy and guide off for any
pass authoring a `renderSource` — any future attempt should gate on `HasAuthoredValue()`. Second,
`HdRenderTagTokens->geometry` currently returns unconditional `true` in `DrawRenderTag`, so
supporting the `default` purpose would mean changing existing behaviour, not just adding to it.

Both lived on the `UsdRenderSettings` reached through the pass's `renderSource` relationship, not on
the pass itself. Everything else on the schema is either pipeline metadata (`passType`, `command`,
`fileName`, `inputPasses`), meaningless to VP2 (`disableMotionBlur`, `disableDepthOfField`,
`instantaneousShutter`), about AOVs (`products`), or likely to fight Maya's own settings
(`renderingColorSpace`).

## Prerequisite: follow `renderSource`

The camera is not on the pass prim; it is on a `UsdRenderSettings` reached through `renderSource`,
so resolving it means reading a second prim.

- If `fileName` is authored the settings live in an external file. **Out of scope** — report and
  ignore.
- If `renderSource` is absent or unresolvable, the feature is simply inactive.
- This is global pass state, not per-prim, so **it does not belong in the scene index.**
  `MayaUsdRenderPassSceneIndex` and `MayaUsdRenderPassPublisher` are both untouched — the demo
  script resolves it directly off the stage.

## DECLINED — `includedPurposes` (kept for the record)

**Decision: the pass overrides while active; clearing it restores the user's toggles.** That matches
what a render pass means and is consistent with `prune` and `renderVisibility`, which already
override rather than negotiate.

**The gotcha to design around:** `includedPurposes` defaults to `["default", "render"]`. So a pass
that authors a `renderSource` but never thinks about purposes will still switch **proxy and guide
off**. That is arguably correct, but it will look like a bug the first time it happens, so:

- Only override when `includedPurposes` is **authored** (`HasAuthoredValue()`), not when it is
  merely defaulted. This keeps the override intentional.
- Log the effective purposes under `HDVP2_DEBUG_RENDER_PASS`.

### Where it hooks in

`ProxyRenderDelegate::DrawRenderTag` (`proxyRenderDelegate.cpp:2297`) is a single function mapping
render tags to the proxy shape's toggles:

```cpp
} else if (renderTag == HdRenderTagTokens->render) {
    return _proxyShapeData->DrawRenderPurpose();
```

The override goes here — when a pass supplies purposes, they answer instead of the toggles.
`HdRenderTagTokens->geometry` maps to the `default` purpose and currently returns an unconditional
`true`; it must become conditional too, since a pass can exclude `default`.

Invalidation reuses what already exists: `UpdatePurpose` (`:1946`) diffs the purposes, builds
`changedRenderTags` and dirties them. The effective-purpose computation moves into `UpdatePurpose`
so a pass change flows through the same path as a user toggling a checkbox — no new invalidation
mechanism, which matters given how often that has been the failure point in this feature.

### Mapping

| `includedPurposes` token | Render tag |
|---|---|
| `default` | `HdRenderTagTokens->geometry` |
| `render` | `HdRenderTagTokens->render` |
| `proxy` | `HdRenderTagTokens->proxy` |
| `guide` | `HdRenderTagTokens->guide` |

## Look through the pass camera

**Decision: explicit action, never automatic.** Moving someone's viewport as a side effect of
setting an attribute is hostile and has no obvious undo. An explicit command also stays sensible
when the pass has no camera.

**This is far cheaper than it first appears.** maya-usd already ships `ProxyShapeCameraHandler`, a
`Ufe::CameraHandler`, and `cmds.lookThru` already accepts a UFE path — there is a passing test doing
exactly this:

```python
cmds.lookThru('|stage|stageShape,/cam')     # testVP2RenderDelegateUsdCamera.py:74
```

So no new look-through machinery is needed. The work is:

1. Resolve `renderSource` → `UsdRenderSettings` → `camera` relationship → prim path.
2. Build the UFE path: `<proxy shape DAG path>,<prim path>`.
3. Call `lookThru`.

A Python command on the proxy shape is enough for the spike — no C++ required. It can live beside
the demo script initially, and become a real command or menu item if it graduates.

Failure modes, all just report and do nothing: no `renderSource`, no `camera` relationship, target
prim missing, or target is not a `UsdGeomCamera`.

## Verification

Manual, matching the rest of the spike. The demo asset gains three `UsdGeomCamera`s under
`/Cameras` and three passes that author `renderSource` to a `RenderSettings` prim naming one.

1. **Camera** — `CamShotRed` / `CamShotShaded` / `CamShotWide` plus the Look through pass camera
   button frame the expected group.
2. **Switching a pass moves nothing on its own** — the viewport only changes when the button is
   pressed. This is the design decision, so it is worth confirming rather than assuming.
3. **Graceful failure** — a pass with no `renderSource`, or one resolving no camera, reports and
   does nothing.

## Scope

Out: `includedPurposes` (declined), `fileName` external settings, `materialBindingPurposes`,
`renderingColorSpace`, `resolution` / `dataWindowNDC` gate overlays, `products` / AOVs, and any
automatic camera following.

`materialBindingPurposes` (`full` vs `preview`) is the most plausible next one, but it reaches into
VP2's material resolution rather than reusing an existing toggle, so it is a different size of job.
