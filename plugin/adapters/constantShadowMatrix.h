#ifndef __HDMAYA_SHADOW_MATRIX_H__
#define __HDMAYA_SHADOW_MATRIX_H__

#include <pxr/pxr.h>
#include <pxr/imaging/hdx/shadowMatrixComputation.h>

#include <pxr/base/gf/matrix4d.h>

// FFFFFFFFFFFFFFFffffffffffffffffff
#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class ConstantShadowMatrix : public HdxShadowMatrixComputation {
public:
    inline
    ConstantShadowMatrix(const GfMatrix4d& mat) : _shadowMatrix(mat) { }

    GfMatrix4d Compute(const GfVec4f &viewport,
                       CameraUtilConformWindowPolicy policy) override {
        return _shadowMatrix;
    }
private:
    GfMatrix4d _shadowMatrix;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_SHADOW_MATRIX_H__
