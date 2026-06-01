//
// Copyright 2022 Autodesk
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
#include "proxyShapeStageExtraData.h"

#include <mayaUsd/utils/loadRules.h>
#include <mayaUsd/utils/targetLayer.h>

#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
#include <mayaUsd/editForward/MayaUsdEditForwardHost.h>
#endif

#include <maya/MGlobal.h>
#include <maya/MSceneMessage.h>

#include <set>

namespace MAYAUSD_NS_DEF {

namespace {

using ProxyShapeSet = std::set<MayaUsdProxyShapeBase*>;

MCallbackId beforeFileSaveCallbackId = 0;

ProxyShapeSet& getTrackedProxyShapes()
{
    static ProxyShapeSet tracked;
    return tracked;
}

void onMayaAboutToSave(void* /* unused */) { MayaUsdProxyShapeStageExtraData::saveAllStageData(); }

// Saving some stage data for all valid tracked stages or one specific stage.
typedef MStatus (*SaveFunc)(const PXR_NS::UsdStage& stage, MayaUsdProxyShapeBase& proxyShape);
void saveTrackedData(const UsdStageRefPtr& stage, SaveFunc saveFunc)
{
    ProxyShapeSet& tracked = getTrackedProxyShapes();
    for (MayaUsdProxyShapeBase* proxyShape : tracked) {
        if (!proxyShape)
            continue;

        auto stagePtr = proxyShape->getUsdStage();
        if (!stagePtr)
            continue;

        if (stage && stage != stagePtr)
            continue;

        saveFunc(*stagePtr, *proxyShape);
    }
}

void saveTrackedLoadRules(const UsdStageRefPtr& stage)
{
    saveTrackedData(stage, copyLoadRulesToAttribute);
}

void saveTrackedTargetLayer(const UsdStageRefPtr& stage)
{
    saveTrackedData(
        stage, [](const PXR_NS::UsdStage& stage, MayaUsdProxyShapeBase& proxyShape) -> MStatus {
#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
            // When Edit Forwarding is active the stage edit target is pinned to the session
            // layer. Save the fallback target instead so that the meaningful "where edits go"
            // layer is correctly restored on reload.
            if (auto controller
                = MayaUsdEditForwardController::GetForStage(proxyShape.getUsdStage())) {
                if (controller->isForwardingActive()) {
                    if (auto fallback = controller->fallbackTarget()) {
                        return copyLayerAsTargetLayerAttribute(fallback, proxyShape);
                    }
                    MGlobal::displayWarning(
                        "Edit forwarding is active but no fallback target is set; target layer not saved.");
                }
            }
#endif
            return copyTargetLayerToAttribute(stage, proxyShape);
        });
}

} // namespace

/* static */
MStatus MayaUsdProxyShapeStageExtraData::initialize()
{
    MStatus status = MS::kSuccess;
    if (beforeFileSaveCallbackId == 0) {
        beforeFileSaveCallbackId = MSceneMessage::addCallback(
            MSceneMessage::kBeforeSave, onMayaAboutToSave, nullptr, &status);
    }
    return status;
}

/* static */
MStatus MayaUsdProxyShapeStageExtraData::finalize()
{
    MStatus status = MS::kSuccess;

    if (beforeFileSaveCallbackId != 0) {
        status = MMessage::removeCallback(beforeFileSaveCallbackId);
        beforeFileSaveCallbackId = 0;
    }

    return status;
}

/* static */
bool MayaUsdProxyShapeStageExtraData::containsProxyShape(MayaUsdProxyShapeBase& proxyShape)
{
    auto it = getTrackedProxyShapes().find(&proxyShape);
    return it != getTrackedProxyShapes().end();
}

/* static */
void MayaUsdProxyShapeStageExtraData::addProxyShape(MayaUsdProxyShapeBase& proxyShape)
{
    getTrackedProxyShapes().insert(&proxyShape);
}

/* static */
void MayaUsdProxyShapeStageExtraData::removeProxyShape(MayaUsdProxyShapeBase& proxyShape)
{
    getTrackedProxyShapes().erase(&proxyShape);
}

/* static */
void MayaUsdProxyShapeStageExtraData::saveAllStageData()
{
    saveAllLoadRules();
    saveAllTargetLayers();
}

/* static */
void MayaUsdProxyShapeStageExtraData::saveAllLoadRules()
{
    // Note: passing nullptr means save all stages.
    saveTrackedLoadRules(nullptr);
}

/* static */
void MayaUsdProxyShapeStageExtraData::saveLoadRules(const UsdStageRefPtr& stage)
{
    saveTrackedLoadRules(stage);
}

/* static */
void MayaUsdProxyShapeStageExtraData::saveAllTargetLayers()
{
    // Note: passing nullptr means save all stages.
    saveTrackedTargetLayer(nullptr);
}

/* static */
void MayaUsdProxyShapeStageExtraData::saveTargetLayer(const UsdStageRefPtr& stage)
{
    saveTrackedTargetLayer(stage);
}

} // namespace MAYAUSD_NS_DEF
