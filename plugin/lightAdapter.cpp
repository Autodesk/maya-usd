#include "lightAdapter.h"

#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/hdx/simpleLightTask.h>

#include <maya/MColor.h>
#include <maya/MFnLight.h>

#include <maya/MNodeMessage.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    void _dirtyTransform(MObject& /*node*/, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
        // We need both dirty params and dirty transform to get this working?
        adapter->MarkDirty(HdLight::DirtyTransform | HdLight::DirtyParams);
    }

    void _dirtyParams(MObject& /*node*/, void* clientData) {
        auto* adapter = reinterpret_cast<HdMayaDagAdapter*>(clientData);
        adapter->MarkDirty(HdLight::DirtyParams);
    }
}

HdMayaLightAdapter::HdMayaLightAdapter(
    HdMayaDelegateCtx* delegate, const MDagPath& dag)
    : HdMayaDagAdapter(delegate->GetSPrimPath(dag), delegate, dag) {
}

void
HdMayaLightAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
}

VtValue
HdMayaLightAdapter::Get(const TfToken& key) {
    if (key == HdTokens->transform) {
        // std::cerr << "[HdMayaLightAdapter::Get] Transform on " << GetID() << std::endl;
        return VtValue(HdMayaDagAdapter::GetTransform());
    } else if (key == HdLightTokens->shadowParams) {
        // std::cerr << "[HdMayaLightAdapter::Get] Shadow params on " << GetID() << std::endl;
        // MFnLight light(GetDagPath().node());
        HdxShadowParams shadowParams;
        shadowParams.enabled = true;
        shadowParams.resolution = 1024;
        return VtValue(shadowParams);
    } else if (key == HdLightTokens->shadowCollection) {
        // FIXME
        return VtValue(GetDelegate()->GetRprimCollection());
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

PXR_NAMESPACE_CLOSE_SCOPE
