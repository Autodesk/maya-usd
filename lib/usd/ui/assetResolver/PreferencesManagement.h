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

namespace Adsk {
class AssetResolverSettings;
}

namespace MAYAUSD_NS_DEF {

/**
 * \namespace PreferencesManagement
 * \brief Functions for managing USD preferences and applying them to the Autodesk Asset Resolver.
 */
namespace PreferencesManagement {

/// Initialize the USD Preferences system (called once at plugin startup)
MAYAUSD_UI_PUBLIC void InitializeUsdPreferences();

MAYAUSD_UI_PUBLIC void SaveUsdPreferences(const Adsk::AssetResolverSettings& options);


} // namespace PreferencesManagement
} // namespace MAYAUSD_NS_DEF
