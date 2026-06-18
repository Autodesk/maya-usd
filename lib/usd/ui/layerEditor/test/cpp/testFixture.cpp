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
#include "warningDialogs.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <ghc/fs_std.hpp>

namespace UsdLayerEditor {

void LayerEditorTestFixture::SetUp()
{
    EditForwardingFns ef;
    ef.supportsEditForwarding = [this]() { return _efSupported; };
    ef.echoEditForwarding     = []() { return false; };
    ef.setEchoEditForwarding  = [](bool) {};
    setEditForwardingFns(ef);

    DccObjectFns dcc;
    dcc.isDccObjectStageIncoming = [this](const std::string&) { return _stageIncoming; };
    dcc.isDccObjectSharedStage   = [this](const std::string&) { return _sharedStage; };
    setDccObjectFns(dcc);

    ComponentFns component;
    component.isStageAComponent
        = [this](const std::string&) { return _isComponent; };
    component.isUnsavedComponent
        = [this](const PXR_NS::UsdStageRefPtr&) { return _isUnsavedComponent; };
    component.shouldDisplayComponentInitialSaveDialog
        = [](const PXR_NS::UsdStageRefPtr&, const std::string&) { return false; };
    component.saveComponent
        = [this](const PXR_NS::UsdStageRefPtr&, const std::string&) { ++_saveComponentCalls; };
    component.reloadComponent
        = [this](const std::string&) { ++_reloadComponentCalls; };
    component.moveComponent
        = [this](const std::string&, const std::string&, const std::string&) {
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

    SaveOptionFns saveOption;
    saveOption.confirmExistingFileSave = [this]() { return _confirmExistingFileSave; };
    setSaveOptionFns(saveOption);

    setModalDialogTestHandler([this](const QString&, const QString&) {
        ++_modalDialogCount;
        return _modalDialogAnswer;
    });

    // Create one real mayaUsdProxyShape per stub stage so Maya DAG lookups
    // succeed. Use generic_string() so forward slashes reach MEL on Windows
    // (backslashes are escape characters in MEL string literals).
    {
        namespace fss = fs::filesystem;
        const auto stages = _sessionState.allStages();
        for (int i = 0; i < 2; ++i) {
            _tempStagePaths[i] = (fss::temp_directory_path()
                / ("le_test_stage_" + std::to_string(i) + ".usda")).generic_string();
            stages[i]._stage->GetRootLayer()->Export(_tempStagePaths[i]);

            const std::string xformName = "leTestXform" + std::to_string(i);
            const std::string shapeName = "leTestProxy" + std::to_string(i);
            MGlobal::executeCommand(
                MString("createNode transform -n \"") + xformName.c_str() + "\"");
            MGlobal::executeCommand(
                MString("createNode mayaUsdProxyShape -n \"") + shapeName.c_str()
                + "\" -p " + xformName.c_str());
            MGlobal::executeCommand(
                MString("setAttr \"") + shapeName.c_str() + ".filePath\" -type \"string\" \""
                + _tempStagePaths[i].c_str() + "\"");

            MSelectionList sel;
            sel.add(MString(shapeName.c_str()));
            MDagPath dagPath;
            sel.getDagPath(0, dagPath);
            _proxyShapePaths[i] = dagPath.fullPathName().asChar();

            _sessionState.setProxyShapePath(i, _proxyShapePaths[i]);
        }
    }

    _mainWindow = new QMainWindow();
    _window     = std::make_unique<OldEditorStubLayerEditorWindow>(_sessionState, _mainWindow);
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
    _widget = nullptr;
    _window.reset();
    delete _mainWindow;
    _mainWindow = nullptr;

    // Delete real proxy shape nodes and temp stage files created in SetUp.
    {
        namespace fss = fs::filesystem;
        for (int i = 0; i < 2; ++i) {
            if (!_proxyShapePaths[i].empty()) {
                const std::string xformPath = "|leTestXform" + std::to_string(i);
                MGlobal::executeCommand(
                    MString("delete \"") + xformPath.c_str() + "\"");
                _proxyShapePaths[i].clear();
            }
            if (!_tempStagePaths[i].empty()) {
                std::error_code ec;
                fss::remove(_tempStagePaths[i], ec);
                _tempStagePaths[i].clear();
            }
        }
    }
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
    // Stub always shows session layer (autoHideSessionLayer=false).
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
    layerTree()->selectionModel()->select(
        index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    layerTree()->setCurrentIndex(index);
    QApplication::processEvents();
}

QAction* findAction(QMenu* menu, const QString& text)
{
    if (!menu) return nullptr;
    for (QAction* action : menu->actions()) {
        if (action->text() == text) return action;
        if (action->menu()) {
            QAction* found = findAction(action->menu(), text);
            if (found) return found;
        }
    }
    return nullptr;
}

} // namespace UsdLayerEditor
