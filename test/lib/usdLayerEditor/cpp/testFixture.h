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
#pragma once

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
#include <string>

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

    // Select a single tree row so LayerEditorWindow state queries are valid.
    void selectRow(const QModelIndex& index);

    StubSessionState                        _sessionState;
    std::unique_ptr<StubLayerEditorWindow>  _window;
    QMainWindow*                            _mainWindow { nullptr };

    // Convenience: the widget owned by _window.
    LayerEditorWidget* _widget { nullptr };

    // DCC-function registry driven by these flags (installed in SetUp,
    // restored in TearDown). Lambdas read the flags at call time, so flips
    // mid-test take effect on the next model rebuild.
    bool _efSupported   { false };
    bool _sharedStage   { false };
    bool _stageIncoming { false };

    bool        _isComponent { false };
    bool        _isUnsavedComponent { false };
    std::string _moveComponentResult; // non-empty => move "succeeds"
    int         _saveComponentCalls { 0 };
    int         _reloadComponentCalls { 0 };
    int         _transferSessionCalls { 0 };
    int         _setProxyRootPathCalls { 0 };

    // Modal dialogs (confirmDialog/warningDialog) are suppressed during tests via
    // a handler installed in SetUp; this counts how many would have been shown,
    // and _modalDialogAnswer is the value the suppressed dialog returns.
    int  _modalDialogCount { 0 };
    bool _modalDialogAnswer { true };
    bool _confirmExistingFileSave { false };

    ScopedLayerEditorDCCFunctions _scopedDCCFunctions;

    void setEditForwardingSupported(bool supported) { _efSupported = supported; }
    void setSharedStage(bool shared) { _sharedStage = shared; }
    void setStageIncoming(bool incoming) { _stageIncoming = incoming; }
    void setIsComponent(bool v) { _isComponent = v; }
    void setIsUnsavedComponent(bool v) { _isUnsavedComponent = v; }
};

} // namespace UsdLayerEditor
