# Design: C++ Test Coverage for Layer-Editor Discrepancy Fixes

Date: 2026-06-10

## Context

`lib/usdLayerEditor/SLOP_REVIEW.md` documented logic discrepancies (D1–D10) introduced while
unifying the Maya layer editor into the shared `lib/usdLayerEditor/lib/` component. The code fixes
have landed and pass build + the existing layer-editor test suites, but the fixes themselves are not
yet pinned by automated tests. This design adds focused C++ coverage so the fixed behavior cannot
silently regress again.

Scope was agreed with the maintainer: cover **D8, D9, D10** behaviorally, plus **registry-contract
tests** for the new DCC-function accessors that are in scope (`mainWindowParent`,
`layerContentsArraySizeLimit`, `layerContentsTimeSamplesSizeLimit`). **D5 is out of scope** (no test,
no refactor). **D6** is covered only by its `mainWindowParent()` contract test — not by a
widget-driven dialog-parenting test. D1/D2/D3 were "no fix / benign" conclusions and need no tests.

All tests go into the existing `UsdLayerEditorNewTests` target
(`test/lib/usdLayerEditor/cpp/`), which links the shared `UsdLayerEditorLib`, runs under a real
`QApplication` (set up in `testMain.cpp`), and provides:
- `LayerEditorTestFixture` (`testFixture.h`) — builds a `StubLayerEditorWindow` + `LayerEditorWidget`
  around a `StubSessionState` with two in-memory stages, each with one anonymous sublayer.
- `StubSessionState` (`stubSessionState.{h,cpp}`) — overrides `SessionState`; embeds a
  `StubCommandHook`; exposes `_saveLayerCallCount` and `addStage()/removeStage()/setStageEntry()`.
- `StubCommandHook` (`stubCommandHook.{h,cpp}`) — records calls (`hasCall`, `callCount`, `lastCall`).
- `ScopedLayerEditorDCCFunctions` (`scopedLayerEditorDCCFunctions.h`) — RAII save/restore of the
  whole DCC-functions registry; use it in any test that installs stub DCC fns.
- `findActionInMenuBar(win, text)` (`testMenusAndStageLogic.h`) — menu introspection helper.
- `findContentsWidget(root)` (`testLayerContentsWidgetLogic.h`) — locates the `LayerContentsWidget`.

## Test design

### 1. Registry-contract tests — new `testDCCFunctions.cpp` (+ `testDCCFunctionsLogic.h`)

Plain `TEST`s (no fixture/widgets). Each test constructs a local `ScopedLayerEditorDCCFunctions` so
global registry state is restored on exit. Pattern mirrors the fixture's `setEnvironmentFns` usage.

- **`MainWindowParent_DefaultsToNull`**: with a fresh/default `EnvironmentFns`, assert
  `UsdLayerEditor::mainWindowParent() == nullptr`.
- **`MainWindowParent_ReturnsRegisteredWidget`**: install `EnvironmentFns` whose `mainWindowParent`
  returns a stack `QWidget`; assert `UsdLayerEditor::mainWindowParent()` returns that pointer.
- **`LayerContentsLimits_DefaultToEight`**: with default `EnvironmentFns`, assert
  `layerContentsArraySizeLimit() == 8` and `layerContentsTimeSamplesSizeLimit() == 8`.
- **`LayerContentsLimits_ReturnRegisteredValues`**: install getters returning e.g. `3` and `5`;
  assert the accessors return those values.

These lock the "unset → documented default, set → registered value" contract that D6/D8 rely on.

### 2. D9 — auto-hide menu action restored & ordered first

Add to `testMenusAndStageLogic.h`. Add a small helper `findMenuByTitle(win, title)` that returns the
`QMenu*` whose title matches `StringResources::getAsQString(StringResources::kOption)`.

- **`OptionMenu_AutoHideAction_IsFirstAndCheckable`**:
  - Resolve the Option menu; read `menu->actions()`.
  - Assert `actions()[0]` text == `StringResources::getAsQString(kAutoHideSessionLayer)`, and
    `actions()[0]->isCheckable()`, and `isChecked() == _sessionState.autoHideSessionLayer()`.
  - Assert a separator (`isSeparator()`) appears before the Display-Layer-Contents action, and that
    Display-Layer-Contents comes after auto-hide (locks the restored ordering: auto-hide → separator
    → display items → EF items).

Use `StringResources` values for action text rather than hardcoded literals.

### 3. D10 — component-creator early-out in `saveAnonymousLayer`

Add to `testLayerTreeItemLogic.h`. Trigger via the public `LayerTreeItem::saveEditsNoPrompt()` on an
anonymous, dirty layer item (the stub stages already carry an anonymous sublayer; dirty it first,
e.g. `item->layer()->SetComment(...)`). Guard registry state with `ScopedLayerEditorDCCFunctions`.

- **`SaveAnonymousLayer_ComponentStage_DelegatesToSaveStage`**:
  - Install `ComponentFns` with `isStageAComponent → true` (for the current entry's `_dccObjectPath`)
    and a recording `saveComponent`. (Provide a no-op `displayError` as existing command tests do.)
  - Call `saveEditsNoPrompt()` on the anonymous item.
  - Assert `saveComponent` was invoked **and** `StubSessionState::_saveLayerCallCount == 0` (the
    generic anonymous-save path was skipped).
- **`SaveAnonymousLayer_NonComponentStage_UsesGenericPath`**:
  - Install `ComponentFns` with `isStageAComponent → false`.
  - Call `saveEditsNoPrompt()`.
  - Assert the generic path ran — `StubSessionState::_saveLayerCallCount > 0` (it calls
    `saveLayerUI`) and `saveComponent` was not invoked.

Implementer note: confirm the exact entry method (`saveEditsNoPrompt()` vs `saveEdits()`) and the
preconditions (anonymous + dirty) by reading the current `layerTreeItem.cpp` flow; `saveStage`'s
component branch routes through `UsdLayerEditor::saveComponent`, so the recording `saveComponent`
stub is the observable.

### 4. D8 — layer-contents size limit is applied

Add to `testLayerContentsWidgetLogic.h`. `exportPseudoLayer` is **private**, so drive the public
`LayerContentsWidget::setLayer(layer)` and read the rendered text from the inner `QTextEdit`
(`cw->findChild<QTextEdit*>()->toPlainText()`). Guard registry state with
`ScopedLayerEditorDCCFunctions`.

- **`SetLayer_RespectsArraySizeLimit`**:
  - Build an in-memory stage; define a prim with an array attribute holding many elements (e.g. an
    `int[]` of ~100 values); take its root layer.
  - Render once with `EnvironmentFns::layerContentsArraySizeLimit` stubbed **small** (e.g. 2) and once
    **large** (e.g. 1000), each time `cw->setLayer(layer)` then reading the QTextEdit text.
  - Assert the small-limit output is strictly shorter than the large-limit output (robust to the exact
    elision marker). This proves the widget applies the registry getter rather than a hardcoded
    constant. Both limits share the same plumbing, so the array case is sufficient.

## Out of scope
- **D5** (exact stage-entry match after rename) — no test; the matching loop stays inlined in
  `SaveLayersDialog::onSaveAll`. The maintainer chose to skip it (would require driving the full
  component-save flow or extracting a helper).
- **D6 dialog parenting behavior** — only the `mainWindowParent()` contract test; no
  `activeModalWidget()` parenting assertion.
- **D1/D2/D3** — resolved-as-correct/benign; no regression tests.

## Verification
- `configure` if CMake source list changes (the new `testDCCFunctions.cpp` must be added to
  `test/lib/usdLayerEditor/cpp/CMakeLists.txt`), then `build`.
- Run `UsdLayerEditorNewTests` via the `test` relay command (filter `[Ll]ayer.?[Ee]ditor` or the
  specific suite) and confirm the new tests pass alongside the existing suites.
