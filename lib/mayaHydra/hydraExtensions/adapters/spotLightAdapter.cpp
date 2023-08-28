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
#include <mayaHydraLib/adapters/adapterDebugCodes.h>
#include <mayaHydraLib/adapters/adapterRegistry.h>
#include <mayaHydraLib/adapters/lightAdapter.h>
#include <mayaHydraLib/adapters/mayaAttrs.h>
#include <mayaHydraLib/mayaHydraSceneProducer.h>

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

/**
 * \brief MayaHydraSpotLightAdapter is used to handle the translation from a Maya spot light to
 * hydra.
 */
class MayaHydraSpotLightAdapter : public MayaHydraLightAdapter
{
public:
    MayaHydraSpotLightAdapter(MayaHydraSceneProducer* producer, const MDagPath& dag)
        : MayaHydraLightAdapter(producer, dag)
    {
    }

    const TfToken& LightType() const override
    {
        if (GetSceneProducer()->IsHdSt()) {
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
        TF_DEBUG(MAYAHYDRALIB_ADAPTER_GET)
            .Msg(
                "Called MayaHydraSpotLightAdapter::Get(%s) - %s\n",
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

        return MayaHydraLightAdapter::Get(key);
    }

    VtValue GetLightParamValue(const TfToken& paramName) override
    {
        TF_DEBUG(MAYAHYDRALIB_ADAPTER_GET_LIGHT_PARAM_VALUE)
            .Msg(
                "Called MayaHydraSpotLightAdapter::GetLightParamValue(%s) - %s\n",
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
            } else if (paramName == UsdLuxTokens->inputsShapingConeAngle) {
                return VtValue(GetSpotCutoff(light));
            } else if (paramName == UsdLuxTokens->inputsShapingConeSoftness) {
                return VtValue(GetSpotSoftness(light));
            } else if (paramName == UsdLuxTokens->inputsShapingFocus) {
                return VtValue(GetSpotFalloff(light));
            }
        }
        return MayaHydraLightAdapter::GetLightParamValue(paramName);
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<MayaHydraSpotLightAdapter, TfType::Bases<MayaHydraLightAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(MayaHydraAdapterRegistry, pointLight)
{
    MayaHydraAdapterRegistry::RegisterLightAdapter(
        TfToken("spotLight"),
        [](MayaHydraSceneProducer* producer, const MDagPath& dag) -> MayaHydraLightAdapterPtr {
            return MayaHydraLightAdapterPtr(new MayaHydraSpotLightAdapter(producer, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
