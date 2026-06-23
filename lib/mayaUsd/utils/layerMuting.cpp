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

#include "layerMuting.h"

#include <mayaUsd/listeners/notice.h>

#include <layerMuting.h>

#include <pxr/base/tf/weakBase.h>

#include <memory>

namespace MAYAUSD_NS_DEF {

namespace {

// Automatic reset of recorded muted layers when the Maya scene is reset.
struct SceneResetListener : public PXR_NS::TfWeakBase
{
    SceneResetListener()
    {
        PXR_NS::TfWeakPtr<SceneResetListener> me(this);
        PXR_NS::TfNotice::Register(me, &SceneResetListener::OnSceneReset);
    }

    void OnSceneReset(const UsdMayaSceneResetNotice&)
    {
        // Make sure we don't hold onto muted layers now that the
        // Maya scene is reset.
        UsdLayerEditor::forgetMutedLayers();
    }
};

// The muted layers live in the DCC-agnostic UsdLayerEditor store, which has no
// notion of a Maya scene. This Maya-side listener clears that store on scene
// reset. Registered deterministically at plugin init.
std::unique_ptr<SceneResetListener> sSceneResetListener;

} // namespace

void registerLayerMutingSceneResetListener()
{
    if (!sSceneResetListener)
        sSceneResetListener = std::make_unique<SceneResetListener>();
}

void unregisterLayerMutingSceneResetListener() { sSceneResetListener.reset(); }

MStatus copyLayerMutingToAttribute(const PXR_NS::UsdStage& stage, MayaUsdProxyShapeBase& proxyShape)
{
    return proxyShape.setMutedLayers(stage.GetMutedLayers());
}

MStatus copyLayerMutingFromAttribute(
    const MayaUsdProxyShapeBase& proxyShape,
    const LayerNameMap&          nameMap,
    PXR_NS::UsdStage&            stage)
{
    const std::vector<std::string> muted = proxyShape.getMutedLayers();
    UsdLayerEditor::loadLayerMuteState(muted, nameMap, stage);
    return MS::kSuccess;
}

bool addMutedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    return UsdLayerEditor::addMutedLayer(layer);
}

bool removeMutedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    return UsdLayerEditor::removeMutedLayer(layer);
}

const LayerRefSet& getMutedLayers(const std::string& mutedIdentifier)
{
    return UsdLayerEditor::getMutedLayers(mutedIdentifier);
}

void forgetMutedLayers() { UsdLayerEditor::forgetMutedLayers(); }

} // namespace MAYAUSD_NS_DEF
