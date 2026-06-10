# AI Slop Comment Review — `lib/usdLayerEditor/lib/`

Review of comments in the shared layer editor that (a) do not exist in **any** predecessor
file (checked: `lib/usd/ui/layerEditor/`, `lib/mayaUsd/commands/layerEditorCommand.cpp/.h`,
`lib/mayaUsd/utils/layerLocking.cpp/.h`, `lib/mayaUsd/utils/layerMuting.cpp/.h`,
`lib/mayaUsd/utils/utilFileSystem.h`, `lib/mayaUsd/utils/customLayerData.h`,
`lib/usd/ui/layerEditor/qtUtils.h/.cpp`) and (b) are needlessly verbose or restate what
the code already says. Logic discrepancies are documented at the end.

Each entry has a blank **decision:** field for you to fill in.

> **Note on methodology errors:** The first pass flagged several comments that actually
> exist verbatim in their predecessor files and are therefore NOT AI slop. Those have
> been removed. A separate section at the bottom documents typo regressions and comment
> downgrades found during verification.

---

## Files with no new slop comments

- `abstractCommandHook.h`
- `batchSaveLayersUIDelegate.h` / `.cpp`
- `componentSaveWidget.h` / `.cpp`
- `customLayerData.h` *(all four doc blocks exist verbatim in `mayaUsd/utils/customLayerData.h`)*
- `dirtyLayersCountBadge.h` / `.cpp`
- `generatedIconButton.h` / `.cpp`
- `layerContentsWidget.h`
- `layerEditorCommands.cpp` *(all flagged comments exist verbatim in `mayaUsd/commands/layerEditorCommand.cpp`)*
- `layerEditorWidget.h`
- `layerLocking.h` — briefs on accessors, `lockLayer` brief, static-init comments *(all exist in `mayaUsd/utils/layerLocking.h/.cpp`)*
- `layerMuting.cpp` — "Use a set to accelerate", "Kept in a function", C++ thread-safety note *(all exist in `mayaUsd/utils/layerMuting.cpp`)*
- `layerTreeItem.h` / `.cpp`
- `layerTreeItemDelegate.h`
- `LayerEditorCommands.h` — most class/member comments *(ported from `layerEditorCommand.cpp`)*
- `loadLayersDialog.h` / `.cpp`
- `pathChecker.h` / `.cpp`
- `stageSelectorWidget.h`
- `usdSyntaxHighlighter.h` / `.cpp`
- `utilFileSystem.h` — path-function doc blocks, resolvePath, getDir *(all exist in `mayaUsd/utils/utilFileSystem.h`)*
- `utilFileSystem.cpp` — "Find the layer entry", "Update sublayer paths", "Update references", "Erase the layer entry" *(all exist in `mayaUsd/utils/utilFileSystem.cpp`)*
- `utilQT.h` — `QtDisableRepaintUpdates` doc *(exists in `qtUtils.h`)*
- `utilQT.cpp` — "overkill, but used to generate the grayed out version", "returns the widget after setting it fixed-size" *(both exist in `qtUtils.cpp` / `qtUtils.h`)*
- `warningDialogs.h` / `.cpp`

---

## `layerEditorWidget.cpp`

### L481–483: extra sentence appended to the component-creator block comment

The original ends at "...ask it what layers will be impacted." The sentence that follows
was added and restates what the `else { ... }` branch immediately below already shows.

**Old (`lib/usd/ui/layerEditor/layerEditorWidget.cpp:478`):**
```cpp
// Special case for components created by the component creator. Non-local layers,
// non-active layers, and non-dirty but to be renamed layers, can be impacted when saving a
// component. Only the component creator knows how to save a component properly, we need to
// ask it what layers will be impacted.
if (MayaUsd::ComponentUtils::isAdskUsdComponent(...))
```

**New (`lib/usdLayerEditor/lib/layerEditorWidget.cpp:478`):**
```cpp
// Special case for components created by the component creator. Non-local layers,
// non-active layers, and non-dirty but to be renamed layers, can be impacted when
// saving a component. Only the component creator knows how to save a component
// properly, we need to ask it what layers will be impacted. The hook returns an
// empty vector for DCCs without component support; that case falls through to the
// normal counting below.
if (UsdLayerEditor::isStageAComponent(...))
```

Added sentence: "The hook returns an empty vector for DCCs without component support;
that case falls through to the normal counting below." — describes the `else` block that
follows.

**decision:** use old comment

---

### L418: `// The DCC integration registers how to open its EF configuration dialog.`

Inside `openEditForwardDialog()`, which is a one-liner. The DCC registry pattern is
used pervasively; this comment explains nothing a reader could not deduce. Borderline.

```cpp
void LayerEditorWidget::openEditForwardDialog()
{
    // The DCC integration registers how to open its EF configuration dialog.
    UsdLayerEditor::openEditForwardDialog(_sessionState.stage());
}
```

**decision:** Do nothing.

---

## `layerTreeItemDelegate.cpp`

### L96: `// Detect if the UI is in light mode - and adjust some colors accordingly.`

The light-mode block did not exist in the original file, so there is no predecessor
comment to compare against. The comment restates `if (utils->lightTheme())` on the
next line.

```cpp
    // Detect if the UI is in light mode - and adjust some colors accordingly.
    if (utils->lightTheme()) {
        DISABLED_BACKGROUND_IMAGE = utils->lightPixmap(DISABLED_BACKGROUND_IMAGE, 2.8f);
        ...
    }
```

**decision:** do nothing.

---

## `layerTreeView.cpp`

### L383: `// Initial state`

Labels the `else { expandAll(); }` branch of a two-branch conditional. Not in original.

```cpp
    if (_cachedModelState)
        _cachedModelState->restore(*this, *_model);
    else
        // Initial state
        expandAll();
```

**Old (`lib/usd/ui/layerEditor/layerTreeView.cpp:398`):**
```cpp
    } else {
        expandAll();
    }
```

**decision:** do nothing

---

## `stageSelectorWidget.cpp`

### L178: `// Remove observer - if not observing this is a no-op.`

`std::set::erase` on a missing key is a no-op by definition. Not in original.

```cpp
StageSelectorWidget::~StageSelectorWidget()
{
    // Remove observer - if not observing this is a no-op.
    StageSelectorSelectionObserver::instance()->removeStageSelector(*this);
}
```

**Old (`lib/usd/ui/layerEditor/stageSelectorWidget.cpp:189`):**
```cpp
    StageSelectorSelectionObserver::instance()->removeStageSelector(*this);
```

**decision:** remove

---

## `stringResources.h`

### L37/L40: Hallucinated type name `std::wstringResourceId`

The original had comments referencing `MStringResourceId`. When ported, the type was
correctly changed to `Resource`, but the comments were rewritten with a type name that
does not exist.

**Old (`lib/usd/ui/layerEditor/stringResources.h:35`):**
```cpp
// Retrive a string resource from the given MStringResourceId.
MString getAsMString(const MStringResourceId& strResID);

// Retrive a string resource from the given MStringResourceId.
QString getAsQString(const MStringResourceId& strResID);

// create a MStringResourceId, must be called before registerAll()
MStringResourceId create(const char* key, const char* value);
```

**New (`lib/usdLayerEditor/lib/stringResources.h:37`):**
```cpp
// Retreive a string resource from the given std::wstringResourceId.
LayerEditorAPI QString getAsQString(const Resource& strResID);

// create a std::wstringResourceId, must be called before registerAll()
LayerEditorAPI Resource create(const char* key, const char* value);
```

`std::wstringResourceId` does not exist. The actual type is `Resource`.

**decision:** fix comment to match type

---

## `sessionState.h`

### L83–87: virtual display-options comment block

Not in original. The block describes the standard virtual/override pattern, which is
self-evident from the declarations.

**New:**
```cpp
    // Layer-contents display options. Default implementations store the value
    // in protected members; DCC integrations (e.g. MayaSessionState) override
    // these setters to persist the value in their preference store (optionVar
    // etc.). The default getters return the cached value.
    virtual bool displayLayerContents() const { return _displayLayerContents; }
    virtual void setDisplayLayerContents(bool show);
```

**Old (`lib/usd/ui/layerEditor/sessionState.h:91`):**
```cpp
    virtual bool displayLayerContents() const { return _displayLayerContents; }
    virtual void setDisplayLayerContents(bool show);
```

**decision:** Remove

---

## `saveLayersDialog.cpp`

### L460–467: DCC object path resolution — verbose expansion of a two-line fallback

The original had no comment here at all (it used `info.dagPath.fullPathName().asChar()`
directly). The new code adds a four-line explanation for two lines of fallback logic.

**New:**
```cpp
        // Resolve the DCC object path for this stage. Prefer the explicit
        // dccObjectPath set on the info (Maya side fills this in); fall back
        // to the UFE-derived stage path for callers that haven't been
        // updated yet.
        std::string dccObjectPath = info.dccObjectPath;
        if (dccObjectPath.empty()) {
            dccObjectPath = UsdUfe::stagePath(info.stage).string();
        }
```

**decision:** Remove.

---

### L469–473: component-check — verbose expansion of a one-line comment

Original had `// Check if this stage is a component stage` (one line). Four lines of
architecture prose about the DCC-functions registry were appended.

**Old (`lib/usd/ui/layerEditor/saveLayersDialog.cpp:463`):**
```cpp
        // Check if this stage is a component stage
        if (MayaUsd::ComponentUtils::shouldDisplayComponentInitialSaveDialog(
                info.stage, proxyPath)) {
```

**New:**
```cpp
        // Check if this stage is a component stage. The check is routed through
        // the DCC-functions registry, which returns false by default so DCCs
        // without component support simply skip this branch. The accessor is a
        // free function that reads the registry directly, so it works even in
        // this bulk-save constructor where no SessionState is attached.
        const bool isComponent = UsdLayerEditor::shouldDisplayComponentInitialSaveDialog(...);
```

**decision:** Match old.

---

### L519–521: single-stage component check — one sentence added to existing comment

Original had `// Check if this stage is an unsaved component stage.` The routing
explanation is new.

**Old (`lib/usd/ui/layerEditor/saveLayersDialog.cpp:506`):**
```cpp
        // Check if this stage is an unsaved component stage.
```

**New:**
```cpp
        // Check if this stage is an unsaved component stage. Routed through
        // SessionState so DCCs without component support fall straight into
        // the normal getLayersToSave path.
```

**decision:** match old.

---

### L547–550: `getLayersToSave` — new comment with no original counterpart

The original used `getLayersToSaveFromProxy` with no comment. The new comment is the
only place this UFE path-format caveat is documented.

```cpp
    // Get the layers to save for this stage. Use the stage directly when available
    // to avoid UFE path-format mismatches (e.g. Maya DAG paths vs |world-prefixed
    // UFE keys) that cause getLayersToSaveFromDCCObject to silently return nothing.
```

The "silent failure" callout is the useful part; the first sentence restates the
function name. Borderline.

**decision:** to nothing.

---

### L855–859: component-save — verbose expansion of a one-line comment

Original had `// Save component stages first` (one line). Five lines of DCC delegation
architecture were appended.

**Old (`lib/usd/ui/layerEditor/saveLayersDialog.cpp:848`):**
```cpp
    // Save component stages first
    for (auto* componentWidget : _componentSaveWidgets) {
```

**New:**
```cpp
    // Save component stages first. The actual DCC-specific work (moving the
    // component on disk, renaming the proxy/object in the DCC, transferring
    // the session layer content, locking the new root layer) is routed
    // through SessionState / AbstractCommandHook virtuals so that DCCs
    // without component-creator support simply skip this branch.
    for (auto& componentWidget : _componentWidgets) {
```

**decision:** Match old.

---

### L880–883: post-rename stage-entry lookup — new comment, no original

```cpp
                // After the rename, the session state should point at the
                // (now-renamed) stage entry. Try to relocate it among the
                // session's known stages by matching on the new name.
                auto entries = _sessionState->allStages();
                for (const auto& entry : entries) {
```

The original located the stage entry differently (exact path match) and had no comment.
This comment paraphrases the loop.

**decision:** Do nothing.

---

### L892–894: lockLayer call — new comment, no original

```cpp
                // Lock the newly-saved root layer. The shared lockLayer
                // helper is DCC-agnostic.
                lockLayer(lockShapePath, newRootLayer, LayerLockType::LayerLock_Locked, true);
```

"DCC-agnostic" adds nothing at a call site; the first sentence restates the function
name.

**decision:** XXX

---

## `LayerEditorCommands.h`

The class/member comments in this header (`// commands that need to backup`,
`// Backup and restore edit targets`, `// Backup dirty layer`, `// Edit targets that
were made invalid`, `// The command itself doesn't retain`) all exist verbatim in the
predecessor `layerEditorCommand.cpp`. They are ported, not generated.

Two items that are genuinely new:

### L297–298: constructor disambiguation comments

The original `RemoveSubPath` class had a single default constructor with no comment.
The new class has two overloads; the comments label them by parameter type — which the
parameter declarations already do unambiguously.

```cpp
    // Constructor using subpath index.
    RemoveSubPathCmd(... const int index ...) : ...

    // Constructor using subpath.
    RemoveSubPathCmd(... const std::string& subpath ...) : ...
```

**decision:** Remove

---

### L121–122: `// we need to hold onto the layer if we dirty it`

The original `BackupLayerBase` in `layerEditorCommand.cpp` had no comment on
`_backupLayer`. This comment is new and imprecise ("if we dirty it" — it's a backup
copy, not the dirtied layer).

```cpp
    // we need to hold onto the layer if we dirty it
    PXR_NS::SdfLayerRefPtr _backupLayer;
```

**decision:** Double check it wasnt in base. If indeed not, remove.

---

## `layerLocking.h`

### L84–91: `loadLayerLockState` @param doc — new function, partially restatement

`loadLayerLockState` is new to the shared editor (original had different functions:
`copyLayerLockingToAttribute` / `copyLayerLockingFromAttribute`). The `@param locked`
and `@param stage` lines restate parameter names; `@param nameMap` is the one
informative line.

```cpp
/**
 * Loads a layer lock state.
 * @param locked  The layer identifiers of layers to be locked.     ← restates name
 * @param nameMap Layer name map. When Anon layers are saved ...    ← useful
 * @param stage   The USD Stage.                                    ← restates name
 */
LayerEditorAPI void loadLayerLockState(
    const std::vector<std::string>& locked,
    const LayerNameMap&             nameMap,
    PXR_NS::UsdStage&               stage);
```

**decision:** do nothing.

---

## `layerMuting.h`

### L29–34: `loadLayerMuteState` @param doc — new function, partially restatement

Same as `layerLocking.h` above. The original `layerMuting.h` had no `loadLayerMuteState`
function; it used `copyLayerMutingFromAttribute`. The `@param muted` and `@param stage`
lines restate parameter names.

```cpp
/**
 * Loads a layer mute state.
 * @param muted   The layer identifiers of layers to be muted.     ← restates name
 * @param nameMap Layer name map. When Anon layers are saved ...   ← useful
 * @param stage   The USD Stage.                                   ← restates name
 */
```

**decision:** Do nothing.

---

## `utilQT.h`

### L31–33: `initializeQtUtils` doc

`initializeQtUtils` is a new function with no predecessor in `qtUtils.h`. The doc
restates the function name.

```cpp
/**
 * Initializes the qt utilities.
 */
LayerEditorAPI void initializeQtUtils();
```

**decision:** remove.

---

### L96–97 (first sentence only): `ValidTfIdentifierValidator` class doc

The original `qtUtils.h` had the class declaration with no doc comment. The first
sentence restates the class name; the second sentence is useful.

```cpp
/**
 * @brief Validator that only accepts strings that are also valid Tf identifiers.
 *                                            ← restates class name
 * Used by the component-save widget to gate component-name entry to a USD-safe
 * identifier.                               ← useful: explains usage context
 */
```

**decision:** (keep second sentence)

---

---

# Typo Regressions and Comment Downgrades

Found during the verification pass. These are not AI-generated slop (the comments
existed before) but are regressions introduced during porting.

---

## `utilFileSystem.h` — `appendPaths` doc has typos introduced

The original `mayaUsd/utils/utilFileSystem.h` spells these words correctly. The new
shared file has typos.

| | original | new |
|---|---|---|
| `@param a` | `represents` | `respresents` |
| `@param b` | `represents` | `respresents` |
| `@return` | `separator` | `seperator` |

**decision:** fix typos

---

## `layerMuting.cpp` — `getMutedLayers` comment downgraded

The original had an informative comment explaining *what* the map holds. The new file
replaced it with a vaguer description.

**Old (`mayaUsd/utils/layerMuting.cpp:84`):**
```cpp
// Maps muted layer identifier -> set of held layers (root + descendants).
//
// Kept in a function to avoid problem with the order of construction
// of global variables in C++.
```

**New (`lib/usdLayerEditor/lib/layerMuting.cpp:26`):**
```cpp
// The set of muted layers.
//
// Kept in a function to avoid problem with the order of construction
// of global variables in C++.
```

Note: the data structure also changed from a `std::unordered_map` (original) to a
`std::set` (new), so "The set of muted layers" is at least accurate — but less
informative than the original.

**decision:** Match old comment.

---

---

# Logic Discrepancies

---

## D1 — `batchSaveLayersUIDelegate.cpp`: `kSaveToMayaSceneFile` branch missing

**Severity: Functional regression (Maya only)**

The new shared delegate only handles `kSaveToUSDFiles`. The `kSaveToMayaSceneFile`
branch that showed the component-save dialog is absent. When Maya has serialization set
to "Save to Maya Scene File" and there are component stages, the shared version silently
returns `kNotHandled` instead of showing the dialog.

**Old (handles both cases):**
```cpp
if (MayaUsd::utils::kSaveToUSDFiles == opt) {
    ...
    return kAbort or kPartiallyCompleted;
} else if (MayaUsd::utils::kSaveToMayaSceneFile == opt) {
    // When saving to Maya scene file, only show dialog for component stages
    bool hasComponentStages = false;
    for (const auto& info : infos) {
        if (MayaUsd::ComponentUtils::isAdskUsdComponent(info.dagPath...))
            hasComponentStages = true;
    }
    if (hasComponentStages) {
        SaveLayersDialog dlg(nullptr, infos, isExporting, /*componentsOnly=*/true);
        dlg.exec();
        return MayaUsd::kPartiallyCompleted;
    }
}
return MayaUsd::kNotHandled;
```

**New (only handles `kSaveToUSDFiles`):**
```cpp
if (Serialization::kSaveToUSDFiles == opt) {
    ...
    return kAbort or kPartiallyCompleted;
}
return BatchSaveResult::kNotHandled;   // ← kSaveToMayaSceneFile falls here
```

**decision:** Need to plan a fix and ideally a test case.

**resolution:** No change — already resolved. The delegate registered at runtime is the Maya
overload (`plugin.cpp:447`), which carries the `kSaveToMayaSceneFile` branch
(`lib/usd/ui/layerEditor/batchSaveLayersUIDelegate.cpp:208-238`) and routes it through the shared
`SaveLayersDialog` componentsOnly ctor. The shared overload is intentionally USD-files-only. This
review predates the Maya-side re-add.

---

## D2 — `layerTreeItemDelegate.cpp`: `TARGET_OFF` pixmap names differ

**Severity: Visual — wrong icons shown for non-target layers**

**Old:**
```cpp
const char* targetOffPixmaps[3] { "target_off", "target_off_hover", "target_off_pressed" };
```

**New:**
```cpp
const char* targetOffPixmaps[3] { "target_regular", "target_hover", "target_pressed" };
```

The `target_on` names are unchanged. Only the `TARGET_OFF` names differ. One set will
find the wrong or missing resource files.

**decision:** Investigate this more. This seems surprising, targets visually seem fine. Maybe the icon names are changed? Maybe compare the icon files on disk.

**resolution:** No change — benign rename. `target_off*` was always a `.qrc` *alias* mapping to the
physical files `target_regular_*.png` / `target_hover_*.png` / `target_pressed_*.png`. The new code
uses those real base names directly, and the new `.qrc` keeps the same aliases pointing at the same
(byte-identical) images. The off-state was always physically named `target_regular`; the unification
just dropped the misleading `target_off` alias layer. (Unrelated: the new `resources.qrc` has two
pre-existing typos in the `target_on` set — `target_on_hover_100` aliases the `_150` source, and a
stray `P` in `target_on_pressedP_100`. Out of scope here; worth a separate cleanup.)

---

## D3 — `layerTreeView.h`: `simpleLayerMethod` dropped `QWidget*` parameter

**Severity: Functional — dialog parenting lost for all methods called via this pointer**

Every `LayerTreeItem` method invoked via `callMethodOnSelection` (e.g. `saveEdits`,
`discardEdits`, `removeSubLayer`) previously received the tree view as a parent widget
for any dialogs they open. They now receive nothing.

**Old:**
```cpp
typedef void (LayerTreeItem::*simpleLayerMethod)(QWidget* in_parent);
void callMethodOnSelection(const QString& undoName, simpleLayerMethod method);
void callMethodOnSelectionNoDelay(const QString& undoName, simpleLayerMethod method);
```

**New:**
```cpp
typedef void (LayerTreeItem::*simpleLayerMethod)();
void callMethodOnSelection(const QString& undoName, simpleLayerMethod method);
// callMethodOnSelectionNoDelay removed
```

**decision:** Why was callMethodOnSelectionNoDelay removed. Also investigate this difference further, what are the consequences of this?

**resolution:** `callMethodOnSelectionNoDelay` removal is safe — it existed only to work around
EMSUSD-1619, where the old MEL-string `MayaCommandHook::removeSubLayerPath` baked the sublayer index
into the command at queue time, so delayed multi-removals used stale indices. The new
`UfeCommandHook` path constructs `RemoveSubPathCmd` with the path string and resolves the index
*lazily at execution* (`layerEditorCommands.cpp` `InsertRemoveSubPathBaseCmd::doIt`), so the
reorder bug cannot recur; routing through the normal (delayed) `callMethodOnSelection` is correct.
The only real fallout of dropping `QWidget*` is dialog parenting, which is fixed together with D6
via the new `mainWindowParent()` DCC hook (keeping `simpleLayerMethod` parameterless).

---

## D4 — `layerTreeModel.cpp`: dropped `effectiveTargetLayer` comment in `rebuildModel`

**Severity: Documentation — non-obvious EF-mode behavior is no longer explained**

**Old (`lib/usd/ui/layerEditor/layerTreeModel.cpp:328`):**
```cpp
    if (_sessionState->autoHideSessionLayer()) {
        // Use the effective target so that in EF mode the decision follows the fallback
        // target rather than the session layer (which is always the stage edit target there).
        showSessionLayer
            = sessionLayer->IsDirty() || sessionLayer == _sessionState->effectiveTargetLayer();
    }
```

**New (`lib/usdLayerEditor/lib/layerTreeModel.cpp:327`):**
```cpp
    if (_sessionState->autoHideSessionLayer()) {
        showSessionLayer
            = sessionLayer->IsDirty() || sessionLayer == _sessionState->effectiveTargetLayer();
    }
```

**decision:** Match old comment.

**resolution:** Fixed — re-added the `effectiveTargetLayer` explanatory comment in
`layerTreeModel.cpp` `rebuildModel`.

---

## D5 — `saveLayersDialog.cpp`: post-rename stage-entry lookup uses substring match

**Severity: Correctness — could match the wrong stage**

**Old (exact path match):**
```cpp
if (entry._proxyShapePath
    == std::string(newProxyShapePath.fullPathName().asUTF8())) {
    _sessionState->setStageEntry(entry);
    break;
}
```

**New (substring match):**
```cpp
if (!entry._dccObjectPath.empty()
    && entry._dccObjectPath.find(componentName) != std::string::npos) {
    _sessionState->setStageEntry(entry);
    break;
}
```

If two stages have names where one path contains another's component name as a substring,
the wrong entry may be selected.

**decision:** Seems wrong indeed, need to plan a fix.

**resolution:** Fixed — the `renameProxyShape` DCC hook now returns the new DCC object path
(`RenameProxyShapeFn` is `std::function<std::string(...)>`; Maya returns the renamed proxy's full
DAG path). The post-rename loop in `saveLayersDialog.cpp` now matches `entry._dccObjectPath ==
newDccObjectPath` exactly instead of a substring of `componentName`.

---

## D6 — `warningDialogs.cpp`: `QMessageBox` constructed without parent

**Severity: UI behavior — dialogs may float over wrong window or not be modal correctly**

**Old:**
```cpp
bool confirmDialog_internal(bool okCancel, QWidget* parent, ...) {
    QMessageBox msgBox(parent);
```

**New:**
```cpp
bool confirmDialog_internal(bool okCancel, ...) {   // parent removed
    QMessageBox msgBox;                              // no parent
```

**decision:** Need to match old, plan a fix.

**resolution:** Fixed — restored parenting without re-threading `QWidget*` (D3 stays
parameterless). Added a `mainWindowParent()` DCC hook (Maya wires `MQtUtil::mainWindow()`);
`warningDialogs` re-gained an optional trailing `QWidget* parent` that defaults to that hook when
null. `SaveLayersDialog` passes `this`; other call sites use the main-window fallback.

---

## D7 — `stringResources.h/cpp`: `kReloadTitle`/`kReloadMsg` wording changed

**Severity: UI string change — different dialog title and message text**

| | Old (`kRevertToFile*`) | New (`kReload*`) |
|---|---|---|
| Title | `"Revert to File \"^1s\""` | `"Reload \"^1s\""` |
| Message | `"Are you sure you want to revert \"^1s\" to its state on disk? All edits will be discarded."` | `"Reloading \"^1s\" will discard all current edits and revert it to its state on disk. Are you sure you want to proceed?"` |
| Button text resource | (none) | `kReloadButtonText = "Reload"` |

**decision:** Do nothing.

---

## D8 — `layerContentsWidget.cpp`: Maya optionVar size limits removed

**Severity: Feature loss (Maya) — array/time-sample display limits no longer user-configurable**

**Old:**
```cpp
OutputType::ReportParams params;
bool exists { false };
int opt = MGlobal::optionVarIntValue(MayaUsdOptionVars->LayerContentsArraySizeLimit, &exists);
if (exists) params.arraySizeLimit = opt;
opt = MGlobal::optionVarIntValue(MayaUsdOptionVars->LayerContentsTimeSamplesSizeLimit, &exists);
if (exists) params.timeSamplesSizeLimit = opt;
```

**New:**
```cpp
OutputType::ReportParams params;
// Note: in the shared component we keep the default array/timeSample size limits.
//       DCC integrations can later expose an override mechanism if needed ...
```

The comment acknowledges this is intentional. Whether Maya still needs to restore this
via a DCC override hook is the open question.

**decision:** We need to re-add this capability, need a new DCC function, plan for it.

**resolution:** Fixed — added `layerContentsArraySizeLimit()` / `layerContentsTimeSamplesSizeLimit()`
to the `EnvironmentFns` DCC registry (default 8). `layerContentsWidget.cpp` applies them to the
`ReportParams`; Maya registers lambdas reading the `LayerContentsArraySizeLimit` /
`LayerContentsTimeSamplesSizeLimit` optionVars (returning 8 when unset, matching old semantics).

---

## D9 — `layerEditorWidget.cpp`: `_autoHide` removed from `_actions` struct

**Severity: Minor — auto-hide action no longer stored; menu item order changed**

**Old (`layerEditorWidget.h`):**
```cpp
struct {
    QAction* _autoHide { nullptr };
    ...
} _actions;
```

**New:** `_actions` struct has no `_autoHide` field. The action is created as a local
variable inside `setupDefaultMenu` and not retained. Menu order also changed: old placed
auto-hide before EF items; new places it after.

**decision:** Match old, plan for it.

**resolution:** Fixed — re-added `_autoHide` as the first member of the `_actions` struct and moved
the auto-hide action to the front of the Option menu (followed by a separator), matching the old
order. Note this is cosmetic: the field was write-only in the old code; the feature works via the
signal to `SessionState::setAutoHideSessionLayer` regardless.

---

## D10 — `layerTreeItem.cpp`: component-creator early-out removed from `saveAnonymousLayer`

**Severity: Feature loss — component stages no longer delegate to `saveStage`**

**Old:**
```cpp
void LayerTreeItem::saveAnonymousLayer(QWidget* in_parent)
{
    // Special case for components created by the component creator.
    if (SessionState* ss = parentModel()->sessionState()) {
        if (MayaUsd::ComponentUtils::isAdskUsdComponent(ss->stageEntry()._proxyShapePath)) {
            parentModel()->saveStage(in_parent);
            return;
        }
    }
    // ... generic anonymous layer save flow
```

**New:** No such check — anonymous layers inside component stages go through the generic
path.

**decision:** Need to match old, plan for it.

**resolution:** Fixed — re-added the component-creator early-out at the top of
`LayerTreeItem::saveAnonymousLayer`, using the shared predicate
`UsdLayerEditor::isStageAComponent(ss->stageEntry()._dccObjectPath)` (the ported equivalent of
`isAdskUsdComponent`) and delegating to `parentModel()->saveStage(nullptr)`.
