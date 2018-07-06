#include <pxr/pxr.h>

#include <pxr/imaging/hd/light.h>

#include <hdmaya/adapters/lightAdapter.h>
#include <hdmaya/adapters/adapterRegistry.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAreaLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaAreaLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
    : HdMayaLightAdapter(delegate, dag) {

    }

    void
    CalculateLightParams(GlfSimpleLight& light) override {
        light.SetSpotCutoff(90.0f);
    }

    void
    Populate() override {
        if (GetDelegate()->GetPreferSimpleLight()) {
            GetDelegate()->InsertSprim(HdPrimTypeTokens->simpleLight, GetID(), HdLight::AllDirty);
        } else {
            GetDelegate()->InsertSprim(HdPrimTypeTokens->rectLight, GetID(), HdLight::AllDirty);
        }
    }

    bool
    IsSupported() override {
        if (GetDelegate()->GetPreferSimpleLight()) {
            return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->simpleLight);
        } else {
            return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->rectLight);
        }
    }

    void
    RemovePrim() override {
        if (GetDelegate()->GetPreferSimpleLight()) {
            GetDelegate()->RemoveSprim(HdPrimTypeTokens->simpleLight, GetID());
        } else {
            GetDelegate()->RemoveSprim(HdPrimTypeTokens->rectLight, GetID());
        }
    }

    bool
    HasType(const TfToken& typeId) override {
        if (GetDelegate()->GetPreferSimpleLight()) {
            return typeId == HdPrimTypeTokens->simpleLight;
        } else {
            return typeId == HdPrimTypeTokens->rectLight;
        }
    }
};

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterDagAdapter(
        "areaLight",
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> HdMayaDagAdapterPtr {
            return std::static_pointer_cast<HdMayaDagAdapter>(std::make_shared<HdMayaAreaLightAdapter>(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
