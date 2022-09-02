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
#include "proxyShapeStageData.h"

#include <mayaUsd/utils/layerMuting.h>
#include <mayaUsd/utils/loadRules.h>

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

void onMayaAboutToSave(void* /* unused */) { MayaUsdProxyShapeStageData::saveAllStageData(); }

// Saving some stage data for all valid tracked stages or one specific stage.
typedef MStatus (*SaveFunc)(const PXR_NS::UsdStage& stage, MObject& obj);
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

        MObject proxyObj = proxyShape->thisMObject();
        if (proxyObj.isNull())
            continue;

        saveFunc(*stagePtr, proxyObj);
    }
}

void saveTrackedLoadRules(const UsdStageRefPtr& stage)
{
    saveTrackedData(stage, copyLoadRulesToAttribute);
}

void saveTrackedLayerMutings(const UsdStageRefPtr& stage)
{
    saveTrackedData(stage, copyLayerMutingToAttribute);
}

} // namespace

/* static */
MStatus MayaUsdProxyShapeStageData::initialize()
{
    MStatus status = MS::kSuccess;
    if (beforeFileSaveCallbackId == 0) {
        beforeFileSaveCallbackId = MSceneMessage::addCallback(
            MSceneMessage::kBeforeSave, onMayaAboutToSave, nullptr, &status);
    }
    return status;
}

/* static */
MStatus MayaUsdProxyShapeStageData::finalize()
{
    MStatus status = MS::kSuccess;

    if (beforeFileSaveCallbackId != 0) {
        status = MMessage::removeCallback(beforeFileSaveCallbackId);
        beforeFileSaveCallbackId = 0;
    }

    return status;
}

/* static */
void MayaUsdProxyShapeStageData::addProxyShape(MayaUsdProxyShapeBase& proxyShape)
{
    getTrackedProxyShapes().insert(&proxyShape);
}

/* static */
void MayaUsdProxyShapeStageData::removeProxyShape(MayaUsdProxyShapeBase& proxyShape)
{
    getTrackedProxyShapes().erase(&proxyShape);
}

/* static */
void MayaUsdProxyShapeStageData::saveAllStageData()
{
    saveAllLoadRules();
    saveAllLayerMutings();
}

/* static */
void MayaUsdProxyShapeStageData::saveAllLoadRules()
{
    // Note: passing nullptr means save all stages.
    saveTrackedLoadRules(nullptr);
}

/* static */
void MayaUsdProxyShapeStageData::saveLoadRules(const UsdStageRefPtr& stage)
{
    saveTrackedLoadRules(stage);
}

/* static */
void MayaUsdProxyShapeStageData::saveAllLayerMutings()
{
    // Note: passing nullptr means save all stages.
    saveTrackedLayerMutings(nullptr);
}

/* static */
void MayaUsdProxyShapeStageData::saveLayerMuting(const UsdStageRefPtr& stage)
{
    saveTrackedLayerMutings(stage);
}

} // namespace MAYAUSD_NS_DEF
