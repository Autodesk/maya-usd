#include "delegate.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMayaDelegate::HdMayaDelegate(
    HdRenderIndex* parentIndex,
    const SdfPath& delegateID) :
    HdSceneDelegate(parentIndex, delegateID) {
}

PXR_NAMESPACE_CLOSE_SCOPE
