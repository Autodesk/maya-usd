# Always-Build Both Layer Editors — Design Spec

**Date:** 2026-06-04  
**Branch:** deboisj/unify_LE  
**Status:** Approved

---

## Problem

`BUILD_NEW_LAYER_EDITOR` is a CMake switch that picks one of two layer editor implementations at configure time:

- **ON** — `mayaUsdUI` uses `UsdLayerEditorLib` (the new DCC-agnostic shared component). Legacy widget sources are excluded from `mayaUsdUI`.
- **OFF** — `mayaUsdUI` compiles the legacy widget sources directly. `UsdLayerEditorLib` is not built at all.

This causes two problems:

1. **The parity test breaks with ON.** `mayaUsdOldLayerEditorTests` is a Maya plugin that tests the legacy editor's behavior. When `ON`, the legacy sources aren't compiled anywhere, so the test can't find the symbols it needs (`callMethodOnSelectionNoDelay`, `LayerTreeItem::removeSubLayer(QWidget*)`, `utils`, etc.).

2. **The switch itself is unnecessary complexity.** The production path is settled: `mayaUsdUI` should always use `UsdLayerEditorLib`. There is no need for a configure-time choice.

---

## Goal

- `UsdLayerEditorLib` always builds.
- `mayaUsdUI` always uses the new editor.
- `mayaUsdOldLayerEditorTests` always works, testing the legacy implementation in isolation.
- No `BUILD_NEW_LAYER_EDITOR` option in CMake.
- Zero changes to legacy widget source files (`layerTreeItem.cpp`, `layerTreeView.cpp`, etc.).

---

## Design

### 1. Remove `BUILD_NEW_LAYER_EDITOR` from CMake

Delete the `option(BUILD_NEW_LAYER_EDITOR ...)` line from `maya-usd/CMakeLists.txt`.

All `if(BUILD_NEW_LAYER_EDITOR)` guards in CMake files become unconditional (always take the former ON-path). See affected files below.

### 2. `UsdLayerEditorLib` always builds

`maya-usd/lib/usdUfe/CMakeLists.txt`: Remove the `if(BUILD_NEW_LAYER_EDITOR)` gate around `add_subdirectory(usd-layer-editor)`. The subdirectory (and thus `UsdLayerEditorLib`) always builds.

`maya-usd/lib/usdUfe/usd-layer-editor/test/cpp/CMakeLists.txt`: Remove the `if(BUILD_NEW_LAYER_EDITOR)` gate around `UsdLayerEditorNewTests`. That standalone test always builds.

### 3. `mayaUsdUI` always uses the new editor

`maya-usd/lib/usd/ui/layerEditor/CMakeLists.txt`: Remove the `if/else/endif` block. Keep only the former ON-path:

- Only the Maya wiring files compile into `mayaUsdUI` (`mayaLayerEditorWindow`, `mayaCommandHook`, `mayaSessionState`, `mayaQtUtils`, `batchSaveLayersUIDelegate`).
- `target_link_libraries(... UsdLayerEditorLib)` is unconditional.
- `MAYAUSD_USE_SHARED_LAYER_EDITOR=1` is always defined.

### 4. `mayaUsd` library always links `UsdLayerEditorLib`

`maya-usd/lib/mayaUsd/CMakeLists.txt`: Remove the `if(BUILD_NEW_LAYER_EDITOR)` guard. Always link `UsdLayerEditorLib` and always define `MAYAUSD_USE_SHARED_LAYER_EDITOR=1` for this target.

### 5. New editor tests always registered

`maya-usd/test/lib/CMakeLists.txt`: Remove the `if(BUILD_NEW_LAYER_EDITOR ...)` gate. Shared layer editor tests are always registered with CTest.

### 6. `MAYAUSD_USE_SHARED_LAYER_EDITOR` guards in C++ — left as-is

Files like `mayaCommandHook.cpp`, `mayaLayerEditorWindow.cpp`, and `layerEditorCommand.cpp` contain `#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)` guards. Since the symbol is now always defined, the `#else` branches become permanently dead code. They are left in place for now — cleaning them up is a separate mechanical task with no functional impact.

### 7. `mayaUsdOldLayerEditorTests` — compile legacy sources directly

This is the key structural change.

**Current setup:** The test links `mayaUsdUI` to get the legacy widget classes. With `mayaUsdUI` now pointing at the new editor, this no longer works.

**New setup:** The test compiles the legacy widget sources as its own **private** `target_sources`. It does not link `mayaUsdUI` or `UsdLayerEditorLib`.

**Sources added to `mayaUsdOldLayerEditorTests`** (referenced by path, no changes to the files):

```
../../layerTreeItem.cpp           ../../layerTreeModel.cpp
../../layerTreeView.cpp           ../../layerEditorWidget.cpp
../../sessionState.cpp            ../../layerContentsWidget.cpp
../../loadLayersDialog.cpp        ../../saveLayersDialog.cpp
../../stageSelectorWidget.cpp     ../../qtUtils.cpp
../../pathChecker.cpp             ../../stringResources.cpp
../../layerTreeItemDelegate.cpp   ../../dirtyLayersCountBadge.cpp
../../componentSaveWidget.cpp     ../../generatedIconButton.cpp
../../usdSyntaxHighlighter.cpp    ../../warningDialogs.cpp
../../resources.qrc
```

**DLL export macro:** The legacy widget headers mark classes with `MAYAUSD_UI_PUBLIC`, which expands to `__declspec(dllexport)` only when `MAYAUSD_UI_EXPORT` is defined. `mayaUsdUI` defines this privately; the test target must define it too so the legacy sources it compiles are exported from the test DLL rather than incorrectly treated as imports.

```cmake
target_compile_definitions(${TARGET_NAME}
    PRIVATE
        MAYAUSD_UI_EXPORT
        ...existing definitions...
)
```

**Link libraries** (replacing `mayaUsdUI` + conditional `UsdLayerEditorLib`):

```cmake
target_link_libraries(${TARGET_NAME}
    PRIVATE
        mayaUsd         # for abstractLayerEditorWindow.h and USD/Maya base APIs
        GTest::GTest
        Qt::Core Qt::Gui Qt::Widgets
        sdf tf usd
        ${MAYA_LIBRARIES}
)
```

**Include paths** (replacing the three-dir setup with two-dir):

```cmake
target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}      # test stubs (stubSessionState.h, etc.)
        ${CMAKE_CURRENT_SOURCE_DIR}/../../  # legacy editor headers (layerTreeItem.h, etc.)
)
```

The new editor test/lib dirs are removed. The test compiles entirely against legacy headers. This makes `callMethodOnSelectionNoDelay`, `QWidget*` method params, `utils`, and `diplayLayerExpandAllValues` (typo in old `SessionState`) all resolve to their legacy implementations compiled directly into the test DLL — no linker errors.

**No ODR conflict:** The legacy `UsdLayerEditor::LayerTreeItem` lives only in the test `.mll`. The new `UsdLayerEditor::LayerTreeItem` lives only in `UsdLayerEditorLib.dll`. They are in separate DLLs and are never mixed within a single binary.

### 8. `ecg-maya-usd/build.py`

Remove `-DBUILD_NEW_LAYER_EDITOR=ON` from the `--build-args` list. The option no longer exists.

---

## Files Changed

| File | Change |
|---|---|
| `maya-usd/CMakeLists.txt` | Remove `option(BUILD_NEW_LAYER_EDITOR ...)` |
| `maya-usd/lib/usdUfe/CMakeLists.txt` | Remove `if(BUILD_NEW_LAYER_EDITOR)` gate on `add_subdirectory(usd-layer-editor)` |
| `maya-usd/lib/usdUfe/usd-layer-editor/test/cpp/CMakeLists.txt` | Remove `if(BUILD_NEW_LAYER_EDITOR)` gate on `UsdLayerEditorNewTests` |
| `maya-usd/lib/usd/ui/layerEditor/CMakeLists.txt` | Remove `if/else/endif`, keep ON-path unconditionally |
| `maya-usd/lib/mayaUsd/CMakeLists.txt` | Remove `if(BUILD_NEW_LAYER_EDITOR)` guard |
| `maya-usd/test/lib/CMakeLists.txt` | Remove `if(BUILD_NEW_LAYER_EDITOR)` guard |
| `maya-usd/lib/usd/ui/layerEditor/test/cpp/CMakeLists.txt` | Add legacy sources, replace link libs, fix include paths |
| `ecg-maya-usd/build.py` | Remove `-DBUILD_NEW_LAYER_EDITOR=ON` from build args |

**Files NOT changed:** All `.cpp` and `.h` files in `lib/usd/ui/layerEditor/` (legacy widget sources), all `.cpp` and `.h` files in `lib/usdUfe/usd-layer-editor/lib/` (new editor sources).

---

## Non-Goals

- Cleaning up `#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)` / `#if !defined(...)` guards in C++ — deferred.
- Deleting legacy widget sources — happens later, after parity is confirmed.
- Changing the new editor's API — out of scope.
