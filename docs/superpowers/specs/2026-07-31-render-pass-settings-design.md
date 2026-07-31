# Design — render pass `includedPurposes` and camera in the viewport

**Date:** 2026-07-31
**Status:** design. Not implemented.
**Builds on:** [`../2026-07-31-render-pass-summary.md`](../2026-07-31-render-pass-summary.md)

## Goal

Support the two remaining `UsdRenderPass` properties that mean something in a viewport:

- **`includedPurposes`** — the pass decides which USD purposes draw.
- **`camera`** — look through the camera the pass renders from.

Both live on the `UsdRenderSettings` reached through the pass's `renderSource` relationship, not on
the pass itself. Everything else on the schema is either pipeline metadata (`passType`, `command`,
`fileName`, `inputPasses`), meaningless to VP2 (`disableMotionBlur`, `disableDepthOfField`,
`instantaneousShutter`), about AOVs (`products`), or likely to fight Maya's own settings
(`renderingColorSpace`).

## Shared prerequisite: follow `renderSource`

`MayaUsdRenderPassPublisher::Publish` currently reads collections off the pass prim and nothing
else. Both features need it to also resolve `renderSource` to a `UsdRenderSettings` prim on the same
stage.

- If `fileName` is authored the settings live in an external file. **Out of scope** — log and ignore.
- If `renderSource` is absent or unresolvable, both features are simply inactive.
- Neither feature is per-prim, so **neither belongs in the scene index.** They are global pass state
  and live in the publisher / proxy shape layer. `MayaUsdRenderPassSceneIndex` is untouched.

Our demo asset authors no `renderSource`, so it needs a `RenderSettings` prim before any of this is
testable.

## 1. `includedPurposes`

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

## 2. Look through the pass camera

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

Manual, matching the rest of the spike. The demo asset gains a `RenderSettings` prim and a couple of
`UsdGeomCamera`s, plus passes that vary purposes and camera.

1. **Purposes apply** — a pass with `includedPurposes = ["default"]` hides render/proxy/guide
   geometry. Needs prims tagged with non-default purposes, which the demo does not have yet.
2. **Clearing restores** — switching to `NoFilter` brings back exactly the user's toggle state, not
   a default. This is the check that matters; every previous feature here failed on the *clear*
   path, not the apply path.
3. **Unauthored purposes are left alone** — a pass whose `renderSource` never authors
   `includedPurposes` must not disturb the toggles.
4. **Camera** — the command frames the pass camera; a pass without one reports and does nothing.

## Scope

Out: `fileName` external settings, `materialBindingPurposes`, `renderingColorSpace`, `resolution` /
`dataWindowNDC` gate overlays, `products` / AOVs, and any automatic camera following.

`materialBindingPurposes` (`full` vs `preview`) is the most plausible next one, but it reaches into
VP2's material resolution rather than reusing an existing toggle, so it is a different size of job.
