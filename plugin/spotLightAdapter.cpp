#include <pxr/pxr.h>

#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/glf/simpleLight.h>
#include <pxr/imaging/hdx/simpleLightTask.h>

#include <maya/MColor.h>
#include <maya/MFnLight.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>

#include "lightAdapter.h"
#include "adapterRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaSpotLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaSpotLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
    : HdMayaLightAdapter(delegate, dag) {

    }
protected:
    void CalculateLightParams(GlfSimpleLight& light) override {
        MFnLight mayaLight(GetDagPath().node());
        light.SetHasShadow(true);
        auto coneAnglePlug = mayaLight.findPlug("coneAngle");
        if (!coneAnglePlug.isNull()) {
            // Divided by two.
            light.SetSpotCutoff(90.0f * coneAnglePlug.asFloat() / static_cast<float>(M_PI));
        }
        auto dropoffPlug = mayaLight.findPlug("dropoff");
        if (!dropoffPlug.isNull()) {
            light.SetSpotFalloff(dropoffPlug.asFloat());
        }
    }

    VtValue Get(const TfToken& key) override {
        if (key == HdLightTokens->shadowParams) {
            HdxShadowParams shadowParams;
            shadowParams.enabled = true;
            shadowParams.resolution = 1024;
            return VtValue(shadowParams);
        }

        return HdMayaLightAdapter::Get(key);
    }
};


TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterDagAdapter(
    "spotLight",
    [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> std::shared_ptr<HdMayaDagAdapter> {
        return std::static_pointer_cast<HdMayaDagAdapter>(std::make_shared<HdMayaSpotLightAdapter>(delegate, dag));
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
