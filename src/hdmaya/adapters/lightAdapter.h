//
// Copyright 2018 Luma Pictures
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
#ifndef __HDMAYA_LIGHT_ADAPTER_H__
#define __HDMAYA_LIGHT_ADAPTER_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/frustum.h>

#include <pxr/imaging/glf/simpleLight.h>
#include <pxr/imaging/hdx/simpleLightTask.h>

#include <maya/MFnLight.h>

#include <hdmaya/adapters/dagAdapter.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaLightAdapter : public HdMayaDagAdapter {
public:
    HDMAYA_API
    HdMayaLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag);
    HDMAYA_API
    virtual ~HdMayaLightAdapter() = default;
    HDMAYA_API
    virtual const TfToken& LightType() = 0;
    HDMAYA_API
    bool IsSupported() override;
    HDMAYA_API
    void Populate() override;
    HDMAYA_API
    void MarkDirty(HdDirtyBits dirtyBits) override;
    HDMAYA_API
    virtual void RemovePrim() override;
    HDMAYA_API
    bool HasType(const TfToken& typeId) override;
    HDMAYA_API
    virtual VtValue GetLightParamValue(const TfToken& paramName);
    HDMAYA_API
    VtValue Get(const TfToken& key) override;
    HDMAYA_API
    virtual void CreateCallbacks() override;

protected:
    HDMAYA_API
    virtual void _CalculateLightParams(GlfSimpleLight& light);
    HDMAYA_API
    void _CalculateShadowParams(MFnLight& light, GfFrustum& frustum, HdxShadowParams& params);
};

using HdMayaLightAdapterPtr = std::shared_ptr<HdMayaLightAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_LIGHT_ADAPTER_H__
