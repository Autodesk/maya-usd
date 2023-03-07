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
#ifndef HDMAYA_SHAPE_ADAPTER_H
#define HDMAYA_SHAPE_ADAPTER_H

#include <hdMaya/adapters/dagAdapter.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaShapeAdapter : public HdMayaDagAdapter
{
protected:
    HDMAYA_API
    HdMayaShapeAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate, const MDagPath& dagPath);

public:
    HDMAYA_API
    virtual ~HdMayaShapeAdapter() = default;

    HDMAYA_API
    virtual size_t
    SamplePrimvar(const TfToken& key, size_t maxSampleCount, float* times, VtValue* samples);
    HDMAYA_API
    virtual HdMeshTopology GetMeshTopology();
    HDMAYA_API
    virtual HdBasisCurvesTopology GetBasisCurvesTopology();
    HDMAYA_API
    virtual HdDisplayStyle GetDisplayStyle();
    HDMAYA_API
    virtual PxOsdSubdivTags GetSubdivTags();
    HDMAYA_API
    virtual HdPrimvarDescriptorVector GetPrimvarDescriptors(HdInterpolation interpolation)
    {
        return {};
    }
    HDMAYA_API
    virtual void MarkDirty(HdDirtyBits dirtyBits) override;

    HDMAYA_API
    virtual MObject GetMaterial();
    HDMAYA_API
    virtual bool GetDoubleSided() { return true; };

    HDMAYA_API
    const GfRange3d& GetExtent();

    HDMAYA_API
    virtual TfToken GetRenderTag() const;

    HDMAYA_API
    virtual void PopulateSelectedPaths(
        const MDagPath&                             selectedDag,
        SdfPathVector&                              selectedSdfPaths,
        std::unordered_set<SdfPath, SdfPath::Hash>& selectedMasters,
        const HdSelectionSharedPtr&                 selection);

protected:
    HDMAYA_API
    void _CalculateExtent();

private:
    GfRange3d _extent;
    bool      _extentDirty;
};

using HdMayaShapeAdapterPtr = std::shared_ptr<HdMayaShapeAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_SHAPE_ADAPTER_H
