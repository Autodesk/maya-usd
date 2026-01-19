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
#include "USDAssetResolverSettingsWidget.h"

#include <QtWidgets/QVBoxLayout>

namespace MAYAUSD_NS_DEF {

USDAssetResolverDialog::USDAssetResolverDialog(QWidget* parent)
    : QDialog { parent }
{
    setParent(parent, windowFlags());

    // Set the default size for the dialog
    resize(800, 600);
    setWindowTitle("USD Asset Resolver Settings");

    // Create the settings widget
    settingsWidget = new Adsk::USDAssetResolverSettingsWidget(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(settingsWidget);

    // Connect only the action signals (save and close)
    QObject::connect(
        settingsWidget,
        &Adsk::USDAssetResolverSettingsWidget::saveRequested,
        this,
        &USDAssetResolverDialog::OnSaveRequested);

    QObject::connect(
        settingsWidget,
        &Adsk::USDAssetResolverSettingsWidget::closeRequested,
        this,
        &USDAssetResolverDialog::OnCloseRequested);

    // Load current preferences into the dialog
    loadOptions(PreferencesManagement::GetUsdPreferences());
}

USDAssetResolverDialog::~USDAssetResolverDialog() { }

bool USDAssetResolverDialog::execute() { return exec() == QDialog::Accepted; }

void USDAssetResolverDialog::loadOptions(const UsdPreferenceOptions& options)
{
    if (settingsWidget) {
        settingsWidget->setIncludeProjectTokens(options.IsUsingProjectTokens());
        settingsWidget->setMappingFilePath(QString::fromStdString(options.GetMappingFile()));

        settingsWidget->setUserPathsFirst(options.IsUsingUserSearchPathsFirst());
        settingsWidget->setUserPathsOnly(!options.IsIncludingEnvironmentSearchPaths());

        QStringList qUserPaths;
        for (const auto& path : options.GetUserSearchPaths()) {
            qUserPaths.append(QString::fromStdString(path));
        }
        settingsWidget->setUserPaths(qUserPaths);

        QStringList qEnvPaths;
        for (const auto& path : options.GetEnvironmentSearchPaths()) {
            qEnvPaths.append(QString::fromStdString(path));
        }
        settingsWidget->setExtAndEnvPaths(qEnvPaths);
    }
}

const UsdPreferenceOptions USDAssetResolverDialog::getOptions() const
{
    UsdPreferenceOptions options;

    if (settingsWidget) {
        options.SetUsingProjectTokens(settingsWidget->includeProjectTokens());
        options.SetMappingFile(settingsWidget->mappingFilePath().toStdString());

        std::vector<std::string> userPaths;
        for (auto qPath : settingsWidget->userPaths()) {
            userPaths.push_back(qPath.toStdString());
        }
        options.SetUserSearchPaths(userPaths);
        options.SetUsingUserSearchPathsFirst(settingsWidget->userPathsFirst());
        options.SetIncludingEnvironmentSearchPaths(!settingsWidget->userPathsOnly());
    }

    return options;
}

void USDAssetResolverDialog::OnSaveRequested()
{
    // Get the current preferences
    UsdPreferenceOptions oldOptions = PreferencesManagement::GetUsdPreferences();

    // Get the new options from the dialog UI
    UsdPreferenceOptions newOptions = getOptions();

    // Apply the changes to the asset resolver
    PreferencesManagement::ApplyUsdPreferences(oldOptions, newOptions);

    // Save the preferences to Maya option vars
    PreferencesManagement::SaveUsdPreferences(newOptions);

    // Close the dialog
    accept();
}

void USDAssetResolverDialog::OnCloseRequested() { accept(); }

} // namespace MAYAUSD_NS_DEF
