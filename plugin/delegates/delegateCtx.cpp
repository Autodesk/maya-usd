#include "delegateCtx.h"

#include <usdMaya/util.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

SdfPath
_GetPrimPath(const SdfPath& base, const MDagPath& dg) {
    const auto mayaPath = PxrUsdMayaUtil::MDagPathToUsdPath(dg, false
#ifdef LUMA_USD_BUILD
            , false
#endif
        );
    if (mayaPath.IsEmpty()) { return {}; }
    const auto* chr = mayaPath.GetText();
    if (chr == nullptr) { return {}; };
    std::string s(chr + 1);
    if (s.empty()) { return {}; }
    return base.AppendPath(SdfPath(s));
}

}

HdMayaDelegateCtx::HdMayaDelegateCtx(
    HdRenderIndex* renderIndex,
    const SdfPath& delegateID)
    : HdMayaDelegate(renderIndex, delegateID),
     _rprimPath(delegateID.AppendPath(SdfPath(std::string("rprims")))),
     _sprimPath(delegateID.AppendPath(SdfPath(std::string("sprims")))) {
    _rprimCollection.SetName(TfToken("visible"));
    _rprimCollection.SetRootPath(_rprimPath);
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

SdfPath
HdMayaDelegateCtx::GetRPrimPath(const MDagPath& dg) {
    return _GetPrimPath(_rprimPath, dg);
}

SdfPath
HdMayaDelegateCtx::GetSPrimPath(const MDagPath& dg) {
    return _GetPrimPath(_sprimPath, dg);
}

PXR_NAMESPACE_CLOSE_SCOPE
