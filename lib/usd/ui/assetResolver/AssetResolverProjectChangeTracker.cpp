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
#include "AssetResolverProjectChangeTracker.h"

// This is added to prevent multiple definitions of the MApiVersion string.
#define MNoVersionString
#include "AssetResolverUtils.h"

#include <maya/MCallbackIdArray.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MUserEventMessage.h>

namespace MAYAUSD_NS_DEF {

static MCallbackId _projectChangedId;

void AssetResolverProjectChangeTracker::onProjectChanged(void* clientData)
{
    const bool includeMayaProjectTokens
        = MGlobal::optionVarIntValue("mayaUsd_AdskAssetResolverIncludeMayaToken");
    if (includeMayaProjectTokens)
        AssetResolverUtils::includeMayaProjectTokensInAdskAssetResolver();
}

MStatus AssetResolverProjectChangeTracker::startTracking()
{
    MStatus status;
    _projectChangedId = MUserEventMessage::addUserEventCallback(
        "projectChanged", onProjectChanged, nullptr, &status);

    return status;
}

MStatus AssetResolverProjectChangeTracker::stopTracking()
{
    MMessage::removeCallback(_projectChangedId);
    return MStatus::kSuccess;
}

} // namespace MAYAUSD_NS_DEF