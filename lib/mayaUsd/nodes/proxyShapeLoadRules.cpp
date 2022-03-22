//
// Copyright 2016 Pixar
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
#include "proxyShapeLoadRules.h"

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

void onMayaAboutToSave(void* /* unused */)
{
    ProxyShapeSet& tracked = getTrackedProxyShapes();
    for (MayaUsdProxyShapeBase* proxyShape : tracked) {
        if (!proxyShape)
            continue;

        auto stagePtr = proxyShape->getUsdStage();
        if (!stagePtr)
            continue;

        MObject proxyObj = proxyShape->thisMObject();
        if (proxyObj.isNull())
            continue;

        copyLoadRulesToAttribute(*stagePtr, proxyObj);
    }
}

} // namespace

/* static */
MStatus MayaUsdProxyShapeLoadRules::initialize()
{
    MStatus status = MS::kSuccess;
    if (beforeFileSaveCallbackId == 0) {
        beforeFileSaveCallbackId = MSceneMessage::addCallback(
            MSceneMessage::kBeforeSave, onMayaAboutToSave, nullptr, &status);
    }
    return status;
}

/* static */
MStatus MayaUsdProxyShapeLoadRules::finalize()
{
    MStatus status = MS::kSuccess;

    if (beforeFileSaveCallbackId != 0) {
        status = MMessage::removeCallback(beforeFileSaveCallbackId);
        beforeFileSaveCallbackId = 0;
    }

    return status;
}

/* static */
void MayaUsdProxyShapeLoadRules::addProxyShape(MayaUsdProxyShapeBase& proxyShape)
{
    getTrackedProxyShapes().insert(&proxyShape);
}

/* static */
void MayaUsdProxyShapeLoadRules::removeProxyShape(MayaUsdProxyShapeBase& proxyShape)
{
    getTrackedProxyShapes().erase(&proxyShape);
}

} // namespace MAYAUSD_NS_DEF
