//
// Copyright 2019 Luma Pictures
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
// Copyright 2023 Autodesk, Inc. All rights reserved.
//
#ifndef MAYAHYDRALIB_SCENE_DELEGATE_H
#define MAYAHYDRALIB_SCENE_DELEGATE_H

#include <mayaHydraLib/adapters/cameraAdapter.h>
#include <mayaHydraLib/adapters/lightAdapter.h>
#include <mayaHydraLib/adapters/materialAdapter.h>
#include <mayaHydraLib/adapters/renderItemAdapter.h>
#include <mayaHydraLib/adapters/shapeAdapter.h>
#include <mayaHydraLib/delegates/delegateCtx.h>

#include <pxr/base/gf/vec4d.h>
#include <pxr/imaging/hd/meshTopology.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MDagPath.h>
#include <maya/MFrameContext.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>

#include <memory>
#include <vector>

/*
 * Notes.
 *
 * To remove the need of casting between different adapter types or
 * making the base adapter class too heavy I decided to use 3 different set
 * or map types. This adds a bit of extra code to the RemoveAdapter function
 * but simplifies the rest of the functions significantly (and no downcasting!).
 *
 * All this would be probably way nicer / easier with C++14 and the polymorphic
 * lambdas.
 *
 * This also optimizes other things, like it's easier to separate functionality
 * that only affects shapes, lights or materials.
 */

PXR_NAMESPACE_OPEN_SCOPE

/**
 * \brief MayaHydraSceneDelegate is an Hydra custom scene delegate used to translate from a Maya
 * scene to hydra.
 *
 * If you want to know how to add a custom scene index to this plug-in, then please see the
 * registration.cpp file.
 */
class MayaHydraSceneDelegate : public MayaHydraDelegateCtx
{
public:
    template <typename T> using AdapterMap = std::unordered_map<SdfPath, T, SdfPath::Hash>;

    MAYAHYDRALIB_API
    MayaHydraSceneDelegate(const InitData& initData);

    MAYAHYDRALIB_API
    ~MayaHydraSceneDelegate() override;

    MAYAHYDRALIB_API
    void Populate() override;

    MAYAHYDRALIB_API
    void PreFrame(const MHWRender::MDrawContext& context) override;

    MAYAHYDRALIB_API
    void RemoveAdapter(const SdfPath& id) override;

    MAYAHYDRALIB_API
    void RecreateAdapter(const SdfPath& id, const MObject& obj) override;

    MAYAHYDRALIB_API
    void RecreateAdapterOnIdle(const SdfPath& id, const MObject& obj) override;

    MAYAHYDRALIB_API
    void RebuildAdapterOnIdle(const SdfPath& id, uint32_t flags) override;

    MAYAHYDRALIB_API
    void AddArnoldLight(const MDagPath& dag) override;

    MAYAHYDRALIB_API
    void RemoveArnoldLight(const MDagPath& dag) override;

    /// \brief Notifies the scene delegate when a material tag changes.
    ///
    /// This function is only affects the render index when its using HdSt.
    /// HdSt requires rebuilding the shapes whenever the tags affecting
    /// translucency change.
    ///
    /// \param id Id of the Material that changed its tag.
    MAYAHYDRALIB_API
    void MaterialTagChanged(const SdfPath& id) override;

    MAYAHYDRALIB_API
    MayaHydraShapeAdapterPtr GetShapeAdapter(const SdfPath& id);

    MAYAHYDRALIB_API
    MayaHydraLightAdapterPtr GetLightAdapter(const SdfPath& id);

    MAYAHYDRALIB_API
    MayaHydraMaterialAdapterPtr GetMaterialAdapter(const SdfPath& id);

#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    MAYAHYDRALIB_API
    void InsertDag(const MDagPath& dag);
#endif

    void OnDagNodeAdded(const MObject& obj);

    void OnDagNodeRemoved(const MObject& obj);

    MAYAHYDRALIB_API
    void UpdateLightVisibility(const MDagPath& dag);

    MAYAHYDRALIB_API
    void AddNewInstance(const MDagPath& dag);

    MAYAHYDRALIB_API
    void SetParams(const MayaHydraParams& params) override;

    MAYAHYDRALIB_API
    SdfPath SetCameraViewport(const MDagPath& camPath, const GfVec4d& viewport);

    MAYAHYDRALIB_API
    void HandleCompleteViewportScene(
        const MDataServerOperation::MViewportScene& scene,
        MFrameContext::DisplayStyle                 ds);

    MAYAHYDRALIB_API
    bool AddPickHitToSelectionList(
        const HdxPickHit&                hit,
        const MHWRender::MSelectionInfo& selectInfo,
        MSelectionList&                  selectionList,
        MPointArray&                     worldSpaceHitPts) override;

    bool GetPlaybackRunning() const { return _isPlaybackRunning; }

protected:
    MAYAHYDRALIB_API
    HdMeshTopology GetMeshTopology(const SdfPath& id) override;

    MAYAHYDRALIB_API
    HdBasisCurvesTopology GetBasisCurvesTopology(const SdfPath& id) override;

    MAYAHYDRALIB_API
    PxOsdSubdivTags GetSubdivTags(const SdfPath& id) override;

    MAYAHYDRALIB_API
    GfRange3d GetExtent(const SdfPath& id) override;

    MAYAHYDRALIB_API
    GfMatrix4d GetTransform(const SdfPath& id) override;

    MAYAHYDRALIB_API
    size_t
    SampleTransform(const SdfPath& id, size_t maxSampleCount, float* times, GfMatrix4d* samples)
        override;

    MAYAHYDRALIB_API
    bool GetVisible(const SdfPath& id) override;

    MAYAHYDRALIB_API
    bool IsEnabled(const TfToken& option) const override;

    MAYAHYDRALIB_API
    bool GetDoubleSided(const SdfPath& id) override;

    MAYAHYDRALIB_API
    HdCullStyle GetCullStyle(const SdfPath& id) override;

    MAYAHYDRALIB_API
    VtValue GetShadingStyle(const SdfPath& id) override;

    MAYAHYDRALIB_API
    HdDisplayStyle GetDisplayStyle(const SdfPath& id) override;
    // TfToken GetReprName(const SdfPath& id) override;

    MAYAHYDRALIB_API
    VtValue Get(const SdfPath& id, const TfToken& key) override;

    MAYAHYDRALIB_API
    size_t SamplePrimvar(
        const SdfPath& id,
        const TfToken& key,
        size_t         maxSampleCount,
        float*         times,
        VtValue*       samples) override;

    MAYAHYDRALIB_API
    TfToken GetRenderTag(SdfPath const& id) override;

    MAYAHYDRALIB_API
    HdPrimvarDescriptorVector
    GetPrimvarDescriptors(const SdfPath& id, HdInterpolation interpolation) override;

    MAYAHYDRALIB_API
    VtValue GetLightParamValue(const SdfPath& id, const TfToken& paramName) override;

    MAYAHYDRALIB_API
    VtValue GetCameraParamValue(const SdfPath& cameraId, const TfToken& paramName) override;

    MAYAHYDRALIB_API
    VtIntArray GetInstanceIndices(const SdfPath& instancerId, const SdfPath& prototypeId) override;

    MAYAHYDRALIB_API
    SdfPathVector GetInstancerPrototypes(SdfPath const& instancerId) override;

    MAYAHYDRALIB_API
    SdfPath GetInstancerId(const SdfPath& primId) override;

    MAYAHYDRALIB_API
    GfMatrix4d GetInstancerTransform(SdfPath const& instancerId) override;

    MAYAHYDRALIB_API
    SdfPath GetScenePrimPath(
        const SdfPath&      rprimPath,
        int                 instanceIndex,
        HdInstancerContext* instancerContext) override;

    MAYAHYDRALIB_API
    SdfPath GetMaterialId(const SdfPath& id) override;

    MAYAHYDRALIB_API
    VtValue GetMaterialResource(const SdfPath& id) override;

private:
    template <typename AdapterPtr, typename Map>
    AdapterPtr Create(
        const MDagPath&                                                          dag,
        const std::function<AdapterPtr(MayaHydraDelegateCtx*, const MDagPath&)>& adapterCreator,
        Map&                                                                     adapterMap,
        bool                                                                     isSprim = false);

    MAYAHYDRALIB_API
    bool _GetRenderItem(int fastId, MayaHydraRenderItemAdapterPtr& adapter);

    MAYAHYDRALIB_API
    void _AddRenderItem(const MayaHydraRenderItemAdapterPtr& ria);

    MAYAHYDRALIB_API
    void _RemoveRenderItem(const MayaHydraRenderItemAdapterPtr& ria);

    MAYAHYDRALIB_API
    bool
    _GetRenderItemMaterial(const MRenderItem& ri, SdfPath& material, MObject& shadingEngineNode);

    static VtValue CreateMayaDefaultMaterial();

    bool _CreateMaterial(const SdfPath& id, const MObject& obj);
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    /// \brief Unordered Map storing the shape adapters.
    AdapterMap<MayaHydraShapeAdapterPtr> _shapeAdapters;
#endif
    /// \brief Unordered Map storing the shape adapters.
    AdapterMap<MayaHydraRenderItemAdapterPtr>              _renderItemsAdapters;
    std::unordered_map<int, MayaHydraRenderItemAdapterPtr> _renderItemsAdaptersFast;

    /// \brief Unordered Map storing the light adapters.
    AdapterMap<MayaHydraLightAdapterPtr> _lightAdapters;
    /// \brief Unordered Map storing the camera adapters.
    AdapterMap<MayaHydraCameraAdapterPtr> _cameraAdapters;
    /// \brief Unordered Map storing the material adapters.
    AdapterMap<MayaHydraMaterialAdapterPtr>    _materialAdapters;
    std::vector<MCallbackId>                   _callbacks;
    std::vector<std::tuple<SdfPath, MObject>>  _adaptersToRecreate;
    std::vector<std::tuple<SdfPath, uint32_t>> _adaptersToRebuild;
#ifdef MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
    std::vector<MObject> _addedNodes;
#endif

    using LightAdapterCreator
        = std::function<MayaHydraLightAdapterPtr(MayaHydraDelegateCtx*, const MDagPath&)>;
    std::vector<std::pair<MObject, LightAdapterCreator>> _lightsToAdd;

    /// Is used to maintain a list of Arnold lights, they are not seen as lights by Maya but as
    /// locators
    std::vector<MDagPath> _arnoldLightPaths;

    std::vector<SdfPath> _materialTagsChanged;

    /// _fallbackMaterial is an SdfPath used when there is no material assigned to a Maya object
    static SdfPath _fallbackMaterial;
    /// _mayaDefaultMaterialPath is common to all scene delegates, it's the SdfPath of
    /// _mayaDefaultMaterial
    static SdfPath _mayaDefaultMaterialPath;
    /// _mayaDefaultMaterial is an hydra material used to override all materials from the scene when
    /// _useDefaultMaterial is true
    static VtValue _mayaDefaultMaterial;

    bool _useDefaultMaterial = false;
    bool _xRayEnabled = false;
    bool _isPlaybackRunning = false;
};

typedef std::shared_ptr<MayaHydraSceneDelegate> MayaSceneDelegateSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_SCENE_DELEGATE_H
