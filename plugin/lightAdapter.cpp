#include "lightAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMayaLightAdapter::HdMayaLightAdapter(
    const SdfPath& id, HdSceneDelegate* delegate, const MDagPath& dagPath)
    : HdMayaDagAdapter(id, delegate, dagPath) {
}

void
HdMayaLightAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetRenderIndex().GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
}

PXR_NAMESPACE_CLOSE_SCOPE
