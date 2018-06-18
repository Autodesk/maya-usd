#include "pointLightAdapter.h"

#include "adapterRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterDagAdapter("pointLight",
    [](const SdfPath& id, HdSceneDelegate* delegate, const MDagPath& dag) -> std::shared_ptr<HdMayaDagAdapter> {
        return std::static_pointer_cast<HdMayaDagAdapter>(std::make_shared<HdMayaPointLightAdapter>(id, delegate, dag));
    });
}


HdMayaPointLightAdapter::HdMayaPointLightAdapter(
    const SdfPath& id, HdSceneDelegate* delegate, const MDagPath& dagPath)
    : HdMayaLightAdapter(id, delegate, dagPath) {
}

void
HdMayaPointLightAdapter::Populate(
    HdRenderIndex& renderIndex,
    HdSceneDelegate* delegate,
    const SdfPath& id) {
    renderIndex.InsertSprim(HdPrimTypeTokens->sphereLight, delegate, id);
    // renderIndex.GetChangeTracker().SprimInserted(id, HdChangeTracker::DirtyTransform);
}

PXR_NAMESPACE_CLOSE_SCOPE
