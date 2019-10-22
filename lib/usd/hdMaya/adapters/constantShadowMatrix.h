//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
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
