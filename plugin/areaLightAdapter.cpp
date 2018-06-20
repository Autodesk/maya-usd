#include <pxr/pxr.h>

#include <pxr/imaging/hd/light.h>

#include "lightAdapter.h"
#include "adapterRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAreaLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaAreaLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
    : HdMayaLightAdapter(delegate, dag) {

    }

    void Populate() override {
        GetDelegate()->InsertSprim(HdPrimTypeTokens->rectLight, GetID(), HdLight::AllDirty);
    }
};


TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterDagAdapter("areaLight",
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> std::shared_ptr<HdMayaDagAdapter> {
            return std::static_pointer_cast<HdMayaDagAdapter>(std::make_shared<HdMayaAreaLightAdapter>(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
