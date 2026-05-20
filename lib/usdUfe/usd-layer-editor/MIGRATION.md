# USD Layer Editor Migration Tracking

Resume point for porting maya-usd layer editor commits into the shared component (`lib/usdUfe/usd-layer-editor/`). See `docs/superpowers/specs/2026-05-19-usd-layer-editor-migration-design.md` and `docs/superpowers/plans/2026-05-19-usd-layer-editor-migration.md` for the full migration design and plan.

## Current state (as of 2026-05-20, after component-creator feature batch)

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
| c985a4f9 | EMSUSD-3687 moved comments | bug-fix | ported | Comment/style cleanup inside updateTreeContainerBorder/Style: shared lib/usdUfe/usd-layer-editor/lib/layerEditorWidget.cpp now ships the post-cleanup version (banner comment in updateTreeContainerStyle, margin adjust grouped with style) |
| 60cf0f23 | EMSUSD-3687 fix layer-editor-crash | bug-fix | ported | Crash fix landed together with EF banner port: _treeContainer is QPointer, updateTreeContainerBorder/Style check it, destructor revokes TfNotice + disconnects focus signal |
| d29de840 | Merge pull request #4595 from Autodesk/deboisj/ef_echo | ef-banner | ported | Merge PR — individual commits 7f4b4cf3 (want echo) ported via SessionState virtuals (echoEditForwarding / setEchoEditForwarding / supportsEditForwarding); Maya option-var storage stays in MayaSessionState. Maya-only MayaUsdEditForwardHost echo wiring is maya-only. |
| 954b7917 | clang | skip | skip | Formatting/lint commit, no functional change |
| 43f7c18c | update | bug-fix | skip | Formatting/lint commit, no functional change |
| 7f4b4cf3 | want echo | bug-fix | ported | Added echoEditForwarding() / setEchoEditForwarding() / supportsEditForwarding() virtuals on shared SessionState (default no-op / false); kEchoEditForwarding string resource; shared layerEditorWidget.setupDefaultMenu adds the Echo menu item only when supportsEditForwarding() returns true. MayaSessionState wires the optionVar; MayaUsdEditForwardHost Echo/WantsEcho are maya-only. |
| 026d3073 | EMSUSD-3654 - MayaUsd: Update gulrak filesystem usage * Fix failing test on W... | component-creator | skip | Touches Maya-only componentSaveDialog.cpp (removed in 20baa74a) and Maya-only filesystem helpers. Shared componentSaveWidget.cpp uses std::filesystem; no gulrak dependency to update on the shared side |
| 78b55a22 | EMSUSD-3654 - MayaUsd: Update gulrak filesystem usage * Remove the download a... | component-creator | skip | Touches Maya-only componentSaveDialog.cpp (removed in 20baa74a). Shared componentSaveWidget.cpp uses std::filesystem; no gulrak dependency to update on the shared side |
| d9fe6df8 | Fix error un pseudo layers with variants & refresh system lock error | bug-fix | ported | The layerContentsWidget.cpp variant-path fix (IsPrimPath gating around SdfCreatePrimInLayer) ships with the maya-usd source that was ported into lib/usdUfe/usd-layer-editor/lib/layerContentsWidget.cpp during the layer-contents feature batch. mayaCommandHook.cpp side is Maya-only. |
| 93f72d11 | Merge pull request #4572 from Autodesk/deboisj/LE_EF_banner | ef-banner | ported | Merge PR — individual commits 5359d7f0 (banner), 692d93e6 (clang), 32d2d8d0 (UX color), 6d805cce1 (comment, Maya-only), d739150cd (clang) ported via EF banner feature batch |
| 32d2d8d0 | Update color after feedback from UX | bug-fix | ported | Shared lib/usdUfe/usd-layer-editor/lib/layerEditorWidget.cpp sets _editForwardBanner background to rgb(55, 55, 55) per UX feedback |
| 692d93e6 | clang | skip | skip | Formatting/lint commit, no functional change |
| 5359d7f0 | Add banner for EF | ef-banner | ported | Ported the EF banner UI into shared lib/usdUfe/usd-layer-editor/lib/layerEditorWidget.cpp/h: _treeContainer QFrame wraps banner + tree view, _editForwardBanner QLabel, updateTreeContainerStyle/Border focus highlighting, TfNotice listener on root-layer CustomLayerData, updateEditForwardBanner gated on SessionState::hasEditForwarding() (DCC injection). LayerTreeView style now sets outline/border to none so the container provides the visual border. No Maya symbols (WANT_ADSK_USD_EDIT_FORWARD_BUILD / MayaUsdEditForwardHost / AdskUsdEditForward::StageRuleProvider) enter shared — those stay on the maya side via MayaSessionState overrides. |
| fc9d5ec7 | Merge pull request #4563 from Autodesk/bailp/EMSUSDC-411/refresh-lock-not-und... | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| a20f8d63 | Merge pull request #4566 from Autodesk/kylerasinger/dev/EMSUSD-3219_flatten_l... | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 4f0bcc73 | fix | bug-fix | skip | Formatting/lint commit, no functional change |
| 735d45f6 | EMSUSDC-411 do not disturb redo | bug-fix | maya-only | Touches only mayaCommandHook.cpp/h (Maya MEL/Python undoable flag plumbing) |
| 85c3966a | EMSUSD-3189: [GitHub #4521] 'Hide indices option' for the Display Layer Conte... | layer-contents | ported | PXR_VERSION < 2508 / < 2403 compat in layerContentsWidget.cpp (LCWBASEFILEFORMAT macro + std::optional vs boost::optional) is included in the ported lib/usdUfe/usd-layer-editor/lib/layerContentsWidget.cpp |
| 7b3f6c83 | EMSUSD-3189: [GitHub #4521] 'Hide indices option' for the Display Layer Conte... | layer-contents | ported | Main commit. Ported the pseudoLayer sdffilter logic, Expand All Values menu, and three SessionState display-layer-* virtuals (displayLayerContents/ExpandAllValues/HideIndices) into shared lib. Maya optionVar storage stays in MayaSessionState (DCC injection). HideIndices is a no-op shared virtual; the feature title is misleading — actual implementation is pseudoLayer + Expand All Values. Array/timeSamples size limits use the default ReportParams values (8, 8) in shared. |
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
| cf187c34 | make sure we dont always popup the save dialog for components | bug-fix | ported | Component-creator feature batch: shared SaveLayersDialog now branches on SessionState::shouldDisplayComponentInitialSaveDialog() and populates _componentStageInfos. DCC-agnostic part is in shared; Maya override of the SessionState virtual will wire the real predicate when the bridge tasks resume |
| 3d5bc6ff | clang fixes | skip | skip | Formatting/lint commit, no functional change |
| 81a18e43 | Undo functionality | bug-fix | skip | Already in shared — undo is handled via FlattenLayerCmd's BackupLayerBaseCmd; LayerTreeItem::mergeWithSublayers in shared now just calls commandHook()->flattenLayer() (no MayaUsdUndoBlock needed) |
| 049f2d62 | Merge with Sublayers, undo missing, no unit tests. | bug-fix | skip | Already in shared — original inline flatten logic was superseded in shared by FlattenLayerCmd dispatched via commandHook()->flattenLayer() in layerTreeItem.cpp |
| 8aa5a9ce | Add context menu option | bug-fix | skip | Already in shared — LayerTreeItem::mergeWithSublayers() exists in lib/usdUfe/usd-layer-editor/lib/layerTreeItem.{cpp,h}; abstractLayerEditorWindow/mayaLayerEditorWindow/MEL wiring is Maya-only |
| 3f262318 | Merge pull request #4455 from dj-mcg/pr/Remove_Unnecessary_Headers | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 2f2eb930 | Remove (seemingly) unnecessary headers | bug-fix | skip | Already in shared — ghc/filesystem.hpp include is not present in shared layerTreeModel.cpp |
| 7fba398f | Use PXR_NS namespacing macro instead of pxr namespace | bug-fix | ported | ValidTfIdentifierValidator added to shared lib/usdUfe/usd-layer-editor/lib/utilQT.{cpp,h} as part of the component-creator feature batch, using PXR_NS::TfMakeValidIdentifier |
| f31f60b2 | anon layers with locked parents are not saveable. | bug-fix | skip | Already in shared — getLayersToSave in saveLayersDialog.cpp already checks (isLayerLocked \|\| isLayerSystemLocked) for the anonymous-layer parent |
| 85721e66 | EMSUSD-3020 change function nae | component-creator | ported | Component-creator feature batch: shared LayerTreeModel::reloadComponent added with the corresponding AbstractCommandHook::reloadComponent / SessionState::isUnsavedComponent virtuals. The exact function name will be matched by the Maya override |
| c032474d | EMSUSD-3020 reload component | component-creator | ported | Component-creator feature batch: shared LayerTreeModel::reloadComponent routes through AbstractCommandHook::reloadComponent. Maya override will wire ComponentUtils::reloadAdskUsdComponent when the bridge tasks resume |
| 1ca43946 | EMSUSD-3016 save edits for component | component-creator | ported | Component-creator feature batch: shared LayerTreeModel::saveStage branches on SessionState::isStageAComponent and routes through AbstractCommandHook::saveComponent for the actual edits save |
| cc852191 | Reapplying lost change on bulk save merge. | bug-fix | skip | Maya-only code path inside SaveLayersDialog::onSaveAll — uses UsdMayaUtil::GetStageByProxyName, proxy-shape rename, MayaUsd::lockLayer; shared onSaveAll does not perform that proxy-shape swap |
| d8c679f0 | Fix component save after initial saving. | bug-fix | ported | Component-creator feature batch: shared SaveLayersDialog and LayerTreeModel::saveStage now branch on SessionState::shouldDisplayComponentInitialSaveDialog() and route the save through AbstractCommandHook::saveComponent(). Maya overrides will wire the real predicates when the bridge tasks resume |
| b25862b5 | Lint | skip | skip | Formatting/lint commit, no functional change |
| 9998b98f | Add logic to prompt user for saving components when serializing to disk | bug-fix | ported | Component-creator feature batch: SaveLayersDialog bulk-save ctor in shared now accepts a componentsOnly parameter (default false) and uses SessionState::shouldDisplayComponentInitialSaveDialog to identify component stages. The kSaveToMayaSceneFile branch in batchSaveLayersUIDelegate is Maya-only and will be added on the Maya side when the bridge tasks resume |
| e0f8b216 | Address PR feedback and lint | skip | skip | Formatting/lint commit, no functional change |
| 40f455bd | Merge branch 'dev' into kheloua/dev/EMSUSD-2997_bulk_save_components | component-creator | skip | Merge commit — content tracked in the individual commits being merged |
| cbf6ba84 | Merge pull request #4401 from Autodesk/deboisj/change_default_component_folder | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 409ad376 | unused failing linux | bug-fix | skip | Touches componentSaveDialog.cpp which was removed in 20baa74a and never existed in shared. The shared componentSaveWidget.cpp ported in this batch has no unused-include issues |
| 4bd0774c | Lint the code | skip | skip | Formatting/lint commit, no functional change |
| 7d5ff145 | Merge branch 'dev' into kheloua/dev/EMSUSD-2997_bulk_save_components | component-creator | skip | Merge commit — content tracked in the individual commits being merged |
| f33b4b84 | Remove bad comments | bug-fix | ported | Component-creator feature batch: comments in shared SaveLayersDialog's CC code paths are clean (no stale comment to remove on the shared side) |
| da8e6c30 | Change to use stringResources strings instead of string formating for compone... | bug-fix | ported | Component-creator feature batch: kToSaveTheStageSaveComponents/kToExportTheStageSaveComponents string resources added to lib/usdUfe/usd-layer-editor/lib/stringResources.h; msg3 plumbed through getDialogMessages and buildDialog |
| 20baa74a | Remove componentSaveDialog from codebase (using SaveLayerDialog instead) | bug-fix | skip | Removes componentSaveDialog.cpp/h which never existed in shared — nothing to remove on shared side |
| 3e470937 | Add 10px of padding above tree area for componentsavewidget | component-creator | ported | Component-creator feature batch: shared lib/usdUfe/usd-layer-editor/lib/componentSaveWidget.cpp uses DPIScale(10) tree margins (top) matching the maya-usd source |
| 492988e4 | Move towards using the SaveLayerDialog instead of the ComponentSaveDialog for... | bug-fix | ported | Component-creator feature batch: shared SaveLayersDialog now hosts the component-save UI via ComponentSaveWidget, _componentStageInfos populated from SessionState::shouldDisplayComponentInitialSaveDialog. LayerTreeModel::saveStage uses the dialog flow rather than a separate ComponentSaveDialog |
| f6f7b68a | clang | skip | skip | Formatting/lint commit, no functional change |
| 0086a2f0 | Remove unused | bug-fix | skip | Removes unused getCurrentSceneDirectory from componentSaveDialog.cpp — file not in shared |
| 831664f8 | Use existing utils | bug-fix | skip | Refactor inside componentSaveDialog.cpp to use MayaUsd::utils::getSceneFolder — file not in shared |
| 77aca3e7 | Make sure we init right | bug-fix | skip | Touches componentSaveDialog.cpp and ComponentSaveDialog code path in layerTreeModel — file/branch not in shared (component-creator feature batch) |
| e79550e3 | Change the default save location for components to the maya scene if it exists | bug-fix | skip | Touches only componentSaveDialog.cpp — file not in shared (component-creator feature batch) |
| 4652debe | UI formating and remove component stages from layer processing in the rest of... | bug-fix | ported | Component-creator feature batch: shared componentSaveWidget.cpp + haveComponentStages branches in saveLayersDialog ship the UI formatting and the skip-component-stages-in-layer-processing logic |
| f6cdb922 | Add onSave logic for components in bulk save | bug-fix | ported | Component-creator feature batch: shared SaveLayersDialog::onSaveAll loops _componentSaveWidgets and routes through SessionState::moveComponent + AbstractCommandHook::renameProxyShape. Maya overrides will wire ComponentUtils::moveAdskUsdComponent and MDagModifier renames when the bridge tasks resume |
| c2e4af5e | Create moveComponent function and make use of it | bug-fix | ported | Component-creator feature batch: SessionState::moveComponent virtual added (default empty). Shared SaveLayersDialog::onSaveAll uses it instead of inline ComponentUtils::moveAdskUsdComponent |
| 944ac093 | Move normalization code into setter | bug-fix | skip | Touches only componentSaveWidget.cpp — file not in shared (component-creator feature batch) |
| 71a9640d | Fix CC api usage | component-creator | ported | Component-creator feature batch: shared componentSaveWidget + saveLayersDialog use the SessionState::previewComponentSave / SessionState::moveComponent / SessionState::sceneFolder virtuals. The Maya overrides will call the CC API correctly |
| 1c7f731d | Add SaveLayerPathRowArea to components section | bug-fix | ported | Component-creator feature batch: shared SaveLayersDialog::buildDialog now creates _componentStagesWidget + componentScrollArea (SaveLayerPathRowArea) for the component section |
| e3f4ec64 | Add compact mode to widget, label and fix button size | bug-fix | ported | Component-creator feature batch: ComponentSaveWidget::setCompactMode in shared lib/usdUfe/usd-layer-editor/lib/componentSaveWidget.{cpp,h}; saveLayersDialog wires compact mode on all but the first component widget; msg3 plumbed through |
| e3a08466 | clang | skip | skip | Formatting/lint commit, no functional change |
| 8701f40f | Transfer over session layer content when we save. | bug-fix | maya-only | Session-layer TransferContent inside the proxy-shape rename flow is a Maya-specific concern (it relies on UsdMayaUtil::GetStageByProxyName around the proxy-shape swap). Shared SaveLayersDialog::onSaveAll calls AbstractCommandHook::renameProxyShape; the Maya override will encapsulate TransferContent + the proxy-shape swap |
| 5ff15789 | Add basic abstraction of component save widget from dialog and use it in the ... | bug-fix | ported | Component-creator feature batch: lib/usdUfe/usd-layer-editor/lib/componentSaveWidget.{cpp,h} added to shared and wired into SaveLayersDialog via _componentSaveWidgets |
| b1b118b3 | Merge pull request #4391 from Autodesk/deboisj/block_overwrite | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 2f31607b | Rename to not prefix with TF | bug-fix | ported | Component-creator feature batch: shared utilQT.{cpp,h} ships the ValidTfIdentifierValidator (un-TF-prefixed) name already |
| ab4828e9 | Only do it for <= 2017 | bug-fix | skip | Maya-side CMakeLists.txt MSVC QT_NO_FLOAT16_OPERATORS fix; shared CMakeLists has no such MSVC compile-definition block |
| 9f4843a5 | Attempt | bug-fix | skip | Maya-side CMakeLists.txt MSVC QT_NO_FLOAT16_OPERATORS fix + componentSaveDialog cleanup; neither in shared |
| bf969b60 | fix 2023 windows? | bug-fix | skip | QT_NO_FLOAT16_OPERATORS workaround added to componentSaveDialog.cpp — file not in shared |
| 445d7dd6 | Use GHC for filesystem access. | filesystem | skip | Touches only componentSaveDialog.cpp — file not in shared (filesystem/component-creator feature batch dependency) |
| 635f0277 | Move var inside scope | bug-fix | skip | Scope tweak inside componentSaveDialog.cpp — file not in shared |
| 530753c4 | typo / clang | skip | skip | Formatting/lint commit, no functional change |
| d888d13e | move validator to util. | bug-fix | ported | Component-creator feature batch: ValidTfIdentifierValidator lives in shared lib/usdUfe/usd-layer-editor/lib/utilQT.{cpp,h} from the start of the port (no intermediate componentSaveDialog location in shared) |
| c43de2b8 | Merge pull request #4386 from Autodesk/deboisj/comp_mgr | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 6953003f | Merge pull request #4389 from Autodesk/kheloua/dev/EMSUSD-2981_CC_crash_add_s... | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 7186d7ae | Misc fixes | bug-fix | ported | Component-creator feature batch: shared layerEditorWidget::updateButtons now has the CC branch routed through SessionState::isStageAComponent + SessionState::getComponentLayersToSave virtuals. Maya overrides will wire ComponentUtils::isAdskUsdComponent/getAdskUsdComponentLayersToSave when the bridge tasks resume |
| bb186a1f | Linting | bug-fix | skip | Formatting/lint commit, no functional change |
| 393cb6dd | Add condition that checks for a valid LayerTreeItem for disabling of buttons | bug-fix | ported | Applied to lib/usdUfe/usd-layer-editor/lib/layerEditorWidget.cpp::updateNewLayerButton — added defensive `else { disabled = true; }` when item is null |
| df85d8af | lint.. | skip | skip | Formatting/lint commit, no functional change |
| 0ade03dd | block overwrite | bug-fix | skip | Touches only componentSaveDialog.cpp::onSaveStage — file not in shared (component-creator feature batch) |
| 87da45d2 | Move CC code to util | component-creator | ported | Component-creator feature batch: CC code in shared lives behind SessionState/AbstractCommandHook virtuals. Maya overrides will call ComponentUtils helpers when the bridge tasks resume |
| f2fa343c | Useless include | bug-fix | skip | Already in shared — layerTreeModel.cpp does not include mayaSessionState.h |
| 3ad2d39b | Merge remote-tracking branch 'public/kheloua/dev/EMSUSD-2913_implement_show_m... | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| d0f3ee01 | Lint again | skip | skip | Formatting/lint commit, no functional change |
| f3e10f4a | Address PR feedback | bug-fix | skip | Patches the Maya-only proxy-shape-rename branch in layerTreeModel::saveStage (uses MayaSessionState::getStageEntry, setNewProxyPath) — that branch is not in shared (component-creator feature batch) |
| 50a6589a | cleanup pass | bug-fix | ported | Component-creator feature batch: layerTreeModel::saveStage in shared now branches on SessionState::isStageAComponent + SessionState::shouldDisplayComponentInitialSaveDialog and routes the actual save through AbstractCommandHook::saveComponent |
| 8cb15558 | Add component manager | bug-fix | ported | Component-creator feature batch: utilComponentCreator stays on the Maya side; shared LayerTreeModel uses SessionState::isStageAComponent + AbstractCommandHook::saveComponent virtuals. Maya overrides will wire ComponentUtils::isAdskUsdComponent/saveAdskUsdComponent when the bridge tasks resume |
| c53d6850 | Update VE | bug-fix | ported | Component-creator feature batch: MoveComponent/proxy-rename flow lives behind SessionState::moveComponent + AbstractCommandHook::renameProxyShape virtuals in shared. The Maya override will wire the Python MoveComponent call + setNewProxyPath + MDagModifier rename when the bridge tasks resume |
| 80eecf6b | Support component save | bug-fix | ported | Component-creator feature batch: shared LayerTreeModel::saveStage calls AbstractCommandHook::saveComponent(stage, dccObjectPath) for component stages. Maya override will embed the MayaComponentManager.SaveComponent Python call when the bridge tasks resume |
| 02f044db | Clang format again | skip | skip | Formatting/lint commit, no functional change |
| 68d7e07e | Add constants for show more and less strings | bug-fix | skip | Touches only componentSaveDialog.cpp — file not in shared (component-creator feature batch) |
| 9f9c2426 | Remove json parsing logic and add no data message | bug-fix | skip | Touches only componentSaveDialog.cpp/h — file not in shared (component-creator feature batch) |
| 59eada75 | Remove unnecessary include | bug-fix | skip | Touches only componentSaveDialog.cpp — file not in shared (component-creator feature batch) |
| 0a832995 | Linting issues | bug-fix | skip | Formatting/lint commit, no functional change |
| 060811c7 | Fix error related to bad stage entry state | bug-fix | maya-only | Ordering of MDagModifier rename + setNewProxyPath + MayaSessionState::getStageEntry inside the MoveComponent branch is a Maya-side concern. Shared SaveLayersDialog::onSaveAll routes the rename through AbstractCommandHook::renameProxyShape and reselects the stage by name; the exact ordering lives inside the Maya override |
| 489a611c | Adjust constructor call to pass proxy shape path | bug-fix | ported | Component-creator feature batch: shared SaveLayersDialog single-stage ctor passes _sessionState->stageEntry()._dccObjectPath into the StageSavingInfo it pushes into _componentStageInfos, which is then handed to ComponentSaveWidget |
| 4aa8c952 | Add Show More functionality to the component save dialog | bug-fix | skip | Touches only componentSaveDialog.cpp/h — file not in shared (component-creator feature batch) |
| 8adf4920 | Remove dependency to utilSerialization.h and bring in code to get workspace s... | bug-fix | skip | Touches only componentSaveDialog.cpp — file not in shared (component-creator feature batch) |
| 6437fb66 | Lint | skip | skip | Formatting/lint commit, no functional change |
| fa7efa32 | Change to use #include <ghc/filesystem.hpp> | filesystem | skip | std::filesystem→ghc::filesystem swap inside isPathInside helper used only by shouldDisplayComponentInitialSaveDialog — not in shared (component-creator feature batch dependency) |
| e2eff3ea | Address missed PR feedback | bug-fix | maya-only | MStatus tracking is Maya-internal to shouldDisplayComponentInitialSaveDialog's implementation. Shared SessionState exposes the predicate as a virtual; the implementation details live in the Maya override |
| 73f9c497 | Address PR feedback again | bug-fix | maya-only | isStageAComponent error/warning branches live inside the Maya implementation of shouldDisplayComponentInitialSaveDialog. Shared SessionState only exposes the bool virtual |
| c77d22f8 | Add import failure check based on PR feedback | bug-fix | maya-only | AdskUsdComponentCreator python import try/except lives inside the Maya implementation of shouldDisplayComponentInitialSaveDialog. Shared SessionState only exposes the bool virtual |
| f562376e | Address PR feedback part 2 | bug-fix | maya-only | Refactor of file-scope helper lives inside the Maya implementation. Shared SessionState exposes only the bool virtual |
| 819b110f | Linting issue | bug-fix | skip | Formatting/lint commit, no functional change |
| 1a5b496a | Fix address and address part of the feedback | bug-fix | maya-only | Indentation + isStageAComponent==0 early-return live inside the Maya implementation; shared LayerTreeModel just calls AbstractCommandHook::saveComponent unconditionally for component stages |
| 6a29d115 | Local linter misbehaving | bug-fix | skip | Formatting/lint commit, no functional change |
| 0d3da146 | Replace manual rename emit signal with rename command of proxy object | bug-fix | ported | Component-creator feature batch: shared SaveLayersDialog::onSaveAll calls AbstractCommandHook::renameProxyShape() for the component rename. Maya override will use MDagModifier; no SessionState::renameCurrentStageEntry helper is needed on the shared side |
| 4219f936 | Apply linting | bug-fix | skip | Formatting/lint commit, no functional change |
| 3f256b6f | Add code to change the stage entry display name and update the stage selector... | bug-fix | skip | The renameCurrentStageEntry helper was later removed in 0d3da146 (now uses MDagModifier rename → routed through AbstractCommandHook::renameProxyShape in shared). No helper needed in shared |
| 1f367bcb | Add basic CC save dialog | component-creator | ported | Component-creator feature batch: shared lib/usdUfe/usd-layer-editor/lib/componentSaveWidget.{cpp,h} ported with Maya symbols stripped. The save flow uses the existing SaveLayersDialog rather than a separate ComponentSaveDialog (per maya-usd commit 20baa74a) |
| 80a63d77 | EMSUSD-2841 - Performance is really slow when updating prims in the viewport ... | layer-contents | ported | Lazy-update timer (QBasicTimer + onLazyUpdateLayerContents + onSplitterMoved) is in the ported lib/usdUfe/usd-layer-editor/lib/layerEditorWidget.cpp; resource icon path tweaks are maya-only |
| 738aa5a8 | EMSUSD-2839 - Update styling in our layer content view * Update usdSyntaxConf... | bug-fix | ported | "Courier New" font + new color scheme are in the ported layerContentsWidget.cpp and usdSyntaxConfig.json (both copied verbatim from maya-usd) |
| 18d52fda | Linter again | bug-fix | skip | Formatting/lint commit, no functional change |
| 50f4a23f | Linter errors | bug-fix | skip | Formatting/lint commit, no functional change |
| 7383bc12 | Fix focus to Confirm/OK instead of cancel (EMSUSD-2328) | bug-fix | skip | Already in shared — warningDialogs.cpp::confirmDialog_internal already uses setDefaultButton(QMessageBox::Ok) + setFocus() on the Ok button |
| bbef9502 | Add Expression Var support to Layer Editor | bug-fix | skip | Already in shared — layerTreeItem.{cpp,h} already carry _stage member, expression-var resolution via SdfVariableExpression, and layerTreeModel passes _sessionState->stage() to LayerTreeItem ctor |
| 39b722a9 | EMSUSD-2655: Show Layer Data Window * Another attempt to fix the error only t... | bug-fix | ported | Mac-build fix in usdSyntaxHighlighter.cpp ships with the verbatim port to lib/usdUfe/usd-layer-editor/lib/usdSyntaxHighlighter.cpp |
| bd3b808c | EMSUSD-2655: Show Layer Data Window * Linux/Mac build fix | bug-fix | ported | Linux/Mac build fix in usdSyntaxHighlighter.cpp ships with the verbatim port to lib/usdUfe/usd-layer-editor/lib/usdSyntaxHighlighter.cpp |
| 52ad9a67 | EMSUSD-2655: Show Layer Data Window * Adjusted message displayed in Window wh... | bug-fix | ported | kDisplayLayerContentsEmpty message added to lib/usdUfe/usd-layer-editor/lib/stringResources.h with the same wording |
| f22aebe5 | EMSUSD-2655: Show Layer Data Window * Code review - adding extra code comment. | bug-fix | ported | Comment ships verbatim in the ported lib/usdUfe/usd-layer-editor/lib/layerContentsWidget.cpp |
| 8b00f113 | EMSUSD-2655: Show Layer Data Window * Added new layer contents widget (to Lay... | layer-contents | ported | Foundational commit. The layer contents widget + usdSyntaxHighlighter + usdSyntaxConfig.json + LayerEditorWidget integration + DisplayLayerContents optionVar are all in lib/usdUfe/usd-layer-editor/lib/ now. Maya optionVar wiring stays in MayaSessionState. |
| defbebe9 | Update layerTreeViewStyle.h | bug-fix | skip | Fixes QT_DISABLE_DEPRECATED_BEFOR typo introduced by 3150544f in layerTreeViewStyle.h; shared port (3150544f) already applies the corrected spelling, so no extra change needed |
| 3150544f | Fixes for building with QT_DISABLE_DEPRECATED_BEFORE | bug-fix | ported | Added `QT_DISABLE_DEPRECATED_BEFORE \|\|` to the Qt6 #if checks in shared layerTreeItem.cpp, layerTreeItemDelegate.cpp, and layerTreeViewStyle.h (importDialog files are not in shared) |
| 9be70aed | EMSUSD-2232 - Disables cache rebuild due to layer editor | bug-fix | skip | Maya-only rebuildCache plumbing — shared stageSelectorWidget already has the Maya getProxyShape/getChildProxyShape paths commented out; Utils.cpp/h are Maya-only |
| 80cb4182 | Port layers editor fixes | bug-fix | skip | Already in shared — layerEditorCommands.cpp::backupLayer uses broader `_cmdId != kDiscardEdit` predicate (covers kClearLayer); stageSelectorWidget::updateFromSessionState already has _pinStageSelection/currentEntryQVariant logic; warningDialogs already sets focus on Ok. mayaSessionState.cpp is Maya-only |
| b4ec9672 | EMSUSD-2301 - Layer Editor Won't Launch * The LayerEditorWindowCommand proxyS... | bug-fix | ported | Applied to lib/usdUfe/usd-layer-editor/lib/layerEditorWidget.cpp: getSelectedLayers now uses _treeView->getSelectedLayerItems(); selectLayers sets currentIndex on the first selected layer and applies the selection in a single ClearAndSelect|Rows pass. Maya files (layerEditorWindowCommand, abstractLayerEditorWindow, mayaCommandHook, mayaLayerEditorWindow, Readme) are Maya-only |
| 4f945f1d | Add _cachedModelState nullptr check | bug-fix | skip | Already in shared: LayerTreeView::updateFromSessionState() in lib/usdUfe/usd-layer-editor/lib/layerTreeView.cpp already has the nullptr guard for _cachedModelState |
| 49753ba9 | Add documentation for notification and fix gcc compilation issue | bug-fix | ported | Applied gcc fix `for (const auto& stageLayer : stageLayers)` in lib/usdUfe/usd-layer-editor/lib/layerTreeView.cpp; Readme.md is Maya-only documentation |
| 93b2eb24 | Remove _cachedModelState reset call | bug-fix | skip | Already in shared: LayerTreeView::onModelReset() in shared lib already does not call _cachedModelState.reset() |
| 41282ecb | Change elide mode to middle | bug-fix | skip | Already in shared: lib/usdUfe/usd-layer-editor/lib/layerTreeItemDelegate.cpp already uses Qt::ElideMiddle |
| c5a79c20 | Update pin layer tooltip | bug-fix | skip | Already in shared: kPinUsdStageTooltip in lib/usdUfe/usd-layer-editor/lib/stringResources.h already has the updated text |
| ecbc1dc8 | Keep cache object state in memory throughout session | bug-fix | skip | Already in shared: selectLayerRequest rename, updateFromSessionState, preserve method, and stageListChangedSignal connect all present in shared layerTreeView.{cpp,h} |
| 48fb043e | Remove layerEditorCommands and layerEditorWidgetManager, move implementation ... | bug-fix | maya-only | Removes Maya-only MPxCommand-based layerEditorCommands.{h,cpp} and layerEditorWidgetManager.{h,cpp} from maya-usd, consolidating into Maya layerEditorWindowCommand.cpp. Shared lib has DCC-agnostic equivalents that remain in use |
| fc11cde3 | Add space for linter | bug-fix | maya-only | Linter fix on `#endif //X` -> `#endif // X` in Maya-only layerEditorCommands.h and layerEditorWidgetManager.h. Shared headers had no include guards before; guards added with proper spacing as part of faa0f9ef port |
| faa0f9ef | Add include guards | bug-fix | ported | Added include guards LAYER_EDITOR_COMMANDS_H and LAYER_EDITOR_WIDGETMANAGER_H to lib/usdUfe/usd-layer-editor/lib/LayerEditorCommands.h and layerEditorWidgetManager.h |
| b37d31ee | Reorder includes | bug-fix | maya-only | Reorders maya/MSyntax.h and MStringArray.h in Maya-only MPxCommand layerEditorCommands.cpp. Shared layerEditorCommands.cpp has no Maya headers |
| 1ed52bce | Address PR feedback 3 | bug-fix | maya-only | Maya MPxCommand layerEditorCommands.cpp and PXR_NAMESPACE_USING_DIRECTIVE move are Maya-only. Shared layerEditorWidgetManager.{cpp,h} already uses the un-prefixed names (instance, layerWidgetInstance) that match this commit's intent |
| 45745e01 | Address PR feedback 2 | bug-fix | maya-only | Touches Maya-only files (layerEditorCommands MPxCommand .h/.cpp, plugin/adsk/plugin/plugin.cpp). Shared manager .h/.cpp ended up with the un-prefixed naming applied by the later commit 1ed52bce |
| 0755cdbb | Fix linux compilation issue | bug-fix | maya-only | Touches Maya-only MPxCommand layerEditorCommands.cpp (loop signedness fix) |
| 1ed7bf6c | Add include vector | bug-fix | maya-only | Adds `#include <vector>` to Maya-only MPxCommand layerEditorCommands.h |
| e9aa2a49 | Format with clang | skip | skip | Formatting/lint commit, no functional change |
| b73dfe21 | Address PR feedback | bug-fix | skip | Maya-only MPxCommand layerEditorCommands.cpp parse() fix; layerEditorWidget.cpp duplicate loadSubLayers removal already in shared (single call only) |
| 458ff6bb | Add "mayaUsdSetSelectedLayers" and "mayaUsdGetSelectedLayers" layer editor hooks | bug-fix | skip | DCC-agnostic parts (onSelectionChanged, getSelectedLayers, selectLayers in layerEditorWidget, findUSDLayerItem public in layerTreeModel) already in shared. MPxCommand layerEditorCommands.{h,cpp} and plugin.cpp registration are Maya-only |
| f17afccb | EMSUSD-1722 fix node origin detection | bug-fix | maya-only | Touches Maya-only files: layerManager.{h,cpp}, proxyShapeBase.{h,cpp}, util.{h,cpp}, mayaSessionState.cpp, and a test file. No DCC-agnostic counterpart in shared lib |
| 9d770c0a | Merge pull request #3915 from Autodesk/bailp/EMSUSD-1619/remove-anon-layers | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
| 5d5c4250 | Merge pull request #3911 from Autodesk/bailp/EMSUSD-1510/save-layer-button-name | bug-fix | skip | Merge commit — content tracked in the individual commits being merged |
