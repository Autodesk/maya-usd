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

namespace MAYAUSD_NS_DEF {

class UsdPreferenceOptions;

/**
 * \namespace PreferencesManagement
 * \brief Functions for managing USD preferences and applying them to the Autodesk Asset Resolver.
 */
namespace PreferencesManagement {

/// Initialize the USD Preferences system (called once at plugin startup)
MAYAUSD_UI_PUBLIC void InitializeUsdPreferences();

/// Get the current USD Preferences
MAYAUSD_UI_PUBLIC const UsdPreferenceOptions GetUsdPreferences();

/// Apply newOptions to asset resolver context, only changing values that differ from options
MAYAUSD_UI_PUBLIC
void ApplyUsdPreferences(
    const UsdPreferenceOptions& options,
    const UsdPreferenceOptions& newOptions);

/// Save the USD Preferences to Maya option variables
MAYAUSD_UI_PUBLIC void SaveUsdPreferences(const UsdPreferenceOptions& options);

} // namespace PreferencesManagement
} // namespace MAYAUSD_NS_DEF
