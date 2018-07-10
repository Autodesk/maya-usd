#include <hdmaya/delegates/delegate.h>

#include <pxr/base/tf/type.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdMayaDelegate>();
}

void
HdMayaDelegate::SetParams(const HdMayaParams& params) {
    _params = params;
}

PXR_NAMESPACE_CLOSE_SCOPE
