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

#ifndef COMPONENTSAVEDIALOG_H
#define COMPONENTSAVEDIALOG_H

#include <QtCore/QString>
#include <QtWidgets/QDialog>
#include <QtWidgets/QtWidgets>

#include <string>

class QKeyEvent;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QShowEvent;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;

namespace UsdLayerEditor {

class ComponentSaveWidget;

/**
 * @brief Dialog for saving a component with name and location fields.
 *
 */
class ComponentSaveDialog : public QDialog
{
    Q_OBJECT

public:
    ComponentSaveDialog(
        QWidget*           in_parent = nullptr,
        const std::string& proxyShapePath = std::string());
    ~ComponentSaveDialog() override;

    // Set the component name programmatically
    void setComponentName(const QString& name);

    // Set the folder location programmatically
    void setFolderLocation(const QString& location);

    // Get the component name
    QString componentName() const;

    // Get the folder location
    QString folderLocation() const;

protected:
    void showEvent(QShowEvent* event) override;

private Q_SLOTS:
    void onSaveStage();
    void onCancel();
    void onWidgetExpandedStateChanged(bool isExpanded);

private:
    void setupUI(const std::string& proxyShapePath);

    ComponentSaveWidget* _contentWidget;
    QPushButton*         _saveStageButton;
    QPushButton*         _cancelButton;
    int                  _originalHeight;
};

} // namespace UsdLayerEditor

#endif // COMPONENTSAVEDIALOG_H
