# USD Layer Editor Migration Tracking

Resume point for porting maya-usd layer editor commits into the shared component (`lib/usdUfe/usd-layer-editor/`). See `docs/superpowers/specs/2026-05-19-usd-layer-editor-migration-design.md` and `docs/superpowers/plans/2026-05-19-usd-layer-editor-migration.md` for the full migration design and plan.

## Current state (as of 2026-05-20, after Batch 6)

- `UsdLayerEditorLib` builds as a parallel artifact in the maya-usd build (Tasks 1-4 of the plan complete).
- The bridge from maya-usd's existing `mayaUsdUI` layer editor to `UsdLayerEditorLib` is **deferred** (Tasks 5-6 of the plan were attempted and reverted) because the shared component's API has diverged substantively from maya-usd's mature implementation. Bridging requires the shared API to first be brought up to parity.
- The 161 commits below are the maya-usd-side changes since the standalone component was extracted (~Sep 17, 2024). Each row must be reviewed and ported (or skipped) before the bridge tasks can resume.

## How to use this file

1. Find the first row with status `pending` (top-down).
2. Read the commit (`git show <hash>` from the `maya-usd` submodule) and identify the file(s) it touches.
3. Apply the porting rule (see "Porting rules" below) and update the row's `Status` and `Notes`.
4. Build with `BUILD_NEW_LAYER_EDITOR=ON` (the default in `ecg-maya-usd/build.py`). The shared lib must compile cleanly.
5. Commit the row update alongside the code change.

## Porting rules per commit

- Changed file has a counterpart in `lib/usdUfe/usd-layer-editor/lib/` → diff and port the logic, stripping any Maya dependencies. Mark `ported`.
- Changed file is Maya-specific (`maya*.cpp/h`, `mayaCommandHook`, `mayaSessionState`, `mayaLayerEditorWindow`, `mayaQtUtils`) → no action. Mark `maya-only`.
- Changed file doesn't exist in shared component but is DCC-agnostic → bring the whole file in, add to `lib/CMakeLists.txt`. Mark `ported`.
- Changed file has Maya dependencies that must be removed → add a virtual hook to `SessionState` / `AbstractCommandHook`, inject the Maya behavior from `MayaSessionState` / `MayaCommandHook`. Mark `needs-hook` until the hook lands, then `ported`.
- Pure formatting / clang / whitespace → mark `skip`.
- Already in shared component (diff shows the change is already there) → mark `skip` with note "Already in shared".

## Group labels

- `bug-fix` - small bug fixes, default for un-classified rows
- `layer-contents` - EMSUSD-3189, layerContentsWidget, usdSyntaxHighlighter, pseudoLayer
- `component-creator` - EMSUSD-2997/3016/3020/3654, componentSaveWidget
- `ef-banner` - Edit Forwarding banner + echo
- `filesystem` - EMSUSD-3654 gulrak filesystem update (where not CC-related)
- `maya-only` - touches only Maya-specific files
- `skip` - pure formatting / clang / lint

## Statuses

- `pending` - not yet reviewed
- `ported` - change applied to shared component
- `maya-only` - only Maya wiring files were touched, no action needed
- `skip` - formatting / already in shared / no change needed
- `needs-hook` - change requires adding a DCC injection hook before porting; do not mark `ported` until the hook is in place
- `needs-test` - change is ported but lacks coverage in `layer_editor_test.py`

## Commits

| Commit | Description | Group | Status | Notes |
|--------|-------------|-------|--------|-------|
| eb6c0123 | Revert "Conditionally drop shared sources from mayaUsdUI layer editor" | bug-fix | skip | Migration infrastructure (Tasks 5-6 of plan), not divergent feature work |
| 0708dd9e | Revert "Point maya layer editor wiring at UsdLayerEditorLib headers" | bug-fix | skip | Migration infrastructure (Tasks 5-6 of plan), not divergent feature work |
| 9a13cd9f | Point maya layer editor wiring at UsdLayerEditorLib headers | bug-fix | skip | Migration infrastructure (Tasks 5-6 of plan), not divergent feature work |
| dbfb9b43 | Conditionally drop shared sources from mayaUsdUI layer editor | bug-fix | skip | Migration infrastructure (Tasks 5-6 of plan), not divergent feature work |
| e585a445 | Merge pull request #4609 from Autodesk/bailp/EMSUSD-3181/faster-all-stages | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 7b51144e | EMSUSD-3181 use depth-first | bug-fix | maya-only | Touches only mayaSessionState.cpp (Maya MItDag iteration) — no shared counterpart |
| f18d7545 | EMSUSD-3181 filter instances | bug-fix | maya-only | Touches only mayaSessionState.cpp (Maya DAG instance filtering) — no shared counterpart |
| 5afb128c | EMSUSD-3181 faster all-stages | bug-fix | skip | mayaSessionState.cpp is Maya-only; shared stageSelectorWidget.cpp::selectionChanged already calls allStages() only once via selectedStages() path |
| 4f8b0a66 | Fix save icon refresh | bug-fix | ported | Applied to lib/usdUfe/usd-layer-editor/lib/layerTreeModel.cpp (usd_layerDirtinessChanged emits dataChanged for non-local layers) |
| 78c34506 | Fix comment wording | bug-fix | skip | Comment-only change to maya-usd updateNewLayerButton fallback branch; that branch logic isn't in the shared implementation |
| c710f926 | Fix crash on add layer when root is locked. | bug-fix | ported | Tightened TF_VERIFY in layerEditorCommands.cpp InsertSubLayerPath; updateNewLayerButton in layerEditorWidget.cpp now falls back to root-layer item when nothing is selected, so locked-root state is detected |
| c985a4f9 | EMSUSD-3687 moved comments | bug-fix | needs-port-file | Refactor inside updateTreeContainerBorder/Style; shared layerEditorWidget has no _treeContainer/banner feature yet (depends on the EF banner feature batch) |
| 60cf0f23 | EMSUSD-3687 fix layer-editor-crash | bug-fix | needs-port-file | Crash fix around _treeContainer/EF-banner focus tracking; shared layerEditorWidget has no _treeContainer/_editForwardBanner yet (depends on EF banner feature batch) |
| d29de840 | Merge pull request #4595 from Autodesk/deboisj/ef_echo | ef-banner | pending |  |
| 954b7917 | clang | skip | skip | Formatting/lint commit, no functional change |
| 43f7c18c | update | bug-fix | skip | Formatting/lint commit, no functional change |
| 7f4b4cf3 | want echo | bug-fix | needs-port-file | Adds EF Echo plumbing — depends on EF banner/SessionState extensions not yet in shared (echoEditForwarding option, MayaUsdEditForwardHost is Maya-only); part of EF banner feature batch |
| 026d3073 | EMSUSD-3654 - MayaUsd: Update gulrak filesystem usage * Fix failing test on W... | component-creator | pending |  |
| 78b55a22 | EMSUSD-3654 - MayaUsd: Update gulrak filesystem usage * Remove the download a... | component-creator | pending |  |
| d9fe6df8 | Fix error un pseudo layers with variants & refresh system lock error | bug-fix | needs-port-file | Touches layerContentsWidget.cpp (not in shared yet; layer-contents feature batch) and mayaCommandHook.cpp (Maya-only) |
| 93f72d11 | Merge pull request #4572 from Autodesk/deboisj/LE_EF_banner | ef-banner | pending |  |
| 32d2d8d0 | Update color after feedback from UX | bug-fix | needs-port-file | EF banner background-color tweak; banner not in shared layerEditorWidget yet (EF banner feature batch) |
| 692d93e6 | clang | skip | skip | Formatting/lint commit, no functional change |
| 5359d7f0 | Add banner for EF | ef-banner | pending |  |
| fc9d5ec7 | Merge pull request #4563 from Autodesk/bailp/EMSUSDC-411/refresh-lock-not-und... | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| a20f8d63 | Merge pull request #4566 from Autodesk/kylerasinger/dev/EMSUSD-3219_flatten_l... | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 4f0bcc73 | fix | bug-fix | skip | Formatting/lint commit, no functional change |
| 735d45f6 | EMSUSDC-411 do not disturb redo | bug-fix | maya-only | Touches only mayaCommandHook.cpp/h (Maya MEL/Python undoable flag plumbing) |
| 85c3966a | EMSUSD-3189: [GitHub #4521] 'Hide indices option' for the Display Layer Conte... | layer-contents | pending |  |
| 7b3f6c83 | EMSUSD-3189: [GitHub #4521] 'Hide indices option' for the Display Layer Conte... | layer-contents | pending |  |
| 51cad71d | clang | skip | skip | Formatting/lint commit, no functional change |
| 379c5b14 | Do not show the confirmation dialog if there are no layers to save | bug-fix | ported | Applied to lib/usdUfe/usd-layer-editor/lib/batchSaveLayersUIDelegate.cpp: track atLeastOneLayerToSave / atLeastOneAnonToSave and only show dialog when there are layers to save |
| 79e3197a | linter fix | bug-fix | skip | Formatting/lint commit, no functional change |
| 41293729 | StitchLayer constructor and private member vars, missing consts | bug-fix | skip | Already in shared — StitchLayersCmd in LayerEditorCommands.h already uses constructor-based init with private _layerIdentifiersByStrength/_stage; const-correctness diffs are Maya-side only |
| bd13ab96 | cherrypick | bug-fix | skip | Formatting/lint commit, no functional change |
| 67ae6ead | Merge pull request #4469 from Autodesk/kylerasinger/dev/EMSUSD-3078_flatten_l... | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 336c8781 | Merge pull request #4480 from Autodesk/bailp/EMSUSD-1397/multi-refs-stage-crash | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 7c572bf5 | EMSUSD-1397 fixMaya scene with Maya refs | bug-fix | skip | Touches getChildProxyShape() in stageSelectorWidget.cpp — that helper is Maya-only (MayaUsdProxyShapeBase) and is commented out in the shared version |
| 265edb85 | undo works for script editor | bug-fix | skip | Already in shared — FlattenLayerCmd / kFlattenLayer in LayerEditorCommands.h + UfeCommandHook::flattenLayer; LayerTreeItem::mergeWithSublayers already routes through commandHook()->flattenLayer() |
| 3c22da42 | clang | skip | skip | Formatting/lint commit, no functional change |
| cf187c34 | make sure we dont always popup the save dialog for components | bug-fix | needs-port-file | Depends on MayaUsd::ComponentUtils::shouldDisplayComponentInitialSaveDialog and _componentStageInfos in saveLayersDialog — component-creator feature batch not yet in shared |
| 3d5bc6ff | clang fixes | skip | skip | Formatting/lint commit, no functional change |
| 81a18e43 | Undo functionality | bug-fix | skip | Already in shared — undo is handled via FlattenLayerCmd's BackupLayerBaseCmd; LayerTreeItem::mergeWithSublayers in shared now just calls commandHook()->flattenLayer() (no MayaUsdUndoBlock needed) |
| 049f2d62 | Merge with Sublayers, undo missing, no unit tests. | bug-fix | skip | Already in shared — original inline flatten logic was superseded in shared by FlattenLayerCmd dispatched via commandHook()->flattenLayer() in layerTreeItem.cpp |
| 8aa5a9ce | Add context menu option | bug-fix | skip | Already in shared — LayerTreeItem::mergeWithSublayers() exists in lib/usdUfe/usd-layer-editor/lib/layerTreeItem.{cpp,h}; abstractLayerEditorWindow/mayaLayerEditorWindow/MEL wiring is Maya-only |
| 3f262318 | Merge pull request #4455 from dj-mcg/pr/Remove_Unnecessary_Headers | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 2f2eb930 | Remove (seemingly) unnecessary headers | bug-fix | skip | Already in shared — ghc/filesystem.hpp include is not present in shared layerTreeModel.cpp |
| 7fba398f | Use PXR_NS namespacing macro instead of pxr namespace | bug-fix | needs-port-file | Touches qtUtils.cpp::ValidTfIdentifierValidator — qtUtils.cpp/ValidTfIdentifierValidator not yet in shared; trivial pxr::→PXR_NS:: tweak to apply when the file is ported |
| f31f60b2 | anon layers with locked parents are not saveable. | bug-fix | skip | Already in shared — getLayersToSave in saveLayersDialog.cpp already checks (isLayerLocked \|\| isLayerSystemLocked) for the anonymous-layer parent |
| 85721e66 | EMSUSD-3020 change function nae | component-creator | pending |  |
| c032474d | EMSUSD-3020 reload component | component-creator | pending |  |
| 1ca43946 | EMSUSD-3016 save edits for component | component-creator | pending |  |
| cc852191 | Reapplying lost change on bulk save merge. | bug-fix | skip | Maya-only code path inside SaveLayersDialog::onSaveAll — uses UsdMayaUtil::GetStageByProxyName, proxy-shape rename, MayaUsd::lockLayer; shared onSaveAll does not perform that proxy-shape swap |
| d8c679f0 | Fix component save after initial saving. | bug-fix | needs-port-file | Depends on MayaUsd::ComponentUtils::shouldDisplayComponentInitialSaveDialog and component-stage dialog flow in saveLayersDialog/layerTreeModel — component-creator feature batch |
| b25862b5 | Lint | skip | skip | Formatting/lint commit, no functional change |
| 9998b98f | Add logic to prompt user for saving components when serializing to disk | bug-fix | needs-port-file | Adds componentsOnly mode to SaveLayersDialog + kSaveToMayaSceneFile branch in batchSaveLayersUIDelegate keyed off MayaUsd::ComponentUtils::isAdskUsdComponent — component-creator feature batch |
| e0f8b216 | Address PR feedback and lint | skip | skip | Formatting/lint commit, no functional change |
| 40f455bd | Merge branch 'dev' into kheloua/dev/EMSUSD-2997_bulk_save_components | component-creator | pending |  |
| cbf6ba84 | Merge pull request #4401 from Autodesk/deboisj/change_default_component_folder | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 409ad376 | unused failing linux | bug-fix | needs-port-file | Touches componentSaveDialog.cpp which is not in shared — component-creator feature batch |
| 4bd0774c | Lint the code | skip | skip | Formatting/lint commit, no functional change |
| 7d5ff145 | Merge branch 'dev' into kheloua/dev/EMSUSD-2997_bulk_save_components | component-creator | pending |  |
| f33b4b84 | Remove bad comments | bug-fix | needs-port-file | Comment deletions inside _componentStageInfos / _componentSaveWidgets code paths in saveLayersDialog — component-creator feature batch |
| da8e6c30 | Change to use stringResources strings instead of string formating for compone... | bug-fix | needs-port-file | Component-section msg3 in getDialogMessages and kToSaveTheStageSaveComponents string resource — depends on component-creator feature batch (msg3/component flow not in shared saveLayersDialog) |
| 20baa74a | Remove componentSaveDialog from codebase (using SaveLayerDialog instead) | bug-fix | skip | Removes componentSaveDialog.cpp/h which never existed in shared — nothing to remove on shared side |
| 3e470937 | Add 10px of padding above tree area for componentsavewidget | component-creator | pending |  |
| 492988e4 | Move towards using the SaveLayerDialog instead of the ComponentSaveDialog for... | bug-fix | needs-port-file | Removes ComponentSaveDialog usage from saveStage; uses shouldDisplayComponentInitialSaveDialog/_componentStageInfos/ComponentUtils::isAdskUsdComponent — component-creator feature batch |
| f6f7b68a | clang | skip | skip | Formatting/lint commit, no functional change |
| 0086a2f0 | Remove unused | bug-fix | skip | Removes unused getCurrentSceneDirectory from componentSaveDialog.cpp — file not in shared |
| 831664f8 | Use existing utils | bug-fix | skip | Refactor inside componentSaveDialog.cpp to use MayaUsd::utils::getSceneFolder — file not in shared |
| 77aca3e7 | Make sure we init right | bug-fix | skip | Touches componentSaveDialog.cpp and ComponentSaveDialog code path in layerTreeModel — file/branch not in shared (component-creator feature batch) |
| e79550e3 | Change the default save location for components to the maya scene if it exists | bug-fix | skip | Touches only componentSaveDialog.cpp — file not in shared (component-creator feature batch) |
| 4652debe | UI formating and remove component stages from layer processing in the rest of... | bug-fix | needs-port-file | Touches componentSaveWidget.cpp (not in shared) and _componentStageInfos / haveComponentStages branches in saveLayersDialog — component-creator feature batch |
| f6cdb922 | Add onSave logic for components in bulk save | bug-fix | needs-port-file | Adds _componentSaveWidgets, component onSaveAll loop using ComponentUtils::moveAdskUsdComponent and Maya DAG renames — component-creator feature batch |
| c2e4af5e | Create moveComponent function and make use of it | bug-fix | needs-port-file | Adds ComponentUtils::moveAdskUsdComponent + refactors saveStage to call it via ComponentSaveDialog branch — component-creator feature batch |
| 944ac093 | Move normalization code into setter | bug-fix | skip | Touches only componentSaveWidget.cpp — file not in shared (component-creator feature batch) |
| 71a9640d | Fix CC api usage | component-creator | pending |  |
| 1c7f731d | Add SaveLayerPathRowArea to components section | bug-fix | needs-port-file | Adds _componentStagesWidget + componentScrollArea in saveLayersDialog::buildDialog — component-creator feature batch |
| e3f4ec64 | Add compact mode to widget, label and fix button size | bug-fix | needs-port-file | Adds compact mode to componentSaveWidget + msg3 / haveComponentStages branches in saveLayersDialog — component-creator feature batch |
| e3a08466 | clang | skip | skip | Formatting/lint commit, no functional change |
| 8701f40f | Transfer over session layer content when we save. | bug-fix | needs-port-file | TransferContent for session layer inside ComponentSaveDialog branch in saveStage — component-creator feature batch |
| 5ff15789 | Add basic abstraction of component save widget from dialog and use it in the ... | bug-fix | needs-port-file | Introduces componentSaveWidget.cpp/h (not in shared) and wires into saveLayersDialog — component-creator feature batch |
| b1b118b3 | Merge pull request #4391 from Autodesk/deboisj/block_overwrite | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 2f31607b | Rename to not prefix with TF | bug-fix | needs-port-file | Renames TfValidIdentifierValidator → ValidTfIdentifierValidator in qtUtils.cpp/h (not in shared) — qtUtils file not in shared (component-creator/feature batch dependency) |
| ab4828e9 | Only do it for <= 2017 | bug-fix | skip | Maya-side CMakeLists.txt MSVC QT_NO_FLOAT16_OPERATORS fix; shared CMakeLists has no such MSVC compile-definition block |
| 9f4843a5 | Attempt | bug-fix | skip | Maya-side CMakeLists.txt MSVC QT_NO_FLOAT16_OPERATORS fix + componentSaveDialog cleanup; neither in shared |
| bf969b60 | fix 2023 windows? | bug-fix | skip | QT_NO_FLOAT16_OPERATORS workaround added to componentSaveDialog.cpp — file not in shared |
| 445d7dd6 | Use GHC for filesystem access. | filesystem | skip | Touches only componentSaveDialog.cpp — file not in shared (filesystem/component-creator feature batch dependency) |
| 635f0277 | Move var inside scope | bug-fix | skip | Scope tweak inside componentSaveDialog.cpp — file not in shared |
| 530753c4 | typo / clang | skip | skip | Formatting/lint commit, no functional change |
| d888d13e | move validator to util. | bug-fix | needs-port-file | Moves TfValidIdentifierValidator from componentSaveDialog.cpp into qtUtils.cpp/h — qtUtils.cpp/h and componentSaveDialog not in shared (component-creator feature batch) |
| c43de2b8 | Merge pull request #4386 from Autodesk/deboisj/comp_mgr | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 6953003f | Merge pull request #4389 from Autodesk/kheloua/dev/EMSUSD-2981_CC_crash_add_s... | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 7186d7ae | Misc fixes | bug-fix | needs-port-file | Touches componentSaveDialog.cpp (not in shared) + adds CC branch in layerEditorWidget::updateButtons calling MayaUsd::ComponentUtils::isAdskUsdComponent/getAdskUsdComponentLayersToSave — component-creator feature batch |
| bb186a1f | Linting | bug-fix | skip | Formatting/lint commit, no functional change |
| 393cb6dd | Add condition that checks for a valid LayerTreeItem for disabling of buttons | bug-fix | ported | Applied to lib/usdUfe/usd-layer-editor/lib/layerEditorWidget.cpp::updateNewLayerButton — added defensive `else { disabled = true; }` when item is null |
| df85d8af | lint.. | skip | skip | Formatting/lint commit, no functional change |
| 0ade03dd | block overwrite | bug-fix | skip | Touches only componentSaveDialog.cpp::onSaveStage — file not in shared (component-creator feature batch) |
| 87da45d2 | Move CC code to util | component-creator | needs-port-file | Refactors componentSaveDialog to call MayaUsd::ComponentUtils helpers — component-creator feature batch (file not in shared) |
| f2fa343c | Useless include | bug-fix | skip | Already in shared — layerTreeModel.cpp does not include mayaSessionState.h |
| 3ad2d39b | Merge remote-tracking branch 'public/kheloua/dev/EMSUSD-2913_implement_show_m... | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| d0f3ee01 | Lint again | skip | skip | Formatting/lint commit, no functional change |
| f3e10f4a | Address PR feedback | bug-fix | skip | Patches the Maya-only proxy-shape-rename branch in layerTreeModel::saveStage (uses MayaSessionState::getStageEntry, setNewProxyPath) — that branch is not in shared (component-creator feature batch) |
| 50a6589a | cleanup pass | bug-fix | needs-port-file | Refactors isComponent check + saveAdskUsdComponent call in layerTreeModel::saveStage + shouldDisplayComponentInitialSaveDialog — component-creator feature batch (MayaUsd::ComponentUtils not in shared) |
| 8cb15558 | Add component manager | bug-fix | needs-port-file | Introduces utilComponentCreator + replaces inline Python ComponentDescription check in layerTreeModel with MayaUsd::ComponentUtils::isAdskUsdComponent/saveAdskUsdComponent — component-creator feature batch |
| c53d6850 | Update VE | bug-fix | needs-port-file | Updates the Maya-only MoveComponent Python branch in saveStage (executePythonCommand + setNewProxyPath + MDagModifier rename) — component-creator feature batch |
| 80eecf6b | Support component save | bug-fix | needs-port-file | Embeds Python MayaComponentManager.SaveComponent call inside layerTreeModel::saveStage + utilSerialization changes — component-creator feature batch |
| 02f044db | Clang format again | skip | skip | Formatting/lint commit, no functional change |
| 68d7e07e | Add constants for show more and less strings | bug-fix | skip | Touches only componentSaveDialog.cpp — file not in shared (component-creator feature batch) |
| 9f9c2426 | Remove json parsing logic and add no data message | bug-fix | skip | Touches only componentSaveDialog.cpp/h — file not in shared (component-creator feature batch) |
| 59eada75 | Remove unnecessary include | bug-fix | skip | Touches only componentSaveDialog.cpp — file not in shared (component-creator feature batch) |
| 0a832995 | Linting issues | bug-fix | skip | Formatting/lint commit, no functional change |
| 060811c7 | Fix error related to bad stage entry state | bug-fix | needs-port-file | Reorders proxy-shape rename/setNewProxyPath/MayaSessionState::getStageEntry calls inside the Maya-only MoveComponent branch in saveStage — component-creator feature batch |
| 489a611c | Adjust constructor call to pass proxy shape path | bug-fix | needs-port-file | Passes proxyShapePath to ComponentSaveDialog ctor in the saveStage CC branch — component-creator feature batch (dialog/branch not in shared) |
| 4aa8c952 | Add Show More functionality to the component save dialog | bug-fix | skip | Touches only componentSaveDialog.cpp/h — file not in shared (component-creator feature batch) |
| 8adf4920 | Remove dependency to utilSerialization.h and bring in code to get workspace s... | bug-fix | skip | Touches only componentSaveDialog.cpp — file not in shared (component-creator feature batch) |
| 6437fb66 | Lint | skip | skip | Formatting/lint commit, no functional change |
| fa7efa32 | Change to use #include <ghc/filesystem.hpp> | filesystem | skip | std::filesystem→ghc::filesystem swap inside isPathInside helper used only by shouldDisplayComponentInitialSaveDialog — not in shared (component-creator feature batch dependency) |
| e2eff3ea | Address missed PR feedback | bug-fix | needs-port-file | Tweaks MStatus tracking inside shouldDisplayComponentInitialSaveDialog — component-creator feature batch (function not in shared) |
| 73f9c497 | Address PR feedback again | bug-fix | needs-port-file | Adjusts isStageAComponent error/warning branches inside shouldDisplayComponentInitialSaveDialog — component-creator feature batch |
| c77d22f8 | Add import failure check based on PR feedback | bug-fix | needs-port-file | Wraps AdskUsdComponentCreator import in try/except (-1 sentinel) inside shouldDisplayComponentInitialSaveDialog — component-creator feature batch |
| f562376e | Address PR feedback part 2 | bug-fix | needs-port-file | Lifts shouldDisplayComponentInitialSaveDialog out of saveStage into file-scope helper and renames Python helpers — component-creator feature batch |
| 819b110f | Linting issue | bug-fix | skip | Formatting/lint commit, no functional change |
| 1a5b496a | Fix address and address part of the feedback | bug-fix | needs-port-file | Indents Python helpers + adds isStageAComponent==0 early-return inside saveStage's CC branch — component-creator feature batch |
| 6a29d115 | Local linter misbehaving | bug-fix | skip | Formatting/lint commit, no functional change |
| 0d3da146 | Replace manual rename emit signal with rename command of proxy object | bug-fix | needs-port-file | Replaces SessionState::renameCurrentStageEntry call inside saveStage with MDagModifier rename of proxy shape — component-creator/Maya feature batch (CC branch not in shared) |
| 4219f936 | Apply linting | bug-fix | skip | Formatting/lint commit, no functional change |
| 3f256b6f | Add code to change the stage entry display name and update the stage selector... | bug-fix | needs-port-file | Adds SessionState::renameCurrentStageEntry helper + call from CC branch in saveStage — component-creator feature batch (CC branch not in shared); helper later removed in 0d3da146 |
| 1f367bcb | Add basic CC save dialog | component-creator | pending |  |
| 80a63d77 | EMSUSD-2841 - Performance is really slow when updating prims in the viewport ... | layer-contents | pending |  |
| 738aa5a8 | EMSUSD-2839 - Update styling in our layer content view * Update usdSyntaxConf... | bug-fix | needs-port-file | Touches layerContentsWidget.cpp/h and usdSyntaxConfig.json — layer-contents feature batch (file not in shared) |
| 18d52fda | Linter again | bug-fix | skip | Formatting/lint commit, no functional change |
| 50f4a23f | Linter errors | bug-fix | skip | Formatting/lint commit, no functional change |
| 7383bc12 | Fix focus to Confirm/OK instead of cancel (EMSUSD-2328) | bug-fix | skip | Already in shared — warningDialogs.cpp::confirmDialog_internal already uses setDefaultButton(QMessageBox::Ok) + setFocus() on the Ok button |
| bbef9502 | Add Expression Var support to Layer Editor | bug-fix | skip | Already in shared — layerTreeItem.{cpp,h} already carry _stage member, expression-var resolution via SdfVariableExpression, and layerTreeModel passes _sessionState->stage() to LayerTreeItem ctor |
| 39b722a9 | EMSUSD-2655: Show Layer Data Window * Another attempt to fix the error only t... | bug-fix | needs-port-file | Touches usdSyntaxHighlighter.cpp — layer-contents feature batch (file not in shared) |
| bd3b808c | EMSUSD-2655: Show Layer Data Window * Linux/Mac build fix | bug-fix | needs-port-file | Touches usdSyntaxHighlighter.cpp — layer-contents feature batch (file not in shared) |
| 52ad9a67 | EMSUSD-2655: Show Layer Data Window * Adjusted message displayed in Window wh... | bug-fix | needs-port-file | Tweaks kDisplayLayerContentsEmpty string — string not yet in shared stringResources.h (layer-contents feature batch) |
| f22aebe5 | EMSUSD-2655: Show Layer Data Window * Code review - adding extra code comment. | bug-fix | needs-port-file | Comment-only addition to layerContentsWidget.cpp — layer-contents feature batch (file not in shared) |
| 8b00f113 | EMSUSD-2655: Show Layer Data Window * Added new layer contents widget (to Lay... | layer-contents | pending |  |
| defbebe9 | Update layerTreeViewStyle.h | bug-fix | skip | Fixes QT_DISABLE_DEPRECATED_BEFOR typo introduced by 3150544f in layerTreeViewStyle.h; shared port (3150544f) already applies the corrected spelling, so no extra change needed |
| 3150544f | Fixes for building with QT_DISABLE_DEPRECATED_BEFORE | bug-fix | ported | Added `QT_DISABLE_DEPRECATED_BEFORE \|\|` to the Qt6 #if checks in shared layerTreeItem.cpp, layerTreeItemDelegate.cpp, and layerTreeViewStyle.h (importDialog files are not in shared) |
| 9be70aed | EMSUSD-2232 - Disables cache rebuild due to layer editor | bug-fix | skip | Maya-only rebuildCache plumbing — shared stageSelectorWidget already has the Maya getProxyShape/getChildProxyShape paths commented out; Utils.cpp/h are Maya-only |
| 80cb4182 | Port layers editor fixes | bug-fix | skip | Already in shared — layerEditorCommands.cpp::backupLayer uses broader `_cmdId != kDiscardEdit` predicate (covers kClearLayer); stageSelectorWidget::updateFromSessionState already has _pinStageSelection/currentEntryQVariant logic; warningDialogs already sets focus on Ok. mayaSessionState.cpp is Maya-only |
| b4ec9672 | EMSUSD-2301 - Layer Editor Won't Launch * The LayerEditorWindowCommand proxyS... | bug-fix | ported | Applied to lib/usdUfe/usd-layer-editor/lib/layerEditorWidget.cpp: getSelectedLayers now uses _treeView->getSelectedLayerItems(); selectLayers sets currentIndex on the first selected layer and applies the selection in a single ClearAndSelect|Rows pass. Maya files (layerEditorWindowCommand, abstractLayerEditorWindow, mayaCommandHook, mayaLayerEditorWindow, Readme) are Maya-only |
| 4f945f1d | Add _cachedModelState nullptr check | bug-fix | pending |  |
| 49753ba9 | Add documentation for notification and fix gcc compilation issue | bug-fix | pending |  |
| 93b2eb24 | Remove _cachedModelState reset call | bug-fix | pending |  |
| 41282ecb | Change elide mode to middle | bug-fix | pending |  |
| c5a79c20 | Update pin layer tooltip | bug-fix | pending |  |
| ecbc1dc8 | Keep cache object state in memory throughout session | bug-fix | pending |  |
| 48fb043e | Remove layerEditorCommands and layerEditorWidgetManager, move implementation ... | bug-fix | pending |  |
| fc11cde3 | Add space for linter | bug-fix | pending |  |
| faa0f9ef | Add include guards | bug-fix | pending |  |
| b37d31ee | Reorder includes | bug-fix | pending |  |
| 1ed52bce | Address PR feedback 3 | bug-fix | pending |  |
| 45745e01 | Address PR feedback 2 | bug-fix | pending |  |
| 0755cdbb | Fix linux compilation issue | bug-fix | pending |  |
| 1ed7bf6c | Add include vector | bug-fix | pending |  |
| e9aa2a49 | Format with clang | skip | skip | Formatting/lint commit, no functional change |
| b73dfe21 | Address PR feedback | bug-fix | pending |  |
| 458ff6bb | Add "mayaUsdSetSelectedLayers" and "mayaUsdGetSelectedLayers" layer editor hooks | bug-fix | pending |  |
| f17afccb | EMSUSD-1722 fix node origin detection | bug-fix | pending |  |
| 9d770c0a | Merge pull request #3915 from Autodesk/bailp/EMSUSD-1619/remove-anon-layers | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 5d5c4250 | Merge pull request #3911 from Autodesk/bailp/EMSUSD-1510/save-layer-button-name | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
