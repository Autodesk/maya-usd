//
// Copyright 2023 Autodesk, Inc. All rights reserved.
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

#ifndef MAYAHYDRASCENEINDEX_H
#define MAYAHYDRASCENEINDEX_H

#include <maya/MDagPath.h>
#include <maya/MFrameContext.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MViewport2Renderer.h>

#include <mayaHydraLib/api.h>
#include <mayaHydraLib/delegates/params.h>
#include <mayaHydraLib/delegates/delegate.h>
#include <mayaHydraLib/adapters/shapeAdapter.h>
#include <mayaHydraLib/adapters/renderItemAdapter.h>
#include <mayaHydraLib/adapters/materialAdapter.h>
#include <mayaHydraLib/adapters/lightAdapter.h>
#include <mayaHydraLib/adapters/cameraAdapter.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/imaging/hd/changeTracker.h>
#include <pxr/imaging/hd/selection.h>
#include <pxr/imaging/hd/driver.h>
#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/rendererPlugin.h>
#include <pxr/imaging/hdx/taskController.h>
#include <pxr/imaging/hd/sceneIndex.h>

#include <unordered_map>


PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraSceneIndex;
TF_DECLARE_REF_PTRS(MayaHydraSceneIndex);

/**
 * \brief MayaHydraSceneIndex is a scene index to produce the hydra scene from Maya native scene.
 * TODO: MayaHydraSceneIndex can be derived from HdRetainedSceneIndex with usd 23.05+.
 */
class MAYAHYDRALIB_API MayaHydraSceneIndex : public HdSceneIndexBase
{
public:
    template <typename T> using AdapterMap = std::unordered_map<SdfPath, T, SdfPath::Hash>;

    static MayaHydraSceneIndexRefPtr New(SdfPath& id,
        MayaHydraDelegate::InitData& initData,
        bool lightEnabled) {
        return TfCreateRefPtr(new MayaHydraSceneIndex(id, initData, lightEnabled));
    }

    // ------------------------------------------------------------------------
    // HdSceneIndexBase implementations
    // TODO: Reuse the implementations from HdRetainedSceneIndex with usd 23.05+
    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override;

    struct AddedPrimEntry
    {
        SdfPath primPath;
        TfToken primType;
        HdContainerDataSourceHandle dataSource;
    };
    using AddedPrimEntries = std::vector<AddedPrimEntry>;

    void AddPrims(const AddedPrimEntries& entries);
    void RemovePrims(const HdSceneIndexObserver::RemovedPrimEntries& entries);
    void DirtyPrims(const HdSceneIndexObserver::DirtiedPrimEntries& entries);

    // ------------------------------------------------------------------------
    // Maya Hydra scene producer implementations
    // Propogate scene changes from Maya to Hydra
    void HandleCompleteViewportScene(const MDataServerOperation::MViewportScene& scene, MFrameContext::DisplayStyle ds);

    // Populate data from Maya
    void Populate();

    // Populate selected paths from Maya
    void PopulateSelectedPaths(
        const MSelectionList& mayaSelection,
        SdfPathVector& selectedSdfPaths,
        const HdSelectionSharedPtr& selection);

    // Add hydra pick points and items to Maya's selection list
    bool AddPickHitToSelectionList(
        const HdxPickHit& hit,
        const MHWRender::MSelectionInfo& selectInfo,
        MSelectionList& selectionList,
        MPointArray& worldSpaceHitPts);

    // Insert a Rprim to hydra scene
    void InsertRprim(
        MayaHydraAdapter* adapter,
        const TfToken& typeId,
        const SdfPath& id,
        const SdfPath& instancerId);

    // Remove a Rprim from hydra scene
    void RemoveRprim(const SdfPath& id);

    // Mark a Rprim in hydra scene as dirty
    void MarkRprimDirty(
        const SdfPath& id,
        HdDirtyBits dirtyBits);

    // Insert a Sprim to hydra scene
    void InsertSprim(
        const TfToken& typeId,
        const SdfPath& id,
        HdDirtyBits    initialBits);

    // Remove a Sprim from hydra scene
    void RemoveSprim(const TfToken& typeId, const SdfPath& id);

    // Operation that's performed on rendering a frame
    void PreFrame(const MHWRender::MDrawContext& drawContext);
    void PostFrame();

    void SetParams(const MayaHydraParams& params) { _params = params; }
    const MayaHydraParams& GetParams() const { return _params; }

    // Adapter operations
    void RemoveAdapter(const SdfPath& id);
    void RecreateAdapterOnIdle(const SdfPath& id, const MObject& obj);

    // Update viewport info to camera
    SdfPath SetCameraViewport(const MDagPath& camPath, const GfVec4d& viewport);

    // Enable or disable lighting
    void SetLightsEnabled(const bool enabled) { _lightsEnabled = enabled; }
    bool GetLightsEnabled() { return _lightsEnabled; }

    GfInterval GetCurrentTimeSamplingInterval() const;

    HdChangeTracker& GetChangeTracker();

    HdRenderIndex& GetRenderIndex() { return *_renderIndex; }

    SdfPath GetDelegateID(TfToken name);

    HdMeshTopology GetMeshTopology(const SdfPath& id);

    SdfPath GetPrimPath(const MDagPath& dg, bool isSprim);

    SdfPath GetLightedPrimsRootPath() const;

    SdfPath GetRprimPath() const { return _rprimPath; }

    bool IsHdSt() const { return _isHdSt; }

private:
    MayaHydraSceneIndex(
        SdfPath& id,
        MayaHydraDelegate::InitData& initData,
        bool lightEnabled);

    // Utilites
    bool _GetRenderItem(int fastId, MayaHydraRenderItemAdapterPtr& adapter);
    void _AddRenderItem(const MayaHydraRenderItemAdapterPtr& ria);
    void _RemoveRenderItem(const MayaHydraRenderItemAdapterPtr& ria);
    bool _GetRenderItemMaterial(const MRenderItem& ri, SdfPath& material, MObject& shadingEngineNode);
    SdfPath _GetRenderItemPrimPath(const MRenderItem& ri);

private:
    // ------------------------------------------------------------------------
    // HdSceneIndexBase implementations
    // TODO: Reuse the implementations from HdRetainedSceneIndex with usd 23.05+
    struct _PrimEntry
    {
        HdSceneIndexPrim prim;
    };
    using _PrimEntryTable = SdfPathTable<_PrimEntry>;
    _PrimEntryTable _entries;

    SdfPath _ID;
    MayaHydraParams _params;

    // Weak refs
    MayaHydraSceneProducer* _producer = nullptr;
    HdRenderIndex* _renderIndex = nullptr;

    // Adapters
    AdapterMap<MayaHydraLightAdapterPtr> _lightAdapters;
    AdapterMap<MayaHydraCameraAdapterPtr> _cameraAdapters;
    AdapterMap<MayaHydraShapeAdapterPtr> _shapeAdapters;
    AdapterMap<MayaHydraRenderItemAdapterPtr> _renderItemsAdapters;
    std::unordered_map<int, MayaHydraRenderItemAdapterPtr> _renderItemsAdaptersFast;
    AdapterMap<MayaHydraMaterialAdapterPtr>    _materialAdapters;
    std::vector<MCallbackId>                   _callbacks;
    std::vector<std::tuple<SdfPath, MObject>>  _adaptersToRecreate;
    std::vector<std::tuple<SdfPath, uint32_t>> _adaptersToRebuild;

    static SdfPath _fallbackMaterial;
    bool _isPlaybackRunning = false;
    bool _lightsEnabled = true;
    bool _isHdSt = false;

    SdfPath _rprimPath;
    SdfPath _sprimPath;
    SdfPath _materialPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRASCENEINDEX_H
