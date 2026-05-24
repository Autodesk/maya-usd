# Decisions to Review

Architectural decisions that were made with known trade-offs and should be revisited if circumstances change.

---

## No try/catch in `LayerEditorCommand::redoIt()` / `undoIt()`

**Decision:** `redoIt()` and `undoIt()` do not wrap UFE command execution in try/catch, and do not check for failure — they always return `MS::kSuccess`.

**Context:** After the `Impl` namespace was removed (`lib/mayaUsd/commands/layerEditorCommand.cpp`), `_subCommands` became `vector<shared_ptr<Ufe::UndoableCommand>>`. `Ufe::UndoableCommand::redo()` / `undo()` return `void`, so there is no return value to check.

**Old behaviour:** Each `Impl::BaseCmd::doIt()` returned `bool`. `redoIt()` stopped the loop and returned `MS::kFailure` on first failure.

**Why we didn't add try/catch:**
- The shared-library UFE commands (`MoveSubPathCmd`, `RefreshSystemLockLayerCmd`, etc.) do not throw — errors go through `TF_RUNTIME_ERROR` (logs, does not throw).
- `parseArgs()` now pre-validates layer existence and proxy shape validity before any command is constructed, so most failure modes are caught before `redoIt()` is ever called.
- In practice there is always exactly one sub-command per invocation (the flags are mutually exclusive), so the old "early exit stops subsequent commands" protection was largely theoretical.
- Catching `(...)` and returning `kFailure` would silently swallow unexpected exceptions and make real bugs harder to diagnose.

**When to revisit:** If sub-commands are ever batched (multiple flags in one invocation where ordering matters), or if any UFE command in this path starts throwing, add a per-command try/catch that logs the exception via `TF_RUNTIME_ERROR` and returns `MS::kFailure`.
