//
// Copyright 2025 Autodesk
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

#ifndef COMPONENTSAVEWIDGET_H
#define COMPONENTSAVEWIDGET_H

#include <QtCore/QString>
#include <QtWidgets/QWidget>

#include <string>

class QJsonObject;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QScrollArea;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;

namespace UsdLayerEditor {

class GeneratedIconButton;

/**
 * @brief Widget for component save form with name, location, and preview tree view.
 * This widget can be reused in multiple dialogs or contexts.
 *
 */
class ComponentSaveWidget : public QWidget
{
    Q_OBJECT

public:
    ComponentSaveWidget(
        QWidget*           in_parent = nullptr,
        const std::string& proxyShapePath = std::string());
    ~ComponentSaveWidget() override;

    // Set the component name programmatically
    void setComponentName(const QString& name);

    // Set the folder location programmatically
    void setFolderLocation(const QString& location);

    // Get the component name
    QString componentName() const;

    // Get the folder location
    QString folderLocation() const;

    // Get the current expanded state
    bool isExpanded() const { return _isExpanded; }

    // Get the original height (before expansion)
    int originalHeight() const { return _originalHeight; }

    // Set the original height (used by parent dialog for sizing)
    void setOriginalHeight(int height) { _originalHeight = height; }

    // Set compact representation mode (hides Name/Location labels)
    void setCompactMode(bool compact);

    // Get compact representation mode
    bool isCompactMode() const { return _isCompact; }

    // Get the proxy shape path
    const std::string& proxyShapePath() const { return _proxyShapePath; }

Q_SIGNALS:
    // Emitted when the widget expands or collapses
    void expandedStateChanged(bool isExpanded);

protected:
    void keyPressEvent(QKeyEvent* event) override;

private Q_SLOTS:
    void onBrowseFolder();
    void onShowMore();

private:
    void setupUI();
    void populateTreeView(const QJsonObject& jsonObj, QTreeWidgetItem* parentItem = nullptr);
    void toggleExpandedState();
    void updateTreeView();

    QLineEdit*           _nameEdit;
    QLineEdit*           _locationEdit;
    GeneratedIconButton* _browseButton;
    QLabel*              _showMoreLabel;
    QLabel*              _nameLabel;
    QLabel*              _locationLabel;
    QScrollArea*         _treeScrollArea;
    QTreeWidget*         _treeWidget;
    QWidget*             _treeContainer;
    bool                 _isExpanded;
    bool                 _isCompact;
    int                  _originalHeight;
    std::string          _proxyShapePath;
    QString              _lastComponentName;
};

} // namespace UsdLayerEditor

#endif // COMPONENTSAVEWIDGET_H
