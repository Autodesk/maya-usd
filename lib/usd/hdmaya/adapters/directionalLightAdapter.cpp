//
// Copyright 2019 Luma Pictures
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
#include <pxr/imaging/hd/light.h>
#include <pxr/usd/usdLux/tokens.h>

#include <maya/MFnDirectionalLight.h>

#include "adapterDebugCodes.h"
#include "adapterRegistry.h"
#include "lightAdapter.h"
#include "mayaAttrs.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDirectionalLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaDirectionalLightAdapter(
        HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaLightAdapter(delegate, dag) {}

    const TfToken& LightType() const override {
        if (GetDelegate()->IsHdSt()) {
            return HdPrimTypeTokens->simpleLight;
        } else {
            return HdPrimTypeTokens->distantLight;
        }
    }

    void _CalculateLightParams(GlfSimpleLight& light) override {
        // Directional lights point toward -Z, but we need the opposite
        // for the position so the light acts as a directional light.
        const auto direction = GfVec4f(0.0, 0.0, 1.0, 0.0) * GetTransform();
        light.SetHasShadow(true);
        light.SetPosition({direction[0], direction[1], direction[2], 0.0f});
    }

    VtValue Get(const TfToken& key) override {
        TF_DEBUG(HDMAYA_ADAPTER_GET)
            .Msg(
                "Called HdMayaSpotLightAdapter::Get(%s) - %s\n", key.GetText(),
                GetDagPath().partialPathName().asChar());

        if (key == HdLightTokens->shadowParams) {
            HdxShadowParams shadowParams;
            MFnDirectionalLight mayaLight(GetDagPath());
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

    VtValue GetLightParamValue(const TfToken& paramName) override {
        if (paramName == HdLightTokens->angle) {
            MStatus status;
            MFnDependencyNode lightNode(GetNode(), &status);
            if (ARCH_UNLIKELY(!status)) { return VtValue(0.0f); }
            return VtValue(
                lightNode
                    .findPlug(MayaAttrs::directionalLight::lightAngle, true)
                    .asFloat());
        } else {
            return HdMayaLightAdapter::GetLightParamValue(paramName);
        }
    }
};

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<
        HdMayaDirectionalLightAdapter, TfType::Bases<HdMayaLightAdapter> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterLightAdapter(
        TfToken("directionalLight"),
        [](HdMayaDelegateCtx* delegate,
           const MDagPath& dag) -> HdMayaLightAdapterPtr {
            return HdMayaLightAdapterPtr(
                new HdMayaDirectionalLightAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
