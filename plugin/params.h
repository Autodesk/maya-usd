#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

struct MayaRenderParams{
    bool enableLighting = true;

    size_t hash() const {
        auto ret = size_t(enableLighting);
        return ret;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __PARAMS_H__
