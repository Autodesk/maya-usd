# Layer Editor Test Parity — Design Spec

**Date:** 2026-05-21
**Branch:** `deboisj/unify_LE` (maya-usd submodule)
**Goal:** Expand `UsdLayerEditorNewTests` from 23 to ~190 tests by auditing every significant behavior in the old in-tree layer editor (`lib/usd/ui/layerEditor/`) and writing a corresponding GTest that verifies the new shareable implementation (`lib/usdUfe/usd-layer-editor/lib/`) reproduces it.

**Related specs:**
- `docs/superpowers/specs/2026-05-20-layer-editor-cpp-tests-design.md` — original 23-test suite design
- `docs/superpowers/specs/2026-05-19-usd-layer-editor-migration-design.md` — migration overview

---

## Constraints

- **No changes to production UI code.** All new code lives in `lib/usdUfe/usd-layer-editor/test/cpp/` only.
- **No new test executables.** All tests are added to the existing `UsdLayerEditorNewTests` target.
- **Dialog testing depth: medium.** Test construction, initial state, and non-modal interactions (checkbox toggling, row management, path manipulation). Auto-dismiss any `QDialog::exec()` calls with `QTimer::singleShot`.
- **No pixel/paint testing.** `layerTreeItemDelegate` paint methods are not testable without display output. Only geometry calculations (hit rects, text rect shrinkage) and `actionAppearsChecked` logic are tested.
- **Stub layer for Maya-specific APIs.** The `stageSelectorWidget` references Maya optionVars for pin state persistence; tests verify UI behavior only, not optionVar I/O.

---

## Test File Map

| File | Scope | Old editor source | Est. tests |
|------|-------|-------------------|-----------|
| Expand `testButtons.cpp` | button enable/disable matrix | `layerEditorWidget.h/.cpp` | +8 → 12 total |
| Expand `testContextMenu.cpp` | additional actions + setEditTarget guards | `layerEditorWindow.h/.cpp` | +11 → 24 total |
| Expand `testReorder.cpp` | canDropMimeData rules + dropMimeData ordering | `layerTreeModel.h/.cpp` | +10 → 12 total |
| Expand `testMenusAndStage.cpp` | pin button, displayLayerContents toggle | `stageSelectorWidget.h/.cpp` | +5 → 9 total |
| **New** `testLayerTreeItem.cpp` | all state queries + action button logic | `layerTreeItem.h/.cpp` | ~30 |
| **New** `testLayerTreeModel.cpp` | flags, rebuild, filtering, naming | `layerTreeModel.h/.cpp` | ~30 |
| **New** `testLayerTreeView.cpp` | memento, selection, dbl-click + delegate geometry | `layerTreeView.h/.cpp` + delegate | ~25 |
| **New** `testLayerContentsWidget.cpp` | setLayer, isEmpty, export truncation | `layerContentsWidget.h/.cpp` | ~8 |
| **New** `testSaveLayersDialog.cpp` | construction, rows, relative/absolute paths | `saveLayersDialog.h/.cpp` | ~12 |
| **New** `testLoadLayersDialog.cpp` | row add/remove, validation, inserter row | `loadLayersDialog.h/.cpp` | ~10 |
| **New** `testLayerLocking.cpp` | lock/unlock/systemlock transitions, persistence | `layerLocking.h/.cpp` | ~10 |
| **New** `testLayerMuting.cpp` | mute/unmute transitions, persistence | `layerMuting.h/.cpp` | ~8 |

**Estimated total: ~195 tests** (23 existing + ~172 new)

---

## Per-file Test Inventory

### Expand: `testButtons.cpp` (+8)

Derived from `layerEditorWidget.cpp` `updateNewLayerButton()` and `updateButtons()`:

```
NewLayerButton_DisabledWhenNoSelection
NewLayerButton_EnabledForRootLayer
NewLayerButton_EnabledForSessionLayer
NewLayerButton_DisabledWhenSelectionIsReadOnly
NewLayerButton_DisabledWhenSelectionIsSystemLocked
NewLayerButton_DisabledWhenSelectionAppearsSystemLocked
SaveButton_DisabledWhenNoLayersNeedSaving
DirtyBadge_CountMatchesNeedsSavingLayers
```

### Expand: `testContextMenu.cpp` (+11 → 24 total)

Derived from `layerEditorWindow.cpp` action preconditions:

```
ContextMenu_ClearLayer_CallsHook
ContextMenu_MergeWithSublayers_CallsHook
ContextMenu_MergeWithSublayers_BlockedWhenLayerHasNoSublayers
ContextMenu_MergeWithSublayers_BlockedWhenLayerIsLocked
ContextMenu_SaveEdits_CallsHook
ContextMenu_DiscardEdits_SkipsConfirmForAnonymousLayer
ContextMenu_DiscardEdits_SkipsConfirmForCleanLayer
SetEditTarget_BlockedWhenLayerIsMuted
SetEditTarget_BlockedWhenLayerIsReadOnly
SetEditTarget_BlockedWhenLayerIsLocked
SetEditTarget_BlockedWhenLayerIsSystemLocked
```

### Expand: `testReorder.cpp` (+10 → 12 total)

Derived from `layerTreeModel.cpp` `canDropMimeData()` and `dropMimeData()`:

```
DragDrop_CanDrop_ReturnsFalseForNonMoveAction
DragDrop_CanDrop_ReturnsFalseForWrongMimeType
DragDrop_CanDrop_ReturnsFalseForReadOnlyParent
DragDrop_CanDrop_ReturnsFalseForLockedParent
DragDrop_CanDrop_ReturnsFalseForRelativePathOntoAnonymousParent
DragDrop_CanDrop_ReturnsTrueForAbsolutePathOntoAnonymousParent
DragDrop_Drop_ProcessesLayersInReverseOrder
DragDrop_Drop_AdjustsRowIndexWhenMovingUp
DragDrop_Drop_NoAdjustWhenMovingDown
DragDrop_Drop_NoAdjustWhenMovingToDifferentParent
```

### Expand: `testMenusAndStage.cpp` (+5)

Derived from `stageSelectorWidget.cpp` pin and content toggle logic:

```
StageSelector_PinButton_TogglesPinState
StageSelector_Pinned_SelectsPinnedStageOnRebuild
StageSelector_Unpinned_SelectsLastUsedStageOnRebuild
StageSelector_InternalChangeFlagPreventsSessionStateUpdate
CollapseContent_TogglesDisplayLayerContentsInSessionState
```

---

### New: `testLayerTreeItem.cpp` (~30)

Derived from `layerTreeItem.cpp` state query methods and action button logic:

```
// isMuted / appearsMuted
IsMuted_ReturnsFalseWhenLayerNull
IsMuted_ReturnsStageMuteState
AppearsMuted_PropagatesUpParentChain
AppearsMuted_StopsAtFirstMutedParent

// isReadOnly
IsReadOnly_TrueForSharedLayer
IsReadOnly_TrueWhenParentIsShared
IsReadOnly_FalseForNonSharedSublayer

// isDirty / needsSaving
IsDirty_ReturnsLayerDirtyFlag
NeedsSaving_FalseForSessionLayer
NeedsSaving_FalseForUnsharedStage
NeedsSaving_TrueForDirtySharedNonSessionLayer
NeedsSaving_TrueForAnonymousSharedNonSessionLayer

// isLocked / appearsLocked / isSystemLocked / appearsSystemLocked
IsLocked_InvertsPermissionToEdit
AppearsLocked_ChecksImmediateParentOnly
AppearsLocked_FalseForRootItem
IsSystemLocked_IncludesIsReadOnly
AppearsSystemLocked_ChecksImmediateParentOnly

// isMovable
IsMovable_FalseForSessionLayer
IsMovable_FalseForRootLayer
IsMovable_FalseWhenAppearsMuted
IsMovable_FalseWhenAppearsLocked
IsMovable_FalseWhenIsLocked
IsMovable_FalseWhenIsSystemLocked
IsMovable_FalseWhenSublayerOfShared
IsMovable_TrueForNormalSublayer

// misc
IsTargetLayer_MirrorsInternalFlag
HasSubLayers_ChecksSdfSubLayerCount
PopulateChildren_SkipsCyclicLayer
GetActionButton_LockCheckedStateMatchesIsLocked
GetActionButton_MuteCheckedStateMatchesIsMuted
ActionButtons_MuteAppliesToSublayerOnly
ActionButtons_LockAppliesToRootAndSublayer
```

---

### New: `testLayerTreeModel.cpp` (~30)

Derived from `layerTreeModel.cpp` flags, drag-drop, rebuild, and filtering:

```
// flags / supported actions / MIME
Flags_DragEnabledOnlyForMovableItems
Flags_DropAlwaysEnabled
SupportedDropActions_OnlyMoveAction
MimeTypes_ReturnsTextPlain
MimeData_SerializesIdentifiersWithSemicolon
MimeData_HandlesInvalidLayerWithSubLayerPath

// canDropMimeData (already partly in testReorder.cpp — new ones here)
CanDrop_ReturnsFalseForNullMimeData

// rebuildModel / session layer visibility
Rebuild_ClearsAllRowsFirst
Rebuild_ShowsSessionLayerWhenDirty_AutoHide
Rebuild_ShowsSessionLayerWhenTarget_AutoHide
Rebuild_HidesSessionLayerWhenCleanNotTarget_AutoHide
Rebuild_AlwaysShowsSessionLayerWhenAutoHideFalse

// rebuildModelOnIdle deduplication
RebuildOnIdle_SchedulesOnce
RebuildOnIdle_SecondCallDoesNotDoubleSchedule

// updateTargetLayer
UpdateTargetLayer_DetectsSessionLayerAsTargetAndTriggersRebuild
UpdateTargetLayer_WalksAllRootItems

// filtering helpers
GetAllNeedsSavingLayers_FiltersByNeedsSaving
GetAllAnonymousLayers_ExcludesSessionLayer
FindNameForNewAnonymousLayer_IncrementsMaxIndex
FindNameForNewAnonymousLayer_HandlesGapsInNumbering

// edit target guard
SetEditTarget_OnlySetWhenLayerAccessible

// layer change notifications
LayerChanged_TriggersRebuildOnIdle
EditTargetChanged_TriggersUpdateTargetLayer
LayerDirtinessChanged_CallsFetchDataOnFoundItem
LayerDirtinessChanged_EmitsDataChangedOnRow0WhenItemNotFound
```

---

### New: `testLayerTreeView.cpp` (~25)

Derived from `layerTreeView.cpp` (memento, double-click, selection) and `layerTreeItemDelegate.cpp` (geometry only):

```
// LayerViewMemento
Memento_PreservesScrollPosition
Memento_PreservesExpandedStateByIdentifier
Memento_PreservesSelectionByIdentifier
Memento_PreservesCurrentItem
Memento_RestoreHandlesMissingItemsGracefully
Memento_RestoredAfterModelReset

// selection helpers
GetSelectedLayerItems_ReturnsAllSelected
CurrentLayerItem_ReturnsNullForInvalidIndex

// double-click behavior
DoubleClick_SkipsWhenLayerDoesNotNeedSaving
DoubleClick_SkipsWhenSystemLocked
DoubleClick_SkipsWhenAppearsSystemLocked
DoubleClick_CallsSaveEditsOtherwise

// mute / lock button dispatch
MuteButton_TogglesSelectedItems
LockButton_TogglesSelectedItems

// delegate geometry (hit rects)
Delegate_TargetIconRect_XOffsetIsArrowAreaWidth
Delegate_TargetIconRect_WidthIsCheckMarkAreaWidth
Delegate_TextRect_ShrinksForEachVisibleActionButton
Delegate_TextRect_IncludesLockButtonWhenLocked
Delegate_TextRect_IncludesLockButtonWhenSystemLocked
Delegate_TextRect_IncludesMuteButtonWhenMuted
Delegate_ActionAppearsChecked_LockMatchesIsLocked
Delegate_ActionAppearsChecked_MuteMatchesIsMuted
```

---

### New: `testLayerContentsWidget.cpp` (~8)

Derived from `layerContentsWidget.cpp` `setLayer()` and `exportPseudoLayer()`:

```
SetLayer_IsEmptyFalseAfterLayerWithContent
SetLayer_IsEmptyTrueForEmptyLayer
Clear_SetsIsEmptyTrue
IsEmpty_ReturnsFlagValue
ExportPseudoLayer_SucceedsForValidLayer
ExportPseudoLayer_TruncatesArraysLargerThanLimit
ExportPseudoLayer_TruncatesTimeSamplesLargerThanLimit
ExportPseudoLayer_RespectsArraySizeLimitParameter
```

---

### New: `testSaveLayersDialog.cpp` (~12)

Derived from `saveLayersDialog.cpp` construction, row logic, and checkbox behavior:

```
SaveLayersDialog_ConstructsFromSessionWithCorrectRowCount
SaveLayersDialog_HasSaveAllButton
SaveLayersDialog_HasCancelButton
SaveLayersDialog_HasAllAsRelativeCheckbox
SaveLayerPathRow_NeedToSaveAsRelative_MirrorsCheckbox
SaveLayerPathRow_SetSaveAsRelative_UpdatesDisplay
SaveLayerPathRow_CalculatesRelativeAnchorFromParentLayer
AllAsRelative_Checked_SetsAllRowsToRelative
AllAsRelative_Unchecked_SetsAllRowsToAbsolute
QuietlyUncheckAllAsRelative_DoesNotTriggerCallbacks
OkToSave_ValidatesPathsBeforeAccept
SaveLayersDialog_ExecDismissedByTimer_DoesNotHang
```

---

### New: `testLoadLayersDialog.cpp` (~10)

Derived from `loadLayersDialog.cpp` row management and validation:

```
LoadLayersDialog_StartsWithOneEmptyRowAndOneInserter
LayerPathRow_PathToUse_ReturnsEditText
LayerPathRow_SetPathToUse_SetsTextAndEnablesEdit
LayerPathRow_SetAsRowInserter_HidesTrashButton
LayerPathRow_SetAsRowInserter_DisablesTextEdit
LoadLayersDialog_AddRow_AppendsNewInserterRow
LoadLayersDialog_DeleteRow_RemovesRowFromLayout
LoadLayersDialog_AdjustScrollArea_ScalesUpToEightRows
LoadLayersDialog_OnOk_CollectsNonEmptyPaths
LoadLayersDialog_OnOk_RejectsInvalidPath
```

---

### New: `testLayerLocking.cpp` (~10)

Derived from `layerLocking.h/.cpp` centralized lock state API:

```
LockLayer_SetsLayerAsLocked
UnlockLayer_SetsLayerAsUnlocked
IsLayerLocked_ReflectsCurrentState
LockLayer_SystemLock_SetsSystemLocked
IsLayerSystemLocked_IncludesReadOnlyLayers
LockStatePersistsAcrossModelRebuild
ForgetLockedLayers_ClearsAllState
LockLayer_WithIncludeSubLayers_PropagatesChildren
UnlockLayer_WithIncludeSubLayers_PropagatesChildren
LockLayer_ToggleRoundtrip_RestoresOriginalState
```

---

### New: `testLayerMuting.cpp` (~8)

Derived from `layerMuting.h/.cpp` centralized mute state API:

```
MuteLayer_SetsLayerAsMuted
UnmuteLayer_SetsLayerAsUnmuted
IsMuted_ReflectsCurrentState
MuteStatePersistsAcrossModelRebuild
ForgetMutedLayers_ClearsAllState
AddMutedLayer_PreventUsdAutoCleanup
RemoveMutedLayer_AllowsUsdCleanup
MuteToggleRoundtrip_RestoresOriginalState
```

---

## Test Infrastructure

### Existing infrastructure (unchanged)
- `testFixture.h/.cpp` — `LayerEditorTestFixture` base class, `selectRow()`, model/view/window accessors
- `stubCommandHook.h/.cpp` — tracks method calls by name
- `stubSessionState.h/.cpp` — in-memory stage, `addStage()`, `autoHideSessionLayer()=false`
- `stubLayerEditorWindow.h` — exposes window action methods directly

### New helpers needed (test-only)

**`testUtils.h`** (new, test-only utility header)
- `makeAnonymousStage()` — creates `UsdStage::CreateInMemory()` with one anonymous sublayer already inserted
- `makeDirtyLayer(stage)` — sets a comment on root layer to mark it dirty
- `makeLockedLayer(layer)` — calls `layer->SetPermissionToEdit(false)`
- `dismissNextModal(ms=200)` — fires `QTimer::singleShot(ms)` to close any active modal

These four helpers eliminate boilerplate repeated across dozens of tests.

### CMake change
Add all new `test*.cpp` files to the `UsdLayerEditorNewTests` target in `lib/usdUfe/usd-layer-editor/test/cpp/CMakeLists.txt`. No new targets, no new build gates.

---

## Implementation Order

Tasks are ordered so each builds on verified infrastructure:

1. **`testUtils.h`** — write shared helpers first; no build change needed
2. **`testLayerTreeItem.cpp`** — pure state query tests, no window needed; validates core model logic
3. **`testLayerTreeModel.cpp`** — depends on item state; validates rebuild, flags, filtering
4. **Expand `testReorder.cpp`** — `canDropMimeData` / `dropMimeData` ordering rules
5. **`testLayerTreeView.cpp`** — memento + delegate geometry; depends on model being correct
6. **Expand `testButtons.cpp`** — button enable/disable; depends on item state logic
7. **Expand `testContextMenu.cpp`** — action preconditions; depends on state queries
8. **Expand `testMenusAndStage.cpp`** — stage selector pin/content toggle
9. **`testLayerContentsWidget.cpp`** — independent; just needs a stage and layer
10. **`testSaveLayersDialog.cpp`** — independent dialog tests
11. **`testLoadLayersDialog.cpp`** — independent dialog tests
12. **`testLayerLocking.cpp`** — centralized lock API
13. **`testLayerMuting.cpp`** — centralized mute API
14. **CMakeLists.txt update** — add all new files; build and run full suite

---

## Success Criteria

- All ~195 tests in `UsdLayerEditorNewTests` pass (exit 0 from relay test command with filter `UsdLayerEditorNewTests`)
- No changes to any file outside `lib/usdUfe/usd-layer-editor/test/cpp/`
- Each test name follows the convention `ClassName_Condition_ExpectedBehavior`
- No test depends on disk I/O, Maya runtime, or display output
