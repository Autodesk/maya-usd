#include <pxr/pxr.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/light.h>

#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/lightAdapter.h>
#include <hdmaya/adapters/adapterRegistry.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAreaLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaAreaLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
    : HdMayaLightAdapter(delegate, dag) {

    }

    void
    _CalculateLightParams(GlfSimpleLight& light) override {
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

    VtValue
    GetLightParamValue(const TfToken& paramName) override {
        TF_DEBUG(HDMAYA_ADAPTER_GET).Msg(
                "Called HdMayaAreaLightAdapter::GetLightParamValue(%s) - %s\n",
                paramName.GetText(),
                GetDagPath().partialPathName().asChar());

        if (paramName == HdLightTokens->width) {
            return VtValue(1.0f);
        } else if (paramName == HdLightTokens->height) {
            return VtValue(1.0f);
        }
        return HdMayaLightAdapter::GetLightParamValue(paramName);
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

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaAreaLightAdapter, TfType::Bases<HdMayaLightAdapter> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterLightAdapter(
        TfToken("areaLight"),
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> HdMayaLightAdapterPtr {
            return HdMayaLightAdapterPtr(new HdMayaAreaLightAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
