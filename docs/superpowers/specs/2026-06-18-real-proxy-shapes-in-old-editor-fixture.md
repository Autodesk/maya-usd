# Real Proxy Shapes in Old Layer Editor Test Fixture

**Date:** 2026-06-18  
**Status:** Approved, ready for implementation

---

## Goal

Eliminate the `stub_stage_0` / `proxy|shape` test failures that occur when new tests are shared with the old editor test binary (`mayaUsdOldLayerEditorTests`). The root cause is that the old editor's widget code calls Maya DAG lookup APIs (e.g. `MSelectionList::add`, proxy shape registry queries) using the stub path `"stub_stage_0"`, which is not a registered Maya node.

Fix: create two real `mayaUsdProxyShape` nodes in the Maya scene during each test's `SetUp`, backed by the fixture's in-memory USD stages (exported to temp files), and wire their real DAG paths into the stub session state.

---

## Background

`OldEditorStubSessionState` creates two in-memory USD stages and gives each a `StageEntry` with `_proxyShapePath = "stub_stage_0"` / `"stub_stage_1"`. The old editor's widgets use `_proxyShapePath` as a Maya DAG path to locate the associated `MayaUsdProxyShape` node. Since no such node exists, Maya emits an error for every API call that references the path. This causes `MGlobal::displayError` to accumulate, and the Maya command eventually returns `MS::kFailure`, aborting the Python test runner.

The new editor avoids this via DCC-agnostic fallbacks (querying the stage directly). The old editor has no such fallback.

---

## Design

### SetUp — two proxy shapes per test

After the existing stub setup creates both in-memory stages:

1. **Export each stage to a temp file.**  
   `stage->GetRootLayer()->Export(tempPath)` serialises the full layer stack (root + anonymous sublayer) that the stub already built. Files are placed in the OS temp directory:
   ```
   <temp>/le_test_stage_0.usda
   <temp>/le_test_stage_1.usda
   ```

2. **Create a Maya node pair per stage** via `MGlobal::executeCommand`:
   ```
   createNode transform -n "leTestXform0"
   createNode mayaUsdProxyShape -n "leTestProxy0" -p leTestXform0
   setAttr "leTestProxy0.filePath" -type "string" "<tempPath0>"
   ```
   Deterministic node names avoid DAG path ambiguity. Maya opens the file, registers the stage in the proxy shape registry, and assigns a full DAG path such as `|leTestXform0|leTestProxy0`.

3. **Retrieve the full DAG path** via `MSelectionList` + `MDagPath::fullPathName()`. Store in `_proxyShapePaths[i]`.

4. **Patch the stub session state** so the stage entries carry real DAG paths:
   ```cpp
   _sessionState.setProxyShapePath(0, _proxyShapePaths[0]);
   _sessionState.setProxyShapePath(1, _proxyShapePaths[1]);
   ```
   `setProxyShapePath` also updates `_stageEntry._proxyShapePath` when the index matches the active entry (index 0 by default).

### TearDown — cleanup per test

```
MGlobal::executeCommand("delete |leTestXform0 |leTestXform1")
```
Then `std::filesystem::remove()` on both temp files. Both operations are guarded by non-empty path checks so partial-setup failures don't cascade.

### Stage divergence — not a concern for these tests

The proxy shape opens its own `UsdStageRefPtr` from the exported file, distinct from the one the fixture's session state holds in memory. Tests that go through the session state's in-memory stage (tree model, layer helpers) and tests that go through the proxy shape's Maya API will see structurally identical stages (same root + sublayer). Any in-test mutations to the in-memory stage are not reflected in the proxy's stage. None of the new tests mutate the stage then query it via proxy — they test widget UI behaviour, not layer content changes. If such a test is added in future it should be noted in `PARITY_FAILURES_DETAIL.md`.

---

## Files Changed

### `lib/usd/ui/layerEditor/test/cpp/testFixture.h`

Add two members to `LayerEditorTestFixture`:

```cpp
std::string _proxyShapePaths[2];   // real Maya DAG paths, e.g. "|leTestXform0|leTestProxy0"
std::string _tempStagePaths[2];    // temp .usda paths written during SetUp
```

### `lib/usd/ui/layerEditor/test/cpp/testFixture.cpp`

**`SetUp()`** — append after existing stub setup:

```cpp
// Export stages to temp files and create proxy shapes.
// Use generic_string() throughout so forward slashes reach MEL on Windows
// (backslashes are escape characters in MEL string literals).
namespace fss = std::filesystem;
for (int i = 0; i < 2; ++i) {
    _tempStagePaths[i] = (fss::temp_directory_path()
        / ("le_test_stage_" + std::to_string(i) + ".usda")).generic_string();
    const auto stages = _sessionState.allStages();
    stages[i]._stage->GetRootLayer()->Export(_tempStagePaths[i]);

    const std::string xform = "leTestXform" + std::to_string(i);
    const std::string shape = "leTestProxy" + std::to_string(i);
    MGlobal::executeCommand(MString("createNode transform -n \"") + xform.c_str() + "\"");
    MGlobal::executeCommand(MString("createNode mayaUsdProxyShape -n \"")
        + shape.c_str() + "\" -p " + xform.c_str());
    MGlobal::executeCommand(MString("setAttr \"") + shape.c_str()
        + ".filePath\" -type \"string\" \"" + _tempStagePaths[i].c_str() + "\"");

    MSelectionList sel;
    sel.add(MString(shape.c_str()));
    MDagPath dagPath;
    sel.getDagPath(0, dagPath);
    _proxyShapePaths[i] = dagPath.fullPathName().asChar();

    _sessionState.setProxyShapePath(i, _proxyShapePaths[i]);
}
```

**`TearDown()`** — prepend before existing teardown:

```cpp
for (int i = 0; i < 2; ++i) {
    if (!_proxyShapePaths[i].empty()) {
        // Delete the transform (which deletes the shape child)
        std::string xform = "leTestXform" + std::to_string(i);
        MGlobal::executeCommand(MString("delete |") + xform.c_str());
        _proxyShapePaths[i].clear();
    }
    if (!_tempStagePaths[i].empty()) {
        std::filesystem::remove(_tempStagePaths[i]);
        _tempStagePaths[i].clear();
    }
}
```

### `lib/usd/ui/layerEditor/test/cpp/stubSessionState.h`

Add one public method to `OldEditorStubSessionState`:

```cpp
void setProxyShapePath(int index, const std::string& path);
```

### `lib/usd/ui/layerEditor/test/cpp/stubSessionState.cpp`

```cpp
void OldEditorStubSessionState::setProxyShapePath(int index, const std::string& path)
{
    if (index >= 0 && index < (int)_stages.size()) {
        _stages[index]._proxyShapePath = path;
        if (stageEntry()._id == _stages[index]._id)
            setStageEntry(_stages[index]);
    }
}
```

### `test/lib/usdLayerEditor/cpp/testComponentSaveWidgetLogic.h`

Update `ComponentSaveWidgetTest::makeWidget()` — **only the `#ifdef LAYER_EDITOR_TEST_FIXTURE_INCLUDED` branch changes**; the new editor branch is untouched:

```cpp
std::unique_ptr<ComponentSaveWidget> makeWidget(const std::string& dccPath = {})
{
#ifdef LAYER_EDITOR_TEST_FIXTURE_INCLUDED
    // Old editor: use real proxy shape DAG path from fixture.
    const std::string& path = dccPath.empty() ? _proxyShapePaths[0] : dccPath;
    return std::make_unique<ComponentSaveWidget>(_mainWindow, path);
#else
    // New editor: unchanged.
    const std::string path = dccPath.empty() ? "proxy|shape" : dccPath;
    return std::make_unique<ComponentSaveWidget>(_mainWindow, &_sessionState, path);
#endif
}
```

---

## Impact on New Editor Tests

None. Every change is either:
- In `lib/usd/ui/layerEditor/test/cpp/` (never compiled into `UsdLayerEditorNewTests`), or  
- Inside a `#ifdef LAYER_EDITOR_TEST_FIXTURE_INCLUDED` block (compiled only when the old editor's fixture is in scope).

The 514 new editor tests are unaffected.

---

## Expected Outcome

After this change, `mayaUsdOldLayerEditorTests` should have:
- Two real Maya proxy shape nodes registered in the scene per test
- All `stub_stage_0` / `proxy|shape` `MGlobal::displayError` calls eliminated
- Previously-failing tests now able to exercise old editor widget code paths
- Pre-existing 1 failure (`SaveLayersDialogTest.AllAsRelative_ToggleDoesNotCrash`) unchanged — it self-skips for a different architectural reason documented in `PARITY_FAILURES_DETAIL.md`
- Potentially some new assertion-level failures surfaced (old editor behaviour differs from new editor) — these should be documented in `PARITY_FAILURES_DETAIL.md`
