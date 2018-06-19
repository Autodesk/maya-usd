#include "delegateCtx.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMayaDelegateCtx::HdMayaDelegateCtx(
    HdRenderIndex* renderIndex,
    const SdfPath& delegateID) : HdSceneDelegate(renderIndex, delegateID) {

}

PXR_NAMESPACE_CLOSE_SCOPE
