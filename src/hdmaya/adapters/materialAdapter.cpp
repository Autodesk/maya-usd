#include <hdmaya/adapters/materialAdapter.h>

#include <pxr/imaging/hd/material.h>

PXR_NAMESPACE_OPEN_SCOPE


HdMayaMaterialAdapter::HdMayaMaterialAdapter(const MObject& node, HdMayaDelegateCtx* delegate)
    : HdMayaAdapter(node, delegate->GetMaterialPath(node), delegate) {

}

bool
HdMayaMaterialAdapter::IsSupported() {
    return GetDelegate()->GetRenderIndex().IsSprimTypeSupported(HdPrimTypeTokens->material);
}

bool
HdMayaMaterialAdapter::HasType(const TfToken& typeId) {
    return typeId == HdPrimTypeTokens->material;
}

void
HdMayaMaterialAdapter::MarkDirty(HdDirtyBits dirtyBits) {
    GetDelegate()->GetChangeTracker().MarkSprimDirty(GetID(), dirtyBits);
}

void
HdMayaMaterialAdapter::RemovePrim() {
    GetDelegate()->RemoveSprim(HdPrimTypeTokens->material, GetID());
}


void
HdMayaMaterialAdapter::Populate() {
    GetDelegate()->InsertSprim(HdPrimTypeTokens->material, GetID(), HdMaterial::AllDirty);
}

PXR_NAMESPACE_CLOSE_SCOPE
