# USD Layer Editor — C++ Test Suite Review

Review of the C++ tests in `lib/usdUfe/usd-layer-editor/test/cpp` for completeness and
correctness, using the production layer-editor behavior (shared `lib/usdUfe/usd-layer-editor/lib`
and the mature `lib/usd/ui/layerEditor`) as the reference.

## Architecture note (shapes every recommendation)

The `*Logic.h` test bodies are **shared** between two harnesses:
- the new standalone GoogleTest binary `UsdLayerEditorNewTests` (links `UsdLayerEditorLib`), and
- the old Maya-plugin parity harness in `lib/usd/ui/layerEditor/test/cpp` (`BUILD_NEW_LAYER_EDITOR=OFF`).

Both compile the same `TEST_F` bodies. New-editor-only assertions must be wrapped in
`#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED` (that macro is **undefined** in the new build and
**defined** in the old build, so guarded blocks compile only for the new editor). Any fix must
keep both editors compiling and must not silently break parity.

## Overall assessment

Breadth is good — nearly every widget/command class is exercised, real USD stages are used, and
teardown is clean. The systemic weakness is **assertion depth**: a meaningful fraction of tests can
pass without exercising the behavior their name claims. Estimated ~25–35 of 224 tests are currently
tautological, misdirected, or neutralized by the default fixture configuration.

---

## Cross-cutting systemic issues

1. **`hasCall("X")`-only assertions ignore recorded arguments.** `StubCommandHook` records the
   target layer identifier for every call, but most tests check only that *a* call happened — not
   *which layer*. This makes paired tests indistinguishable (e.g. `..._AddsToRoot` vs
   `..._AddsSibling` both just check `hasCall("addAnonymousSubLayer")`). Fix: assert
   `lastCall().args[0] == expectedLayer->GetIdentifier()`.

2. **Conditional / escape-hatch / self-skip assertions that cannot fail.** The reorder suite's
   `if (accepted) { EXPECT real } else { EXPECT trivial }`; dialog tests' dead `if(!cb) GTEST_SKIP()`.
   A real regression passes via the trivial branch.

3. **The `isIdenticalItem` rebuild skip-optimization neutralizes rebuild/memento tests.** Several
   tests call `forceRefresh()` on an *unchanged* stage, so `rebuildModel` early-returns without
   clearing rows or emitting `beginResetModel`/`endResetModel`. They assert pre-existing state.
   Fix: mutate layer structure first so a genuine rebuild fires.

4. **The default fixture's `_isSharedStage = false` neutralizes save / needs-saving tests.**
   `updateButtons()` only manages the save button's enabled state in the shared-stage branch
   (layerEditorWidget.cpp:459-499); `needsSaving()` returns early at the `!_isSharedStage` guard.
   So those tests pass for the wrong reason. Fix: use a shared-stage fixture (see
   `SharedStageFixture` in `testSharedStageLogic.h`).

5. **Test names over-promise relative to assertions** (e.g. `DragDrop_Drop_AdjustsRowIndexWhenMovingUp`
   asserts only `rowCount >= 1`; `Delegate_TargetIconRect_XOffsetIsArrowAreaWidth` asserts `>` rather
   than `== left()+DPIScale(16)`).

6. **Stub no-ops are sometimes the thing being "tested."** `setEditTarget`, `flattenLayer`,
   `stitchLayers`, `muteSubLayer`, `refreshLayerSystemLock` only *record*; they do not mutate the
   stage. Tests needing real state must use `stage->MuteLayer` / `lockLayerDirect`, not the hook.

---

## Correctness issues

Status legend: ✅ verified against source · **(fixed)** addressed in this change.

| # | Location | Issue | Status |
|---|----------|-------|--------|
| C1 | `testContextMenuLogic.h` `MergeWithSublayers_BlockedWhenNoSublayers` / `...WhenLayerIsLocked` | Assert `!hasCall("stitchLayers")`, but `mergeWithSublayers` calls **`flattenLayer`**, never `stitchLayers` (layerTreeItem.cpp:635). Tautological. | ✅ **(fixed)** assert `!hasCall("flattenLayer")` + positive control added |
| C2 | `testButtonsLogic.h` `SaveStageButton_EnabledWhenDirty` / `SaveStageButton_Click_DismissesDialog` | Run with `_isSharedStage=false`; the save button is hidden and its enabled-state is never managed, so the assertion is vacuous. | ✅ **(fixed)** retargeted to a local shared-stage fixture |
| C3 | `testLayerTreeModelLogic.h` `SetEditTarget_BlockedWhenLayerIsMuted` | `item` captured before `MuteLayer`+`processEvents()` triggers a rebuild that deletes it; later dereference is use-after-free. | ✅ **(fixed)** re-fetch item after `processEvents()` |
| C4 | `testReorderLogic.h` drop tests | Escape-hatch `if(accepted)/else`; no test asserts resulting sublayer **order** (the riskiest `dropMimeData` row-adjust logic). | ✅ **(fixed)** `ASSERT_TRUE(accepted)` + order assertions |
| C5 | `testLayerTreeViewLogic.h` `Memento_RestoredAfterModelReset` | Never triggers a reset (skip-optimization); passes because `expandAll()` already ran. | ✅ **(fixed)** force a structural change before refresh |
| C6 | `testLayerTreeViewLogic.h` `DoubleClick_SkipsWhen...` (×2) | Never invoke the handler; assert a manually-zeroed counter. | ✅ **(fixed)** invoke `onItemDoubleClicked` and assert no save attempt |
| C7 | `testSaveLayersDialogLogic.h` | `AllAsRelativeCheckboxExists` only `SUCCEED()`s; `AllAsRelative_ToggleDoesNotCrash` self-skips on a precondition that always holds in the new editor; `OkToSave_DoesNotCrashWithNoLayers` is misnamed (dialog is not empty; `okToSave()` is never called). | ✅ **(fixed)** new-editor-guarded checkbox assertions + rename |

> **Old-editor parity:** the C1/C3/C4/C5/C6 fixes are parity-safe (both editors share the behavior).
> C2 uses a shared-stage fixture supported by both editors. C7's strengthened assertions are
> guarded new-editor-only because the old editor's proxy-based layer discovery legitimately finds
> no anonymous layers in the stub (documented in `PARITY_STATUS.md`). The old-editor parity build
> (`BUILD_NEW_LAYER_EDITOR=OFF`, interactive Maya) must be re-run to confirm.

---

## Coverage gaps (untested production behavior)

**Commands (`testLayerEditorCommands.cpp`) — highest production risk:**
- No `do → undo → redo` round-trip anywhere; `AddAnonSubLayerCmd`'s deliberate identifier-caching
  on redo is documented but untested.
- Three command classes have **zero** behavioral coverage: `StitchLayersCmd`, `FlattenLayerCmd`,
  `SetEditTargetCmd`. `RefreshSystemLockLayerCmd` has only a trivial getter test.
- The documented crash-fix path is untested: `RemoveSubPathCmd` edit-target redirect (when the
  removed layer *is* the edit target → retarget to root, restore on undo).
- Error/edge branches broadly untested: out-of-bounds insert/remove, `MoveSubPathCmd`'s four
  `return false` paths, locked-layer stitch.

**Other areas:**
- **Context menu:** no test invokes `buildContextMenu` — action presence/absence, enable/disable
  per layer state, and labels (Lock vs Unlock) untested. `addParentLayer`, `loadSubLayers`,
  `stitchLayers`, `lockLayerAndSubLayers` have no test.
- **Locking:** `PermissionToSave` — the property that distinguishes a system lock from a user lock —
  is never asserted; cross-type transitions untested. `loadLayerLockState` / `loadLayerMuteState`
  (the most complex functions) untested.
- **Muting:** 4 of 8 tests exercise raw `UsdStage::MuteLayer`, not `layerMuting.cpp`;
  `addMutedLayer` retention logic untested.
- **View:** `keyPressEvent` (Delete→remove, R→refresh), `selectLayerRequest`, expand/collapse
  recursion, multi-selection `getSelectedLayerItems` uncovered.
- **Buttons/menus:** Load-button routing, sibling-reorder path, Echo-Edit-Forwarding menu,
  stage-selector pin/content buttons, muted/multi-select disable conditions.
- **Save dialog:** the entire `onSaveAll` flow, including the observable error/empty-layer
  collection paths (`saveLayerUI` is stubbed to fail, but `_problemLayers`/`_emptyLayers` are still
  observable).
- **Shared stage:** referenced-layer read-only protection only tested on non-shared stages; the
  two-token (`adskSharedLayers` + `mayaSharedLayers`) merge is never exercised together.
- **LayerContentsWidget:** `expandAllValues=true` path and empty-layer placeholder untested; no
  test verifies displayed text matches the layer.
- **Item:** parent-propagation / false / unchecked branches of `appearsSystemLocked`, `appearsMuted`,
  `isTargetLayer`, `getActionButton` under-tested; `depth()` / `childrenVector()` untested.

---

## Clarity / nits
- Duplicate helpers (`findButtonByTooltip` vs `findButtonByTooltipFull`) and duplicate tests
  (`LoadLayerButton_ExistsAndEnabled` ×2; redundant roundtrip/visibility pairs).
- Inert setup lines implying false causality (`SetPermissionToEdit(false)` where `isSystemLocked()`
  ignores permission).
- Modal-dismiss timers (`QTimer::singleShot(100/200ms, close)`) are CI-flaky if the modal is not up
  yet — poll for the modal, or use `show()`+`reject()`.

## Suggested priorities
1. Fix the can't-fail tests (C1–C7, systemic #2/#3/#4) — done in this change for C1–C7.
2. Add `do → undo → redo` coverage and the three untested command classes.
3. Add `buildContextMenu` enable/disable tests and `PermissionToSave` lock-type assertions.
4. Sweep `hasCall`-only assertions to verify the target layer.
