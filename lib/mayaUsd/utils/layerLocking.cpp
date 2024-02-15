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

#include <pxr/base/tf/weakBase.h>

namespace MAYAUSD_NS_DEF {

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
        forgetSystemLockedLayers();
    }
};

// The set of locked layers.
//
// Kept in a function to avoid problem with the order of construction
// of global variables in C++.
using LockedLayers = std::set<PXR_NS::SdfLayerRefPtr>;
LockedLayers& getLockedLayers()
{
    // Note: C++ guarantees correct multi-thread protection for static
    //       variables initialization in functions.
    static SceneResetListener onSceneResetListener;
    static LockedLayers       layers;
    return layers;
}
} // namespace

void addSystemLockedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    LockedLayers& layers = getLockedLayers();
    layers.insert(layer);
}

void removeSystemLockedLayer(const PXR_NS::SdfLayerRefPtr& layer)
{
    if (!layer)
        return;

    LockedLayers& layers = getLockedLayers();
    layers.erase(layer);
}

bool isLayerSystemLocked(const PXR_NS::SdfLayerRefPtr& layer)
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

void forgetSystemLockedLayers()
{
    LockedLayers& layers = getLockedLayers();
    layers.clear();
}

} // namespace MAYAUSD_NS_DEF
