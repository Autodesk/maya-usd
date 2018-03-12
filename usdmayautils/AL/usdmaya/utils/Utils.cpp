//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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

#include "AL/usd/utils/DebugCodes.h"
#include "AL/maya/utils/Utils.h"

#include "maya/MMatrix.h"
#include "maya/MEulerRotation.h"
#include "maya/MVector.h"
#include "maya/MObject.h"
#include "maya/MFnDagNode.h"
#include "maya/MDagPath.h"
#include "maya/MGlobal.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/editTarget.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/stage.h"

#include <algorithm>

#include "AL/usdmaya/utils/ForwardDeclares.h"

namespace AL {
namespace usdmaya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
void matrixToSRT(GfMatrix4d& value, double S[3], MEulerRotation& R, double T[3])
{
  double matrix[4][4];
  value.Get(matrix);
  T[0] = matrix[3][0];
  T[1] = matrix[3][1];
  T[2] = matrix[3][2];
  MVector xAxis(matrix[0][0], matrix[0][1], matrix[0][2]);
  MVector yAxis(matrix[1][0], matrix[1][1], matrix[1][2]);
  MVector zAxis(matrix[2][0], matrix[2][1], matrix[2][2]);
  double scaleX = xAxis.length();
  double scaleY = yAxis.length();
  double scaleZ = zAxis.length();
  S[0] = scaleX;
  S[1] = scaleY;
  S[2] = scaleZ;
  bool isNegated = ((xAxis ^ yAxis) * zAxis) < 0.0;
  xAxis /= scaleX;
  yAxis /= scaleY;
  zAxis /= scaleZ;
  if(isNegated)
  {
    zAxis = -zAxis;
    scaleZ = -scaleZ;
  }
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
MString mapUsdPrimToMayaNode(const UsdPrim& usdPrim,
                             const MObject& mayaObject,
                             const MDagPath* const proxyShapeNode)
{
  if(!usdPrim.IsValid())
  {
    MGlobal::displayError("mapUsdPrimToMayaNode: Invalid prim!");
    return MString();
  }
  TfToken mayaPathAttributeName("MayaPath");

  UsdStageWeakPtr stage = usdPrim.GetStage();

  // copy the previousTarget, and restore it later
  UsdEditTarget previousTarget = stage->GetEditTarget();
  //auto previousLayer = previousTarget.GetLayer();
  auto sessionLayer = stage->GetSessionLayer();
  stage->SetEditTarget(sessionLayer);

  MFnDagNode mayaNode(mayaObject);
  MDagPath mayaDagPath;
  mayaNode.getPath(mayaDagPath);
  std::string mayaElementPath = AL::maya::utils::convert(mayaDagPath.fullPathName());

  if(mayaDagPath.length() == 0 && proxyShapeNode)
  {
    // Prepend the mayaPathPrefix
    mayaElementPath = proxyShapeNode->fullPathName().asChar() + usdPrim.GetPath().GetString();
    std::replace(mayaElementPath.begin(), mayaElementPath.end(), '/','|');
  }

  VtValue mayaPathValue(mayaElementPath);
  usdPrim.SetCustomDataByKey(mayaPathAttributeName, mayaPathValue);

  TF_DEBUG(ALUTILS_INFO).Msg("Capturing the path for prim=%s mayaObject=%s\n", usdPrim.GetName().GetText(), mayaElementPath.c_str());

  //restore the edit target
  stage->SetEditTarget(previousTarget);

  return AL::maya::utils::convert(mayaElementPath);
}

//----------------------------------------------------------------------------------------------------------------------
} // utils
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
