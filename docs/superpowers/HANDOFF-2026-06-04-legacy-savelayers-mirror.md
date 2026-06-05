# Handoff: Mirror new-editor layer gathering in the legacy SaveLayersDialog

**Date:** 2026-06-04
**Repo:** `maya-usd` submodule, branch `feature/unify_layer_editors`
**Parent repo:** `ecg-maya-usd`, branch `deboisj/unify_LE`
**Author of handoff:** prior Claude session

---

## TL;DR for the next session

One test in the legacy layer-editor parity suite (`mayaUsdOldLayerEditorTests`) **skips** instead of running:

```
SaveLayersDialogTest.AllAsRelative_ToggleDoesNotCrash
  → "No checkbox present (no anonymous layers in stub)"
```

The user wants it to **run**, by making the legacy `SaveLayersDialog` gather layers the same way the new editor already does — **from the stage directly** instead of re-resolving the stage from a real Maya proxy-shape node. The user's exact words:

> "Mirror the new editor in the legacy dialog. But just add a capitalized comment."

So: make the minimal legacy change that mirrors the new editor, and flag the deviation with a **prominent capitalized comment**.

**Do NOT** change the stub to create real Maya proxy shapes — that approach was considered and rejected as too heavy/risky. The decision is to change the dialog's gathering path.

---

## How we got here (context)

This branch unifies two layer-editor implementations:
- **Legacy / old:** `lib/usd/ui/layerEditor/` — widget sources compiled into the test
- **New / shared:** `lib/usdUfe/usd-layer-editor/lib/` — `UsdLayerEditorLib.dll`

Earlier this session we removed the `BUILD_NEW_LAYER_EDITOR` CMake switch so **both** editors always build (see `docs/superpowers/specs/2026-06-04-always-build-both-editors-design.md` and the matching plan). That work is **done and verified**:

- `UsdLayerEditorNewTests` — **222/222 pass**
- `mayaUsdOldLayerEditorTests` — builds, runs, **1 test skips** (the one this handoff is about). No real failures.

The skip is a long-standing pre-existing issue, not a regression from the build refactor.

⚠️ **Note on existing commits:** the recent commit messages on this branch have garbled `Co-Authored-By` trailers (text got spliced into the subject line). Cosmetic only. Write clean commit messages going forward; consider a single-line subject + body via a heredoc.

---

## Root cause (fully investigated — do not re-derive)

The test `AllAsRelative_ToggleDoesNotCrash` lives in the **shared** logic header
`lib/usdUfe/usd-layer-editor/test/cpp/testSaveLayersDialogLogic.h:110`. It constructs a
`SaveLayersDialog` from the stub session state and looks for the "all as relative" `QCheckBox`.
That checkbox is only created when the dialog has **anonymous layers to save**
(`_anonLayerInfos` non-empty). When there are none, the test calls `GTEST_SKIP()`.

The two editors populate `_anonLayerInfos` differently:

### New editor (works — finds the in-memory stub's anonymous layers)
`lib/usdUfe/usd-layer-editor/lib/saveLayersDialog.cpp:561`
```cpp
if (stage) {
    Serialization::getLayersToSaveFromStage(stage, objectPath, StageLayersToSave);  // walks the stage directly
} else {
    Serialization::getLayersToSaveFromDCCObject(objectPath, StageLayersToSave);
}
```
`getLayersToSaveFromStage` (`lib/usdUfe/usd-layer-editor/lib/utilSerialization.cpp:626`) walks
`stage->GetRootLayer()` / session layer directly — no proxy lookup. The in-memory stub stage
(which has an anonymous sublayer) is found, the checkbox appears, the test **runs and passes**.
There is even a comment there (lines 557-559) noting this was a deliberate change to avoid
path-resolution fragility.

### Legacy editor (skips — cannot find the in-memory stub's layers)
`lib/usd/ui/layerEditor/saveLayersDialog.cpp:528` `getLayersToSave()`:
```cpp
MayaUsd::utils::getLayersToSaveFromProxy(proxyPath, StageLayersToSave);
```
`getLayersToSaveFromProxy` (`lib/mayaUsd/utils/utilSerialization.cpp:619`):
```cpp
auto stage = UsdMayaUtil::GetStageByProxyName(proxyPath);  // looks up a REAL Maya proxy node
if (!stage) { return; }                                    // ← bails: stub proxyPath is fake
```
`GetStageByProxyName` → `GetProxyShapeByProxyName` requires a real `MayaUsdProxyShapeBase` node in
the Maya scene. The stub's `_proxyShapePath` is the string `"stub_stage_0"`, which matches no real
node → returns null → `_anonLayers` stays empty → no checkbox → **test skips**.

**The dialog already receives the `stage` object** (`getLayersToSave`'s first parameter) but
currently ignores it and re-resolves via the proxy path. Mirroring the new editor means using that
stage directly.

---

## The fix (what to implement)

Mirror the new editor: when a stage is available, gather layers from the stage directly; otherwise
fall back to the existing proxy path. This needs a stage-based helper in legacy `MayaUsd::utils`,
because the stage-walking primitive `populateChildren` is **file-local** (anonymous namespace in
`utilSerialization.cpp:50`) and cannot be called from the dialog.

### Change 1 — add `getLayersToSaveFromStage` to legacy `MayaUsd::utils`

The body already exists, inlined inside `getLayersToSaveFromProxy`
(`lib/mayaUsd/utils/utilSerialization.cpp:641-662`). Extract it into a new function and have
`getLayersToSaveFromProxy` call it after it resolves the stage (and after the component special-case).

**`lib/mayaUsd/utils/utilSerialization.h`** — declare next to `getLayersToSaveFromProxy` (line 202):
```cpp
MAYAUSD_CORE_PUBLIC
void getLayersToSaveFromStage(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            proxyPath,
    StageLayersToSave&            layersInfo);
```
(Check the existing export macro on `getLayersToSaveFromProxy` in this header and match it exactly —
do not guess the macro name.)

**`lib/mayaUsd/utils/utilSerialization.cpp`** — define it by extracting the existing stage-walk
(current lines 641-662), and refactor `getLayersToSaveFromProxy` to call it:
```cpp
void getLayersToSaveFromStage(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            proxyPath,
    StageLayersToSave&            layersInfo)
{
    if (!stage) {
        return;
    }

    auto root = stage->GetRootLayer();
    populateChildren(
        proxyPath, stage, root, nullptr, layersInfo._anonLayers, layersInfo._dirtyFileBackedLayers);
    if (root->IsAnonymous()) {
        LayerInfo info;
        info.stage = stage;
        info.layer = root;
        info.parent._proxyPath = proxyPath;
        info.parent._layerParent = nullptr;
        layersInfo._anonLayers.push_back(info);
    } else if (root->IsDirty()) {
        layersInfo._dirtyFileBackedLayers.push_back(root);
    }

    auto session = stage->GetSessionLayer();
    populateChildren(
        proxyPath, stage, session, nullptr,
        layersInfo._anonLayers, layersInfo._dirtyFileBackedLayers);
}
```
Then `getLayersToSaveFromProxy` becomes (keeping its proxy lookup + component special-case):
```cpp
void getLayersToSaveFromProxy(const std::string& proxyPath, StageLayersToSave& layersInfo)
{
    auto stage = UsdMayaUtil::GetStageByProxyName(proxyPath);
    if (!stage) {
        return;
    }

    // ... existing ComponentUtils::isAdskUsdComponent(proxyPath) special-case block unchanged ...

    getLayersToSaveFromStage(stage, proxyPath, layersInfo);
}
```
Verify the extracted code is byte-identical in behavior to the original lines 641-662 (it is, above).

### Change 2 — use the stage directly in the legacy dialog (+ CAPITALIZED comment)

**`lib/usd/ui/layerEditor/saveLayersDialog.cpp`**, in `getLayersToSave` (currently line 528-535),
replace:
```cpp
    // Get the layers to save for this stage.
    MayaUsd::utils::StageLayersToSave StageLayersToSave;
    MayaUsd::utils::getLayersToSaveFromProxy(proxyPath, StageLayersToSave);
```
with (the **capitalized comment is required by the user**; tighten wording to project style — concise):
```cpp
    // MIRRORS THE NEW SHARED LAYER EDITOR: gather from the stage directly when available
    // instead of re-resolving it from a Maya proxy-shape node, which fails for stages not
    // backed by a real proxy (e.g. test stubs). See lib/usdUfe/usd-layer-editor/lib/saveLayersDialog.cpp.
    MayaUsd::utils::StageLayersToSave StageLayersToSave;
    if (stage) {
        MayaUsd::utils::getLayersToSaveFromStage(stage, proxyPath, StageLayersToSave);
    } else {
        MayaUsd::utils::getLayersToSaveFromProxy(proxyPath, StageLayersToSave);
    }
```
`stage` is already the first parameter of `getLayersToSave` (line 529), so no new plumbing needed.

> Note: the dialog's second constructor (line 516) passes `stageEntry._stage` into `getLayersToSave`,
> and the stub's current stage entry has a valid in-memory stage with an anonymous sublayer at
> index 0. So the `if (stage)` branch will be taken in the test and find the anon layer.

---

## Files touched (all legacy — keep changes minimal & surgical)

| File | Change |
|---|---|
| `lib/mayaUsd/utils/utilSerialization.h` | Declare `getLayersToSaveFromStage` |
| `lib/mayaUsd/utils/utilSerialization.cpp` | Define it (extract from `getLayersToSaveFromProxy`); refactor proxy fn to call it |
| `lib/usd/ui/layerEditor/saveLayersDialog.cpp` | Use stage directly when available + CAPITALIZED comment |

Do not touch the stub, the test, or any other file. Do not "improve" adjacent code.

---

## How to build & test (sandboxed — use the relay, never run build/test directly)

Pre-flight:
```bash
python3 /d/repos/agent_repos/ecg-maya-usd/_host_command/relay_client.py --help > /dev/null && echo ok
```

Build (one request at a time; exit 2 = relay busy, exit 3 = timeout → retry):
```bash
result=$(python3 /d/repos/agent_repos/ecg-maya-usd/_host_command/relay_client.py run build \
  --db /d/repos/agent_repos/ecg-maya-usd/_host_command/relay.db \
  --commands-json /d/repos/agent_repos/ecg-maya-usd/_host_command/commands.json)
python3 -c "import json,sys; d=json.loads(sys.argv[1]); print('exit',d['exit_code']); print(d['stdout'][-3000:])" "$result"
```
(If CMake files changed, run `configure` first the same way. For this task only `.h`/`.cpp` change,
so a plain `build` is enough — but `configure` is harmless.)

Run the legacy parity test:
```bash
result=$(python3 /d/repos/agent_repos/ecg-maya-usd/_host_command/relay_client.py run test mayaUsdOldLayerEditorTests \
  --db /d/repos/agent_repos/ecg-maya-usd/_host_command/relay.db \
  --commands-json /d/repos/agent_repos/ecg-maya-usd/_host_command/commands.json)
python3 -c "import json,sys; d=json.loads(sys.argv[1]); print('exit',d['exit_code']); print(d['stdout'][-3000:])" "$result"
```

### Success criteria
- Build links `mayaUsdUI`, `UsdLayerEditorLib`, and `mayaUsdOldLayerEditorTests` cleanly.
- `mayaUsdOldLayerEditorTests`: `SaveLayersDialogTest.AllAsRelative_ToggleDoesNotCrash`
  **no longer skips** — it runs and passes (the checkbox is found, toggled true/false, no crash).
- No other test in the suite regresses.
- Sanity-check the new editor still passes (you changed a shared-ish utility namespace — but legacy
  `MayaUsd::utils` and new `UsdLayerEditor::Serialization` are **separate**, so this should not affect
  `UsdLayerEditorNewTests`; verify anyway):
  ```bash
  ... run test UsdLayerEditorNewTests ...   # expect 222/222
  ```

---

## Guardrails (from CLAUDE.md — follow these)

- **Sandboxed:** never run build/test directly; always via the relay above. One request at a time.
- **Git:** read-only git unless the user authorizes commits. The user has been committing each logical
  step — ask before committing, or follow their lead. End commit messages with the `Co-Authored-By`
  trailer and keep the subject clean (avoid the garbled-trailer problem noted above).
- **Code style:** concise comments; comment only non-obvious logic/deviations. The capitalized comment
  is the *one* required comment here — keep it tight.
- **Surgical:** every changed line must trace to this task. Don't refactor unrelated code.

---

## Verification of facts in this doc (line numbers as of this writing)

- Skip site: `lib/usdUfe/usd-layer-editor/test/cpp/testSaveLayersDialogLogic.h:110-120`
- New gather: `lib/usdUfe/usd-layer-editor/lib/saveLayersDialog.cpp:561`,
  `lib/usdUfe/usd-layer-editor/lib/utilSerialization.cpp:626`
- Legacy gather: `lib/usd/ui/layerEditor/saveLayersDialog.cpp:528-535`,
  `lib/mayaUsd/utils/utilSerialization.cpp:619-663`
- `populateChildren` is file-local (anon namespace `utilSerialization.cpp:50`, fn at line 68)
- Legacy `MayaUsd::utils` has **no** existing `getLayersToSaveFromStage` (only `...FromProxy`)

Line numbers may drift — grep to confirm before editing.
