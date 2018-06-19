#include "delegateBase.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMayaDelegateBase::HdMayaDelegateBase(
    HdRenderIndex* renderIndex,
    const SdfPath& delegateID) : HdSceneDelegate(renderIndex, delegateID) {

}

PXR_NAMESPACE_CLOSE_SCOPE
