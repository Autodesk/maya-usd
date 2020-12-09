#ifndef SAVELAYERSDIALOG_H
#define SAVELAYERSDIALOG_H

#include <QtCore/QStringList>
#include <QtWidgets/QDialog>
#include <QtWidgets/QtWidgets>

class QScrollArea;
class QVBoxLayout;

namespace UsdLayerEditor {

class LayerTreeItem;

class SaveLayersDialog : public QDialog
{
public:
    SaveLayersDialog(
        const QString&                     title,
        const QString&                     message,
        const std::vector<LayerTreeItem*>& layerItems,
        QWidget*                           in_parent);

protected:
    void onSaveAll();
    void onCancel();

    bool okToSave();

public:
    const QStringList& layersSavedToPairs() const { return _newPaths; }
    const QStringList& layersWithErrorPairs() const { return _problemLayers; }
    const QStringList& layersNotSaved() const { return _emptyLayers; }

private:
    QStringList                 _newPaths;
    QStringList                 _problemLayers;
    QStringList                 _emptyLayers;
    QVBoxLayout*                _rowsLayout;
    std::vector<LayerTreeItem*> _layerItems;
};

}; // namespace UsdLayerEditor

#endif // saveLAYERSDIALOG_H
