# `TODO LE-EXTRACT` Review

Review of the 36 `// TODO LE-EXTRACT` comments in `lib/usdUfe/usd-layer-editor/lib/`, assessed against the current code and the DCC-injection mechanisms introduced during the maya-usd → shared-component porting (see `MIGRATION.md`).

**Injection mechanisms referenced below:**
- `layerEditorDCCFunctions` registry — component / edit-forwarding / DCC-object `std::function` callbacks.
- `SessionState` / `AbstractCommandHook` / `UfeCommandHook` virtuals.
- Free-function setters: `updateDCCObjectRootLayerFunction`, `dccWorkspaceSceneSaveLocationFunc`, `UIUtils` error-display callback, `UsdUfe::registerUICallback`, and the `QtUtils` virtual override.

**Verdict legend:**
- **OBSOLETE** — feature already resolved; the comment is stale and can be deleted.
- **STILL APPLIES** — real functional gap; behavior is stubbed/hardcoded with no injection point.
- **PARTIAL** — mechanism exists but not wired, or a product/naming decision is pending.

> **How to annotate:** fill in the `**Your notes:**` line under any entry. Use the `Decision` field to record keep/drop/defer or assign priority.

---

## A. OBSOLETE — stale comment, safe to delete (10)

### A1. `utilQT.cpp:48` — Maya icon / pixmap behavior
- **Asks for:** extract Maya's DPI-scaling pixmap handling.
- **State:** handled by generic `QtUtils::createPNGResPixmap` + `MayaQtUtils::createPixmap` override + `setQtUtils()`; DPI-suffix logic is shared.
- **Verdict:** OBSOLETE
- **Your notes:**
- **Decision:** OK FIXED

### A2. `layerTreeView.cpp:154` — Refresh treeview on new system lock
- **Asks for:** refresh tree when system-lock status changes.
- **State:** implemented via `registerUICallback("onRefreshSystemLock", _refreshCallback)` → `repaint()`.
- **Verdict:** OBSOLETE
- **Your notes:**
- **Decision:** OK FIXED. Do we have test coverage for this, or can we add some?
- **Resolution (2026-06-08):** Coverage already exists in `test/lib/testMayaUsdLayerEditorCommands.py` — `testRefreshSystemLock`, `testRefreshSystemLockCallback`, `testRefreshSystemLockWithoutCallback`, `testRefreshSystemLockCallbackLockingAll`, `testRefreshSystemLockWithCallbackUnlockingAll` (+ `_verifyStageAfterRefreshSystemLock` helper) exercise the `onRefreshSystemLock` callback and assert lock state + edit-target handling. The Qt `repaint()` itself isn't asserted (not unit-testable). No new test added.

### A3. `layerTreeView.cpp:564` — Update mouse cursor over layer tree
- **Asks for:** custom hover cursor.
- **State:** implemented with generic Qt pixmap resources (`:/rmbMenu`), no Maya dependency.
- **Verdict:** OBSOLETE
- **Your notes:**
- **Decision:** OK FIXED.

### A4. `stageSelectorWidget.cpp:330` — Handle maya proxy shape selection changed
- **Asks for:** switch to a proxy-shape's stage on UFE selection.
- **State:** intentionally commented out; shared path uses the `SessionState::selectedStages()` virtual that Maya overrides.
- **Verdict:** OBSOLETE (by design)
- **Your notes:**
- **Decision:** OK FIXED

### A5. `stageSelectorWidget.cpp:368` — Handle maya proxy shape selection changed
- **Asks for:** loop UFE selection to detect proxy shapes.
- **State:** same as A4 — deferred to `SessionState::selectedStages()`.
- **Verdict:** OBSOLETE (by design)
- **Your notes:**
- **Decision:** OK FIXED

### A6. `batchSaveLayersUIDelegate.h:40` — Batch save layers
- **Asks for:** (label on struct/function).
- **State:** the delegate is fully implemented (shows `SaveLayersDialog`, handles anon layers, returns `BatchSaveResult`).
- **Verdict:** OBSOLETE
- **Your notes:**
- **Decision:** OK FIXED

### A7. `stringResources.cpp:21` — Is the string mechanism enough for maya-usd?
- **Asks for:** confirm C++ string-resource mechanism is sufficient / whether Maya registration matters.
- **State:** answered by implementation — shared reads `Resource::value` directly; Maya `.pres` registration is optional and lives in Maya code (`initStringResources.cpp`).
- **Verdict:** OBSOLETE
- **Your notes:**
- **Decision:** OK FIXED.

### A8. `stringResources.h:26` — Is the string mechanism enough? (header)
- **State:** same as A7.
- **Verdict:** OBSOLETE
- **Your notes:**
- **Decision:** OK FIXED

### A9. `utilSerialization.cpp:434` — MIGHT NEED cache-updating approach
- **Asks for:** decide whether to call `updateAllCachedStageWithLayer` here.
- **State:** resolved by design — cache updating is delegated to the `updateDCCObjectRootLayerFunction` DCC callback; inline call intentionally disabled and documented.
- **Verdict:** OBSOLETE (resolved by design)
- **Your notes:**
- **Decision:** NO - THIS SEEMS LIKE A GAP. We need to have a DCC function to get Stage caches. In max we will just return one, the global one, and in maya we will return UsdMayaStageCache::GetAllCaches()
- **Resolution (2026-06-08): DONE.** Added `Serialization::setGetStageCachesFunction` (returns `std::vector<UsdStageCache*>`, defaults to `{ &UsdUtilsStageCache::Get() }` when unset). `updateAllCachedStageWithLayer` now loops over `getStageCaches()`. Maya registers it in `batchSaveLayersUIDelegate.cpp` to return `UsdMayaStageCache::GetAllCaches()`. Build + layer-editor test suite green (5/5).

### A10. `utilSerialization.cpp:523` — Double-check `wasTargetLayer` usage
- **Asks for:** verify how maya-usd tracked anon-root-layer-as-edit-target.
- **State:** variable commented out and unused; post-save flow doesn't need it. Verify once, then delete.
- **Verdict:** OBSOLETE (verify-only)
- **Your notes:**
- **Decision:** THIS IS A GAP, we need to wire this bool like it was before. See here     UsdLayerEditor::Serialization::setUpdateDCCObjectRootLayerFunction(
        [](const std::string& proxyPath, const std::string& layerPath) {
            MayaUsd::utils::setNewProxyPath(
                MString(proxyPath.c_str()),
                MString(layerPath.c_str()),
                MayaUsd::utils::kProxyPathFollowProxyShape,
                nullptr,
                false);
        });
- The last arg is hardcoded to false, but it should be wasTarget...
- **Resolution (2026-06-08): DONE.** `wasTargetLayer` is now computed in `saveAnonymousLayer` and threaded through `updateDCCObjectRootLayerFunction` (widened to `(string, string, const SdfLayerRefPtr&, bool)`). The Maya lambda forwards the real `layer` + `wasTargetLayer` into `setNewProxyPath` instead of `nullptr`/`false`. Build + layer-editor test suite green (5/5).

---

## B. STILL APPLIES — real functional gaps, no injection point (19)

### B1. Save-path & composition options — hardcoded *(highest impact)*

All seven return **hardcoded constants** instead of reading the corresponding Maya optionVars. Confirmed called live from `layerTreeItem.cpp`, `saveLayersDialog.cpp`, `loadLayersDialog.cpp` → **user preferences are currently ignored.** Shared root cause: no DCC option-storage callback.

| # | TODO | Function | Returns now | Should read |
|---|------|----------|-------------|-------------|
| B1.1 | `utilFileSystem.cpp:415` | `requireUsdPathsRelativeToDCCSceneFile` | `true` | `mayaUsd_MakePathRelativeToSceneFile` |
| B1.2 | `utilFileSystem.cpp:425` | `requireUsdPathsRelativeToParentLayer` | `true` | `mayaUsd_MakePathRelativeToParentLayer` |
| B1.3 | `utilFileSystem.cpp:435` | `requireUsdPathsRelativeToEditTargetLayer` | `true` | `mayaUsd_MakePathRelativeToEditTargetLayer` |
| B1.4 | `utilFileSystem.cpp:445` | `wantReferenceCompositionArc` | `false` | `mayaUsd_WantReferenceCompositionArc` |
| B1.5 | `utilFileSystem.cpp:454` | `wantPrependCompositionArc` | `true` | `mayaUsd_WantPrependCompositionArc` |
| B1.6 | `utilFileSystem.cpp:463` | `wantPayloadLoaded` | `true` | `mayaUsd_WantPayloadLoaded` |
| B1.7 | `utilFileSystem.cpp:472` | `getReferencedPrimPath` | `{}` | `mayaUsd_ReferencedPrimPath` |

- **Verdict:** STILL APPLIES (all 7)
- **Your notes:**
- **Decision:** YES, GAP. Fix. Maybe we should have in DCC functions a group for all saved config access & write.

### B2. Option persistence backend — writes are dropped (3)
`utilOptions.h:24` (`optionVarExists`), `:36` (`optionVarIntValue`), `:56` (`setOptionVarValue`)
- **State:** only `ConfirmExistingFileSave` is special-cased; everything else returns `false`/`0`. `setOptionVarValue` is a **no-op**. Yet `saveLayersDialog.cpp` and `utilSerialization.cpp` call it to persist save-as-relative / binary-format / edits-location choices → **silently lost.**
- **Fix shape:** DCC option-storage callback (Maya → `MGlobal::optionVar*`). Would also resolve B1.
- **Verdict:** STILL APPLIES (all 3)
- **Your notes:**
- **Decision:** YES, GAP. Fix it. Maybe we should have in DCC functions a group for all saved config access & write.

### B3. Project-relative paths — stubbed (3)
`utilFileSystem.cpp:135` (`getPathRelativeToProject`), `:166` (`makeProjectRelatedPath`), `:404` (`prepareLayerSaveUILayer`)
- **State:** return empty/`true` stubs; no "DCC project/workspace path" callback. Affects relative-path anchoring during save UI.
- **Verdict:** STILL APPLIES (all 3)
- **Your notes:**
- **Decision:** YES, GAP, FIX it. Need new DCC functions as well.

### B4. `utilSerialization.cpp:513` — Up-axis & units callback
- **State:** `setLayerUpAxisAndUnits` commented out → saved anonymous root layers get no up-axis/units metadata. No pre-save DCC callback exists.
- **Verdict:** STILL APPLIES
- **Your notes:**
- **Decision:**

### B5. `stageSelectorWidget.cpp:65` & `:74` — Save/load pinned stage option
- **State:** pinned-stage preference doesn't persist (stub returns `false` / no-op save). Tied to B2.
- **Verdict:** STILL APPLIES (both)
- **Your notes:**
- **Decision:** YES, GAP. We need a a DCC function.

### B6. `layerTreeView.cpp:208` — SHIFT modifier = expand/collapse all
- **State:** returns hardcoded `false` → recursive expand/collapse disabled.
- **Note:** likely needs **no** DCC injection — `QApplication::keyboardModifiers()` is generic Qt and can replace the old `MGlobal getModifiers`.
- **Verdict:** STILL APPLIES
- **Your notes:**
- **Decision:** GAP, CREATE A DCC FUNCTION.

### B7. `ufeCommandHook.cpp:146` — Select prims with spec
- **State:** empty override → context-menu "select prims with spec" is a no-op in shared.
- **Verdict:** STILL APPLIES
- **Your notes:**
- **Decision:** Leave the TODO comment. We dont use the ufe command hook in maya-usd we can defer to later.

### B8. `ufeCommandHook.cpp:35` — Delayed command execution
- **State:** `executeDelayedCommands` has an empty body. Delay-counting mechanism exists but never flushes.
- **Note:** needs investigation — if any shared path enters the delayed state, commands could be queued and never run.
- **Verdict:** STILL APPLIES (needs investigation)
- **Your notes:**
- **Decision:** Leave the TODO comment. We dont use the ufe command hook in maya-usd we can defer to later.

### B9. `batchSaveLayersUIDelegate.cpp:31` — Maya non-interactive mode
- **State:** `MGlobal::mayaState()` guard commented out → shared editor always shows the save dialog, even in batch/headless. Needs an "isInteractive" hook.
- **Verdict:** STILL APPLIES
- **Your notes:**
- **Decision:** We need a DCC function IsInteractiveDCCSession() to support this.

### B10. `stringResources.h:102` — Temporary confirmation string `kToSaveStageFilesConfirm`
- **State:** dead string — defined and Maya-registered but unused anywhere (real message is `kToSaveTheStageSaveFiles`). Pure cleanup.
- **Verdict:** STILL APPLIES (cleanup — remove it)
- **Your notes:**
- **Decision:** OK, FIXED.

---

## C. PARTIAL / by-design decision (5)

### C1. `layerEditorCommands.cpp:133` — Error message when no available edit target
- **State:** functionality works (falls back to session layer), but the user-facing message is dropped. **Injection point already exists** — `UIUtils::displayError(errMsg)` is used 20 lines below in the same file. Trivial wire-up.
- **Verdict:** PARTIAL (quick fix)
- **Your notes:**
- **Decision:** FIX IT, Use TF_ERROR

### C2. `layerEditorWidget.cpp:144` — Maya menus (auto-hide session layer / help menu)
- **State:** menu items commented out. Auto-hide is a `SessionState` virtual; help routes through `commandHook()->showLayerEditorHelp()`.
- **Decision needed:** do we want these menus in the shared editor, or keep them Maya-only?
- **Verdict:** PARTIAL (product decision)
- **Your notes:**
- **Decision:** Uncomment, we want these menus.

### C3. `ufeCommandHook.cpp:141` — Show layer editor help
- **State:** empty no-op; only matters if the help menu (C2) is re-enabled. Decide together with C2.
- **Verdict:** PARTIAL (tied to C2)
- **Your notes:**
- **Decision:** Leave the TODO, maya-usd doesnt use the ufeCommandHook, we will do this later.

### C4. `utilFileSystem.h:81` — "Workspace" terminology outside Maya
- **State:** injection (`dccWorkspaceSceneSaveLocationFunc`) is done; remaining question is purely naming ("workspace" is Maya-centric). Documentation/naming decision.
- **Verdict:** PARTIAL (naming decision)
- **Your notes:**
- **Decision:** OK, FIXED.

### C5. `utilSerialization.cpp:194` — Maya has multiple stage caches
- **State:** `updateAllCachedStageWithLayer` is hardcoded to the global cache but is **currently dead in the shared flow** (only caller is the disabled call at `:434`; cache updating is delegated to `updateDCCObjectRootLayerFunction`). Either delete the helper or add a cache-enumeration callback if it should be DCC-pluggable.
- **Verdict:** PARTIAL (dead helper / decision)
- **Your notes:**
- **Decision:** I think this was already adressed from a previous point.

---

## Suggested remediation priorities

1. **B1 (save options, 7) + B2 (option persistence, 3)** — biggest correctness gap; user prefs ignored/lost. Shared root cause: **no DCC option-storage callback.** A single `OptionsFns`-style registry group could resolve all 10 at once.
2. **B3 / B4 / B9** — save correctness in non-Maya / batch contexts (project-relative paths, up-axis/units, non-interactive guard).
3. **Quick wins:** C1 (wire existing `UIUtils::displayError`), B6 (use Qt `keyboardModifiers()`), delete the 10 stale comments in bucket A and the dead string B10.
4. **Decisions needed:** C2/C3 (help & auto-hide menus), C4 (terminology), B7/B8 (select-prims-with-spec & delayed commands — required in shared or Maya-only?).

**Overall notes / priority for this round:**
-
