# Old Layer Editor Parity Tests

Temporary test suite that runs the shared layer editor test logic against the
**old** (legacy) layer editor widgets. It exists to guard against regressions in
the old editor while the new editor is being adopted, and will be **deleted when
the old editor is retired**.

## What it covers

The C++ test logic lives in the new editor's test directory
(`test/lib/usdLayerEditor/cpp`) as `test*.cpp` files. Those same sources are
compiled here against the old editor's widgets and fixtures, so a single body of
tests exercises both editors. Coverage of the old editor is intentionally a
subset of the new editor suite — cases that only apply to the new editor are
skipped via the `MAYAUSD_OLD_LAYER_EDITOR` compile define.

`cpp/CMakeLists.txt` builds these sources, the old editor's stubs/fixtures, and
the legacy widget sources (from `lib/usd/ui/layerEditor`) into a self-contained
Maya plugin.

## How it works

These are Qt-widget GTests: they need a running Maya with its Qt application and
USD runtime, so they cannot run as a standalone GTest executable. Instead:

1. **Maya plugin** (`cpp/`, target `mayaUsdOldLayerEditorTests`) — bundles the
   GTests and registers an MEL/Python command, `mayaUsd_runLayerEditorTests`,
   that runs them via `RUN_ALL_TESTS()` and returns the results as JSON. The
   command always returns success; per-test pass/fail is encoded in the JSON.

2. **Python runner** (`testOldLayerEditorParity.py`) — the actual ctest entry
   point. It launches Maya, loads the plugin, calls the command, parses the JSON,
   and calls `self.fail()` with the details of any failing test so ctest reports
   the failure.

This plugin + Python split is what lets a C++ GTest suite run inside a live Maya
session under ctest.

## Running

```
ctest -R testOldLayerEditorParity
```

This is an interactive test (it launches Maya), so it also carries the
auto-applied `interactive` ctest label.
