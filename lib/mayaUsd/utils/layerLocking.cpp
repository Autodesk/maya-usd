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
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/query.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/weakBase.h>

#include <ufe/path.h>

namespace MAYAUSD_NS_DEF {

MStatus copyLayerLockingToAttribute(MayaUsdProxyShapeBase* proxyShape)
{
    if (proxyShape != nullptr) {
        LockedLayers& lockedLayers = getLockedLayers();

        std::vector<std::string> toAttribute;
        toAttribute.reserve(lockedLayers.size());

        for (auto layer : lockedLayers) {
            toAttribute.emplace_back(layer->GetIdentifier());
        }
        return proxyShape->setLockedLayers(toAttribute);
    } else {
        return MStatus::kFailure;
    }
}

MStatus copyLayerLockingFromAttribute(
    const MayaUsdProxyShapeBase& proxyShape,
    const LayerNameMap&          nameMap,
    PXR_NS::UsdStage&            stage)
{
    std::vector<std::string> locked = proxyShape.getLockedLayers();

    // Remap the locked layer names in case the layers were renamed when reloaded.
    for (std::string& name : locked) {
        auto iter = nameMap.find(name);
        if (iter != nameMap.end()) {
            name = iter->second;
        }
    }

    // Add locked layers to the retained locked layer set to avoid losing them.
    // This is necessary because USD only keeps layers in memory if at least one
    // referencing pointer holds it, but locking in the stage makes the stage no
    // longer reference the layer, so the layer would be lost otherwise.
    //
    // Use a set to accelerate lookup of locked layers.
    PXR_NS::SdfLayerHandleVector layers = stage.GetLayerStack();
    std::set<std::string>        lockedSet(locked.begin(), locked.end());
    for (const auto& layer : layers) {
        const auto iter = lockedSet.find(layer->GetIdentifier());
        if (iter != lockedSet.end()) {
            std::string emptyShapePath;
            lockLayer(emptyShapePath, layer, LayerLockType::LayerLock_Locked, false);
        }
    }

    return MS::kSuccess;
}

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
        forgetLockedLayers();
        forgetSystemLockedLayers();
    }
};

void updateProxyShapeAttribute(const std::string proxyShapePath)
{
    if (!proxyShapePath.empty()) {
        MayaUsdProxyShapeBase* proxyShape = UsdMayaUtil::GetProxyShapeByProxyName(proxyShapePath);
        copyLayerLockingToAttribute(proxyShape);
    }
}

void lockLayer(
    std::string                   proxyShapePath,
    const PXR_NS::SdfLayerRefPtr& layer,
    LayerLockType                 locktype,
    bool                          updateProxyShapeAttr /*= true */)
{

    switch (locktype) {
    default:
    case LayerLock_Unlocked: {
        layer->SetPermissionToEdit(true);
        layer->SetPermissionToSave(true);
        removeLockedLayer(layer);
        removeSystemLockedLayer(layer);
        if (updateProxyShapeAttr) {
            updateProxyShapeAttribute(proxyShapePath);
        }
        break;
    }
    case LayerLock_Locked: {
        layer->SetPermissionToEdit(false);
        layer->SetPermissionToSave(true);
        addLockedLayer(layer);
        removeSystemLockedLayer(layer);
        if (updateProxyShapeAttr) {
            updateProxyShapeAttribute(proxyShapePath);
        }
        break;
    }
    case LayerLock_SystemLocked: {
        layer->SetPermissionToSave(false);
        layer->SetPermissionToEdit(false);
        addSystemLockedLayer(layer);
        removeLockedLayer(layer);
        if (updateProxyShapeAttr) {
            updateProxyShapeAttribute(proxyShapePath);
        }
        break;
    }
    }
}

// The set of locked layers.
//
// Kept in a function to avoid problem with the order of construction
// of global variables in C++.

LockedLayers& getLockedLayers()
{
    // Note: C++ guarantees correct multi-thread protection for static
    //       variables initialization in functions.
    static SceneResetListener onSceneResetListener;
    static LockedLayers       layers;
    return layers;
}

LockedLayers& getSystemLockedLayers()
{
    // Note: C++ guarantees correct multi-thread protection for static
    //       variables initialization in functions.
    static SceneResetListener onSceneResetListener;
    static LockedLayers       layers;
    return layers;
}

void addLockedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    LockedLayers& layers = getLockedLayers();
    layers.insert(layer);
}

void removeLockedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    LockedLayers& layers = getLockedLayers();
    layers.erase(layer);
}

bool isLayerLocked(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return false;

    LockedLayers& layers = getLockedLayers();
    auto          iter = layers.find(layer);
    if (iter != layers.end()) {
        return true;
    }
    return false;
}

void forgetLockedLayers()
{
    LockedLayers& layers = getLockedLayers();
    layers.clear();
}

void addSystemLockedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    LockedLayers& layers = getSystemLockedLayers();
    layers.insert(layer);
}

void removeSystemLockedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    LockedLayers& layers = getSystemLockedLayers();
    layers.erase(layer);
}

bool isLayerSystemLocked(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return false;

    LockedLayers& layers = getSystemLockedLayers();
    auto          iter = layers.find(layer);
    if (iter != layers.end()) {
        return true;
    }
    return false;
}

void forgetSystemLockedLayers()
{
    LockedLayers& layers = getSystemLockedLayers();
    layers.clear();
}

} // namespace MAYAUSD_NS_DEF
