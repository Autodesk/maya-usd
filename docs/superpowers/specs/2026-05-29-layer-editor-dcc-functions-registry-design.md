# Layer Editor DCC-Functions Registry — Design

Date: 2026-05-29
Branch: `feature/unify_layer_editors`
Status: Approved (design)

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

In scope to relocate: **Component Creator**, **Edit Forwarding** (query
functions only — see Notification), and **DCC object/stage queries**.

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
using HasEditForwardingFn      = std::function<bool()>;
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
    HasEditForwardingFn      hasEditForwarding;
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
LayerEditorAPI bool hasEditForwarding();      // false
LayerEditorAPI bool echoEditForwarding();     // false
LayerEditorAPI void setEchoEditForwarding(bool); // no-op

// DCC object/stage queries
LayerEditorAPI bool isDccObjectStageIncoming(const std::string&); // false
LayerEditorAPI bool isDccObjectSharedStage(const std::string&);   // true (matches current default)
```

Note: `isDccObjectSharedStage` keeps its current default of **true**; all
others default to false / empty / no-op, matching today's base behavior.

### Change-notification (Edit Forwarding banner)

`std::function` cannot emit Qt signals, so notification stays with the editor:

- `SessionState::editForwardingChanged()` **remains a Qt signal** on
  `SessionState`. UI eventing belongs with the editor's session.
- `layerEditorWidget` keeps its existing connection to that signal; on fire it
  reads the current value via `UsdLayerEditor::hasEditForwarding()` from the
  registry (instead of `_sessionState.hasEditForwarding()`).
- The Maya `SessionState` subclass keeps the logic that emits
  `editForwardingChanged()`; it just no longer overrides the *query* methods.

## Changes to existing classes

### `AbstractCommandHook` (`.../lib/abstractCommandHook.h`)
Remove the misplaced virtuals:
- `saveComponent`, `reloadComponent`, `renameProxyShape`
- `isDccObjectStageIncoming`, `isDccObjectSharedStage`

### `SessionState` (`.../lib/sessionState.h`)
- Remove the 4 Edit-Forwarding query virtuals
  (`supportsEditForwarding`/`hasEditForwarding`/`echoEditForwarding`/`setEchoEditForwarding`).
- Remove the 7 Component-Creator virtuals.
- **Keep** the `editForwardingChanged()` signal.
- **Keep** the `displayLayer*` options and their members.

### Call sites (read the registry free functions instead)
- `lib/usdUfe/usd-layer-editor/lib/layerTreeModel.cpp`
  (`saveComponent`, `reloadComponent`, `isStageAComponent`,
  `shouldDisplayComponentInitialSaveDialog`, `isUnsavedComponent`)
- `lib/usdUfe/usd-layer-editor/lib/saveLayersDialog.cpp`
  (`shouldDisplayComponentInitialSaveDialog`, `moveComponent`, `renameProxyShape`)
- `lib/usdUfe/usd-layer-editor/lib/componentSaveWidget.cpp`
  (`sceneFolder`, `previewComponentSave`)
- `lib/usdUfe/usd-layer-editor/lib/layerEditorWidget.cpp`
  (`supportsEditForwarding`, `echoEditForwarding`, `setEchoEditForwarding`,
  `hasEditForwarding`, `isStageAComponent`, `getComponentLayersToSave`)

### Maya side
- `lib/usd/ui/layerEditor/mayaCommandHook.{h,cpp}` and
  `lib/usd/ui/layerEditor/mayaSessionState.{h,cpp}`: drop the overrides for the
  moved methods.
- Add `registerLayerEditorDCCFunctions()` (and a matching clear on plugin
  unload) at Maya plugin initialization — the natural seam mirroring
  `UsdUfe::initialize`. It builds `ComponentFns` / `EditForwardingFns` /
  `DccObjectFns` whose entries are lambdas wrapping the existing
  `MayaUsd::ComponentUtils` and edit-forward helpers, then calls the per-group
  setters. Wrapped in the existing `MAYAUSD_USE_SHARED_LAYER_EDITOR` /
  `WANT_ADSK_USD_EDIT_FORWARD_BUILD` guards so a build without those features
  simply registers nothing and the accessors return defaults.

## Testing

- The C++ test stubs (`test/cpp/stubSessionState.h`, `test/cpp/stubCommandHook.h`)
  shed the overrides for the moved methods.
- Add a RAII helper `ScopedLayerEditorDCCFunctions` that installs a registry
  state on construction and restores the previous state on destruction, so
  tests that exercise component / EF behavior do not leak global state between
  cases.
- Existing layer-editor C++ tests continue to pass; tests that relied on the
  stub overrides switch to registering test functions via the scoped helper.

## Error handling

All "not supported" behavior is centralized in the accessor free functions: an
unset `std::function` yields the documented default (false / empty / no-op,
except `isDccObjectSharedStage` → true). No call site branches on support.

## Out of scope / non-goals

- No change to the non-unified Maya layer editor code paths beyond removing the
  moved overrides.
- No change to layer-display preference handling.
- Not folding these functions into core `usdUfe::DCCFunctions`.
