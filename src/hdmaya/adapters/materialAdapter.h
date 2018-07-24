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
};

using HdMayaMaterialAdapterPtr = std::shared_ptr<HdMayaMaterialAdapter>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_MATERIAL_ADAPTER_H__
