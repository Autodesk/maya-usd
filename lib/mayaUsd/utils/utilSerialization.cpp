//
// Copyright 2021 Autodesk
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
#include "utilSerialization.h"

#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilComponentCreator.h>
#include <mayaUsd/utils/utilFileSystem.h>
#include <usdLayerEditor/utilSerialization.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MString.h>

namespace {

void updateTargetLayer(const std::string& proxyNodeName, const SdfLayerRefPtr& layer)
{
    if (MayaUsdProxyShapeBase* proxyShape = UsdMayaUtil::GetProxyShapeByProxyName(proxyNodeName)) {
        proxyShape->getUsdStage()->SetEditTarget(layer);
    }
}

} // namespace

namespace MAYAUSD_NS_DEF {
namespace utils {

bool isProxyShapePathRelative(MayaUsdProxyShapeBase& proxyShape)
{
    MStatus           status;
    MFnDependencyNode depNode(proxyShape.thisMObject(), &status);
    if (!status)
        return false;

    MPlug filePathRelativePlug = depNode.findPlug(MayaUsdProxyShapeBase::filePathRelativeAttr);
    return filePathRelativePlug.asBool();
}

bool isProxyPathModeRelative(ProxyPathMode proxyPathMode, const MString& proxyNodeName)
{
    if (kProxyPathRelative == proxyPathMode)
        return true;

    if (kProxyPathAbsolute == proxyPathMode)
        return false;

    if (kProxyPathFollowProxyShape == proxyPathMode) {
        // Note: if we fail to find the proxy shape, we will fallback on
        //       using the options var preference instead.
        MayaUsdProxyShapeBase* proxyShape
            = UsdMayaUtil::GetProxyShapeByProxyName(proxyNodeName.asChar());
        if (proxyShape) {
            return isProxyShapePathRelative(*proxyShape);
        }
    }

    return UsdMayaUtilFileSystem::requireUsdPathsRelativeToMayaSceneFile();
}

void setNewProxyPath(
    const MString&        proxyNodeName,
    const MString&        newRootLayerPath,
    ProxyPathMode         proxyPathMode,
    const SdfLayerRefPtr& layer,
    bool                  isTargetLayer)
{
    const bool  needRelativePath = isProxyPathModeRelative(proxyPathMode, proxyNodeName);
    const char* filePathCmd = "setAttr -type \"string\" ^1s.filePath \"^2s\"; "
                              "setAttr ^1s.filePathRelative ^3s; ";

    MString script;
    script.format(filePathCmd, proxyNodeName, newRootLayerPath, needRelativePath ? "1" : "0");
    MGlobal::executeCommand(
        script,
        /*display*/ true,
        /*undo*/ false);

    if (isTargetLayer)
        updateTargetLayer(proxyNodeName.asChar(), layer);
}

void getLayersToSaveFromProxy(
    const std::string&                                proxyPath,
    UsdLayerEditor::Serialization::StageLayersToSave& layersInfo)
{
    auto stage = UsdMayaUtil::GetStageByProxyName(proxyPath);
    if (!stage) {
        return;
    }

    // Special case for components created by the component creator. Non-local layers,
    // non-active layers, and non-dirty but to be renamed layers, can be impacted when saving a
    // component. Only the component creator knows how to save a component properly.
    if (ComponentUtils::isAdskUsdComponent(proxyPath)) {
        UsdLayerEditor::Serialization::appendRenderLayers(stage, layersInfo);
        const auto layerIds = ComponentUtils::getAdskUsdComponentLayersToSave(proxyPath);
        for (const auto& ccLayerId : layerIds) {
            auto ccLayer = PXR_NS::SdfLayer::FindOrOpen(ccLayerId);
            if (!ccLayer || ccLayer->IsAnonymous()) {
                continue;
            }
            layersInfo._dirtyFileBackedLayers.push_back(ccLayer);
        }
        return;
    }

    UsdLayerEditor::Serialization::getLayersToSaveFromStage(stage, proxyPath, layersInfo);
}

} // namespace utils
} // namespace MAYAUSD_NS_DEF
