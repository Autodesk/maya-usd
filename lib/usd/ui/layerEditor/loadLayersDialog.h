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

#ifndef LOADLAYERSDIALOG_H
#define LOADLAYERSDIALOG_H

#include <QtWidgets/QDialog>
#include <QtWidgets/QtWidgets>

#include <list>
#include <string>

class QCheckBox;
class QLabel;
class QLineEdit;
class QScrollArea;
class QVBoxLayout;

namespace UsdLayerEditor {

class LayerTreeItem;
class LayerTreeView;
class LayerPathRow;

/**
 * @brief Dialog to load multiple USD sublayers at once onto a parent layer.
 *
 */
class LoadLayersDialog : public QDialog
{
public:
    typedef std::list<std::string> PathList;

    LoadLayersDialog(LayerTreeItem* in_treeItem, QWidget* in_parent);
    const PathList& pathsToLoad() { return _pathToLoad; }

    std::string findDirectoryToUse(const std::string& rowText) const;

protected:
    // slots
    void onOk();
    void onCancel();

public:
    // slots connected by LayerPathRow class
    void onOpenBrowser();
    void onDeleteRow();
    void onAddRow();

protected:
    PathList       _pathToLoad;
    LayerTreeItem* _treeItem;
    QVBoxLayout*   _rowsLayout;
    QScrollArea*   _scrollArea;

    LayerPathRow* getLastRow() const;
    void          appendInserterRow();
    void          adjustScrollArea();
    void          scrollToEnd();
};

class LayerPathRow : public QWidget
{
public:
    LayerPathRow(LoadLayersDialog* in_parent);
    void setAsRowInserter(bool setIt);

    // returns the path to store, either the rel or abs path
    std::string pathToUse() const;
    // returns the absolute path, always
    std::string absolutePath() const { return _absolutePath; }
    // sets the absolute path
    void setAbsolutePath(const std::string& path);

protected:
    void onTextChanged(const QString& text);
    void onRelativeButtonChecked(bool checked);

protected:
    std::string       _absolutePath;
    std::string       _parentPath;
    LoadLayersDialog* _parent;
    QLabel*           _label;
    QLineEdit*        _pathEdit;
    QAbstractButton*  _openBrowser;
    QAbstractButton*  _trashIcon;
    QAbstractButton*  _addPathIcon;
    QCheckBox*        _convertToRel;
};

}; // namespace UsdLayerEditor

#endif // LOADLAYERSDIALOG_H
