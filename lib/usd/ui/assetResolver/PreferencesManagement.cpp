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
#include "PreferencesApplicationHost.h"
#include "PreferencesOptions.h"

#include <AssetResolverPreferences/AssetResolverSettings.h>
#include <AssetResolverPreferences/AssetResolverSettingsManagement.h>

#include <mayaUsdUI/ui/USDQtUtil.h>

#include <algorithm>

namespace MAYAUSD_NS_DEF {
namespace PreferencesManagement {

void InitializeUsdPreferences()
{
    // Add ApplicationHost for the USD Preferences dialog
    PreferencesApplicationHost::CreateInstance(MQtUtil::mainWindow());

    // Load USD Preference options to ensure the Adsk Asset Resolver works as configured
    Adsk::AssetResolverSettings::GetInstance().SetSettings(LoadUsdPreferences());
    Adsk::AssetResolverSettingsManagement::ApplySettings(
        Adsk::AssetResolverSettings(), Adsk::AssetResolverSettings::GetInstance());
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
