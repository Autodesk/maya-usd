//
// Copyright 2024 Autodesk
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

#include "layerLocking.h"

#include <mayaUsd/listeners/notice.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/weakBase.h>

#include <memory>

#include <layerLocking.h>

namespace MAYAUSD_NS_DEF {

MStatus copyLayerLockingToAttribute(MayaUsdProxyShapeBase* proxyShape)
{
    if (proxyShape == nullptr) {
        return MStatus::kFailure;
    }
    return proxyShape->setLockedLayers(UsdLayerEditor::getLockedLayersIdentifiers());
}

MStatus copyLayerLockingFromAttribute(
    const MayaUsdProxyShapeBase& proxyShape,
    const LayerNameMap&          nameMap,
    PXR_NS::UsdStage&            stage)
{
    UsdLayerEditor::loadLayerLockState(proxyShape.getLockedLayers(), nameMap, stage);
    return MS::kSuccess;
}

void updateProxyShapeLockAttribute(const std::string& proxyShapePath)
{
    if (proxyShapePath.empty()) {
        return;
    }

    MayaUsdProxyShapeBase* proxyShape = UsdMayaUtil::GetProxyShapeByProxyName(proxyShapePath);
    if (!proxyShape) {
        // The layer editor commands pass UFE path strings ("|world|group|shape"). Maya does not
        // resolve the leading "world" component. Parsing the string back into a Ufe::Path is not
        // an option: the Maya runtime string handler does not parse it the same way in all
        // Maya versions.
        static const std::string worldPrefix = "|world";
        if (proxyShapePath.compare(0, worldPrefix.size(), worldPrefix) == 0) {
            proxyShape
                = UsdMayaUtil::GetProxyShapeByProxyName(proxyShapePath.substr(worldPrefix.size()));
        }
    }
    copyLayerLockingToAttribute(proxyShape);
}

namespace {

// Automatic reset of recorded locked layers when the Maya scene is reset.
struct SceneResetListener : public PXR_NS::TfWeakBase
{
    SceneResetListener()
    {
        PXR_NS::TfWeakPtr<SceneResetListener> me(this);
        PXR_NS::TfNotice::Register(me, &SceneResetListener::OnSceneReset);
    }

    void OnSceneReset(const UsdMayaSceneResetNotice&)
    {
        // Make sure we don't hold onto locked layers now that the
        // Maya scene is reset.
        UsdLayerEditor::forgetLockedLayers();
        UsdLayerEditor::forgetSystemLockedLayers();
    }
};

// The locked layers live in the DCC-agnostic UsdLayerEditor store, which has no
// notion of a Maya scene. This Maya-side listener clears that store on scene reset.
std::unique_ptr<SceneResetListener> sSceneResetListener;

} // namespace

void registerLayerLockingSceneResetListener()
{
    if (!sSceneResetListener)
        sSceneResetListener = std::make_unique<SceneResetListener>();
}

void unregisterLayerLockingSceneResetListener() { sSceneResetListener.reset(); }

} // namespace MAYAUSD_NS_DEF
