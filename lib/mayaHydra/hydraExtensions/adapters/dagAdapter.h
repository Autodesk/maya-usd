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
#ifndef MAYAHYDRALIB_DG_ADAPTER_H
#define MAYAHYDRALIB_DG_ADAPTER_H

#include <mayaHydraLib/adapters/adapter.h>
#include <mayaHydraLib/adapters/adapterDebugCodes.h>
#include <mayaHydraLib/utils.h>

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

/**
 * \brief MayaHydraDagAdapter is the adapter base class for any dag object.
 */
class MayaHydraDagAdapter : public MayaHydraAdapter
{
protected:
    MAYAHYDRALIB_API
    MayaHydraDagAdapter(const SdfPath& id, MayaHydraDelegateCtx* delegate, const MDagPath& dagPath);

public:
    MAYAHYDRALIB_API
    virtual ~MayaHydraDagAdapter() = default;
    MAYAHYDRALIB_API
    virtual bool GetVisible() override { return IsVisible(); }
    MAYAHYDRALIB_API
    virtual void CreateCallbacks() override;
    MAYAHYDRALIB_API
    virtual void MarkDirty(HdDirtyBits dirtyBits) override;
    MAYAHYDRALIB_API
    virtual void RemovePrim() override;
    MAYAHYDRALIB_API
    GfMatrix4d GetTransform() override;
    MAYAHYDRALIB_API
    size_t SampleTransform(size_t maxSampleCount, float* times, GfMatrix4d* samples);
    MAYAHYDRALIB_API
    bool            UpdateVisibility();
    bool            IsVisible(bool checkDirty = true);
    const MDagPath& GetDagPath() const { return _dagPath; }
    void            InvalidateTransform() { _invalidTransform = true; }
    bool            IsInstanced() const { return _isInstanced; }
    MAYAHYDRALIB_API
    SdfPath GetInstancerID() const;
    MAYAHYDRALIB_API
    virtual VtIntArray GetInstanceIndices(const SdfPath& prototypeId);
    MAYAHYDRALIB_API
    HdPrimvarDescriptorVector GetInstancePrimvarDescriptors(HdInterpolation interpolation) const;
    MAYAHYDRALIB_API
    VtValue GetInstancePrimvar(const TfToken& key);

protected:
    MAYAHYDRALIB_API
    void _AddHierarchyChangedCallbacks(MDagPath& dag);
    MAYAHYDRALIB_API
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

#endif // MAYAHYDRALIB_DG_ADAPTER_H
