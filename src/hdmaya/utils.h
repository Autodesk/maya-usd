#ifndef __HDMAYA_UTILS_H__
#define __HDMAYA_UTILS_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/matrix4d.h>

#include <maya/MMatrix.h>

PXR_NAMESPACE_OPEN_SCOPE

inline
GfMatrix4d
getGfMatrixFromMaya(const MMatrix& mayaMat) {
    GfMatrix4d mat;
    memcpy(mat.GetArray(), mayaMat[0], sizeof(double) * 16);
    return mat;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_UTILS_H_
