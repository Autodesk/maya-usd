#ifndef SAVELAYERSDIALOG_H
#define SAVELAYERSDIALOG_H

#include <mayaUsd/nodes/layerManager.h>
#include <mayaUsd/utils/utilSerialization.h>

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

namespace UsdLayerEditor {

class SessionState;

class SaveLayersDialog : public QDialog
{
public:
    typedef std::unordered_multimap<SdfLayerRefPtr, std::string, TfHash> stageLayerMap;

    // Create dialog using single stage (from session state).
    SaveLayersDialog(SessionState* in_sessionState, QWidget* in_parent);

#if defined(WANT_UFE_BUILD)
    // Create dialog for bulk save using all provided proxy shapes and their owned stages.
    SaveLayersDialog(QWidget* in_parent, const std::vector<MayaUsd::BatchSaveInfo>& infos);
#endif

    ~SaveLayersDialog();

    // UI to get a file path to save a layer.
    // As output returns the path.
    static bool saveLayerFilePathUI(std::string& out_filePath);

protected:
    void onSaveAll();
    void onCancel();

    bool okToSave();

public:
    const QStringList&   layersSavedToPairs() const { return _newPaths; }
    const QStringList&   layersWithErrorPairs() const { return _problemLayers; }
    const QStringList&   layersNotSaved() const { return _emptyLayers; }
    const stageLayerMap& stageLayers() const { return _stageLayerMap; }
    SessionState*        sessionState() { return _sessionState; }
    QString              buildTooltipForLayer(SdfLayerRefPtr layer);

private:
    void buildDialog(const QString& msg1, const QString& msg2);
    void getLayersToSave(const std::string& proxyPath, const std::string& stageName);

private:
    typedef std::vector<std::pair<SdfLayerRefPtr, MayaUsd::utils::LayerParent>> layerPairs;
    typedef std::unordered_set<SdfLayerRefPtr, TfHash>                          layerSet;

    QStringList   _newPaths;
    QStringList   _problemLayers;
    QStringList   _emptyLayers;
    QWidget*      _anonLayersWidget { nullptr };
    QWidget*      _fileLayersWidget { nullptr };
    layerPairs    _anonLayerPairs;
    layerSet      _dirtyFileBackedLayers;
    stageLayerMap _stageLayerMap;
    SessionState* _sessionState;
};

}; // namespace UsdLayerEditor

#endif // saveLAYERSDIALOG_H
