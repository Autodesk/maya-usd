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

#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/usdLux/tokens.h>

#include "adapterDebugCodes.h"
#include "adapterRegistry.h"
#include "lightAdapter.h"
#include "mayaAttrs.h"
#include "tokens.h"

#include <maya/MPlugArray.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAiSkyDomeLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaAiSkyDomeLightAdapter(
        HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaLightAdapter(delegate, dag) {}

    const TfToken& LightType() const override {
        return HdPrimTypeTokens->domeLight;
    }

    VtValue GetLightParamValue(const TfToken& paramName) override {
        MStatus status;
        MFnDependencyNode light(GetNode(), &status);
        if (ARCH_UNLIKELY(!status)) { return {}; }

        // We are not using precomputed attributes here, because we don't have
        // a guarantee that mtoa will be loaded before mtoh.
        if (paramName == HdLightTokens->color) {
            const auto plug = light.findPlug("color", true);
            MPlugArray conns;
            plug.connectedTo(conns, true, false);
            return VtValue(
                conns.length() > 0
                    ? GfVec3f(1.0f, 1.0f, 1.0f)
                    : GfVec3f(
                          plug.child(0).asFloat(), plug.child(1).asFloat(),
                          plug.child(2).asFloat()));
        } else if (paramName == HdLightTokens->intensity) {
            return VtValue(light.findPlug("intensity", true).asFloat());
        } else if (paramName == HdLightTokens->exposure) {
            return VtValue(light.findPlug("aiExposure", true).asFloat());
        } else if (paramName == HdLightTokens->normalize) {
            return VtValue(light.findPlug("aiNormalize", true).asBool());
        } else if (paramName == UsdLuxTokens->textureFormat) {
            const auto format = light.findPlug("format", true).asShort();
            // mirrored_ball : 0
            // angular : 1
            // latlong : 2
            if (format == 0) {
                return VtValue(UsdLuxTokens->mirroredBall);
            } else if (format == 2) {
                return VtValue(UsdLuxTokens->latlong);
            } else {
                return VtValue(UsdLuxTokens->automatic);
            }
        } else if (paramName == HdLightTokens->textureFile) {
            MPlugArray conns;
            light.findPlug("color", true).connectedTo(conns, true, false);
            if (conns.length() < 1) { return VtValue(SdfAssetPath()); }
            MFnDependencyNode file(conns[0].node(), &status);
            if (ARCH_UNLIKELY(
                    !status ||
                    (file.typeName() != HdMayaAdapterTokens->file.GetText()))) {
                return VtValue(SdfAssetPath());
            }

            return VtValue(SdfAssetPath(
                file.findPlug(MayaAttrs::file::fileTextureName, true)
                    .asString()
                    .asChar()));
        } else if (paramName == HdLightTokens->enableColorTemperature) {
            return VtValue(false);
        }
        return {};
    }
};

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<
        HdMayaAiSkyDomeLightAdapter, TfType::Bases<HdMayaLightAdapter> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, domeLight) {
    HdMayaAdapterRegistry::RegisterLightAdapter(
        TfToken("aiSkyDomeLight"),
        [](HdMayaDelegateCtx* delegate,
           const MDagPath& dag) -> HdMayaLightAdapterPtr {
            return HdMayaLightAdapterPtr(
                new HdMayaAiSkyDomeLightAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
