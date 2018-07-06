#ifndef __HDMAYA_PARAMS_H__
#define __HDMAYA_PARAMS_H__

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

struct HdMayaParams {
    int maximumShadowMapResolution = 2048;
    bool displaySmoothMeshes = true;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_PARAMS_H__
