#include <pxr/pxr.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/light.h>

#include <hdmaya/adapters/lightAdapter.h>
#include <hdmaya/adapters/adapterRegistry.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaPointLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaPointLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaLightAdapter(delegate, dag) {

    }

    void
    Populate() override {
        if (GetDelegate()->GetPreferSimpleLight()) {
            GetDelegate()->InsertSprim(HdPrimTypeTokens->simpleLight, GetID(), HdLight::AllDirty);
        } else {
            GetDelegate()->InsertSprim(HdPrimTypeTokens->sphereLight, GetID(), HdLight::AllDirty);
        }
    }

    void
    RemovePrim() override {
        if (GetDelegate()->GetPreferSimpleLight()) {
            GetDelegate()->RemoveSprim(HdPrimTypeTokens->simpleLight, GetID());
        } else {
            GetDelegate()->RemoveSprim(HdPrimTypeTokens->sphereLight, GetID());
        }
    }

    bool
    IsSupported() override {
        if (GetDelegate()->GetPreferSimpleLight()) {
            return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->simpleLight);
        } else {
            return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->sphereLight);
        }
    }

    bool
    HasType(const TfToken& typeId) override {
        if (GetDelegate()->GetPreferSimpleLight()) {
            return typeId == HdPrimTypeTokens->simpleLight;
        } else {
            return typeId == HdPrimTypeTokens->sphereLight;
        }
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaPointLightAdapter, TfType::Bases<HdMayaLightAdapter> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterLightAdapter(
        TfToken("pointLight"),
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> HdMayaLightAdapterPtr {
            return HdMayaLightAdapterPtr(new HdMayaPointLightAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
