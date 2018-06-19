#include "delegateCtx.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMayaDelegateCtx::HdMayaDelegateCtx(
    HdRenderIndex* renderIndex,
    const SdfPath& delegateID) : HdSceneDelegate(renderIndex, delegateID) {
    _rprimCollection.SetName(TfToken("visible"));
    _rprimCollection.SetRootPath(GetDelegateID());
    _rprimCollection.SetRenderTags({HdTokens->geometry});
    GetChangeTracker().AddCollection(TfToken("visible"));
}

void
HdMayaDelegateCtx::InsertRprim(const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits) {
    GetRenderIndex().InsertRprim(typeId, this, id);
    GetChangeTracker().RprimInserted(id, initialBits);
}

void
HdMayaDelegateCtx::InsertSprim(const TfToken& typeId, const SdfPath& id, HdDirtyBits initialBits) {
    GetRenderIndex().InsertSprim(typeId, this, id);
    GetChangeTracker().SprimInserted(id, initialBits);
}

PXR_NAMESPACE_CLOSE_SCOPE
