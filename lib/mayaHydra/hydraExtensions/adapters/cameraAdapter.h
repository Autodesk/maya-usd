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
#ifndef MAYAHYDRALIB_CAMERA_ADAPTER_H
#define MAYAHYDRALIB_CAMERA_ADAPTER_H

#include <mayaHydraLib/adapters/dagAdapter.h>
#include <mayaHydraLib/adapters/shapeAdapter.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

/**
 * \brief MayaHydraCameraAdapter is used to handle the translation from a Maya camera to hydra.
 */
class MayaHydraCameraAdapter : public MayaHydraShapeAdapter
{
public:
    MAYAHYDRALIB_API
    MayaHydraCameraAdapter(MayaHydraDelegateCtx* delegate, const MDagPath& dag);

    MAYAHYDRALIB_API
    virtual ~MayaHydraCameraAdapter();

    MAYAHYDRALIB_API
    bool IsSupported() const override;

    MAYAHYDRALIB_API
    void MarkDirty(HdDirtyBits dirtyBits) override;

    MAYAHYDRALIB_API
    void Populate() override;

    MAYAHYDRALIB_API
    void RemovePrim() override;

    MAYAHYDRALIB_API
    bool HasType(const TfToken& typeId) const override;

    MAYAHYDRALIB_API
    VtValue Get(const TfToken& key) override;

    MAYAHYDRALIB_API
    VtValue GetCameraParamValue(const TfToken& paramName);

    MAYAHYDRALIB_API
    void CreateCallbacks() override;

    MAYAHYDRALIB_API
    void SetViewport(const GfVec4d& viewport);

protected:
    static TfToken CameraType();

    /// The use of a pointer here helps us track whether this camera is (or has ever been)
    /// the active viewport camera.  NOTE: it's possible that _viewport will be out of date
    /// after switching to a new camera and resizing the viewport, but _viewport will eventually
    /// be re-synched before any output/pixels of the stale size is requested.
    std::unique_ptr<GfVec4d> _viewport;
};

using MayaHydraCameraAdapterPtr = std::shared_ptr<MayaHydraCameraAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_CAMERA_ADAPTER_H
