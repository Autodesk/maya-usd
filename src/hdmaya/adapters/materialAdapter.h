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
#ifndef __HDMAYA_MATERIAL_ADAPTER_H__
#define __HDMAYA_MATERIAL_ADAPTER_H__

#include <pxr/pxr.h>

#include <hdmaya/adapters/adapter.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaMaterialAdapter : public HdMayaAdapter {
public:
    HDMAYA_API
    HdMayaMaterialAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate, const MObject& node);
    HDMAYA_API
    virtual ~HdMayaMaterialAdapter() = default;

    HDMAYA_API
    bool IsSupported() override;

    HDMAYA_API
    bool HasType(const TfToken& typeId) override;

    HDMAYA_API
    void MarkDirty(HdDirtyBits dirtyBits) override;
    HDMAYA_API
    void RemovePrim() override;
    HDMAYA_API
    void Populate() override;

    HDMAYA_API
    virtual std::string GetSurfaceShaderSource();
    HDMAYA_API
    virtual std::string GetDisplacementShaderSource();
    HDMAYA_API
    virtual VtValue GetMaterialParamValue(const TfToken& paramName);
    HDMAYA_API
    virtual HdMaterialParamVector GetMaterialParams();
    HDMAYA_API
    virtual HdTextureResource::ID GetTextureResourceID(const TfToken& paramName);
    HDMAYA_API
    virtual HdTextureResourceSharedPtr GetTextureResource(const TfToken& paramName);
    HDMAYA_API
    virtual VtValue GetMaterialResource();

    HDMAYA_API
    static const HdMaterialParamVector& GetPreviewMaterialParams();
    HDMAYA_API
    static const std::string& GetPreviewSurfaceSource();
    HDMAYA_API
    static const std::string& GetPreviewDisplacementSource();
    HDMAYA_API
    static const VtValue& GetPreviewMaterialParamValue(const TfToken& paramName);

    HDMAYA_API
    static VtValue GetPreviewMaterialResource(const SdfPath& materialID);
};

using HdMayaMaterialAdapterPtr = std::shared_ptr<HdMayaMaterialAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_MATERIAL_ADAPTER_H__
