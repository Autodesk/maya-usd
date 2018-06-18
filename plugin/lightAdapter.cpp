#include "lightAdapter.h"

#include <pxr/imaging/hd/light.h>

#include <maya/MColor.h>
#include <maya/MFnLight.h>

PXR_NAMESPACE_OPEN_SCOPE

HdMayaLightAdapter::HdMayaLightAdapter(
    const SdfPath& id, HdSceneDelegate* delegate, const MDagPath& dagPath)
    : HdMayaDagAdapter(id, delegate, dagPath) {
}

void
HdMayaLightAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetRenderIndex().GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
}

VtValue
HdMayaLightAdapter::Get(const TfToken& key) {
    if (key == HdTokens->transform) {
        return VtValue(GetTransform());
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

PXR_NAMESPACE_CLOSE_SCOPE
