//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef __HDMAYA_SCENE_DELEGATE_H__
#define __HDMAYA_SCENE_DELEGATE_H__

#include <pxr/pxr.h>

#include <hdmaya/hdmaya.h>

#include <pxr/base/gf/vec4d.h>

#include <pxr/usd/sdf/path.h>

#include <pxr/imaging/hd/meshTopology.h>
#include <pxr/imaging/hd/sceneDelegate.h>

#include <maya/MDagPath.h>
#include <maya/MObject.h>

#include <memory>

#include <hdmaya/adapters/lightAdapter.h>
#include <hdmaya/adapters/materialAdapter.h>
#include <hdmaya/adapters/shapeAdapter.h>
#include <hdmaya/delegates/delegateCtx.h>

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

class HdMayaSceneDelegate : public HdMayaDelegateCtx {
public:
    HdMayaSceneDelegate(const InitData& initData);

    ~HdMayaSceneDelegate() override;

    void Populate() override;
    void PreFrame(const MHWRender::MDrawContext& context) override;
    void RemoveAdapter(const SdfPath& id) override;
    void RecreateAdapter(const SdfPath& id, const MObject& obj) override;
    void RecreateAdapterOnIdle(const SdfPath& id, const MObject& obj) override;
    void RebuildAdapterOnIdle(const SdfPath& id, uint32_t flags) override;
    /// \brief Notifies the scene delegate when a material tag changes.
    ///
    /// This function is only affects the render index when its using HdSt.
    /// HdSt requires rebuilding the shapes whenever the tags affecting
    /// translucency change.
    ///
    /// \param id Id of the Material that changed its tag.
    void MaterialTagChanged(const SdfPath& id) override;
    void InsertDag(const MDagPath& dag);
    void NodeAdded(const MObject& obj);
    void UpdateLightVisibility(const MDagPath& dag);
    void AddNewInstance(const MDagPath& dag);
    void SetParams(const HdMayaParams& params) override;
    void PopulateSelectedPaths(
        const MSelectionList& mayaSelection, SdfPathVector& selectedSdfPaths,
        HdSelection* selection) override;

protected:
    HdMeshTopology GetMeshTopology(const SdfPath& id) override;
    HdBasisCurvesTopology GetBasisCurvesTopology(const SdfPath& id) override;
    PxOsdSubdivTags GetSubdivTags(const SdfPath& id) override;
    GfRange3d GetExtent(const SdfPath& id) override;
    GfMatrix4d GetTransform(const SdfPath& id) override;
    size_t SampleTransform(
        const SdfPath& id, size_t maxSampleCount, float* times,
        GfMatrix4d* samples) override;
    bool GetVisible(const SdfPath& id) override;
    bool IsEnabled(const TfToken& option) const override;
    bool GetDoubleSided(const SdfPath& id) override;
    HdCullStyle GetCullStyle(const SdfPath& id) override;
    // VtValue GetShadingStyle(const SdfPath& id) override;
    HdDisplayStyle GetDisplayStyle(const SdfPath& id) override;
    // TfToken GetReprName(const SdfPath& id) override;
    VtValue Get(const SdfPath& id, const TfToken& key) override;
    size_t SamplePrimvar(
        const SdfPath& id, const TfToken& key, size_t maxSampleCount,
        float* times, VtValue* samples) override;
    TfToken GetRenderTag(SdfPath const& id) override;
    HdPrimvarDescriptorVector GetPrimvarDescriptors(
        const SdfPath& id, HdInterpolation interpolation) override;
    VtValue GetLightParamValue(
        const SdfPath& id, const TfToken& paramName) override;
    VtIntArray GetInstanceIndices(
        const SdfPath& instancerId, const SdfPath& prototypeId) override;
    GfMatrix4d GetInstancerTransform(
#ifdef HDMAYA_USD_001905_BUILD
        SdfPath const& instancerId
#else
        SdfPath const& instancerId, SdfPath const& prototypeId
#endif
        ) override;
    SdfPath GetPathForInstanceIndex(
        const SdfPath& protoPrimPath, int instanceIndex,
        int* absoluteInstanceIndex, SdfPath* rprimPath,
        SdfPathVector* instanceContext) override;
    SdfPath GetMaterialId(const SdfPath& id) override;
    std::string GetSurfaceShaderSource(const SdfPath& id) override;
    std::string GetDisplacementShaderSource(const SdfPath& id) override;
    VtValue GetMaterialParamValue(
        const SdfPath& id, const TfToken& paramName) override;
    HdMaterialParamVector GetMaterialParams(const SdfPath& id) override;
    VtValue GetMaterialResource(const SdfPath& id) override;
    TfTokenVector GetMaterialPrimvars(const SdfPath& id) override;
    HdTextureResource::ID GetTextureResourceID(
        const SdfPath& textureId) override;
    HdTextureResourceSharedPtr GetTextureResource(
        const SdfPath& textureId) override;
    /// \brief Gets the metadata from a material.
    ///
    /// For now we are only returning the materialTag for translucency.
    ///
    /// \param materialId Path to the material.
    /// \return Dictionary storing the metadata.
    VtDictionary GetMaterialMetadata(const SdfPath& materialId) override;
    bool _CreateMaterial(const SdfPath& id, const MObject& obj);

private:
    std::unordered_map<SdfPath, HdMayaShapeAdapterPtr, SdfPath::Hash>
        _shapeAdapters;
    std::unordered_map<SdfPath, HdMayaLightAdapterPtr, SdfPath::Hash>
        _lightAdapters;
    std::unordered_map<SdfPath, HdMayaMaterialAdapterPtr, SdfPath::Hash>
        _materialAdapters;
    std::vector<MCallbackId> _callbacks;
    std::vector<std::tuple<SdfPath, MObject>> _adaptersToRecreate;
    std::vector<std::tuple<SdfPath, uint32_t>> _adaptersToRebuild;
    std::vector<MObject> _addedNodes;
    std::vector<SdfPath> _materialTagsChanged;

    SdfPath _fallbackMaterial;
};

typedef std::shared_ptr<HdMayaSceneDelegate> MayaSceneDelegateSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_SCENE_DELEGATE_H__
