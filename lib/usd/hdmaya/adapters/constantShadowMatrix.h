//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef __HDMAYA_SHADOW_MATRIX_H__
#define __HDMAYA_SHADOW_MATRIX_H__

#include <pxr/imaging/hdx/shadowMatrixComputation.h>
#include <pxr/pxr.h>

#include <pxr/base/gf/matrix4d.h>

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaConstantShadowMatrix : public HdxShadowMatrixComputation {
public:
    explicit HdMayaConstantShadowMatrix(const GfMatrix4d& mat)
        : _shadowMatrix(mat) {}

    inline GfMatrix4d Compute(
        const GfVec4f& viewport,
        CameraUtilConformWindowPolicy policy) override {
        return _shadowMatrix;
    }

private:
    GfMatrix4d _shadowMatrix;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_SHADOW_MATRIX_H__
