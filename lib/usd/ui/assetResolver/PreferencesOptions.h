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
#pragma once

#include <mayaUsd/mayaUsd.h>
#include <mayaUsdUI/ui/api.h>

#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {

/**
 * \class UsdPreferenceOptions
 * \brief Manages USD preferences for the Autodesk Asset Resolver in Maya.
 *
 * This class provides a structured way to get and set USD Asset Resolver preferences,
 * using Maya option variables for persistence.
 */
class MAYAUSD_UI_PUBLIC UsdPreferenceOptions
{
public:
    UsdPreferenceOptions();
    ~UsdPreferenceOptions() = default;

    UsdPreferenceOptions& operator=(const UsdPreferenceOptions& other);

    /// Load preferences from Maya option variables
    void Load();

    /// Save preferences to Maya option variables
    void Save() const;

    /// Get the singleton instance (loads from Maya option vars on first access)
    static UsdPreferenceOptions& GetInstance();

    /// Get/Set whether to use project tokens in the resolver
    bool IsUsingProjectTokens() const;
    void SetUsingProjectTokens(bool useProjectTokens);

    /// Get/Set whether to prioritize user search paths over environment paths
    bool IsUsingUserSearchPathsFirst() const;
    void SetUsingUserSearchPathsFirst(bool userPathsFirst);

    /// Get/Set the path to the mapping file
    const std::string& GetMappingFile() const;
    void               SetMappingFile(const std::string& mappingFile);

    /// Get/Set user-defined search paths
    const std::vector<std::string>& GetUserSearchPaths() const;
    void SetUserSearchPaths(const std::vector<std::string>& userSearchPaths);

    /// Get/Set whether to include environment search paths
    bool IsIncludingEnvironmentSearchPaths() const;
    void SetIncludingEnvironmentSearchPaths(bool includeEnvironmentSearchPaths);

    /// Get/Set environment search paths (read-only from resolver, not persisted)
    const std::vector<std::string>& GetEnvironmentSearchPaths() const;
    void SetEnvironmentSearchPaths(const std::vector<std::string>& environmentSearchPaths);

private:
    static UsdPreferenceOptions* instance;

    bool                     _useProjectTokens;
    std::string              _mappingFile;
    std::vector<std::string> _userSearchPaths;
    bool                     _userPathsFirst;
    bool                     _includeEnvironmentSearchPaths;
    std::vector<std::string> _environmentSearchPaths; // Not persisted
};

} // namespace MAYAUSD_NS_DEF
