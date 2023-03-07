//
// Copyright 2021 Autodesk
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
#ifndef HDMAYA_CAMERA_ADAPTER_H
#define HDMAYA_CAMERA_ADAPTER_H

#include <hdMaya/adapters/dagAdapter.h>
#include <hdMaya/adapters/shapeAdapter.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaCameraAdapter : public HdMayaShapeAdapter
{
public:
    HDMAYA_API
    HdMayaCameraAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag);

    HDMAYA_API
    virtual ~HdMayaCameraAdapter();

    HDMAYA_API
    bool IsSupported() const override;

    HDMAYA_API
    void MarkDirty(HdDirtyBits dirtyBits) override;

    HDMAYA_API
    void Populate() override;

    HDMAYA_API
    void RemovePrim() override;

    HDMAYA_API
    bool HasType(const TfToken& typeId) const override;

    HDMAYA_API
    VtValue Get(const TfToken& key) override;

    HDMAYA_API
    VtValue GetCameraParamValue(const TfToken& paramName);

    HDMAYA_API
    void CreateCallbacks() override;

    HDMAYA_API
    void SetViewport(const GfVec4d& viewport);

protected:
    static TfToken CameraType();

    // The use of a pointer here helps us track whether this camera is (or has ever been)
    // the active viewport camera.  NOTE: it's possile that _viewport will be out of date
    // after switching to a new camera and resizing the viewport, but _viewport will eventually
    // be re-synched before any output/pixels of the stale size is requested.
    std::unique_ptr<GfVec4d> _viewport;
};

using HdMayaCameraAdapterPtr = std::shared_ptr<HdMayaCameraAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_CAMERA_ADAPTER_H
