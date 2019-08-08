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

#include "adapterDebugCodes.h"
#include "adapterRegistry.h"
#include "lightAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAreaLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaAreaLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaLightAdapter(delegate, dag) {}

    void _CalculateLightParams(GlfSimpleLight& light) override {
        light.SetSpotCutoff(90.0f);
    }

    const TfToken& LightType() const override {
        if (GetDelegate()->IsHdSt()) {
            return HdPrimTypeTokens->simpleLight;
        } else {
            return HdPrimTypeTokens->rectLight;
        }
    }

    VtValue GetLightParamValue(const TfToken& paramName) override {
        TF_DEBUG(HDMAYA_ADAPTER_GET_LIGHT_PARAM_VALUE)
            .Msg(
                "Called HdMayaAreaLightAdapter::GetLightParamValue(%s) - %s\n",
                paramName.GetText(), GetDagPath().partialPathName().asChar());

        if (paramName == HdLightTokens->width) {
            return VtValue(2.0f);
        } else if (paramName == HdLightTokens->height) {
            return VtValue(2.0f);
        }
        return HdMayaLightAdapter::GetLightParamValue(paramName);
    }
};

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<
        HdMayaAreaLightAdapter, TfType::Bases<HdMayaLightAdapter> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterLightAdapter(
        TfToken("areaLight"),
        [](HdMayaDelegateCtx* delegate,
           const MDagPath& dag) -> HdMayaLightAdapterPtr {
            return HdMayaLightAdapterPtr(
                new HdMayaAreaLightAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
