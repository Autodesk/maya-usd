# Layer Editor C++ Test Suite Design

**Date:** 2026-05-20  
**Status:** Approved  
**Branch:** `deboisj/unify_LE`

---

## Background

The USD Layer Editor has two implementations:
- **New (shared):** `lib/usdUfe/usd-layer-editor/lib/` — `UsdLayerEditorLib`, DCC-agnostic
- **Old (Maya):** `lib/usd/ui/layerEditor/` — `mayaUsdUI`, has Maya SDK dependencies

Both share identical abstract interfaces: `SessionState` (QObject) and `AbstractCommandHook` (pure-virtual). The widget code itself (`LayerEditorWidget`, `LayerTreeView`, `LayerTreeModel`, etc.) is DCC-agnostic in both.

Today only Python tests exist (running inside Maya). The goal is a standalone C++ GTest suite that covers interactive widget behavior, runs on developer machines without Maya, and acts as a behavioral contract for both implementations.

Migration strategy: get the suite to 100% pass on the old editor first, then run it against the new shared component and iterate to 100%.

---

## Approach

**GTest + QTest helpers.** GTest is already used in the codebase (`find_package(GTest REQUIRED)`). QTest helpers (`QTest::mouseClick`, `QTest::keyClick`, `QTRY_VERIFY`) provide battle-tested Qt event simulation. No GMock — hand-written functional stubs suffice.

Runtime: developer machines with a display. Headless/CI support is a future upgrade (design accommodates it via `QT_QPA_PLATFORM=offscreen`).

---

## File Layout

```
maya-usd/lib/usdUfe/usd-layer-editor/test/cpp/
├── CMakeLists.txt           ← defines both UsdLayerEditorNewTests and UsdLayerEditorOldTests
├── testMain.cpp             ← GTest main + QApplication singleton
├── testFixture.h
├── testFixture.cpp
├── stubCommandHook.h
├── stubCommandHook.cpp
├── stubSessionState.h
├── stubSessionState.cpp
├── testButtons.cpp
├── testContextMenu.cpp
├── testReorder.cpp
└── testMenusAndStage.cpp
```

Hooked into existing placeholder in `test/CMakeLists.txt` via `add_subdirectory(cpp)`.

---

## Build: Two Targets, One Source Set

The same source files compile into two executables. Only include paths and link targets differ.

### `UsdLayerEditorNewTests` — new shared component

```cmake
add_executable(UsdLayerEditorNewTests ${LAYER_EDITOR_TEST_SOURCES})
target_include_directories(UsdLayerEditorNewTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../lib
)
target_link_libraries(UsdLayerEditorNewTests PRIVATE
    UsdLayerEditorLib GTest::GTest Qt6::Test usd sdf tf
)
add_test(NAME UsdLayerEditorNewTests COMMAND UsdLayerEditorNewTests)
```

### `UsdLayerEditorOldTests` — old Maya editor, DCC-agnostic sources only

Rather than linking `mayaUsdUI` (which carries Maya SDK deps), the test target directly compiles the DCC-agnostic source files from `lib/usd/ui/layerEditor/`:

```cmake
set(OLD_LE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../lib/usd/ui/layerEditor)
set(OLD_LE_SOURCES
    ${OLD_LE_DIR}/layerEditorWidget.cpp
    ${OLD_LE_DIR}/layerTreeView.cpp
    ${OLD_LE_DIR}/layerTreeModel.cpp
    ${OLD_LE_DIR}/layerTreeItem.cpp
    ${OLD_LE_DIR}/layerTreeItemDelegate.cpp
    ${OLD_LE_DIR}/layerContentsWidget.cpp
    ${OLD_LE_DIR}/stageSelectorWidget.cpp
    ${OLD_LE_DIR}/dirtyLayersCountBadge.cpp
    ${OLD_LE_DIR}/generatedIconButton.cpp
    ${OLD_LE_DIR}/sessionState.cpp
    ${OLD_LE_DIR}/qtUtils.cpp
    ${OLD_LE_DIR}/stringResources.cpp
    ${OLD_LE_DIR}/usdSyntaxHighlighter.cpp
    ${OLD_LE_DIR}/loadLayersDialog.cpp
    ${OLD_LE_DIR}/saveLayersDialog.cpp
    ${OLD_LE_DIR}/resources.qrc
)
# Excluded: mayaCommandHook, mayaSessionState, mayaLayerEditorWindow, mayaQtUtils

add_executable(UsdLayerEditorOldTests ${LAYER_EDITOR_TEST_SOURCES} ${OLD_LE_SOURCES})
target_include_directories(UsdLayerEditorOldTests PRIVATE ${OLD_LE_DIR})
target_link_libraries(UsdLayerEditorOldTests PRIVATE
    GTest::GTest Qt6::Test usd sdf tf ufe
)
add_test(NAME UsdLayerEditorOldTests COMMAND UsdLayerEditorOldTests)
```

The stub `.h` files use bare includes (`"abstractCommandHook.h"`, `"sessionState.h"`). CMake `target_include_directories` routes each target to the right copy with no `#ifdef` needed in test code.

---

## Stubs

### `StubCommandHook : public AbstractCommandHook`

Implements all pure-virtual methods. Each method:
1. Appends a `Call{name, args}` to `std::vector<Call> _calls`
2. For USD-mutating methods (`insertSubLayerPath`, `removeSubLayerPath`, `muteSubLayer`, `lockLayer`, `discardEdits`, `moveSubLayerPath`): also applies the operation directly on the `SdfLayer`/`UsdStage` via the USD C++ API so the widget model reflects the change

Dialog-producing and undo bracket methods record only (no-op behavior).

Test helpers:
- `clearCalls()` — reset recorded calls
- `lastCall() -> Call&` — most recent call
- `callCount(std::string_view method) -> int`
- `hasCall(std::string_view method) -> bool`

### `StubSessionState : public SessionState`

Holds two `UsdStageRefPtr` created via `UsdStage::CreateInMemory()`. Each has one `SdfLayer::CreateAnonymous()` sublayer added.

- `commandHook()` → `&_commandHook` (a `StubCommandHook` member)
- `allStages()` / `selectedStages()` → the in-memory stages as `StageEntry` objects
- `loadLayersUI(...)` → returns a pre-baked anonymous layer identifier, no dialog
- `saveLayerUI(...)` → returns `true`, no dialog
- `setupCreateMenu(QMenu*)` → adds one dummy action so the menu is non-empty
- `addStage(UsdStageRefPtr)` — appends a stage and emits `stageListChangedSignal`
- `removeStage(std::string id)` — removes a stage and emits signal

---

## Test Fixture

```cpp
class LayerEditorTestFixture : public ::testing::Test {
protected:
    static void SetUpTestSuite();    // create QApplication if absent
    void SetUp() override;           // create stubs + widget + show + wait exposed
    void TearDown() override;        // destroy widget, clearCalls

    LayerTreeView*  layerTree();
    LayerTreeModel* treeModel();
    QModelIndex     firstSublayerIndex();
    QModelIndex     sessionLayerIndex();

    StubCommandHook              _commandHook;
    StubSessionState             _sessionState;
    std::unique_ptr<LayerEditorWidget> _widget;
};
```

`SetUpTestSuite` uses a static `int argc = 0` and creates the `QApplication` once for the whole binary. `SetUp` calls `_widget->show()` then `QTest::qWaitForWindowExposed(_widget.get())`.

---

## Test Cases

### Toolbar Buttons (`testButtons.cpp`) — 4 tests

| Test | Action | Assertion |
|------|--------|-----------|
| `NewLayerButton_Click_CallsInsertSubLayer` | click `_newLayer` button | `hasCall("insertSubLayerPath")` |
| `LoadLayerButton_Click_CallsInsertWithPath` | click `_loadLayer`, stub returns fake path | `lastCall().args contains stub path` |
| `SaveStageButton_EnabledWhenDirty` | mark stage dirty; call `updateButtonsOnIdle()` | `_saveStageButton->isEnabled()` |
| `SaveStageButton_Click_CallsSave` | click `_saveStageButton` | `hasCall("saveLayer")` |

### Layer Tree Context Menu (`testContextMenu.cpp`) — 13 tests

**Core actions:**

| Test | Right-click target | Action triggered | Assertion |
|------|--------------------|-----------------|-----------|
| `ContextMenu_SetEditTarget` | sublayer | "Set as Edit Target" | `hasCall("setEditTarget")` |
| `ContextMenu_MuteLayer` | sublayer | "Mute" | `hasCall("muteSubLayer")` |
| `ContextMenu_LockLayer` | sublayer | "Lock" | `hasCall("lockLayer")` |
| `ContextMenu_RemoveLayer` | sublayer | "Remove" | `hasCall("removeSubLayerPath")` |
| `ContextMenu_DiscardEdits` | sublayer | "Discard Edits" | `hasCall("discardEdits")` |
| `ContextMenu_StitchLayers` | sublayer | "Flatten Layer Stack" | `hasCall("stitchLayers")` |
| `ContextMenu_SelectPrimsWithSpec` | sublayer | "Select Prims With Spec" | `hasCall("selectPrimsWithSpec")` |
| `ContextMenu_PrintLayer` | sublayer | "Print to Script Editor" | `_sessionState.printLayerCallCount > 0` |

**Lock state:**

| Test | Setup | Assertion |
|------|-------|-----------|
| `ContextMenu_LockedLayer_DisablesMutatingActions` | lock layer via stub; right-click it | "Set as Edit Target", "Discard Edits", "Remove" actions are disabled |

**Layer-type aware:**

| Test | Right-click target | Assertion |
|------|--------------------|-----------|
| `ContextMenu_SessionLayer_HasNoRemoveAction` | session/root layer | "Remove" action is absent or disabled |
| `ContextMenu_Sublayer_HasRemoveAction` | sublayer | "Remove" action is present and enabled |
| `ContextMenu_SessionLayer_HasFlattenAllAction` | session layer | "Flatten All Layers" action present |
| `ContextMenu_Sublayer_NoFlattenAllAction` | sublayer | "Flatten All Layers" action absent |

### Drag-Drop Reorder (`testReorder.cpp`) — 2 tests

| Test | Action | Assertion |
|------|--------|-----------|
| `DragDrop_MoveRowDown_UpdatesModelOrder` | drag row 0 to row 1 | `moveSubLayerPath` recorded with correct from/to indices |
| `DragDrop_MoveRowUp_UpdatesModelOrder` | drag row 1 to row 0 | inverse of above |

Simulation strategy: use `QAbstractItemModel::dropMimeData` directly on the model (not mouse event simulation). Tree-view drag is notoriously sensitive to widget geometry and platform differences; testing at the model level is deterministic and still validates the move logic. If the command hook uses a `moveSubLayerPath`-equivalent method (name to confirm against the actual `AbstractCommandHook` API during implementation), assert it is called. If the API uses remove+insert, assert both are called in order.

### Window Menus + Stage Selector (`testMenusAndStage.cpp`) — 4 tests

| Test | Action | Assertion |
|------|--------|-----------|
| `ViewMenu_DisplayLayerContentsAction_Exists` | — | action present in View/top-level menu |
| `ViewMenu_DisplayLayerContents_Toggles_Panel` | trigger the action | `layerContentsWidget()->isVisible()` toggled |
| `StageSelector_ChangeStage_UpdatesTreeModel` | `_sessionState.addStage()`; change selector | `treeModel()->stage()` points to new stage |
| `StageSelector_PinButton_KeepsStageOnListChange` | click pin button; call `_sessionState.addStage()`; emit `stageListChangedSignal` | previously selected stage remains in the combo box selection |

---

## Test Count Summary

| Area | Tests |
|------|-------|
| Buttons | 4 |
| Context menu | 13 |
| Drag-drop reorder | 2 |
| Menus + stage selector | 4 |
| **Total** | **23** |

---

## Notes and Constraints

- `QT_NO_KEYWORDS` is set globally — use `Q_SIGNALS`/`Q_SLOTS`/`Q_EMIT` throughout (never `signals`/`slots`/`emit`)
- Qt6 (6.5+) primary; test CMakeLists should use same Qt version variable pattern as the library
- `QApplication` must be created before any `QWidget` — the `SetUpTestSuite` static ensures it is constructed once and not destroyed between test cases
- Context menu simulation: use `QTest::mouseClick(view->viewport(), Qt::RightButton, Qt::NoModifier, rowRect.center())` then `QApplication::activePopupWidget()` to grab the menu
- For headless future: add `QT_QPA_PLATFORM=offscreen` to the CTest environment, which requires `QTest::qWaitForWindowExposed` to be guarded with a platform check or replaced with `QCoreApplication::processEvents()`
