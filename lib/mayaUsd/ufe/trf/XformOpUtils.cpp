//
// Copyright 2024 Autodesk
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

#include "XformOpUtils.h"

#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>

#include <cstring>

namespace MAYAUSD_NS_DEF {
namespace ufe {

void extractTRS(const Ufe::Matrix4d& m, Ufe::Vector3d* t, Ufe::Vector3d* r, Ufe::Vector3d* s)
{
    // Decompose new matrix to extract TRS.  Neither GfMatrix4d::Factor
    // nor GfTransform decomposition provide results that match Maya,
    // so use MTransformationMatrix.

    // We use this function override the default one from UsdUfe(which uses USD API)
    // to extract the matrix.
    //
    //    Example: with input matrix
    //      [ -0.86, 0, -0.5,  0 ]
    //      [  0,    1,  0,    0 ]
    //      [  0.5,  0, -0.86, 0 ]
    //      [  0,   20,  0,    1 ]
    //
    //      USD returns:  [ 180, 30, 180 ]
    //      Maya returns: [ 0,  150,   0 ]
    //
    // These two rotations are visually identical, but not the same values.

    MMatrix mm;
    std::memcpy(mm[0], &m.matrix[0][0], sizeof(double) * 16);
    MTransformationMatrix xformM(mm);
    if (nullptr != t) {
        auto tv = xformM.getTranslation(MSpace::kTransform);
        *t = Ufe::Vector3d(tv[0], tv[1], tv[2]);
    }
    if (nullptr != r) {
        double rv[3];
        // We created the MTransformationMatrix with the default XYZ rotation
        // order, so rotOrder parameter is unused.
        MTransformationMatrix::RotationOrder rotOrder;
        xformM.getRotation(rv, rotOrder);
        constexpr double radToDeg = 57.295779506;
        *r = Ufe::Vector3d(rv[0] * radToDeg, rv[1] * radToDeg, rv[2] * radToDeg);
    }
    if (nullptr != s) {
        double sv[3];
        xformM.getScale(sv, MSpace::kTransform);
        *s = Ufe::Vector3d(sv[0], sv[1], sv[2]);
    }
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
