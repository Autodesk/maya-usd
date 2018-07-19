#ifndef __HDMAYA_PARAMS_H__
#define __HDMAYA_PARAMS_H__

#include <pxr/pxr.h>

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

struct HdMayaParams {
    size_t textureMemoryPerTexture = 4 * 1024 * 1024;
    int maximumShadowMapResolution = 2048;
    bool displaySmoothMeshes = true;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_PARAMS_H__
