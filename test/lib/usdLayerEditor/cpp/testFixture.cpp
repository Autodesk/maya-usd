//
// Copyright 2026 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "testFixture.h"

#include "layerEditorDCCFunctions.h"
#include "saveLayersDialog.h"
#include "warningDialogs.h"

#include <QtCore/QDir>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMainWindow>

namespace UsdLayerEditor {

void LayerEditorTestFixture::SetUp()
{
    EditForwardingFns ef;
    ef.supportsEditForwarding = [this]() { return _efSupported; };
    ef.echoEditForwarding = []() { return false; };
    ef.setEchoEditForwarding = [](bool) {};
    setEditForwardingFns(ef);

    DccObjectFns dcc;
    dcc.isDccObjectStageIncoming = [this](const std::string&) { return _stageIncoming; };
    dcc.isDccObjectSharedStage = [this](const std::string&) { return _sharedStage; };
    setDccObjectFns(dcc);

    {
        ComponentFns component;
        component.isStageAComponent = [this](const std::string&) { return _isComponent; };
        component.isUnsavedComponent
            = [this](const PXR_NS::UsdStageRefPtr&) { return _isUnsavedComponent; };
        component.shouldDisplayComponentInitialSaveDialog
            = [](const PXR_NS::UsdStageRefPtr&, const std::string&) { return false; };
        component.saveComponent
            = [this](const PXR_NS::UsdStageRefPtr&, const std::string&) { ++_saveComponentCalls; };
        component.reloadComponent = [this](const std::string&) { ++_reloadComponentCalls; };
        component.moveComponent = [this](const std::string&, const std::string&, const std::string&) {
            return _moveComponentResult;
        };
        component.renameProxyShape
            = [](const std::string&, const std::string& name) { return std::string("|") + name; };
        component.captureSessionLayer
            = [](const std::string&) { return PXR_NS::SdfLayerRefPtr {}; };
        component.transferSessionLayer
            = [this](const PXR_NS::SdfLayerRefPtr&, const std::string&) { ++_transferSessionCalls; };
        component.setProxyRootLayerPath
            = [this](const std::string&, const std::string&, const PXR_NS::SdfLayerRefPtr&) {
                  ++_setProxyRootPathCalls;
              };
        setComponentFns(component);
    }

    // Direct any generated save paths to the system temp dir so test-run artifacts
    // are never created inside the source tree.
    {
        FileSystemFns fns;
        fns.getDCCSceneDir = []() { return QDir::tempPath().toStdString(); };
        setFileSystemFns(fns);
    }

    // Headless tests must not pop the overwrite-confirm / save-layers dialog.
    // Defaulting this to false makes save paths take the non-interactive branch.
    {
        SaveOptionFns saveOption;
        saveOption.confirmExistingFileSave = [this]() { return _confirmExistingFileSave; };
        setSaveOptionFns(saveOption);
    }

    // Suppress blocking modal dialogs (confirmDialog/warningDialog) in headless
    // tests; record how many would have shown and answer with _modalDialogAnswer.
    setModalDialogTestHandler([this](const QString&, const QString&) {
        ++_modalDialogCount;
        return _modalDialogAnswer;
    });

    // The bulk save-layers dialog is modal too; never show it in headless tests.
    SaveLayersDialog::setExecTestHandler([]() { return QDialog::Rejected; });

    _mainWindow = new QMainWindow();
    _window     = std::make_unique<StubLayerEditorWindow>(_sessionState, _mainWindow);
    _widget     = _window->widget();
    _mainWindow->show();
    _widget->show();
    QApplication::processEvents();
    _sessionState._commandHookImpl.clearCalls();
    _sessionState._saveLayerCallCount  = 0;
    _sessionState._printLayerCallCount = 0;
    _sessionState._loadLayersCallCount = 0;
}

void LayerEditorTestFixture::TearDown()
{
    setModalDialogTestHandler(nullptr);
    SaveLayersDialog::setExecTestHandler(nullptr);
    _widget = nullptr;
    _window.reset();
    delete _mainWindow;
    _mainWindow = nullptr;
}

LayerTreeView* LayerEditorTestFixture::layerTree()
{
    return _widget->layerTree();
}

LayerTreeModel* LayerEditorTestFixture::treeModel()
{
    return layerTree()->layerTreeModel();
}

QModelIndex LayerEditorTestFixture::sessionLayerIndex()
{
    // The stub always shows the session layer (autoHideSessionLayer=false).
    // It is always the first top-level item in the tree.
    return treeModel()->index(0, 0);
}

QModelIndex LayerEditorTestFixture::rootLayerIndex()
{
    return treeModel()->rootLayerIndex();
}

QModelIndex LayerEditorTestFixture::firstSublayerIndex()
{
    return treeModel()->index(0, 0, rootLayerIndex());
}

void LayerEditorTestFixture::selectRow(const QModelIndex& index)
{
    layerTree()->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    layerTree()->setCurrentIndex(index);
    QApplication::processEvents();
}

QAction* findAction(QMenu* menu, const QString& text)
{
    if (!menu)
        return nullptr;
    for (QAction* action : menu->actions()) {
        if (action->text() == text)
            return action;
        if (action->menu()) {
            QAction* found = findAction(action->menu(), text);
            if (found)
                return found;
        }
    }
    return nullptr;
}

} // namespace UsdLayerEditor
