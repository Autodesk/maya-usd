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

#include <mayaUsd/fileio/importData.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsdUI/ui/USDAssetResolverDialog.h>
#include <mayaUsdUI/ui/USDQtUtil.h>

#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/usd/variantSets.h>

#include <maya/MArgParser.h>
#include <maya/MCallbackIdArray.h>
#include <maya/MDagPath.h>
#include <maya/MFileObject.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnStringData.h>
#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <maya/MUserEventMessage.h>

#include <AdskAssetResolver/AdskAssetResolver.h>
#include <AdskAssetResolver/AssetResolverContextDataRegistry.h>

namespace MAYAUSD_NS_DEF {

static MCallbackId _projectChangedId;

void AssetResolverProjectChangeTracker::onProjectChanged(void* clientData)
{
    const bool includeMayaProjectTokens
        = MGlobal::optionVarIntValue("mayaUsd_AdskAssetResolverIncludeMayaToken");
    if (includeMayaProjectTokens)
        AssetResolverUtils::IncludeMayaProjectTokensInAdskAssetResolver();
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