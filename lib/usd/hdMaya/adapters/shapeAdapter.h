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
#ifndef __HDMAYA_SHAPE_ADAPTER_H__
#define __HDMAYA_SHAPE_ADAPTER_H__

#include <pxr/pxr.h>

#include "dagAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaShapeAdapter : public HdMayaDagAdapter {
protected:
    HDMAYA_API
    HdMayaShapeAdapter(
        const SdfPath& id, HdMayaDelegateCtx* delegate,
        const MDagPath& dagPath);

public:
    HDMAYA_API
    virtual ~HdMayaShapeAdapter() = default;

    HDMAYA_API
    virtual size_t SamplePrimvar(
        const TfToken& key, size_t maxSampleCount, float* times,
        VtValue* samples);
    HDMAYA_API
    virtual HdMeshTopology GetMeshTopology();
    HDMAYA_API
    virtual HdBasisCurvesTopology GetBasisCurvesTopology();
    HDMAYA_API
    virtual HdDisplayStyle GetDisplayStyle();
    HDMAYA_API
    virtual PxOsdSubdivTags GetSubdivTags();
    HDMAYA_API
    virtual HdPrimvarDescriptorVector GetPrimvarDescriptors(
        HdInterpolation interpolation) {
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
        const MDagPath& selectedDag, SdfPathVector& selectedSdfPaths,
        std::unordered_set<SdfPath, SdfPath::Hash>& selectedMasters,
        const HdSelectionSharedPtr& selection);

protected:
    HDMAYA_API
    void _CalculateExtent();

private:
    GfRange3d _extent;
    bool _extentDirty;
};

using HdMayaShapeAdapterPtr = std::shared_ptr<HdMayaShapeAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_SHAPE_ADAPTER_H__
