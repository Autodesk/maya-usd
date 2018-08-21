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
#include "lightAdapter.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/type.h>
#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hdx/simpleLightTask.h>

#include <maya/MColor.h>
#include <maya/MFnLight.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>

#include <maya/MNodeMessage.h>

#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/constantShadowMatrix.h>
#include <hdmaya/adapters/mayaAttrs.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<HdMayaLightAdapter, TfType::Bases<HdMayaDagAdapter> >();
}

namespace {

void _changeTransform(
    MNodeMessage::AttributeMessage /*msg*/, MPlug& /*plug*/,
    MPlug& /*otherPlug*/, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    // We need both dirty params and dirty transform to get this working?
    adapter->MarkDirty(
        HdLight::DirtyTransform | HdLight::DirtyParams |
        HdLight::DirtyShadowParams);
}

void _dirtyParams(MObject& /*node*/, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    adapter->MarkDirty(HdLight::DirtyParams | HdLight::DirtyShadowParams);
}

} // namespace

HdMayaLightAdapter::HdMayaLightAdapter(
    HdMayaDelegateCtx* delegate, const MDagPath& dag)
    : HdMayaDagAdapter(delegate->GetPrimPath(dag), delegate, dag) {}

bool HdMayaLightAdapter::IsSupported() {
    return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(LightType());
}

void HdMayaLightAdapter::Populate() {
    GetDelegate()->InsertSprim(LightType(), GetID(), HdLight::AllDirty);
}

void HdMayaLightAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    if (dirtyBits != 0) {
        GetDelegate()->GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
    }
}

void HdMayaLightAdapter::RemovePrim() {
    GetDelegate()->RemoveSprim(LightType(), GetID());
}

bool HdMayaLightAdapter::HasType(const TfToken& typeId) {
    return typeId == LightType();
}

VtValue HdMayaLightAdapter::Get(const TfToken& key) {
    TF_DEBUG(HDMAYA_ADAPTER_GET)
        .Msg(
            "Called HdMayaLightAdapter::Get(%s) - %s\n", key.GetText(),
            GetDagPath().partialPathName().asChar());

    if (key == HdLightTokens->params) {
        MFnLight mayaLight(GetDagPath());
        GlfSimpleLight light;
        const auto color = mayaLight.color();
        const auto intensity = mayaLight.intensity();
        MPoint pt(0.0, 0.0, 0.0, 1.0);
        const auto inclusiveMatrix = GetDagPath().inclusiveMatrix();
        const auto position = pt * inclusiveMatrix;
        // This will return zero / false if the plug is nonexistent.
        const auto decayRate =
            mayaLight
                .findPlug(MayaAttrs::nonAmbientLightShapeNode::decayRate, true)
                .asShort();
        const auto emitDiffuse =
            mayaLight
                .findPlug(
                    MayaAttrs::nonAmbientLightShapeNode::emitDiffuse, true)
                .asBool();
        const auto emitSpecular =
            mayaLight
                .findPlug(
                    MayaAttrs::nonAmbientLightShapeNode::emitSpecular, true)
                .asBool();
        MVector pv(0.0, 0.0, -1.0);
        const auto lightDirection = (pv * inclusiveMatrix).normal();
        light.SetHasShadow(false);
        const GfVec4f zeroColor(0.0f, 0.0f, 0.0f, 1.0f);
        const GfVec4f lightColor(
            color.r * intensity, color.g * intensity, color.b * intensity,
            1.0f);
        light.SetDiffuse(emitDiffuse ? lightColor : zeroColor);
        light.SetAmbient(zeroColor);
        light.SetSpecular(emitSpecular ? lightColor : zeroColor);
        light.SetShadowResolution(1024);
        light.SetID(GetID());
        light.SetPosition(
            GfVec4f(position.x, position.y, position.z, position.w));
        light.SetSpotDirection(
            GfVec3f(lightDirection.x, lightDirection.y, lightDirection.z));
        if (decayRate == 0) {
            light.SetAttenuation(GfVec3f(1.0f, 0.0f, 0.0f));
        } else if (decayRate == 1) {
            light.SetAttenuation(GfVec3f(0.0f, 1.0f, 0.0f));
        } else if (decayRate == 2) {
            light.SetAttenuation(GfVec3f(0.0f, 0.0f, 1.0f));
        }
        light.SetTransform(
            getGfMatrixFromMaya(GetDagPath().inclusiveMatrixInverse()));
        _CalculateLightParams(light);
        return VtValue(light);
    } else if (key == HdTokens->transform) {
        return VtValue(HdMayaDagAdapter::GetTransform());
    } else if (key == HdLightTokens->shadowCollection) {
        HdRprimCollection coll(HdTokens->geometry, HdTokens->refined);
        coll.SetRenderTags({HdTokens->geometry});
        return VtValue(coll);
    } else if (key == HdLightTokens->shadowParams) {
        HdxShadowParams shadowParams;
        shadowParams.enabled = false;
        return VtValue(shadowParams);
    }
    return {};
}

VtValue HdMayaLightAdapter::GetLightParamValue(const TfToken& paramName) {
    TF_DEBUG(HDMAYA_ADAPTER_GET_LIGHT_PARAM_VALUE)
        .Msg(
            "Called HdMayaLightAdapter::GetLightParamValue(%s) - %s\n",
            paramName.GetText(), GetDagPath().partialPathName().asChar());

    MFnLight light(GetDagPath());
    if (paramName == HdTokens->color) {
        const auto color = light.color();
        return VtValue(GfVec3f(color.r, color.g, color.b));
    } else if (paramName == HdLightTokens->intensity) {
        return VtValue(light.intensity());
    } else if (paramName == HdLightTokens->exposure) {
        return VtValue(0.0f);
    } else if (paramName == HdLightTokens->normalize) {
        return VtValue(true);
    } else if (paramName == HdLightTokens->enableColorTemperature) {
        return VtValue(false);
    }
    return {};
}

void HdMayaLightAdapter::CreateCallbacks() {
    MStatus status;
    auto dag = GetDagPath();
    auto obj = dag.node();
    auto id =
        MNodeMessage::addNodeDirtyCallback(obj, _dirtyParams, this, &status);
    dag.pop();
    if (status) { AddCallback(id); }
    for (; dag.length() > 0; dag.pop()) {
        // The adapter itself will free the callbacks, so we don't have to worry
        // about passing raw pointers to the callbacks. Hopefully.
        obj = dag.node();
        if (obj != MObject::kNullObj) {
            id = MNodeMessage::addAttributeChangedCallback(
                obj, _changeTransform, this, &status);
            if (status) { AddCallback(id); }
        }
    }
    HdMayaAdapter::CreateCallbacks();
}

void HdMayaLightAdapter::_CalculateLightParams(GlfSimpleLight& /*light*/) {}

void HdMayaLightAdapter::_CalculateShadowParams(
    MFnLight& light, GfFrustum& frustum, HdxShadowParams& params) {
    TF_DEBUG(HDMAYA_ADAPTER_LIGHT_SHADOWS)
        .Msg(
            "Called HdMayaLightAdapter::_CalculateShadowParams - %s\n",
            GetDagPath().partialPathName().asChar());

    auto dmapResolutionPlug =
        light.findPlug(MayaAttrs::nonExtendedLightShapeNode::dmapResolution);
    auto dmapBiasPlug =
        light.findPlug(MayaAttrs::nonExtendedLightShapeNode::dmapBias);
    auto dmapFilterSizePlug =
        light.findPlug(MayaAttrs::nonExtendedLightShapeNode::dmapFilterSize);

    const auto decayRate =
        light.findPlug(MayaAttrs::nonExtendedLightShapeNode::decayRate, true)
            .asShort();
    if (decayRate > 0) {
        const auto color = light.color();
        const auto intensity = light.intensity();
        const auto maxIntensity = static_cast<double>(std::max(
            color.r * intensity,
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
                    params.enabled = false;
                } else {
                    frustum.SetNearFar(
                        GfRange1d(nearFar.GetMin(), maxDistance));
                }
            }
        }
    }

    params.enabled = true;
    params.resolution =
        dmapResolutionPlug.isNull()
            ? GetDelegate()->GetParams().maximumShadowMapResolution
            : std::min(
                  GetDelegate()->GetParams().maximumShadowMapResolution,
                  dmapResolutionPlug.asInt());
    params.shadowMatrix =
        boost::static_pointer_cast<HdxShadowMatrixComputation>(
            boost::make_shared<ConstantShadowMatrix>(
                frustum.ComputeProjectionMatrix()));
    params.bias = dmapBiasPlug.isNull() ? -0.001 : -dmapBiasPlug.asFloat();
    params.blur = dmapFilterSizePlug.isNull()
                      ? 0.0
                      : (static_cast<double>(dmapFilterSizePlug.asInt())) /
                            static_cast<double>(params.resolution);

    if (TfDebug::IsEnabled(HDMAYA_ADAPTER_LIGHT_SHADOWS)) {
        std::cout << "Resulting HdxShadowParams:" << std::endl;
        std::cout << params << std::endl;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
