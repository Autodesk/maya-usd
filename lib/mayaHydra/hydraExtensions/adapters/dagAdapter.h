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
#ifndef HDMAYA_DG_ADAPTER_H
#define HDMAYA_DG_ADAPTER_H

#include <hdMaya/adapters/adapter.h>
#include <hdMaya/adapters/adapterDebugCodes.h>
#include <hdMaya/utils.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/range3d.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/meshTopology.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/pxr.h>

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MMatrix.h>
#include <maya/MMessage.h>

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDagAdapter : public HdMayaAdapter
{
protected:
    HDMAYA_API
    HdMayaDagAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate, const MDagPath& dagPath);

public:
    HDMAYA_API
    virtual ~HdMayaDagAdapter() = default;
    HDMAYA_API
    virtual bool GetVisible() { return IsVisible(); }
    HDMAYA_API
    virtual void CreateCallbacks() override;
    HDMAYA_API
    virtual void MarkDirty(HdDirtyBits dirtyBits) override;
    HDMAYA_API
    virtual void RemovePrim() override;
    HDMAYA_API
    GfMatrix4d GetTransform();
    HDMAYA_API
    size_t SampleTransform(size_t maxSampleCount, float* times, GfMatrix4d* samples);
    HDMAYA_API
    bool            UpdateVisibility();
    bool            IsVisible(bool checkDirty = true);
    const MDagPath& GetDagPath() const { return _dagPath; }
    void            InvalidateTransform() { _invalidTransform = true; }
    bool            IsInstanced() const { return _isInstanced; }
    HDMAYA_API
    SdfPath GetInstancerID() const;
    HDMAYA_API
    virtual VtIntArray GetInstanceIndices(const SdfPath& prototypeId);
    HDMAYA_API
    HdPrimvarDescriptorVector GetInstancePrimvarDescriptors(HdInterpolation interpolation) const;
    HDMAYA_API
    VtValue GetInstancePrimvar(const TfToken& key);

protected:
    HDMAYA_API
    void _AddHierarchyChangedCallbacks(MDagPath& dag);
    HDMAYA_API
    virtual bool _GetVisibility() const;

private:
    MDagPath   _dagPath;
    GfMatrix4d _transform;
    bool       _isVisible = true;
    bool       _visibilityDirty = true;
    bool       _invalidTransform = true;
    bool       _isInstanced = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_DG_ADAPTER_H
