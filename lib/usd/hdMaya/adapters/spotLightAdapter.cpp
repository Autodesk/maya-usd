//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <hdMaya/adapters/adapterDebugCodes.h>
#include <hdMaya/adapters/adapterRegistry.h>
#include <hdMaya/adapters/lightAdapter.h>
#include <hdMaya/adapters/mayaAttrs.h>
#include <hdMaya/utils.h>

#include <pxr/base/tf/type.h>
#include <pxr/imaging/glf/simpleLight.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hdx/shadowMatrixComputation.h>
#include <pxr/imaging/hdx/simpleLightTask.h>
#include <pxr/pxr.h>
#include <pxr/usd/usdLux/tokens.h>

#include <maya/MColor.h>
#include <maya/MFnSpotLight.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
void GetSpotCutoffAndSoftness(MFnSpotLight& mayaLight, float& cutoffOut, float& softnessOut)
{
    // Divided by two.
    auto coneAngle = static_cast<float>(GfRadiansToDegrees(mayaLight.coneAngle())) * 0.5f;
    auto penumbraAngle = static_cast<float>(GfRadiansToDegrees(mayaLight.penumbraAngle()));
    cutoffOut = coneAngle + penumbraAngle;
    softnessOut = cutoffOut == 0 ? 0 : penumbraAngle / cutoffOut;
}

float GetSpotCutoff(MFnSpotLight& mayaLight)
{
    float cutoff;
    float softness;
    GetSpotCutoffAndSoftness(mayaLight, cutoff, softness);
    return cutoff;
}

float GetSpotSoftness(MFnSpotLight& mayaLight)
{
    float cutoff;
    float softness;
    GetSpotCutoffAndSoftness(mayaLight, cutoff, softness);
    return softness;
}

float GetSpotFalloff(MFnSpotLight& mayaLight) { return static_cast<float>(mayaLight.dropOff()); }
} // namespace

class HdMayaSpotLightAdapter : public HdMayaLightAdapter
{
public:
    HdMayaSpotLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaLightAdapter(delegate, dag)
    {
    }

    const TfToken& LightType() const override
    {
        if (GetDelegate()->IsHdSt()) {
            return HdPrimTypeTokens->simpleLight;
        } else {
            return HdPrimTypeTokens->sphereLight;
        }
    }

protected:
    void _CalculateLightParams(GlfSimpleLight& light) override
    {
        MStatus      status;
        MFnSpotLight mayaLight(GetDagPath(), &status);
        if (TF_VERIFY(status)) {
            light.SetHasShadow(true);
            light.SetSpotCutoff(GetSpotCutoff(mayaLight));
            light.SetSpotFalloff(GetSpotFalloff(mayaLight));
        }
    }

    VtValue Get(const TfToken& key) override
    {
        TF_DEBUG(HDMAYA_ADAPTER_GET)
            .Msg(
                "Called HdMayaSpotLightAdapter::Get(%s) - %s\n",
                key.GetText(),
                GetDagPath().partialPathName().asChar());

        if (key == HdLightTokens->shadowParams) {
            HdxShadowParams shadowParams;
            MFnSpotLight    mayaLight(GetDagPath());
            if (!GetShadowsEnabled(mayaLight)) {
                shadowParams.enabled = false;
                return VtValue(shadowParams);
            }

            _CalculateShadowParams(mayaLight, shadowParams);
            // Use the radius as the "blur" amount, for PCSS
            shadowParams.blur = mayaLight.shadowRadius();
            return VtValue(shadowParams);
        }

        return HdMayaLightAdapter::Get(key);
    }

    VtValue GetLightParamValue(const TfToken& paramName) override
    {
        TF_DEBUG(HDMAYA_ADAPTER_GET_LIGHT_PARAM_VALUE)
            .Msg(
                "Called HdMayaSpotLightAdapter::GetLightParamValue(%s) - %s\n",
                paramName.GetText(),
                GetDagPath().partialPathName().asChar());

        MStatus      status;
        MFnSpotLight light(GetDagPath(), &status);
        if (TF_VERIFY(status)) {
            if (paramName == HdLightTokens->radius) {
                const float radius = light.shadowRadius();
                return VtValue(radius);
            } else if (paramName == UsdLuxTokens->treatAsPoint) {
                const bool treatAsPoint = (light.shadowRadius() == 0.0);
                return VtValue(treatAsPoint);
            } else if (paramName == HdLightTokens->shapingConeAngle) {
                return VtValue(GetSpotCutoff(light));
            } else if (paramName == HdLightTokens->shapingConeSoftness) {
                return VtValue(GetSpotSoftness(light));
            } else if (paramName == HdLightTokens->shapingFocus) {
                return VtValue(GetSpotFalloff(light));
            }
        }
        return HdMayaLightAdapter::GetLightParamValue(paramName);
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaSpotLightAdapter, TfType::Bases<HdMayaLightAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight)
{
    HdMayaAdapterRegistry::RegisterLightAdapter(
        TfToken("spotLight"),
        [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> HdMayaLightAdapterPtr {
            return HdMayaLightAdapterPtr(new HdMayaSpotLightAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
