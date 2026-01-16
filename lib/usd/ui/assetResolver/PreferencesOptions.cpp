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
#include "PreferencesOptions.h"

#define MNoVersionString
#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <pxr/base/tf/stringUtils.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

// Option variable names
static const MString OPT_VAR_USE_PROJECT_TOKENS = "mayaUsd_AdskAssetResolverIncludeMayaToken";
static const MString OPT_VAR_MAPPING_FILE = "mayaUsd_AdskAssetResolverMappingFile";
static const MString OPT_VAR_USER_SEARCH_PATHS = "mayaUsd_AdskAssetResolverUserSearchPaths";
static const MString OPT_VAR_USER_PATHS_FIRST = "mayaUsd_AdskAssetResolverUserPathsFirst";
static const MString OPT_VAR_USER_PATHS_ONLY = "mayaUsd_AdskAssetResolverUserPathsOnly";

UsdPreferenceOptions* UsdPreferenceOptions::instance = nullptr;

UsdPreferenceOptions::UsdPreferenceOptions()
    : _useProjectTokens(true)
    , _mappingFile()
    , _userSearchPaths()
    , _userPathsFirst(true)
    , _includeEnvironmentSearchPaths(true)
    , _environmentSearchPaths()
{
}

UsdPreferenceOptions& UsdPreferenceOptions::operator=(const UsdPreferenceOptions& other)
{
    _useProjectTokens = other._useProjectTokens;
    _mappingFile = other._mappingFile;
    _userSearchPaths = other._userSearchPaths;
    _userPathsFirst = other._userPathsFirst;
    _includeEnvironmentSearchPaths = other._includeEnvironmentSearchPaths;
    // Note: _environmentSearchPaths is not copied as it's read-only from resolver
    return *this;
}

UsdPreferenceOptions& UsdPreferenceOptions::GetInstance()
{
    if (!instance) {
        instance = new UsdPreferenceOptions();
        instance->Load();
    }
    return *instance;
}

void UsdPreferenceOptions::Load()
{
    if (MGlobal::optionVarExists(OPT_VAR_USE_PROJECT_TOKENS)) {
        _useProjectTokens = MGlobal::optionVarIntValue(OPT_VAR_USE_PROJECT_TOKENS) != 0;
    } else {
        _useProjectTokens = true;
    }

    if (MGlobal::optionVarExists(OPT_VAR_MAPPING_FILE)) {
        _mappingFile = MGlobal::optionVarStringValue(OPT_VAR_MAPPING_FILE).asChar();
    } else {
        _mappingFile = "";
    }

    if (MGlobal::optionVarExists(OPT_VAR_USER_SEARCH_PATHS)) {
        MString userSearchPathsStr = MGlobal::optionVarStringValue(OPT_VAR_USER_SEARCH_PATHS);
        std::string userSearchPathsStdStr(userSearchPathsStr.asChar());
        _userSearchPaths = TfStringSplit(userSearchPathsStdStr, ";");
    } else {
        _userSearchPaths.clear();
    }

    if (MGlobal::optionVarExists(OPT_VAR_USER_PATHS_FIRST)) {
        _userPathsFirst = MGlobal::optionVarIntValue(OPT_VAR_USER_PATHS_FIRST) != 0;
    } else {
        _userPathsFirst = true;
    }

    if (MGlobal::optionVarExists(OPT_VAR_USER_PATHS_ONLY)) {
        bool userPathsOnly = MGlobal::optionVarIntValue(OPT_VAR_USER_PATHS_ONLY) != 0;
        _includeEnvironmentSearchPaths = !userPathsOnly;
    } else {
        _includeEnvironmentSearchPaths = true;
    }
}

void UsdPreferenceOptions::Save() const
{
    MGlobal::setOptionVarValue(OPT_VAR_USE_PROJECT_TOKENS, _useProjectTokens ? 1 : 0);

    MGlobal::setOptionVarValue(OPT_VAR_MAPPING_FILE, MString(_mappingFile.c_str()));

    std::string userSearchPathsStr = TfStringJoin(_userSearchPaths, ";");
    MGlobal::setOptionVarValue(OPT_VAR_USER_SEARCH_PATHS, MString(userSearchPathsStr.c_str()));

    MGlobal::setOptionVarValue(OPT_VAR_USER_PATHS_FIRST, _userPathsFirst ? 1 : 0);

    bool userPathsOnly = !_includeEnvironmentSearchPaths;
    MGlobal::setOptionVarValue(OPT_VAR_USER_PATHS_ONLY, userPathsOnly ? 1 : 0);
}

bool UsdPreferenceOptions::IsUsingProjectTokens() const { return _useProjectTokens; }

void UsdPreferenceOptions::SetUsingProjectTokens(bool useProjectTokens)
{
    _useProjectTokens = useProjectTokens;
}

const std::string& UsdPreferenceOptions::GetMappingFile() const { return _mappingFile; }

void UsdPreferenceOptions::SetMappingFile(const std::string& mappingFile)
{
    _mappingFile = mappingFile;
}

const std::vector<std::string>& UsdPreferenceOptions::GetUserSearchPaths() const
{
    return _userSearchPaths;
}

void UsdPreferenceOptions::SetUserSearchPaths(const std::vector<std::string>& userSearchPaths)
{
    _userSearchPaths = userSearchPaths;
}

bool UsdPreferenceOptions::IsUsingUserSearchPathsFirst() const { return _userPathsFirst; }

void UsdPreferenceOptions::SetUsingUserSearchPathsFirst(bool userPathsFirst)
{
    _userPathsFirst = userPathsFirst;
}

bool UsdPreferenceOptions::IsIncludingEnvironmentSearchPaths() const
{
    return _includeEnvironmentSearchPaths;
}

void UsdPreferenceOptions::SetIncludingEnvironmentSearchPaths(bool includeEnvironmentSearchPaths)
{
    _includeEnvironmentSearchPaths = includeEnvironmentSearchPaths;
}

const std::vector<std::string>& UsdPreferenceOptions::GetEnvironmentSearchPaths() const
{
    return _environmentSearchPaths;
}

void UsdPreferenceOptions::SetEnvironmentSearchPaths(
    const std::vector<std::string>& environmentSearchPaths)
{
    _environmentSearchPaths = environmentSearchPaths;
}

} // namespace MAYAUSD_NS_DEF
