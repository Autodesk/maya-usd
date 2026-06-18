# Parity Test Failures — Detailed Analysis

3 tests fail when running the old layer editor (`lib/usd/ui/layerEditor`) against the shared
parity suite. This document examines each one in depth.

---

## Failure 1 — `LayerTreeModelTest.Rebuild_SkipsResetWhenLayersAreIdentical`

### What it checks

After a `forceRefresh()` on a model whose layer structure has not changed, `modelReset` must
**not** be emitted. The test connects a counter to the signal, triggers a second rebuild of the
same state, then asserts the counter is zero.

### Why the old editor fails

**Old editor** (`lib/usd/ui/layerEditor/layerTreeModel.cpp:301`):

```cpp
void LayerTreeModel::rebuildModel(bool refreshLockState)
{
    _rebuildOnIdlePending = false;
    _lastAskedAnonLayerNameSinceRebuild = 0;

    beginResetModel();            // ← fires unconditionally, always

    if (rowCount() > 0)
        removeRows(0, rowCount());

    // ... rebuild items ...

    endResetModel();              // ← signal fires here, every time
}
```

**New editor** (`lib/usdUfe/usd-layer-editor/lib/layerTreeModel.cpp:305`):

```cpp
void LayerTreeModel::rebuildModel(bool refreshLockState)
{
    // Build candidate new items without touching the model.
    auto newRootItem    = std::make_unique<LayerTreeItem>(...);
    auto newSessionItem = ...;

    const bool rootIdentical    = newRootItem->isIdenticalItem(oldRootItem);
    const bool sessionIdentical = ...;

    if (!refreshLockState && rootIdentical && sessionIdentical) {
        return;                   // ← early exit: no reset, no signal
    }

    beginResetModel();
    // ... remove and append ...
    endResetModel();
}
```

### Where the optimization came from

The `isIdenticalItem()` method and the skip-if-identical check were added to the new editor as
part of bug fix **EMSUSD-3680** ("rebuilding the model when layer structure has not changed
should not emit modelReset, to avoid redundant tree redraws"). The old editor was not touched
because it was already in maintenance mode by the time that fix landed.

### Is this surprising?

Yes. The old editor is more feature-rich in terms of production capabilities, but it pre-dates
the signal-efficiency work. The optimization was developed *during* the new editor's build,
not before. There was no reason to back-port it since the old editor was already being retired
at the time.

**Verdict:** new-editor-only optimization. The old editor's behavior (always reset) is
functionally correct but fires unnecessary `modelReset` signals, causing the tree view to
redraw even when nothing changed.

---

## Failure 2 — `SaveLayersDialogTest.AllAsRelative_ToggleDoesNotCrash`

### What it checks

Opens a `SaveLayersDialog`, finds a `QCheckBox` (the "Save All As Relative" toggle that
appears when anonymous layers are present), toggles it twice, expects no crash.

### The skip message

```
Skipped — No checkbox present (no anonymous layers in stub)
```

The test guards itself:

```cpp
auto* cb = dlg.findChild<QCheckBox*>(...);
if (!cb) GTEST_SKIP() << "No checkbox present (no anonymous layers in stub)";
```

If no checkbox is found, it skips. The checkbox only appears when the dialog populates a
non-empty anonymous layers list.

### Why both stubs create anonymous layers but only one editor sees them

The stubs are **identical** in setup:

```cpp
// Both OldEditorStubSessionState and StubSessionState do:
auto stage    = PXR_NS::UsdStage::CreateInMemory();
auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sublayer0");
stage->GetRootLayer()->InsertSubLayerPath(sublayer->GetIdentifier(), 0);
```

The anonymous sublayer exists in the USD data. The problem is in how each editor's
`SaveLayersDialog::getLayersToSave()` retrieves it:

**Old editor** (`lib/usd/ui/layerEditor/saveLayersDialog.cpp:535`):

```cpp
MayaUsd::utils::getLayersToSaveFromProxy(proxyPath, StageLayersToSave);
```

`proxyPath` is `stageEntry._proxyShapePath`, which the stub sets to `"stub_stage_0"`. This
function looks up a Maya proxy shape node in the active Maya scene by DAG path and queries
which of its layers need saving. Since `"stub_stage_0"` is not a real Maya DAG path pointing
to a registered `MayaUsdProxyShape` node, the lookup returns nothing — `StageLayersToSave`
is empty, `_anonLayerInfos` stays empty, the checkbox is never created.

**New editor** (`lib/usdUfe/usd-layer-editor/lib/saveLayersDialog.cpp:561`):

```cpp
if (stage) {
    Serialization::getLayersToSaveFromStage(stage, objectPath, StageLayersToSave);
} else {
    Serialization::getLayersToSaveFromDCCObject(objectPath, StageLayersToSave);
}
```

The new editor passes the actual `UsdStageRefPtr` and falls back to a **stage-direct
inspection** if the DCC object path is not resolvable. `getLayersToSaveFromStage` walks the
stage's layer stack directly, finds the anonymous sublayer in the stub's stage object, and
populates `_anonLayerInfos`. The checkbox appears. The test runs.

The comment in the new editor's code explains this was a deliberate fix:

```cpp
// Get the layers to save for this stage. Use the stage directly when available
// to avoid UFE path-format mismatches (e.g. Maya DAG paths vs |world-prefixed
// UFE keys) that cause getLayersToSaveFromDCCObject to silently return nothing.
```

### Is this surprising?

Yes. The anonymous layer capability itself is identical — both editors have it, both stubs have
the data. The difference is that the old editor's dialog is **architecturally coupled to Maya's
proxy shape registry**: it cannot discover layers without a real registered proxy shape node.
The new editor's dialog has a stage-direct fallback that works with just a `UsdStageRefPtr`.

This is a testability improvement in the new editor, not a user-visible feature difference.
In production both editors are always given real proxy shape paths. But the old editor's
tighter coupling to Maya means its dialog code cannot be exercised in isolation — you need a
full Maya scene with a registered proxy shape to exercise the anonymous layer path.

**Verdict:** architectural — old editor's `SaveLayersDialog` cannot discover anonymous layers
without a real Maya proxy shape in the scene. The test cannot exercise that code path with a
stub, so the checkbox never appears and the test self-skips. The underlying crash-prevention
logic being tested is present in both editors; only the testability differs.

---

## Failure 3 — `ReferencedLayersFixture.IsReadOnly_TrueForReferencedLayer`

### What it checks

A sublayer listed in the stage root layer's "referenced layers" metadata must be marked
read-only by `LayerTreeItem::isReadOnly()`.

The fixture stamps the metadata before the widget is built:

```cpp
CustomLayerData::setStringArray(refs, rootLayer, TfToken("adskSharedLayers"));
```

### Why the old editor misses it

**Old editor** reads referenced layers with:

```cpp
// lib/mayaUsd/base/tokens.h line 114:
((ReferencedLayers, "mayaSharedLayers"))

// lib/usd/ui/layerEditor/layerTreeModel.cpp:328:
auto layers = MayaUsd::CustomLayerData::getStringArray(
    rootLayer, MayaUsdMetadata->ReferencedLayers);   // reads "mayaSharedLayers"
```

**New editor** reads referenced layers with:

```cpp
// lib/usdUfe/usd-layer-editor/lib/tokens.h line 58:
/* TODO LE-EXTRACT : mayaSharedLayers -> dccSharedLayers, do we need to
   support cross DCC metadata? */
((ReferencedLayers, "adskSharedLayers"))

// lib/usdUfe/usd-layer-editor/lib/layerTreeModel.cpp:330:
auto layers = CustomLayerData::getStringArray(
    rootLayer, UsdLayerEditorMetadata->ReferencedLayers);   // reads "adskSharedLayers"
```

The test fixture writes `"adskSharedLayers"` (the new editor's token). The old editor looks
for `"mayaSharedLayers"`. The metadata key doesn't match → no layers are added to
`sharedLayers` → `LayerTreeItem::_isSharedLayer` is never set to `true` →
`isReadOnly()` returns `false`.

### Why the token was renamed

The TODO comment in the new editor's token file is explicit:

```
/* TODO LE-EXTRACT : mayaSharedLayers -> dccSharedLayers, do we need to
   support cross DCC metadata? */
```

The rename from `"mayaSharedLayers"` to `"adskSharedLayers"` was made to drop the
Maya-specific prefix in preparation for the layer editor being shared across DCCs. The
intent was to eventually land on a fully DCC-neutral name (`"dccSharedLayers"`), but the
current state settled on `"adskSharedLayers"` as an intermediate.

The old editor was not updated to read the new key.

### Production impact

This is not just a test gap — it is a **real behavioral difference in production**. If a USD
file was saved by the new editor with `"adskSharedLayers"` metadata, and then opened with the
old editor (`BUILD_NEW_LAYER_EDITOR=OFF`), those referenced layers would not be marked
read-only. Users could accidentally edit layers that should be protected.

Conversely, USD files written by the old editor using `"mayaSharedLayers"` would not have
their referenced layers recognized by the new editor.

**Verdict:** deliberate token rename, not back-ported. Real production incompatibility: files
written by one editor will not have referenced layers correctly protected when opened by the
other. Both `"mayaSharedLayers"` and `"adskSharedLayers"` metadata should be checked by
whichever editor is active during the transition period.

---

## Failure 4 — `LayerEditorTestFixture.Widget_GetSelectedLayers_NothingSelected_ReturnsEmpty`
## Failure 5 — `LayerEditorTestFixture.Widget_SelectLayers_Empty_ClearsSelection`

These two failures share the same root cause and are documented together.

### What they check

**Failure 4:** After calling `_widget->selectLayers({})` (with an empty list), `getSelectedLayers()` must return an empty vector.

**Failure 5:** After `selectLayers({})`, the tree view's `selectionModel()->currentIndex()` must be invalid (no current item).

### Why the old editor fails

`LayerEditorWidget::selectLayers()` (old editor, `lib/usd/ui/layerEditor/layerEditorWidget.cpp`) clears the Qt selection model's selected rows but does **not** also clear `currentIndex`. The `getSelectedLayerItems()` helper falls back to the current index when `selectedRows()` is empty:

```cpp
// Old editor — simplified:
auto selected = layerTree()->selectionModel()->selectedRows();
if (selected.isEmpty())
    selected = { layerTree()->selectionModel()->currentIndex() };  // ← fallback
```

Since `currentIndex` is still valid after `selectLayers({})`, `getSelectedLayerItems()` returns the current item, so `getSelectedLayers()` is non-empty.

The new editor's `selectLayers({})` explicitly clears both selected rows **and** `currentIndex`:

```cpp
// New editor:
selectionModel->clearSelection();
selectionModel->clearCurrentIndex();  // ← extra step
```

### Production impact

None in typical usage — `selectLayers({})` is only called explicitly from code. In production the widget always has a real selection. The gap only surfaces in the test environment where the initial current index is set during fixture construction.

### Fix path

Add `selectionModel()->clearCurrentIndex()` to `layerEditorWidget.cpp` in the `selectLayers({})` branch (when `in_layerIds` is empty). This is a one-line patch to the old editor.

**Note:** Per the agreed review policy (2026-06-18), no changes to tests or the old editor are made until this report is reviewed.

---

## Summary

| Failure | Category | Production impact | Fix path |
|---------|----------|-------------------|----------|
| `Rebuild_SkipsReset` | Missing optimization | None — old behavior is correct, just fires extra signals | Back-port `isIdenticalItem` skip to old editor, or accept divergence |
| `AllAsRelative_ToggleDoesNotCrash` | Architectural coupling to Maya proxy shape | None in production (real proxy always present) — only affects testability | Add `getLayersToSaveFromStage` fallback to old editor's dialog, or accept skip |
| `IsReadOnly_TrueForReferencedLayer` | Token rename not back-ported | **Real** — files written by new editor lose read-only protection in old editor | Old editor should also read `"adskSharedLayers"`, or a migration step renames the key in USD files |
| `Widget_GetSelectedLayers_NothingSelected_ReturnsEmpty` | `selectLayers({})` doesn't clear `currentIndex` | None in production | Add `clearCurrentIndex()` to old editor's `selectLayers` empty-list branch |
| `Widget_SelectLayers_Empty_ClearsSelection` | Same as above | None in production | Same fix |
