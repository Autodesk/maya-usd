#include <pxr/pxr.h>

#include <pxr/base/gf/frustum.h>

#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hdx/simpleLightTask.h>
#include <pxr/imaging/hdx/shadowMatrixComputation.h>
#include <pxr/imaging/glf/simpleLight.h>


#include <maya/MColor.h>
#include <maya/MFnLight.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>

#include "adapterRegistry.h"
#include "lightAdapter.h"
#include "constantShadowMatrix.h"

#include "../utils.h"
#include "../viewportRenderer.h"

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

    VtValue Get(const TfToken& key) override {
        if (key == HdLightTokens->shadowParams) {
            MFnLight mayaLight(GetDagPath().node());
            const auto useDepthMapShadows = mayaLight.findPlug("useDepthMapShadows", true).asBool();
            if (!useDepthMapShadows) {
                HdxShadowParams shadowParams;
                shadowParams.enabled = false;
                return VtValue(shadowParams);
            }

            auto coneAnglePlug = mayaLight.findPlug("coneAngle", true);
            if (coneAnglePlug.isNull()) { return {}; }

            auto dmapResolutionPlug = mayaLight.findPlug("dmapResolution");
            auto dmapBiasPlug = mayaLight.findPlug("dmapBias");
            auto dmapFilterSizePlug = mayaLight.findPlug("dmapFilterSize");

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

            const auto decayRate = mayaLight.findPlug("decayRate", true).asShort();
            if (decayRate > 0) {
                const auto color = mayaLight.color();
                const auto intensity = mayaLight.intensity();
                const auto maxIntensity = static_cast<double>(std::max(color.r * intensity,
                    std::max(color.g * intensity, color.b * intensity)));
                constexpr auto LIGHT_CUTOFF = 0.01;
                auto maxDistance = std::numeric_limits<double>::max();
                if (decayRate == 1) {
                    maxDistance = maxIntensity / LIGHT_CUTOFF;
                } else if (decayRate == 2) {
                    maxDistance = sqrt(maxIntensity / LIGHT_CUTOFF);
                }

                if (maxDistance < std::numeric_limits<double>::max()) {
                    const auto& nearFar = frustum.GetNearFar();
                    if (nearFar.GetMax() > maxDistance) {
                        if (nearFar.GetMin() < maxDistance) {
                            HdxShadowParams shadowParams;
                            shadowParams.enabled = false;
                            return VtValue(shadowParams);
                        } else {
                            frustum.SetNearFar(GfRange1d(nearFar.GetMin(), maxDistance));
                        }
                    }
                }
            }

            HdxShadowParams shadowParams;
            shadowParams.enabled = true;
            shadowParams.resolution = dmapResolutionPlug.isNull() ?
                HdMayaViewportRenderer::GetFallbackShadowMapResolution() : dmapResolutionPlug.asInt();
            shadowParams.shadowMatrix = boost::static_pointer_cast<HdxShadowMatrixComputation>(
                boost::make_shared<ConstantShadowMatrix>(frustum.ComputeViewMatrix() * frustum.ComputeProjectionMatrix()));
            shadowParams.bias = dmapBiasPlug.isNull() ?
                -0.001 : -dmapBiasPlug.asFloat();
            shadowParams.blur = dmapFilterSizePlug.isNull() ?
                0.0 : (static_cast<double>(dmapFilterSizePlug.asInt())) / static_cast<double>(shadowParams.resolution);
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
