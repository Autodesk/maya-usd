# USD Layer Editor Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Get the shared `usd-layer-editor` component compiling within maya-usd as a new CMake target behind a toggle, wire up Maya's command hooks and tests against it, and set up the iterative tracking framework to port 161 diverged commits.

**Architecture:** New CMake target `UsdLayerEditorLib` under `lib/usdUfe/`, gated by `BUILD_NEW_LAYER_EDITOR=OFF` (default). When `ON`, the old `lib/usd/ui/layerEditor/` keeps only Maya-wiring files and links against the new target. DCC-specific behavior (Maya MEL commands, Component Creator, Edit Forwarding) is injected via `SessionState` and `AbstractCommandHook` virtual hooks. Migration progress tracked in `MIGRATION.md`.

**Tech Stack:** C++ (Qt 5/6, USD, UFE), Python (Maya tests), CMake.

**Scope of THIS plan:** Phases 1–3 (infrastructure) + Phase 4 setup (MIGRATION.md generation + 1 example port to establish the recipe). The actual porting of the remaining 160 commits becomes follow-up plans driven by MIGRATION.md. Phase 5 (flip default + delete shared sources from old editor) is a separate plan after all of Phase 4 is complete.

> **Plan pivot (2026-05-20):** Tasks 5 and 6 (the `mayaUsdUI` ↔ `UsdLayerEditorLib` bridge) were attempted and reverted. The shared component's API has diverged substantively from maya-usd's (renames like `_proxyShapePath` → `_dccObjectPath`, removed Maya-specific setters in `SessionState`, `StringResources` made DCC-agnostic, `LayerLockType` moved namespaces, `callMethodOnSelectionNoDelay` removed, globals replaced by accessors, menu callback signatures changed). Bridging the Maya wiring to the shared API isn't a "fix the includes" job — it's substantial porting work that should follow Phase 4. **Current architecture:** `UsdLayerEditorLib` builds as a parallel artifact (intended for 3dsmax + as the porting target); `mayaUsdUI` keeps its existing in-tree layer editor sources with no regression. Tasks 5 and 6 are deferred until Phase 4 brings the shared component up to maya-usd's API maturity, at which point the bridge becomes a straightforward include/link step.

**Build command (used in every verification step):**

```bash
ls /d/repos/agent_repos/ecg-maya-usd/_host_command/in-progress.json /d/repos/agent_repos/ecg-maya-usd/_host_command/result.json 2>/dev/null && echo "FILES EXIST — wait" || true
rm -f /d/repos/agent_repos/ecg-maya-usd/_host_command/result.json
echo '{"repo":"ecg-maya-usd","command":"build"}' > /d/repos/agent_repos/ecg-maya-usd/_host_command/request.json
_wait_start=$(date +%s)
until [ -f /d/repos/agent_repos/ecg-maya-usd/_host_command/result.json ]; do
  sleep 1
  _now=$(date +%s)
  _hb=$(cat /d/repos/agent_repos/ecg-maya-usd/_host_command/heartbeat 2>/dev/null | tr -d '\r\n' | sed 's/[^0-9]//g')
  if [ -n "$_hb" ] && [ "$_hb" -gt 0 ] 2>/dev/null; then _age=$(( _now - _hb )); else _age=$(( _now - _wait_start )); fi
  if [ $_age -gt 60 ]; then echo "ERROR: relay not responding" >&2; exit 1; fi
done
cat /d/repos/agent_repos/ecg-maya-usd/_host_command/result.json
rm /d/repos/agent_repos/ecg-maya-usd/_host_command/result.json
rm -f /d/repos/agent_repos/ecg-maya-usd/_host_command/in-progress.json
```

A non-zero `"exit_code"` means failure — read `"stderr"` from the JSON.

---

## File Structure

**Created:**
- `lib/usdUfe/usd-layer-editor/MIGRATION.md` — commit-by-commit migration tracking, resume point across sessions
- `lib/usdUfe/usd-layer-editor/test/mayaLayerEditorTestSetup.py` — Maya DCC hook bindings for shared component tests
- `lib/usdUfe/usd-layer-editor/test/testMayaUsdSharedLayerEditor.py` — ctest wrapper that calls setup then runs `UsdLayerEditorTest`

**Modified:**
- `CMakeLists.txt` (root) — declare `BUILD_NEW_LAYER_EDITOR` option
- `lib/usdUfe/CMakeLists.txt` — conditional `add_subdirectory(usd-layer-editor)`
- `lib/usdUfe/usd-layer-editor/lib/CMakeLists.txt` — rewritten to use parent's Qt/UFE/usdUfe targets (was standalone `find_package`)
- `lib/usd/ui/layerEditor/CMakeLists.txt` — when `BUILD_NEW_LAYER_EDITOR=ON`, exclude the shared sources from `target_sources` (keep only `maya*` files)
- `lib/usd/ui/layerEditor/mayaCommandHook.h/.cpp` — adjust `#include` paths to `UsdLayerEditorLib` headers
- `lib/usd/ui/layerEditor/mayaSessionState.h/.cpp` — same
- `lib/usd/ui/layerEditor/mayaLayerEditorWindow.h/.cpp` — same
- `lib/usd/ui/layerEditor/mayaQtUtils.h/.cpp` — same
- `test/lib/CMakeLists.txt` — register `testMayaUsdSharedLayerEditor.py`

---

## Phase 1: Build System (`UsdLayerEditorLib` target compiles)

### Task 1: Declare the `BUILD_NEW_LAYER_EDITOR` option

**Files:**
- Modify: `CMakeLists.txt` (root, around line 27)

- [ ] **Step 1: Add the option declaration**

Open `/d/repos/agent_repos/ecg-maya-usd/maya-usd/CMakeLists.txt`. Find the line:

```cmake
option(BUILD_PXR_PLUGIN "Build the Pixar USD plugin and libraries." ON)
```

Insert immediately AFTER:

```cmake
option(BUILD_NEW_LAYER_EDITOR "Build and use the shared usd-layer-editor component instead of the in-tree mayaUsd layer editor." OFF)
```

- [ ] **Step 2: Verify regression build still passes (option OFF)**

Run the build command above (see header). Expected: `"exit_code": 0`, no new build outputs (the option default `OFF` should be inert).

- [ ] **Step 3: Commit**

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd add CMakeLists.txt
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd commit -m "Add BUILD_NEW_LAYER_EDITOR cmake option

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

### Task 2: Rewrite `usd-layer-editor/lib/CMakeLists.txt` to use parent's targets

The current file does standalone `find_package(Qt5/Qt6/USD/UFE/USDUFE)`. In maya-usd, Qt is already found via `Qt::` alias targets, UFE via `${UFE_LIBRARY}`, and `usdUfe` is a direct cmake target. We need to wire to those.

**Files:**
- Modify: `lib/usdUfe/usd-layer-editor/lib/CMakeLists.txt`

- [ ] **Step 1: Replace the full contents of `lib/usdUfe/usd-layer-editor/lib/CMakeLists.txt`**

Path: `/d/repos/agent_repos/ecg-maya-usd/maya-usd/lib/usdUfe/usd-layer-editor/lib/CMakeLists.txt`

Replace the entire file with:

```cmake
#
# Copyright 2026 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# (Apache 2.0 boilerplate omitted for brevity — keep the existing header from
#  lib/usdUfe/CMakeLists.txt verbatim when writing this file)
#

# This CMakeLists is invoked from lib/usdUfe/CMakeLists.txt when
# BUILD_NEW_LAYER_EDITOR is ON. Qt, UFE, USD and usdUfe are already
# found/defined by the parent build — do not call find_package here.

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(UsdLayerEditorLib SHARED
    abstractCommandHook.h
    abstractLayerEditorWindow.h
    batchSaveLayersUIDelegate.cpp
    batchSaveLayersUIDelegate.h
    customLayerData.cpp
    customLayerData.h
    dirtyLayersCountBadge.cpp
    dirtyLayersCountBadge.h
    generatedIconButton.cpp
    generatedIconButton.h
    layerEditorAPI.h
    LayerEditorCommands.h
    layerEditorCommands.cpp
    layerEditorWidget.cpp
    layerEditorWidget.h
    layerEditorWidgetManager.cpp
    layerEditorWidgetManager.h
    layerEditorWindow.cpp
    layerEditorWindow.h
    layerLocking.cpp
    layerLocking.h
    layerMuting.cpp
    layerMuting.h
    layers.cpp
    layers.h
    layerTreeItem.cpp
    layerTreeItem.h
    layerTreeItemDelegate.cpp
    layerTreeItemDelegate.h
    layerTreeModel.cpp
    layerTreeModel.h
    layerTreeView.cpp
    layerTreeView.h
    layerTreeViewStyle.h
    loadLayersDialog.cpp
    loadLayersDialog.h
    pathChecker.cpp
    pathChecker.h
    resources.qrc
    saveLayersDialog.cpp
    saveLayersDialog.h
    sessionState.cpp
    sessionState.h
    stageSelectorWidget.cpp
    stageSelectorWidget.h
    stringResources.cpp
    stringResources.h
    tokens.cpp
    tokens.h
    ufeCommandHook.cpp
    ufeCommandHook.h
    utilFileSystem.cpp
    utilFileSystem.h
    utilOptions.h
    utilQT.cpp
    utilQT.h
    utilSerialization.cpp
    utilSerialization.h
    utilString.h
    utilUI.cpp
    utilUI.h
    warningDialogs.cpp
    warningDialogs.h
)

# QT_NO_KEYWORDS: match mayaUsdUI convention (forbids `signals`/`slots`/`emit` macros)
get_target_property(qt_core_aliased Qt::Core ALIASED_TARGET)
if (qt_core_aliased)
    set_target_properties(${qt_core_aliased} PROPERTIES INTERFACE_COMPILE_DEFINITIONS QT_NO_KEYWORDS)
else()
    set_target_properties(Qt::Core PROPERTIES INTERFACE_COMPILE_DEFINITIONS QT_NO_KEYWORDS)
endif()

target_compile_definitions(UsdLayerEditorLib
    PRIVATE
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

mayaUsd_compile_config(UsdLayerEditorLib)

target_include_directories(UsdLayerEditorLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${UFE_INCLUDE_DIR}
)

target_link_libraries(UsdLayerEditorLib
    PUBLIC
        usdUfe
        ${UFE_LIBRARY}
        sdf
        tf
        usd
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

# run-time search paths (mirrors usdUfe)
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_install_rpath(rpath UsdLayerEditorLib)
endif()

install(TARGETS UsdLayerEditorLib
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:UsdLayerEditorLib>
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL
    )
endif()
```

Note: the Apache license header at the top of the file should match the format used by other CMakeLists in `lib/usdUfe/` — copy that boilerplate verbatim.

- [ ] **Step 2: Commit (not yet wired in — won't affect build)**

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd add lib/usdUfe/usd-layer-editor/lib/CMakeLists.txt
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd commit -m "Rewrite usd-layer-editor CMakeLists for maya-usd build system

Replaces standalone find_package() calls with references to the parent
build's already-found targets (Qt, UFE, usdUfe). The target is renamed
to UsdLayerEditorLib. Not yet wired in — needs add_subdirectory under
lib/usdUfe/.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

### Task 3: Hook `usd-layer-editor` into `lib/usdUfe/CMakeLists.txt`

**Files:**
- Modify: `lib/usdUfe/CMakeLists.txt`
- Create: `lib/usdUfe/usd-layer-editor/CMakeLists.txt` (top-level for the new target)

- [ ] **Step 1: Create the top-level CMakeLists.txt for usd-layer-editor**

Path: `/d/repos/agent_repos/ecg-maya-usd/maya-usd/lib/usdUfe/usd-layer-editor/CMakeLists.txt`

Contents:

```cmake
#
# Copyright 2026 Autodesk
#
# (Apache 2.0 boilerplate — match style of sibling files)
#

# Top-level CMakeLists for the shared usd-layer-editor component
# (DCC-agnostic). Built when BUILD_NEW_LAYER_EDITOR is ON.

add_subdirectory(lib)

# Python bindings and tests are added in later tasks.
```

- [ ] **Step 2: Add the conditional `add_subdirectory` in `lib/usdUfe/CMakeLists.txt`**

Open `/d/repos/agent_repos/ecg-maya-usd/maya-usd/lib/usdUfe/CMakeLists.txt`. Find the section at the bottom:

```cmake
add_subdirectory(base)
add_subdirectory(python)
add_subdirectory(resources)
add_subdirectory(ufe)
add_subdirectory(undo)
add_subdirectory(utils)
```

Add immediately after `add_subdirectory(utils)`:

```cmake

# Shared layer editor component (Qt-dependent). Off by default during migration.
if(BUILD_NEW_LAYER_EDITOR)
    add_subdirectory(usd-layer-editor)
endif()
```

- [ ] **Step 3: Run the build with `BUILD_NEW_LAYER_EDITOR=OFF` (default) to confirm no regression**

Run the build command. Expected: `"exit_code": 0`, no new artifacts mention `UsdLayerEditorLib`.

- [ ] **Step 4: Commit**

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd add lib/usdUfe/CMakeLists.txt lib/usdUfe/usd-layer-editor/CMakeLists.txt
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd commit -m "Hook usd-layer-editor into usdUfe build under BUILD_NEW_LAYER_EDITOR

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

### Task 4: Verify the new target compiles standalone with `BUILD_NEW_LAYER_EDITOR=ON`

The new target's compile-time dependency on the existing `mayaUsdUI` layer editor sources doesn't exist yet — at this point `UsdLayerEditorLib` and `lib/usd/ui/layerEditor/` will produce duplicate symbols if both link into the same final binary. That's expected; for this task we only need the new target itself to compile.

**Files:** none (build-only verification).

- [ ] **Step 1: Ask the user to enable `BUILD_NEW_LAYER_EDITOR=ON`**

The build defaults are controlled outside the agent. Before running this verification, ask the user:

> "I'm about to verify the new `UsdLayerEditorLib` target compiles. Please toggle `BUILD_NEW_LAYER_EDITOR=ON` in your build configuration (e.g., add `-DBUILD_NEW_LAYER_EDITOR=ON` to your cmake args / `build.py` invocation) and run a clean reconfigure. Let me know when ready."

Wait for confirmation before continuing.

- [ ] **Step 2: Trigger a configure step**

The host relay has a `configure` command. Run it via:

```bash
echo '{"repo":"ecg-maya-usd","command":"configure"}' > /d/repos/agent_repos/ecg-maya-usd/_host_command/request.json
# (use the wait loop pattern from the plan header)
```

Inspect the result: the configure stdout should mention `UsdLayerEditorLib` being added.

- [ ] **Step 3: Trigger a build**

Run the build command. Compile errors are *expected* at the linker stage (duplicate symbols between `UsdLayerEditorLib` and the old layer editor). The success criterion for this task is: **`UsdLayerEditorLib` itself compiled cleanly** (each `.cpp` file in `usd-layer-editor/lib/` produced an `.obj`/`.o`).

Look in the build output for entries like `usd-layer-editor/lib/sessionState.cpp -> sessionState.obj`. If those appear without compile errors, the new target builds.

If you see compile errors *inside* the new target (not link errors against the old one), they reveal differences between the standalone shared component's expectations and maya-usd's parent build (e.g., missing include path, Qt version mismatch). Fix them inline in `usd-layer-editor/lib/CMakeLists.txt` — add what's missing, then re-build.

- [ ] **Step 4: Commit any fixes**

If you made fixes in Step 3, commit them:

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd add lib/usdUfe/usd-layer-editor/lib/CMakeLists.txt
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd commit -m "Fix UsdLayerEditorLib compile issues against maya-usd parent build

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

If no fixes were needed, skip the commit.

---

## Phase 2: Maya Wiring (old editor links against new target when ON) — DEFERRED

> **Status (2026-05-20):** Tasks 5 and 6 were attempted on commits `dbfb9b436` and `9a13cd9f3`, and reverted (`eb6c01235`, `0708dd9e8`). The shared component's API has diverged too far from maya-usd's for a simple include/decoration bridge. These tasks resume after Phase 4 brings the shared component up to parity with maya-usd's mature API. The text of Tasks 5 and 6 below is preserved for reference but is **not** active work right now — skip to Phase 3.

### Task 5: Conditionally exclude shared sources from `lib/usd/ui/layerEditor/`

When `BUILD_NEW_LAYER_EDITOR=ON`, the old layer editor's `CMakeLists.txt` must drop all shared `.cpp/.h` files from its `target_sources` — keeping only the Maya-specific wiring files. This eliminates the duplicate-symbol issue from Task 4.

**Files:**
- Modify: `lib/usd/ui/layerEditor/CMakeLists.txt`

- [ ] **Step 1: Open the current file**

Path: `/d/repos/agent_repos/ecg-maya-usd/maya-usd/lib/usd/ui/layerEditor/CMakeLists.txt`.

The current contents are:

```cmake
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
```

- [ ] **Step 2: Replace with a conditional version**

Replace the `target_sources(...)` block above with:

```cmake
if(BUILD_NEW_LAYER_EDITOR)
    # When using the shared usd-layer-editor component, only the Maya-specific
    # wiring files build here. All shared sources are provided by UsdLayerEditorLib.
    target_sources(${PROJECT_NAME}
        PRIVATE
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

The remainder of the file (the MSVC qfloat16 fix, promoted headers, install) stays unchanged.

- [ ] **Step 3: Build with `BUILD_NEW_LAYER_EDITOR=OFF` (regression check)**

Run the build command. Expected: `"exit_code": 0`, same as before. No change in artifacts.

- [ ] **Step 4: Commit**

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd add lib/usd/ui/layerEditor/CMakeLists.txt
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd commit -m "Conditionally drop shared sources from mayaUsdUI layer editor

When BUILD_NEW_LAYER_EDITOR=ON, the in-tree layer editor compiles only
its Maya-wiring files and links against UsdLayerEditorLib. When OFF,
the build is unchanged.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

### Task 6: Adjust `#include` paths in the Maya wiring files

When `BUILD_NEW_LAYER_EDITOR=ON`, the four `maya*` files include sibling files (`#include "abstractCommandHook.h"`, `#include "sessionState.h"`, etc.) that no longer exist in `lib/usd/ui/layerEditor/`. We need them to find the headers in `UsdLayerEditorLib`'s `include` interface.

Since `UsdLayerEditorLib`'s `target_include_directories(... PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})` exposes `lib/usdUfe/usd-layer-editor/lib/` as an include path to consumers, plain `#include "abstractCommandHook.h"` will find the right header automatically when linking against `UsdLayerEditorLib`. No `#include` changes should be required — but we need to verify.

**Files:** None up front (verification first; fix only if needed).

- [ ] **Step 1: Build with `BUILD_NEW_LAYER_EDITOR=ON`**

Same procedure as Task 4 Step 1: ask the user to confirm `BUILD_NEW_LAYER_EDITOR=ON` is set in their build config, then trigger a configure + build.

- [ ] **Step 2: Inspect compile errors (expected on first try)**

Two failure categories are likely:

  a) **Header not found** — a `maya*` file `#include`s a sibling that wasn't auto-resolved through `UsdLayerEditorLib`'s public include dirs. Fix: either change the `#include` to use angle-brackets with a clearer path, or confirm `UsdLayerEditorLib` is on the `PRIVATE` link line for the old editor (we set this in Task 5 step 2).

  b) **API drift** — `MayaCommandHook` overrides a method on `AbstractCommandHook` that has a different signature now in the shared component (e.g., a parameter was added during the divergence). Fix: log this commit in `MIGRATION.md` as a `needs-hook` item, add the missing virtual in `abstractCommandHook.h` with a default no-op, and re-implement in `MayaCommandHook`.

- [ ] **Step 3: Apply fixes file-by-file**

For each compile error, edit the corresponding `maya*.cpp/h` (or add the missing virtual to `abstractCommandHook.h` / `sessionState.h` in the shared component if it's category (b)). Re-run the build between each fix to make progress visible.

- [ ] **Step 4: Build succeeds with `BUILD_NEW_LAYER_EDITOR=ON`**

Final criterion: `"exit_code": 0` with the new target linked. The `mayaUsdPlugin.mll` should appear in the build output as before.

- [ ] **Step 5: Build with `BUILD_NEW_LAYER_EDITOR=OFF` (regression check)**

Ask the user to flip back to OFF and re-build. Confirm the old build still works — no fix in this task should have broken the off-path.

- [ ] **Step 6: Commit**

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd add -u lib/
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd commit -m "Wire maya layer editor to UsdLayerEditorLib (BUILD_NEW_LAYER_EDITOR=ON)

Adjusts includes / adds missing virtual hooks so the in-tree Maya wiring
files (mayaCommandHook, mayaSessionState, mayaLayerEditorWindow,
mayaQtUtils) compile against the shared usd-layer-editor component.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Phase 3: Test Infrastructure (shared component tests run from Maya)

### Task 7: Create the Maya DCC hook setup script

**Files:**
- Create: `lib/usdUfe/usd-layer-editor/test/mayaLayerEditorTestSetup.py`

- [ ] **Step 1: Write the failing test first**

Wait — there's no test infrastructure yet to write a failing test against. The "test" for this task is: a Maya Python session can `import mayaLayerEditorTestSetup; mayaLayerEditorTestSetup.setup()` without exceptions, and after setup the five `UsdLayerEditorTest._*` static methods are populated.

We'll define this verification in Task 8 (the wrapper test). For Task 7, we just produce the setup script.

- [ ] **Step 2: Inspect the 3dsmax setup script as the reference**

The 3dsmax script (provided by the user) does:

```python
from layer_editor_test import UsdLayerEditorTest

def createStage(rootFile): ...
def resetScene(): mxs.resetMaxFile(mxs.Name("noprompt"))
def undo(): pymxs.run_undo()
def redo(): pymxs.run_redo()
def openStageLayerEditor(rootFile): ...

def setup():
    UsdLayerEditorTest._createStage = staticmethod(createStage)
    UsdLayerEditorTest._resetScene = staticmethod(resetScene)
    UsdLayerEditorTest._undo = staticmethod(undo)
    UsdLayerEditorTest._redo = staticmethod(redo)
    UsdLayerEditorTest._openStageLayerEditor = staticmethod(openStageLayerEditor)
```

- [ ] **Step 3: Write the Maya equivalent**

Path: `/d/repos/agent_repos/ecg-maya-usd/maya-usd/lib/usdUfe/usd-layer-editor/test/mayaLayerEditorTestSetup.py`

Contents:

```python
#
# Copyright 2026 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Bind the DCC-agnostic UsdLayerEditorTest static hooks to Maya implementations.

Mirrors max_layer_editor_test_setup.py from the 3dsmax USD plugin so the same
shared `layer_editor_test.py` runs in both DCCs.
"""

from maya import cmds, mel
import mayaUsd
import mayaUsd_createStageWithNewLayer
from pxr import Usd, UsdUtils

from layer_editor_test import UsdLayerEditorTest


def _stageFromShape(shapePath):
    return mayaUsd.lib.GetPrim(shapePath).GetStage()


def createStage(rootFile):
    """Create a Maya USD stage backed by rootFile and return the Usd.Stage."""
    cmds.file(new=True, force=True)
    shapePath = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
    stage = _stageFromShape(shapePath)
    if rootFile:
        # Replace the anonymous root with the provided file.
        rootLayer = stage.GetRootLayer()
        rootLayer.subLayerPaths.clear()
        rootLayer.subLayerPaths.append(rootFile)
        stage.Reload()
    return stage


def resetScene():
    cmds.file(new=True, force=True)


def undo():
    cmds.undo()


def redo():
    cmds.redo()


def openStageLayerEditor(rootFile):
    """Create a stage from rootFile and open the Maya USD Layer Editor panel."""
    stage = createStage(rootFile)
    # MEL command that opens the Maya USD Layer Editor for the active stage.
    mel.eval('mayaUsdLayerEditorWindow mayaUsdLayerEditor')
    return stage


def setup():
    """Install the Maya implementations into UsdLayerEditorTest."""
    UsdLayerEditorTest._createStage = staticmethod(createStage)
    UsdLayerEditorTest._resetScene = staticmethod(resetScene)
    UsdLayerEditorTest._undo = staticmethod(undo)
    UsdLayerEditorTest._redo = staticmethod(redo)
    UsdLayerEditorTest._openStageLayerEditor = staticmethod(openStageLayerEditor)
```

- [ ] **Step 4: Commit**

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd add lib/usdUfe/usd-layer-editor/test/mayaLayerEditorTestSetup.py
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd commit -m "Add Maya DCC hook setup for shared layer editor tests

Mirrors the 3dsmax setup script: binds Maya-specific create-stage, undo,
redo, reset, and open-editor implementations to UsdLayerEditorTest.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

### Task 8: Create the ctest wrapper that runs `UsdLayerEditorTest` in Maya

**Files:**
- Create: `lib/usdUfe/usd-layer-editor/test/testMayaUsdSharedLayerEditor.py`

- [ ] **Step 1: Confirm `UsdLayerEditorTest` is the test class to import**

Read `/d/repos/agent_repos/ecg-maya-usd/maya-usd/lib/usdUfe/usd-layer-editor/test/layer_editor_test.py` and verify it defines a class `UsdLayerEditorTest(unittest.TestCase)` with `test_*` methods. The wrapper relies on this class name. If the class is named differently, adjust the `from layer_editor_test import ...` line in Step 2 accordingly.

- [ ] **Step 2: Write the wrapper test file**

Path: `/d/repos/agent_repos/ecg-maya-usd/maya-usd/lib/usdUfe/usd-layer-editor/test/testMayaUsdSharedLayerEditor.py`

Contents:

```python
#!/usr/bin/env python
#
# Copyright 2026 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# (Apache 2.0 boilerplate — match style of other tests in test/lib/)
#
"""ctest entry point: runs the shared layer_editor_test.py suite inside Maya.

This module is registered in test/lib/CMakeLists.txt and discovered by ctest.
It performs the DCC hook setup, then re-exports the test classes from the
shared component so unittest finds them.
"""

import os
import sys
import unittest

# Make sure the shared component's test directory is on sys.path so the
# `layer_editor_test` and `mayaLayerEditorTestSetup` modules import correctly.
_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)

import mayaLayerEditorTestSetup
mayaLayerEditorTestSetup.setup()

from layer_editor_test import UsdLayerEditorTest  # noqa: E402

# Re-export so unittest discovery finds the test class under this module name.
__all__ = ['UsdLayerEditorTest']

if __name__ == '__main__':
    unittest.main(verbosity=2)
```

- [ ] **Step 3: Commit (test still not registered with ctest — Task 9)**

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd add lib/usdUfe/usd-layer-editor/test/testMayaUsdSharedLayerEditor.py
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd commit -m "Add Maya ctest wrapper for shared layer editor tests

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

### Task 9: Register the wrapper test with ctest

**Files:**
- Modify: `test/lib/CMakeLists.txt`

- [ ] **Step 1: Add the test to `TEST_SCRIPT_FILES`**

Open `/d/repos/agent_repos/ecg-maya-usd/maya-usd/test/lib/CMakeLists.txt`. Find:

```cmake
set(TEST_SCRIPT_FILES
    testMayaUsdConverter.py
    testMayaUsdCreateStageCommands.py
    testMayaUsdCreateStageInMayaRef.py
    testMayaUsdDirtyScene.py
    testMayaUsdLayerEditorCommands.py
    testMayaUsdProxyAccessor.py
    testMayaUsdCacheId.py
    testMayaUsdInfoCommand.py
    testMayaUsdSchemaCommand.py
)
```

The wrapper test lives at a non-default path (`lib/usdUfe/usd-layer-editor/test/`). The existing list assumes tests live next to `CMakeLists.txt` itself. We need to either copy the script in or extend the registration.

Add (preserving the existing list) at the end:

```cmake
# Shared layer editor component tests — only registered when the shared
# component is built. The Python module lives outside this directory.
if(BUILD_NEW_LAYER_EDITOR)
    set(SHARED_LAYER_EDITOR_TEST_DIR
        ${CMAKE_SOURCE_DIR}/lib/usdUfe/usd-layer-editor/test
    )
    mayaUsd_get_unittest_target(target testMayaUsdSharedLayerEditor.py)
    mayaUsd_add_test(${target}
        WORKING_DIRECTORY ${SHARED_LAYER_EDITOR_TEST_DIR}
        PYTHON_MODULE testMayaUsdSharedLayerEditor
        ENV
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "PYTHONPATH=${SHARED_LAYER_EDITOR_TEST_DIR}"
    )
    set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsd)
endif()
```

Note: if `mayaUsd_get_unittest_target` and `mayaUsd_add_test` use the working directory as the python search path, this should work out of the box. If not, the `PYTHONPATH=...` env entry above provides a fallback. Confirm during the next step.

- [ ] **Step 2: Configure + build with `BUILD_NEW_LAYER_EDITOR=ON`**

Run configure, then build, via the host relay (same pattern as Task 4 Step 2). The test should appear in `ctest -N` output.

- [ ] **Step 3: Run the test**

Ask the user to run:

```
ctest -R testMayaUsdSharedLayerEditor --output-on-failure
```

(The host relay doesn't have a `test` command listed in `commands.json`; either ask the user or add one via the user's host-side relay configuration.)

Expected: the test runs in a Maya session, the DCC setup is applied without exception, and at least one shared test method executes. PASS or FAIL is informative — what matters is the test is *discoverable and runs to completion*.

If it fails inside `mayaLayerEditorTestSetup.setup()` or in the wrapper, that's a setup bug — fix it in `mayaLayerEditorTestSetup.py` or `testMayaUsdSharedLayerEditor.py`.

If individual tests inside `layer_editor_test.py` fail because the shared component hasn't received a feature yet — that's *expected*. Those failures become entries in `MIGRATION.md`.

- [ ] **Step 4: Commit**

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd add test/lib/CMakeLists.txt
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd commit -m "Register testMayaUsdSharedLayerEditor with ctest

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Phase 4 Setup: Migration tracking + first example port

### Task 10: Generate `MIGRATION.md` with all 161 commits

**Files:**
- Create: `lib/usdUfe/usd-layer-editor/MIGRATION.md`

- [ ] **Step 1: Pull commit metadata**

Run:

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd log \
  --pretty=format:'| %h | %s | bug-fix | pending | |' \
  --since="2024-09-17" \
  -- lib/usd/ui/layerEditor/ \
  > /tmp/migration_rows.txt
wc -l /tmp/migration_rows.txt
```

Expected count: ~161 rows.

- [ ] **Step 2: Write `MIGRATION.md` with the rows and a preamble**

Path: `/d/repos/agent_repos/ecg-maya-usd/maya-usd/lib/usdUfe/usd-layer-editor/MIGRATION.md`

Template (paste the row content from `/tmp/migration_rows.txt` into the table body):

```markdown
# USD Layer Editor Migration Tracking

Resume point for porting maya-usd layer editor commits into the shared component. See `../docs/superpowers/specs/2026-05-19-usd-layer-editor-migration-design.md` for the full plan.

## How to use this file

1. Find the first row with status `pending` (top-down).
2. Read the commit (`git show <hash>`) and identify the file(s) it touches.
3. Apply the porting rule (see below) and update the row.
4. Build + run the test gate (Section 4 of the design spec).
5. Commit your change to the row alongside the code change.

## Porting rules per commit

- Changed file has a counterpart in `usd-layer-editor/lib/` → diff and port logic, stripping any Maya dependencies. Mark `ported`.
- Changed file is Maya-specific (`maya*.cpp/h`) → no action. Mark `maya-only`.
- Changed file doesn't exist in shared component but is DCC-agnostic → bring the whole file in, add to `CMakeLists.txt`. Mark `ported`.
- Changed file has Maya dependencies that need removing → add virtual hook to `SessionState` / `AbstractCommandHook`, inject from Maya side. Mark `needs-hook` until the hook lands, then `ported`.
- Pure formatting / clang / whitespace → mark `skip`.

## Group labels (set during initial pass)

- `bug-fix` — small bug fixes, default for un-classified rows
- `layer-contents` — EMSUSD-3189, layerContentsWidget, usdSyntaxHighlighter
- `component-creator` — EMSUSD-2997/3016/3020, componentSaveWidget
- `ef-banner` — Edit Forwarding banner + echo
- `filesystem` — EMSUSD-3654 gulrak filesystem update
- `maya-only` — touches only `maya*` files

## Statuses

`pending` / `ported` / `maya-only` / `skip` / `needs-hook` / `needs-test`

## Commits

| Commit | Description | Group | Status | Notes |
|--------|-------------|-------|--------|-------|
<paste rows from /tmp/migration_rows.txt here>
```

- [ ] **Step 3: First-pass classification (group label)**

Run a quick scan over the table and update obvious group labels. Heuristics:
- Commit subject contains `clang` or `lint` → `skip`
- Subject contains `component`, `componentSaveWidget`, `CC` → `component-creator`
- Subject contains `layer contents`, `Display Layer Content`, `EMSUSD-3189` → `layer-contents`
- Subject contains `EF`, `Edit Forward`, `edit forwarding` → `ef-banner`
- Subject contains `gulrak`, `filesystem` → `filesystem`

This pass is approximate — refine as commits are actually ported.

- [ ] **Step 4: Commit**

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd add lib/usdUfe/usd-layer-editor/MIGRATION.md
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd commit -m "Add MIGRATION.md to track usd-layer-editor commit porting

Lists all 161 commits to lib/usd/ui/layerEditor/ since 2024-09-17 with
their initial group labels. Status defaults to 'pending'; updated as
each commit is reviewed and ported.

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

### Task 11: Port one example commit (establish the recipe)

Pick a small `bug-fix` commit that touches a file with a clear counterpart in the shared component — e.g. `78c345062 Fix comment wording`, `c710f9260 Fix crash on add layer when root is locked.`, or similar. The exact one doesn't matter; the goal is to walk the recipe end-to-end and prove the porting workflow.

**Files:** Determined by the chosen commit. Likely:
- Read: `lib/usd/ui/layerEditor/<file>.cpp/h`
- Modify: `lib/usdUfe/usd-layer-editor/lib/<file>.cpp/h`
- Modify: `lib/usdUfe/usd-layer-editor/MIGRATION.md` (update row)

- [ ] **Step 1: Pick the example commit**

Skim the first ~20 rows of `MIGRATION.md` for a small commit that:
- Touches a single file
- Has a counterpart in `lib/usdUfe/usd-layer-editor/lib/`
- Is a clear bug fix or small improvement (not formatting)

Record its hash: `<example_hash>`.

- [ ] **Step 2: Inspect the commit**

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd show <example_hash>
```

Note the file(s) changed and the actual diff.

- [ ] **Step 3: Locate the counterpart**

For each file changed in the original commit, find the corresponding file in `lib/usdUfe/usd-layer-editor/lib/`. For example, if the commit modifies `lib/usd/ui/layerEditor/layerTreeView.cpp`, the counterpart is `lib/usdUfe/usd-layer-editor/lib/layerTreeView.cpp`.

- [ ] **Step 4: Diff the two versions**

```bash
diff /d/repos/agent_repos/ecg-maya-usd/maya-usd/lib/usd/ui/layerEditor/layerTreeView.cpp \
     /d/repos/agent_repos/ecg-maya-usd/maya-usd/lib/usdUfe/usd-layer-editor/lib/layerTreeView.cpp
```

Look for: does the bug fix from `<example_hash>` already exist in the shared component? If yes → status `skip`, done. If no → continue.

- [ ] **Step 5: Apply the same fix to the counterpart**

Edit the shared component file to apply the equivalent change. **Strip any Maya-specific symbols** (in this kind of bug fix, there usually aren't any — but check). If you find Maya symbols you can't remove cleanly, mark status `needs-hook` and write a separate plan to add the missing virtual.

- [ ] **Step 6: Build + run tests**

Build with `BUILD_NEW_LAYER_EDITOR=ON`. Then:
- Run the MEL tests: `ctest -R testMayaUsdLayerEditorCommands --output-on-failure`
- Run the new wrapper test: `ctest -R testMayaUsdSharedLayerEditor --output-on-failure`

Expected: no new failures introduced by the port.

- [ ] **Step 7: Update `MIGRATION.md`**

Change the example commit's row from `pending` to `ported`. Add a note describing what was done. Example:

```
| 78c34506 | Fix comment wording | skip | skip | Comment-only, no logic change |
```

or

```
| c710f926 | Fix crash on add layer when root is locked | bug-fix | ported | Same fix applied to shared customLayerData.cpp; covered by testCrashAddLayerWhenRootLocked |
```

- [ ] **Step 8: Commit the port + the tracking update together**

```bash
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd add -u lib/
git -C /d/repos/agent_repos/ecg-maya-usd/maya-usd commit -m "Port <example_hash> to shared usd-layer-editor

<short description of what was ported>

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## What comes next (out of this plan's scope)

- **Phase 4 batches**: 160 remaining commits to port. Each batch (5–20 commits grouped by area) becomes its own implementation plan, generated by reading `MIGRATION.md` and writing tasks per pending row.
- **Phase 5 (flip default)**: Once `MIGRATION.md` has zero `pending` rows AND all tests pass, change `BUILD_NEW_LAYER_EDITOR` default to `ON`, then delete shared source files from `lib/usd/ui/layerEditor/` (keep only the `maya*` files).

The branch `deboisj/unify_le` in the `maya-usd` submodule holds all of this work.
