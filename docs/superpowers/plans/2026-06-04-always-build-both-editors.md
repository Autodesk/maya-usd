# Always-Build Both Layer Editors Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove the `BUILD_NEW_LAYER_EDITOR` CMake switch so `UsdLayerEditorLib` and `mayaUsdOldLayerEditorTests` always build, with zero changes to legacy widget source files.

**Architecture:** Six CMake files lose their `BUILD_NEW_LAYER_EDITOR` guards and always take the former ON-path. The test target `mayaUsdOldLayerEditorTests` gains the legacy widget sources as private compilation units so it no longer depends on `mayaUsdUI` for them. `ecg-maya-usd/build.py` drops the now-redundant `-DBUILD_NEW_LAYER_EDITOR=ON` flag.

**Tech Stack:** CMake, C++/Qt, MSVC (Windows primary target), Maya plugin system (.mll)

---

## Files Changed

| File | What changes |
|---|---|
| `maya-usd/CMakeLists.txt:28` | Delete `option(BUILD_NEW_LAYER_EDITOR ...)` |
| `maya-usd/lib/usdUfe/CMakeLists.txt:185-188` | Remove gate around `add_subdirectory(usd-layer-editor)` |
| `maya-usd/lib/usdUfe/usd-layer-editor/test/cpp/CMakeLists.txt:41-83` | Remove `if(BUILD_NEW_LAYER_EDITOR)` / `endif()` wrapper |
| `maya-usd/lib/mayaUsd/CMakeLists.txt:274-277` | Remove `if(BUILD_NEW_LAYER_EDITOR)` guard |
| `maya-usd/test/lib/CMakeLists.txt:143` | Remove `BUILD_NEW_LAYER_EDITOR AND` from compound condition |
| `maya-usd/lib/usd/ui/layerEditor/CMakeLists.txt:18-84` | Remove `if/else/endif`, keep ON-path only |
| `maya-usd/lib/usd/ui/layerEditor/test/cpp/CMakeLists.txt` | Add legacy sources, replace link libs, fix includes, add `MAYAUSD_UI_EXPORT` |
| `ecg-maya-usd/build.py:1621-1622` | Remove `-DBUILD_NEW_LAYER_EDITOR=ON` append |

---

### Task 1: Remove the CMake option and simple guards (5 files)

**Files:**
- Modify: `maya-usd/CMakeLists.txt:28`
- Modify: `maya-usd/lib/usdUfe/CMakeLists.txt:185-188`
- Modify: `maya-usd/lib/usdUfe/usd-layer-editor/test/cpp/CMakeLists.txt:41-83`
- Modify: `maya-usd/lib/mayaUsd/CMakeLists.txt:274-277`
- Modify: `maya-usd/test/lib/CMakeLists.txt:143`

- [ ] **Step 1: Delete the option declaration**

In `maya-usd/CMakeLists.txt`, delete line 28:

```
option(BUILD_NEW_LAYER_EDITOR "Build and use the shared usd-layer-editor component instead of the in-tree mayaUsd layer editor." ON)
```

The line above it (27) and below it (29) remain untouched.

- [ ] **Step 2: Always build the usd-layer-editor subdirectory**

In `maya-usd/lib/usdUfe/CMakeLists.txt`, replace:

```cmake
# Shared layer editor component (Qt-dependent). Off by default during migration.
if(BUILD_NEW_LAYER_EDITOR)
    add_subdirectory(usd-layer-editor)
endif()
```

with:

```cmake
add_subdirectory(usd-layer-editor)
```

- [ ] **Step 3: Always build UsdLayerEditorNewTests**

In `maya-usd/lib/usdUfe/usd-layer-editor/test/cpp/CMakeLists.txt`, replace:

```cmake
# ------------------------------------------------------------------------------
# UsdLayerEditorNewTests
# Links UsdLayerEditorLib (the DCC-agnostic shared component).
# Built only when BUILD_NEW_LAYER_EDITOR=ON.
# ------------------------------------------------------------------------------
if(BUILD_NEW_LAYER_EDITOR)
    set(NEW_LE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)
```

with:

```cmake
# ------------------------------------------------------------------------------
# UsdLayerEditorNewTests
# Links UsdLayerEditorLib (the DCC-agnostic shared component).
# ------------------------------------------------------------------------------
set(NEW_LE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)
```

Then delete the closing `endif()` at the end of the file (line 83, after the `mayaUsd_add_test` block).

- [ ] **Step 4: Always link UsdLayerEditorLib from mayaUsd**

In `maya-usd/lib/mayaUsd/CMakeLists.txt`, replace:

```cmake
if(BUILD_NEW_LAYER_EDITOR)
    target_link_libraries(${PROJECT_NAME} PRIVATE UsdLayerEditorLib Qt::Core)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MAYAUSD_USE_SHARED_LAYER_EDITOR=1)
endif()
```

with:

```cmake
target_link_libraries(${PROJECT_NAME} PRIVATE UsdLayerEditorLib Qt::Core)
target_compile_definitions(${PROJECT_NAME} PRIVATE MAYAUSD_USE_SHARED_LAYER_EDITOR=1)
```

- [ ] **Step 5: Drop BUILD_NEW_LAYER_EDITOR from the shared test guard**

In `maya-usd/test/lib/CMakeLists.txt`, replace:

```cmake
if(BUILD_NEW_LAYER_EDITOR AND ENABLE_SHARED_LAYER_EDITOR_TESTS)
```

with:

```cmake
if(ENABLE_SHARED_LAYER_EDITOR_TESTS)
```

- [ ] **Step 6: Commit**

```bash
git -C maya-usd add \
  CMakeLists.txt \
  lib/usdUfe/CMakeLists.txt \
  lib/usdUfe/usd-layer-editor/test/cpp/CMakeLists.txt \
  lib/mayaUsd/CMakeLists.txt \
  test/lib/CMakeLists.txt
git -C maya-usd commit -m "build: remove BUILD_NEW_LAYER_EDITOR option, always build UsdLayerEditorLib"
```

---

### Task 2: Make mayaUsdUI always use the new editor

**Files:**
- Modify: `maya-usd/lib/usd/ui/layerEditor/CMakeLists.txt:18-84`

- [ ] **Step 1: Replace the if/else/endif with the ON-path only**

In `maya-usd/lib/usd/ui/layerEditor/CMakeLists.txt`, replace the entire block from line 18 through line 84:

```cmake
if(BUILD_NEW_LAYER_EDITOR)
    # When using the shared usd-layer-editor component, only the Maya-specific
    # wiring files build here. All shared sources are provided by UsdLayerEditorLib.
    target_sources(${PROJECT_NAME}
        PRIVATE
            batchSaveLayersUIDelegate.cpp
            batchSaveLayersUIDelegate.h
            mayaCommandHook.cpp
            mayaCommandHook.h
            mayaLayerEditorWindow.cpp
            mayaLayerEditorWindow.h
            mayaSessionState.cpp
            mayaSessionState.h
            mayaQtUtils.cpp
            mayaQtUtils.h
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE UsdLayerEditorLib)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MAYAUSD_USE_SHARED_LAYER_EDITOR=1)
else()
    target_sources(${PROJECT_NAME}
        PRIVATE
            batchSaveLayersUIDelegate.cpp
            componentSaveWidget.cpp
            componentSaveWidget.h
            dirtyLayersCountBadge.cpp
            generatedIconButton.cpp
            layerContentsWidget.cpp
            layerContentsWidget.h
            layerEditorWidget.cpp
            layerEditorWidget.h
            layerTreeItem.cpp
            layerTreeItem.h
            layerTreeItemDelegate.cpp
            layerTreeItemDelegate.h
            layerTreeModel.cpp
            layerTreeModel.h
            layerTreeView.cpp
            layerTreeView.h
            loadLayersDialog.cpp
            loadLayersDialog.h
            mayaCommandHook.cpp
            mayaCommandHook.h
            mayaLayerEditorWindow.cpp
            mayaLayerEditorWindow.h
            mayaSessionState.cpp
            mayaSessionState.h
            mayaQtUtils.cpp
            mayaQtUtils.h
            pathChecker.cpp
            pathChecker.h
            qtUtils.cpp
            qtUtils.h
            resources.qrc
            saveLayersDialog.cpp
            saveLayersDialog.h
            sessionState.cpp
            sessionState.h
            stageSelectorWidget.cpp
            stageSelectorWidget.h
            stringResources.cpp
            stringResources.h
            usdSyntaxHighlighter.cpp
            usdSyntaxHighlighter.h
            warningDialogs.cpp
            warningDialogs.h
    )
endif()
```

with:

```cmake
target_sources(${PROJECT_NAME}
    PRIVATE
        batchSaveLayersUIDelegate.cpp
        batchSaveLayersUIDelegate.h
        mayaCommandHook.cpp
        mayaCommandHook.h
        mayaLayerEditorWindow.cpp
        mayaLayerEditorWindow.h
        mayaSessionState.cpp
        mayaSessionState.h
        mayaQtUtils.cpp
        mayaQtUtils.h
)
target_link_libraries(${PROJECT_NAME} PRIVATE UsdLayerEditorLib)
target_compile_definitions(${PROJECT_NAME} PRIVATE MAYAUSD_USE_SHARED_LAYER_EDITOR=1)
```

- [ ] **Step 2: Commit**

```bash
git -C maya-usd add lib/usd/ui/layerEditor/CMakeLists.txt
git -C maya-usd commit -m "build: mayaUsdUI always uses UsdLayerEditorLib, drop legacy widget sources from it"
```

---

### Task 3: Restructure mayaUsdOldLayerEditorTests

**Files:**
- Modify: `maya-usd/lib/usd/ui/layerEditor/test/cpp/CMakeLists.txt`

This is the key change. The test loses its `mayaUsdUI` link (which no longer has the legacy widget symbols) and gains the legacy widget sources compiled directly as private sources. `MAYAUSD_UI_EXPORT` is added so `MAYAUSD_UI_PUBLIC` on legacy classes expands to `__declspec(dllexport)` rather than `__declspec(dllimport)`.

- [ ] **Step 1: Add the LEGACY_SOURCES variable after OLD_LE_OWN_SOURCES**

In `maya-usd/lib/usd/ui/layerEditor/test/cpp/CMakeLists.txt`, replace:

```cmake
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Path to the new editor's test/cpp — contains *Logic.h headers.
set(NEW_LE_TEST_CPP
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../usdUfe/usd-layer-editor/test/cpp)

# ── Sources ────────────────────────────────────────────────────────────────────
set(OLD_LE_OWN_SOURCES
```

with:

```cmake
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Sources ────────────────────────────────────────────────────────────────────
set(OLD_LE_OWN_SOURCES
```

(This removes the now-unused `NEW_LE_TEST_CPP` variable.)

- [ ] **Step 2: Add LEGACY_SOURCES after the OLD_LE_OWN_SOURCES block**

After the closing `)` of `set(OLD_LE_OWN_SOURCES ...)`, insert:

```cmake

# Legacy widget sources compiled directly into this test DLL. This gives the
# test a self-contained copy of the old editor, avoiding ODR conflicts with
# UsdLayerEditorLib which exports the new editor's versions of the same classes.
set(LEGACY_SOURCES
    ../../componentSaveWidget.cpp
    ../../dirtyLayersCountBadge.cpp
    ../../generatedIconButton.cpp
    ../../layerContentsWidget.cpp
    ../../layerEditorWidget.cpp
    ../../layerTreeItem.cpp
    ../../layerTreeItemDelegate.cpp
    ../../layerTreeModel.cpp
    ../../layerTreeView.cpp
    ../../loadLayersDialog.cpp
    ../../pathChecker.cpp
    ../../qtUtils.cpp
    ../../resources.qrc
    ../../saveLayersDialog.cpp
    ../../sessionState.cpp
    ../../stageSelectorWidget.cpp
    ../../stringResources.cpp
    ../../usdSyntaxHighlighter.cpp
    ../../warningDialogs.cpp
)
```

- [ ] **Step 3: Add LEGACY_SOURCES to add_library**

Replace:

```cmake
add_library(${TARGET_NAME} SHARED
    ${OLD_LE_OWN_SOURCES}
)
```

with:

```cmake
add_library(${TARGET_NAME} SHARED
    ${OLD_LE_OWN_SOURCES}
    ${LEGACY_SOURCES}
)
```

- [ ] **Step 4: Add MAYAUSD_UI_EXPORT to compile definitions**

Replace:

```cmake
target_compile_definitions(${TARGET_NAME}
    PRIVATE
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)
```

with:

```cmake
target_compile_definitions(${TARGET_NAME}
    PRIVATE
        MAYAUSD_UI_EXPORT
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)
```

- [ ] **Step 5: Fix include directories — remove new editor paths**

Replace:

```cmake
# Include path order is critical:
#   1. OLD editor test dir FIRST — shim headers shadow new editor versions.
#   2. NEW editor test dir SECOND — *Logic.h headers and testUtils.h.
#   3. OLD editor lib sources — layerTreeItem.h, layerTreeModel.h, etc.
target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NEW_LE_TEST_CPP}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
)
```

with:

```cmake
target_include_directories(${TARGET_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../
)
```

- [ ] **Step 6: Fix link libraries — drop mayaUsdUI and UsdLayerEditorLib**

Replace:

```cmake
target_link_libraries(${TARGET_NAME}
    PRIVATE
        mayaUsdUI
        $<$<BOOL:${BUILD_NEW_LAYER_EDITOR}>:UsdLayerEditorLib>
        GTest::GTest
        Qt::Core Qt::Gui Qt::Widgets
        sdf tf usd
        ${MAYA_LIBRARIES}
)
```

with:

```cmake
target_link_libraries(${TARGET_NAME}
    PRIVATE
        mayaUsd
        GTest::GTest
        Qt::Core Qt::Gui Qt::Widgets
        sdf tf usd
        ${MAYA_LIBRARIES}
)
```

- [ ] **Step 7: Commit**

```bash
git -C maya-usd add lib/usd/ui/layerEditor/test/cpp/CMakeLists.txt
git -C maya-usd commit -m "build: mayaUsdOldLayerEditorTests compiles legacy sources directly, drops mayaUsdUI link"
```

---

### Task 4: Remove the flag from ecg-maya-usd/build.py

**Files:**
- Modify: `ecg-maya-usd/build.py:1621-1622`

- [ ] **Step 1: Delete the BUILD_NEW_LAYER_EDITOR append**

In `ecg-maya-usd/build.py`, delete these two lines (1621-1622):

```python
        # Migration: enable shared usd-layer-editor component in usdUfe.
        build_args.append('-DBUILD_NEW_LAYER_EDITOR=ON')
```

- [ ] **Step 2: Commit**

```bash
git -C /d/repos/agent_repos/ecg-maya-usd add build.py
git -C /d/repos/agent_repos/ecg-maya-usd commit -m "build: remove BUILD_NEW_LAYER_EDITOR=ON from build args, option no longer exists"
```

---

### Task 5: Configure, build, and verify

**Files:** None — verification only.

- [ ] **Step 1: Run configure**

```bash
result=$(python3 /d/repos/agent_repos/ecg-maya-usd/_host_command/relay_client.py run configure \
  --db /d/repos/agent_repos/ecg-maya-usd/_host_command/relay.db \
  --commands-json /d/repos/agent_repos/ecg-maya-usd/_host_command/commands.json)
python3 -c "import json,sys; d=json.loads(sys.argv[1]); print('Exit:', d['exit_code']); print(d['stderr'] or d['stdout'][-3000:])" "$result"
```

Expected: exit 0. Any CMake error about unknown variable `BUILD_NEW_LAYER_EDITOR` means a guard was missed — grep for it and fix.

- [ ] **Step 2: Run build**

```bash
result=$(python3 /d/repos/agent_repos/ecg-maya-usd/_host_command/relay_client.py run build \
  --db /d/repos/agent_repos/ecg-maya-usd/_host_command/relay.db \
  --commands-json /d/repos/agent_repos/ecg-maya-usd/_host_command/commands.json)
python3 -c "import json,sys; d=json.loads(sys.argv[1]); print('Exit:', d['exit_code']); print(d['stdout'])" "$result" | grep -i "error LNK\|error C[0-9]\|FAILED\|Build succeeded" | tail -20
```

Expected: "Build succeeded" with no LNK or C-level errors.

Common failures and fixes:
- `LNK2019: unresolved external symbol` in `mayaUsdOldLayerEditorTests` → a legacy source file was missed from `LEGACY_SOURCES` in Task 3
- `LNK2005: already defined` → `MAYAUSD_UI_EXPORT` causes a symbol to be exported from both `mayaUsdUI` and the test DLL — check that no `mayaUsdUI` headers are leaking `dllexport` into the test's link
- CMake error `target not found: UsdLayerEditorLib` → the `add_subdirectory(usd-layer-editor)` change in Task 1 was not applied correctly

- [ ] **Step 3: Verify both targets built**

```bash
python3 -c "import json,sys; d=json.loads(sys.argv[1]); print(d['stdout'])" "$result" | grep -i "UsdLayerEditorLib\|mayaUsdOldLayerEditor"
```

Expected: Lines showing both `UsdLayerEditorLib.dll` and `mayaUsdOldLayerEditorTests.mll` were linked.

- [ ] **Step 4: Run the parity test**

```bash
result=$(python3 /d/repos/agent_repos/ecg-maya-usd/_host_command/relay_client.py run test mayaUsdOldLayerEditorTests \
  --db /d/repos/agent_repos/ecg-maya-usd/_host_command/relay.db \
  --commands-json /d/repos/agent_repos/ecg-maya-usd/_host_command/commands.json)
python3 -c "import json,sys; d=json.loads(sys.argv[1]); print('Exit:', d['exit_code']); print(d['stdout'][-3000:])" "$result"
```

Expected: same result as before the refactor — 1 test failure (`SaveLayersDialogTest.AllAsRelative_ToggleDoesNotCrash: No checkbox present`) which is a pre-existing test defect, not a regression.
