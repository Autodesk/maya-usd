---

# Complete USD Layer Editor Test Suite Documentation

## Test Infrastructure Files

### testFixture.cpp (Shared Setup)
Provides the `LayerEditorTestFixture` class that establishes baseline conditions for all tests:
- Creates a `QMainWindow` and `StubLayerEditorWindow`
- Initializes two in-memory USD stages, each with one anonymous sublayer
- Provides helper methods: `sessionLayerIndex()`, `rootLayerIndex()`, `firstSublayerIndex()`, `selectRow()`
- Clears command hook call history before each test

### stubCommandHook.cpp (Command Tracking)
Implements `StubCommandHook` that records all command invocations for test verification:
- Tracked commands: `setEditTarget`, `insertSubLayerPath`, `removeSubLayerPath`, `replaceSubLayerPath`, `moveSubLayerPath`, `discardEdits`, `clearLayer`, `flattenLayer`, `addAnonymousSubLayer`, `muteSubLayer`, `lockLayer`, `refreshLayerSystemLock`, `stitchLayers`, `openUndoBracket`, `closeUndoBracket`, `showLayerEditorHelp`, `selectPrimsWithSpec`
- Provides methods: `hasCall()`, `callCount()`, `lastCall()`, `clearCalls()`

### stubSessionState.cpp (Session State Stub)
Provides `StubSessionState` that simulates the DCC environment:
- Creates two in-memory stages with anonymous sublayers
- Tracks function calls: `saveLayerUI()`, `loadLayersUI()`, `printLayer()`
- Allows simulating user interactions through counters

---

## testButtons.cpp (14 Tests)

**Shared setup:** All tests use `LayerEditorTestFixture` with a stage containing a root layer and one anonymous sublayer. Button visibility and enablement are queried by tooltip matching.

### 1. New Layer Button Click Calls Add Anonymous Sub Layer
`TEST_F(LayerEditorTestFixture, NewLayerButton_Click_CallsAddAnonymousSubLayer)`
**Where:** In the Layer Editor toolbar at the top of the panel, the "Add a New Layer" button is the first icon on the left (a document with a plus symbol).
**Context:** Verifies that clicking the toolbar's "Add a New Layer" button triggers the correct command hook. This is the primary way users create new layers, so confirming the button wires up to the right action is fundamental.
**Pre-conditions:** Layer editor widget is displayed with no selection, New Layer button is present and enabled.
**Action:** Click the "Add a New Layer" button.
**Expected result:** `addAnonymousSubLayer` is called on the command hook.

### 2. Load Layer Button Exists And Enabled
`TEST_F(LayerEditorTestFixture, LoadLayerButton_ExistsAndEnabled)`
**Where:** In the Layer Editor toolbar at the top of the panel, the "Load an Existing Layer" button (folder icon) appears to the right of the New Layer button.
**Context:** Confirms the "Load an Existing Layer" button is present and interactive in the toolbar. QA needs to know the button exists and is not accidentally disabled before any file-picker interaction is attempted.
**Pre-conditions:** Layer editor widget is displayed.
**Action:** Search for the "Load an Existing Layer" button.
**Expected result:** Button is found and enabled (disabled check is not performed to avoid blocking on file picker dialog).

### 3. Save Stage Button Enabled When Dirty
`TEST_F(LayerEditorTestFixture, SaveStageButton_EnabledWhenDirty)`
**Where:** In the Layer Editor toolbar at the top of the panel, the Save Stage button is near the right side. It is greyed out when no changes are pending and becomes active when the stage has unsaved edits.
**Context:** Confirms the Save Stage button activates when the stage has unsaved changes. Users rely on this visual cue to know a save is needed; if the button stays greyed out when the stage is dirty, edits could be silently lost.
**Pre-conditions:** Layer editor widget is displayed. Stage root layer is marked dirty.
**Action:** Process events twice to allow idle update to trigger.
**Expected result:** Save Stage button is enabled.

### 4. Save Stage Button Click Dismisses Dialog
`TEST_F(LayerEditorTestFixture, SaveStageButton_Click_DismissesDialog)`
**Where:** In the Layer Editor toolbar, click the Save Stage button (only active when the stage has unsaved changes). A save dialog may appear momentarily and should close on its own.
**Context:** Ensures clicking Save Stage with a dirty stage completes without hanging or crashing, even when the save flow opens a modal dialog. A hang here would block the entire DCC application.
**Pre-conditions:** Layer editor widget is displayed. Stage root layer is marked dirty.
**Action:** Set a timer to dismiss any modal dialog. Click Save Stage button and process events.
**Expected result:** Test completes without hang or crash.

### 5. New Layer Button Enabled When No Selection Defaults To Root
`TEST_F(LayerEditorTestFixture, NewLayerButton_EnabledWhenNoSelectionDefaultsToRoot)`
**Where:** In the Layer Editor toolbar, deselect all rows by clicking in an empty area of the tree. Observe whether the "Add a New Layer" toolbar button remains enabled.
**Context:** Validates that the New Layer button remains usable when no tree row is selected, falling back to the root layer as the insertion target. This prevents the button from being unexpectedly disabled during normal use.
**Pre-conditions:** Layer editor widget is displayed. No rows are selected in the tree.
**Action:** Clear all selection and process events.
**Expected result:** New Layer button is enabled and defaults to operating on root layer.

### 6. New Layer Button Click No Selection Adds To Root
`TEST_F(LayerEditorTestFixture, NewLayerButton_Click_NoSelection_AddsToRoot)`
**Where:** In the Layer Editor toolbar, with no row highlighted in the tree, click the "Add a New Layer" button. The new entry should appear as a sublayer directly under the root layer row.
**Context:** Confirms that with no selection, a new anonymous sublayer is added under the root layer rather than at an unexpected location. Correct default insertion behavior prevents users from creating layers in the wrong place.
**Pre-conditions:** Layer editor widget is displayed. No rows are selected.
**Action:** Clear selection, process events, click New Layer button.
**Expected result:** `addAnonymousSubLayer` is called on the root layer (not a sublayer).

### 7. New Layer Button Enabled For Root Layer
`TEST_F(LayerEditorTestFixture, NewLayerButton_EnabledForRootLayer)`
**Where:** In the layer tree, click the root layer row (labeled with the stage file name). Observe whether the "Add a New Layer" toolbar button above the tree is enabled.
**Context:** Checks that selecting the root layer keeps the New Layer button enabled, allowing users to add sublayers directly under the root. This is one of the most common authoring workflows.
**Pre-conditions:** Layer editor widget is displayed.
**Action:** Select the root layer row.
**Expected result:** New Layer button is enabled.

### 8. New Layer Button Enabled For Session Layer
`TEST_F(LayerEditorTestFixture, NewLayerButton_EnabledForSessionLayer)`
**Where:** In the layer tree, click the topmost "Session Layer" row. Observe whether the "Add a New Layer" toolbar button above the tree is enabled.
**Context:** Checks that selecting the session layer also enables the New Layer button, since users may want to add sublayers beneath the session layer. Verifies the button's enable logic covers all top-level layer types.
**Pre-conditions:** Layer editor widget is displayed.
**Action:** Select the session layer row.
**Expected result:** New Layer button is enabled.

### 9. New Layer Button Disabled When Selection Is Locked
`TEST_F(LayerEditorTestFixture, NewLayerButton_DisabledWhenSelectionIsLocked)`
**Where:** In the layer tree, click a row that displays a padlock icon. Observe that the "Add a New Layer" toolbar button above the tree becomes greyed out.
**Context:** Ensures the New Layer button is disabled when the selected layer is locked, preventing users from accidentally adding children to a read-only layer. This guards against silent permission violations during collaborative workflows.
**Pre-conditions:** Layer editor widget is displayed. Root layer is locked via TestUtils::lockLayerDirect.
**Action:** Select root layer and query button state.
**Expected result:** New Layer button is disabled. Test then unlocks the root layer.

### 10. New Layer Button Disabled When Selection Is System Locked
`TEST_F(LayerEditorTestFixture, NewLayerButton_DisabledWhenSelectionIsSystemLocked)`
**Where:** In the layer tree, click a row that shows a system-lock indicator (a distinct padlock style set by pipeline tooling, not the user). Observe that the "Add a New Layer" toolbar button becomes greyed out.
**Context:** Confirms the New Layer button is also disabled for system-locked layers, which are locked by an external authority (e.g., pipeline or file permissions) rather than the user. This is a stricter form of locking that must block layer creation just as user locks do.
**Pre-conditions:** Layer editor widget is displayed. Root layer is marked as system-locked and has edit permission revoked.
**Action:** Select root layer and query button state.
**Expected result:** New Layer button is disabled. Test then removes system lock and unlocks layer.

### 11. Save Button Hidden When Stage Is Not Shared
`TEST_F(LayerEditorTestFixture, SaveButton_HiddenWhenStageIsNotShared)`
**Where:** In the Layer Editor toolbar, look for a "Save all edits" button. On a non-shared stage this button is absent from the toolbar entirely.
**Context:** Verifies that the "Save all edits" button is hidden for non-shared stages, where per-layer saving is not relevant. Showing the button in this context would confuse users about which save workflow applies.
**Pre-conditions:** Layer editor widget is displayed. Stub stage is not a shared stage (default).
**Action:** Process events and search for "Save all edits" button.
**Expected result:** Button exists but is not visible (hidden for non-shared stages).

### 12. Load Layer Button Exists And Is Enabled
`TEST_F(LayerEditorTestFixture, LoadLayerButton_ExistsAndIsEnabled)`
**Where:** In the Layer Editor toolbar at the top of the panel, the "Load an Existing Layer" button (folder icon) should be present and not greyed out.
**Context:** A second explicit check that the Load Layer button is present and enabled. This duplicates coverage at a slightly different code path to catch regressions in button construction order or toolbar layout.
**Pre-conditions:** Layer editor widget is displayed.
**Action:** Search for the "Load an Existing Layer" button.
**Expected result:** Button is found and enabled.

### 13. New Layer Button Enabled For Sublayer Selection
`TEST_F(LayerEditorTestFixture, NewLayerButton_EnabledForSublayerSelection)`
**Where:** In the layer tree, click any indented sublayer row (under the root). Observe that the "Add a New Layer" toolbar button remains enabled.
**Context:** Verifies the New Layer button is enabled when a sublayer is selected, which would insert a sibling layer. This covers the nested-layer authoring case where users build up a layer stack incrementally.
**Pre-conditions:** Layer editor widget is displayed.
**Action:** Select the first sublayer (child of root).
**Expected result:** New Layer button is enabled.

### 14. New Layer Button Click With Sublayer Selection Adds Sibling
`TEST_F(LayerEditorTestFixture, NewLayerButton_Click_WithSublayerSelectionAddsSibling)`
**Where:** In the layer tree, click a sublayer row (indented under root), then click the "Add a New Layer" toolbar button. The new layer should appear alongside the selected sublayer at the same indent level, not nested inside it.
**Context:** Confirms that clicking New Layer with a sublayer selected inserts the new layer as a sibling (under the parent), not as a child of the selected sublayer. Correct sibling-insertion behavior is important for maintaining the intended layer stack structure.
**Pre-conditions:** Layer editor widget is displayed. First sublayer is selected.
**Action:** Click New Layer button and process events.
**Expected result:** `addAnonymousSubLayer` is called on the root layer (parent, creating a sibling).

---

## testContextMenu.cpp (23 Tests)

**Shared setup:** All tests use `LayerEditorTestFixture`. Tests select the first sublayer, then invoke operations directly through the window (not via menu exec, to avoid modal interaction).

### 15. Add Anonymous Sublayer Calls Hook
`TEST_F(LayerEditorTestFixture, ContextMenu_AddAnonymousSublayer_CallsHook)`
**Where:** Right-click on any layer row in the tree to open the context menu. The "Add Anonymous Sublayer" option is near the top of the menu.
**Context:** Verifies that the "Add Anonymous Sublayer" context menu action routes through the command hook rather than executing directly. This ensures undo/redo integration works and that the DCC host receives the operation for scene-graph bookkeeping.
**Pre-conditions:** First sublayer is selected.
**Action:** Call `_window->addAnonymousSublayer()`.
**Expected result:** `addAnonymousSubLayer` is recorded in command hook calls.

### 16. Mute Layer Calls Hook
`TEST_F(LayerEditorTestFixture, ContextMenu_MuteLayer_CallsHook)`
**Where:** Right-click on a sublayer row in the tree to open the context menu. The "Mute Layer" option toggles the layer in and out of stage composition.
**Context:** Confirms that muting a layer from the context menu dispatches through the command hook. Muting suppresses a layer's opinions on the stage, so proper hook dispatch is required for the host to reflect that state change.
**Pre-conditions:** First sublayer is selected.
**Action:** Call `_window->muteLayer()`.
**Expected result:** `muteSubLayer` is recorded in command hook calls.

### 17. Lock Layer Calls Hook
`TEST_F(LayerEditorTestFixture, ContextMenu_LockLayer_CallsHook)`
**Where:** Right-click on a layer row in the tree to open the context menu. The "Lock Layer" option applies a user lock to prevent edits to that layer.
**Context:** Confirms that locking a layer from the context menu dispatches through the command hook. Lock state prevents edits to a layer, so routing through the hook guarantees the host can persist and undo that restriction.
**Pre-conditions:** First sublayer is selected.
**Action:** Call `_window->lockLayer()`.
**Expected result:** `lockLayer` is recorded in command hook calls.

### 18. Remove Layer Calls Hook
`TEST_F(LayerEditorTestFixture, ContextMenu_RemoveLayer_CallsHook)`
**Where:** Right-click on a sublayer row in the tree to open the context menu. The "Remove Layer" option removes that row from the layer stack.
**Context:** Verifies that removing a sublayer via the context menu issues the correct hook call rather than directly mutating the stage. This is critical because layer removal is undoable and the host must track the affected path.
**Pre-conditions:** First sublayer is selected.
**Action:** Call `_window->removeSubLayer()`.
**Expected result:** `removeSubLayerPath` is recorded in command hook calls.

### 19. Discard Edits Calls Hook
`TEST_F(LayerEditorTestFixture, ContextMenu_DiscardEdits_CallsHook)`
**Where:** Right-click on a layer row that has unsaved changes (shows an asterisk or similar indicator). The "Discard Edits" option in the context menu reverts all unsaved changes on that layer.
**Context:** Checks that discarding edits on a layer routes through the command hook, ensuring the operation participates in undo history and the host is notified to refresh any dependent UI.
**Pre-conditions:** First sublayer is selected.
**Action:** Call `_window->discardEdits()`.
**Expected result:** `discardEdits` is recorded in command hook calls.

### 20. Print Layer Calls Session State
`TEST_F(LayerEditorTestFixture, ContextMenu_PrintLayer_CallsSessionState)`
**Where:** Right-click on a layer row in the tree to open the context menu. The "Print Layer" option sends the layer's USD content to Maya's Script Editor output.
**Context:** Verifies that the "Print Layer" debug action invokes the session state rather than the command hook, confirming the correct delegation boundary. This matters because print is a non-undoable diagnostic operation handled by the DCC environment.
**Pre-conditions:** First sublayer is selected.
**Action:** Call `_window->printLayer()`.
**Expected result:** `_sessionState._printLayerCallCount` is incremented.

### 21. Select Prims With Spec Calls Hook
`TEST_F(LayerEditorTestFixture, ContextMenu_SelectPrimsWithSpec_CallsHook)`
**Where:** Right-click on a layer row in the tree to open the context menu. The "Select Prims With Spec" option highlights scene objects that have definitions in that layer.
**Context:** Confirms that selecting scene prims whose definitions exist in the chosen layer dispatches through the command hook. This wires the layer editor to scene selection, which is host-managed and must be undoable.
**Pre-conditions:** First sublayer is selected.
**Action:** Call `_window->selectPrimsWithSpec()`.
**Expected result:** `selectPrimsWithSpec` is recorded in command hook calls.

### 22. Layer Query Session Layer Is Session Layer
`TEST_F(LayerEditorTestFixture, LayerQuery_SessionLayer_IsSessionLayer)`
**Where:** Click on the topmost row in the layer tree, labeled "Session Layer". Context menu items and toolbar buttons should reflect its special non-editable status.
**Context:** Validates that the window correctly identifies the session layer row as the session layer. Context menu items like "Discard Edits" behave differently for session layers, so this classification must be accurate.
**Pre-conditions:** Session layer is selected.
**Action:** Call `_window->isSessionLayer()`.
**Expected result:** Returns true.

### 23. Layer Query Sublayer Is Not Session Layer
`TEST_F(LayerEditorTestFixture, LayerQuery_Sublayer_IsNotSessionLayer)`
**Where:** Click on any indented sublayer row in the layer tree (under the root). It should not be treated as the special session layer.
**Context:** Validates the negative case: a regular sublayer must not be misidentified as the session layer. A false positive here would incorrectly suppress context menu options that are only restricted for the session layer.
**Pre-conditions:** First sublayer is selected.
**Action:** Call `_window->isSessionLayer()`.
**Expected result:** Returns false.

### 24. Layer Query Sublayer Is Sub Layer
`TEST_F(LayerEditorTestFixture, LayerQuery_Sublayer_IsSubLayer)`
**Where:** Click on any indented sublayer row (under the root) in the layer tree.
**Context:** Confirms that the window correctly classifies an ordinary sublayer as a sublayer. Several context menu actions (such as Remove and Move) are only enabled for sublayers, making this classification a gating condition.
**Pre-conditions:** First sublayer is selected.
**Action:** Call `_window->isSubLayer()`.
**Expected result:** Returns true.

### 25. Layer Query Session Layer Is Not Sub Layer
`TEST_F(LayerEditorTestFixture, LayerQuery_SessionLayer_IsNotSubLayer)`
**Where:** Click on the topmost "Session Layer" row in the tree. Its context menu differs from ordinary sublayers (e.g., Remove Layer is absent).
**Context:** Validates the negative case: the session layer must not be classified as a sublayer. Misclassifying it would incorrectly enable destructive actions such as Remove on the session layer.
**Pre-conditions:** Session layer is selected.
**Action:** Call `_window->isSubLayer()`.
**Expected result:** Returns false.

### 26. Context Menu Locked Layer Is Locked
`TEST_F(LayerEditorTestFixture, ContextMenu_LockedLayer_IsLocked)`
**Where:** Right-click on a layer row that displays a padlock icon. The context menu should reflect its locked status and editing options should be restricted.
**Context:** Verifies that after locking a layer the window's lock-query method reflects the new state. Context menu items for locked layers (such as disabling edits or showing a lock icon) depend on this query returning the correct value.
**Pre-conditions:** First sublayer is selected.
**Action:** Lock the sublayer via `_window->lockLayer()`, then reselect and check `_window->layerIsLocked()`.
**Expected result:** Returns true.

### 27. Context Menu Unlocked Layer Is Not Locked
`TEST_F(LayerEditorTestFixture, ContextMenu_UnlockedLayer_IsNotLocked)`
**Where:** Right-click on a freshly created layer row with no padlock icon. All editing context menu options should be available.
**Context:** Verifies the baseline: a freshly created sublayer reports itself as unlocked. This prevents the UI from incorrectly disabling edit actions on layers that have never been locked.
**Pre-conditions:** First sublayer is selected (fresh, unlocked).
**Action:** Call `_window->layerIsLocked()`.
**Expected result:** Returns false.

### 28. Context Menu Clear Layer Calls Hook
`TEST_F(LayerEditorTestFixture, ContextMenu_ClearLayer_CallsHook)`
**Where:** Right-click on a layer row in the tree. The "Clear Layer" option in the context menu wipes all authored data from that layer (distinct from removing the layer itself).
**Context:** Confirms that clearing all content from a layer dispatches through the command hook so the operation is undoable and the host can update dependent caches or authoring state.
**Pre-conditions:** First sublayer is selected.
**Action:** Call `_window->clearLayer()`.
**Expected result:** `clearLayer` is recorded in command hook calls.

### 29. Context Menu Save Edits Does Not Crash
`TEST_F(LayerEditorTestFixture, ContextMenu_SaveEdits_DoesNotCrash)`
**Where:** Right-click on an anonymous (in-memory) layer row that has unsaved content. The "Save Edits" option should open a save path without crashing.
**Context:** Smoke-tests the save path for an anonymous sublayer where the stub session state reports that no save UI was shown. Ensures the editor handles this edge case gracefully without an unhandled exception.
**Pre-conditions:** First sublayer is selected. Session state has saveLayerUI stubbed to return false.
**Action:** Call `_window->saveEdits()` and process events.
**Expected result:** No exception is thrown.

### 30. Context Menu Merge With Sublayers Blocked When No Sublayers
`TEST_F(LayerEditorTestFixture, ContextMenu_MergeWithSublayers_BlockedWhenNoSublayers)`
**Where:** Right-click on a leaf sublayer row (one with no expand arrow, meaning no children). The "Merge with Sublayers" option should be greyed out or absent.
**Context:** Ensures that attempting to merge a leaf layer (one with no children) is a no-op. Allowing a merge on a layer with no sublayers would invoke stitchLayers unnecessarily and could corrupt stage data.
**Pre-conditions:** First sublayer is selected (leaf layer with no children).
**Action:** Clear command hook history and call `_window->mergeWithSublayers()`.
**Expected result:** `stitchLayers` is not called.

### 31. Context Menu Merge With Sublayers Blocked When Layer Is Locked
`TEST_F(LayerEditorTestFixture, ContextMenu_MergeWithSublayers_BlockedWhenLayerIsLocked)`
**Where:** Right-click on a locked layer row (shows a padlock icon). The "Merge with Sublayers" option in the context menu should be greyed out.
**Context:** Ensures that a locked layer cannot be merged with its sublayers even if it has children. Merging flattens opinions into the parent layer, which would bypass the lock restriction if not guarded.
**Pre-conditions:** Root layer is locked via TestUtils. Root layer is selected.
**Action:** Clear command hook history and call `_window->mergeWithSublayers()`.
**Expected result:** `stitchLayers` is not called. Layer is then unlocked.

### 32. Context Menu Discard Edits Skips Confirm For Anonymous Layer
`TEST_F(LayerEditorTestFixture, ContextMenu_DiscardEdits_SkipsConfirmForAnonymousLayer)`
**Where:** Right-click on an anonymous (in-memory) layer row and choose "Discard Edits". No confirmation dialog should appear — the edits should be discarded immediately.
**Context:** Verifies that anonymous layers bypass the confirmation dialog when discarding edits, since anonymous layers cannot be reloaded from disk and the discard is always safe to perform immediately.
**Pre-conditions:** First sublayer is selected. Sublayer is anonymous.
**Action:** Clear command hook history and call `_window->discardEdits()`.
**Expected result:** `discardEdits` is called without showing a confirmation dialog.

### 33. Context Menu Discard Edits Skips Confirm For Clean Layer
`TEST_F(LayerEditorTestFixture, ContextMenu_DiscardEdits_SkipsConfirmForCleanLayer)`
**Where:** Right-click on a layer row with no pending changes and choose "Discard Edits". No confirmation dialog should appear since there is nothing to discard.
**Context:** Verifies that a layer with no unsaved edits skips the confirmation dialog when discard is invoked, avoiding an unnecessary prompt that would interrupt the artist's workflow.
**Pre-conditions:** First sublayer is selected. Sublayer has no dirty content.
**Action:** Clear command hook history and call `_window->discardEdits()`.
**Expected result:** `discardEdits` is called without showing a confirmation dialog.

### 34. Set Edit Target Blocked When Layer Is Muted
`TEST_F(LayerEditorTestFixture, SetEditTarget_BlockedWhenLayerIsMuted)`
**Where:** In the layer tree, try to double-click a dimmed (muted) sublayer row to set it as the edit target. The target indicator (pencil/arrow icon) should not move to that row.
**Context:** Confirms that a muted layer cannot become the edit target. Muted layers have no effect on the composed stage, so writing new opinions into a muted layer would silently produce no visible result and confuse artists.
**Pre-conditions:** First sublayer is selected and muted at the stage level.
**Action:** Clear command hook history and call `treeModel()->setEditTarget(item)`.
**Expected result:** `setEditTarget` is not called. Layer is then unmuted.

### 35. Set Edit Target Blocked When Layer Is Locked
`TEST_F(LayerEditorTestFixture, SetEditTarget_BlockedWhenLayerIsLocked)`
**Where:** In the layer tree, try to double-click a locked sublayer row (padlock visible) to set it as the edit target. The target indicator should not move to that row.
**Context:** Confirms that a locked layer cannot become the edit target. Designating a locked layer as the edit target would immediately prevent any new authoring and is therefore blocked at the model level before the hook is called.
**Pre-conditions:** First sublayer is selected and locked via TestUtils.
**Action:** Clear command hook history and call `treeModel()->setEditTarget(item)`.
**Expected result:** `setEditTarget` is not called. Layer is then unlocked.

### 36. Set Edit Target Blocked When Layer Is System Locked
`TEST_F(LayerEditorTestFixture, SetEditTarget_BlockedWhenLayerIsSystemLocked)`
**Where:** In the layer tree, try to double-click a system-locked sublayer row (distinct padlock indicator) to set it as the edit target. The target indicator should not move to that row.
**Context:** Confirms that a system-locked layer (locked by an external policy, not the user) also blocks edit-target assignment. System locks are enforced by the pipeline and must not be overridden through normal layer editor interactions.
**Pre-conditions:** First sublayer is selected and system-locked.
**Action:** Clear command hook history and call `treeModel()->setEditTarget(item)`.
**Expected result:** `setEditTarget` is not called. Layer is then cleaned up.

### 37. Set Edit Target Allowed For Normal Sublayer
`TEST_F(LayerEditorTestFixture, SetEditTarget_AllowedForNormalSublayer)`
**Where:** In the layer tree, double-click a normal unlocked sublayer row to set it as the edit target. A pencil or arrow icon should appear next to that row indicating it is now the active edit target.
**Context:** Verifies the positive case: a normal, unlocked, unmuted sublayer can be promoted to the edit target so that new USD opinions are authored into it. This is the standard artist workflow for directing edits to a specific layer.
**Pre-conditions:** First sublayer is selected (normal, unlocked, unmuted state).
**Action:** Clear command hook history and call `treeModel()->setEditTarget(item)`.
**Expected result:** `setEditTarget` is called.

---

## testLayerContentsWidget.cpp (6 Tests)

**Shared setup:** All tests use `LayerEditorTestFixture`. The `LayerContentsWidget` is located in the widget tree via `findContentsWidget()`.

### 38. Contents Widget Exists In Layout
`TEST_F(LayerContentsWidgetTest, ContentsWidget_ExistsInLayout)`
**Where:** Below the layer tree in the Layer Editor panel, there is an optional contents pane. It becomes visible when "Display Layer Content" is enabled in the Options menu.
**Context:** The layer contents panel is an optional pane that displays USD layer data inline in the editor. QA needs to confirm this widget is actually embedded in the layout before any content-display tests can be meaningful.
**Pre-conditions:** Layer editor widget is created and displayed.
**Action:** Search for `LayerContentsWidget` in the widget tree.
**Expected result:** Widget is found and not null.

### 39. Is Empty True By Default
`TEST_F(LayerContentsWidgetTest, IsEmpty_TrueByDefault)`
**Where:** The layer contents pane below the tree should appear blank (no text or content) when the Layer Editor first opens and no layer has been selected.
**Context:** When no layer has been selected or assigned to the contents widget, it should show nothing. This baseline check ensures the widget does not accidentally pre-populate with stale or garbage content on construction.
**Pre-conditions:** Layer editor widget is created with contents widget.
**Action:** Call `isEmpty()` on the contents widget.
**Expected result:** Returns true (no layer selected initially).

### 40. Set Layer Sets Is Empty False For Layer With Content
`TEST_F(LayerContentsWidgetTest, SetLayer_SetsIsEmptyFalseForLayerWithContent)`
**Where:** Click a layer row in the tree; the contents pane below should populate with that layer's USD text data.
**Context:** Assigning a layer that contains data (here, a comment) should cause the contents widget to display something. This verifies the widget correctly transitions from empty to populated when given a real layer.
**Pre-conditions:** Layer editor widget is created. Root layer has comment set to mark it with content.
**Action:** Call `setLayer(rootLayer)` on contents widget and process events.
**Expected result:** `isEmpty()` returns false.

### 41. Clear Sets Is Empty True
`TEST_F(LayerContentsWidgetTest, Clear_SetsIsEmptyTrue)`
**Where:** After the contents pane shows a layer's USD data, deselecting that layer (or calling clear programmatically) should return the pane to a blank state.
**Context:** The contents widget must be clearable so the editor can blank it out when a user deselects a layer. This confirms that calling `clear()` after loading content properly resets the widget state.
**Pre-conditions:** Layer editor widget is created. Contents widget has a layer set.
**Action:** Call `setLayer(layer)` to populate, then call `clear()`.
**Expected result:** `isEmpty()` returns true.

### 42. Set Layer With Null Layer Is Empty
`TEST_F(LayerContentsWidgetTest, SetLayer_WithNullLayer_IsEmpty)`
**Where:** The layer contents pane below the tree should show no content when no layer is selected or when the selection is cleared.
**Context:** Passing a null layer is the programmatic equivalent of "no layer selected." The widget must handle this gracefully and report itself as empty rather than crashing or retaining previous content.
**Pre-conditions:** Layer editor widget is created.
**Action:** Call `setLayer(nullptr)` and process events.
**Expected result:** `isEmpty()` returns true.

### 43. Set Layer Does Not Crash
`TEST_F(LayerContentsWidgetTest, SetLayer_DoesNotCrash)`
**Where:** Click any layer row in the tree while "Display Layer Content" is enabled in the Options menu. The contents pane below should update without crashing.
**Context:** Loading a layer into the contents widget triggers internal display logic tied to the "Display Layer Content" option. This smoke test ensures that basic invocation path does not throw or assert, even without verifying the rendered output.
**Pre-conditions:** Layer editor widget is created. Root layer is available.
**Action:** Call `setLayer(rootLayer)`.
**Expected result:** No exception is thrown.

---

## testLayerEditorCommands.cpp (31 Tests)

All tests in this file use Google Test's `::testing::Test` base class and create USD stages directly, not the `LayerEditorTestFixture`.

### 44. Header Includes Compile
`TEST(LayerEditorCommandsSmokeTest, HeaderIncludesCompile)`
**Where:** This is a build-only smoke test with no visual UI component.
**Context:** Confirms that all command class headers can be included together without compilation errors. This is a basic smoke test ensuring the build system and include paths are correctly configured for the commands module.
**Pre-conditions:** Command class headers are included.
**Action:** Succeed marker.
**Expected result:** Code compiles without errors.

### 45. When No Modifiable Layers Edit Target Changes To Session Layer
`TEST_F(UpdateEditTargetTest, WhenNoModifiableLayers_EditTargetChangesToSessionLayer)`
**Where:** In the layer tree, mute a sublayer when all remaining layers are also locked. The edit-target indicator (pencil or arrow icon) should automatically jump to the "Session Layer" row at the top.
**Context:** Verifies that when a mute operation leaves no writable layers on the stage, the editor automatically redirects the edit target to the session layer. This prevents the user from getting into a state where all edits would be silently rejected.
**Pre-conditions:** Stage with root and sublayer. Both layers are locked. Edit target is set to root.
**Action:** Create MuteLayerCmd to mute the sublayer and execute.
**Expected result:** Edit target switches to session layer (no modifiable layers remain).

### 46. When Checker Disables Auto Retarget Edit Target Unchanged
`TEST_F(UpdateEditTargetTest, WhenCheckerDisablesAutoRetarget_EditTargetUnchanged)`
**Where:** In the layer tree, the edit-target indicator should remain on its current row even after all sublayers are locked/muted, when the host application has registered a checker that suppresses auto-retarget.
**Context:** Verifies that a registered checker callback can suppress the automatic edit-target fallback. This allows host applications to opt out of the auto-retarget behaviour when they manage edit targets themselves.
**Pre-conditions:** Stage with root and sublayer. Both locked. Edit target is root. Auto-retarget checker is set to return true.
**Action:** Create and execute MuteLayerCmd on sublayer.
**Expected result:** Edit target remains unchanged (checker suppresses auto-retarget).

### 47. With Out Provider Edit Target Not Restored On Undo
`TEST_F(BackupEditTargetsTest, WithoutProvider_EditTargetNotRestoredOnUndo)`
**Where:** In the layer tree, after using Edit > Undo to reverse a "Clear Layer" operation, the edit-target indicator should remain wherever it currently points (no automatic restore without a provider).
**Context:** Confirms that without a stage-cache provider registered, undo of a clear operation does not attempt to restore the edit target. This establishes the baseline behaviour when the optional backup mechanism is absent.
**Pre-conditions:** Stage not in global cache. Root -> A -> B with edit target B. ClearLayerCmd on A.
**Action:** Execute clear, then undo.
**Expected result:** Edit target remains at B (no backup provider, so edit target not restored).

### 48. With Provider Edit Target Restored On Undo
`TEST_F(BackupEditTargetsTest, WithProvider_EditTargetRestoredOnUndo)`
**Where:** In the layer tree, after using Edit > Undo to reverse a "Clear Layer" operation, the edit-target indicator should jump back to the layer that was the target before the clear.
**Context:** Verifies the full backup-and-restore cycle: when a stage-cache provider is registered, clearing a layer saves the current edit target so that undo can bring it back. This is critical for a correct undo history from a user perspective.
**Pre-conditions:** Stage in global cache via provider. Root -> A -> B with edit target B. ClearLayerCmd on A.
**Action:** Execute clear (edit target backed up and reset), then undo.
**Expected result:** Edit target is restored to B after undo.

### 49. Do It Replaces Old Path With New Path
`TEST_F(ReplaceSubPathCmdTest, DoIt_ReplacesOldPathWithNewPath)`
**Where:** In the layer tree, a path-replacement operation (triggered when renaming or relocating a file on disk) updates the layer row's displayed file path.
**Context:** Verifies that ReplaceSubPathCmd correctly swaps a sublayer path inside its parent layer. Path replacement is used when a layer file is renamed or relocated on disk.
**Pre-conditions:** Parent layer with sublayer A. ReplaceSubPathCmd to replace A with B.
**Action:** Execute command.
**Expected result:** Parent no longer contains A's path, now contains B's path.

### 50. Undo It Restores Old Path
`TEST_F(ReplaceSubPathCmdTest, UndoIt_RestoresOldPath)`
**Where:** In the layer tree, after using Edit > Undo following a path replacement, the original file path should re-appear on that layer row.
**Context:** Confirms that undoing a path replacement restores the original path and removes the replacement. This ensures the undo stack correctly reverses a rename/relocate operation.
**Pre-conditions:** Parent with A replaced by B via ReplaceSubPathCmd.
**Action:** Undo the command.
**Expected result:** Parent now contains A, no longer contains B.

### 51. Do It Returns False When Old Path Not Found
`TEST_F(ReplaceSubPathCmdTest, DoIt_ReturnsFalse_WhenOldPathNotFound)`
**Where:** If a Replace Path operation targets a path not present in the layer stack, no change should appear in the tree and an error should be raised internally.
**Context:** Verifies that attempting to replace a path that does not exist in the parent raises an error rather than silently corrupting the layer stack. This guards against stale or incorrect path references.
**Pre-conditions:** Parent with sublayer A. ReplaceSubPathCmd with nonexistent old path.
**Action:** Execute command.
**Expected result:** Throws std::runtime_error; A still in parent, B not added.

### 52. Discard Edit Cmd Do It Clears Layer Content
`TEST_F(BackupLayerCmdTest, DiscardEditCmd_DoIt_ClearsLayerContent)`
**Where:** Right-click a layer row and choose "Discard Edits" from the context menu. The layer's unsaved content is wiped and any dirty indicator on the row disappears.
**Context:** Verifies that DiscardEditCmd wipes all content from a layer, simulating a "revert to empty" operation. QA should confirm that after discarding, the layer holds no authored data.
**Pre-conditions:** Layer with "original content" comment.
**Action:** Create and execute DiscardEditCmd.
**Expected result:** Layer comment is cleared.

### 53. Discard Edit Cmd Undo Restores Layer Content
`TEST_F(BackupLayerCmdTest, DiscardEditCmd_Undo_RestoresLayerContent)`
**Where:** After discarding edits (via context menu > "Discard Edits"), use Edit > Undo. The dirty indicator and original content should return to that layer row.
**Context:** Confirms that undoing a discard operation brings back the original layer content. This is essential for preserving the user's work if they accidentally discard edits.
**Pre-conditions:** Layer with content. DiscardEditCmd executed.
**Action:** Undo the command.
**Expected result:** Layer comment is restored to "original content".

### 54. Clear Layer Cmd Do It Empties Layer
`TEST_F(BackupLayerCmdTest, ClearLayerCmd_DoIt_EmptiesLayer)`
**Where:** Right-click a layer row and choose "Clear Layer" from the context menu. All authored data is removed from that layer (the row remains but is now empty).
**Context:** Verifies that ClearLayerCmd removes all authored content from a layer, similar to DiscardEditCmd but intended as a persistent clear rather than a revert. QA should confirm the layer is empty after execution.
**Pre-conditions:** Layer with "original content" comment.
**Action:** Create and execute ClearLayerCmd.
**Expected result:** Layer comment is cleared.

### 55. Clear Layer Cmd Undo Restores Content
`TEST_F(BackupLayerCmdTest, ClearLayerCmd_Undo_RestoresContent)`
**Where:** After clearing a layer (via context menu > "Clear Layer"), use Edit > Undo. The layer's content and dirty indicator should be restored.
**Context:** Confirms that clearing a layer is undoable and that the original content is fully restored. This protects users from accidental data loss through the clear operation.
**Pre-conditions:** Layer with content. ClearLayerCmd executed.
**Action:** Undo the command.
**Expected result:** Layer comment is restored to "original content".

### 56. Do It Mutes Layer
`TEST_F(MuteLayerCmdTest, DoIt_MutesLayer)`
**Where:** Right-click a sublayer row and choose "Mute Layer", or click the mute icon button on the right side of the row. The row should appear visually dimmed to indicate it is excluded from composition.
**Context:** Verifies that MuteLayerCmd with the mute flag set true causes the stage to report the layer as muted. Muting suppresses a layer's contributions without removing it from the stack.
**Pre-conditions:** Stage with mutable sublayer.
**Action:** Create and execute MuteLayerCmd(stage, layer, true).
**Expected result:** `stage->IsLayerMuted(layer->GetIdentifier())` returns true.

### 57. Undo Unmutes Layer
`TEST_F(MuteLayerCmdTest, Undo_UnmutesLayer)`
**Where:** After muting a layer, use Edit > Undo. The dimmed row should return to its normal (fully visible) appearance and the mute icon should appear untoggled.
**Context:** Confirms that undoing a mute operation restores the layer to its active (unmuted) state. This ensures the mute command participates correctly in the undo history.
**Pre-conditions:** Layer muted via MuteLayerCmd.
**Action:** Undo the command.
**Expected result:** `stage->IsLayerMuted()` returns false.

### 58. Do It Unmute Unmutes Already Muted Layer
`TEST_F(MuteLayerCmdTest, DoIt_Unmute_UnmutesAlreadyMutedLayer)`
**Where:** Right-click a dimmed (muted) sublayer row and choose "Unmute Layer" (or click the active mute icon). The row should return to its normal undimmed appearance.
**Context:** Verifies that MuteLayerCmd with the mute flag set false can unmute a layer that was already muted outside the command system. This covers the unmute direction of the same command class.
**Pre-conditions:** Stage with layer already muted.
**Action:** Create and execute MuteLayerCmd(stage, layer, false).
**Expected result:** `stage->IsLayerMuted()` returns false.

### 59. Undo Unmute Restores Muted State
`TEST_F(MuteLayerCmdTest, Undo_Unmute_RestoresMutedState)`
**Where:** After unmuting a layer, use Edit > Undo. The row should return to its dimmed appearance and the mute icon should appear toggled.
**Context:** Confirms that undoing an unmute operation puts the layer back into a muted state. Both directions of MuteLayerCmd must be fully undoable for a consistent undo experience.
**Pre-conditions:** Layer already muted. MuteLayerCmd(false) executed.
**Action:** Undo the command.
**Expected result:** `stage->IsLayerMuted()` returns true.

### 60. Do It Locks Layer
`TEST_F(LockLayerCmdTest, DoIt_LocksLayer)`
**Where:** Right-click a sublayer row and choose "Lock Layer" from the context menu, or click the lock icon button on the right side of the row. A padlock icon should appear on that row.
**Context:** Verifies that LockLayerCmd marks a layer as locked, preventing further edits to it. Locking is a key workflow safeguard that QA must confirm is applied by the command.
**Pre-conditions:** Stage with lockable sublayer.
**Action:** Create and execute LockLayerCmd(stage, layer, LayerLock_Locked).
**Expected result:** `isLayerLocked(layer)` returns true.

### 61. Undo Unlocks Layer
`TEST_F(LockLayerCmdTest, Undo_UnlocksLayer)`
**Where:** After locking a layer, use Edit > Undo. The padlock icon on that row should disappear.
**Context:** Confirms that undoing a lock operation restores the layer to an unlocked, editable state. This ensures lock commands are reversible via the standard undo mechanism.
**Pre-conditions:** Layer locked via LockLayerCmd.
**Action:** Undo the command.
**Expected result:** `isLayerLocked(layer)` returns false.

### 62. Skip System Locked Does Not Lock System Locked Sublayers
`TEST_F(LockLayerCmdTest, SkipSystemLocked_DoesNotLockSystemLockedSublayers)`
**Where:** In the layer tree, lock a parent layer; child rows that already display a system-lock indicator should keep their existing icon unchanged, not overwritten by the user lock.
**Context:** Verifies that when locking a parent layer with the "skip system-locked" option, sublayers already under a system lock are left untouched. This prevents the command from accidentally overwriting externally enforced lock states.
**Pre-conditions:** Parent layer with system-locked sublayer. LockLayerCmd(parent, includeSubLayers=true, skipSystemLocked=true).
**Action:** Execute command.
**Expected result:** Parent is locked. Sublayer remains system-locked (not relocked).

### 63. Do It Inserts Sub Layer At Index
`TEST_F(InsertSubPathCmdTest, DoIt_InsertsSubLayerAtIndex)`
**Where:** Click the "Add a New Layer" toolbar button or right-click a layer row and choose "Add Anonymous Sublayer". A new row should appear in the tree under the selected parent.
**Context:** Verifies that InsertSubPathCmd adds a layer path at the specified position in a parent layer's sublayer list. Correct index placement is important for layer opinion ordering in USD.
**Pre-conditions:** Parent layer with no sublayers. InsertSubPathCmd to insert sub at index 0.
**Action:** Execute command.
**Expected result:** Sub is found in parent's sublayer paths.

### 64. Undo Removes Inserted Sub Layer
`TEST_F(InsertSubPathCmdTest, Undo_RemovesInsertedSubLayer)`
**Where:** After adding a new layer via the toolbar or context menu, use Edit > Undo. The newly added row should disappear from the tree.
**Context:** Confirms that undoing an insert removes the previously added sublayer path, restoring the parent to its original state. This is required for a clean undo of "Add Layer" operations.
**Pre-conditions:** Sub inserted via InsertSubPathCmd.
**Action:** Undo the command.
**Expected result:** Sub is no longer in parent's sublayer paths.

### 65. Do It Removes Sub Layer
`TEST_F(RemoveSubPathCmdTest, DoIt_RemovesSubLayer)`
**Where:** Right-click a sublayer row and choose "Remove Layer". The row should disappear from the tree.
**Context:** Verifies that RemoveSubPathCmd detaches a sublayer from its parent by removing its path entry. This is the underlying operation behind "Remove Layer" in the editor UI.
**Pre-conditions:** Parent with sublayer at index 0. RemoveSubPathCmd.
**Action:** Execute command.
**Expected result:** Sublayer is no longer found in parent's paths.

### 66. Undo Restores Sub Layer
`TEST_F(RemoveSubPathCmdTest, Undo_RestoresSubLayer)`
**Where:** After removing a sublayer (via context menu > "Remove Layer"), use Edit > Undo. The row should reappear in the tree at its original position.
**Context:** Confirms that undoing a remove operation re-inserts the sublayer path at its original position. This ensures users can recover from accidental layer removals.
**Pre-conditions:** Sublayer removed via RemoveSubPathCmd.
**Action:** Undo the command.
**Expected result:** Sublayer is restored to parent.

### 67. Do It Inserts Anon Layer
`TEST_F(AddAnonSubLayerCmdTest, DoIt_InsertsAnonLayer)`
**Where:** Click the "Add a New Layer" toolbar button; a new anonymous layer row appears in the tree as a child of the selected parent (or root if nothing is selected).
**Context:** Verifies that AddAnonSubLayerCmd creates a new anonymous layer and adds it to the parent's sublayer list. This is the command invoked by the "New Layer" button in the editor.
**Pre-conditions:** Parent layer. AddAnonSubLayerCmd with name set.
**Action:** Execute command.
**Expected result:** Parent has one sublayer path.

### 68. Do It Returns Non Empty Identifier
`TEST_F(AddAnonSubLayerCmdTest, DoIt_ReturnsNonEmptyIdentifier)`
**Where:** After adding a new anonymous layer via the toolbar button, the new row in the tree displays a generated name label rather than a blank entry.
**Context:** Confirms that after executing AddAnonSubLayerCmd, the command exposes the identifier of the newly created layer. The identifier is needed by other commands and by the UI to track the new layer.
**Pre-conditions:** Parent layer. AddAnonSubLayerCmd.
**Action:** Execute command and call `addedLayer()`.
**Expected result:** Returns non-empty string identifier.

### 69. Undo Removes Anon Layer
`TEST_F(AddAnonSubLayerCmdTest, Undo_RemovesAnonLayer)`
**Where:** After adding an anonymous layer via the toolbar button, use Edit > Undo. The newly created row should disappear from the tree.
**Context:** Confirms that undoing the add-anonymous-layer command removes the layer from the parent, leaving no sublayer paths. This pairs with the insert test to fully verify the command's undo support.
**Pre-conditions:** Anon layer added via AddAnonSubLayerCmd.
**Action:** Undo the command.
**Expected result:** Parent has zero sublayer paths.

### 70. Do It Same Parent Reorders Sub Layer
`TEST_F(MoveSubPathCmdTest, DoIt_SameParent_ReordersSubLayer)`
**Where:** In the layer tree, drag a sublayer row to a new position within the same parent. The rows should update their order to reflect the drag.
**Context:** Verifies that MoveSubPathCmd can change a sublayer's position within the same parent, which controls USD opinion strength ordering. Correct reordering is essential for the drag-and-drop reorder feature.
**Pre-conditions:** Parent with three sublayers [A, B, C] at indices 0, 1, 2. MoveSubPathCmd to move A to index 2.
**Action:** Execute command.
**Expected result:** Sublayers are now [B, C, A].

### 71. Undo Same Parent Restores Original Order
`TEST_F(MoveSubPathCmdTest, Undo_SameParent_RestoresOriginalOrder)`
**Where:** After reordering sublayers by drag, use Edit > Undo. The rows should return to their previous order.
**Context:** Confirms that undoing an in-parent reorder restores the exact original sublayer sequence. This ensures drag-and-drop reordering is safely reversible.
**Pre-conditions:** Sublayers reordered via MoveSubPathCmd.
**Action:** Undo the command.
**Expected result:** Order is restored to [A, B, C].

### 72. Do It Cross Parent Moves Sub Layer To New Parent
`TEST_F(MoveSubPathCmdTest, DoIt_CrossParent_MovesSubLayerToNewParent)`
**Where:** In the layer tree, drag a sublayer row and drop it under a different parent layer. The row should move to the new parent's children.
**Context:** Verifies that MoveSubPathCmd can transfer a sublayer from one parent layer to a different parent, supporting drag-and-drop between layers in the tree view.
**Pre-conditions:** Parent with sublayer A. MoveSubPathCmd to move A to newParent.
**Action:** Execute command.
**Expected result:** A is no longer in parent, now in newParent.

### 73. Undo Cross Parent Restores Sub Layer To Original Parent
`TEST_F(MoveSubPathCmdTest, Undo_CrossParent_RestoresSubLayerToOriginalParent)`
**Where:** After moving a sublayer to a new parent via drag, use Edit > Undo. The row should return to its original parent.
**Context:** Confirms that undoing a cross-parent move returns the sublayer to its original parent and removes it from the destination. Full undo support for cross-parent moves is required for safe drag-and-drop.
**Pre-conditions:** Sublayer moved to newParent via MoveSubPathCmd.
**Action:** Undo the command.
**Expected result:** Sublayer is back in original parent.

### 74. Add Callback Context Stores Entry
`TEST(RefreshSystemLockCallbackContextTest, AddCallbackContext_StoresEntry)`
**Where:** This is an internal command metadata test. No direct visual component; it enables pipeline tools to attach path context when system locks are refreshed on layers.
**Context:** Verifies that RefreshSystemLockLayerCmd can accept and store arbitrary key-value metadata (such as a proxy shape path) for use by downstream callbacks. This extensibility mechanism lets host applications attach context that the command passes to lock-refresh hooks.
**Pre-conditions:** RefreshSystemLockLayerCmd created.
**Action:** Call `addCallbackContext("proxyShapePath", VtValue(string))`.
**Expected result:** Entry is stored in `_extraCallbackContext` map.

---

## testLayerLocking.cpp (13 Tests)

**Shared setup:** All tests use `::testing::Test` base class. Each test creates a fresh anonymous layer. Setup clears all lock state. Teardown restores layer permissions and clears locks.

### 75. Is Layer Locked False By Default
`TEST_F(LayerLockingTest, IsLayerLocked_FalseByDefault)`
**Where:** In the layer tree, a freshly created sublayer row shows no padlock icon on the right side, confirming it starts in an unlocked state.
**Context:** Verifies that a brand-new layer carries no lock state out of the box. This is the baseline that all other locking tests depend on — if the default is wrong, every locked/unlocked check becomes unreliable.
**Pre-conditions:** Fresh anonymous layer created.
**Action:** Call `isLayerLocked(layer)`.
**Expected result:** Returns false.

### 76. Lock Layer Sets Layer As Locked
`TEST_F(LayerLockingTest, LockLayer_SetsLayerAsLocked)`
**Where:** In the layer tree, after locking a layer (via context menu > "Lock Layer" or the lock icon button on the right side of the row), a padlock icon should appear on that row.
**Context:** Verifies that calling the lock API with the "locked" state actually registers the layer as locked. This is the most fundamental locking operation: without it, the entire lock feature is broken.
**Pre-conditions:** Fresh layer.
**Action:** Call `lockLayer("", layer, LayerLock_Locked, false)`.
**Expected result:** `isLayerLocked(layer)` returns true.

### 77. Unlock Layer Sets Layer As Unlocked
`TEST_F(LayerLockingTest, UnlockLayer_SetsLayerAsUnlocked)`
**Where:** In the layer tree, after unlocking a locked layer (via context menu > "Unlock Layer" or clicking the active padlock icon), the padlock icon should disappear from that row.
**Context:** Verifies that a locked layer can be returned to the unlocked state. Unlocking is a critical workflow — artists need to be able to re-enable editing after a layer was locked by mistake or policy.
**Pre-conditions:** Layer locked via lockLayer.
**Action:** Call `lockLayer("", layer, LayerLock_Unlocked, false)`.
**Expected result:** `isLayerLocked(layer)` returns false.

### 78. Lock Layer Revokes Permission To Edit
`TEST_F(LayerLockingTest, LockLayer_RevokesPermissionToEdit)`
**Where:** In the layer tree, a locked row (padlock icon visible) cannot be set as the edit target. Double-clicking it to make it the active edit layer should have no effect.
**Context:** Verifies that locking a layer also removes its USD-level edit permission. This is what prevents changes from being written to the layer at the USD API level, not just in the layer editor UI.
**Pre-conditions:** Layer has edit permission.
**Action:** Lock the layer.
**Expected result:** `layer->PermissionToEdit()` returns false.

### 79. Unlock Layer Restores Permission To Edit
`TEST_F(LayerLockingTest, UnlockLayer_RestoresPermissionToEdit)`
**Where:** In the layer tree, after unlocking a previously locked row (padlock gone), double-clicking it to set it as the edit target should succeed — the edit-target indicator should move to that row.
**Context:** Verifies that unlocking reverses the edit-permission revocation. If permissions are not restored, the layer would appear unlocked in the UI but still reject edits at the USD level.
**Pre-conditions:** Locked layer.
**Action:** Unlock the layer.
**Expected result:** `layer->PermissionToEdit()` returns true.

### 80. Lock Layer Toggle Roundtrip Restores Original State
`TEST_F(LayerLockingTest, LockLayer_ToggleRoundtrip_RestoresOriginalState)`
**Where:** In the layer tree, clicking the lock icon on a row and then clicking it again should leave the row in its original unlocked state with no visible padlock.
**Context:** Verifies that lock followed immediately by unlock leaves the layer in exactly its original state. This guards against state leakage where repeated toggling accumulates side effects.
**Pre-conditions:** Unlocked layer.
**Action:** Lock then unlock.
**Expected result:** Layer is unlocked and has edit permission.

### 81. System Lock Layer Sets System Locked
`TEST_F(LayerLockingTest, SystemLockLayer_SetsSystemLocked)`
**Where:** In the layer tree, a system-locked row displays a distinct lock indicator (different icon or coloring from a user lock) applied by external pipeline tooling.
**Context:** Verifies that the system-lock variant of locking is tracked separately from a regular user lock. System locks are applied by external tooling (e.g. pipeline or DCC), so QA must confirm they are distinguishable from manual locks.
**Pre-conditions:** Fresh layer.
**Action:** Call `lockLayer("", layer, LayerLock_SystemLocked, false)`.
**Expected result:** `isLayerSystemLocked(layer)` returns true.

### 82. System Lock Layer Revokes Permission To Edit
`TEST_F(LayerLockingTest, SystemLockLayer_RevokesPermissionToEdit)`
**Where:** In the layer tree, a system-locked row (distinct padlock indicator) cannot be set as the edit target, just as a user-locked row cannot.
**Context:** Verifies that a system lock also removes the USD edit permission, just like a regular lock. A system-locked layer that still allows edits would silently bypass pipeline write-protection.
**Pre-conditions:** Fresh layer.
**Action:** Apply system lock.
**Expected result:** `layer->PermissionToEdit()` returns false and `isLayerSystemLocked()` returns true.

### 83. Forget Locked Layers Clears All State
`TEST_F(LayerLockingTest, ForgetLockedLayers_ClearsAllState)`
**Where:** This API runs internally during scene close or stage reload. All padlock icons should disappear from every row in the tree as a result.
**Context:** Verifies that the "forget" API wipes the entire locked-layer registry in one call. This is used when a scene is closed or a stage is replaced — stale lock entries must not carry over to a new session.
**Pre-conditions:** Layer is locked.
**Action:** Call `forgetLockedLayers()`.
**Expected result:** `isLayerLocked(layer)` returns false.

### 84. Add Locked Layer Appears In Locked List
`TEST_F(LayerLockingTest, AddLockedLayer_AppearsInLockedList)`
**Where:** In the layer tree, a layer added directly to the locked registry (bypassing the lock command) should display a padlock icon on its row.
**Context:** Verifies the lower-level addLockedLayer helper, which registers a layer as locked without going through the full lockLayer workflow. This path is used when restoring lock state from saved scene data.
**Pre-conditions:** Fresh layer.
**Action:** Call `addLockedLayer(layer)`.
**Expected result:** `isLayerLocked(layer)` returns true.

### 85. Remove Locked Layer Disappears From Locked List
`TEST_F(LayerLockingTest, RemoveLockedLayer_DisappearsFromLockedList)`
**Where:** In the layer tree, removing a layer from the locked registry should cause its padlock icon to disappear from the row.
**Context:** Verifies the complementary removeLockedLayer helper. If removal does not work, layers that were unlocked via this path would still appear locked, blocking editing.
**Pre-conditions:** Layer in locked list.
**Action:** Call `removeLockedLayer(layer)`.
**Expected result:** `isLayerLocked(layer)` returns false.

### 86. Add System Locked Layer Appears In System Locked List
`TEST_F(LayerLockingTest, AddSystemLockedLayer_AppearsInSystemLockedList)`
**Where:** In the layer tree, registering a layer as system-locked should show a distinct system-lock indicator (not the regular user padlock) on that row.
**Context:** Verifies the low-level helper for registering a system lock, mirroring the addLockedLayer test for the system-lock list. Correct registration is required before any system-lock query or UI indicator can work.
**Pre-conditions:** Fresh layer.
**Action:** Call `addSystemLockedLayer(layer)`.
**Expected result:** `isLayerSystemLocked(layer)` returns true.

### 87. Forget System Locked Layers Clears System Locked List
`TEST_F(LayerLockingTest, ForgetSystemLockedLayers_ClearsSystemLockedList)`
**Where:** This API clears system-lock metadata during stage teardown or pipeline lock reload. All system-lock indicators should disappear from rows in the tree as a result.
**Context:** Verifies that the system-lock registry can be fully cleared, independent of the regular lock registry. This is needed when a stage closes or pipeline lock metadata is reloaded from scratch.
**Pre-conditions:** Layer in system locked list.
**Action:** Call `forgetSystemLockedLayers()`.
**Expected result:** `isLayerSystemLocked(layer)` returns false.

---

## testLayerMuting.cpp (8 Tests)

**Shared setup:** All tests use `::testing::Test` base class. Each test creates a stage and layer. Setup clears muted layer list. Teardown unmutes the layer and clears the list.

### 88. Is Muted False By Default
`TEST_F(LayerMutingTest, IsMuted_FalseByDefault)`
**Where:** In the layer tree, a freshly created sublayer row appears at full brightness with no mute icon, confirming it starts unmuted.
**Context:** Verifies the baseline mute state of a freshly created sublayer. QA needs to know that layers start unmuted so that muting-related bugs are not masked by incorrect initial state.
**Pre-conditions:** Fresh layer in stage.
**Action:** Call `stage->IsLayerMuted(layer->GetIdentifier())`.
**Expected result:** Returns false.

### 89. Mute Layer Sets Layer As Muted In Stage
`TEST_F(LayerMutingTest, MuteLayer_SetsLayerAsMutedInStage)`
**Where:** In the layer tree, after muting a sublayer (via context menu > "Mute Layer" or the mute icon button on the row), that row should appear visually dimmed.
**Context:** Confirms that muting a layer marks it as muted at the USD stage level. A muted layer is excluded from composition, so QA needs to verify the mute operation takes effect immediately.
**Pre-conditions:** Fresh layer.
**Action:** Call `stage->MuteLayer(layer->GetIdentifier())`.
**Expected result:** `stage->IsLayerMuted()` returns true.

### 90. Unmute Layer Sets Layer As Unmuted
`TEST_F(LayerMutingTest, UnmuteLayer_SetsLayerAsUnmuted)`
**Where:** In the layer tree, after unmuting a dimmed row (via context menu > "Unmute Layer" or clicking the active mute icon), the row should return to its normal undimmed appearance.
**Context:** Confirms that unmuting reverses the effect of a mute operation. QA needs assurance that the unmute path works correctly so users can restore a layer to active composition.
**Pre-conditions:** Layer muted.
**Action:** Call `stage->UnmuteLayer(layer->GetIdentifier())`.
**Expected result:** `stage->IsLayerMuted()` returns false.

### 91. Mute Toggle Roundtrip Restores Original State
`TEST_F(LayerMutingTest, MuteToggleRoundtrip_RestoresOriginalState)`
**Where:** In the layer tree, clicking the mute icon on a row and clicking it again should return the row to its original fully visible state with no dimming.
**Context:** Verifies that muting then unmuting a layer leaves it in the same state as before. This guards against cumulative side effects that could silently corrupt mute state across multiple operations.
**Pre-conditions:** Unmuted layer.
**Action:** Mute then unmute.
**Expected result:** Layer is unmuted.

### 92. Add Muted Layer Appears In Retained List
`TEST_F(LayerMutingTest, AddMutedLayer_AppearsInRetainedList)`
**Where:** In the layer tree, a muted layer remains visible as a dimmed row rather than disappearing from the list. The editor holds an internal reference to prevent USD from garbage-collecting it.
**Context:** Verifies that the editor's internal reference-retention mechanism (`addMutedLayer`) does not crash when called. USD may garbage-collect layers with no remaining references, so the editor must hold onto muted layers explicitly.
**Pre-conditions:** Fresh layer.
**Action:** Call `addMutedLayer(layer)` (this retains reference).
**Expected result:** Test completes without crash.

### 93. Remove Muted Layer Does Not Crash
`TEST_F(LayerMutingTest, RemoveMutedLayer_DoesNotCrash)`
**Where:** In the layer tree, when a muted layer is deleted or unmuted, the editor releases its internal reference. The row should be removed or updated cleanly with no crash.
**Context:** Verifies that removing a layer from the editor's retention list is safe. QA needs to confirm that cleanup paths (e.g. when a layer is deleted or unmuted) do not introduce crashes or memory errors.
**Pre-conditions:** Layer added to muted list.
**Action:** Call `removeMutedLayer(layer)`.
**Expected result:** No exception is thrown.

### 94. Forget Muted Layers Clears Retained List
`TEST_F(LayerMutingTest, ForgetMutedLayers_ClearsRetainedList)`
**Where:** This API runs internally during stage teardown. All dimmed (muted) rows should be cleaned up and the editor's internal muted-layer reference list is cleared.
**Context:** Verifies that bulk-clearing all retained muted layer references completes without error. This operation is used during stage teardown or full reset, so a crash here would affect every stage-close scenario.
**Pre-conditions:** Layer in muted list.
**Action:** Call `forgetMutedLayers()`.
**Expected result:** No exception is thrown.

### 95. Add Muted Layer Preserves Layer Reference
`TEST_F(LayerMutingTest, AddMutedLayer_PreservesLayerReference)`
**Where:** In the layer tree, a dimmed (muted) row remains identifiable by its layer path or generated name — the editor's reference retention ensures the row does not show a blank or corrupted label.
**Context:** Confirms that after adding a layer to the retention list, the layer object remains valid and its identifier is accessible. This guards against the retention mechanism accidentally releasing the layer it is meant to keep alive.
**Pre-conditions:** Fresh layer.
**Action:** Call `addMutedLayer(layer)` and retrieve identifier.
**Expected result:** Identifier is not empty.

---

## testLayerTreeItem.cpp (33 Tests)

**Shared setup:** All tests use `LayerEditorTestFixture`. Teardown clears lock state. Tests use `itemAt()` helper to cast tree items. Tests use indices provided by fixture methods.

### 96. Is Muted Returns False By Default
`TEST_F(LayerTreeItemTest, IsMuted_ReturnsFalseByDefault)`
**Where:** In the layer tree, look at a sublayer row (indented under the root). It should appear at full brightness with no mute icon, confirming it is unmuted.
**Context:** Verifies the baseline mute state of a sublayer item. A QA engineer needs to confirm that layers start unmuted so the layer editor does not incorrectly show muted indicators when nothing has been done.
**Pre-conditions:** First sublayer item in tree.
**Action:** Call `item->isMuted()`.
**Expected result:** Returns false.

### 97. Is Muted Returns True After Stage Mute
`TEST_F(LayerTreeItemTest, IsMuted_ReturnsTrueAfterStageMute)`
**Where:** In the layer tree, after muting a sublayer, the row appears visually dimmed and the mute icon button on the right side of the row shows its active (toggled) state.
**Context:** Confirms that muting a layer at the USD stage level is correctly reflected in the tree item. This is the primary way layers become muted, so the tree item must stay in sync.
**Pre-conditions:** First sublayer item.
**Action:** Mute layer at stage level, process events, check isMuted.
**Expected result:** Returns true. Unmute cleanup done.

### 98. Appears Muted False When Neither Self Nor Parent Muted
`TEST_F(LayerTreeItemTest, AppearsMuted_FalseWhenNeitherSelfNorParentMuted)`
**Where:** In the layer tree, a row with no dimming applied (neither it nor its parent is muted) should display at full brightness with no mute styling.
**Context:** Verifies that `appearsMuted` (the visual state affecting display) is false when no muting has occurred in the hierarchy. This baseline check ensures no false muted styling is shown.
**Pre-conditions:** First sublayer item (normal state).
**Action:** Call `item->appearsMuted()`.
**Expected result:** Returns false.

### 99. Appears Muted True When Self Is Muted
`TEST_F(LayerTreeItemTest, AppearsMuted_TrueWhenSelfIsMuted)`
**Where:** In the layer tree, a row that has been directly muted appears dimmed and may show a mute icon, indicating its opinions are excluded from the composed stage.
**Context:** Confirms that a directly muted layer reports itself as visually muted. The UI relies on `appearsMuted` to dim or style rows, so this must be true when the layer itself is muted.
**Pre-conditions:** First sublayer item.
**Action:** Mute layer at stage level, process events, check appearsMuted.
**Expected result:** Returns true. Unmute cleanup done.

### 100. Is Read Only False For Normal Sublayer
`TEST_F(LayerTreeItemTest, IsReadOnly_FalseForNormalSublayer)`
**Where:** In the layer tree, right-click a regular sublayer row (no lock or special icons). The context menu should offer editing options without any greyed-out restrictions.
**Context:** Verifies that a regular sublayer is not flagged as read-only. Read-only status blocks editing and certain context menu actions, so a plain sublayer must not incorrectly carry that restriction.
**Pre-conditions:** First sublayer item.
**Action:** Call `item->isReadOnly()`.
**Expected result:** Returns false.

### 101. Is Dirty False For Clean Layer
`TEST_F(LayerTreeItemTest, IsDirty_FalseForCleanLayer)`
**Where:** In the layer tree, a sublayer row that has not been edited shows no asterisk or unsaved indicator next to its name label.
**Context:** Establishes the baseline dirty state for a layer that has not been modified. The dirty indicator in the layer editor helps artists track unsaved work, so a fresh layer must not falsely appear dirty.
**Pre-conditions:** First sublayer item (fresh, unmodified).
**Action:** Call `item->isDirty()`.
**Expected result:** Returns false.

### 102. Is Dirty True After Layer Modified
`TEST_F(LayerTreeItemTest, IsDirty_TrueAfterLayerModified)`
**Where:** In the layer tree, after authoring any change to a layer, its row shows an asterisk or unsaved indicator next to the name label, signaling that a save may be needed.
**Context:** Confirms that modifying a layer (even just setting a comment) causes the tree item to report it as dirty. Artists rely on dirty indicators to know which layers need saving.
**Pre-conditions:** First sublayer item.
**Action:** Set layer comment to mark it modified. Call `isDirty()`.
**Expected result:** Returns true.

### 103. Needs Saving False For Session Layer
`TEST_F(LayerTreeItemTest, NeedsSaving_FalseForSessionLayer)`
**Where:** In the layer tree, the topmost "Session Layer" row — even if it shows unsaved content — never displays a save-needed indicator, since Maya manages it separately.
**Context:** Verifies that the session layer is never flagged as needing saving, even when dirty. The session layer is managed by the DCC application (Maya), not by the layer editor's save workflow.
**Pre-conditions:** Session layer item with content.
**Action:** Set layer comment and call `needsSaving()`.
**Expected result:** Returns false (session layers are not counted for saving).

### 104. Is Locked False By Default
`TEST_F(LayerTreeItemTest, IsLocked_FalseByDefault)`
**Where:** In the layer tree, a freshly created sublayer row shows no padlock icon on the right side, confirming it is editable.
**Context:** Establishes that a sublayer starts unlocked. Lock state controls whether editing is permitted, so a false baseline ensures the layer editor does not incorrectly block edits on a fresh layer.
**Pre-conditions:** First sublayer item.
**Action:** Call `item->isLocked()`.
**Expected result:** Returns false.

### 105. Is Locked True When Permission To Edit Revoked
`TEST_F(LayerTreeItemTest, IsLocked_TrueWhenPermissionToEditRevoked)`
**Where:** In the layer tree, after locking a layer, a padlock icon appears on the right side of that row to indicate editing is blocked.
**Context:** Confirms that revoking a layer's edit permission is reflected as locked in the tree item. Locked layers must be visually indicated so artists know they cannot modify them.
**Pre-conditions:** First sublayer item.
**Action:** Lock layer via TestUtils, call `isLocked()`.
**Expected result:** Returns true. Cleanup unlocks layer.

### 106. Appears Locked False For Root Item With Unlocked Self
`TEST_F(LayerTreeItemTest, AppearsLocked_FalseForRootItemWithUnlockedSelf)`
**Where:** In the layer tree, the root layer row (labeled with the scene file name) shows no lock icon or greyed-out styling when it and its ancestors are all unlocked.
**Context:** Verifies that the root layer does not appear locked when nothing has been locked. `appearsLocked` drives visual inheritance in the tree; a false baseline prevents spurious lock icons on the root.
**Pre-conditions:** Root layer item.
**Action:** Call `item->appearsLocked()`.
**Expected result:** Returns false.

### 107. Appears Locked True When Parent Is Locked
`TEST_F(LayerTreeItemTest, AppearsLocked_TrueWhenParentIsLocked)`
**Where:** In the layer tree, when the root layer row shows a padlock icon, all indented child sublayer rows beneath it also appear locked (greyed out or with an inherited lock indicator).
**Context:** Confirms that locking a parent layer causes child items to appear locked. Children of a locked parent are implicitly restricted, and the UI must reflect this so artists understand why child layers are read-only.
**Pre-conditions:** Root layer locked. First sublayer item (child of root).
**Action:** Call `item->appearsLocked()`.
**Expected result:** Returns true (child inherits parent lock). Cleanup unlocks parent.

### 108. Appears Locked Does Not Check Self
`TEST_F(LayerTreeItemTest, AppearsLocked_DoesNotCheckSelf)`
**Where:** In the layer tree, a sublayer that has its own lock does not cause itself to show the inherited-lock appearance — only parent locks propagate visually downward.
**Context:** Verifies that `appearsLocked` only propagates lock state downward from ancestors, never from the item itself. This is the intended design so that `isLocked` and `appearsLocked` serve distinct purposes in the UI.
**Pre-conditions:** First sublayer item locked directly.
**Action:** Call `item->appearsLocked()`.
**Expected result:** Returns false (self lock does not count, only parent propagation). Cleanup unlocks.

### 109. Is System Locked False By Default
`TEST_F(LayerTreeItemTest, IsSystemLocked_FalseByDefault)`
**Where:** In the layer tree, a fresh sublayer row shows no special system-lock indicator; system locks are applied externally by pipeline tools.
**Context:** Establishes the baseline system-lock state. System locks are applied externally (e.g., by pipeline tools) and must not appear unless explicitly set, so a fresh layer must report false.
**Pre-conditions:** First sublayer item.
**Action:** Call `item->isSystemLocked()`.
**Expected result:** Returns false.

### 110. Is System Locked True After System Lock Applied
`TEST_F(LayerTreeItemTest, IsSystemLocked_TrueAfterSystemLockApplied)`
**Where:** In the layer tree, a system-locked row displays a distinct lock indicator (different from the regular user padlock) to signal it is controlled by an external authority.
**Context:** Confirms that registering a layer as system-locked and revoking edit permission is correctly detected by the tree item. System-locked layers require a distinct visual treatment in the editor to signal they are controlled externally.
**Pre-conditions:** First sublayer item.
**Action:** Add system lock, set no-edit permission, call `isSystemLocked()`.
**Expected result:** Returns true. Cleanup removes lock.

### 111. Appears System Locked False When Parent Not System Locked
`TEST_F(LayerTreeItemTest, AppearsSystemLocked_FalseWhenParentNotSystemLocked)`
**Where:** In the layer tree, look at a sublayer row whose parent has no system-lock icon. That sublayer row should show no system-lock styling.
**Context:** Verifies the baseline for inherited system-lock display. Like `appearsLocked`, `appearsSystemLocked` propagates from ancestors; with no system lock in the hierarchy the item must return false.
**Pre-conditions:** First sublayer item.
**Action:** Call `item->appearsSystemLocked()`.
**Expected result:** Returns false.

### 112. Is Movable False For Session Layer
`TEST_F(LayerTreeItemTest, IsMovable_FalseForSessionLayer)`
**Where:** In the layer tree, try to drag the topmost "Session Layer" row. It should not respond to drag gestures and cannot be repositioned in the tree.
**Context:** Confirms that the session layer cannot be dragged and reordered. It occupies a fixed, special-purpose position in the layer stack and must never be relocated by the user.
**Pre-conditions:** Session layer item.
**Action:** Call `item->isMovable()`.
**Expected result:** Returns false.

### 113. Is Movable False For Root Layer
`TEST_F(LayerTreeItemTest, IsMovable_FalseForRootLayer)`
**Where:** In the layer tree, try to drag the root layer row. It should not respond to drag gestures and cannot be repositioned.
**Context:** Confirms that the root layer cannot be moved. The root layer anchors the entire layer stack and has no valid drag destination, so it must be excluded from drag-and-drop.
**Pre-conditions:** Root layer item.
**Action:** Call `item->isMovable()`.
**Expected result:** Returns false.

### 114. Is Movable True For Normal Sublayer
`TEST_F(LayerTreeItemTest, IsMovable_TrueForNormalSublayer)`
**Where:** In the layer tree, a regular sublayer row can be dragged (a drag cursor appears) and dropped to a new position within the same parent or under another parent.
**Context:** Verifies that a regular sublayer can be reordered via drag-and-drop. Layer ordering affects opinion strength in USD composition, so artists must be able to rearrange sublayers freely.
**Pre-conditions:** First sublayer item (normal, unlocked, unmuted).
**Action:** Call `item->isMovable()`.
**Expected result:** Returns true.

### 115. Is Movable False When Locked
`TEST_F(LayerTreeItemTest, IsMovable_FalseWhenLocked)`
**Where:** In the layer tree, a sublayer row displaying a padlock icon cannot be dragged; attempting to drag it should have no effect.
**Context:** Confirms that a directly locked sublayer cannot be moved. Allowing drag-and-drop on a locked layer would contradict the intent of locking it, so the move must be blocked.
**Pre-conditions:** First sublayer item.
**Action:** Lock layer, call `isMovable()`.
**Expected result:** Returns false. Cleanup unlocks.

### 116. Is Movable False When Appears Locked
`TEST_F(LayerTreeItemTest, IsMovable_FalseWhenAppearsLocked)`
**Where:** In the layer tree, a sublayer row whose parent shows a padlock icon also cannot be dragged, even though the sublayer itself may not display its own padlock.
**Context:** Confirms that a sublayer whose parent is locked also cannot be moved. Because the parent is locked, the child's position in the stack is implicitly protected, and the drag should be blocked.
**Pre-conditions:** Root layer locked. First sublayer item.
**Action:** Call `item->isMovable()`.
**Expected result:** Returns false (parent lock blocks move). Cleanup unlocks parent.

### 117. Is Movable False When Muted
`TEST_F(LayerTreeItemTest, IsMovable_FalseWhenMuted)`
**Where:** In the layer tree, a dimmed (muted) sublayer row cannot be dragged for reordering; attempting to drag it should have no effect.
**Context:** Confirms that a muted sublayer cannot be dragged. Muted layers are excluded from composition, and moving them could confuse the layer stack ordering; the move must be blocked for consistency.
**Pre-conditions:** First sublayer item.
**Action:** Mute layer at stage level, process events, call `isMovable()`.
**Expected result:** Returns false. Cleanup unmutes.

### 118. Is Target Layer True For Current Edit Target
`TEST_F(LayerTreeItemTest, IsTargetLayer_TrueForCurrentEditTarget)`
**Where:** In the layer tree, the row with the edit-target indicator (a pencil or arrow icon on the left side) is the active edit layer where new USD scene data will be authored.
**Context:** Verifies that the tree item correctly identifies when its layer is the active edit target. The edit target indicator in the UI tells artists which layer new opinions will be written to, so this must be accurate.
**Pre-conditions:** Root layer item (default edit target).
**Action:** Call `item->isTargetLayer()`.
**Expected result:** Returns true.

### 119. Has Sub Layers True When Sublayers Exist
`TEST_F(LayerTreeItemTest, HasSubLayers_TrueWhenSublayersExist)`
**Where:** In the layer tree, a row that has child layers shows an expand/collapse arrow on its left side. Click the arrow to reveal the nested sublayer rows beneath it.
**Context:** Confirms that the root layer reports having sublayers when at least one exists. This drives whether the expand arrow is shown in the tree, so it must be true whenever children are present.
**Pre-conditions:** Root layer item (has one sublayer by default in stub).
**Action:** Call `item->hasSubLayers()`.
**Expected result:** Returns true.

### 120. Has Sub Layers False For Leaf Sublayer
`TEST_F(LayerTreeItemTest, HasSubLayers_FalseForLeafSublayer)`
**Where:** In the layer tree, a sublayer row with no children shows no expand arrow on its left side, confirming it is a leaf node.
**Context:** Verifies that a leaf sublayer with no children correctly reports no sublayers. This prevents the tree from rendering a spurious expand arrow on leaf nodes.
**Pre-conditions:** First sublayer item (no children).
**Action:** Call `item->hasSubLayers()`.
**Expected result:** Returns false.

### 121. Is Anonymous True For Anonymous Layer
`TEST_F(LayerTreeItemTest, IsAnonymous_TrueForAnonymousLayer)`
**Where:** In the layer tree, an anonymous (in-memory) layer row displays a generated name (e.g., "anon:…" or a default label) rather than a file path. Saving it will prompt for a file location.
**Context:** Confirms that a layer without a file path is correctly identified as anonymous. Anonymous layers require special handling at save time (e.g., prompting for a file path), so this flag must be correct.
**Pre-conditions:** First sublayer item (stub creates anonymous sublayers).
**Action:** Call `item->isAnonymous()`.
**Expected result:** Returns true.

### 122. Get Action Button Lock Checked Matches Is Locked
`TEST_F(LayerTreeItemTest, GetActionButton_LockCheckedMatchesIsLocked)`
**Where:** In the layer tree, the lock icon button on the right side of a locked row should appear in its active/toggled state (e.g., a filled padlock vs. an outline padlock).
**Context:** Verifies that the Lock action button's checked state matches the layer's actual lock state. The action button drives the lock toggle icon in each row; if it misreports, the icon will show the wrong state to artists.
**Pre-conditions:** First sublayer item.
**Action:** Lock layer, call `getActionButton(LayerActionType::Lock, info)`.
**Expected result:** `info._checked` is true. Cleanup unlocks.

### 123. Get Action Button Mute Checked Matches Is Muted
`TEST_F(LayerTreeItemTest, GetActionButton_MuteCheckedMatchesIsMuted)`
**Where:** In the layer tree, the mute icon button on the right side of a muted (dimmed) row should appear in its active/toggled state.
**Context:** Verifies that the Mute action button's checked state matches the layer's actual mute state. The mute toggle icon in each row must accurately reflect whether the layer is excluded from composition.
**Pre-conditions:** First sublayer item.
**Action:** Mute layer at stage level, process events, call `getActionButton(LayerActionType::Mute, info)`.
**Expected result:** `info._checked` is true. Cleanup unmutes.

### 124. Action Buttons Mute Applies To Sublayer Only
`TEST_F(LayerTreeItemTest, ActionButtons_MuteAppliesToSublayerOnly)`
**Where:** In the layer tree, the mute icon button appears only on indented sublayer rows, not on the root layer row. The root layer itself cannot be muted.
**Context:** Confirms that the Mute action is only available for sublayers, not the root layer. USD stage-level muting applies to sublayers; offering mute on the root would be incorrect and potentially confusing.
**Pre-conditions:** Query static action buttons definition.
**Action:** Look up Mute action and check layer masks.
**Expected result:** Allowed for SubLayer, not for Root.

### 125. Action Buttons Lock Applies To Root And Sublayer
`TEST_F(LayerTreeItemTest, ActionButtons_LockAppliesToRootAndSublayer)`
**Where:** In the layer tree, the lock icon button appears on both the root layer row and all sublayer rows, since both types can be locked.
**Context:** Confirms that the Lock action is offered for both the root layer and sublayers. Both layer types can be locked to prevent edits, and the toolbar must expose the lock button for either selection.
**Pre-conditions:** Query static action buttons definition.
**Action:** Look up Lock action and check layer masks.
**Expected result:** Allowed for both Root and SubLayer.

### 126. Is Identical Item Null Other Returns False
`TEST_F(LayerTreeItemTest, IsIdenticalItem_NullOtherReturnsFalse)`
**Where:** This is an internal tree-item identity check used during model rebuilds. No direct visual component in the UI.
**Context:** Verifies safe handling of a null pointer in the identity comparison. This guards against crashes when the layer editor compares items during tree refresh or selection changes.
**Pre-conditions:** First sublayer item.
**Action:** Call `item->isIdenticalItem(nullptr)`.
**Expected result:** Returns false.

### 127. Is Identical Item Same Pointer Returns True
`TEST_F(LayerTreeItemTest, IsIdenticalItem_SamePointerReturnsTrue)`
**Where:** This is an internal comparison used during tree refreshes to decide whether to reuse existing rows. No direct visual component.
**Context:** Confirms that an item is considered identical to itself. The identity check is used during tree rebuilds to determine whether items can be reused rather than replaced, so reflexive equality must hold.
**Pre-conditions:** First sublayer item.
**Action:** Call `item->isIdenticalItem(item)`.
**Expected result:** Returns true.

### 128. Is Identical Item Different Layer Returns False
`TEST_F(LayerTreeItemTest, IsIdenticalItem_DifferentLayerReturnsFalse)`
**Where:** This is an internal comparison; when a row's underlying layer changes between rebuilds, the row is replaced with a fresh one to prevent stale display data.
**Context:** Confirms that two items wrapping different layers are not considered identical. This ensures the tree rebuild correctly detects when an item's underlying layer has changed and replaces it rather than reusing stale state.
**Pre-conditions:** Root layer item and first sublayer item (different layers).
**Action:** Call `root->isIdenticalItem(sub)`.
**Expected result:** Returns false.

---

## testLayerTreeModel.cpp (20 Tests)

**Shared setup:** All tests use `LayerEditorTestFixture`.

### 129. Flags Drag Enabled Only For Movable Items
`TEST_F(LayerTreeModelTest, Flags_DragEnabledOnlyForMovableItems)`
**Where:** In the layer tree, only indented sublayer rows respond to drag gestures. Root and session layer rows should not show a drag cursor when hovered.
**Context:** The tree model controls which layers can be dragged for reordering. Only layers that are movable (sublayers) should have drag enabled — root and session layers must not be draggable, preventing accidental structural changes.
**Pre-conditions:** Tree model with session layer, root, and sublayers.
**Action:** Query flags for first sublayer and root layer.
**Expected result:** Sublayer has ItemIsDragEnabled, root does not.

### 130. Flags Drop Always Enabled
`TEST_F(LayerTreeModelTest, Flags_DropAlwaysEnabled)`
**Where:** In the layer tree, any row can be a drop target when dragging a sublayer. A visual drop indicator should appear on any row as you hover over it during a drag.
**Context:** Any layer row can be a drop target so that dragged layers can be reinserted anywhere in the stack. This ensures the user can always choose where a moved layer lands.
**Pre-conditions:** Tree model.
**Action:** Query flags for root and first sublayer.
**Expected result:** Both have ItemIsDropEnabled.

### 131. Supported Drop Actions Only Move Action
`TEST_F(LayerTreeModelTest, SupportedDropActions_OnlyMoveAction)`
**Where:** In the layer tree, dragging a sublayer row always moves it (the original position becomes empty). Copy-dragging (e.g., holding a modifier key) is not supported.
**Context:** The layer tree only supports moving layers, not copying them. Accepting only MoveAction prevents accidental duplication of layers when the user drags within the tree.
**Pre-conditions:** Tree model.
**Action:** Call `supportedDropActions()`.
**Expected result:** Returns `Qt::MoveAction`.

### 132. Mime Types Returns Text Plain
`TEST_F(LayerTreeModelTest, MimeTypes_ReturnsTextPlain)`
**Where:** This is an internal data-serialization test for drag-and-drop. When a row is dragged, its identifier is encoded as plain text in the drag payload.
**Context:** The drag-and-drop protocol requires a known MIME type so the model can validate drops and reject foreign data. Using "text/plain" keeps the format simple and predictable.
**Pre-conditions:** Tree model.
**Action:** Call `mimeTypes()`.
**Expected result:** Returns list with one entry: "text/plain".

### 133. Mime Data Serializes Identifiers With Semicolon
`TEST_F(LayerTreeModelTest, MimeData_SerializesIdentifiersWithSemicolon)`
**Where:** This is an internal serialization test. The dragged layer's identifier embedded in the drag payload is what allows the model to identify which row was moved on drop.
**Context:** When a layer is dragged, its identity must be encoded in the MIME payload so the drop handler knows which layer to move. This verifies the serialized data contains the correct layer identifier.
**Pre-conditions:** Tree model.
**Action:** Call `mimeData({firstSublayerIndex()})`.
**Expected result:** MIME data contains the layer identifier.

### 134. Can Drop Returns False For Null Mime Data
`TEST_F(LayerTreeModelTest, CanDrop_ReturnsFalseForNullMimeData)`
**Where:** In the layer tree, dropping data from an external source (e.g., a file from the OS file browser) onto a row should be silently rejected with no visual drop indicator.
**Context:** The model must safely reject drop events that carry no data, such as drags originating from outside the layer editor. A null check guards against crashes during drag-and-drop.
**Pre-conditions:** Tree model.
**Action:** Call `canDropMimeData(nullptr, Qt::MoveAction, 0, 0, rootLayerIndex())`.
**Expected result:** Returns false.

### 135. Rebuild Always Shows Session Layer When Auto Hide False
`TEST_F(LayerTreeModelTest, Rebuild_AlwaysShowsSessionLayerWhenAutoHideFalse)`
**Where:** In the layer tree, after any model rebuild (triggered by a stage reload or USD notice), the "Session Layer" row should remain at the top of the list.
**Context:** When the session layer is not set to auto-hide, it must appear as the first item after every model rebuild. This ensures the session layer remains consistently visible for user interaction.
**Pre-conditions:** Tree model with autoHideSessionLayer = false (stub default).
**Action:** Call `forceRefresh()` and process events. Get first top-level item.
**Expected result:** Item is session layer.

### 136. Rebuild Clears And Repopulates Rows
`TEST_F(LayerTreeModelTest, Rebuild_ClearsAndRepopulatesRows)`
**Where:** In the layer tree, after a stage reload or forced refresh, all layer rows should reappear correctly with no rows missing or duplicated.
**Context:** A full model rebuild must produce the same layer tree that existed before, with no rows lost or duplicated. This guards against regressions where a rebuild leaves the tree in a broken state.
**Pre-conditions:** Tree model with known row count.
**Action:** Call `forceRefresh()` and process events.
**Expected result:** Row count matches before rebuild.

### 137. Rebuild On Idle Deduplicates Scheduling
`TEST_F(LayerTreeModelTest, RebuildOnIdle_DeduplicatesScheduling)`
**Where:** In the layer tree, rapid stage notifications should not cause multiple visible flicker/redraw cycles. The tree should settle after at most one visible refresh.
**Context:** Rapid successive refresh requests (e.g. from multiple USD change notices) should collapse into a single rebuild to avoid unnecessary UI flicker. This verifies the pending-rebuild guard works correctly.
**Pre-conditions:** Tree model with reset signal connection to count resets.
**Action:** Call `forceRefresh()` twice, then process events.
**Expected result:** At most 2 modelReset signals (deduplicates calls but USD notices may trigger rebuild).

### 138. Rebuild Skips Reset When Layers Are Identical
`TEST_F(LayerTreeModelTest, Rebuild_SkipsResetWhenLayersAreIdentical)`
**Where:** In the layer tree, if the layer structure has not changed, the expand/collapse state of rows should be preserved after an internal refresh (no visible flicker or collapse to defaults).
**Context:** If the layer structure has not changed, emitting modelReset forces the tree view to redraw and lose expand/scroll state unnecessarily. This test guards the optimization that skips the reset when nothing changed.
**Pre-conditions:** Tree model. Reset signal connected to counter.
**Action:** Process events to settle, then call `forceRefresh()` again with no changes.
**Expected result:** No modelReset signal fires.

### 139. Get All Needs Saving Layers Empty When No Layers Are Dirty And Shared
`TEST_F(LayerTreeModelTest, GetAllNeedsSavingLayers_EmptyWhenNoLayersAreDirtyAndShared)`
**Where:** In the Layer Editor toolbar, the "Save all edits" button and the Save Layers dialog show no layers to save when the stage is non-shared.
**Context:** The "save all" workflow should only present layers that actually need saving. On a non-shared stage no layers require saving through the editor, so the list must be empty to avoid prompting the user unnecessarily.
**Pre-conditions:** Stub stage (non-shared by default).
**Action:** Call `getAllNeedsSavingLayers()`.
**Expected result:** Returns empty list.

### 140. Get All Anonymous Layers Excludes Session Layer
`TEST_F(LayerTreeModelTest, GetAllAnonymousLayers_ExcludesSessionLayer)`
**Where:** In the Save Layers dialog (opened from the "Save all edits" toolbar button), the "Session Layer" row should never appear in the list of layers to be saved.
**Context:** Anonymous layer queries are used to find layers that need names before saving. The session layer is managed by the DCC host and must never appear in this list, as it is not saved through the layer editor.
**Pre-conditions:** Tree model.
**Action:** Call `getAllAnonymousLayers()`.
**Expected result:** No item in list is a session layer.

### 141. Get All Anonymous Layers Includes Anonymous Sublayers
`TEST_F(LayerTreeModelTest, GetAllAnonymousLayers_IncludesAnonymousSublayers)`
**Where:** In the Save Layers dialog, in-memory (anonymous) sublayer rows appear as entries requiring a file path before they can be saved.
**Context:** Anonymous sublayers are the primary target of the "find layers to name/save" feature. This confirms the query correctly surfaces them so they are not silently skipped during a save operation.
**Pre-conditions:** Stub stage with anonymous sublayers.
**Action:** Call `getAllAnonymousLayers()`.
**Expected result:** List contains at least one item.

### 142. Find Name For New Anonymous Layer Returns Non Empty String
`TEST_F(LayerTreeModelTest, FindNameForNewAnonymousLayer_ReturnsNonEmptyString)`
**Where:** In the layer tree, a newly added anonymous layer row displays a generated name label (e.g., "Layer1") rather than a blank or empty label.
**Context:** When adding a new anonymous layer the editor must generate a display name for it. A blank name would create an unnamed, unidentifiable layer in the UI, confusing users.
**Pre-conditions:** Tree model.
**Action:** Call `findNameForNewAnonymousLayer()`.
**Expected result:** Returns non-empty string.

### 143. Find Name For New Anonymous Layer Does Not Collide With Existing
`TEST_F(LayerTreeModelTest, FindNameForNewAnonymousLayer_DoesNotCollideWithExisting)`
**Where:** In the layer tree, adding a second anonymous layer produces a different generated name (e.g., "Layer2") so both rows have distinct, distinguishable labels.
**Context:** If two anonymous layers receive the same generated name, users cannot distinguish them. This test ensures the naming function increments or varies its output when a collision would otherwise occur.
**Pre-conditions:** Tree model.
**Action:** Get name, create and add layer with that name, get name again.
**Expected result:** Two names are different.

### 144. Set Edit Target Calls Hook For Accessible Layer
`TEST_F(LayerTreeModelTest, SetEditTarget_CallsHookForAccessibleLayer)`
**Where:** In the layer tree, double-click a normal unlocked sublayer row to make it the edit target. The edit-target indicator (pencil/arrow icon) should move to that row.
**Context:** Selecting a layer as the edit target routes all USD edits to that layer. This verifies the command hook is invoked for a normal, unlocked, unmuted layer so that edits are correctly redirected.
**Pre-conditions:** First sublayer item (normal state).
**Action:** Clear hook history and call `treeModel()->setEditTarget(item)`.
**Expected result:** `setEditTarget` is called.

### 145. Set Edit Target Blocked When Layer Is Locked
`TEST_F(LayerTreeModelTest, SetEditTarget_BlockedWhenLayerIsLocked)`
**Where:** In the layer tree, trying to double-click a locked row (padlock icon visible) to set it as the edit target should have no effect — the edit-target indicator should not move.
**Context:** Writing to a locked layer would corrupt the scene or silently fail. The model must refuse to set a locked layer as the edit target and must not invoke the command hook in this case.
**Pre-conditions:** First sublayer item locked.
**Action:** Clear hook history and call `setEditTarget(item)`.
**Expected result:** `setEditTarget` is not called. Cleanup unlocks.

### 146. Set Edit Target Blocked When Layer Is Muted
`TEST_F(LayerTreeModelTest, SetEditTarget_BlockedWhenLayerIsMuted)`
**Where:** In the layer tree, trying to double-click a dimmed (muted) row to set it as the edit target should have no effect — the edit-target indicator should not move.
**Context:** A muted layer is excluded from stage composition and cannot receive meaningful edits. Blocking the edit-target switch prevents user confusion when a muted layer appears selected but edits would have no effect.
**Pre-conditions:** First sublayer item muted at stage.
**Action:** Clear hook history and call `setEditTarget(item)`.
**Expected result:** `setEditTarget` is not called. Cleanup unmutes.

### 147. Root Layer Index Is Valid
`TEST_F(LayerTreeModelTest, RootLayerIndex_IsValid)`
**Where:** In the layer tree, the root layer row (labeled with the scene file name) is always present and can be reliably retrieved by the editor.
**Context:** Many tree operations depend on being able to locate the root layer by its model index. An invalid index would cause crashes or silent failures whenever code navigates to the root layer.
**Pre-conditions:** Tree model.
**Action:** Call `rootLayerIndex()`.
**Expected result:** Index is valid.

### 148. Root Layer Index Item Is Root Layer
`TEST_F(LayerTreeModelTest, RootLayerIndex_ItemIsRootLayer)`
**Where:** In the layer tree, the row retrieved as the root layer correctly shows the scene's root layer name (not the session layer or a sublayer).
**Context:** Code that retrieves the root layer index must resolve to the actual root layer item, not a different row. This confirms the index and item are consistent, which is a prerequisite for all root-layer operations.
**Pre-conditions:** Tree model.
**Action:** Get item at `rootLayerIndex()`.
**Expected result:** Item's `isRootLayer()` returns true.

---

## testLayerTreeView.cpp (16 Tests)

**Shared setup:** All tests use `LayerEditorTestFixture`. Teardown clears lock state.

### 149. Memento Populated On Construction
`TEST_F(LayerTreeViewTest, Memento_PopulatedOnConstruction)`
**Where:** In the layer tree, the expand/collapse state of all rows is saved automatically when the view is created. After a stage reload, rows you had expanded should still be expanded.
**Context:** Verifies that the view's state-snapshot object (memento) immediately captures the tree's expand/collapse state when it is created, rather than starting empty. This matters because the memento is relied upon to survive model rebuilds without losing the user's view state.
**Pre-conditions:** LayerViewMemento created.
**Action:** Check `memento.empty()`.
**Expected result:** Returns false.

### 150. Memento Not Empty After Preserve
`TEST_F(LayerTreeViewTest, Memento_NotEmptyAfterPreserve)`
**Where:** In the layer tree, before each model rebuild the view saves (preserves) the expand/collapse state of every row. This ensures the tree does not collapse to its defaults after a stage change.
**Context:** Confirms that explicitly calling `preserve()` on an existing memento captures state and leaves it non-empty. A QA engineer needs confidence that calling preserve at any point produces a valid snapshot, not a blank one.
**Pre-conditions:** LayerViewMemento created and `preserve()` called.
**Action:** Check `memento.empty()`.
**Expected result:** Returns false.

### 151. Memento Preserves Expanded State By Identifier
`TEST_F(LayerTreeViewTest, Memento_PreservesExpandedStateByIdentifier)`
**Where:** In the layer tree, expand a parent row by clicking its left-side arrow. After the model rebuilds (e.g., due to a stage reload), that row should still be expanded.
**Context:** Checks that the memento records which layers were expanded in the tree, keyed by the layer's unique identifier. This is critical so the editor can restore the user's expanded/collapsed nodes after a model refresh.
**Pre-conditions:** Root layer expanded. Memento created and populated.
**Action:** Get items state and look up root layer identifier.
**Expected result:** Entry has `_expanded = true`.

### 152. Memento Restored After Model Reset
`TEST_F(LayerTreeViewTest, Memento_RestoredAfterModelReset)`
**Where:** In the layer tree, expand the root layer row to see its sublayers. After any stage reload or forced refresh, the root row should still be expanded.
**Context:** Ensures that when the tree model is rebuilt (e.g., after a stage change), the view automatically saves and restores the previous expand state so the user does not lose their tree navigation context.
**Pre-conditions:** Root layer expanded. Model rebuild via `forceRefresh()`.
**Action:** Check if root is expanded after rebuild.
**Expected result:** Root layer is still expanded.

### 153. Memento Restore Handles Missing Items Gracefully
`TEST_F(LayerTreeViewTest, Memento_RestoreHandlesMissingItemsGracefully)`
**Where:** In the layer tree, if a row that was previously expanded is removed from the stage, the subsequent rebuild should complete without freezing or crashing Maya.
**Context:** Validates that restoring a memento containing identifiers for layers no longer present in the tree does not crash. This protects against edge cases where layers are removed between a save and restore of the view state.
**Pre-conditions:** Memento with fake identifier added.
**Action:** Call `restore()`.
**Expected result:** No exception thrown.

### 154. Get Selected Layer Items Returns All Selected
`TEST_F(LayerTreeViewTest, GetSelectedLayerItems_ReturnsAllSelected)`
**Where:** In the layer tree, click a layer row to select it (it highlights). The selection drives all toolbar button and context menu operations.
**Context:** Verifies that the tree view correctly reports all currently selected layer items through its public API. Reliable selection reporting is the basis for all context-menu and toolbar operations that act on the current selection.
**Pre-conditions:** First sublayer selected.
**Action:** Call `layerTree()->getSelectedLayerItems()`.
**Expected result:** Returns list with one item.

### 155. Current Layer Item Returns Null For Invalid Index
`TEST_F(LayerTreeViewTest, CurrentLayerItem_ReturnsNullForInvalidIndex)`
**Where:** In the layer tree, clicking outside any row or pressing Escape to deselect all rows should result in no current item. Toolbar buttons that require a selection should become disabled.
**Context:** Confirms that querying the current item when nothing valid is selected returns null rather than a dangling pointer. Callers that act on the current item must handle a null result without crashing.
**Pre-conditions:** Invalid index set as current.
**Action:** Call `currentLayerItem()`.
**Expected result:** Returns nullptr.

### 156. Current Layer Item Returns Item For Valid Index
`TEST_F(LayerTreeViewTest, CurrentLayerItem_ReturnsItemForValidIndex)`
**Where:** In the layer tree, clicking a layer row makes it the current item (highlighted). Toolbar buttons and context menu operations act on this highlighted row.
**Context:** Verifies that selecting a valid row in the tree and then querying the current item returns a usable, non-null layer item. This is the normal code path for every operation that acts on the selected layer.
**Pre-conditions:** First sublayer selected.
**Action:** Call `currentLayerItem()`.
**Expected result:** Returns non-null item.

### 157. Mute Action Calls Mute Sub Layer On Selected Item
`TEST_F(LayerTreeViewTest, MuteAction_CallsMuteSubLayerOnSelectedItem)`
**Where:** In the layer tree, select a sublayer row, then click the mute icon button on the right side of that row (or right-click and choose "Mute Layer"). The row should become dimmed.
**Context:** Confirms that triggering the mute action from the layer editor window dispatches the correct command to the hook when a sublayer is selected. Ensures the UI-to-command wiring is intact for muting layers.
**Pre-conditions:** First sublayer selected.
**Action:** Clear hook history and call `_window->muteLayer()`.
**Expected result:** `muteSubLayer` is called.

### 158. Lock Action Calls Lock Layer On Selected Item
`TEST_F(LayerTreeViewTest, LockAction_CallsLockLayerOnSelectedItem)`
**Where:** In the layer tree, select a sublayer row, then click the lock icon button on the right side of that row (or right-click and choose "Lock Layer"). A padlock icon should appear.
**Context:** Confirms that triggering the lock action from the layer editor window dispatches the correct command to the hook when a sublayer is selected. Ensures the UI-to-command wiring is intact for locking layers.
**Pre-conditions:** First sublayer selected.
**Action:** Clear hook history and call `_window->lockLayer()`.
**Expected result:** `lockLayer` is called.

### 159. Delegate Target Icon Rect X Offset Is Arrow Area Width
`TEST_F(LayerTreeViewTest, Delegate_TargetIconRect_XOffsetIsArrowAreaWidth)`
**Where:** In the layer tree, the edit-target indicator icon (pencil or arrow) on the left side of the current target row is positioned just to the right of the expand/collapse arrow, not overlapping or hidden behind it.
**Context:** Verifies that the item delegate positions the edit-target icon to the right of the tree's expand/collapse arrow area, not at the leftmost edge. Correct horizontal placement prevents the icon from being hidden under the arrow control.
**Pre-conditions:** TestableDelegateWrapper created.
**Action:** Call `getTargetIconRect(itemRect)` with test rectangle.
**Expected result:** `targetRect.left()` is greater than `itemRect.left()`.

### 160. Delegate Target Icon Rect Has Positive Width
`TEST_F(LayerTreeViewTest, Delegate_TargetIconRect_HasPositiveWidth)`
**Where:** In the layer tree, the edit-target indicator icon is visibly rendered as a distinct icon; it is not invisible or zero-width when a stage has an active edit target.
**Context:** Ensures the edit-target icon rectangle computed by the delegate has a non-zero width, meaning the icon is actually renderable. A zero-width rect would make the icon invisible and the target indicator non-functional.
**Pre-conditions:** TestableDelegateWrapper created.
**Action:** Call `getTargetIconRect(itemRect)`.
**Expected result:** `targetRect.width()` is greater than zero.

### 161. Delegate Lock Info Checked When Layer Is Locked
`TEST_F(LayerTreeViewTest, Delegate_LockInfoChecked_WhenLayerIsLocked)`
**Where:** In the layer tree, the lock icon button on the right side of a locked row should appear in its active/toggled visual state (e.g., a filled padlock vs. an outline padlock).
**Context:** Verifies that when a layer is locked, the lock action button's checked state is true, so the UI renders the button in its active/pressed appearance. A mismatch here would mean the lock icon looks toggled off even when the layer is locked.
**Pre-conditions:** First sublayer locked. Item object.
**Action:** Call `getActionButton(LayerActionType::Lock, lockInfo)`.
**Expected result:** `lockInfo._checked` is true. Cleanup unlocks.

### 162. Delegate Mute Info Checked When Layer Is Muted
`TEST_F(LayerTreeViewTest, Delegate_MuteInfoChecked_WhenLayerIsMuted)`
**Where:** In the layer tree, the mute icon button on the right side of a muted (dimmed) row should appear in its active/toggled visual state.
**Context:** Verifies that when a layer is muted at the stage level, the mute action button's checked state is true. This ensures the mute icon visually reflects the actual muted state so users can see at a glance which layers are muted.
**Pre-conditions:** First sublayer muted at stage. Item object.
**Action:** Call `getActionButton(LayerActionType::Mute, muteInfo)`.
**Expected result:** `muteInfo._checked` is true. Cleanup unmutes.

### 163. Double Click Skips When Layer Does Not Need Saving
`TEST_F(LayerTreeViewTest, DoubleClick_SkipsWhenLayerDoesNotNeedSaving)`
**Where:** In the layer tree, double-clicking a layer row that is not dirty or anonymous should not open any save dialog. The interaction is silently ignored.
**Context:** Confirms that double-clicking a layer row does not trigger a save dialog when the layer has no unsaved content. This prevents spurious save prompts on clean layers and verifies that the save-on-double-click path is gated on the layer's dirty/shared status.
**Pre-conditions:** Stub stage (non-shared). First sublayer item (needsSaving = false).
**Action:** Verify precondition and check call counter.
**Expected result:** Counter is zero (saveLayerUI not called).

### 164. Double Click Skips When System Locked
`TEST_F(LayerTreeViewTest, DoubleClick_SkipsWhenSystemLocked)`
**Where:** In the layer tree, double-clicking a system-locked row (distinct lock indicator) should not open a save dialog, since system-locked layers cannot be written to.
**Context:** Confirms that a system-locked layer correctly reports it does not need saving, so double-clicking it will not attempt to open a save dialog. System-locked layers have their edit permission revoked and should be treated as read-only by all save paths.
**Pre-conditions:** First sublayer system-locked and no-edit. Item object.
**Action:** Check `needsSaving()` and clean up.
**Expected result:** `needsSaving()` is false. Cleanup removes lock.

---

## testLoadLayersDialog.cpp (8 Tests)

**Shared setup:** All tests use `LayerEditorTestFixture`. Dialog is created with root layer item as context.

### 165. Load Layers Dialog Constructs Without Crash
`TEST_F(LoadLayersDialogTest, LoadLayersDialog_ConstructsWithoutCrash)`
**Where:** In the Layer Editor toolbar, click the "Load an Existing Layer" button (folder icon) to open the Load Layers dialog.
**Context:** Verifies that the "Load Layers" dialog can be instantiated without error when given a valid root layer. This is the baseline sanity check that the dialog is safe to open from the layer editor.
**Pre-conditions:** Root layer item available.
**Action:** Create `LoadLayersDialog(rootItem, mainWindow)`.
**Expected result:** No exception thrown.

### 166. Load Layers Dialog Has At Least One Line Edit
`TEST_F(LoadLayersDialogTest, LoadLayersDialog_HasAtLeastOneLineEdit)`
**Where:** In the Load Layers dialog, there is at least one text field where you can type or paste a layer file path directly.
**Context:** Confirms the dialog contains at least one text input field so the user can type or paste a file path to load. Without an editable field, the user would have no way to specify a layer path.
**Pre-conditions:** Dialog constructed.
**Action:** Find all `QLineEdit` children.
**Expected result:** At least one found.

### 167. Load Layers Dialog Has Ok And Cancel Buttons
`TEST_F(LoadLayersDialogTest, LoadLayersDialog_HasOkAndCancelButtons)`
**Where:** At the bottom of the Load Layers dialog, there are OK (or "Load") and Cancel buttons to confirm or dismiss the operation.
**Context:** Verifies the dialog provides both a confirm and a dismiss action, which are required for any modal workflow. A missing OK or Cancel button would leave the user unable to complete or abort the operation.
**Pre-conditions:** Dialog constructed.
**Action:** Find buttons and check labels.
**Expected result:** Button with OK/Load text and button with Cancel text both found.

### 168. Load Layers Dialog Starts With Empty Path
`TEST_F(LoadLayersDialogTest, LoadLayersDialog_StartsWithEmptyPath)`
**Where:** In the Load Layers dialog, the file path text field is blank when the dialog first opens. No pre-filled path should appear.
**Context:** Confirms the path input field is blank when the dialog first opens, so the user is not confused by a pre-filled value from a previous session or an unrelated layer. An unexpected default path could cause accidental layer loads.
**Pre-conditions:** Dialog constructed.
**Action:** Get first line edit and check text.
**Expected result:** Text is empty.

### 169. Load Layers Dialog Has Scroll Area
`TEST_F(LoadLayersDialogTest, LoadLayersDialog_HasScrollArea)`
**Where:** In the Load Layers dialog, the area containing path fields scrolls when multiple layer paths are listed, so the dialog does not need to resize.
**Context:** Verifies the dialog contains a scroll area, which is needed when the user wants to load multiple layers at once and the list of path rows grows longer than the visible window area.
**Pre-conditions:** Dialog constructed.
**Action:** Find `QScrollArea` child.
**Expected result:** Widget is found.

### 170. Load Layers Dialog Exec Dismissed By Timer Does Not Hang
`TEST_F(LoadLayersDialogTest, LoadLayersDialog_ExecDismissedByTimerDoesNotHang)`
**Where:** Open the Load Layers dialog via the "Load an Existing Layer" toolbar button and click Cancel (or let it auto-dismiss). Maya should not freeze — the dialog should close promptly.
**Context:** Confirms the dialog can be opened and closed programmatically without freezing the application. This guards against the modal event loop blocking indefinitely, which would make the layer editor unusable.
**Pre-conditions:** Dialog constructed. Timer set to dismiss modal after 100ms.
**Action:** Call `exec()`.
**Expected result:** Dialog closes without hang.

### 171. Load Layers Dialog Add Row Button Exists
`TEST_F(LoadLayersDialogTest, LoadLayersDialog_AddRowButtonExists)`
**Where:** In the Load Layers dialog, there is a button (such as "+" or "Add") to insert additional path rows for loading multiple layers in a single operation.
**Context:** Verifies the dialog exposes a button that lets the user add additional path rows, enabling batch loading of multiple layers in a single operation. If no such button exists, users are limited to loading one layer at a time.
**Pre-conditions:** Dialog constructed.
**Action:** Count push buttons.
**Expected result:** At least one found.

### 172. Load Layers Dialog Path Edit Is Enabled
`TEST_F(LoadLayersDialogTest, LoadLayersDialog_PathEditIsEnabled)`
**Where:** In the Load Layers dialog, the file path text field accepts keyboard input immediately when the dialog opens (it is not read-only or disabled).
**Context:** Confirms the path input field is interactive and not read-only when the dialog opens, so the user can immediately type or edit the layer path. A disabled field would silently block all user input.
**Pre-conditions:** Dialog constructed.
**Action:** Get first line edit and check enabled state.
**Expected result:** Is enabled.

---

## testMenusAndStage.cpp (9 Tests)

**Shared setup:** All tests use `LayerEditorTestFixture`. Menu bar is accessed via parent main window.

### 173. Option Menu Display Layer Contents Action Exists
`TEST_F(LayerEditorTestFixture, OptionMenu_DisplayLayerContentsAction_Exists)`
**Where:** In the Layer Editor panel, look for an "Options" or gear-icon menu in the menu bar at the top. It contains a "Display Layer Content" toggle item.
**Context:** Verifies that the "Display Layer Content" toggle action is present in the layer editor's Options menu. A QA engineer needs to confirm this feature entry point exists before testing the content panel visibility behavior.
**Pre-conditions:** Widget with menu bar in main window.
**Action:** Search menu bar for "Display Layer Content" action.
**Expected result:** Action is found.

### 174. Option Menu Display Layer Contents Toggles
`TEST_F(LayerEditorTestFixture, OptionMenu_DisplayLayerContents_Toggles)`
**Where:** In the Layer Editor's Options menu, clicking "Display Layer Content" shows or hides the contents pane below the tree. Clicking it again reverses the visibility.
**Context:** Confirms that clicking "Display Layer Content" actually changes the checked state of the action. This is the basic smoke test for the content-panel show/hide toggle — if the action does not toggle, the feature is broken at the menu level.
**Pre-conditions:** "Display Layer Content" action found.
**Action:** Record initial checked state, trigger action, process events.
**Expected result:** Checked state has toggled.

### 175. Stage Selector Change Stage Updates Session State
`TEST_F(LayerEditorTestFixture, StageSelector_ChangeStage_UpdatesSessionState)`
**Where:** At the top of the Layer Editor panel, there is a stage-selector dropdown. Choosing a different stage switches the entire layer tree to show that stage's layers.
**Context:** Verifies that selecting a different stage in the stage-selector combo box causes the session state to switch its active stage. This is the core workflow for multi-stage scenes — the editor must track whichever stage the user picks.
**Pre-conditions:** Widget with stage selector combo box. At least 2 stages.
**Action:** Record current stage, change combo index, process events.
**Expected result:** Active stage changed.

### 176. Stage List Add Stage Appears In Selector
`TEST_F(LayerEditorTestFixture, StageList_AddStage_AppearsInSelector)`
**Where:** At the top of the Layer Editor, the stage-selector dropdown shows one new entry each time a USD stage is opened in Maya's scene.
**Context:** Confirms that when a new USD stage is registered with the session state, the stage-selector combo box gains a new entry. QA engineers use this to validate that dynamically opened stages become available in the editor without a restart.
**Pre-conditions:** Widget with stage selector. Known count before.
**Action:** Create and add new stage via `_sessionState.addStage()`, process events.
**Expected result:** Combo count increased.

### 177. Stage Selector Has At Least One Entry
`TEST_F(LayerEditorTestFixture, StageSelector_HasAtLeastOneEntry)`
**Where:** At the top of the Layer Editor panel, the stage-selector dropdown should never be empty; it always shows at least the currently active stage.
**Context:** Basic sanity check that the stage selector is not empty on startup. If no stage entry appears, every other stage-selector interaction will fail, so this acts as a baseline guard.
**Pre-conditions:** Widget with stage selector.
**Action:** Check combo count.
**Expected result:** Count >= 1.

### 178. Stage Selector Initial Count Matches Session Stage Count
`TEST_F(LayerEditorTestFixture, StageSelector_InitialCountMatchesSessionStageCount)`
**Where:** At the top of the Layer Editor, the stage-selector dropdown should have exactly as many entries as there are open USD stages in the current Maya session.
**Context:** Verifies that the combo box is fully in sync with the session state at startup — every stage known to the session must appear in the selector and nothing extra. Mismatches here would indicate a population bug in the UI initialization code.
**Pre-conditions:** Widget with stage selector.
**Action:** Get session stage count and combo count.
**Expected result:** Counts match.

### 179. Stage Selector Add Stage Increments Combo Count
`TEST_F(LayerEditorTestFixture, StageSelector_AddStage_IncrementsComboCount)`
**Where:** At the top of the Layer Editor, opening a new USD stage in Maya should add one new entry to the stage-selector dropdown.
**Context:** A focused variant of the "add stage" test that checks only the count increment, isolating the combo-update signal path from the broader session-state change behavior covered by other tests.
**Pre-conditions:** Widget with stage selector. Known count before.
**Action:** Add stage, process events, check count.
**Expected result:** Count increased.

### 180. Collapse Content Toggles Display Layer Contents In Menu
`TEST_F(LayerEditorTestFixture, CollapseContent_TogglesDisplayLayerContentsInMenu)`
**Where:** In the Layer Editor's Options menu, clicking "Display Layer Content" multiple times should reliably toggle its checkmark on and off, showing or hiding the contents pane each time.
**Context:** Tests the full round-trip of the content-panel toggle: trigger once to hide, trigger again to restore, verifying the checked state flips each time. This catches regressions where the action fires but the state does not update correctly on repeated clicks.
**Pre-conditions:** Menu bar with Display Layer Content action.
**Action:** Trigger action, process events, trigger again.
**Expected result:** State toggled successfully both times (or skip if menu unavailable).

### 181. Stage Selector Remove Add Stage Roundtrip Does Not Crash
`TEST_F(LayerEditorTestFixture, StageSelector_RemoveAddStage_RoundtripDoesNotCrash)`
**Where:** At the top of the Layer Editor, adding and closing USD stages should keep the stage-selector dropdown stable and not cause Maya to crash.
**Context:** Stability test that adds a stage and confirms the editor does not crash or assert during the combo-update path. Because the stub lacks a removeStage API, it focuses on verifying the add side of a typical open/close roundtrip does not destabilize the widget.
**Pre-conditions:** Widget with stage selector. Known count before.
**Action:** Add stage, process events, check count.
**Expected result:** Count increased and no crash.

---

## testReorder.cpp (9 Tests)

**Shared setup:** All tests use `LayerEditorTestFixture`. Helper function `addSecondSublayer()` creates a second sublayer for reordering tests.

### 182. Drag Drop Move Row Down Calls Move Sub Layer Path
`TEST_F(LayerEditorTestFixture, DragDrop_MoveRowDown_CallsMoveSubLayerPath)`
**Where:** In the layer tree, drag a sublayer row downward and drop it below another sublayer row in the same parent. The dragged row should move to the new lower position.
**Context:** Verifies that dragging the first sublayer downward (to the last position) triggers the reorder command. QA engineers need confidence that the drag-and-drop reorder operation actually invokes the command hook, not just silently rearranges the UI.
**Pre-conditions:** Two sublayers in root. First sublayer mime data prepared.
**Action:** Drop at row 2 (below both). Process events.
**Expected result:** If accepted, `moveSubLayerPath` is called. If not accepted, MIME types not empty.

### 183. Drag Drop Move Row Up Calls Move Sub Layer Path
`TEST_F(LayerEditorTestFixture, DragDrop_MoveRowUp_CallsMoveSubLayerPath)`
**Where:** In the layer tree, drag a sublayer row upward and drop it above another sublayer row in the same parent. The dragged row should move to the new higher position.
**Context:** Verifies that dragging the second sublayer upward (to the first position) triggers the reorder command. This is the mirror case to moving down, ensuring both drag directions correctly dispatch the move command.
**Pre-conditions:** Two sublayers in root. Second sublayer mime data prepared.
**Action:** Drop at row 0 (above first). Process events.
**Expected result:** If accepted, `moveSubLayerPath` is called. If not accepted, MIME types not empty.

### 184. Drag Drop Can Drop Returns False For Non Move Action
`TEST_F(LayerEditorTestFixture, DragDrop_CanDrop_ReturnsFalseForNonMoveAction)`
**Where:** In the layer tree, attempting to copy-drop (e.g., holding Ctrl while dropping) a sublayer row should be rejected. Only move-drops are supported.
**Context:** Confirms the layer tree rejects drop operations that are not a Move (e.g. Copy). This prevents accidental duplication of layers when a user tries to copy-drop instead of reorder.
**Pre-conditions:** Mime data for first sublayer.
**Action:** Call `canDropMimeData()` with `Qt::CopyAction`.
**Expected result:** Returns false.

### 185. Drag Drop Can Drop Returns False For Wrong Mime Type
`TEST_F(LayerEditorTestFixture, DragDrop_CanDrop_ReturnsFalseForWrongMimeType)`
**Where:** In the layer tree, dragging data from outside the Layer Editor (e.g., a file from the OS file manager) and dropping it on a row should be silently rejected.
**Context:** Confirms the layer tree ignores drops of unrelated data (such as files or text from other applications). This prevents crashes or corruption when foreign MIME data is dragged onto the layer list.
**Pre-conditions:** Mime data with "application/x-wrong" type.
**Action:** Call `canDropMimeData()`.
**Expected result:** Returns false.

### 186. Drag Drop Can Drop Returns False For Locked Parent
`TEST_F(LayerEditorTestFixture, DragDrop_CanDrop_ReturnsFalseForLockedParent)`
**Where:** In the layer tree, dragging a sublayer and hovering over a locked parent row (padlock visible) should not show a valid drop indicator; the drop should be rejected.
**Context:** Ensures a sublayer cannot be reordered into a locked parent layer. Dropping into a locked parent would require modifying it, which must be blocked to respect the lock state.
**Pre-conditions:** Root layer locked. Mime data for first sublayer.
**Action:** Call `canDropMimeData()`.
**Expected result:** Returns false. Cleanup unlocks root.

### 187. Drag Drop Can Drop Returns False For Read Only Parent
`TEST_F(LayerEditorTestFixture, DragDrop_CanDrop_ReturnsFalseForReadOnlyParent)`
**Where:** In the layer tree, dragging a sublayer and hovering over a parent row that lacks write permission should not show a valid drop indicator; the drop should be rejected.
**Context:** Ensures a sublayer cannot be dropped into a parent whose edit and save permissions have been revoked. This covers the case where a layer is read-only at the USD permission level rather than through the editor's lock mechanism.
**Pre-conditions:** Root layer permissions revoked. Mime data for first sublayer.
**Action:** Call `canDropMimeData()`.
**Expected result:** Returns false. Cleanup restores permissions.

### 188. Drag Drop Can Drop Returns True For Valid Move
`TEST_F(LayerEditorTestFixture, DragDrop_CanDrop_ReturnsTrueForValidMove)`
**Where:** In the layer tree, dragging a sublayer row and hovering over an unlocked parent row should show a valid drop indicator (e.g., a horizontal insertion line), confirming the drop is accepted.
**Context:** Confirms that a well-formed move drag onto an editable parent is accepted by the model. This is the positive baseline that QA can use to distinguish legitimate reorders from blocked ones.
**Pre-conditions:** Mime data for first sublayer.
**Action:** Call `canDropMimeData()` on root.
**Expected result:** Returns true.

### 189. Drag Drop Drop Adjusts Row Index When Moving Up
`TEST_F(LayerEditorTestFixture, DragDrop_Drop_AdjustsRowIndexWhenMovingUp)`
**Where:** In the layer tree, after dragging a row upward and dropping it, the row should appear at the correct intended slot rather than one position off.
**Context:** Verifies the model correctly handles the row-index adjustment needed when a layer is moved upward — the target row must be recalculated after the source is removed. A bug here would place the layer at the wrong position.
**Pre-conditions:** Two sublayers. Moving second (index 1) to first (index 0).
**Action:** Perform drop, process events, re-fetch parent index.
**Expected result:** Parent has at least 1 child row.

### 190. Drag Drop Drop Calls Move Sub Layer Path On Success
`TEST_F(LayerEditorTestFixture, DragDrop_Drop_CallsMoveSubLayerPathOnSuccess)`
**Where:** In the layer tree, completing a drag-and-drop of a sublayer row should visually reorder the rows and update the layer stack order used by USD for opinion strength.
**Context:** End-to-end check that a completed drop operation dispatches `moveSubLayerPath` to the command hook. This confirms the full drop pipeline — MIME data creation, acceptance check, and command dispatch — works together correctly.
**Pre-conditions:** Two sublayers. Clear hook history. Mime data for first (index 0).
**Action:** Drop at row 2. Process events.
**Expected result:** If accepted, `moveSubLayerPath` called. If not, MIME types not empty.

---

## testSaveLayersDialog.cpp (12 Tests)

**Shared setup:** All tests use `LayerEditorTestFixture`. Dialog is created with session state.

### 191. Save Layers Dialog Constructs From Session State
`TEST_F(SaveLayersDialogTest, SaveLayersDialog_ConstructsFromSessionState)`
**Where:** In the Layer Editor toolbar on a shared stage, click the "Save all edits" button to open the Save Layers dialog.
**Context:** Verifies the Save Layers dialog can be constructed without crashing. This is the most basic sanity check — if the dialog fails to construct, no save workflow is possible.
**Pre-conditions:** Session state available.
**Action:** Create `SaveLayersDialog(&_sessionState, mainWindow, false)`.
**Expected result:** No exception thrown.

### 192. Save Layers Dialog Has Save All Button
`TEST_F(SaveLayersDialogTest, SaveLayersDialog_HasSaveAllButton)`
**Where:** In the Save Layers dialog, at least one action button (such as "Save All" or "OK") is present to initiate the save operation.
**Context:** Confirms the dialog presents at least one action button to the user. Without any push button, users would have no way to trigger or dismiss the save operation.
**Pre-conditions:** Dialog constructed.
**Action:** Find push buttons.
**Expected result:** At least one button found.

### 193. Save Layers Dialog Has Cancel Button
`TEST_F(SaveLayersDialogTest, SaveLayersDialog_HasCancelButton)`
**Where:** At the bottom of the Save Layers dialog, a "Cancel" button lets the user dismiss the dialog without saving any layers.
**Context:** Confirms users can dismiss the dialog without saving. A missing Cancel button would force users to complete or crash out of the save flow with no escape.
**Pre-conditions:** Dialog constructed.
**Action:** Find Cancel button.
**Expected result:** Button found.

### 194. Save Layers Dialog All As Relative Checkbox Exists
`TEST_F(SaveLayersDialogTest, SaveLayersDialog_AllAsRelativeCheckboxExists)`
**Where:** In the Save Layers dialog, look for an "All As Relative" checkbox that controls whether saved file paths are stored as relative or absolute.
**Context:** Smoke-tests that the "All As Relative" checkbox area does not cause a crash during dialog construction. The checkbox may or may not appear depending on whether anonymous layers are present in the stub.
**Pre-conditions:** Dialog constructed.
**Action:** Find `QCheckBox` children.
**Expected result:** Test succeeds (checkbox presence depends on layer configuration).

### 195. Quietly Uncheck All As Relative Does Not Crash
`TEST_F(SaveLayersDialogTest, QuietlyUncheckAllAsRelative_DoesNotCrash)`
**Where:** In the Save Layers dialog, the "All As Relative" checkbox can be silently cleared by the editor (without user interaction) when conditions change. This should not crash the dialog.
**Context:** Verifies that programmatically unchecking the "all paths as relative" option is safe to call. This method is used internally when conditions change and the option must be silently cleared.
**Pre-conditions:** Dialog constructed.
**Action:** Call `quietlyUncheckAllAsRelative()`.
**Expected result:** No exception thrown.

### 196. Ok To Save Does Not Crash With No Layers
`TEST_F(SaveLayersDialogTest, OkToSave_DoesNotCrashWithNoLayers)`
**Where:** Open the Save Layers dialog and click OK immediately when there are no layers to save. The dialog should close without hanging or crashing Maya.
**Context:** Validates that opening and immediately dismissing the dialog does not hang or crash when there are no layers needing to be saved. This guards against the dialog blocking indefinitely on an empty layer set.
**Pre-conditions:** Dialog constructed. Timer set to dismiss after 100ms.
**Action:** Call `exec()`.
**Expected result:** No exception thrown and no hang.

### 197. Layers Saved To Pairs Is Empty Initially
`TEST_F(SaveLayersDialogTest, LayersSavedToPairs_IsEmptyInitially)`
**Where:** In the Save Layers dialog, before clicking Save, no layers are listed as already saved — the saved-results section starts empty.
**Context:** Confirms that the list of successfully saved layer/path pairs starts empty before any save has been attempted. Stale data in this list could cause incorrect post-save reporting to the user.
**Pre-conditions:** Dialog constructed.
**Action:** Call `layersSavedToPairs()`.
**Expected result:** Returns empty list.

### 198. Layers With Error Pairs Is Empty Initially
`TEST_F(SaveLayersDialogTest, LayersWithErrorPairs_IsEmptyInitially)`
**Where:** In the Save Layers dialog, before clicking Save, the error section starts empty — no failed saves are reported before any operation has been attempted.
**Context:** Confirms that the error list starts empty so that no false save-failure notices are shown to the user before any operation has been attempted.
**Pre-conditions:** Dialog constructed.
**Action:** Call `layersWithErrorPairs()`.
**Expected result:** Returns empty list.

### 199. Layers Not Saved Is Empty Initially
`TEST_F(SaveLayersDialogTest, LayersNotSaved_IsEmptyInitially)`
**Where:** In the Save Layers dialog, before clicking Save, no layers are listed as skipped. The not-saved section starts empty.
**Context:** Confirms that the "not saved" list starts empty, ensuring the dialog does not incorrectly report skipped layers before any save action has occurred.
**Pre-conditions:** Dialog constructed.
**Action:** Call `layersNotSaved()`.
**Expected result:** Returns empty list.

### 200. Save Layers Dialog Exporting Flag Changes Title
`TEST_F(SaveLayersDialogTest, SaveLayersDialog_ExportingFlagChangesTitle)`
**Where:** The same dialog can appear with a different title (e.g., "Export Layers") when triggered from an export workflow. The title in the dialog's title bar should reflect the current mode.
**Context:** Verifies that the dialog can be instantiated in both "save" and "export" modes without crashing. The `isExporting` flag is expected to change the dialog title so QA can confirm the correct workflow label is shown.
**Pre-conditions:** Create export dialog and save dialog.
**Action:** Both construct successfully.
**Expected result:** No exception thrown (titles differ via isExporting flag).

### 201. All As Relative Toggle Does Not Crash
`TEST_F(SaveLayersDialogTest, AllAsRelative_ToggleDoesNotCrash)`
**Where:** In the Save Layers dialog, repeatedly checking and unchecking the "All As Relative" checkbox should not crash or freeze the dialog.
**Context:** Ensures that toggling the "all paths as relative" checkbox on and off does not crash the dialog. This option affects how file paths are written and must remain stable under rapid user interaction.
**Pre-conditions:** Dialog constructed.
**Action:** Find checkbox, set checked true/false, process events.
**Expected result:** No exception thrown (or skip if no checkbox).

### 202. For Each Entry Does Not Crash With No Layers
`TEST_F(SaveLayersDialogTest, ForEachEntry_DoesNotCrashWithNoLayers)`
**Where:** In the Save Layers dialog, when no layers are present (the dialog is empty), internal operations should complete cleanly without crashing.
**Context:** Validates that iterating over dialog entries via `forEachEntry` is safe even when no layers are present. This method is used to apply bulk operations and must handle the empty-list edge case gracefully.
**Pre-conditions:** Dialog constructed.
**Action:** Call `forEachEntry()` with counter lambda.
**Expected result:** No exception thrown.

---

## testSharedStage.cpp (16 Tests)

This file tests layer editor behavior for different stage types via fixture classes.

### SharedStageFixture Tests (8 tests)

**Shared setup:** `SharedStageFixture` sets `_isSharedStage = true` and runs full `SetUp()`.

#### 203. Needs Saving True For Anonymous Layer
`TEST_F(SharedStageFixture, NeedsSaving_TrueForAnonymousLayer)`
**Where:** In the layer tree on a shared stage, an anonymous sublayer row shows a save-needed indicator (e.g., asterisk or special styling) telling the user it must be saved to a file.
**Context:** On a shared stage the layer editor owns responsibility for persisting layers, so anonymous (in-memory) sublayers must be flagged as needing a save. This verifies the ownership contract that keeps unsaved work from being silently lost.
**Pre-conditions:** Shared stage with anonymous sublayer.
**Action:** Get first sublayer item and call `needsSaving()`.
**Expected result:** Returns true (anonymous layers must be saved by editor on shared stage).

#### 204. Needs Saving True For Dirty Root Layer
`TEST_F(SharedStageFixture, NeedsSaving_TrueForDirtyRootLayer)`
**Where:** In the layer tree on a shared stage, a root layer row with unsaved modifications shows a save-needed indicator.
**Context:** A shared stage's root layer that has been modified must be surfaced to the user for saving. This confirms the dirty-flag check feeds through to the save-needed status correctly.
**Pre-conditions:** Shared stage. Root layer marked dirty.
**Action:** Get root item and call `needsSaving()`.
**Expected result:** Returns true.

#### 205. Needs Saving False For Session Layer
`TEST_F(SharedStageFixture, NeedsSaving_FalseForSessionLayer)`
**Where:** In the layer tree on a shared stage, the topmost "Session Layer" row shows no save-needed indicator — Maya manages it separately.
**Context:** The session layer is managed by the DCC application (Maya), not the layer editor, so it should never appear in the editor's save responsibility list. This guards against double-save prompts or editor interference with Maya's own session management.
**Pre-conditions:** Shared stage. Session layer marked dirty.
**Action:** Get session item and call `needsSaving()`.
**Expected result:** Returns false (Maya manages session layer).

#### 206. Get All Needs Saving Layers Non Empty
`TEST_F(SharedStageFixture, GetAllNeedsSavingLayers_NonEmpty)`
**Where:** On a shared stage, the "Save all edits" button in the Layer Editor toolbar should be enabled (not greyed out) when at least one layer row needs saving.
**Context:** The tree model must be able to produce a list of layers that require saving so the Save button and save dialog can operate correctly. This confirms that at least one layer is identified when the stage is shared and contains an anonymous sublayer.
**Pre-conditions:** Shared stage with anonymous sublayer.
**Action:** Call `getAllNeedsSavingLayers()`.
**Expected result:** Returns non-empty list.

#### 207. Save Button Visible On Shared Stage
`TEST_F(SharedStageFixture, SaveButton_VisibleOnSharedStage)`
**Where:** In the Layer Editor toolbar, the "Save all edits" button is visible only when the active stage is shared. On non-shared stages this button is absent from the toolbar.
**Context:** The "Save all edits" button is only relevant when the editor owns the stage; it should be shown on shared stages and hidden otherwise. This verifies the correct visibility rule is applied at startup.
**Pre-conditions:** Shared stage.
**Action:** Find "Save all edits" button.
**Expected result:** Button is visible.

#### 208. Save Button Enabled When Layers Need Saving
`TEST_F(SharedStageFixture, SaveButton_EnabledWhenLayersNeedSaving)`
**Where:** In the Layer Editor toolbar, the "Save all edits" button is clickable (not greyed out) when the shared stage has anonymous or dirty layers.
**Context:** The Save button should be actionable only when there is something to save, preventing confusing no-op interactions. This checks that the button is both visible and enabled when the stage has layers pending a save.
**Pre-conditions:** Shared stage with anonymous sublayer.
**Action:** Find "Save all edits" button.
**Expected result:** Button is visible and enabled.

#### 209. Needs Saving False After Switching To Non Shared Stage
`TEST_F(SharedStageFixture, NeedsSaving_FalseAfterSwitchingToNonSharedStage)`
**Where:** In the layer tree, after switching to a non-shared stage via the stage-selector dropdown at the top of the panel, all row save-needed indicators should disappear.
**Context:** When the active stage switches from shared to non-shared, the editor should immediately stop treating layers as its save responsibility. This ensures the model reacts dynamically so stale save indicators do not mislead the user.
**Pre-conditions:** Shared stage. Dynamically flip `_isSharedStage = false` and rebuild.
**Action:** Get first sublayer item and call `needsSaving()`.
**Expected result:** Returns false.

#### 210. Get All Needs Saving Layers Empty After Switching To Non Shared Stage
`TEST_F(SharedStageFixture, GetAllNeedsSavingLayers_EmptyAfterSwitchingToNonSharedStage)`
**Where:** In the Layer Editor toolbar, after switching to a non-shared stage, the "Save all edits" button should become hidden or disabled.
**Context:** The save-needed layer list must be empty after switching to a non-shared stage, keeping the Save button disabled and the save dialog empty. This complements the per-item check by verifying the aggregate query also reflects the stage-type change.
**Pre-conditions:** Shared stage. Dynamically flip to non-shared and rebuild.
**Action:** Call `getAllNeedsSavingLayers()`.
**Expected result:** Returns empty list.

### ReferencedLayersFixture Tests (4 tests)

**Shared setup:** `ReferencedLayersFixture` leaves stage non-shared and stamps first sublayer in "adskSharedLayers" metadata before widget build.

#### 211. Is Read Only True For Referenced Layer
`TEST_F(ReferencedLayersFixture, IsReadOnly_TrueForReferencedLayer)`
**Where:** In the layer tree, a layer row listed in the "adskSharedLayers" metadata set shows a read-only indicator; its context menu has editing options greyed out.
**Context:** A sublayer listed in the "adskSharedLayers" metadata is owned by another asset and must not be editable in this context. This verifies the read-only guard that prevents accidental modifications to shared data.
**Pre-conditions:** Sublayer listed in adskSharedLayers metadata.
**Action:** Get sublayer item and call `isReadOnly()`.
**Expected result:** Returns true.

#### 212. Needs Saving False For Referenced Layer On Non Shared Stage
`TEST_F(ReferencedLayersFixture, NeedsSaving_FalseForReferencedLayerOnNonSharedStage)`
**Where:** In the layer tree on a non-shared stage, a referenced (shared) sublayer row shows no save-needed indicator.
**Context:** On a non-shared stage the editor does not own saving, so even a referenced sublayer should not be reported as needing a save. This prevents spurious save prompts appearing for layers the editor has no authority to write.
**Pre-conditions:** Non-shared stage with referenced sublayer.
**Action:** Get sublayer item and call `needsSaving()`.
**Expected result:** Returns false.

#### 213. Is Read Only False For Non Referenced Root Layer
`TEST_F(ReferencedLayersFixture, IsReadOnly_FalseForNonReferencedRootLayer)`
**Where:** In the layer tree, the root layer row (not in the shared-layers list) shows no read-only indicator and all context menu editing options are available.
**Context:** Only layers explicitly listed in the shared-layers metadata should be read-only; all other layers, including the root, remain editable. This confirms the read-only flag is applied selectively and does not leak to unrelated layers.
**Pre-conditions:** Root layer not in adskSharedLayers.
**Action:** Get root item and call `isReadOnly()`.
**Expected result:** Returns false.

#### 214. Get All Needs Saving Layers Empty On Non Shared Stage
`TEST_F(ReferencedLayersFixture, GetAllNeedsSavingLayers_EmptyOnNonSharedStage)`
**Where:** On a non-shared stage, the Save Layers dialog (if opened) shows no entries; the "Save all edits" button in the toolbar is either hidden or disabled.
**Context:** The aggregate save list must stay empty on a non-shared stage regardless of what metadata layers carry. This is the collection-level counterpart to the per-item check, ensuring the Save button remains hidden or disabled.
**Pre-conditions:** Non-shared stage with referenced sublayer.
**Action:** Call `getAllNeedsSavingLayers()`.
**Expected result:** Returns empty list.

### IncomingStageFixture Tests (5 tests)

**Shared setup:** `IncomingStageFixture` sets `_isSharedStage = true` and `_isStageIncoming = true` before widget build.

#### 215. Is Incoming True For Root Layer
`TEST_F(IncomingStageFixture, IsIncoming_TrueForRootLayer)`
**Where:** In the layer tree on an incoming shared stage, the root layer row displays a special indicator (icon or coloring) showing it originates from an external source.
**Context:** When the stage is driven by an external source (incoming), all authored layers including the root should be tagged as incoming so the UI can indicate their origin. This verifies the root layer correctly reflects the incoming state.
**Pre-conditions:** Incoming shared stage.
**Action:** Get root item and call `isIncoming()`.
**Expected result:** Returns true.

#### 216. Is Incoming True For Sublayer
`TEST_F(IncomingStageFixture, IsIncoming_TrueForSublayer)`
**Where:** In the layer tree on an incoming shared stage, all sublayer rows also display the incoming indicator, showing the whole layer stack is externally driven.
**Context:** The incoming classification must propagate to every authored layer in the stack, not just the root. This confirms sublayers are also marked incoming so the editor displays a consistent signal across the whole layer hierarchy.
**Pre-conditions:** Incoming shared stage.
**Action:** Get first sublayer item and call `isIncoming()`.
**Expected result:** Returns true.

#### 217. Is Incoming False For Session Layer
`TEST_F(IncomingStageFixture, IsIncoming_FalseForSessionLayer)`
**Where:** In the layer tree on an incoming shared stage, the topmost "Session Layer" row shows no incoming indicator — it is always a local DCC-managed layer.
**Context:** The session layer is a local DCC-managed layer and is never part of the externally-driven incoming set. This guards against the incoming flag incorrectly bleeding onto the session layer and misrepresenting its ownership.
**Pre-conditions:** Incoming shared stage.
**Action:** Get session item and call `isIncoming()`.
**Expected result:** Returns false (session layers never in incoming set).

#### 218. Needs Saving True For Anonymous Layer On Incoming Stage
`TEST_F(IncomingStageFixture, NeedsSaving_TrueForAnonymousLayerOnIncomingStage)`
**Where:** In the layer tree on an incoming shared stage, anonymous sublayer rows still show a save-needed indicator; the incoming flag does not affect save responsibility.
**Context:** The incoming flag describes where the stage data comes from, but it does not change who is responsible for saving in-memory layers; that is still determined by the shared-stage flag. This confirms that save responsibility is evaluated independently of incoming status.
**Pre-conditions:** Incoming shared stage with anonymous sublayer.
**Action:** Get sublayer item and call `needsSaving()`.
**Expected result:** Returns true (incoming flag does not suppress saving).
