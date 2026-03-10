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
#include "USDAssetResolverDialog.h"

#include "PreferencesManagement.h"
#include "PreferencesOptions.h"

#include <AssetResolverPreferences/USDAssetResolverSettingsWidget.h>
#include <AssetResolverPreferences/AssetResolverSettingsManagement.h>

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>

namespace MAYAUSD_NS_DEF {

USDAssetResolverDialog::USDAssetResolverDialog(QWidget* parent)
    : QDialog { parent }
{
    setParent(parent, windowFlags());

    // Set the default size for the dialog
    resize(800, 600);
    setWindowTitle("USD Asset Resolver Settings");

    // Create the settings widget
    settingsWidget = new Adsk::USDAssetResolverSettingsWidget(
        Adsk::AssetResolverSettingsManagement::FillSettingsWithExtensions(), this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(settingsWidget);

    QWidget* saveAndCloseWidget = new QWidget(this);
    QHBoxLayout* saveAndCloselayout = new QHBoxLayout(saveAndCloseWidget);
    QPushButton* saveButtonBox = new QPushButton(tr("Save && Refresh"), this);
    saveButtonBox->setDefault(true);
    saveButtonBox->setAutoDefault(true);
    saveButtonBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    saveButtonBox->setToolTip(tr("Save settings"));
    saveAndCloselayout->addWidget(saveButtonBox);
    QPushButton* closeButtonBox = new QPushButton(tr("Close"), this);
    closeButtonBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    saveAndCloselayout->addWidget(closeButtonBox);
    layout->addWidget(saveAndCloseWidget);

    // Connect only the action signals (save and close)
    QObject::connect(
        saveButtonBox,
        &QPushButton::clicked,
        this,
        &USDAssetResolverDialog::OnSaveRequested);
    QObject::connect(
        closeButtonBox, 
        &QPushButton::clicked,
        this,
        &USDAssetResolverDialog::OnCloseRequested);
}

USDAssetResolverDialog::~USDAssetResolverDialog() { }

bool USDAssetResolverDialog::execute() { return exec() == QDialog::Accepted; }

void USDAssetResolverDialog::OnSaveRequested()
{
    auto newOptions = settingsWidget->getSettings();
    Adsk::AssetResolverSettingsManagement::ApplySettings(
        Adsk::AssetResolverSettings::GetInstance(), newOptions);
    PreferencesManagement::SaveUsdPreferences(newOptions);

    // Close the dialog
    accept();
}

void USDAssetResolverDialog::OnCloseRequested() { accept(); }

} // namespace MAYAUSD_NS_DEF
