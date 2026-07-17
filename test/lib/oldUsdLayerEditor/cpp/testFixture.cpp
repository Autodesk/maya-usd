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

#include "warningDialogs.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#include <pxr/usd/usdUtils/stageCache.h>

#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

namespace UsdLayerEditor {

void LayerEditorTestFixture::SetUp()
{
    setModalDialogTestHandler([this](const QString&, const QString&) {
        ++_modalDialogCount;
        return _modalDialogAnswer;
    });

    // Back one real mayaUsdProxyShape per stub stage with the SAME in-memory stage (via the stage
    // cache) so proxy-based discovery sees the identical layers.
    {
        const auto stages = _sessionState.allStages();
        for (int i = 0; i < 2; ++i) {
            _stageCacheIds[i] = PXR_NS::UsdUtilsStageCache::Get().Insert(stages[i]._stage);

            const std::string xformName = "leTestXform" + std::to_string(i);
            const std::string shapeName = "leTestProxy" + std::to_string(i);
            MGlobal::executeCommand(
                MString("createNode transform -n \"") + xformName.c_str() + "\"");
            MGlobal::executeCommand(
                MString("createNode mayaUsdProxyShape -n \"") + shapeName.c_str()
                + "\" -p " + xformName.c_str());
            MGlobal::executeCommand(
                MString("setAttr \"") + shapeName.c_str() + ".stageCacheId\" "
                + std::to_string(_stageCacheIds[i].ToLongInt()).c_str());

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

    // Delete real proxy shape nodes and erase the stage-cache entries created in SetUp.
    {
        for (int i = 0; i < 2; ++i) {
            if (!_proxyShapePaths[i].empty()) {
                const std::string xformPath = "|leTestXform" + std::to_string(i);
                MGlobal::executeCommand(
                    MString("delete \"") + xformPath.c_str() + "\"");
                _proxyShapePaths[i].clear();
            }
            if (_stageCacheIds[i].IsValid()) {
                PXR_NS::UsdUtilsStageCache::Get().Erase(_stageCacheIds[i]);
                _stageCacheIds[i] = PXR_NS::UsdStageCache::Id();
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

} // namespace UsdLayerEditor
