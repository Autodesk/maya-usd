#include <hdmaya/delegates/delegate.h>

PXR_NAMESPACE_OPEN_SCOPE

void
HdMayaDelegate::SetParams(const HdMayaParams& params) {
    _params.maximumShadowMapResolution =
        std::max(std::min(8192, params.maximumShadowMapResolution), 32);
}

PXR_NAMESPACE_CLOSE_SCOPE
