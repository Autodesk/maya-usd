#include <pxr/pxr.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hdx/simpleLightTask.h>
#include <pxr/imaging/hdx/shadowMatrixComputation.h>
#include <pxr/imaging/glf/simpleLight.h>

#include <maya/MColor.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>

#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/adapters/lightAdapter.h>

#include <hdmaya/utils.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaSpotLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaSpotLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaLightAdapter(delegate, dag) {

    }
protected:
    void
    CalculateLightParams(GlfSimpleLight& light) override {
        MFnLight mayaLight(GetDagPath().node());
        light.SetHasShadow(true);
        auto coneAnglePlug = mayaLight.findPlug("coneAngle", true);
        if (!coneAnglePlug.isNull()) {
            // Divided by two.
            light.SetSpotCutoff(
                static_cast<float>(GfRadiansToDegrees(coneAnglePlug.asFloat())) * 0.5f);
        }
        auto dropoffPlug = mayaLight.findPlug("dropoff", true);
        if (!dropoffPlug.isNull()) {
            light.SetSpotFalloff(dropoffPlug.asFloat());
        }
    }

    void
    Populate() override {
        GetDelegate()->InsertSprim(HdPrimTypeTokens->simpleLight, GetID(), HdLight::AllDirty);
    }

    bool
    IsSupported() override {
        return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->simpleLight);
    }

    VtValue
    Get(const TfToken& key) override {
        if (key == HdLightTokens->shadowParams) {
            HdxShadowParams shadowParams;
            MFnLight mayaLight(GetDagPath().node());
            const auto useDepthMapShadows = mayaLight.findPlug("useDepthMapShadows", true).asBool();
            if (!useDepthMapShadows) {
                shadowParams.enabled = false;
                return VtValue(shadowParams);
            }

            auto coneAnglePlug = mayaLight.findPlug("coneAngle", true);
            if (coneAnglePlug.isNull()) { return {}; }


            GfFrustum frustum;
            frustum.SetPositionAndRotationFromMatrix(getGfMatrixFromMaya(GetDagPath().inclusiveMatrix()));
            frustum.SetProjectionType(GfFrustum::Perspective);
            frustum.SetPerspective(
                GfRadiansToDegrees(coneAnglePlug.asFloat()),
                true,
                1.0f,
                1.0f,
                50.0f);

            GetDelegate()->FitFrustumToRprims(frustum);
            CalculateShadowParams(mayaLight, frustum, shadowParams);
            return VtValue(shadowParams);
        }

        return HdMayaLightAdapter::Get(key);
    }

    bool
    HasType(const TfToken& typeId) override {
        return typeId == HdPrimTypeTokens->simpleLight;
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaSpotLightAdapter, TfType::Bases<HdMayaLightAdapter> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterDagAdapter(
        "spotLight",
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> std::shared_ptr<HdMayaDagAdapter> {
            return std::static_pointer_cast<HdMayaDagAdapter>(std::make_shared<HdMayaSpotLightAdapter>(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
