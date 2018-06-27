#include "lightAdapter.h"

#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hdx/simpleLightTask.h>

#include <maya/MColor.h>
#include <maya/MFnLight.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>

#include <maya/MNodeMessage.h>

#include "constantShadowMatrix.h"

#include "../viewportRenderer.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    void _dirtyTransform(MObject& /*node*/, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
        // We need both dirty params and dirty transform to get this working?
        adapter->MarkDirty(HdLight::DirtyTransform | HdLight::DirtyParams | HdLight::DirtyShadowParams);
    }

    void _dirtyParams(MObject& /*node*/, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
        adapter->MarkDirty(HdLight::DirtyParams | HdLight::DirtyShadowParams);
    }
}

HdMayaLightAdapter::HdMayaLightAdapter(
    HdMayaDelegateCtx* delegate, const MDagPath& dag)
    : HdMayaDagAdapter(delegate->GetPrimPath(dag), delegate, dag) {
}

void
HdMayaLightAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
    if (dirtyBits & HdLight::DirtyTransform) {
        CalculateTransform();
    }
}

void
HdMayaLightAdapter::RemovePrim() {
    GetDelegate()->RemoveSprim(HdPrimTypeTokens->simpleLight, GetID());
}

VtValue
HdMayaLightAdapter::Get(const TfToken& key) {
    if (key == HdLightTokens->params) {
        MFnLight mayaLight(GetDagPath().node());
        GlfSimpleLight light;
        const auto color = mayaLight.color();
        const auto intensity = mayaLight.intensity();
        MPoint pt(0.0, 0.0, 0.0, 1.0);
        const auto inclusiveMatrix = GetDagPath().inclusiveMatrix();
        const auto position = pt * inclusiveMatrix;
        // This will return zero / false if the plug is nonexistent.
        const auto decayRate = mayaLight.findPlug("decayRate", true).asShort();
        const auto emitDiffuse = mayaLight.findPlug("emitDiffuse", true).asBool();
        const auto emitSpecular = mayaLight.findPlug("emitSpecular", true).asBool();
        MVector pv(0.0, 0.0, -1.0);
        const auto lightDirection = (pv * inclusiveMatrix).normal();
        light.SetHasShadow(false);
        const GfVec4f zeroColor(0.0f, 0.0f, 0.0f, 1.0f);
        const GfVec4f lightColor(color.r * intensity, color.g * intensity, color.b * intensity, 1.0f);
        light.SetDiffuse(emitDiffuse ? lightColor : zeroColor);
        light.SetAmbient(zeroColor);
        light.SetSpecular(emitSpecular ? lightColor : zeroColor);
        light.SetShadowResolution(1024);
        light.SetID(GetID());
        light.SetPosition(GfVec4f(position.x, position.y, position.z, position.w));
        light.SetSpotDirection(GfVec3f(lightDirection.x, lightDirection.y, lightDirection.z));
        if (decayRate == 0) {
            light.SetAttenuation(GfVec3f(1.0f, 0.0f, 0.0f));
        } else if (decayRate == 1) {
            light.SetAttenuation(GfVec3f(0.0f, 1.0f, 0.0f));
        } else if (decayRate == 2) {
            light.SetAttenuation(GfVec3f(0.0f, 0.0f, 1.0f));
        }
        CalculateLightParams(light);
        return VtValue(light);
    } else if (key == HdTokens->transform) {
        return VtValue(HdMayaDagAdapter::GetTransform());
    } else if (key == HdLightTokens->shadowCollection) {
        return VtValue(HdRprimCollection(HdTokens->geometry, HdTokens->hull));
        // return VtValue(GetDelegate()->GetRprimCollection());
    } else if (key == HdLightTokens->shadowParams) {
        HdxShadowParams shadowParams;
        shadowParams.enabled = false;
        return VtValue(shadowParams);
    }
    return {};
}

VtValue
HdMayaLightAdapter::GetLightParamValue(const TfToken& paramName) {
    MFnLight light(GetDagPath().node());
    if (paramName == HdTokens->color) {
        const auto color = light.color();
        return VtValue(GfVec3f(color.r, color.g, color.b));
    } else if (paramName == HdLightTokens->intensity) {
        return VtValue(light.intensity());
    } else if (paramName == HdLightTokens->exposure) {
        return VtValue(0.0f);
    }
    return {};
}

void
HdMayaLightAdapter::CreateCallbacks() {
    MStatus status;
    auto dag = GetDagPath();
    auto obj = dag.node();
    auto id = MNodeMessage::addNodeDirtyCallback(obj, _dirtyParams, this, &status);
    dag.pop();
    if (status) { AddCallback(id); }
    for (; dag.length() > 0; dag.pop()) {
        // The adapter itself will free the callbacks, so we don't have to worry about
        // passing raw pointers to the callbacks. Hopefully.
        obj = dag.node();
        if (obj != MObject::kNullObj) {
            id = MNodeMessage::addNodeDirtyCallback(obj, _dirtyTransform, this, &status);
            if (status) { AddCallback(id); }
        }
    }
}

void HdMayaLightAdapter::Populate() {
    GetDelegate()->InsertSprim(HdPrimTypeTokens->simpleLight, GetID(), HdLight::AllDirty);
}

void
HdMayaLightAdapter::CalculateLightParams(GlfSimpleLight& /*light*/) {

}

void
HdMayaLightAdapter::CalculateShadowParams(MFnLight& light, GfFrustum& frustum, HdxShadowParams& params) {
    auto dmapResolutionPlug = light.findPlug("dmapResolution");
    auto dmapBiasPlug = light.findPlug("dmapBias");
    auto dmapFilterSizePlug = light.findPlug("dmapFilterSize");

    const auto decayRate = light.findPlug("decayRate", true).asShort();
    if (decayRate > 0) {
        const auto color = light.color();
        const auto intensity = light.intensity();
        const auto maxIntensity = static_cast<double>(std::max(color.r * intensity,
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
                    frustum.SetNearFar(GfRange1d(nearFar.GetMin(), maxDistance));
                }
            }
        }
    }

    params.enabled = true;
    params.resolution = dmapResolutionPlug.isNull() ?
                              HdMayaViewportRenderer::GetFallbackShadowMapResolution() : dmapResolutionPlug.asInt();
    params.shadowMatrix = boost::static_pointer_cast<HdxShadowMatrixComputation>(
        boost::make_shared<ConstantShadowMatrix>(frustum.ComputeViewMatrix() * frustum.ComputeProjectionMatrix()));
    params.bias = dmapBiasPlug.isNull() ?
                        -0.001 : -dmapBiasPlug.asFloat();
    params.blur = dmapFilterSizePlug.isNull() ?
                        0.0 : (static_cast<double>(dmapFilterSizePlug.asInt())) / static_cast<double>(params.resolution);
}

PXR_NAMESPACE_CLOSE_SCOPE
