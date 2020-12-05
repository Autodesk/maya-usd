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
#ifndef HDMAYA_MATERIAL_ADAPTER_H
#define HDMAYA_MATERIAL_ADAPTER_H

#include <hdMaya/adapters/adapter.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaMaterialAdapter : public HdMayaAdapter
{
public:
    HDMAYA_API
    HdMayaMaterialAdapter(const SdfPath& id, HdMayaDelegateCtx* delegate, const MObject& node);
    HDMAYA_API
    virtual ~HdMayaMaterialAdapter() = default;

    HDMAYA_API
    bool IsSupported() const override;

    HDMAYA_API
    bool HasType(const TfToken& typeId) const override;

    HDMAYA_API
    void MarkDirty(HdDirtyBits dirtyBits) override;
    HDMAYA_API
    void RemovePrim() override;
    HDMAYA_API
    void Populate() override;

#if USD_VERSION_NUM > 1911 && USD_VERSION_NUM < 2011

    HDMAYA_API
    virtual HdTextureResourceSharedPtr GetTextureResource(const SdfPath& textureShaderId);
    HDMAYA_API
    virtual HdTextureResource::ID GetTextureResourceID(const TfToken& paramName);

#elif USD_VERSION_NUM <= 1911

    HDMAYA_API
    virtual std::string GetSurfaceShaderSource();
    HDMAYA_API
    virtual std::string GetDisplacementShaderSource();
    HDMAYA_API
    virtual VtValue GetMaterialParamValue(const TfToken& paramName);
    HDMAYA_API
    virtual HdMaterialParamVector GetMaterialParams();
    /// \brief Gets the Metadata for the Material.
    ///
    /// \return Dictionary holding the metadata.
    HDMAYA_API
    virtual VtDictionary GetMaterialMetadata();

    HDMAYA_API
    static const HdMaterialParamVector& GetPreviewMaterialParams();
    HDMAYA_API
    static const std::string& GetPreviewSurfaceSource();
    HDMAYA_API
    static const std::string& GetPreviewDisplacementSource();
    HDMAYA_API
    static const VtValue& GetPreviewMaterialParamValue(const TfToken& paramName);
    HDMAYA_API
    virtual HdTextureResourceSharedPtr GetTextureResource(const TfToken& paramName);
    HDMAYA_API
    virtual HdTextureResource::ID GetTextureResourceID(const TfToken& paramName);

#endif // USD_VERSION_NUM <= 1911

    HDMAYA_API
    virtual VtValue GetMaterialResource();

    /// \brief Updates the material tag for the material.
    ///
    /// \return True if the material tag have changed, false otherwise.
    virtual bool UpdateMaterialTag() { return false; }

    HDMAYA_API
    static VtValue GetPreviewMaterialResource(const SdfPath& materialID);
};

using HdMayaMaterialAdapterPtr = std::shared_ptr<HdMayaMaterialAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_MATERIAL_ADAPTER_H
