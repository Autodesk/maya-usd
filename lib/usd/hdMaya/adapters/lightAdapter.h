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
#ifndef HDMAYA_LIGHT_ADAPTER_H
#define HDMAYA_LIGHT_ADAPTER_H

#include <hdMaya/adapters/dagAdapter.h>

#include <pxr/base/gf/frustum.h>
#include <pxr/imaging/glf/simpleLight.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hdx/simpleLightTask.h>
#include <pxr/pxr.h>

#include <maya/MFnLight.h>
#include <maya/MFnNonExtendedLight.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaLightAdapter : public HdMayaDagAdapter
{
public:
    inline bool GetShadowsEnabled(MFnNonExtendedLight& light)
    {
        return light.useDepthMapShadows() || light.useRayTraceShadows();
    }

    HDMAYA_API
    HdMayaLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag);
    HDMAYA_API
    virtual ~HdMayaLightAdapter() = default;
    HDMAYA_API
    virtual const TfToken& LightType() const = 0;
    HDMAYA_API
    bool IsSupported() const override;
    HDMAYA_API
    void Populate() override;
    HDMAYA_API
    void MarkDirty(HdDirtyBits dirtyBits) override;
    HDMAYA_API
    virtual void RemovePrim() override;
    HDMAYA_API
    bool HasType(const TfToken& typeId) const override;
    HDMAYA_API
    virtual VtValue GetLightParamValue(const TfToken& paramName);
    HDMAYA_API
    VtValue Get(const TfToken& key) override;
    HDMAYA_API
    virtual void CreateCallbacks() override;
    HDMAYA_API
    void SetShadowProjectionMatrix(const GfMatrix4d& matrix);

protected:
    HDMAYA_API
    virtual void _CalculateLightParams(GlfSimpleLight& light) { }
    HDMAYA_API
    void _CalculateShadowParams(MFnLight& light, HdxShadowParams& params);
    HDMAYA_API
    bool _GetVisibility() const override;

    GfMatrix4d _shadowProjectionMatrix;
};

using HdMayaLightAdapterPtr = std::shared_ptr<HdMayaLightAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_LIGHT_ADAPTER_H
