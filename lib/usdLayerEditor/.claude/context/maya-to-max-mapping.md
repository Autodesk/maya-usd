### Things to keep in mind:
- Maya and Max have some core differences, while the logic of the feature may be the same between the two. You can expect some code duplication.
- The core implementation of the layer editor is found within the git submodule usd-layer-editor.
- In maya-usd, commands are implemented as sub-classes of BaseCmd inside layerEditorCommand.cpp, each with doIt()/undoIt() methods, wrapped by the LayerEditorCommand MPxCommand that parses MEL flags and dispatches to them. This command is registered under the name mayaUsdLayerEditor, making it invokable from Python via maya.cmds.mayaUsdLayerEditor(). In max's usd-layer-editor, the layer editor routes all operations through an AbstractCommandHook interface, which the UfeCommandHook implements by creating Ufe::UndoableCommand subclasses that are executed through UFE's undo/redo manager, and these commands are also exposed to Python via boost.python bindings in python/wrapUsdLayerEditor.cpp.
- `LayerTreeView::callMethodOnSelection` in the 3dsMax submodule only accepts `simpleLayerMethod = void (LayerTreeItem::*)()` — no parameters. Maya's simpleLayerMethod is typedef'd as `void (LayerTreeItem::*)(QWidget* in_parent)`, but that does not exist here. If a Maya port declares a `LayerTreeItem` method with a `QWidget*` parameter intended for use with `callMethodOnSelection`, drop the parameter as 3dsMax typedef requires a no-parameter method pointer.
- When adding Python bindings (`wrapUsdLayerEditor.cpp`), always build **both** configurations: `python build-scripts/build-solution.py 2026` and `python build-scripts/build-solution.py hybrid 2026`.

### Key files for 3dsmax-component-usd:
  Core layer editor wrapper:
  - src/MaxUsdObjects/LayerEditor/MaxLayerEditor.h/.cpp
  - src/MaxUsdObjects/LayerEditor/MaxLayerEditorWindow.h/.cpp
  - src/MaxUsdObjects/LayerEditor/MaxLayerEditorItemDelegate.h/.cpp
  - src/MaxUsdObjects/LayerEditor/MaxSessionState.h/.cpp
  - src/MaxUsdObjects/LayerEditor/USDLayerManager.h/.cpp

  Menu & actions:
  - src/MaxUsdObjects/Views/UsdMenu.h/.cpp
  - src/ApplicationPlugins/usd-component/Contents/scripts/registerMenu.ms/.mcr

  Layer editor commands (the actual operations):
  - layerEditorCommands.h/.cpp
  - abstractCommandHook.h
  - ufeCommandHook.h/.cpp

  Layer editor operations:
  - layers.h/.cpp
  - layerMuting.h/.cpp
  - layerLocking.h/.cpp
  - customLayerData.h/.cpp

  Layer editor UI:
  - layerEditorWindow.h/.cpp, abstractLayerEditorWindow.h
  - layerEditorWidget.h/.cpp
  - layerTreeView.h/.cpp, layerTreeModel.h/.cpp, layerTreeItem.h/.cpp, layerTreeItemDelegate.h/.cpp
  - saveLayersDialog.h/.cpp, loadLayersDialog.h/.cpp
  - sessionState.h/.cpp

  Python bindings:
  - usd-layer-editor/python/wrapUsdLayerEditor.cpp
  - usd-layer-editor/python/module.cpp

  Tests:
  - usd-layer-editor/test/layer_editor_test.py
  - src/Tests/Integration/layer_editor_test.ms
  - src/Tests/Integration/usd_layer_editor_test.ms

### Key files for usd-maya layer editor
  Commands:
  - lib/mayaUsd/commands/layerEditorCommand.cpp
  - lib/mayaUsd/commands/layerEditorWindowCommand.cpp

  Layer Editor UI:
  - lib/mayaUsd/ui/layer_editor/abstractLayerEditorWindow.h
  - lib/mayaUsd/ui/layer_editor/abstractCommandHook.h
  - lib/mayaUsd/ui/layer_editor/layerTreeItem.h
  - lib/mayaUsd/ui/layer_editor/layerTreeItem.cpp
  - lib/mayaUsd/ui/layer_editor/mayaCommandHook.h
  - lib/mayaUsd/ui/layer_editor/mayaCommandHook.cpp
  - lib/mayaUsd/ui/layer_editor/mayaLayerEditorWindow.h
  - lib/mayaUsd/ui/layer_editor/mayaLayerEditorWindow.cpp

  UI / Strings:
  - lib/mayaUsd/ui/mayaUsdMenu.mel
  - lib/mayaUsd/ui/mayaUSDRegisterStrings.py

  Tests:
  - test/lib/testMayaUsdLayerEditorCommands.py