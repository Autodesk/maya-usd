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
#ifndef MAYAHYDRALIB_LIGHT_ADAPTER_H
#define MAYAHYDRALIB_LIGHT_ADAPTER_H

#include <mayaHydraLib/adapters/dagAdapter.h>

#include <pxr/base/gf/frustum.h>
#include <pxr/imaging/glf/simpleLight.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hdx/simpleLightTask.h>
#include <pxr/pxr.h>

#include <maya/MFnLight.h>
#include <maya/MFnNonExtendedLight.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraLightAdapter : public MayaHydraDagAdapter
{
public:
    inline bool GetShadowsEnabled(MFnNonExtendedLight& light)
    {
        return light.useDepthMapShadows() || light.useRayTraceShadows();
    }

    MAYAHYDRALIB_API
    MayaHydraLightAdapter(MayaHydraDelegateCtx* delegate, const MDagPath& dag);
    MAYAHYDRALIB_API
    virtual ~MayaHydraLightAdapter() = default;
    MAYAHYDRALIB_API
    virtual const TfToken& LightType() const = 0;
    MAYAHYDRALIB_API
    bool IsSupported() const override;
    MAYAHYDRALIB_API
    void Populate() override;
    MAYAHYDRALIB_API
    void MarkDirty(HdDirtyBits dirtyBits) override;
    MAYAHYDRALIB_API
    virtual void RemovePrim() override;
    MAYAHYDRALIB_API
    bool HasType(const TfToken& typeId) const override;
    MAYAHYDRALIB_API
    virtual VtValue GetLightParamValue(const TfToken& paramName);
    MAYAHYDRALIB_API
    VtValue Get(const TfToken& key) override;
    MAYAHYDRALIB_API
    virtual void CreateCallbacks() override;
    MAYAHYDRALIB_API
    void SetShadowProjectionMatrix(const GfMatrix4d& matrix);
    MAYAHYDRALIB_API
    void SetLightingOn(bool isLightingOn);

protected:
    MAYAHYDRALIB_API
    virtual void _CalculateLightParams(GlfSimpleLight& light) { }
    MAYAHYDRALIB_API
    void _CalculateShadowParams(MFnLight& light, HdxShadowParams& params);
    MAYAHYDRALIB_API
    bool _GetVisibility() const override;

    GfMatrix4d _shadowProjectionMatrix;
    bool _isLightingOn = true;
};

using MayaHydraLightAdapterPtr = std::shared_ptr<MayaHydraLightAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_LIGHT_ADAPTER_H
