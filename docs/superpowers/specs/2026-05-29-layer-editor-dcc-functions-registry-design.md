# Layer Editor DCC-Functions Registry — Design

Date: 2026-05-29 (reconciled with repo 2026-06-05)
Branch: `feature/unify_layer_editors`
Status: Approved (design)

## Repository context (as of 2026-06-05)

Since this design was first written, the **always-build-both-editors** change
landed (`docs/superpowers/specs/2026-06-04-always-build-both-editors-design.md`).
This makes `MAYAUSD_USE_SHARED_LAYER_EDITOR` always-defined, so `mayaUsdUI`'s
`MayaSessionState` / `MayaCommandHook` now derive from the **shared** base
classes in `lib/usdUfe/usd-layer-editor/lib/` in production. Registering DCC
functions at Maya plugin init (this design) is therefore the real production
seam, not a future hypothetical. MIGRATION.md's note that the bridge is
"deferred" predates this change and is stale.

Two parallel base-class trees now coexist:

- **Shared** (`lib/usdUfe/usd-layer-editor/lib/{sessionState,abstractCommandHook}.h`)
  — what production and the new-editor tests use. **This design touches only
  this tree.**
- **Legacy** (`lib/usd/ui/layerEditor/{sessionState,abstractCommandHook}.h`) —
  compiled only into the `mayaUsdOldLayerEditorTests` parity target, which uses
  its own stubs in `lib/usd/ui/layerEditor/test/cpp/`. **Left untouched.**

## Problem

While unifying the layer editors, DCC-specific behavior was added to two
shared base classes whose intent is something different. The new methods do
not belong on those classes:

- **`AbstractCommandHook`** is meant for the *undoable layer-stack commands*
  the layer editor executes (set edit target, insert/remove/move sublayer,
  mute, lock, flatten, …). It gained:
  - `saveComponent(stage, dccObjectPath)`
  - `reloadComponent(dccObjectPath)`
  - `renameProxyShape(oldDccObjectPath, newName)`
  - `isDccObjectStageIncoming(dccObjectPath)`
  - `isDccObjectSharedStage(dccObjectPath)`

  Saving/reloading a component and renaming a proxy shape are DCC scene
  side-effects, not undoable layer edits. The two `isDccObject*` methods are
  *queries* about scene content, not commands.

- **`SessionState`** is meant for *access to the DCC editing session's scene
  content* (stage list, current stage, selected stages) plus app-specific UI.
  It gained two suites of pure DCC-capability injection:
  - Edit Forwarding: `supportsEditForwarding`, `hasEditForwarding`,
    `echoEditForwarding`, `setEchoEditForwarding`.
  - Component Creator: `isStageAComponent`, `isUnsavedComponent`,
    `shouldDisplayComponentInitialSaveDialog`, `sceneFolder`, `moveComponent`,
    `previewComponentSave`, `getComponentLayersToSave`.

These were attached to whichever base class was nearest rather than a
deliberate injection seam.

## Goal

Introduce a dedicated seam for injecting DCC-specific functions into the
shared layer editor, and move the misplaced hooks onto it. Keep
`AbstractCommandHook` to commands and `SessionState` to scene-content + UI.

In scope to relocate: **Component Creator**, **Edit Forwarding**
(`supportsEditForwarding` / `echoEditForwarding` / `setEchoEditForwarding`), and
**DCC object/stage queries**. The `hasEditForwarding` query is **not** relocated
— it is deleted outright (its only consumer, the EF banner, was removed; it has
no call site in the shared lib today — see Change-notification).

Out of scope (stays on `SessionState`): the layer-display preference options
(`displayLayerContents`, `displayLayerExpandAllValues`,
`displayLayerHideIndices`) — these are genuinely app-UI state.

## Precedent

`lib/usdUfe/ufe/Global.h` already defines a struct named `DCCFunctions`: a bag
of `std::function` pointers (mandatory + optional) that a DCC integration
populates and registers once at `UsdUfe::initialize(dccFunctions, handlers,
…)`. The new mechanism follows the same *style* but lives as its own registry
scoped to the layer editor — it is **not** added to core `ufe/Global.h`,
because these functions are layer-editor / UI-flavored.

## Design

### New registry (shared LE lib)

New files `layerEditorDCCFunctions.{h,cpp}` in
`lib/usdUfe/usd-layer-editor/lib/`, namespace `UsdLayerEditor`, exported with
`LayerEditorAPI`.

Functions are organized into grouped sub-structs by concern, aggregated into
one registry struct:

```cpp
namespace UsdLayerEditor {

// std::function typedefs use the EXACT signatures of today's overrides.
using SaveComponentFn        = std::function<void(const PXR_NS::UsdStageRefPtr&, const std::string&)>;
using ReloadComponentFn      = std::function<void(const std::string&)>;
using RenameProxyShapeFn     = std::function<void(const std::string&, const std::string&)>;
using IsStageAComponentFn    = std::function<bool(const std::string&)>;
using IsUnsavedComponentFn   = std::function<bool(const PXR_NS::UsdStageRefPtr&)>;
using ShouldDisplayComponentInitialSaveDialogFn
                             = std::function<bool(const PXR_NS::UsdStageRefPtr&, const std::string&)>;
using SceneFolderFn          = std::function<std::string()>;
using MoveComponentFn        = std::function<std::string(const std::string&, const std::string&, const std::string&)>;
using PreviewComponentSaveFn = std::function<std::string(const std::string&, const std::string&, const std::string&)>;
using GetComponentLayersToSaveFn = std::function<std::vector<std::string>(const std::string&)>;

using SupportsEditForwardingFn = std::function<bool()>;
using EchoEditForwardingFn     = std::function<bool()>;
using SetEchoEditForwardingFn  = std::function<void(bool)>;

using IsDccObjectStageIncomingFn = std::function<bool(const std::string&)>;
using IsDccObjectSharedStageFn   = std::function<bool(const std::string&)>;

struct ComponentFns {
    SaveComponentFn                          saveComponent;
    ReloadComponentFn                        reloadComponent;
    RenameProxyShapeFn                       renameProxyShape;
    IsStageAComponentFn                      isStageAComponent;
    IsUnsavedComponentFn                     isUnsavedComponent;
    ShouldDisplayComponentInitialSaveDialogFn shouldDisplayComponentInitialSaveDialog;
    SceneFolderFn                            sceneFolder;
    MoveComponentFn                          moveComponent;
    PreviewComponentSaveFn                   previewComponentSave;
    GetComponentLayersToSaveFn               getComponentLayersToSave;
};

struct EditForwardingFns {
    SupportsEditForwardingFn supportsEditForwarding;
    EchoEditForwardingFn     echoEditForwarding;
    SetEchoEditForwardingFn  setEchoEditForwarding;
};

struct DccObjectFns {
    IsDccObjectStageIncomingFn isDccObjectStageIncoming;
    IsDccObjectSharedStageFn   isDccObjectSharedStage;
};

struct LayerEditorDCCFunctions {
    ComponentFns      component;
    EditForwardingFns editForwarding;
    DccObjectFns      dccObject;
};

} // namespace UsdLayerEditor
```

### Registration API — per-group setters

DCC integrations register only the feature groups they actually build (this
plays cleanly with the `#ifdef` guards). One getter for internal use.

```cpp
LayerEditorAPI void setComponentFns(const ComponentFns&);
LayerEditorAPI void setEditForwardingFns(const EditForwardingFns&);
LayerEditorAPI void setDccObjectFns(const DccObjectFns&);

LayerEditorAPI const LayerEditorDCCFunctions& layerEditorDCCFunctions();
```

### Accessor free functions (clean call sites + own the defaults)

Callers never null-check. Thin free functions wrap the null-check and
reproduce today's defaults exactly (`false` / empty / no-op):

```cpp
// Component
LayerEditorAPI void        saveComponent(const PXR_NS::UsdStageRefPtr&, const std::string&); // no-op if unset
LayerEditorAPI void        reloadComponent(const std::string&);                              // no-op
LayerEditorAPI void        renameProxyShape(const std::string&, const std::string&);         // no-op
LayerEditorAPI bool        isStageAComponent(const std::string&);                            // false
LayerEditorAPI bool        isUnsavedComponent(const PXR_NS::UsdStageRefPtr&);                 // false
LayerEditorAPI bool        shouldDisplayComponentInitialSaveDialog(const PXR_NS::UsdStageRefPtr&, const std::string&); // false
LayerEditorAPI std::string sceneFolder();                                                    // {}
LayerEditorAPI std::string moveComponent(const std::string&, const std::string&, const std::string&); // {}
LayerEditorAPI std::string previewComponentSave(const std::string&, const std::string&, const std::string&); // {}
LayerEditorAPI std::vector<std::string> getComponentLayersToSave(const std::string&);        // {}

// Edit Forwarding
LayerEditorAPI bool supportsEditForwarding(); // false
LayerEditorAPI bool echoEditForwarding();     // false
LayerEditorAPI void setEchoEditForwarding(bool); // no-op

// DCC object/stage queries
LayerEditorAPI bool isDccObjectStageIncoming(const std::string&); // false
LayerEditorAPI bool isDccObjectSharedStage(const std::string&);   // true (matches current default)
```

Note: `isDccObjectSharedStage` keeps its current default of **true**; all
others default to false / empty / no-op, matching today's base behavior.

### Change-notification (Edit Forwarding)

`std::function` cannot emit Qt signals, so notification stays with the editor:

- `SessionState::editForwardingChanged()` **remains a Qt signal** on
  `SessionState`. UI eventing belongs with the editor's session.
- `layerEditorWidget` keeps its existing connection
  (`editForwardingChanged` → `updateButtonsOnIdle`, `layerEditorWidget.cpp:359`).
  The slot rebuilds the buttons; it does not read any query off the session, so
  no call-site change is needed here. (The historical EF banner that once read
  `hasEditForwarding()` was removed in favor of a runtime-guarded toggle button,
  which is why `hasEditForwarding` has no remaining consumer.)
- The Maya `SessionState` subclass keeps the logic that emits
  `editForwardingChanged()`; it just no longer overrides the *query* methods.

## Changes to existing classes

### `AbstractCommandHook` (`.../lib/abstractCommandHook.h`)
Remove the misplaced virtuals:
- `saveComponent`, `reloadComponent`, `renameProxyShape`
- `isDccObjectStageIncoming`, `isDccObjectSharedStage`

### `SessionState` (`.../lib/sessionState.h`)
- Remove the 3 Edit-Forwarding query virtuals that move to the registry
  (`supportsEditForwarding`/`echoEditForwarding`/`setEchoEditForwarding`).
- **Delete** the `hasEditForwarding` virtual outright (no registry replacement —
  it has no consumer; see Change-notification). Also drop the stale comment
  block above it that describes the removed banner behavior.
- Remove the 7 Component-Creator virtuals.
- **Keep** the `editForwardingChanged()` signal.
- **Keep** the `displayLayer*` options and their members.

### Call sites (read the registry free functions instead)

These currently call via `_sessionState->...` (SessionState methods) or
`_sessionState->commandHook()->...` (AbstractCommandHook methods); each switches
to the corresponding `UsdLayerEditor::` free function. Line numbers as of
2026-06-05.

- `lib/usdUfe/usd-layer-editor/lib/layerTreeModel.cpp`
  — `isDccObjectSharedStage` (332), `isDccObjectStageIncoming` (347),
  `isStageAComponent` (572), `saveComponent` (573),
  `shouldDisplayComponentInitialSaveDialog` (605), `isUnsavedComponent` (648),
  `reloadComponent` (651)
- `lib/usdUfe/usd-layer-editor/lib/saveLayersDialog.cpp`
  — `shouldDisplayComponentInitialSaveDialog` (485, 532), `moveComponent` (880),
  `renameProxyShape` (891)
- `lib/usdUfe/usd-layer-editor/lib/componentSaveWidget.cpp`
  — `sceneFolder` (76, 233), `previewComponentSave` (340)
- `lib/usdUfe/usd-layer-editor/lib/layerEditorWidget.cpp`
  — `supportsEditForwarding` (123, 223), `echoEditForwarding` (133),
  `setEchoEditForwarding` (the menu-action connect at 131 currently binds the
  `SessionState` slot directly — re-point it at the registry setter),
  `isDccObjectSharedStage` (459), `isStageAComponent` (476),
  `getComponentLayersToSave` (477)

### Maya side
- `lib/usd/ui/layerEditor/mayaCommandHook.{h,cpp}` and
  `lib/usd/ui/layerEditor/mayaSessionState.{h,cpp}`: drop the overrides for the
  moved methods, and **also delete the now-orphaned `hasEditForwarding()`
  override** (`mayaSessionState.h:73` / `.cpp:682`) — the shared virtual is
  gone and nothing calls it.
- Add `registerLayerEditorDCCFunctions()` (and a matching clear on plugin
  unload) at Maya plugin initialization — the natural seam mirroring
  `UsdUfe::initialize`. It builds `ComponentFns` / `EditForwardingFns` /
  `DccObjectFns` whose entries are lambdas wrapping the existing
  `MayaUsd::ComponentUtils` helpers (`isAdskUsdComponent`,
  `isUnsavedAdskUsdComponent`, `shouldDisplayComponentInitialSaveDialog`, …) and
  the `MayaUsdEditForwardHost` / `AdskUsdEditForward` edit-forward helpers, then
  calls the per-group setters. Wrapped in the existing
  `MAYAUSD_USE_SHARED_LAYER_EDITOR` / `WANT_ADSK_USD_EDIT_FORWARD_BUILD` guards
  so a build without those features simply registers nothing and the accessors
  return defaults.
- **Echo state moves off `MayaSessionState`.** Today `echoEditForwarding` /
  `setEchoEditForwarding` are backed by the `_echoEditForwarding` member on
  `MayaSessionState` (seeded from the `LayerEditorEchoEditForwarding` optionVar
  and pushed to the EF host). The registry is registered once at plugin init
  with no session instance, so the lambdas must read/write the optionVar and EF
  host directly; the `_echoEditForwarding` member is removed.

## Testing

Scope note: only the **new-editor** stubs change
(`lib/usdUfe/usd-layer-editor/test/cpp/stub{SessionState,CommandHook}.h`). The
old-editor parity stubs (`lib/usd/ui/layerEditor/test/cpp/stub*.h`) target the
legacy base classes and are **not** touched.

- The new-editor stubs shed the overrides for the moved methods —
  `stubSessionState.h`: `supportsEditForwarding` (+ the `_supportsEditForwarding`
  member); `stubCommandHook.h`: `isDccObjectSharedStage` /
  `isDccObjectStageIncoming` (+ `_isSharedStage` / `_isStageIncoming` members).
- Add a RAII helper `ScopedLayerEditorDCCFunctions` that installs a registry
  state on construction and restores the previous state on destruction, so
  tests that exercise component / EF / DCC-object behavior do not leak global
  state between cases.
- Tests that today poke the stub members switch to the scoped helper:
  - `testEFMode` (`testEFModeLogic.h` sets `_sessionState._supportsEditForwarding`)
    → install an `EditForwardingFns` with `supportsEditForwarding` returning true.
  - `testSharedStage` (`testSharedStageLogic.h` sets
    `_commandHookImpl._isSharedStage` / `_isStageIncoming`) → install a
    `DccObjectFns` returning the desired values.
- Existing layer-editor C++ tests otherwise continue to pass unchanged.

## Error handling

All "not supported" behavior is centralized in the accessor free functions: an
unset `std::function` yields the documented default (false / empty / no-op,
except `isDccObjectSharedStage` → true). No call site branches on support.

## Out of scope / non-goals

- No change to the non-unified Maya layer editor code paths beyond removing the
  moved overrides.
- No change to layer-display preference handling.
- Not folding these functions into core `usdUfe::DCCFunctions`.
