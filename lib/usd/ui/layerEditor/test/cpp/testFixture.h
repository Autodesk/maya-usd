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
#pragma once
#define LAYER_EDITOR_TEST_FIXTURE_INCLUDED

#include "scopedLayerEditorDCCFunctions.h"
#include "stubCommandHook.h"
#include "stubLayerEditorWindow.h"
#include "stubSessionState.h"

#include "layerEditorWidget.h"
#include "layerTreeModel.h"
#include "layerTreeView.h"

#include <gtest/gtest.h>

#include <QtCore/QModelIndex>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <memory>

namespace UsdLayerEditor {

class LayerEditorTestFixture : public ::testing::Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    LayerTreeView*  layerTree();
    LayerTreeModel* treeModel();
    QModelIndex     sessionLayerIndex();
    QModelIndex     rootLayerIndex();
    QModelIndex     firstSublayerIndex();
    void            selectRow(const QModelIndex& index);

    void setEditForwardingSupported(bool supported) { _sessionState._supportsEditForwarding = supported; }
    void setSharedStage(bool shared) { _sessionState._commandHookImpl._isSharedStage = shared; }
    void setStageIncoming(bool incoming) { _sessionState._commandHookImpl._isStageIncoming = incoming; }

    // Members — names must match new editor's testFixture.h exactly
    OldEditorStubSessionState                        _sessionState;
    std::unique_ptr<OldEditorStubLayerEditorWindow>  _window;
    QMainWindow*                                     _mainWindow { nullptr };
    LayerEditorWidget*                               _widget { nullptr };

    bool        _efSupported   { false };
    bool        _sharedStage   { false };
    bool        _stageIncoming { false };

    bool        _isComponent        { false };
    bool        _isUnsavedComponent { false };
    std::string _moveComponentResult;
    int         _saveComponentCalls      { 0 };
    int         _reloadComponentCalls    { 0 };
    int         _transferSessionCalls    { 0 };
    int         _setProxyRootPathCalls   { 0 };

    int  _modalDialogCount  { 0 };
    bool _modalDialogAnswer { true };
    bool _confirmExistingFileSave { false };

    ScopedLayerEditorDCCFunctions _scopedDCCFunctions;

    void setIsComponent(bool v)        { _isComponent = v; }
    void setIsUnsavedComponent(bool v) { _isUnsavedComponent = v; }
};

// Find a named action in a menu (searches recursively into submenus).
QAction* findAction(QMenu* menu, const QString& text);

} // namespace UsdLayerEditor
