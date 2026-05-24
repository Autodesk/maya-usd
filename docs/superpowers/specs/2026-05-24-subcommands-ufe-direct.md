# LayerEditorCommand: Replace _subCommands with UFE Commands Directly

**Goal:** Eliminate the `Impl::BaseCmd` class hierarchy entirely by storing
`Ufe::UndoableCommand` instances directly in `_subCommands`, removing ~900 lines
of thin-wrapper boilerplate from `layerEditorCommand.cpp`.

**Architecture:** Retrieve the SDF layer and USD stage once in `parseArgs()`,
construct shared-library UFE commands there with the layer baked in, and store
them as `vector<shared_ptr<Ufe::UndoableCommand>>`. The `redoIt()` /`undoIt()`
loops shrink to trivial range-for calls. No changes to the shared library or tests.

**Constraint:** `Ufe::UndoableCommand` base class is not modified.

---

## Files

- **Modify:** `lib/mayaUsd/commands/layerEditorCommand.h`
  - Change `_subCommands` type
- **Modify:** `lib/mayaUsd/commands/layerEditorCommand.cpp`
  - Delete the entire `Impl` anonymous namespace (~900 lines)
  - Move layer/stage retrieval to `parseArgs()`
  - Construct UFE commands directly in `parseArgs()`
  - Simplify `redoIt()` and `undoIt()`

---

## Design

### 1. `_subCommands` type change

**Header (`layerEditorCommand.h`):**

```cpp
// Before
std::vector<std::shared_ptr<Impl::BaseCmd>> _subCommands;

// After
std::vector<std::shared_ptr<Ufe::UndoableCommand>> _subCommands;
```

`Impl::BaseCmd` and all `Impl::*` subclasses are deleted. The forward declaration
of `Impl` in the header is also removed.

---

### 2. Layer and stage retrieval in `parseArgs()`

Both are retrieved once at the top of the argument-parsing block, before any
`push_back` call:

```cpp
SdfLayerHandle layer = getLayer(this);
if (!layer) return MS::kFailure;

UsdStageRefPtr stage = UsdMayaQuery::GetPrim(_proxyShapePath.c_str()).GetStage();
```

UFE commands are then constructed with the layer (and stage where required) passed
directly to their constructors. Each UFE command holds a `SdfLayerRefPtr`,
keeping the layer alive across undo/redo cycles — equivalent to, and slightly
safer than, re-fetching via `getLayer` on every `redoIt()`.

---

### 3. `redoIt()` and `undoIt()` loops

```cpp
MStatus LayerEditorCommand::redoIt()
{
    for (auto& cmd : _subCommands) {
        cmd->redo();
        // AddAnonSubLayerCmd is the only command that returns a result
        // (the new anonymous layer identifier).
        if (auto* anon = dynamic_cast<UsdLayerEditor::AddAnonSubLayerCmd*>(cmd.get()))
            appendToResult(anon->addedLayer().c_str());
    }
    return MS::kSuccess;
}

MStatus LayerEditorCommand::undoIt()
{
    for (auto it = _subCommands.rbegin(); it != _subCommands.rend(); ++it)
        (*it)->undo();
    return MS::kSuccess;
}
```

`doIt()` delegates to `redoIt()` unchanged. Calling `redo()` on first execution
is valid because `UsdLayerEditor::BaseCmd::execute()` already delegates to
`redo()`, making the two calls equivalent.

---

### 4. Deletions

The entire anonymous `Impl` namespace in `layerEditorCommand.cpp` is removed:

| Class | Lines (approx) |
|---|---|
| `BaseCmd` | 50 |
| `InsertSubPath` | 50 |
| `RemoveSubPath` | 50 |
| `ReplaceSubPath` | 30 |
| `MoveSubPath` | 40 |
| `AddAnonSubLayer` | 40 |
| `MuteLayer` | 80 |
| `LockLayer` | 80 |
| `RefreshSystemLockLayer` | 50 |
| `ClearLayer` | 20 |
| `FlattenLayer` | 20 |
| `DiscardEdit` | 20 |
| `StitchLayers` | 30 |
| **Total** | **~560 lines** |

The `#include <utilFileSystem.h>` added for `registerBackupStagesProvider` is
retained. No changes to the shared library (`lib/usdUfe/usd-layer-editor/`) or
any test files.

---

## Testing

No new tests required — the shared library already has full coverage of each
command, and the Maya integration test suite (`testMayaUsdLayerEditorCommands`,
30 tests) exercises the full parse→execute→undo→redo round-trip for every
command type routed through `LayerEditorCommand`.

Run after implementation:

1. Build: `ecg-maya-usd / build`
2. Shared lib tests: `ecg-maya-usd / test` filter `UsdLayerEditorNewTests`
3. Maya integration: `ecg-maya-usd / test` filter `testMayaUsdLayerEditorCommands`
