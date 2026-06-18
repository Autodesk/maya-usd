# Old Layer Editor — Parity Test Status

## What This Is

The parity test suite runs the old Maya layer editor (`lib/usd/ui/layerEditor`) against the
same test logic written for the new shared editor (`lib/usdUfe/usd-layer-editor`). The goal is
to confirm the two editors behave identically on all shared functionality before the old editor
is retired.

---

## Test Architecture

### How the tests are structured

The new editor's tests live in `lib/usdUfe/usd-layer-editor/test/cpp/` as `*Logic.h` header
files. Each file contains the full test body — fixture classes and `TEST_F` macros — but does
not `#include` its own `testFixture.h` unconditionally. Instead:

```cpp
#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED
#include "testFixture.h"
#endif
```

The old editor's thin wrapper files in `lib/usd/ui/layerEditor/test/cpp/` pre-include the **old
editor's** `testFixture.h` (which sets `LAYER_EDITOR_TEST_FIXTURE_INCLUDED`) before including
the shared Logic.h. This causes the Logic.h to skip the new editor's fixture and inherit the old
editor's fixture instead — the same test body runs against a different implementation.

### Build target

`mayaUsdOldLayerEditorTests.mll` — a Maya plugin built with `BUILD_NEW_LAYER_EDITOR=OFF`.

The normal build uses `BUILD_NEW_LAYER_EDITOR=ON`. To run these parity tests you must rebuild
with `OFF`, which compiles the old widget classes into `mayaUsdUI`. The flag is set in
`ecg-maya-usd/build.py`.

### How to run

The test is registered as an interactive CTest entry (label `MayaUsdOldLEParity`). It runs
inside full interactive Maya so that `QApplication`, `MGlobal`, and `MQtUtil` are all properly
initialized. Running in Maya standalone (`mayapy`) does not work because standalone creates only
`QCoreApplication`, which is insufficient for `QWidget` construction.

```
ctest -L MayaUsdOldLEParity
```

---

## Test Counts

| Suite | New editor | Old editor parity | Notes |
|-------|-----------|------------------|-------|
| `testButtonsLogic.h` | 14 | 14 | |
| `testContextMenuLogic.h` | 23 | 23 | |
| `testLayerContentsWidgetLogic.h` | 6 | 6 | |
| `testLayerLockingLogic.h` | 13 | 13 | |
| `testLayerMutingLogic.h` | 8 | 8 | |
| `testLayerTreeItemLogic.h` | 33 | 30 | 3 excluded: `isIdenticalItem` API missing in old editor |
| `testLayerTreeModelLogic.h` | 20 | 20 | |
| `testLayerTreeViewLogic.h` | 16 | 16 | |
| `testLoadLayersDialogLogic.h` | 8 | 8 | |
| `testMenusAndStageLogic.h` | 9 | 6 | 3 excluded: `addStage`/`removeStage` API missing in old editor |
| `testReorderLogic.h` | 9 | 9 | |
| `testSaveLayersDialogLogic.h` | 12 | 12 | |
| `testSharedStageLogic.h` | 22 | 22 | 4 `ReferencedLayersFixture` excluded; 4 `MayaReferencedLayersFixture` added |
| `testLayerEditorCommands.cpp` | 31 | — | New-editor-only: undo-able command classes |
| `testLayerEditorWidgetLogic.h` | 12 | 10 | 2 fail: `selectLayers({})` doesn't clear currentIndex in old editor |
| `testStageSelectorWidgetLogic.h` | 11 | 11 | |
| `testSessionStateLogic.h` | 9 | 4 | 5 excluded: `displayLayerExpandAllValues` typo, `displayLayerHideIndices` missing, `_dccObjectPath` rename, `StubSessionState` → `OldEditorStubSessionState` |
| `testLayerTreeItemDelegateLogic.h` | 13 | 13 | |
| `testLayerTreeViewMouseLogic.h` | 5 | 5 | |
| `testPathCheckerLogic.h` | 5 | 3 | 2 excluded: `PathCheckerFileTestBase` uses `addStage()` missing in old editor |
| `testUsdSyntaxHighlighterLogic.h` | 5 | 5 | |
| `testComponentSaveWidgetLogic.h` | 18 | 16 | 2 excluded: `dccObjectPath()` method missing in old editor |
| `testLayerEditorWindowLogic.h` | 16 | 0 | All excluded: `AbstractLayerEditorCreator`/`LayerEditorWindow` are new-editor-only |
| **Total** | **314** | **267** | |

### Why 39 tests are absent from the parity suite

**6 tests excluded by `#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED` guards** — APIs that exist
only in the new editor:

- `testLayerTreeItemLogic.h` (3): `isIdenticalItem()` does not exist in the old editor.
- `testMenusAndStageLogic.h` (3): `SessionState::addStage()` / `removeStage()` do not exist in
  the old editor.

**4 `ReferencedLayersFixture` tests** use the DCC-agnostic `"adskSharedLayers"` metadata token.
The old editor only reads the legacy `"mayaSharedLayers"` token so these tests are guarded. The
equivalent behaviour is covered by `MayaReferencedLayersFixture` (see below).

**31 tests in `testLayerEditorCommands.cpp`** test the new editor's `UsdLayerEditor::*Cmd`
classes (undo-able commands, edit-target backup, system-lock callbacks). The old editor has no
equivalent — its commands run as MEL/MPxCommand via `layerEditorCommand.cpp`.

### `MayaReferencedLayersFixture` — runs on both editors

Four tests in `testSharedStageLogic.h` stamping `"mayaSharedLayers"` metadata (the token
`proxyShapeBase.cpp` actually writes). These are not guarded and must pass on both editors.
Both editors read this token; the new editor reads it via `UsdLayerEditorMetadata->MayaReferencedLayers`.

---

## Current Results: 265 pass, 2 fail

Last run: 2026-06-18, Maya 2027 interactive, `BUILD_NEW_LAYER_EDITOR=OFF`.
Old-editor suite expanded from 185 → 267 tests by sharing 9 new logic headers.

### Remaining Failure — `SaveLayersDialogTest.AllAsRelative_ToggleDoesNotCrash`

**What it tests:** In the Save Layers dialog, toggling the "Save All As Relative" checkbox does
not crash.

**Observed:** Test self-skips with `Skipped — No checkbox present (no anonymous layers in stub)`.
Our `JsonResultCollector` treats `GTEST_SKIP()` as a failure because it checks `result.passed()`.

**Root cause:** The old editor's `SaveLayersDialog::getLayersToSave()` calls:
```cpp
MayaUsd::utils::getLayersToSaveFromProxy(proxyPath, StageLayersToSave);
```
This looks up a Maya proxy shape node by DAG path. The stub's proxy path (`"stub_stage_0"`)
is not a real registered proxy shape, so the function returns nothing — no anonymous layers are
discovered, the checkbox is never created, the test skips.

The new editor's dialog uses `getLayersToSaveFromStage(stage, ...)` first, which inspects the
USD stage object directly and correctly finds the stub's anonymous sublayers.

**Classification:** Architectural — the old editor's dialog is tightly coupled to Maya's proxy
shape registry. Not a user-visible gap in production (real proxy shapes are always present). No
production impact.

**Options if it needs to be fixed:**
1. Update `JsonResultCollector` to distinguish `GTEST_SKIP()` from `GTEST_FAIL()`.
2. Guard the test with `#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED` (marks it new-editor-only).
3. Give the old editor stub a real proxy shape registration (significant work, probably not worth it).

---

## Issues Found and Fixed During Parity Work

### EMSUSD-3680 missing from branch

The `isIdenticalItem` skip optimization for `LayerTreeModel::rebuildModel()` landed on `dev`
one commit after this branch diverged. Cherry-picked commits `d70017ab9`, `4ce2af217`,
`7ec54ae42` into `feature/unify_layer_editors`.

The original commit's `isIdenticalItem` did not compare `_isSharedStage`. The port to the new
editor (`5632ef3d8`) added that check. Without it, flipping `_isSharedStage` and calling
`forceRefresh()` left stale items in the tree (the rebuild was skipped as "identical"), causing
`needsSaving()` to return the wrong value. Fixed in `lib/usd/ui/layerEditor/layerTreeItem.cpp`.

### `"adskSharedLayers"` token never written in production

The new editor reads `UsdLayerEditorMetadata->ReferencedLayers = "adskSharedLayers"` to
determine which layers are read-only. However, `proxyShapeBase.cpp` (the only production writer)
uses `MayaUsdMetadata->ReferencedLayers = "mayaSharedLayers"`. The new editor's referenced-layer
read-only protection was therefore **silently broken** — no production code ever wrote the token
it was reading.

Fixed: `UsdLayerEditorMetadata->MayaReferencedLayers = "mayaSharedLayers"` added to the new
editor's token registry; `layerTreeModel.cpp` now merges results from both tokens.

---

## Infrastructure Notes

### Key files changed to make parity tests work

| File | Change |
|------|--------|
| `lib/usd/ui/CMakeLists.txt` | `WINDOWS_EXPORT_ALL_SYMBOLS` on `mayaUsdUI`; `layerEditor/test/cpp` always added when `BUILD_TESTS` |
| `lib/usd/ui/layerEditor/qtUtils.h` | `#include <mayaUsdUI/ui/api.h>`; `MAYAUSD_UI_PUBLIC` on `utils` global so it exports from the DLL |
| `lib/usd/ui/layerEditor/layerContentsWidget.h` | `MAYAUSD_UI_PUBLIC` on class — exports `staticMetaObject` from DLL so `findChild<LayerContentsWidget*>` works across the DLL boundary |
| `lib/usd/ui/layerEditor/sessionState.h` | Same — `MAYAUSD_UI_PUBLIC` on class |
| `lib/usd/ui/layerEditor/layerTreeItem.cpp` | `_isSharedStage` check added to `isIdenticalItem()` (missing from cherry-picked upstream commit, present in the new editor port) |
| `lib/usd/ui/layerEditor/test/cpp/CMakeLists.txt` | Builds as `.mll` plugin; registered as `INTERACTIVE PYTHON_SCRIPT` CTest entry |
| `lib/usd/ui/layerEditor/test/cpp/pluginMain.cpp` | Sets `UsdLayerEditor::utils = new MayaQtUtils()` in `initializePlugin` so `DPIScale()` works before the layer editor window is opened |
| `lib/usd/ui/layerEditor/test/cpp/testOldLayerEditorParity.py` | `fixturesUtils.runTests(globals())` pattern for interactive Maya exit |
| `lib/usdUfe/usd-layer-editor/lib/tokens.h` | Added `MayaReferencedLayers = "mayaSharedLayers"` legacy token |
| `lib/usdUfe/usd-layer-editor/lib/layerTreeModel.cpp` | Reads both `"adskSharedLayers"` and `"mayaSharedLayers"` when building the shared-layer set |
| `lib/usdUfe/usd-layer-editor/test/cpp/testSharedStageLogic.h` | `ReferencedLayersFixture` guarded; `MayaReferencedLayersFixture` added |
| `lib/usdUfe/usd-layer-editor/test/cpp/testLayerMutingLogic.h` | `PXR_NS::UsdStage::CreateInMemory()` (typedef collision with old editor's `abstractCommandHook.h`) |
| `lib/usdUfe/usd-layer-editor/test/cpp/testMenusAndStageLogic.h` | 3 `addStage` tests guarded |
| `lib/usdUfe/usd-layer-editor/test/cpp/testLayerTreeItemLogic.h` | 3 `isIdenticalItem` tests guarded |
| All `*Logic.h` files | `#include "testFixture.h"` wrapped in `#ifndef LAYER_EDITOR_TEST_FIXTURE_INCLUDED` |

### Why `MAYAUSD_UI_PUBLIC` on `LayerContentsWidget` and `SessionState`

`QObject::findChild<T>()` compares `staticMetaObject` by **pointer identity**. When the test
plugin had `sessionState.h` and `layerContentsWidget.h` in its AUTOMOC sources, it generated a
second `staticMetaObject` for each class at a different address from the one in `mayaUsdUI.dll`.
`findChild` never matched, returning null. Adding `MAYAUSD_UI_PUBLIC` exports the data symbol
from the DLL; removing those headers from the plugin's AUTOMOC sources ensures the plugin uses
the DLL's single instance.

### Why interactive Maya is required

The old editor's widget code has hard Maya API dependencies (`MGlobal`, `MQtUtil`, the global
`UsdLayerEditor::utils` pointer) baked into construction paths. These are initialized by Maya at
startup. Running in Maya standalone (`mayapy`) creates only `QCoreApplication`, not
`QApplication`, which is insufficient for `QWidget` construction. Interactive Maya initializes
everything correctly before the test plugin is loaded.
