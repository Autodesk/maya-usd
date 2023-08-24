//
// Copyright 2023 Autodesk
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
#ifndef MAYAUSD_PROXY_SHAPE_SCENE_INDEX_PLUGIN_H
#define MAYAUSD_PROXY_SHAPE_SCENE_INDEX_PLUGIN_H

#include <pxr/pxr.h>

#if PXR_VERSION >= 2211

#include <mayaUsd/base/api.h>
#include <mayaUsd/listeners/proxyShapeNotice.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/base/tf/refPtr.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/sceneIndexPlugin.h>
#include <pxr/imaging/hd/sceneIndexPluginRegistry.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usdImaging/usdImaging/stageSceneIndex.h>

#include <ufe/path.h>

#include <maya/MMessage.h>

#include <memory>

//////////////////////////////////////////////////////////////// MayaUsdProxyShapeSceneIndexPlugin

PXR_NAMESPACE_OPEN_SCOPE

// The plugin class must be present in the pixar scope otherwise the factory method registered by
// HdSceneIndexPluginRegistry::Define will not be available from the string constructed from the
// node type name.

class MayaUsdProxyShapeMayaNodeSceneIndexPlugin : public HdSceneIndexPlugin
{
public:
    MayaUsdProxyShapeMayaNodeSceneIndexPlugin();

protected:
    HdSceneIndexBaseRefPtr _AppendSceneIndex(
        const HdSceneIndexBaseRefPtr&      inputScene,
        const HdContainerDataSourceHandle& inputArgs) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

namespace MAYAUSD_NS_DEF {

//////////////////////////////////////////////////////////////// MayaUsdProxyShapeSceneIndex

class MayaUsdProxyShapeSceneIndex;

TF_DECLARE_REF_PTRS(MayaUsdProxyShapeSceneIndex);

/// <summary>
/// Simply wraps single stage scene index for initial stage assignment and population
/// </summary>
class MayaUsdProxyShapeSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    using ParentClass = HdSingleInputFilteringSceneIndexBase;

    static MayaUsdProxyShapeSceneIndexRefPtr
    New(MayaUsdProxyShapeBase*                 proxyShape,
        const HdSceneIndexBaseRefPtr&          sceneIndexChainLastElement,
        const UsdImagingStageSceneIndexRefPtr& usdImagingStageSceneIndex);

    static Ufe::Path
    InterpretRprimPath(const HdSceneIndexBaseRefPtr& sceneIndex, const SdfPath& path);

    // satisfying HdSceneIndexBase
    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;

    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override;

    MayaUsdProxyShapeSceneIndex(
        HdSceneIndexBaseRefPtr          inputSceneIndex,
        UsdImagingStageSceneIndexRefPtr usdImagingStageSceneIndex,
        MayaUsdProxyShapeBase*          proxyShape);

    virtual ~MayaUsdProxyShapeSceneIndex();

    void UpdateTime();

private:
    void ObjectsChanged(const MayaUsdProxyStageObjectsChangedNotice& notice);

    void StageSet(const MayaUsdProxyStageSetNotice& notice);

    void Populate();

protected:
    void _PrimsAdded(
        const HdSceneIndexBase&                       sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override final;

    void _PrimsRemoved(
        const HdSceneIndexBase&                         sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override final;

    void _PrimsDirtied(
        const HdSceneIndexBase&                         sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override final;

private:
    static void onTimeChanged(void* data);

    UsdImagingStageSceneIndexRefPtr _usdImagingStageSceneIndex;
    MayaUsdProxyShapeBase*          _proxyShape { nullptr };
    std::atomic_bool                _populated { false };
    MCallbackId                     _timeChangeCallbackId;
};
} // namespace MAYAUSD_NS_DEF

#endif
#endif
