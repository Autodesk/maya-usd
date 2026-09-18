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
#include "AssetResolverUtils.h"

// This is added to prevent multiple definitions of the MApiVersion string.
#define MNoVersionString
#include <maya/MGlobal.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>

#include <AdskUsdAssetResolver/ContextDataBuilder.h>
#include <AdskUsdAssetResolver/ContextDataRegistry.h>
#include <AdskUsdAssetResolver/Notice.h>
#include <AdskUsdAssetResolverExtensions/Settings/SettingsManagement.h>

namespace MAYAUSD_NS_DEF {

void AssetResolverUtils::includeMayaProjectTokensInAdskAssetResolver()
{
    {
        Adsk::UsdAssetResolver::PreventContextDataChangedNotification preventNotifications;

        MString      workspaceDirectory = MGlobal::executeCommandStringResult("workspace -q -fn");
        MStringArray workspaceFileRuleList;
        MStatus      status = MGlobal::executeCommand("workspace -q -frl", workspaceFileRuleList);

        if (status == MS::kSuccess) {
            Adsk::UsdAssetResolver::ContextDataBuilder contextData
                = Adsk::UsdAssetResolver::ContextDataRegistry::RegisterContextData(
                    Adsk::UsdAssetResolver::Extensions::SettingsManagement::
                        PROJECT_TOKENS_DATA_SET_NAME);

            contextData.AddStaticToken("Project", workspaceDirectory.asChar());

            for (unsigned int i = 0; i < workspaceFileRuleList.length(); i++) {
                MString tokenValue = MGlobal::executeCommandStringResult(
                    MString("workspace -fileRuleEntry ") + "\"" + workspaceFileRuleList[i] + "\"");
                contextData.AddStaticToken(workspaceFileRuleList[i].asChar(), tokenValue.asChar());
            }
        }
    }
    Adsk::UsdAssetResolver::SendContextDataChanged(Adsk::UsdAssetResolver::ContextDataType::TOKEN);
}

void AssetResolverUtils::excludeMayaProjectTokensFromAdskAssetResolver()
{
    {
        Adsk::UsdAssetResolver::PreventContextDataChangedNotification preventNotifications;
        Adsk::UsdAssetResolver::ContextDataRegistry::RemoveContextData("MayaUSDExtension");
    }
    Adsk::UsdAssetResolver::SendContextDataChanged(Adsk::UsdAssetResolver::ContextDataType::TOKEN);
}

} // namespace MAYAUSD_NS_DEF