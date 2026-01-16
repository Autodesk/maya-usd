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

#include "AssetResolverUtils.h"
#include "PreferencesManagement.h"
#include "PreferencesOptions.h"
#include "USDAssetResolverSettingsWidget.h"

#include <mayaUsdUI/ui/IMayaMQtUtil.h>
#include <mayaUsdUI/ui/TreeModelFactory.h>

#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/iterator.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>

#include <stdexcept>

using namespace Adsk;

namespace MAYAUSD_NS_DEF {

USDAssetResolverDialog::USDAssetResolverDialog(QWidget* parent)
    : QDialog { parent }
{
    setParent(parent, windowFlags());

    // Set the default size for the dialog
    resize(800, 600);
    setWindowTitle("USD Asset Resolver Settings");

    auto extensions = AssetResolverContextDataRegistry::GetAvailableContextData();
    // get the user data, if not found, create it
    std::string userDataExtName = "MayaUsd_UserData";

    // Get Search Paths from option var
    MString optionVarUserSearchPaths
        = MGlobal::optionVarStringValue("mayaUsd_AdskAssetResolverUserSearchPaths");
    std::string optionVarUserSearchPathsStr(optionVarUserSearchPaths.asChar());
    userSearchPaths = TfStringSplit(optionVarUserSearchPathsStr, std::string(";"));

    userDataExt = AssetResolverContextDataRegistry::GetContextData(userDataExtName, false, false);
    if (userDataExt == std::nullopt) {
        // First time creating UserData extension, load the data from optionVar
        auto userContextExt
            = AssetResolverContextDataRegistry::RegisterContextData(userDataExtName);
        userDataExt
            = AssetResolverContextDataRegistry::GetContextData(userDataExtName, false, false);
#if AR_ASSETRESOLVERCONTEXTDATA_HAS_PATHARRAY
        userDataExt->get().searchPaths.Clear();
        userDataExt->get().searchPaths.AddPaths(userSearchPaths);
#else
        userDataExt->get().searchPaths = userSearchPaths;
#endif
    }

    QStringList               extAndEnvPathList;
    const auto envContextData = Adsk::AssetResolverContextDataRegistry::GetContextData(
        Adsk::AssetResolverContextDataRegistry::GetEnvironmentMappingContextDataName());
    const std::vector<std::string>& envContextSearchPaths = envContextData.value().get().searchPaths;
    if (envContextData.has_value()) {
        for (const auto& path : envContextSearchPaths) {
            extAndEnvPathList << QString::fromStdString(path);
        }
    }

    // Get include project tokens from option var
    includeMayaProjectTokens
        = MGlobal::optionVarIntValue("mayaUsd_AdskAssetResolverIncludeMayaToken");

    // Get mapping file path from option var
    static const MString AdskAssetResolverMappingFile = "mayaUsd_AdskAssetResolverMappingFile";
    mappingFilePath = MGlobal::optionVarStringValue(AdskAssetResolverMappingFile).asChar();

    Adsk::USDAssetResolverSettingsWidget* settingsWidget
        = new Adsk::USDAssetResolverSettingsWidget(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(settingsWidget);

    QObject::connect(
        settingsWidget,
        &Adsk::USDAssetResolverSettingsWidget::mappingFilePathChanged,
        this,
        &USDAssetResolverDialog::OnMappingFileChanged);
    settingsWidget->setMappingFilePath(QString::fromStdString(mappingFilePath));

    QObject::connect(
        settingsWidget,
        &Adsk::USDAssetResolverSettingsWidget::includeProjectTokensChanged,
        this,
        &USDAssetResolverDialog::OnIncludeProjectTokensChanged);
    settingsWidget->setIncludeProjectTokens(includeMayaProjectTokens);

    QObject::connect(
        settingsWidget,
        &Adsk::USDAssetResolverSettingsWidget::userPathsOnlyChanged,
        this,
        &USDAssetResolverDialog::OnUserPathsOnlyChanged);
    userPathsOnly = MGlobal::optionVarIntValue("mayaUsd_AdskAssetResolverUserPathsOnly");
    settingsWidget->setUserPathsOnly(userPathsOnly);

    QObject::connect(
        settingsWidget,
        &Adsk::USDAssetResolverSettingsWidget::userPathsFirstChanged,
        this,
        &USDAssetResolverDialog::OnUserPathsFirstChanged);
    if (MGlobal::optionVarExists("mayaUsd_AdskAssetResolverUserPathsFirst")) {
        userPathsFirst = MGlobal::optionVarIntValue("mayaUsd_AdskAssetResolverUserPathsFirst");
    } else {
        userPathsFirst = true;
    }
    settingsWidget->setUserPathsFirst(userPathsFirst);

    settingsWidget->setExtAndEnvPaths(extAndEnvPathList);
    QStringList userPathList;
    for (const auto& s : userSearchPaths)
        userPathList << QString::fromStdString(s);
    settingsWidget->setUserPaths(userPathList);

    QObject::connect(
        settingsWidget,
        &Adsk::USDAssetResolverSettingsWidget::userPathsChanged,
        this,
        &USDAssetResolverDialog::OnUserPathsChanged);
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
}

USDAssetResolverDialog::~USDAssetResolverDialog() { }

bool USDAssetResolverDialog::execute() { return exec() == QDialog::Accepted; }

void USDAssetResolverDialog::OnMappingFileChanged(const QString& path)
{
    mappingFilePath = path.toStdString();
}

void USDAssetResolverDialog::OnIncludeProjectTokensChanged(bool include)
{
    MGlobal::displayInfo("Include project tokens changed");
    includeMayaProjectTokens = include;
}

void USDAssetResolverDialog::OnSaveRequested()
{
    // Get the current preferences
    UsdPreferenceOptions oldOptions = PreferencesManagement::GetUsdPreferences();
    UsdPreferenceOptions newOptions;

    // Set the new options from dialog values
    newOptions.SetUserSearchPaths(userSearchPaths);
    newOptions.SetMappingFile(mappingFilePath);
    newOptions.SetUsingProjectTokens(includeMayaProjectTokens);
    newOptions.SetUsingUserSearchPathsFirst(userPathsFirst);
    newOptions.SetIncludingEnvironmentSearchPaths(!userPathsOnly);

    // Apply the changes to the asset resolver
    PreferencesManagement::ApplyUsdPreferences(oldOptions, newOptions);

    // Save the preferences to Maya option vars
    PreferencesManagement::SaveUsdPreferences(newOptions);

    // Also close the window
    accept();
}

void USDAssetResolverDialog::OnUserPathsChanged(const QStringList& paths)
{
    userSearchPaths.clear();
    for (const QString& path : paths) {
        userSearchPaths.push_back(path.toStdString());
        std::string pathStr = path.toStdString();
    }
}

void USDAssetResolverDialog::OnUserPathsFirstChanged(bool ifUserPathsFirst)
{
    userPathsFirst = ifUserPathsFirst;
}

void USDAssetResolverDialog::OnUserPathsOnlyChanged(bool ifUserPathsOnly)
{
    userPathsOnly = ifUserPathsOnly;
}

void USDAssetResolverDialog::OnCloseRequested() { accept(); }

} // namespace MAYAUSD_NS_DEF
