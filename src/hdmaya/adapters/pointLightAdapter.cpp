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
#include <pxr/imaging/hd/light.h>
#include <pxr/usd/usdLux/tokens.h>

#include <maya/MFnPointLight.h>

#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/adapters/lightAdapter.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaPointLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaPointLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaLightAdapter(delegate, dag) {}

    const TfToken& LightType() override {
        if (GetDelegate()->GetPreferSimpleLight()) {
            return HdPrimTypeTokens->simpleLight;
        } else {
            return HdPrimTypeTokens->sphereLight;
        }
    }

    VtValue GetLightParamValue(const TfToken& paramName) override {
        TF_DEBUG(HDMAYA_ADAPTER_GET_LIGHT_PARAM_VALUE)
            .Msg(
                "Called HdMayaPointLightAdapter::GetLightParamValue(%s) - %s\n",
                paramName.GetText(), GetDagPath().partialPathName().asChar());

        MFnPointLight light(GetDagPath());
        if (paramName == UsdLuxTokens->radius) {
            const float radius = light.shadowRadius();
            return VtValue(radius);
        } else if (paramName == UsdLuxTokens->treatAsPoint) {
            const bool treatAsPoint = (light.shadowRadius() == 0.0);
            return VtValue(treatAsPoint);
        }
        return HdMayaLightAdapter::GetLightParamValue(paramName);
    }
};

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<
        HdMayaPointLightAdapter, TfType::Bases<HdMayaLightAdapter> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterLightAdapter(
        TfToken("pointLight"),
        [](HdMayaDelegateCtx* delegate,
           const MDagPath& dag) -> HdMayaLightAdapterPtr {
            return HdMayaLightAdapterPtr(
                new HdMayaPointLightAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
