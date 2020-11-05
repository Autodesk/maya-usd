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
#ifndef HDMAYA_AL_PROXY_USDIMAGING_DELEGATE_H
#define HDMAYA_AL_PROXY_USDIMAGING_DELEGATE_H

#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/MDagPath.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaUsdProxyShapeBase;

class HdMayaProxyUsdImagingDelegate : public UsdImagingDelegate
{
public:
    HdMayaProxyUsdImagingDelegate(
        HdRenderIndex*         parentIndex,
        SdfPath const&         delegateID,
        MayaUsdProxyShapeBase* proxy,
        const MDagPath&        dagPath);
    virtual ~HdMayaProxyUsdImagingDelegate();

    // We override this just because it's point that we can
    // catch that the root transform is dirty, and recalculate it.
    // (Ideally, we would override GetRootTransform, and leave this
    // alone, but GetRootTransform isn't a virtual func.) So
    // instead, we override this, and make sure that SetRootTransform
    // is called to mark all sub-prim's transforms dirty (so that
    // this override is then called.)
    virtual GfMatrix4d GetTransform(SdfPath const& id) override;

    // Same logic for overriding visibility as with transform, above
    virtual bool GetVisible(SdfPath const& id) override;

    void SetRootTransformDirty() { _rootTransformDirty = true; }
    void SetRootVisibilityDirty() { _rootVisibilityDirty = true; }

    /// Calls SetRootTransform with the current proxy shape's transform.
    ///
    /// Also has the side-effect of marking all sub-prim's transforms
    /// dirty, and marking root transform clean.
    void UpdateRootTransform();

    /// Calls SetRootVisibility with the current proxy shape's visibility.
    ///
    /// Also has the side-effect of marking all sub-prim's visibility
    /// dirty, and marking root visibility clean.
    void UpdateRootVisibility();

private:
    const MDagPath&        _dagPath;
    MayaUsdProxyShapeBase* _proxy { nullptr };
    bool                   _rootTransformDirty { false };
    bool                   _rootVisibilityDirty { false };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_AL_PROXY_USDIMAGING_DELEGATE_H
