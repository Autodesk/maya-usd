//
// Copyright 2018 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include <pxr/pxr.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/glf/simpleLight.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hdx/shadowMatrixComputation.h>
#include <pxr/imaging/hdx/simpleLightTask.h>
#include <pxr/usd/usdLux/tokens.h>

#include <maya/MColor.h>
#include <maya/MFnSpotLight.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>

#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/adapters/lightAdapter.h>
#include <hdmaya/mayaAttrs.h>

#include <hdmaya/utils.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
void GetSpotCutoffAndSoftness(MFnSpotLight& mayaLight, float& cutoffOut, float& softnessOut) {
    // Divided by two.
    auto coneAngle = static_cast<float>(GfRadiansToDegrees(mayaLight.coneAngle())) * 0.5f;
    auto penumbraAngle = static_cast<float>(GfRadiansToDegrees(mayaLight.penumbraAngle()));
    cutoffOut = coneAngle + penumbraAngle;
    softnessOut = cutoffOut / penumbraAngle;
}

float GetSpotCutoff(MFnSpotLight& mayaLight) {
    float cutoff;
    float softness;
    GetSpotCutoffAndSoftness(mayaLight, cutoff, softness);
    return cutoff;
}

float GetSpotSoftness(MFnSpotLight& mayaLight) {
    float cutoff;
    float softness;
    GetSpotCutoffAndSoftness(mayaLight, cutoff, softness);
    return softness;
}

float GetSpotFalloff(MFnSpotLight& mayaLight) { return static_cast<float>(mayaLight.dropOff()); }
} // namespace

class HdMayaSpotLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaSpotLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaLightAdapter(delegate, dag) {}

    const TfToken& LightType() override {
        if (GetDelegate()->GetPreferSimpleLight()) {
            return HdPrimTypeTokens->simpleLight;
        } else {
            return HdPrimTypeTokens->sphereLight;
        }
    }

protected:
    void _CalculateLightParams(GlfSimpleLight& light) override {
        MStatus status;
        MFnSpotLight mayaLight(GetDagPath(), &status);
        if (TF_VERIFY(status)) {
            light.SetHasShadow(true);
            light.SetSpotCutoff(GetSpotCutoff(mayaLight));
            light.SetSpotFalloff(GetSpotFalloff(mayaLight));
        }
    }

    VtValue Get(const TfToken& key) override {
        TF_DEBUG(HDMAYA_ADAPTER_GET)
            .Msg(
                "Called HdMayaSpotLightAdapter::Get(%s) - %s\n", key.GetText(),
                GetDagPath().partialPathName().asChar());

        if (key == HdLightTokens->shadowParams) {
            HdxShadowParams shadowParams;
            MFnLight mayaLight(GetDagPath());
            const auto useDepthMapShadows =
                mayaLight.findPlug(MayaAttrs::useDepthMapShadows, true).asBool();
            if (!useDepthMapShadows) {
                shadowParams.enabled = false;
                return VtValue(shadowParams);
            }

            auto coneAnglePlug = mayaLight.findPlug(MayaAttrs::coneAngle, true);
            if (coneAnglePlug.isNull()) { return {}; }

            GfFrustum frustum;
            frustum.SetPositionAndRotationFromMatrix(
                getGfMatrixFromMaya(GetDagPath().inclusiveMatrix()));
            frustum.SetProjectionType(GfFrustum::Perspective);
            frustum.SetPerspective(
                GfRadiansToDegrees(coneAnglePlug.asFloat()), true, 1.0f, 1.0f, 50.0f);

            GetDelegate()->FitFrustumToRprims(frustum);
            _CalculateShadowParams(mayaLight, frustum, shadowParams);
            return VtValue(shadowParams);
        }

        return HdMayaLightAdapter::Get(key);
    }

    VtValue GetLightParamValue(const TfToken& paramName) override {
        TF_DEBUG(HDMAYA_ADAPTER_GET_LIGHT_PARAM_VALUE)
            .Msg(
                "Called HdMayaSpotLightAdapter::GetLightParamValue(%s) - %s\n", paramName.GetText(),
                GetDagPath().partialPathName().asChar());

        MStatus status;
        MFnSpotLight light(GetDagPath(), &status);
        if (TF_VERIFY(status)) {
            if (paramName == UsdLuxTokens->radius) {
                const float radius = light.shadowRadius();
                return VtValue(radius);
            } else if (paramName == UsdLuxTokens->treatAsPoint) {
                const bool treatAsPoint = (light.shadowRadius() == 0.0);
                return VtValue(treatAsPoint);
            } else if (paramName == UsdLuxTokens->shapingConeAngle) {
                return VtValue(GetSpotCutoff(light));
            } else if (paramName == UsdLuxTokens->shapingConeSoftness) {
                return VtValue(GetSpotSoftness(light));
            } else if (paramName == UsdLuxTokens->shapingFocus) {
                return VtValue(GetSpotFalloff(light));
            }
        }
        return HdMayaLightAdapter::GetLightParamValue(paramName);
    }
};

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<HdMayaSpotLightAdapter, TfType::Bases<HdMayaLightAdapter> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterLightAdapter(
        TfToken("spotLight"),
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> HdMayaLightAdapterPtr {
            return HdMayaLightAdapterPtr(new HdMayaSpotLightAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
