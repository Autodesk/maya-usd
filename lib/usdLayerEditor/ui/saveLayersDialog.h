//
// Copyright 2020 Autodesk
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

 #ifndef USDLAYEREDITOR_SAVELAYERSDIALOG_H
 #define USDLAYEREDITOR_SAVELAYERSDIALOG_H

 #include "batchSaveLayersUIDelegate.h"
 #include "utilSerialization.h"

 #include <pxr/usd/sdf/layer.h>
 #include <pxr/usd/usd/stage.h>

 #include <QtCore/QStringList>
 #include <QtWidgets/QDialog>
 #include <QtWidgets/QtWidgets>
 #include <unordered_map>
 #include <unordered_set>
 #include <vector>

 PXR_NAMESPACE_USING_DIRECTIVE

 class QWidget;
 class SaveLayerPathRow;

 namespace UsdLayerEditor {

 class ComponentSaveWidget;
 class SessionState;


 class LAYEREDITOR_UI_PUBLIC SaveLayersDialog : public QDialog
{
 public:
     typedef std::unordered_multimap<SdfLayerRefPtr, std::string, TfHash> stageLayerMap;

     // Create dialog using single stage (from session state).
     SaveLayersDialog(SessionState* in_sessionState, QWidget* in_parent, bool isExporting);

     // Create dialog for bulk save using all provided proxy shapes and their owned stages.
     // If componentsOnly is true, only component stages are shown (no anonymous/file-backed
     // layers).
     SaveLayersDialog(
         QWidget* in_parent,
         const std::vector<StageSavingInfo>& infos,
         bool                                isExporting,
         bool                                componentsOnly = false);

     ~SaveLayersDialog();

     // UI to get a file path to save a layer.
     // As output returns the path.
     static bool
     saveLayerFilePathUI(std::string& out_filePath, const std::string& parentLayer);
     static bool
     saveLayerFilePathUI(std::string& out_filePath, const SdfLayerRefPtr& parentLayer);

     QWidget* findEntry(SdfLayerRefPtr key);

     void forEachEntry(const std::function<void(QWidget*)>& func);

     void quietlyUncheckAllAsRelative();

     // Test seam: when a handler is installed, exec() returns its result without
     // showing the (blocking) dialog. Production never installs one. Returns the
     // previously-installed handler.
     using ExecTestHandler = std::function<int()>;
     static ExecTestHandler setExecTestHandler(ExecTestHandler handler);
     int                    exec() override;

 protected:
     void onSaveAll();
     void onCancel();
     void onAllAsRelativeChanged();

     bool okToSave();

 public:
     const QStringList&   layersSavedToPairs() const { return _newPaths; }
     const QStringList&   layersWithErrorPairs() const { return _problemLayers; }
     const QStringList&   layersNotSaved() const { return _emptyLayers; }
     const stageLayerMap& stageLayers() const { return _stageLayerMap; }
     SessionState*        sessionState() { return _sessionState; }
     QString              buildTooltipForLayer(SdfLayerRefPtr layer);

 private:
     void buildDialog(const QString& msg1, const QString& msg2, const QString& msg3);
     void getLayersToSave(
         const UsdStageRefPtr& stage,
         const std::string&    proxyPath,
         const std::string&    stageName);

 private:
     typedef std::unordered_set<SdfLayerRefPtr, TfHash> layerSet;
     using LayerInfos = UsdLayerEditor::Serialization::LayerInfos;

     QStringList                       _newPaths;
     QStringList                       _problemLayers;
     QStringList                       _emptyLayers;
     QWidget*                          _anonLayersWidget { nullptr };
     QWidget*                          _fileLayersWidget { nullptr };
     QWidget*                          _componentStagesWidget { nullptr };
     QCheckBox*                        _allAsRelative { nullptr };
     LayerInfos                        _anonLayerInfos;
     layerSet                          _dirtyFileBackedLayers;
     stageLayerMap                     _stageLayerMap;
     SessionState*                     _sessionState;
     std::vector<QWidget*>             _saveLayerPathRows;
     std::vector<StageSavingInfo>      _componentStageInfos;
     std::vector<ComponentSaveWidget*> _componentSaveWidgets;
     bool                              _isExporting { false };
 };

 }; // namespace UsdLayerEditor

 #endif // USDLAYEREDITOR_SAVELAYERSDIALOG_H
