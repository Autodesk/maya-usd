//
// Copyright 2017 Animal Logic
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
#include "AL/usdmaya/utils/Utils.h"

#include "AL/maya/utils/Utils.h"

#include <mayaUsdUtils/DebugCodes.h>

#include <maya/MDagPath.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MVector.h>

#include <atomic>

namespace {
std::atomic<std::int32_t> _blockingCount;
}

namespace AL {
namespace usdmaya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
BlockNotifications::BlockNotifications() { _blockingCount++; }

//----------------------------------------------------------------------------------------------------------------------
BlockNotifications::~BlockNotifications() { _blockingCount--; }

//----------------------------------------------------------------------------------------------------------------------
bool BlockNotifications::isBlockingNotifications() { return _blockingCount.load() > 0; }

//----------------------------------------------------------------------------------------------------------------------
void matrixToSRT(const GfMatrix4d& value, double S[3], MEulerRotation& R, double T[3])
{
    double matrix[4][4];
    value.Get(matrix);
    T[0] = matrix[3][0];
    T[1] = matrix[3][1];
    T[2] = matrix[3][2];
    MVector xAxis(matrix[0][0], matrix[0][1], matrix[0][2]);
    MVector yAxis(matrix[1][0], matrix[1][1], matrix[1][2]);
    MVector zAxis(matrix[2][0], matrix[2][1], matrix[2][2]);
    double  scaleX = xAxis.length();
    double  scaleY = yAxis.length();
    double  scaleZ = zAxis.length();
    bool    isNegated = ((xAxis ^ yAxis) * zAxis) < 0.0;
    if (isNegated) {
        scaleZ = -scaleZ;
    }
    xAxis /= scaleX;
    yAxis /= scaleY;
    zAxis /= scaleZ;
    S[0] = scaleX;
    S[1] = scaleY;
    S[2] = scaleZ;
    matrix[0][0] = xAxis.x;
    matrix[0][1] = xAxis.y;
    matrix[0][2] = xAxis.z;
    matrix[0][3] = 0;
    matrix[1][0] = yAxis.x;
    matrix[1][1] = yAxis.y;
    matrix[1][2] = yAxis.z;
    matrix[1][3] = 0;
    matrix[2][0] = zAxis.x;
    matrix[2][1] = zAxis.y;
    matrix[2][2] = zAxis.z;
    matrix[2][3] = 0;
    matrix[3][0] = 0;
    matrix[3][1] = 0;
    matrix[3][2] = 0;
    matrix[3][3] = 1.0;
    R = MMatrix(matrix);
}

//----------------------------------------------------------------------------------------------------------------------
MString mapUsdPrimToMayaNode(
    const UsdPrim&        usdPrim,
    const MObject&        mayaObject,
    const MDagPath* const proxyShapeNode)
{
    if (!usdPrim.IsValid()) {
        MGlobal::displayError("mapUsdPrimToMayaNode: Invalid prim!");
        return MString();
    }
    TfToken mayaPathAttributeName("MayaPath");

    UsdStageWeakPtr stage = usdPrim.GetStage();

    MFnDagNode mayaNode(mayaObject);
    MDagPath   mayaDagPath;
    mayaNode.getPath(mayaDagPath);
    std::string mayaElementPath = AL::maya::utils::convert(mayaDagPath.fullPathName());

    if (mayaDagPath.length() == 0 && proxyShapeNode) {
        // Prepend the mayaPathPrefix
        mayaElementPath = proxyShapeNode->fullPathName().asChar() + usdPrim.GetPath().GetString();
        std::replace(mayaElementPath.begin(), mayaElementPath.end(), '/', '|');
    }

    TF_DEBUG(MAYAUSDUTILS_INFO)
        .Msg(
            "Mapped the path for prim=%s to mayaObject=%s\n",
            usdPrim.GetName().GetText(),
            mayaElementPath.c_str());

    return AL::maya::utils::convert(mayaElementPath);
}

//----------------------------------------------------------------------------------------------------------------------
void convertDoubleVec4ArrayToFloatVec3Array(
    const double* const input,
    float* const        output,
    size_t              count)
{
    for (size_t i = 0; i < count; ++i) {
        output[i * 3] = float(input[i * 4]);
        output[i * 3 + 1] = float(input[i * 4 + 1]);
        output[i * 3 + 2] = float(input[i * 4 + 2]);
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
