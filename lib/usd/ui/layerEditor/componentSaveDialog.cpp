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

#include "componentSaveWidget.h"
#include "qtUtils.h"

#include <QtCore/QTimer>
#include <QtGui/QShowEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

namespace UsdLayerEditor {

ComponentSaveDialog::ComponentSaveDialog(QWidget* in_parent, const std::string& proxyShapePath)
    : QDialog(in_parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
    , _contentWidget(nullptr)
    , _saveStageButton(nullptr)
    , _cancelButton(nullptr)
    , _originalHeight(0)
{
    setupUI(proxyShapePath);
}

ComponentSaveDialog::~ComponentSaveDialog() { }

void ComponentSaveDialog::setupUI(const std::string& proxyShapePath)
{
    // Main vertical layout
    auto mainLayout = new QVBoxLayout();
    QtUtils::initLayoutMargins(mainLayout);
    mainLayout->setSpacing(0);

    // Create the content widget
    _contentWidget = new ComponentSaveWidget(this, proxyShapePath);
    connect(
        _contentWidget,
        &ComponentSaveWidget::expandedStateChanged,
        this,
        &ComponentSaveDialog::onWidgetExpandedStateChanged);
    mainLayout->addWidget(_contentWidget);

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
    if (_contentWidget) {
        _contentWidget->setComponentName(name);
    }
}

void ComponentSaveDialog::setFolderLocation(const QString& location)
{
    if (_contentWidget) {
        _contentWidget->setFolderLocation(location);
    }
}

QString ComponentSaveDialog::componentName() const
{
    return _contentWidget ? _contentWidget->componentName() : QString();
}

QString ComponentSaveDialog::folderLocation() const
{
    return _contentWidget ? _contentWidget->folderLocation() : QString();
}

void ComponentSaveDialog::onSaveStage() { accept(); }

void ComponentSaveDialog::onCancel() { reject(); }

void ComponentSaveDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // Capture the original height after layout is complete (in collapsed state)
    if (_originalHeight == 0) {
        QTimer::singleShot(0, this, [this]() {
            if (_originalHeight == 0) {
                _originalHeight = height();
            }
        });
    }
}

void ComponentSaveDialog::onWidgetExpandedStateChanged(bool isExpanded)
{
    // Ensure we have the original height (should be captured in showEvent, but fallback here)
    if (_originalHeight == 0) {
        _originalHeight = height();
    }
    
    if (isExpanded) {
        // Expand dialog by ~300px
        setFixedHeight(_originalHeight + DPIScale(300));
    } else {
        // Restore original height
        setFixedHeight(_originalHeight);
    }
}

} // namespace UsdLayerEditor
