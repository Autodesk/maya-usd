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

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#include <mayaUsd/utils/utilSerialization.h>

namespace UsdLayerEditor {

ComponentSaveDialog::ComponentSaveDialog(QWidget* in_parent)
    : QDialog(in_parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
    , _nameEdit(nullptr)
    , _locationEdit(nullptr)
    , _browseButton(nullptr)
    , _showMoreLabel(nullptr)
    , _saveStageButton(nullptr)
    , _cancelButton(nullptr)
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
    contentLayout->setContentsMargins(
        DPIScale(20), DPIScale(15), DPIScale(20), DPIScale(15));
    contentLayout->setSpacing(DPIScale(10));

    // Column stretch factors: 1/6, 4/6, 1/24, 3/24
    // Convert to integers: 4/24, 16/24, 1/24, 3/24
    // Use 24 as base: 4, 16, 1, 3
    contentLayout->setColumnStretch(0, 4);   // 1/6 = 4/24
    contentLayout->setColumnStretch(1, 16);  // 4/6 = 16/24
    contentLayout->setColumnStretch(2, 1);   // 1/24
    contentLayout->setColumnStretch(3, 3);   // 3/24

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
    _showMoreLabel = new QLabel("<a href=\"#\">Show More</a>", this);
    _showMoreLabel->setOpenExternalLinks(false);
    _showMoreLabel->setTextFormat(Qt::RichText);
    connect(_showMoreLabel, &QLabel::linkActivated, this, &ComponentSaveDialog::onShowMore);
    contentLayout->addWidget(_showMoreLabel, 1, 3, Qt::AlignLeft | Qt::AlignVCenter);

    contentWidget->setLayout(contentLayout);
    mainLayout->addWidget(contentWidget);

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
        currentPath = MayaUsd::utils::getSceneFolder().c_str();
    }

    QString selectedFolder = QFileDialog::getExistingDirectory(
        this, "Select Folder", currentPath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!selectedFolder.isEmpty()) {
        _locationEdit->setText(selectedFolder);
    }
}

void ComponentSaveDialog::onSaveStage()
{
    accept();
}

void ComponentSaveDialog::onCancel()
{
    reject();
}

void ComponentSaveDialog::onShowMore()
{
    // Placeholder for future implementation
}

} // namespace UsdLayerEditor

