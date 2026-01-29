//
// Copyright 2026 Autodesk
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
#include "PreferencesManagement.h"

#include "AssetResolverUtils.h"
#include "PreferencesOptions.h"

#include <AdskAssetResolver/AssetResolverContextDataRegistry.h>

#ifdef AR_ASSETRESOLVERCONTEXTDATA_HAS_PATHARRAY
#include <AdskAssetResolver/Notice.h>
#endif

#include <algorithm>

namespace MAYAUSD_NS_DEF {
namespace PreferencesManagement {

// Names of the asset resolver context data sets
static const std::string PREFERENCE_MAPPING_FILE_DATA_SET_NAME = "MayaUsd_MappingFile";
static const std::string SESSION_USER_PATHS_DATA_SET_NAME = "MayaUsd_UserData";
static const std::string PROJECT_TOKENS_DATA_SET_NAME = "MayaUSDExtension";

void InitializeUsdPreferences()
{
    // Load USD Preference options to ensure the Adsk Asset Resolver works as configured
    // (The UsdPreferenceOptions::GetInstance() will take care of the loading)
    ApplyUsdPreferences(UsdPreferenceOptions(), UsdPreferenceOptions::GetInstance());
}

const UsdPreferenceOptions GetUsdPreferences()
{
    UsdPreferenceOptions options = UsdPreferenceOptions::GetInstance();

    // Fill in the environment search paths from the context data manager
    const auto envContextData = Adsk::AssetResolverContextDataRegistry::GetContextData(
        Adsk::AssetResolverContextDataRegistry::GetEnvironmentMappingContextDataName());
    if (envContextData.has_value()) {
        options.SetEnvironmentSearchPaths(envContextData.value().get().searchPaths);
    }

    return options;
}

void ApplyUsdPreferences(
    const UsdPreferenceOptions& options,
    const UsdPreferenceOptions& newOptions)
{
    // Track if any context data changed
    bool somethingChanged = false;
    {
        // Prevent multiple notifications while we update context data
        // The notification will be sent at the end of this block
        // and will trigger the resolver to refresh
#if AR_ASSETRESOLVERCONTEXTDATA_HAS_PATHARRAY
        Adsk::PreventContextDataChangedNotification preventNotifications;
#endif

        auto allContextData = Adsk::AssetResolverContextDataRegistry::GetAvailableContextData();

        // Helper to set the state of a context data, adding it if it does not exist
        auto setContextDataState
            = [&allContextData, &somethingChanged](const std::string& name, bool active) {
                  for (auto& contextData : allContextData) {
                      if (contextData.first == name) {
                          if (contextData.second != active) {
                              somethingChanged = true;
                          }
                          contextData.second = active;
                          return;
                      }
                  }
                  // Introducing a new context data and its state
                  somethingChanged = true;
                  allContextData.insert(allContextData.begin(), { name, active });
              };

        // Update user search paths
        if (options.GetUserSearchPaths() != newOptions.GetUserSearchPaths()) {
            somethingChanged = true;
            auto userSearchPathsContextData
                = Adsk::AssetResolverContextDataRegistry::GetContextData(
                    SESSION_USER_PATHS_DATA_SET_NAME, true);
            if (userSearchPathsContextData.has_value()) {
#if AR_ASSETRESOLVERCONTEXTDATA_HAS_PATHARRAY
                userSearchPathsContextData.value().get().searchPaths.Clear();
                userSearchPathsContextData.value().get().searchPaths.AddPaths(
                    newOptions.GetUserSearchPaths());
#else
                userSearchPathsContextData.value().get().searchPaths
                    = newOptions.GetUserSearchPaths();
#endif
                setContextDataState(SESSION_USER_PATHS_DATA_SET_NAME, true);
            } else {
                setContextDataState(SESSION_USER_PATHS_DATA_SET_NAME, false);
            }
        }

        // Update mapping file
        if (options.GetMappingFile() != newOptions.GetMappingFile()) {
            auto mappingFileContent = Adsk::GetContextDataFromFile(newOptions.GetMappingFile());
            if (mappingFileContent.has_value()) {
                auto preferenceMappingFileContextData
                    = Adsk::AssetResolverContextDataRegistry::GetContextData(
                        PREFERENCE_MAPPING_FILE_DATA_SET_NAME, true);
                if (preferenceMappingFileContextData.has_value()) {
                    preferenceMappingFileContextData.value().get() = mappingFileContent.value();
                    setContextDataState(PREFERENCE_MAPPING_FILE_DATA_SET_NAME, true);
                }
            } else {
                Adsk::AssetResolverContextDataRegistry::RemoveContextData(
                    PREFERENCE_MAPPING_FILE_DATA_SET_NAME);
                setContextDataState(PREFERENCE_MAPPING_FILE_DATA_SET_NAME, false);
            }
        }

        // Update project tokens
        setContextDataState(PROJECT_TOKENS_DATA_SET_NAME, newOptions.IsUsingProjectTokens());

        // Update environment search paths inclusion
        setContextDataState(
            Adsk::AssetResolverContextDataRegistry::GetEnvironmentMappingContextDataName(),
            newOptions.IsIncludingEnvironmentSearchPaths());

        // Now that we have processed options, we can make a list of the selected context data
        std::vector<std::string> selectedContextData;
        for (const auto& contextData : allContextData) {
            if (contextData.second) {
                selectedContextData.push_back(contextData.first);
            }
        }

        // Ordering user search paths first if the option is set
        if (newOptions.IsIncludingEnvironmentSearchPaths()) {
            auto userIt = std::find(
                selectedContextData.begin(),
                selectedContextData.end(),
                SESSION_USER_PATHS_DATA_SET_NAME);
            auto envIt = std::find(
                selectedContextData.begin(),
                selectedContextData.end(),
                Adsk::AssetResolverContextDataRegistry::GetEnvironmentMappingContextDataName());
            if (userIt != selectedContextData.end() && envIt != selectedContextData.end()) {
                if ((newOptions.IsUsingUserSearchPathsFirst()
                     && envIt < userIt /* 'env' appears before 'user' */)
                    || (!newOptions.IsUsingUserSearchPathsFirst()
                        && envIt > userIt /* 'env' appears after 'user' */)) {
                    // Reorder user paths and environment paths context data
                std::swap(*envIt, *userIt);
            }
        }

        if (Adsk::AssetResolverContextDataRegistry::GetActiveContextData() != selectedContextData) {
            somethingChanged = true;
            Adsk::AssetResolverContextDataRegistry::SetActiveContextData(selectedContextData);
        }
    }

    if (somethingChanged) {
        // Notify that context data has changed
#if AR_ASSETRESOLVERCONTEXTDATA_HAS_PATHARRAY
        Adsk::SendContextDataChanged(Adsk::ContextDataType::ALL);
#endif
    }
}

void SaveUsdPreferences(const UsdPreferenceOptions& options)
{
    // Update the options instance
    // The copy clears env search paths as they are not saved
    // Those are only used to display the paths in the dialog
    UsdPreferenceOptions::GetInstance() = options;
    // Save options to disk (Maya option vars)
    UsdPreferenceOptions::GetInstance().Save();
}

} // namespace PreferencesManagement
} // namespace MAYAUSD_NS_DEF
