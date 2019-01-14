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
#include <maya/MPlugArray.h>
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
    MNodeMessage::AttributeMessage /*msg*/, MPlug& plug, MPlug& /*otherPlug*/,
    void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    if (plug == MayaAttrs::dagNode::visibility) {
        if (adapter->UpdateVisibility()) {
            adapter->RemovePrim();
            adapter->Populate();
            adapter->InvalidateTransform();
        }
    } else if (adapter->IsVisible()) {
        // We need both dirty params and dirty transform to get this working?
        adapter->MarkDirty(
            HdLight::DirtyTransform | HdLight::DirtyParams |
            HdLight::DirtyShadowParams);
        adapter->InvalidateTransform();
    }
}

void _dirtyParams(MObject& /*node*/, void* clientData) {
    auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
    if (adapter->IsVisible()) {
        adapter->MarkDirty(HdLight::DirtyParams | HdLight::DirtyShadowParams);
        adapter->InvalidateTransform();
    }
}

const MString defaultLightSet("defaultLightSet");

} // namespace

HdMayaLightAdapter::HdMayaLightAdapter(
    HdMayaDelegateCtx* delegate, const MDagPath& dag)
    : HdMayaDagAdapter(delegate->GetPrimPath(dag), delegate, dag) {
    // This should be avoided, not a good idea to call virtual functions
    // directly or indirectly in a constructor.
    UpdateVisibility();
    _shadowProjectionMatrix.SetIdentity();
}

bool HdMayaLightAdapter::IsSupported() {
    return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(LightType());
}

void HdMayaLightAdapter::Populate() {
    if (_isPopulated) { return; }
    if (IsVisible()) {
        GetDelegate()->InsertSprim(LightType(), GetID(), HdLight::AllDirty);
        _isPopulated = true;
    }
}

void HdMayaLightAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    if (dirtyBits != 0) {
        GetDelegate()->GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
    }
}

void HdMayaLightAdapter::RemovePrim() {
    if (!_isPopulated) { return; }
    GetDelegate()->RemoveSprim(LightType(), GetID());
    _isPopulated = false;
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
            GetGfMatrixFromMaya(GetDagPath().inclusiveMatrixInverse()));
        _CalculateLightParams(light);
        return VtValue(light);
    } else if (key == HdTokens->transform) {
        return VtValue(HdMayaDagAdapter::GetTransform());
    } else if (key == HdLightTokens->shadowCollection) {
        HdRprimCollection coll(
            HdTokens->geometry, HdReprSelector(HdReprTokens->refined));
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
    if (status) { AddCallback(id); }
    dag.pop();
    for (; dag.length() > 0; dag.pop()) {
        // The adapter itself will free the callbacks, so we don't have to worry
        // about passing raw pointers to the callbacks. Hopefully.
        obj = dag.node();
        if (obj != MObject::kNullObj) {
            id = MNodeMessage::addAttributeChangedCallback(
                obj, _changeTransform, this, &status);
            if (status) { AddCallback(id); }
            _AddHierarchyChangedCallback(dag);
        }
    }
    HdMayaAdapter::CreateCallbacks();
}

void HdMayaLightAdapter::SetShadowProjectionMatrix(const GfMatrix4d& matrix) {
    if (!GfIsClose(_shadowProjectionMatrix, matrix, 0.0001)) {
        MarkDirty(HdLight::DirtyShadowParams);
        _shadowProjectionMatrix = matrix;
    }
}

void HdMayaLightAdapter::_CalculateShadowParams(
    MFnLight& light, HdxShadowParams& params) {
    TF_DEBUG(HDMAYA_ADAPTER_LIGHT_SHADOWS)
        .Msg(
            "Called HdMayaLightAdapter::_CalculateShadowParams - %s\n",
            GetDagPath().partialPathName().asChar());

    const auto dmapResolutionPlug = light.findPlug(
        MayaAttrs::nonExtendedLightShapeNode::dmapResolution, true);
    const auto dmapBiasPlug =
        light.findPlug(MayaAttrs::nonExtendedLightShapeNode::dmapBias, true);
    const auto dmapFilterSizePlug = light.findPlug(
        MayaAttrs::nonExtendedLightShapeNode::dmapFilterSize, true);

    params.enabled = true;
    params.resolution =
        dmapResolutionPlug.isNull()
            ? GetDelegate()->GetParams().maximumShadowMapResolution
            : std::min(
                  GetDelegate()->GetParams().maximumShadowMapResolution,
                  dmapResolutionPlug.asInt());
    params.shadowMatrix =
        boost::static_pointer_cast<HdxShadowMatrixComputation>(
            boost::make_shared<HdMayaConstantShadowMatrix>(
                GetTransform() * _shadowProjectionMatrix));
    params.bias = dmapBiasPlug.isNull() ? -0.001 : -dmapBiasPlug.asFloat();
    params.blur = dmapFilterSizePlug.isNull()
                      ? 0.0
                      : (static_cast<double>(dmapFilterSizePlug.asInt())) /
                            static_cast<double>(params.resolution);

    if (TfDebug::IsEnabled(HDMAYA_ADAPTER_LIGHT_SHADOWS)) {
        std::cout << "Resulting HdxShadowParams:\n";
        std::cout << params << "\n";
    }
}

bool HdMayaLightAdapter::_GetVisibility() const {
    if (!GetDagPath().isVisible()) {
        return false;
    }
    MStatus status;
    MFnDependencyNode node(GetDagPath().transform(), &status);
    if (ARCH_UNLIKELY(!status)) { return true; }
    auto p = node.findPlug(MayaAttrs::dagNode::instObjGroups, true);
    if (ARCH_UNLIKELY(p.isNull())) { return true; }
    const auto numElements = p.numElements();
    MPlugArray conns;
    for (auto i = decltype(numElements){0}; i < numElements; ++i) {
        auto ep = p[i]; // == elementByPhysicalIndex
        if (!ep.connectedTo(conns, false, true) || conns.length() < 1) {
            continue;
        }
        const auto numConns = conns.length();
        for (auto j = decltype(numConns){0}; j < numConns; ++j) {
            MFnDependencyNode otherNode(conns[j].node(), &status);
            if (!status) { continue; }
            if (otherNode.name() == defaultLightSet) {
                return true;
            }
        }
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
