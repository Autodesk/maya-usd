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

#include "AssetResolverApplicationHost.h"
#include "AssetResolverUtils.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/stringUtils.h>

#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MString.h>

#if ADSK_USD_ASSET_RESOLVER_LAYOUT_OSS
#include <AdskUsdAssetResolverExtensions/Settings/SettingsManagement.h>
#else
#include <AssetResolverExtensions/Settings/AssetResolverSettingsManagement.h>
#endif

#include <algorithm>

namespace MAYAUSD_NS_DEF {
namespace PreferencesManagement {

// Option variable names
static const MString OPT_VAR_USE_PROJECT_TOKENS
    = UsdMayaUtil::convert(MayaUsdOptionVars->IncludeMayaTokenInAR);
static const MString OPT_VAR_MAPPING_FILE
    = UsdMayaUtil::convert(MayaUsdOptionVars->AdskAssetResolverMappingFile);
static const MString OPT_VAR_USER_SEARCH_PATHS
    = UsdMayaUtil::convert(MayaUsdOptionVars->AdskAssetResolverUserSearchPaths);
static const MString OPT_VAR_USER_PATHS_FIRST
    = UsdMayaUtil::convert(MayaUsdOptionVars->AdskAssetResolverUserPathsFirst);
static const MString OPT_VAR_USER_PATHS_ONLY
    = UsdMayaUtil::convert(MayaUsdOptionVars->AdskAssetResolverUserPathsOnly);

pxr::VtDictionary LoadUsdPreferences()
{
#if ADSK_USD_ASSET_RESOLVER_LAYOUT_OSS
    Adsk::UsdAssetResolver::Extensions::Settings settings;
#else
    Adsk::AssetResolverSettings settings;
#endif

    if (MGlobal::optionVarExists(OPT_VAR_USE_PROJECT_TOKENS)) {
        settings.SetUsingProjectTokens(MGlobal::optionVarIntValue(OPT_VAR_USE_PROJECT_TOKENS) != 0);
    }

    if (MGlobal::optionVarExists(OPT_VAR_MAPPING_FILE)) {
        settings.SetMappingFile(MGlobal::optionVarStringValue(OPT_VAR_MAPPING_FILE).asChar());
    }

    if (MGlobal::optionVarExists(OPT_VAR_USER_SEARCH_PATHS)) {
        MString     userSearchPathsStr = MGlobal::optionVarStringValue(OPT_VAR_USER_SEARCH_PATHS);
        std::string userSearchPathsStdStr(userSearchPathsStr.asChar());
        settings.SetUserSearchPaths(pxr::TfStringSplit(userSearchPathsStdStr, ";"));
    }

    if (MGlobal::optionVarExists(OPT_VAR_USER_PATHS_FIRST)) {
        settings.SetUsingUserSearchPathsFirst(
            MGlobal::optionVarIntValue(OPT_VAR_USER_PATHS_FIRST) != 0);
    }

    if (MGlobal::optionVarExists(OPT_VAR_USER_PATHS_ONLY)) {
        settings.SetUsingUserSearchPathsOnly(
            MGlobal::optionVarIntValue(OPT_VAR_USER_PATHS_ONLY) != 0);
    }

    return settings.GetSettings();
}

#if ADSK_USD_ASSET_RESOLVER_LAYOUT_OSS
void SaveUsdPreferences(const Adsk::UsdAssetResolver::Extensions::Settings& options)
#else
void SaveUsdPreferences(const Adsk::AssetResolverSettings& options)
#endif
{
    // update the options instance
    // the copy clears env search paths as they are not saved
    // those are only used to display the paths in the dialog
#if ADSK_USD_ASSET_RESOLVER_LAYOUT_OSS
    Adsk::UsdAssetResolver::Extensions::Settings::GetInstance() = options;
#else
    Adsk::AssetResolverSettings::GetInstance() = options;
#endif

    MGlobal::setOptionVarValue(OPT_VAR_USE_PROJECT_TOKENS, options.IsUsingProjectTokens() ? 1 : 0);
    MGlobal::setOptionVarValue(OPT_VAR_MAPPING_FILE, MString(options.GetMappingFile().c_str()));
    std::string userSearchPathsStr = pxr::TfStringJoin(options.GetUserSearchPaths(), ";");
    MGlobal::setOptionVarValue(OPT_VAR_USER_SEARCH_PATHS, MString(userSearchPathsStr.c_str()));
    MGlobal::setOptionVarValue(
        OPT_VAR_USER_PATHS_FIRST, options.IsUsingUserSearchPathsFirst() ? 1 : 0);
    MGlobal::setOptionVarValue(
        OPT_VAR_USER_PATHS_ONLY, options.IsUsingUserSearchPathsOnly() ? 1 : 0);
}

void InitializeUsdPreferences()
{
    // Add ApplicationHost for the USD Preferences dialog
    AssetResolverApplicationHost::CreateInstance(MQtUtil::mainWindow());

    // Load USD Preference options to ensure the Adsk Asset Resolver works as configured
#if ADSK_USD_ASSET_RESOLVER_LAYOUT_OSS
    Adsk::UsdAssetResolver::Extensions::Settings::GetInstance().SetSettings(LoadUsdPreferences());
    Adsk::UsdAssetResolver::Extensions::SettingsManagement::ApplySettings(
        Adsk::UsdAssetResolver::Extensions::Settings(),
        Adsk::UsdAssetResolver::Extensions::Settings::GetInstance());
#else
    Adsk::AssetResolverSettings::GetInstance().SetSettings(LoadUsdPreferences());
    Adsk::AssetResolverSettingsManagement::ApplySettings(
        Adsk::AssetResolverSettings(), Adsk::AssetResolverSettings::GetInstance());
#endif
}

} // namespace PreferencesManagement
} // namespace MAYAUSD_NS_DEF
