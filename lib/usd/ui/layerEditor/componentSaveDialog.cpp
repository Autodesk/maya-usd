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

#include "componentSaveDialog.h"

#include "generatedIconButton.h"
#include "qtUtils.h"

#include <mayaUsd/utils/utilComponentCreator.h>

#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtGui/QIcon>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFileIconProvider>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStyle>
#include <QtWidgets/QTreeWidget>

#include <string>

namespace {
const char* SHOW_MORE_TEXT = "<a href=\"#\">Show More</a>";
const char* SHOW_LESS_TEXT = "<a href=\"#\">Show Less</a>";

const char* getScenesFolderScript = R"(
    global proc string UsdMayaUtilFileSystem_GetScenesFolder()
    {
        string $workspaceLocation = `workspace -q -fn`;
        string $scenesFolder = `workspace -q -fileRuleEntry "scene"`;
        $sceneFolder = $workspaceLocation + "/" + $scenesFolder;

        return $sceneFolder;
    }
    UsdMayaUtilFileSystem_GetScenesFolder;
    )";

// This function was copied from utilFileSystem.cpp.
// Including the headers in this class that include USD headers
// was causing transitive compilation issues between QT headers
// and USD headers in MayaUsd 2023.
std::string getMayaWorkspaceScenesDir()
{
    MString scenesFolder;
    ::MGlobal::executeCommand(
        getScenesFolderScript,
        scenesFolder,
        /*display*/ false,
        /*undo*/ false);

    return std::string(scenesFolder.asChar(), scenesFolder.length());
}

} // namespace

namespace UsdLayerEditor {

ComponentSaveDialog::ComponentSaveDialog(QWidget* in_parent, const std::string& proxyShapePath)
    : QDialog(in_parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
    , _nameEdit(nullptr)
    , _locationEdit(nullptr)
    , _browseButton(nullptr)
    , _showMoreLabel(nullptr)
    , _saveStageButton(nullptr)
    , _cancelButton(nullptr)
    , _treeScrollArea(nullptr)
    , _treeWidget(nullptr)
    , _treeContainer(nullptr)
    , _isExpanded(false)
    , _originalHeight(0)
    , _proxyShapePath(proxyShapePath)
    , _lastComponentName()
{
    setupUI();
}

ComponentSaveDialog::~ComponentSaveDialog() { }

void ComponentSaveDialog::setupUI()
{
    // Main vertical layout
    auto mainLayout = new QVBoxLayout();
    QtUtils::initLayoutMargins(mainLayout);
    mainLayout->setSpacing(0);

    // Content widget with padding
    auto contentWidget = new QWidget();
    auto contentLayout = new QGridLayout();

    // Set padding: left, top, right, bottom
    contentLayout->setContentsMargins(DPIScale(20), DPIScale(15), DPIScale(20), DPIScale(15));
    contentLayout->setSpacing(DPIScale(10));

    // Column stretch factors: 1/6, 4/6, 1/24, 3/24
    // Convert to integers: 4/24, 16/24, 1/24, 3/24
    // Use 24 as base: 4, 16, 1, 3
    contentLayout->setColumnStretch(0, 4);  // 1/6 = 4/24
    contentLayout->setColumnStretch(1, 16); // 4/6 = 16/24
    contentLayout->setColumnStretch(2, 1);  // 1/24
    contentLayout->setColumnStretch(3, 3);  // 3/24

    // First row, first column: "Name" label
    auto nameLabel = new QLabel("Name", this);
    contentLayout->addWidget(nameLabel, 0, 0);

    // First row, second column: "Location" label
    auto locationLabel = new QLabel("Location", this);
    contentLayout->addWidget(locationLabel, 0, 1);

    // Second row, first column: Name textbox
    _nameEdit = new QLineEdit(this);
    contentLayout->addWidget(_nameEdit, 1, 0);

    // Second row, second column: Location textbox
    _locationEdit = new QLineEdit(this);
    contentLayout->addWidget(_locationEdit, 1, 1);

    // Second row, third column: Folder browse button
    QIcon folderIcon = utils->createIcon(":/fileOpen.png");
    _browseButton = new GeneratedIconButton(this, folderIcon);
    _browseButton->setToolTip("Browse for folder");
    connect(_browseButton, &QAbstractButton::clicked, this, &ComponentSaveDialog::onBrowseFolder);
    contentLayout->addWidget(_browseButton, 1, 2);

    // Second row, fourth column: "Show More" clickable label
    _showMoreLabel = new QLabel(SHOW_MORE_TEXT, this);
    _showMoreLabel->setOpenExternalLinks(false);
    _showMoreLabel->setTextFormat(Qt::RichText);
    connect(_showMoreLabel, &QLabel::linkActivated, this, &ComponentSaveDialog::onShowMore);
    contentLayout->addWidget(_showMoreLabel, 1, 3, Qt::AlignLeft | Qt::AlignVCenter);

    contentWidget->setLayout(contentLayout);
    mainLayout->addWidget(contentWidget);

    // Tree view container (initially hidden)
    _treeContainer = new QWidget(this);
    auto treeLayout = new QVBoxLayout();
    treeLayout->setContentsMargins(DPIScale(20), 0, DPIScale(20), DPIScale(15));
    treeLayout->setSpacing(DPIScale(10));

    auto treeLabel = new QLabel("The following file structure is created on save.", this);
    treeLayout->addWidget(treeLabel);

    _treeScrollArea = new QScrollArea(this);
    _treeScrollArea->setWidgetResizable(true);
    _treeScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _treeScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _treeScrollArea->setMinimumHeight(DPIScale(250));
    _treeScrollArea->setMaximumHeight(DPIScale(300));

    // Use QStackedWidget to switch between tree and "nothing to preview" label
    auto stackedWidget = new QStackedWidget(this);

    // Tree widget page
    _treeWidget = new QTreeWidget(this);
    _treeWidget->setHeaderHidden(true);
    _treeWidget->setRootIsDecorated(true);
    _treeWidget->setAlternatingRowColors(false);
    stackedWidget->addWidget(_treeWidget);

    // "Nothing to preview" label page - centered
    auto nothingToPreviewWidget = new QWidget(this);
    nothingToPreviewWidget->setMinimumHeight(DPIScale(250));
    auto nothingLayout = new QVBoxLayout();
    nothingLayout->setContentsMargins(0, 0, 0, 0);
    nothingLayout->setSpacing(0);
    auto nothingToPreviewLabel = new QLabel(
        "Nothing to preview since no information about the template is available.",
        nothingToPreviewWidget);
    nothingToPreviewLabel->setAlignment(Qt::AlignCenter);
    nothingLayout->addStretch();
    nothingLayout->addWidget(nothingToPreviewLabel, 0, Qt::AlignCenter);
    nothingLayout->addStretch();
    nothingToPreviewWidget->setLayout(nothingLayout);
    stackedWidget->addWidget(nothingToPreviewWidget);

    // Start with tree widget visible
    stackedWidget->setCurrentWidget(_treeWidget);

    _treeScrollArea->setWidget(stackedWidget);
    treeLayout->addWidget(_treeScrollArea);

    _treeContainer->setLayout(treeLayout);
    _treeContainer->setVisible(false);
    mainLayout->addWidget(_treeContainer);

    // Button layout (bottom right)
    auto buttonLayout = new QHBoxLayout();
    QtUtils::initLayoutMargins(buttonLayout, DPIScale(10));
    buttonLayout->setSpacing(DPIScale(10));
    buttonLayout->addStretch();

    _saveStageButton = new QPushButton("Save Stage", this);
    _saveStageButton->setDefault(true);
    connect(_saveStageButton, &QPushButton::clicked, this, &ComponentSaveDialog::onSaveStage);
    buttonLayout->addWidget(_saveStageButton);

    _cancelButton = new QPushButton("Cancel", this);
    connect(_cancelButton, &QPushButton::clicked, this, &ComponentSaveDialog::onCancel);
    buttonLayout->addWidget(_cancelButton);

    auto buttonWidget = new QWidget(this);
    buttonWidget->setLayout(buttonLayout);
    buttonWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    mainLayout->addWidget(buttonWidget);

    setLayout(mainLayout);
    setWindowTitle("Save Component");
    setFixedWidth(600);
    // Don't set fixed height initially - let it size naturally, then we'll fix it after first show
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
}

void ComponentSaveDialog::setComponentName(const QString& name)
{
    if (_nameEdit) {
        _nameEdit->setText(name);
    }
}

void ComponentSaveDialog::setFolderLocation(const QString& location)
{
    if (_locationEdit) {
        _locationEdit->setText(location);
    }
}

QString ComponentSaveDialog::componentName() const
{
    return _nameEdit ? _nameEdit->text() : QString();
}

QString ComponentSaveDialog::folderLocation() const
{
    return _locationEdit ? _locationEdit->text() : QString();
}

void ComponentSaveDialog::onBrowseFolder()
{
    QString currentPath = _locationEdit->text();
    // default to maya-usd scene folder
    if (currentPath.isEmpty()) {
        currentPath = getMayaWorkspaceScenesDir().c_str();
    }

    QString selectedFolder = QFileDialog::getExistingDirectory(
        this,
        "Select Folder",
        currentPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!selectedFolder.isEmpty()) {
        _locationEdit->setText(selectedFolder);
    }
}

void ComponentSaveDialog::keyPressEvent(QKeyEvent* event)
{
    // Intercept Enter/Return key when focus is on _nameEdit
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && _nameEdit
        && _nameEdit->hasFocus()) {
        // If tree view is minimized (not expanded), always accept the dialog
        if (!_isExpanded) {
            QDialog::keyPressEvent(event);
            return;
        }

        // If tree view is expanded, check if name has changed
        QString currentName = _nameEdit->text();
        if (currentName != _lastComponentName) {
            // Name has changed - update tree view and prevent dialog acceptance
            updateTreeView();
            event->accept();
            return;
        }
        // Name hasn't changed - let default behavior proceed (accept dialog)
    }
    QDialog::keyPressEvent(event);
}

void ComponentSaveDialog::onSaveStage() { accept(); }

void ComponentSaveDialog::onCancel() { reject(); }

void ComponentSaveDialog::populateTreeView(const QJsonObject& jsonObj, QTreeWidgetItem* parentItem)
{
    // Get standard icons for folders and files
    QFileIconProvider iconProvider;
    QIcon             folderIcon = iconProvider.icon(QFileIconProvider::Folder);
    QIcon             fileIcon = iconProvider.icon(QFileIconProvider::File);

    // Fallback to style icons if file icon provider doesn't work
    if (folderIcon.isNull()) {
        folderIcon = style()->standardIcon(QStyle::SP_DirIcon);
    }
    if (fileIcon.isNull()) {
        fileIcon = style()->standardIcon(QStyle::SP_FileIcon);
    }

    for (auto it = jsonObj.begin(); it != jsonObj.end(); ++it) {
        QString    key = it.key();
        QJsonValue value = it.value();

        QTreeWidgetItem* item = nullptr;
        if (parentItem) {
            item = new QTreeWidgetItem(parentItem);
        } else {
            item = new QTreeWidgetItem(_treeWidget);
        }
        item->setText(0, key);

        if (value.isBool()) {
            // Boolean value represents a file
            item->setIcon(0, fileIcon);
        } else if (value.isObject()) {
            // Object represents a folder
            item->setIcon(0, folderIcon);
            item->setExpanded(true);
            populateTreeView(value.toObject(), item);
        }
    }
}

void ComponentSaveDialog::toggleExpandedState()
{
    if (_isExpanded) {
        // Collapsing: hide tree and restore original size
        _isExpanded = false;
        _showMoreLabel->setText(SHOW_MORE_TEXT);
        _treeContainer->setVisible(false);

        // Restore original height
        setFixedHeight(_originalHeight);
    } else {
        // Expanding: capture current height and show tree
        _isExpanded = true;
        // Capture current height before expanding (compact state)
        // For whatever reason, capturing this value at the end
        // of the SetupUI functions isn't getting the right value
        // from height().
        if (_originalHeight == 0) {
            _originalHeight = height();
        }

        _showMoreLabel->setText(SHOW_LESS_TEXT);
        _treeContainer->setVisible(true);
        // Expand dialog by ~300px
        setFixedHeight(_originalHeight + DPIScale(300));
    }
}

void ComponentSaveDialog::updateTreeView()
{
    std::string saveLocation(_locationEdit->text().toStdString());
    std::string componentName(_nameEdit->text().toStdString());

    const auto result = MayaUsd::ComponentUtils::previewSaveAdskUsdComponent(
        saveLocation, componentName, _proxyShapePath.c_str());

    QString jsonStr = QString::fromStdString(result);

    // Clear existing tree first
    _treeWidget->clear();

    QStackedWidget* stackedWidget = qobject_cast<QStackedWidget*>(_treeScrollArea->widget());

    QJsonParseError parseError;
    QJsonDocument   doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
    QJsonObject     jsonObj;
    bool            hasData = false;

    if (parseError.error == QJsonParseError::NoError && doc.isObject() && !doc.isEmpty()) {
        jsonObj = doc.object();
        hasData = !jsonObj.isEmpty();
    }

    if (hasData) {
        // Populate tree view with JSON data and show tree
        populateTreeView(jsonObj);
        if (stackedWidget) {
            stackedWidget->setCurrentIndex(0);
        }
    } else {
        // Show "Nothing to preview" message
        if (stackedWidget) {
            stackedWidget->setCurrentIndex(1);
        }
    }

    // Update last component name
    _lastComponentName = _nameEdit->text();
}

void ComponentSaveDialog::onShowMore()
{
    // If collapsing, just toggle
    if (_isExpanded) {
        toggleExpandedState();
        return;
    }

    // If expanding, fetch data first
    updateTreeView();

    // Always expand the dialog
    toggleExpandedState();
}

} // namespace UsdLayerEditor
