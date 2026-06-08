# usdLayerEditor Relocation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Update all CMake wiring after the physical relocation of the shared layer-editor library from `lib/usdUfe/usd-layer-editor/` to `lib/usdLayerEditor/` and its tests from `lib/usdUfe/usd-layer-editor/test/` to `test/lib/usdLayerEditor/`.

**Architecture:** The physical file moves are already done by the developer before this plan is executed. This plan contains only CMake edits. The library target (`UsdLayerEditorLib`) continues to build unconditionally (same as before the move). The C++ GTest executable (`UsdLayerEditorNewTests`) continues to build whenever `BUILD_TESTS=ON`. The interactive Python test (`testMayaUsdSharedLayerEditor`) remains gated on `BUILD_NEW_LAYER_EDITOR` as before.

**Tech Stack:** CMake 3.13+.

**Prerequisite:** The developer has already:
1. Moved and renamed `lib/usdUfe/usd-layer-editor/` → `lib/usdLayerEditor/` (the `test/` subdirectory was removed from under `lib/usdLayerEditor/` as part of this move).
2. Moved `lib/usdUfe/usd-layer-editor/test/` → `test/lib/usdLayerEditor/`.

No source `.cpp`/`.h`/`.py` files need editing — only CMakeLists.txt files.

> **Note on `BUILD_NEW_LAYER_EDITOR`:** This option is referenced in `test/lib/CMakeLists.txt` (guards the Python test) but is not yet declared as a CMake `option()` in the root `CMakeLists.txt`. That means the Python test never runs until someone sets `-DBUILD_NEW_LAYER_EDITOR=ON` explicitly. Declaring the option properly is tracked separately; this plan does not change that gate.

---

## File Structure

| File | Action | What changes |
|------|--------|--------------|
| `lib/usdUfe/CMakeLists.txt` | Modify | Remove `add_subdirectory(usd-layer-editor)` |
| `lib/CMakeLists.txt` | Modify | Add `add_subdirectory(usdLayerEditor)` |
| `lib/usdLayerEditor/CMakeLists.txt` | Modify | Remove `if(BUILD_TESTS) add_subdirectory(test) endif()` block; update stale comment |
| `lib/usdLayerEditor/lib/CMakeLists.txt` | Modify | Update stale path comment |
| `lib/usdLayerEditor/python/CMakeLists.txt` | Modify | Update stale path comment |
| `test/lib/usdLayerEditor/CMakeLists.txt` | Modify | Add Python interactive test registration (was only `add_subdirectory(cpp)`) |
| `test/lib/usdLayerEditor/cpp/CMakeLists.txt` | Modify | Fix `NEW_LE_DIR` — path to library sources changed |
| `test/lib/CMakeLists.txt` | Modify | Remove old standalone Python-test block; add `add_subdirectory(usdLayerEditor)` |

---

## Task 1: De-register library from usdUfe; re-register at lib level

`usdLayerEditor` is now a sibling of `usdUfe`, not a child. Two files must change atomically so the CMake configure step doesn't error trying to add a non-existent subdirectory.

**Files:**
- Modify: `lib/usdUfe/CMakeLists.txt` (line 185)
- Modify: `lib/CMakeLists.txt`

- [ ] **Step 1: Remove the old subdirectory entry from usdUfe**

In `lib/usdUfe/CMakeLists.txt` delete the line:

```cmake
add_subdirectory(usd-layer-editor)
```

The block before the change looks like:

```cmake
add_subdirectory(base)
add_subdirectory(python)
add_subdirectory(resources)
add_subdirectory(ufe)
add_subdirectory(undo)
add_subdirectory(utils)

add_subdirectory(usd-layer-editor)
```

After the change:

```cmake
add_subdirectory(base)
add_subdirectory(python)
add_subdirectory(resources)
add_subdirectory(ufe)
add_subdirectory(undo)
add_subdirectory(utils)
```

- [ ] **Step 2: Add the new subdirectory entry at lib level**

In `lib/CMakeLists.txt`, add `add_subdirectory(usdLayerEditor)` immediately after `add_subdirectory(usdUfe)`.

Current content:

```cmake
add_subdirectory(usdUfe)
if (BUILD_MAYAUSD_LIBRARY)
    add_subdirectory(mayaUsd)
    add_subdirectory(usd)
endif()
if (BUILD_MAYAUSDAPI_LIBRARY)
    add_subdirectory(mayaUsdAPI)
endif()
if (BUILD_LOOKDEVXUSD_LIBRARY)
    add_subdirectory(lookdevXUsd)
endif()
```

After the change:

```cmake
add_subdirectory(usdUfe)
add_subdirectory(usdLayerEditor)
if (BUILD_MAYAUSD_LIBRARY)
    add_subdirectory(mayaUsd)
    add_subdirectory(usd)
endif()
if (BUILD_MAYAUSDAPI_LIBRARY)
    add_subdirectory(mayaUsdAPI)
endif()
if (BUILD_LOOKDEVXUSD_LIBRARY)
    add_subdirectory(lookdevXUsd)
endif()
```

- [ ] **Step 3: Verify build passes**

Run the build via the relay:

```bash
result=$(python3 _host_command/relay_client.py run build \
  --db _host_command/relay.db \
  --commands-json _host_command/commands.json)
_exit=$(python3 -c "import json,sys; print(json.loads(sys.argv[1])['exit_code'])" "$result")
echo "$result"
```

Expected: `"exit_code": 0`. At this point the library compiles from its new location. The build will error if `lib/usdLayerEditor/CMakeLists.txt` still tries to `add_subdirectory(test)` (since that directory no longer lives under `lib/usdLayerEditor/`); that is fixed in Task 2.

- [ ] **Step 4: Commit**

```bash
git -C maya-usd add lib/usdUfe/CMakeLists.txt lib/CMakeLists.txt
git -C maya-usd commit -m "cmake: move usdLayerEditor to lib/ sibling of usdUfe"
```

---

## Task 2: Remove the `test` subdirectory reference from the library CMakeLists

`lib/usdLayerEditor/CMakeLists.txt` still contains `add_subdirectory(test)` but tests have moved to the test tree. Also update the file's top comment so it no longer claims to live under usdUfe.

**Files:**
- Modify: `lib/usdLayerEditor/CMakeLists.txt`

Current content:

```cmake
# Top-level CMakeLists for the shared usd-layer-editor component
# (DCC-agnostic). Built when BUILD_NEW_LAYER_EDITOR is ON.

add_subdirectory(lib)
add_subdirectory(python)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()
```

- [ ] **Step 1: Remove the test subdirectory block and update the comment**

New content:

```cmake
# Top-level CMakeLists for the shared usdLayerEditor component (DCC-agnostic).

add_subdirectory(lib)
add_subdirectory(python)
```

- [ ] **Step 2: Verify build passes**

```bash
result=$(python3 _host_command/relay_client.py run build \
  --db _host_command/relay.db \
  --commands-json _host_command/commands.json)
_exit=$(python3 -c "import json,sys; print(json.loads(sys.argv[1])['exit_code'])" "$result")
echo "$result"
```

Expected: `"exit_code": 0`. `UsdLayerEditorLib` and `_UsdLayerEditor` (Python binding) build cleanly.

- [ ] **Step 3: Commit**

```bash
git -C maya-usd add lib/usdLayerEditor/CMakeLists.txt
git -C maya-usd commit -m "cmake: remove test subdir from usdLayerEditor lib cmake (tests moved to test tree)"
```

---

## Task 3: Fix the library-path reference inside the C++ test cmake

`test/lib/usdLayerEditor/cpp/CMakeLists.txt` contains a relative path that pointed back to the library's source headers. That relative path is now wrong because the cpp directory is in a completely different subtree.

**Files:**
- Modify: `test/lib/usdLayerEditor/cpp/CMakeLists.txt`

The broken line:

```cmake
set(NEW_LE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)
```

When this file lived at `lib/usdUfe/usd-layer-editor/test/cpp/`, `../../lib` resolved correctly to `lib/usdUfe/usd-layer-editor/lib`. Now the file is at `test/lib/usdLayerEditor/cpp/`, so `../../lib` resolves to `test/lib/` — wrong.

- [ ] **Step 1: Replace the relative path with an absolute CMake path**

Old line:

```cmake
set(NEW_LE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)
```

New line:

```cmake
set(NEW_LE_DIR ${CMAKE_SOURCE_DIR}/lib/usdLayerEditor/lib)
```

(`CMAKE_SOURCE_DIR` is the root `maya-usd/` directory, set by CMake before any subdirectory is processed.)

- [ ] **Step 2: Verify the target_include_directories block still references the variable correctly**

Confirm the file still contains (unchanged):

```cmake
target_include_directories(UsdLayerEditorNewTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${NEW_LE_DIR}
        ${UFE_INCLUDE_DIR}
)
```

No change needed here — `NEW_LE_DIR` is just used here, the fix is in the `set()` above.

- [ ] **Step 3: Commit**

```bash
git -C maya-usd add test/lib/usdLayerEditor/cpp/CMakeLists.txt
git -C maya-usd commit -m "cmake: fix usdLayerEditor C++ test NEW_LE_DIR path after relocation"
```

---

## Task 4: Wire tests into the test tree and move Python test registration

Currently `test/lib/usdLayerEditor/CMakeLists.txt` (which was `usd-layer-editor/test/CMakeLists.txt`) only contains `add_subdirectory(cpp)`. The Python interactive test registration previously lived in `test/lib/CMakeLists.txt` as a standalone block using a hardcoded path. Both things need updating:

1. Move the Python test registration into `test/lib/usdLayerEditor/CMakeLists.txt` where it belongs (co-located with the test files it references).
2. Update `test/lib/CMakeLists.txt` to drive the whole subdirectory rather than containing an inline block.

**Files:**
- Modify: `test/lib/usdLayerEditor/CMakeLists.txt`
- Modify: `test/lib/CMakeLists.txt`

- [ ] **Step 1: Expand `test/lib/usdLayerEditor/CMakeLists.txt`**

Current content of `test/lib/usdLayerEditor/CMakeLists.txt`:

```cmake
#
# Copyright 2026 Autodesk
# ...license header...
#

# C++ GTest suite for the shared layer editor widget (and the old in-tree editor).
add_subdirectory(cpp)
```

New content — keep the cpp subdir entry, add the Python interactive test below it:

```cmake
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

# C++ GTest suite for the shared usdLayerEditor component.
add_subdirectory(cpp)

# Maya-driven interactive Python test that runs the DCC-agnostic
# layer_editor_test.py suite. Gated on BUILD_NEW_LAYER_EDITOR because it
# requires the UsdLayerEditorLib Python bindings to be installed.
if(BUILD_NEW_LAYER_EDITOR)
    mayaUsd_get_unittest_target(target testMayaUsdSharedLayerEditor.py)
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PYTHON_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/testMayaUsdSharedLayerEditor.py
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}"
    )
    set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsd)
endif()
```

Key differences from the old inline block in `test/lib/CMakeLists.txt`:
- `WORKING_DIRECTORY` and `PYTHON_SCRIPT` both use `${CMAKE_CURRENT_SOURCE_DIR}` (which correctly resolves to `test/lib/usdLayerEditor/`) rather than a hardcoded `SHARED_LAYER_EDITOR_TEST_DIR` variable.
- `PYTHONPATH` likewise uses `${CMAKE_CURRENT_SOURCE_DIR}` — no separate variable needed.

- [ ] **Step 2: Replace the inline block in `test/lib/CMakeLists.txt`**

Locate and remove the existing block that looks like this:

```cmake
#
# Shared usd-layer-editor component tests
#
# ...comment block...
# -------------------------------------------------------------------------------------
if(BUILD_NEW_LAYER_EDITOR)
    set(SHARED_LAYER_EDITOR_TEST_DIR
        ${CMAKE_SOURCE_DIR}/lib/usdUfe/usd-layer-editor/test
    )
    mayaUsd_get_unittest_target(target testMayaUsdSharedLayerEditor.py)
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${SHARED_LAYER_EDITOR_TEST_DIR}
        PYTHON_SCRIPT ${SHARED_LAYER_EDITOR_TEST_DIR}/testMayaUsdSharedLayerEditor.py
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "PYTHONPATH=${SHARED_LAYER_EDITOR_TEST_DIR}"
    )
    set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsd)
endif()
```

Replace the entire block with a single `add_subdirectory` call (unconditional — `BUILD_TESTS` is already asserted by the parent chain):

```cmake
#
# Shared usdLayerEditor component tests (C++ GTests + Maya Python interactive test).
# -----------------------------------------------------------------------------
add_subdirectory(usdLayerEditor)
```

- [ ] **Step 3: Verify build and test discovery**

```bash
result=$(python3 _host_command/relay_client.py run build \
  --db _host_command/relay.db \
  --commands-json _host_command/commands.json)
_exit=$(python3 -c "import json,sys; print(json.loads(sys.argv[1])['exit_code'])" "$result")
echo "$result"
```

Expected: `"exit_code": 0`.

After a successful configure+build, confirm `UsdLayerEditorNewTests` is still visible to ctest:

```bash
ctest --test-dir <build-dir> -N -R UsdLayerEditorNewTests
```

Expected output contains `UsdLayerEditorNewTests`.

- [ ] **Step 4: Commit**

```bash
git -C maya-usd add \
    test/lib/usdLayerEditor/CMakeLists.txt \
    test/lib/CMakeLists.txt
git -C maya-usd commit -m "cmake: wire usdLayerEditor tests into test tree"
```

---

## Task 5: Update stale path comments in lib cmake files

Three files carry comments that still reference the old `lib/usdUfe/usd-layer-editor/` path. These are non-functional but misleading.

**Files:**
- Modify: `lib/usdLayerEditor/lib/CMakeLists.txt` (top comment block)
- Modify: `lib/usdLayerEditor/python/CMakeLists.txt` (top comment block)

- [ ] **Step 1: Fix comment in `lib/usdLayerEditor/lib/CMakeLists.txt`**

Find the comment near the top:

```cmake
# This CMakeLists is invoked from lib/usdUfe/CMakeLists.txt when
# BUILD_NEW_LAYER_EDITOR is ON. Qt, UFE, USD and usdUfe are already
# found/defined by the parent build — do not call find_package here.
```

Replace with:

```cmake
# This CMakeLists is invoked from lib/CMakeLists.txt.
# Qt, UFE, USD and usdUfe are already found/defined by the parent build —
# do not call find_package here.
```

- [ ] **Step 2: Fix comment in `lib/usdLayerEditor/python/CMakeLists.txt`**

Find the comment near the top:

```cmake
# This CMakeLists is invoked from lib/usdUfe/usd-layer-editor/CMakeLists.txt
# when BUILD_NEW_LAYER_EDITOR is ON. Python, boost.python, UFE and USD are
# already found/defined by the parent maya-usd build — do not call
# find_package here.
```

Replace with:

```cmake
# This CMakeLists is invoked from lib/usdLayerEditor/CMakeLists.txt.
# Python, boost.python, UFE and USD are already found/defined by the parent
# maya-usd build — do not call find_package here.
```

- [ ] **Step 3: Commit**

```bash
git -C maya-usd add \
    lib/usdLayerEditor/lib/CMakeLists.txt \
    lib/usdLayerEditor/python/CMakeLists.txt
git -C maya-usd commit -m "cmake: update stale path comments after usdLayerEditor relocation"
```

---

## Self-Review

**Spec coverage:**
- ✅ Library moves from `lib/usdUfe/usd-layer-editor/` to `lib/usdLayerEditor/` — Tasks 1 & 2
- ✅ Tests move from `lib/.../test/` to `test/lib/usdLayerEditor/` — Tasks 3 & 4
- ✅ Rename from `usd-layer-editor` to `usdLayerEditor` reflected in all CMake paths — all tasks
- ✅ Physical move done by developer (precondition, not a task)
- ✅ No behavior change to build/test triggering

**Placeholder scan:** None found — all steps show exact file content.

**Type/name consistency:** `UsdLayerEditorLib`, `UsdLayerEditorNewTests`, `_UsdLayerEditor` — consistent across all tasks, unchanged from pre-move names.
