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

#include "lightAdapter.h"
#include "adapterRegistry.h"
#include "../utils.h"

// FFFFFFFFFFFFFFFffffffffffffffffff
#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

class SpotShadowMatrix : public HdxShadowMatrixComputation
{
public:
    SpotShadowMatrix(const GlfSimpleLight& light, const GfMatrix4d& mat) {
        GfFrustum frustum;
        frustum.SetPositionAndRotationFromMatrix(mat);
        frustum.SetProjectionType(GfFrustum::Perspective);
        frustum.SetPerspective(
            light.GetSpotCutoff() * 2.0,
            true,
            1.0f,
            1.0f,
            50.0f);

        _shadowMatrix = frustum.ComputeViewMatrix() * frustum.ComputeProjectionMatrix();
    }

    virtual GfMatrix4d Compute(const GfVec4f &viewport,
                               CameraUtilConformWindowPolicy policy) {
        return _shadowMatrix;
    }
private:
    GfMatrix4d _shadowMatrix;
};

}

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

        _shadowMatrix = boost::static_pointer_cast<HdxShadowMatrixComputation>(
            boost::make_shared<SpotShadowMatrix>(light, getGfMatrixFromMaya(GetDagPath().inclusiveMatrix())));
    }

    VtValue Get(const TfToken& key) override {
        if (key == HdLightTokens->shadowParams) {
            HdxShadowParams shadowParams;
            shadowParams.enabled = true;
            shadowParams.resolution = 1024;
            shadowParams.shadowMatrix = _shadowMatrix;
            shadowParams.bias = -0.001;
            return VtValue(shadowParams);
        }

        return HdMayaLightAdapter::Get(key);
    }

    HdxShadowMatrixComputationSharedPtr _shadowMatrix;
};


TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterDagAdapter(
    "spotLight",
    [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> std::shared_ptr<HdMayaDagAdapter> {
        return std::static_pointer_cast<HdMayaDagAdapter>(std::make_shared<HdMayaSpotLightAdapter>(delegate, dag));
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
