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
#include <maya/MDrawContext.h>

#include <mayaHydraLib/api.h>
#include <mayaHydraLib/delegates/params.h>
#include <mayaHydraLib/delegates/delegate.h>
#include <mayaHydraLib/adapters/shapeAdapter.h>
#include <mayaHydraLib/adapters/renderItemAdapter.h>
#include <mayaHydraLib/adapters/materialAdapter.h>
#include <mayaHydraLib/adapters/lightAdapter.h>
#include <mayaHydraLib/adapters/cameraAdapter.h>
#include <mayaHydraLib/sceneIndex/mayaHydraDefaultLightDataSource.h>

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
    enum RebuildFlags : uint32_t
    {
        RebuildFlagPrim = 1 << 1,
        RebuildFlagCallbacks = 1 << 2,
    };
    template <typename T> using AdapterMap = std::unordered_map<SdfPath, T, SdfPath::Hash>;

    static MayaHydraSceneIndexRefPtr New(SdfPath& id,
        MayaHydraDelegate::InitData& initData,
        bool lightEnabled) {
        return TfCreateRefPtr(new MayaHydraSceneIndex(id, initData, lightEnabled));
    }

    ~MayaHydraSceneIndex();

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

    // Insert a primitive to hydra scene
    void InsertPrim(
        MayaHydraAdapter* adapter,
        const TfToken& typeId,
        const SdfPath& id);

    // Remove a primitive from hydra scene
    void RemovePrim(const SdfPath& id);

    // Mark a primitive in hydra scene as dirty
    void MarkPrimDirty(
        const SdfPath& id,
        HdDirtyBits dirtyBits);

    // Operation that's performed on rendering a frame
    void PreFrame(const MHWRender::MDrawContext& drawContext);
    void PostFrame();

    void SetParams(const MayaHydraParams& params);
    const MayaHydraParams& GetParams() const { return _params; }

    VtValue GetShadingStyle(const SdfPath& id);

    // Adapter operations
    void RemoveAdapter(const SdfPath& id);
    void RecreateAdapter(const SdfPath& id, const MObject& obj);
    void RecreateAdapterOnIdle(const SdfPath& id, const MObject& obj);
    void RebuildAdapterOnIdle(const SdfPath& id, uint32_t flags);

    // Update viewport info to camera
    SdfPath SetCameraViewport(const MDagPath& camPath, const GfVec4d& viewport);

    // Enable or disable lighting
    void SetLightsEnabled(const bool enabled) { _lightsEnabled = enabled; }
    bool GetLightsEnabled() const { return _lightsEnabled; }

    // Enable or disable default lighting
    void SetDefaultLightEnabled(const bool enabled);
    bool GetDefaultLightEnabled() const { return _useMayaDefaultLight; }
    void SetDefaultLight(const GlfSimpleLight& light);
    const GlfSimpleLight& GetDefaultLight() const { return _mayaDefaultLight; }

    // Dag Node operations
    void InsertDag(const MDagPath& dag);
    void OnDagNodeAdded(const MObject& obj);
    void OnDagNodeRemoved(const MObject& obj);
    void AddNewInstance(const MDagPath& dag);
    void UpdateLightVisibility(const MDagPath& dag);

    void AddArnoldLight(const MDagPath& dag);
    void RemoveArnoldLight(const MDagPath& dag);

    void MaterialTagChanged(const SdfPath& id);
    SdfPath GetMaterialId(const SdfPath& id);

    GfInterval GetCurrentTimeSamplingInterval() const;

    HdChangeTracker& GetChangeTracker();

    HdRenderIndex& GetRenderIndex() { return *_renderIndex; }

    SdfPath GetDelegateID(TfToken name);

    HdMeshTopology GetMeshTopology(const SdfPath& id);

    SdfPath GetPrimPath(const MDagPath& dg, bool isSprim);

    SdfPath GetLightedPrimsRootPath() const;

    SdfPath GetRprimPath() const { return _rprimPath; }

    bool IsHdSt() const { return _isHdSt; }

    MayaHydraSceneProducer* GetProducer() { return _producer; };

   
private:
    MayaHydraSceneIndex(
        SdfPath& id,
        MayaHydraDelegate::InitData& initData,
        bool lightEnabled);

    template <typename AdapterPtr, typename Map>
    AdapterPtr _CreateAdapter(
        const MDagPath& dag,
        const std::function<AdapterPtr(MayaHydraSceneProducer*, const MDagPath&)>& adapterCreator,
        Map& adapterMap,
        bool                                                                     isSprim = false);
    MayaHydraLightAdapterPtr CreateLightAdapter(const MDagPath& dagPath);
    MayaHydraCameraAdapterPtr CreateCameraAdapter(const MDagPath& dagPath);
    MayaHydraShapeAdapterPtr CreateShapeAdapter(const MDagPath& dagPath);

    // Utilites
    bool _GetRenderItem(int fastId, MayaHydraRenderItemAdapterPtr& adapter);
    void _AddRenderItem(const MayaHydraRenderItemAdapterPtr& ria);
    void _RemoveRenderItem(const MayaHydraRenderItemAdapterPtr& ria);
    bool _GetRenderItemMaterial(const MRenderItem& ri, SdfPath& material, MObject& shadingEngineNode);
    SdfPath _GetRenderItemPrimPath(const MRenderItem& ri);
    SdfPath GetMaterialPath(const MObject& obj);
    bool _CreateMaterial(const SdfPath& id, const MObject& obj);
    static VtValue CreateMayaDefaultMaterial();
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

    std::vector<MObject> _addedNodes;
    using LightAdapterCreator
        = std::function<MayaHydraLightAdapterPtr(MayaHydraSceneProducer*, const MDagPath&)>;
    std::vector<std::pair<MObject, LightAdapterCreator>> _lightsToAdd;
    std::vector<MDagPath> _arnoldLightPaths;
    std::vector<SdfPath> _materialTagsChanged;

    bool _useDefaultMaterial = false;
    static SdfPath _fallbackMaterial;
    /// _mayaDefaultMaterialPath is common to all scene indexes, it's the SdfPath of
    /// _mayaDefaultMaterial
    static SdfPath _mayaDefaultMaterialPath;
    /// _mayaDefaultMaterial is an hydra material used to override all materials from the scene when
    /// _useDefaultMaterial is true
    static VtValue _mayaDefaultMaterial;

    // Default light
    GlfSimpleLight _mayaDefaultLight;
    bool _useMayaDefaultLight = false;
    static SdfPath _mayaDefaultLightPath;

    bool _xRayEnabled = false;
    bool _isPlaybackRunning = false;
    bool _lightsEnabled = true;
    bool _isHdSt = false;

    SdfPath _rprimPath;
    SdfPath _sprimPath;
    SdfPath _materialPath;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRASCENEINDEX_H
