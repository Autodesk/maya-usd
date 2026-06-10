# Layer-Editor Discrepancy Test Coverage Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add C++ regression coverage in `UsdLayerEditorNewTests` for the layer-editor discrepancy fixes D8, D9, D10, plus contract tests for the new DCC-function accessors (`mainWindowParent`, `layerContentsArraySizeLimit`, `layerContentsTimeSamplesSizeLimit`).

**Architecture:** All tests live in the existing `UsdLayerEditorNewTests` GoogleTest target (`test/lib/usdLayerEditor/cpp/`), which links the shared `UsdLayerEditorLib` under a real `QApplication`. Reuse `LayerEditorTestFixture`, `StubSessionState`, `StubCommandHook`, `ScopedLayerEditorDCCFunctions`, and `TestUtils::dismissNextModal`. Registry-contract tests go in a new `testDCCFunctions.cpp`; behavioral tests extend existing `*Logic.h` files.

**Tech Stack:** C++, GoogleTest, Qt Widgets, OpenUSD (Sdf/Usd), the `UsdLayerEditor` DCC-functions registry.

**Note on git:** Per repo `CLAUDE.md`, commits require explicit user authorization and concise messages with no co-author. Commit steps are included for completeness — confirm with the user before running them.

**Note on test execution:** Tests run on the host via the relay (`_host_command/relay_client.py`). The `test` command filters ctest by name regex; it cannot run a single GoogleTest case, so each run executes the whole `UsdLayerEditorNewTests` suite. Because the fixes are already implemented, these tests should pass when first run (they are regression guards). To prove a test actually guards its fix, optionally revert the one-line fix, confirm the test goes RED, then restore.

---

### Task 1: Registry-contract tests for the new DCC-function accessors

**Files:**
- Create: `test/lib/usdLayerEditor/cpp/testDCCFunctions.cpp`
- Modify: `test/lib/usdLayerEditor/cpp/CMakeLists.txt` (add the new source to `LAYER_EDITOR_TEST_SOURCES`)

- [ ] **Step 1: Create the test file**

Create `test/lib/usdLayerEditor/cpp/testDCCFunctions.cpp`:

```cpp
//
// Copyright 2026 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "scopedLayerEditorDCCFunctions.h"

#include <layerEditorDCCFunctions.h>

#include <gtest/gtest.h>

#include <QtWidgets/QWidget>

#include <cstdint>

using namespace UsdLayerEditor;

// D6: mainWindowParent() returns null when unset, the registered widget when set.
TEST(LayerEditorDCCFunctions, MainWindowParent_DefaultsToNull)
{
    ScopedLayerEditorDCCFunctions guard;
    setEnvironmentFns(EnvironmentFns {});
    EXPECT_EQ(mainWindowParent(), nullptr);
}

TEST(LayerEditorDCCFunctions, MainWindowParent_ReturnsRegisteredWidget)
{
    ScopedLayerEditorDCCFunctions guard;
    QWidget        w;
    EnvironmentFns env;
    env.mainWindowParent = [&w]() { return &w; };
    setEnvironmentFns(env);
    EXPECT_EQ(mainWindowParent(), &w);
}

// D8: layer-contents size limits default to 8, return the registered values when set.
TEST(LayerEditorDCCFunctions, LayerContentsLimits_DefaultToEight)
{
    ScopedLayerEditorDCCFunctions guard;
    setEnvironmentFns(EnvironmentFns {});
    EXPECT_EQ(layerContentsArraySizeLimit(), 8);
    EXPECT_EQ(layerContentsTimeSamplesSizeLimit(), 8);
}

TEST(LayerEditorDCCFunctions, LayerContentsLimits_ReturnRegisteredValues)
{
    ScopedLayerEditorDCCFunctions guard;
    EnvironmentFns                env;
    env.layerContentsArraySizeLimit = []() -> int64_t { return 3; };
    env.layerContentsTimeSamplesSizeLimit = []() -> int64_t { return 5; };
    setEnvironmentFns(env);
    EXPECT_EQ(layerContentsArraySizeLimit(), 3);
    EXPECT_EQ(layerContentsTimeSamplesSizeLimit(), 5);
}
```

- [ ] **Step 2: Register the source in CMake**

In `test/lib/usdLayerEditor/cpp/CMakeLists.txt`, add `testDCCFunctions.cpp` to the
`LAYER_EDITOR_TEST_SOURCES` list (after `testEFMode.cpp`):

```cmake
set(LAYER_EDITOR_TEST_SOURCES
    testMain.cpp
    stubCommandHook.cpp
    stubSessionState.cpp
    testFixture.cpp
    testButtons.cpp
    testContextMenu.cpp
    testReorder.cpp
    testMenusAndStage.cpp
    testLayerTreeItem.cpp
    testLayerTreeModel.cpp
    testLayerTreeView.cpp
    testLayerContentsWidget.cpp
    testSaveLayersDialog.cpp
    testLoadLayersDialog.cpp
    testLayerLocking.cpp
    testLayerMuting.cpp
    testSharedStage.cpp
    testLayerEditorCommands.cpp
    testEFMode.cpp
    testDCCFunctions.cpp
)
```

- [ ] **Step 3: Configure + build**

Run (via relay):
```
configure
build
```
Expected: `Success MayaUsd build and install!` and `UsdLayerEditorNewTests` links.

- [ ] **Step 4: Run the suite**

Run (via relay): `test UsdLayerEditorNewTests`
Expected: PASS, including the four `LayerEditorDCCFunctions.*` tests.

- [ ] **Step 5: Commit** (after user authorization)

```bash
git add test/lib/usdLayerEditor/cpp/testDCCFunctions.cpp test/lib/usdLayerEditor/cpp/CMakeLists.txt
git commit -m "test: cover new layer-editor DCC-function accessors"
```

---

### Task 2: D9 — auto-hide action is first in the Option menu

**Files:**
- Modify: `test/lib/usdLayerEditor/cpp/testMenusAndStageLogic.h`

- [ ] **Step 1: Add includes**

At the top of `testMenusAndStageLogic.h`, alongside the existing Qt includes, add:

```cpp
#include "stringResources.h"

#include <QtWidgets/QMenu>
```

- [ ] **Step 2: Add a menu-by-title helper**

Inside `namespace UsdLayerEditor {`, near `findActionInMenuBar`, add:

```cpp
static QMenu* findMenuByTitle(QMainWindow* win, const QString& title)
{
    if (!win || !win->menuBar())
        return nullptr;
    for (QAction* top : win->menuBar()->actions()) {
        if (QMenu* menu = top->menu()) {
            if (menu->title() == title)
                return menu;
        }
    }
    return nullptr;
}
```

- [ ] **Step 3: Add the test**

Add to `testMenusAndStageLogic.h` (inside the namespace):

```cpp
// D9: the Auto-Hide Session Layer action must be the first Option-menu entry,
// checkable, and followed by a separator then the Display-Layer-Contents action.
TEST_F(LayerEditorTestFixture, OptionMenu_AutoHideAction_IsFirstAndCheckable)
{
    auto* win = qobject_cast<QMainWindow*>(_widget->parent());
    ASSERT_NE(win, nullptr);

    QMenu* optionMenu
        = findMenuByTitle(win, StringResources::getAsQString(StringResources::kOption));
    ASSERT_NE(optionMenu, nullptr) << "Option menu should exist";

    const QList<QAction*> actions = optionMenu->actions();
    ASSERT_GE(actions.size(), 3);

    const QString autoHideText
        = StringResources::getAsQString(StringResources::kAutoHideSessionLayer);
    EXPECT_EQ(actions[0]->text(), autoHideText)
        << "Auto-Hide should be the first action in the Option menu";
    EXPECT_TRUE(actions[0]->isCheckable());
    EXPECT_EQ(actions[0]->isChecked(), _sessionState.autoHideSessionLayer());

    EXPECT_TRUE(actions[1]->isSeparator())
        << "a separator should follow the Auto-Hide action";

    QAction* displayContents = findAction(
        optionMenu, StringResources::getAsQString(StringResources::kDisplayLayerContents));
    ASSERT_NE(displayContents, nullptr);
    EXPECT_GT(actions.indexOf(displayContents), 0)
        << "Display Layer Content should come after Auto-Hide";
}
```

- [ ] **Step 4: Build + run**

Run (via relay): `build` then `test UsdLayerEditorNewTests`
Expected: PASS, including `OptionMenu_AutoHideAction_IsFirstAndCheckable`.

- [ ] **Step 5: (Optional) Prove it guards the fix**

Temporarily move the auto-hide block back to the end of `setupDefaultMenu`
(`lib/usdLayerEditor/lib/layerEditorWidget.cpp`), rebuild, and confirm this test goes RED. Restore
the fix afterward.

- [ ] **Step 6: Commit** (after user authorization)

```bash
git add test/lib/usdLayerEditor/cpp/testMenusAndStageLogic.h
git commit -m "test: assert auto-hide is first in the layer-editor Option menu"
```

---

### Task 3: D10 — component-creator early-out in `saveAnonymousLayer`

**Files:**
- Modify: `test/lib/usdLayerEditor/cpp/testLayerTreeItemLogic.h`

- [ ] **Step 1: Add includes**

At the top of `testLayerTreeItemLogic.h`, add (if not already present):

```cpp
#include "scopedLayerEditorDCCFunctions.h"
#include "testUtils.h"

#include <layerEditorDCCFunctions.h>

#include <QtWidgets/QApplication>
```

- [ ] **Step 2: Add the non-component (generic-path) test**

Add to `testLayerTreeItemLogic.h` (inside `namespace UsdLayerEditor {`):

```cpp
// D10: a non-component stage's anonymous layer goes through the generic save path
// (SessionState::saveLayerUI), which the stub records via _saveLayerCallCount.
TEST_F(LayerEditorTestFixture, SaveAnonymousLayer_NonComponentStage_UsesGenericPath)
{
    ScopedLayerEditorDCCFunctions guard;
    ComponentFns                  comp;
    comp.displayError = [](const std::string&) {};
    comp.isStageAComponent = [](const std::string&) { return false; };
    setComponentFns(comp);

    auto* item
        = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isAnonymous());

    _sessionState._saveLayerCallCount = 0;
    item->saveEditsNoPrompt();
    QApplication::processEvents();

    EXPECT_EQ(_sessionState._saveLayerCallCount, 1)
        << "non-component anonymous layer should use the generic saveLayerUI path";
}
```

- [ ] **Step 3: Run to confirm the generic-path test passes**

Run (via relay): `build` then `test UsdLayerEditorNewTests`
Expected: PASS, including `SaveAnonymousLayer_NonComponentStage_UsesGenericPath`.

- [ ] **Step 4: Add the component-stage (early-out) test**

Add to `testLayerTreeItemLogic.h`:

```cpp
// D10: a component stage's anonymous layer must NOT take the generic save path;
// the early-out delegates to LayerTreeModel::saveStage (which shows a modal
// SaveLayersDialog, dismissed here). _saveLayerCallCount staying 0 proves the
// generic path was skipped — it would be 1 if the early-out were missing.
TEST_F(LayerEditorTestFixture, SaveAnonymousLayer_ComponentStage_SkipsGenericPath)
{
    ScopedLayerEditorDCCFunctions guard;
    ComponentFns                  comp;
    comp.displayError = [](const std::string&) {};
    comp.isStageAComponent = [](const std::string&) { return true; };
    setComponentFns(comp);

    auto* item
        = dynamic_cast<LayerTreeItem*>(treeModel()->itemFromIndex(firstSublayerIndex()));
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isAnonymous());

    // saveStage shows a modal SaveLayersDialog; schedule its dismissal so exec() returns.
    TestUtils::dismissNextModal(50);

    _sessionState._saveLayerCallCount = 0;
    item->saveEditsNoPrompt();
    QApplication::processEvents();

    EXPECT_EQ(_sessionState._saveLayerCallCount, 0)
        << "component stage should delegate to saveStage, skipping the generic "
           "anonymous-save path";
}
```

- [ ] **Step 5: Build + run**

Run (via relay): `build` then `test UsdLayerEditorNewTests`
Expected: PASS, including both `SaveAnonymousLayer_*` tests, with no hang (the modal is dismissed).

- [ ] **Step 6: (Optional) Prove it guards the fix**

Temporarily remove the early-out block at the top of `LayerTreeItem::saveAnonymousLayer`
(`lib/usdLayerEditor/lib/layerTreeItem.cpp`), rebuild, and confirm
`SaveAnonymousLayer_ComponentStage_SkipsGenericPath` goes RED (count becomes 1). Restore afterward.

- [ ] **Step 7: Commit** (after user authorization)

```bash
git add test/lib/usdLayerEditor/cpp/testLayerTreeItemLogic.h
git commit -m "test: cover component-creator early-out in saveAnonymousLayer"
```

---

### Task 4: D8 — layer-contents array size limit is applied

**Files:**
- Modify: `test/lib/usdLayerEditor/cpp/testLayerContentsWidgetLogic.h`

- [ ] **Step 1: Add includes**

At the top of `testLayerContentsWidgetLogic.h`, add:

```cpp
#include "scopedLayerEditorDCCFunctions.h"

#include <layerEditorDCCFunctions.h>

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeNames.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <QtWidgets/QTextEdit>
```

- [ ] **Step 2: Add the test**

Add to `testLayerContentsWidgetLogic.h` (inside `namespace UsdLayerEditor {`):

```cpp
// D8: the array size limit (from the DCC registry) is applied when rendering layer
// contents. A small limit truncates the displayed array, yielding shorter text than
// a large limit for the same layer.
TEST_F(LayerContentsWidgetTest, SetLayer_RespectsArraySizeLimit)
{
    auto* cw = findContentsWidget(_widget);
    ASSERT_NE(cw, nullptr);
    auto* textEdit = cw->findChild<QTextEdit*>(QString(), Qt::FindChildrenRecursively);
    ASSERT_NE(textEdit, nullptr);

    // Build a layer with a large array-valued attribute.
    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto prim  = stage->DefinePrim(PXR_NS::SdfPath("/Test"));
    auto attr  = prim.CreateAttribute(
        PXR_NS::TfToken("arr"), PXR_NS::SdfValueTypeNames->IntArray);
    PXR_NS::VtIntArray values(100);
    for (int i = 0; i < 100; ++i)
        values[i] = i;
    attr.Set(values);
    auto layer = stage->GetRootLayer();

    ScopedLayerEditorDCCFunctions guard;

    EnvironmentFns smallEnv;
    smallEnv.layerContentsArraySizeLimit = []() -> int64_t { return 2; };
    setEnvironmentFns(smallEnv);
    cw->setLayer(layer);
    QApplication::processEvents();
    const int smallLen = textEdit->toPlainText().length();

    EnvironmentFns largeEnv;
    largeEnv.layerContentsArraySizeLimit = []() -> int64_t { return 1000; };
    setEnvironmentFns(largeEnv);
    cw->setLayer(layer);
    QApplication::processEvents();
    const int largeLen = textEdit->toPlainText().length();

    EXPECT_LT(smallLen, largeLen)
        << "a smaller array size limit should truncate the displayed array, "
           "yielding shorter output";
}
```

- [ ] **Step 3: Build + run**

Run (via relay): `build` then `test UsdLayerEditorNewTests`
Expected: PASS, including `SetLayer_RespectsArraySizeLimit`.

If `smallLen` and `largeLen` come out equal, inspect the pseudo-layer output format (the filter may
elide arrays with a fixed marker); in that case assert on a substring/element-count difference
instead of total length. Keep the array element count large (100) to maximize the difference.

- [ ] **Step 4: (Optional) Prove it guards the fix**

Temporarily hardcode `params.arraySizeLimit = 1000;` in `LayerContentsWidget::exportPseudoLayer`
(`lib/usdLayerEditor/lib/layerContentsWidget.cpp`), rebuild, and confirm this test goes RED
(`smallLen == largeLen`). Restore the registry-driven assignment afterward.

- [ ] **Step 5: Commit** (after user authorization)

```bash
git add test/lib/usdLayerEditor/cpp/testLayerContentsWidgetLogic.h
git commit -m "test: assert layer-contents array size limit is applied"
```

---

### Task 5: Full-suite verification

- [ ] **Step 1: Run the full layer-editor test set**

Run (via relay): `test [Ll]ayer.?[Ee]ditor`
Expected: 100% pass across all layer-editor suites (the four existing plus the new tests in
`UsdLayerEditorNewTests`).

---

## Self-Review

**Spec coverage:**
- D8 → Task 1 (contract) + Task 4 (behavioral). ✓
- D9 → Task 2. ✓
- D10 → Task 3 (both component + non-component cases). ✓
- D6 `mainWindowParent` contract → Task 1. ✓
- D5 → intentionally absent (out of scope). ✓
- D1/D2/D3 → no tests (resolved/benign). ✓

**Placeholder scan:** No TBD/TODO; every code step shows full code; every run step gives the command
and expected result. ✓

**Type/name consistency:** `EnvironmentFns`, `ComponentFns`, `setEnvironmentFns`, `setComponentFns`,
`mainWindowParent`, `layerContentsArraySizeLimit`, `layerContentsTimeSamplesSizeLimit`,
`isStageAComponent`, `saveEditsNoPrompt`, `firstSublayerIndex`, `findContentsWidget`, `findAction`,
`TestUtils::dismissNextModal`, `_saveLayerCallCount`, `StringResources::kOption / kAutoHideSessionLayer /
kDisplayLayerContents` all match the harness/registry as read from source. ✓
