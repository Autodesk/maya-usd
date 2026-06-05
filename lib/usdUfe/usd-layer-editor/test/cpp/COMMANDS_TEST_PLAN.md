# Plan (c): `testLayerEditorCommands.cpp` undo/redo & coverage

> **STATUS: IMPLEMENTED.** All Tasks A–G landed in `testLayerEditorCommands.cpp`. New-editor suite
> now passes 250/250. These command tests are new-editor-only (not in the old-editor parity harness).

Plan for closing the command-test gaps identified in `TEST_REVIEW.md`. Target file:
`testLayerEditorCommands.cpp` (new-editor-only; no parity constraint). Source under test:
`LayerEditorCommands.h` / `layerEditorCommands.cpp`.

## Conventions to follow (match the existing file)
- One `::testing::Test` fixture per command; real `UsdStage::CreateInMemory()` + anonymous
  `SdfLayer`s; no widgets/stubs.
- Drive via `auto cmd = std::make_shared<XxxCmd>(...); cmd->execute(); cmd->undo(); cmd->redo();`.
- `execute()` calls `redo()` → `BaseCmd::redo()` throws `std::runtime_error` when `doIt` returns
  false. Negative paths assert with `EXPECT_THROW(cmd->execute(), std::runtime_error)` (see the
  existing `ReplaceSubPathCmdTest.DoIt_ReturnsFalse_WhenOldPathNotFound`).
- Fixtures that touch selection/mute/lock must keep the existing setup: `setStagePathAccessorFn`,
  `Ufe::GlobalSelection::initializeInstance`, `forgetLockedLayers`/`forgetMutedLayers`,
  `UIUtils::setErrorDisplayCallbackFunction([](std::string){})`, and reset them in `TearDown`.

## Coverage matrix (current → target)

| Command class | doIt | undo | **redo** | error path |
|---|---|---|---|---|
| ClearLayerCmd | ✅ | ✅ | ➕ add | — |
| DiscardEditCmd | ✅ | ✅ | ➕ add | — |
| FlattenLayerCmd | ➕ **none** | ➕ | ➕ | ➕ |
| SetEditTargetCmd | ➕ **none** | ➕ | ➕ | n/a |
| MuteLayerCmd | ✅ | ✅ | ➕ add | ➕ (null stage) |
| LockLayerCmd | ✅ | ✅ (single) | ➕ add | ➕ (size-mismatch undo guard; SystemLocked→undo=Unlocked) |
| InsertSubPathCmd | ✅ | ✅ | ➕ add | ➕ (OOB) |
| RemoveSubPathCmd | ✅ | ✅ | ➕ add | ➕ (OOB) + **edit-target redirect** |
| ReplaceSubPathCmd | ✅ | ✅ | ➕ add | ✅ |
| MoveSubPathCmd | ✅ | ✅ | ➕ add | ➕ (4 false branches) |
| AddAnonSubLayerCmd | ✅ | ✅ | ➕ **redo-identifier stability** | — |
| RefreshSystemLockLayerCmd | ➕ (only getter) | ➕ | ➕ | ➕ |
| StitchLayersCmd | ➕ **none** | ➕ | ➕ | ➕ (locked layer) |

## Tasks

### Task A — redo() round-trip on existing tests (highest value / lowest effort)
Add a `cmd->redo()` step + post-redo assertion to the existing do/undo tests so the full
do→undo→redo contract is verified. Concretely, extend (or add siblings to):
`ClearLayerCmd`, `DiscardEditCmd`, `MuteLayerCmd` (mute & unmute), `LockLayerCmd`,
`InsertSubPathCmd`, `RemoveSubPathCmd`, `ReplaceSubPathCmd`, `MoveSubPathCmd` (same- & cross-parent).
Pattern: `execute(); <assert applied>; undo(); <assert reverted>; redo(); <assert applied again>`.

**A-special — `AddAnonSubLayerCmd` redo-identifier stability.** The class deliberately reuses
`_anonIdentifier` across redo (header lines 447-455, "put back that same identifier, for later
commands"). Test:
```
cmd->execute(); auto id1 = cmd->addedLayer();
cmd->undo();
cmd->redo();    auto id2 = cmd->addedLayer();
EXPECT_EQ(id1, id2);                                  // identifier stable across redo
EXPECT_EQ(_parent->GetSubLayerPaths()[0], id2);
```

### Task B — `SetEditTargetCmd` (new fixture `SetEditTargetCmdTest`)
Header-only execute/undo/redo (lines 166-168). Setup: root + sublayer; target = root.
- `DoIt_SetsTarget`: `SetEditTargetCmd(stage, sublayer)`; execute; `EXPECT_EQ(GetEditTarget().GetLayer(), sublayer)`.
- `Undo_RestoresPreviousTarget`: after undo, target == root (the `oldTarget` captured at construction).
- `Redo_ReappliesTarget`: after redo, target == sublayer.

### Task C — `FlattenLayerCmd` (add to `BackupLayerCmdTest` or new fixture)
`BackupLayerBaseCmd::doIt` kFlattenLayer branch (cpp 138-171) opens a temp stage on the layer,
flattens the layer stack, `TransferContent` back. Setup: rootLayer with a sublayer that defines a
prim (`SdfCreatePrimInLayer(sub, SdfPath("/Foo"))`), `rootLayer->InsertSubLayerPath(sub)`.
- `DoIt_FlattensSublayerContentIntoLayer`: execute `FlattenLayerCmd(rootLayer)`; assert
  `rootLayer->GetObjectAtPath(SdfPath("/Foo"))` now exists inline (was only in the sublayer).
- `Undo_RestoresPreFlattenContent`: undo; assert `/Foo` no longer inline in rootLayer
  (`restoreLayer` transfers the pre-flatten backup, which had no `/Foo`).
- `Redo_ReflattensContent`: redo; `/Foo` inline again.
- *(Note: requires `setStagesProvider` like `BackupLayerCmdTest` if edit-target backup matters; the
  flatten itself does not.)*

### Task D — `StitchLayersCmd` (new fixture `StitchLayersCmdTest`)
`doIt` (cpp 855+) sorts by strength, validates locks, `UsdUtilsStitchLayers(strongest, weak)`,
removes weak from its parent inside a `UsdUndoBlock(&_undoItem)`. Setup: stage root with two
sublayers strong/weak, each defining a distinct prim.
- `DoIt_MergesWeakIntoStrongAndRemovesWeak`: construct with `{strongId, weakId}`; execute; assert
  strong layer now contains the weak's prim, and weak is removed from the parent's sublayer list.
- `Undo_RestoresOriginalLayers`: undo (via `_undoItem`); assert original prim placement + the weak
  sublayer is back in the parent.
- `Redo_RestitchesLayers`.
- `DoIt_ReturnsFalse_WhenAnyLayerIsLocked`: lock the weak layer (`PermissionToEdit==false`);
  `EXPECT_THROW(cmd->execute(), std::runtime_error)`; assert nothing changed.
- **Infra risk:** uses `UsdUfe::UsdUndoManager`/`UsdUndoBlock`; confirm it operates in the test
  process. If init is required, document/add it in the fixture.

### Task E — `RefreshSystemLockLayerCmd` do/undo (new fixture `RefreshSystemLockLayerCmdTest`)
`_refreshLayerSystemLock` (cpp 787-829) checks **on-disk file write access** and builds
`LockLayerCmd`s to set/clear the system lock; `doIt` runs them, `undoIt` undoes them.
- `DoIt_SystemLocksReadOnlyFileLayer`: write a layer to a temp `.usda`, `chmod`/set read-only,
  `SdfLayer::FindOrOpen` it, add as sublayer; run the command; assert `isLayerSystemLocked(layer)`.
- `Undo_RestoresLockState`: undo; assert no longer system-locked.
- `Redo_ReappliesSystemLock`.
- **Infra risk / decision needed:** this requires a real read-only file on disk (anonymous in-memory
  layers have no file, so the system-lock branch is a no-op). Options: (1) create+chmod a temp file
  (cross-platform care on Windows — use file attributes), or (2) cover only the do/undo *plumbing*
  on an anonymous layer (asserts it runs without error but not the lock transition — low value).
  **Recommend (1)**; flag if temp-file infra is undesirable.

### Task F — error paths
- `InsertSubPathCmd`/`RemoveSubPathCmd` out-of-bounds index → `validateAndReportIndex` returns false
  → `EXPECT_THROW(cmd->execute(), std::runtime_error)`; assert layer unchanged. (Requires the
  `UIUtils::setErrorDisplayCallbackFunction` no-op to avoid `bad_function_call`.)
- `MoveSubPathCmd` four `return false` branches (cpp 633-708): (a) subpath not found in old parent,
  (b) same-parent index out-of-bounds, (c) cross-parent index out-of-bounds, (d) subpath already
  exists in new parent. Each: `EXPECT_THROW(execute(), std::runtime_error)` + layers unchanged.

### Task G — `RemoveSubPathCmd` edit-target redirect (the documented crash-fix)
`doIt` (cpp 477-532): if the removed sublayer is/contains the current edit target and the target is
not reachable elsewhere, it stores `_editTargetPath` and retargets to root; `undoIt` (cpp 557-562)
re-inserts and restores the edit target.
- Setup: root → sub; `stage->SetEditTarget(sub)`.
- `DoIt_RetargetsToRootWhenRemovingEditTargetLayer`: `RemoveSubPathCmd(stage, root, 0)`; execute;
  `EXPECT_EQ(GetEditTarget().GetLayer(), root)`.
- `Undo_RestoresEditTargetToReinsertedLayer`: undo; `EXPECT_EQ(GetEditTarget().GetLayer(), sub)`.

### Bonus cleanup (from TEST_REVIEW.md)
`BackupEditTargetsTest.WithoutProvider_EditTargetNotRestoredOnUndo` is near-tautological (asserts USD
kept a stale ref). Either delete or convert to a positive assertion. Low priority.

## Estimated additions
~22–26 new TEST_F cases across Tasks A–G (A ≈ 8–10 redo additions; B 3; C 3; D 4–5; E 3; F 6; G 2).
All new-editor-only. Verify via the relay: `build` then `test UsdLayerEditorNewTests`.

## Open questions for you
1. **Task E temp-file infra** — OK to create+chmod a temp `.usda` for the system-lock test, or keep
   it to plumbing-only on anonymous layers?
2. **Scope** — do all of A–G, or a subset (e.g. A+B+C+D first, defer E/F/G)?
3. **redo() style** — extend existing tests in place, or add separate `*_Redo_*` tests (keeps the
   existing do/undo tests untouched)? I lean toward separate tests for clarity.


## Open questions for you
1. **Task E temp-file infra** — OK to create+chmod a temp `.usda` for the system-lock test, or keep
   it to plumbing-only on anonymous layers?
->   Yes you can 
2. **Scope** — do all of A–G, or a subset (e.g. A+B+C+D first, defer E/F/G)?
-> All
3. **redo() style** — extend existing tests in place, or add separate `*_Redo_*` tests (keeps the
   existing do/undo tests untouched)? I lean toward separate tests for clarity.
-> You can extend existing tests