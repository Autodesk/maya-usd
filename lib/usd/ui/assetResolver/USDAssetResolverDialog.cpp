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

    Adsk::AdskResolverContext adskCtx = Adsk::AdskResolverContext();
    auto                      allSearchPaths = adskCtx.GetSearchPaths();
    QStringList               extAndEnvPathList;
    for (const auto& path : allSearchPaths) {
        // if its not already in userDataExt, add it
        if (std::find(userSearchPaths.begin(), userSearchPaths.end(), path)
            == userSearchPaths.end()) {
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
    {
        // prevent multiple notifications while we update context data
        // the notification will be sent at the end of this block
        // and will trigger the resolver to refresh
#if AR_ASSETRESOLVERCONTEXTDATA_HAS_PATHARRAY
        Adsk::PreventContextDataChangedNotification preventNotifications;
#endif

        auto allContextData = AssetResolverContextDataRegistry::GetAvailableContextData();
        // helper to set the state of a context data, adding it if it does not exist
        auto setContextDataState = [&allContextData](const std::string& name, bool active) {
            for (auto& contextData : allContextData) {
                if (contextData.first == name) {
                    contextData.second = active;
                    return;
                }
            }
            // introducing a new context data and its state
            allContextData.insert(allContextData.begin(), { name, active });
        };

        // Update user search paths
        {
            auto userSearchPathsContextData
                = Adsk::AssetResolverContextDataRegistry::GetContextData(
                    "MayaUsd_UserData", true);
            if (userSearchPathsContextData.has_value()) {
#if AR_ASSETRESOLVERCONTEXTDATA_HAS_PATHARRAY
                userSearchPathsContextData.value().get().searchPaths.Clear();
                userSearchPathsContextData.value().get().searchPaths.AddPaths(userSearchPaths);
#else
                userSearchPathsContextData.value().get().searchPaths. = userSearchPaths;
#endif
                setContextDataState("MayaUsd_UserData", true);
            } else {
                setContextDataState("MayaUsd_UserData", false);
            }

            // save the search paths to option var
            std::string optionVarUserSearchPathsStr = TfStringJoin(userSearchPaths, ";");
            MString     optionVarUserSearchPaths(optionVarUserSearchPathsStr.c_str());
            MGlobal::setOptionVarValue(
                "mayaUsd_AdskAssetResolverUserSearchPaths", optionVarUserSearchPaths);
        }

        // Update mapping file
        {
            auto mappingFileContent = Adsk::GetContextDataFromFile(mappingFilePath);
            if (mappingFileContent.has_value()) {
                auto mappingFileContextData
                    = Adsk::AssetResolverContextDataRegistry::GetContextData(
                        "MayaUsd_MappingFile", true);
                if (mappingFileContextData.has_value()) {
                    mappingFileContextData.value().get() = mappingFileContent.value();
                    setContextDataState("MayaUsd_MappingFile", true);
                }
            } else {
                Adsk::AssetResolverContextDataRegistry::RemoveContextData("MayaUsd_MappingFile");
                setContextDataState("MayaUsd_MappingFile", false);
            }

            // save mapping file path to option var
            MGlobal::setOptionVarValue(
                "mayaUsd_AdskAssetResolverMappingFile", MString(mappingFilePath.c_str()));
        }

        // Update project tokens
        {
            //// apply include project tokens changes if needed
            //if (includeMayaProjectTokens) {
            //    AssetResolverUtils::includeMayaProjectTokensInAdskAssetResolver();
            //} else {
            //    AssetResolverUtils::excludeMayaProjectTokensFromAdskAssetResolver();
            //}
            setContextDataState("MayaUsd_IncludeToken", includeMayaProjectTokens);
            MGlobal::setOptionVarValue(
                "mayaUsd_AdskAssetResolverIncludeMayaToken", includeMayaProjectTokens);
        }

        setContextDataState(
            AssetResolverContextDataRegistry::GetEnvironmentMappingContextDataName(),
            !userPathsOnly);

        // save option vars for user paths settings
        MGlobal::setOptionVarValue("mayaUsd_AdskAssetResolverUserPathsFirst", userPathsFirst);
        MGlobal::setOptionVarValue("mayaUsd_AdskAssetResolverUserPathsOnly", userPathsOnly);

        // now that we have processed options, we can make a list of the selected context data
        std::vector<std::string> selectedContextData;
        if (!userPathsFirst) {
            selectedContextData.push_back(
                AssetResolverContextDataRegistry::GetEnvironmentMappingContextDataName());
        }
        for (const auto& contextData : allContextData) {
            if (contextData.second) {
                selectedContextData.push_back(contextData.first);
            }
        }
        // ordering user search paths first if the option is set
        if (!userPathsOnly) {
            auto userIt = std::find(
                selectedContextData.begin(),
                selectedContextData.end(),
                "MayaUsd_UserData");
            auto envIt = std::find(
                selectedContextData.begin(),
                selectedContextData.end(),
                AssetResolverContextDataRegistry::GetEnvironmentMappingContextDataName());
            if (userPathsFirst && userIt != selectedContextData.end()
                && envIt != selectedContextData.end() && envIt > userIt) {
                // move user paths before environment paths
                std::swap(*envIt, *userIt);
            }
        }
        AssetResolverContextDataRegistry::SetActiveContextData(selectedContextData);
    }

#if AR_ASSETRESOLVERCONTEXTDATA_HAS_PATHARRAY
    Adsk::SendContextDataChanged(Adsk::ContextDataType::ALL);
#endif

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
