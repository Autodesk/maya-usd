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
#ifndef MAYAHYDRALIB_SHAPE_ADAPTER_H
#define MAYAHYDRALIB_SHAPE_ADAPTER_H

#include <mayaHydraLib/adapters/dagAdapter.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

/**
 * \brief MayaHydraShapeAdapter is an adapter to translate from Maya shapes to hydra
 * Please note that, at this time, this is not used by the hydra plug-in, we translate from a
 * renderitem to hydra using the MayaHydraRenderItemAdapter class.
 */
class MayaHydraShapeAdapter : public MayaHydraDagAdapter
{
protected:
    MAYAHYDRALIB_API
    MayaHydraShapeAdapter(
        const SdfPath&        id,
        MayaHydraDelegateCtx* delegate,
        const MDagPath&       dagPath);

public:
    MAYAHYDRALIB_API
    virtual ~MayaHydraShapeAdapter() = default;

    MAYAHYDRALIB_API
    virtual size_t
    SamplePrimvar(const TfToken& key, size_t maxSampleCount, float* times, VtValue* samples);
    MAYAHYDRALIB_API
    virtual HdMeshTopology GetMeshTopology() override;
    MAYAHYDRALIB_API
    virtual HdBasisCurvesTopology GetBasisCurvesTopology() override;
    MAYAHYDRALIB_API
    virtual HdDisplayStyle GetDisplayStyle() override;
    MAYAHYDRALIB_API
    virtual PxOsdSubdivTags GetSubdivTags();
    MAYAHYDRALIB_API
    virtual HdPrimvarDescriptorVector GetPrimvarDescriptors(HdInterpolation interpolation) override
    {
        return {};
    }
    MAYAHYDRALIB_API
    virtual void MarkDirty(HdDirtyBits dirtyBits) override;

    MAYAHYDRALIB_API
    virtual MObject GetMaterial();
    MAYAHYDRALIB_API
    virtual bool GetDoubleSided() const override { return true; };

    MAYAHYDRALIB_API
    const GfRange3d& GetExtent();

    MAYAHYDRALIB_API
    virtual TfToken GetRenderTag() const override;

    MAYAHYDRALIB_API
    virtual void PopulateSelectedPaths(
        const MDagPath&                             selectedDag,
        SdfPathVector&                              selectedSdfPaths,
        std::unordered_set<SdfPath, SdfPath::Hash>& selectedMasters,
        const HdSelectionSharedPtr&                 selection);

protected:
    MAYAHYDRALIB_API
    void _CalculateExtent();

private:
    GfRange3d _extent;
    bool      _extentDirty;
};

using MayaHydraShapeAdapterPtr = std::shared_ptr<MayaHydraShapeAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_SHAPE_ADAPTER_H
