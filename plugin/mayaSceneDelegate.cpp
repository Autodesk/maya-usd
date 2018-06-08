#include "mayaSceneDelegate.h"

PXR_NAMESPACE_USING_DIRECTIVE

MayaSceneDelegate::MayaSceneDelegate(
    HdRenderIndex* parentIndex,
    const SdfPath& delegateID) :
    HdSceneDelegate(parentIndex, delegateID) {

}

MayaSceneDelegate::~MayaSceneDelegate() {

}
