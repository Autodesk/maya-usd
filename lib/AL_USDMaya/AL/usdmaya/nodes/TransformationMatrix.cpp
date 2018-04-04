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
#include "AL/maya/utils/CommandGuiHelper.h"
#include "AL/usdmaya/AttributeType.h"
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"

#include "maya/MFileIO.h"
#include "AL/usdmaya/utils/Utils.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
const MTypeId TransformationMatrix::kTypeId(AL_USDMAYA_TRANSFORMATION_MATRIX);

//----------------------------------------------------------------------------------------------------------------------
MPxTransformationMatrix* TransformationMatrix::creator()
{
  return new TransformationMatrix;
}

//----------------------------------------------------------------------------------------------------------------------
TransformationMatrix::TransformationMatrix()
  : MPxTransformationMatrix(),
    m_prim(),
    m_xform(),
    m_time(UsdTimeCode::Default()),
    m_scaleTweak(0, 0, 0),
    m_rotationTweak(0, 0, 0),
    m_translationTweak(0, 0, 0),
    m_shearTweak(0, 0, 0),
    m_scalePivotTweak(0, 0, 0),
    m_scalePivotTranslationTweak(0, 0, 0),
    m_rotatePivotTweak(0, 0, 0),
    m_rotatePivotTranslationTweak(0, 0, 0),
    m_rotateOrientationTweak(0, 0, 0, 1.0),
    m_scaleFromUsd(1.1, 1.1, 1.1),
    m_rotationFromUsd(5, 0, 0),
    m_translationFromUsd(0.1, 0.2, 0.3),
    m_shearFromUsd(0, 0, 0),
    m_scalePivotFromUsd(0, 0, 0),
    m_scalePivotTranslationFromUsd(0, 0, 0),
    m_rotatePivotFromUsd(0, 0, 0),
    m_rotatePivotTranslationFromUsd(0, 0, 0),
    m_rotateOrientationFromUsd(0, 0, 0, 1.0),
    m_localTranslateOffset(0, 0, 0),
    m_flags(0)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::TransformationMatrix\n");
  initialiseToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
TransformationMatrix::TransformationMatrix(const UsdPrim& prim)
  : MPxTransformationMatrix(),
    m_prim(prim),
    m_xform(prim),
    m_time(UsdTimeCode::Default()),
    m_scaleTweak(0, 0, 0),
    m_rotationTweak(0, 0, 0),
    m_translationTweak(0, 0, 0),
    m_shearTweak(0, 0, 0),
    m_scalePivotTweak(0, 0, 0),
    m_scalePivotTranslationTweak(0, 0, 0),
    m_rotatePivotTweak(0, 0, 0),
    m_rotatePivotTranslationTweak(0, 0, 0),
    m_rotateOrientationTweak(0, 0, 0, 1.0),
    m_scaleFromUsd(1.0, 1.0, 1.0),
    m_rotationFromUsd(0, 0, 0),
    m_translationFromUsd(0, 0, 0),
    m_shearFromUsd(0, 0, 0),
    m_scalePivotFromUsd(0, 0, 0),
    m_scalePivotTranslationFromUsd(0, 0, 0),
    m_rotatePivotFromUsd(0, 0, 0),
    m_rotatePivotTranslationFromUsd(0, 0, 0),
    m_rotateOrientationFromUsd(0, 0, 0, 1.0),
    m_localTranslateOffset(0, 0, 0),
    m_flags(0)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::TransformationMatrix\n");
  initialiseToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::setPrim(const UsdPrim& prim)
{
  if(prim.IsValid())
  {
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::setPrim %s\n", prim.GetName().GetText());
    m_prim = prim;
    UsdGeomXform xform(prim);
    m_xform = xform;
  }
  else
  {
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::setPrim null\n");
    m_prim = UsdPrim();
    m_xform = UsdGeomXform();
  }
  m_time = UsdTimeCode(UsdTimeCode::Default());
  // Most of these flags are calculated based on reading the usd prim; however, a few are driven
  // "externally" (ie, from attributes on the controlling transform node), and should NOT be reset
  // when we're re-initializing
  m_flags &= kPreservationMask;
  m_scaleTweak = MVector(0, 0, 0);
  m_rotationTweak = MEulerRotation(0, 0, 0);
  m_translationTweak = MVector(0, 0, 0);
  m_shearTweak = MVector(0, 0, 0);
  m_scalePivotTweak = MPoint(0, 0, 0);
  m_scalePivotTranslationTweak = MVector(0, 0, 0);
  m_rotatePivotTweak = MPoint(0, 0, 0);
  m_rotatePivotTranslationTweak = MVector(0, 0, 0);
  m_rotateOrientationTweak = MQuaternion(0, 0, 0, 1.0);
  m_localTranslateOffset = MVector(0, 0, 0);

  if(m_prim.IsValid())
  {
    m_scaleFromUsd = MVector(1.0, 1.0, 1.0);
    m_rotationFromUsd = MEulerRotation(0, 0, 0);
    m_translationFromUsd = MVector(0, 0, 0);
    m_shearFromUsd = MVector(0, 0, 0);
    m_scalePivotFromUsd = MPoint(0, 0, 0);
    m_scalePivotTranslationFromUsd = MVector(0, 0, 0);
    m_rotatePivotFromUsd = MPoint(0, 0, 0);
    m_rotatePivotTranslationFromUsd = MVector(0, 0, 0);
    m_rotateOrientationFromUsd = MQuaternion(0, 0, 0, 1.0);
    //if(MFileIO::isReadingFile())
    {
      initialiseToPrim(!MFileIO::isReadingFile());
    }

    MPxTransformationMatrix::scaleValue = m_scaleFromUsd;
    MPxTransformationMatrix::rotationValue = m_rotationFromUsd;
    MPxTransformationMatrix::translationValue = m_translationFromUsd;
    MPxTransformationMatrix::shearValue = m_shearFromUsd;
    MPxTransformationMatrix::scalePivotValue = m_scalePivotFromUsd;
    MPxTransformationMatrix::scalePivotTranslationValue = m_scalePivotTranslationFromUsd;
    MPxTransformationMatrix::rotatePivotValue = m_rotatePivotFromUsd;
    MPxTransformationMatrix::rotatePivotTranslationValue = m_rotatePivotTranslationFromUsd;
    MPxTransformationMatrix::rotateOrientationValue = m_rotateOrientationFromUsd;
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readVector(MVector& result, const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::readVector\n");
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kVec3d:
    {
      GfVec3d value;
      const bool retValue = op.GetAs<GfVec3d>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = value[0];
      result.y = value[1];
      result.z = value[2];
    }
    break;

  case UsdDataType::kVec3f:
    {
      GfVec3f value;
      const bool retValue = op.GetAs<GfVec3f>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = double(value[0]);
      result.y = double(value[1]);
      result.z = double(value[2]);
    }
    break;

  case UsdDataType::kVec3h:
    {
      GfVec3h value;
      const bool retValue = op.GetAs<GfVec3h>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = double(value[0]);
      result.y = double(value[1]);
      result.z = double(value[2]);
    }
    break;

  case UsdDataType::kVec3i:
    {
      GfVec3i value;
      const bool retValue = op.GetAs<GfVec3i>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = double(value[0]);
      result.y = double(value[1]);
      result.z = double(value[2]);
    }
    break;

  default:
    return false;
  }

  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::readVector %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushVector(const MVector& result, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::pushVector %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = getAttributeType(vtn);

  switch(attr_type)
  {
  case UsdDataType::kVec3d:
    {
      GfVec3d value(result.x, result.y, result.z);
      op.Set(value, timeCode);
    }
    break;

  case UsdDataType::kVec3f:
    {
      GfVec3f value(result.x, result.y, result.z);
      op.Set(value, timeCode);
    }
    break;

  case UsdDataType::kVec3h:
    {
      GfVec3h value(result.x, result.y, result.z);
      op.Set(value, timeCode);
    }
    break;

  case UsdDataType::kVec3i:
    {
      GfVec3i value(result.x, result.y, result.z);
      op.Set(value, timeCode);
    }
    break;

  default:
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushShear(const MVector& result, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::pushShear %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kMatrix4d:
    {
      GfMatrix4d m(
          1.0,      0.0,      0.0, 0.0,
          result.x, 1.0,      0.0, 0.0,
          result.y, result.z, 1.0, 0.0,
          0.0,      0.0,      0.0, 1.0);
      op.Set(m, timeCode);
    }
    break;

  default:
    return false;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readShear(MVector& result, const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::readShear\n");
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kMatrix4d:
    {
      GfMatrix4d value;
      const bool retValue = op.GetAs<GfMatrix4d>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = value[1][0];
      result.y = value[2][0];
      result.z = value[2][1];
    }
    break;

  default:
    return false;
  }
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::readShear %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readPoint(MPoint& result, const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::readPoint\n");
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kVec3d:
    {
      GfVec3d value;
      const bool retValue = op.GetAs<GfVec3d>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = value[0];
      result.y = value[1];
      result.z = value[2];
    }
    break;

  case UsdDataType::kVec3f:
    {
      GfVec3f value;
      const bool retValue = op.GetAs<GfVec3f>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = double(value[0]);
      result.y = double(value[1]);
      result.z = double(value[2]);
    }
    break;

  case UsdDataType::kVec3h:
    {
      GfVec3h value;
      const bool retValue = op.GetAs<GfVec3h>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = double(value[0]);
      result.y = double(value[1]);
      result.z = double(value[2]);
    }
    break;

  case UsdDataType::kVec3i:
    {
      GfVec3i value;
      const bool retValue = op.GetAs<GfVec3i>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = double(value[0]);
      result.y = double(value[1]);
      result.z = double(value[2]);
    }
    break;

  default:
    return false;
  }
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::readPoint %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readMatrix(MMatrix& result, const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::readMatrix\n");
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kMatrix4d:
    {
      GfMatrix4d value;
      const bool retValue = op.GetAs<GfMatrix4d>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result = *(const MMatrix*)(&value);
    }
    break;

  default:
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushMatrix(const MMatrix& result, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::pushMatrix\n");
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kMatrix4d:
    {
      GfMatrix4d value = *(const GfMatrix4d*)(&result);
      const bool retValue = op.Set<GfMatrix4d>(value, timeCode);
      if (!retValue)
      {
        return false;
      }
    }
    break;

  default:
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushPoint(const MPoint& result, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::pushPoint %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kVec3d:
    {
      GfVec3d value(result.x, result.y, result.z);
      op.Set(value, timeCode);
    }
    break;

  case UsdDataType::kVec3f:
    {
      GfVec3f value(result.x, result.y, result.z);
      op.Set(value, timeCode);
    }
    break;

  case UsdDataType::kVec3h:
    {
      GfVec3h value(result.x, result.y, result.z);
      op.Set(value, timeCode);
    }
    break;

  case UsdDataType::kVec3i:
    {
      GfVec3i value(result.x, result.y, result.z);
      op.Set(value, timeCode);
    }
    break;

  default:
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
double TransformationMatrix::readDouble(const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::readDouble\n");
  double result = 0;
  UsdDataType attr_type = getAttributeType(op.GetTypeName());
  switch(attr_type)
  {
  case UsdDataType::kHalf:
    {
      GfHalf value;
      const bool retValue = op.Get<GfHalf>(&value, timeCode);
      if (retValue)
      {
        result = float(value);
      }
    }
    break;

  case UsdDataType::kFloat:
    {
      float value;
      const bool retValue = op.Get<float>(&value, timeCode);
      if (retValue)
      {
        result = double(value);
      }
    }
    break;

  case UsdDataType::kDouble:
    {
      double value;
      const bool retValue = op.Get<double>(&value, timeCode);
      if (retValue)
      {
        result = value;
      }
    }
    break;

  case UsdDataType::kInt:
    {
      int32_t value;
      const bool retValue = op.Get<int32_t>(&value, timeCode);
      if (retValue)
      {
        result = double(value);
      }
    }
    break;

  default:
    break;
  }
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::readDouble %f\n%s\n", result, op.GetOpName().GetText());
  return result;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushDouble(const double value, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::pushDouble %f\n%s\n", value, op.GetOpName().GetText());
  UsdDataType attr_type = getAttributeType(op.GetTypeName());
  switch(attr_type)
  {
  case UsdDataType::kHalf:
    {
      op.Set(GfHalf(value), timeCode);
    }
    break;

  case UsdDataType::kFloat:
    {
      op.Set(float(value), timeCode);
    }
    break;

  case UsdDataType::kDouble:
    {
      op.Set(double(value), timeCode);
    }
    break;

  case UsdDataType::kInt:
    {
      op.Set(int32_t(value), timeCode);
    }
    break;

  default:
    break;
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readRotation(MEulerRotation& result, const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::readRotation %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());
  const double degToRad = 3.141592654 / 180.0;
  switch(op.GetOpType())
  {
  case UsdGeomXformOp::TypeRotateX:
    {
      result.x = readDouble(op, timeCode) * degToRad;
      result.y = 0.0;
      result.z = 0.0;
      result.order = MEulerRotation::kXYZ;
    }
    break;

  case UsdGeomXformOp::TypeRotateY:
    {
      result.x = 0.0;
      result.y = readDouble(op, timeCode) * degToRad;
      result.z = 0.0;
      result.order = MEulerRotation::kXYZ;
    }
    break;

  case UsdGeomXformOp::TypeRotateZ:
    {
      result.x = 0.0;
      result.y = 0.0;
      result.z = readDouble(op, timeCode) * degToRad;
      result.order = MEulerRotation::kXYZ;
    }
    break;

  case UsdGeomXformOp::TypeRotateXYZ:
    {
      MVector v;
      if(readVector(v, op, timeCode))
      {
        result.x = v.x * degToRad;
        result.y = v.y * degToRad;
        result.z = v.z * degToRad;
        result.order = MEulerRotation::kXYZ;
      }
      else
        return false;
    }
    break;

  case UsdGeomXformOp::TypeRotateXZY:
    {
      MVector v;
      if(readVector(v, op, timeCode))
      {
        result.x = v.x * degToRad;
        result.y = v.y * degToRad;
        result.z = v.z * degToRad;
        result.order = MEulerRotation::kXZY;
      }
      else
        return false;
    }
    break;

  case UsdGeomXformOp::TypeRotateYXZ:
    {
      MVector v;
      if(readVector(v, op, timeCode))
      {
        result.x = v.x * degToRad;
        result.y = v.y * degToRad;
        result.z = v.z * degToRad;
        result.order = MEulerRotation::kYXZ;
      }
      else
        return false;
    }
    break;

  case UsdGeomXformOp::TypeRotateYZX:
    {
      MVector v;
      if(readVector(v, op, timeCode))
      {
        result.x = v.x * degToRad;
        result.y = v.y * degToRad;
        result.z = v.z * degToRad;
        result.order = MEulerRotation::kYZX;
      }
      else
        return false;
    }
    break;

  case UsdGeomXformOp::TypeRotateZXY:
    {
      MVector v;
      if(readVector(v, op, timeCode))
      {
        result.x = v.x * degToRad;
        result.y = v.y * degToRad;
        result.z = v.z * degToRad;
        result.order = MEulerRotation::kZXY;
      }
      else
        return false;
    }
    break;

  case UsdGeomXformOp::TypeRotateZYX:
    {
      MVector v;
      if(readVector(v, op, timeCode))
      {
        result.x = v.x * degToRad;
        result.y = v.y * degToRad;
        result.z = v.z * degToRad;
        result.order = MEulerRotation::kZYX;
      }
      else
        return false;
    }
    break;

  default:
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushRotation(const MEulerRotation& value, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::pushRotation %f %f %f\n%s\n", value.x, value.y, value.z, op.GetOpName().GetText());
  const double radToDeg = 180.0 / 3.141592654;
  switch(op.GetOpType())
  {
  case UsdGeomXformOp::TypeRotateX:
    {
      pushDouble(value.x * radToDeg, op, timeCode);
    }
    break;

  case UsdGeomXformOp::TypeRotateY:
    {
      pushDouble(value.y * radToDeg, op, timeCode);
    }
    break;

  case UsdGeomXformOp::TypeRotateZ:
    {
      pushDouble(value.z * radToDeg, op, timeCode);
    }
    break;

  case UsdGeomXformOp::TypeRotateXYZ:
  case UsdGeomXformOp::TypeRotateXZY:
  case UsdGeomXformOp::TypeRotateYXZ:
  case UsdGeomXformOp::TypeRotateYZX:
  case UsdGeomXformOp::TypeRotateZYX:
  case UsdGeomXformOp::TypeRotateZXY:
    {
      MVector v(value.x, value.y, value.z);
      v *= radToDeg;
      return pushVector(v, op, timeCode);
    }
    break;

  default:
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::initialiseToPrim(bool readFromPrim, Transform* transformNode)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::initialiseToPrim\n");

  // if not yet initialized, do not execute this code! (It will crash!).
  if(!m_prim)
    return;

  bool resetsXformStack = false;
  m_xformops = m_xform.GetOrderedXformOps(&resetsXformStack);
  m_orderedOps.resize(m_xformops.size());

  if(!resetsXformStack)
    m_flags |= kInheritsTransform;

  if(matchesMayaProfile(m_xformops.begin(), m_xformops.end(), m_orderedOps.begin()))
  {
    m_flags |= kFromMayaSchema;
  }
  else
  {
  }

  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::const_iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    const UsdGeomXformOp& op = *it;
    switch(*opIt)
    {
    case kTranslate:
      {
        m_flags |= kPrimHasTranslation;
        if(op.GetNumTimeSamples() > 1)
        {
          m_flags |= kAnimatedTranslation;
        }
        if(readFromPrim) {
          internal_readVector(m_translationFromUsd, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::translateX).setValue(m_translationFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::translateY).setValue(m_translationFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::translateZ).setValue(m_translationFromUsd.z);
          }
        }
      }
      break;

    case kPivot:
      {
        m_flags |= kPrimHasPivot;
        if(readFromPrim) {
          internal_readPoint(m_scalePivotFromUsd, op);
          m_rotatePivotFromUsd = m_scalePivotFromUsd;
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotX).setValue(m_rotatePivotFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotY).setValue(m_rotatePivotFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotZ).setValue(m_rotatePivotFromUsd.z);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotX).setValue(m_scalePivotFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotY).setValue(m_scalePivotFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotZ).setValue(m_scalePivotFromUsd.z);
          }
        }
      }
      break;

    case kRotatePivotTranslate:
      {
        m_flags |= kPrimHasRotatePivotTranslate;
        if(readFromPrim)
        {
          internal_readVector(m_rotatePivotTranslationFromUsd, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotTranslateX).setValue(m_rotatePivotTranslationFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotTranslateY).setValue(m_rotatePivotTranslationFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotTranslateZ).setValue(m_rotatePivotTranslationFromUsd.z);
          }
        }
      }
      break;

    case kRotatePivot:
      {
        m_flags |= kPrimHasRotatePivot;
        if(readFromPrim)
        {
          internal_readPoint(m_rotatePivotFromUsd, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotX).setValue(m_rotatePivotFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotY).setValue(m_rotatePivotFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotZ).setValue(m_rotatePivotFromUsd.z);
          }
        }
      }
      break;

    case kRotate:
      {
        m_flags |= kPrimHasRotation;
        if(op.GetNumTimeSamples() > 1)
        {
          m_flags |= kAnimatedRotation;
        }
        if(readFromPrim)
        {
          internal_readRotation(m_rotationFromUsd, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::rotateX).setValue(m_rotationFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::rotateY).setValue(m_rotationFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::rotateZ).setValue(m_rotationFromUsd.z);
          }
        }
      }
      break;

    case kRotateAxis:
      {
        m_flags |= kPrimHasRotateAxes;
        if(readFromPrim) {
          MVector vec;
          internal_readVector(vec, op);
          MEulerRotation eulers(vec.x, vec.y, vec.z);
          m_rotateOrientationFromUsd = eulers.asQuaternion();
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::rotateAxisX).setValue(vec.x);
            MPlug(transformNode->thisMObject(), MPxTransform::rotateAxisY).setValue(vec.y);
            MPlug(transformNode->thisMObject(), MPxTransform::rotateAxisZ).setValue(vec.z);
          }
        }
      }
      break;

    case kRotatePivotInv:
      {
      }
      break;

    case kScalePivotTranslate:
      {
        m_flags |= kPrimHasScalePivotTranslate;
        if(readFromPrim) {
          internal_readVector(m_scalePivotTranslationFromUsd, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotTranslateX).setValue(m_scalePivotTranslationFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotTranslateY).setValue(m_scalePivotTranslationFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotTranslateZ).setValue(m_scalePivotTranslationFromUsd.z);
          }
        }
      }
      break;

    case kScalePivot:
      {
        m_flags |= kPrimHasScalePivot;
        if(readFromPrim) {
          internal_readPoint(m_scalePivotFromUsd, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotX).setValue(m_scalePivotFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotY).setValue(m_scalePivotFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotZ).setValue(m_scalePivotFromUsd.z);
          }
        }
      }
      break;

    case kShear:
      {
        m_flags |= kPrimHasShear;
        if(op.GetNumTimeSamples() > 1)
        {
          m_flags |= kAnimatedShear;
        }
        if(readFromPrim) {
          internal_readShear(m_shearFromUsd, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::shearXY).setValue(m_shearFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::shearXZ).setValue(m_shearFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::shearYZ).setValue(m_shearFromUsd.z);
          }
        }
      }
      break;

    case kScale:
      {
        m_flags |= kPrimHasScale;
        if(op.GetNumTimeSamples() > 1)
        {
          m_flags |= kAnimatedScale;
        }
        if(readFromPrim) {
          internal_readVector(m_scaleFromUsd, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::scaleX).setValue(m_scaleFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::scaleY).setValue(m_scaleFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::scaleZ).setValue(m_scaleFromUsd.z);
          }
        }
      }
      break;

    case kScalePivotInv:
      {
      }
      break;

    case kPivotInv:
      {
      }
      break;

    case kTransform:
      {
        m_flags |= kPrimHasTransform;
        m_flags |= kFromMatrix;
        m_flags |= kPushPrimToMatrix;
        if(op.GetNumTimeSamples() > 1)
        {
          m_flags |= kAnimatedMatrix;
        }

        if(readFromPrim) {
          MMatrix m;
          internal_readMatrix(m, op);
          decomposeMatrix(m);
          m_scaleFromUsd = scaleValue;
          m_rotationFromUsd = rotationValue;
          m_translationFromUsd = translationValue;
          m_shearFromUsd = shearValue;
          m_scalePivotFromUsd = scalePivotValue;
          m_scalePivotTranslationFromUsd = scalePivotTranslationValue;
          m_rotatePivotFromUsd = rotatePivotValue;
          m_rotatePivotTranslationFromUsd = rotatePivotTranslationValue;
          m_rotateOrientationFromUsd = rotateOrientationValue;
        }
      }
      break;

    case kUnknownOp:
      {
      }
      break;
    }
  }

  // if some animation keys are found on the transform ops, assume we have a read only viewer of the transform data.
  if(m_flags & kAnimationMask)
  {
    m_flags &= ~kPushToPrimEnabled;
    m_flags |= kReadAnimatedValues;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::updateToTime(const UsdTimeCode& time)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::updateToTime %f\n", time.GetValue());
  // if not yet initialized, do not execute this code! (It will crash!).
  if(!m_prim)
  {
    return;
  }

  if(m_time != time)
  {
    m_time = time;
    if(hasAnimation())
    {
      auto opIt = m_orderedOps.begin();
      for(std::vector<UsdGeomXformOp>::const_iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
      {
        const UsdGeomXformOp& op = *it;
        switch(*opIt)
        {
        case kTranslate:
          {
            if(hasAnimatedTranslation())
            {
              internal_readVector(m_translationFromUsd, op);
              MPxTransformationMatrix::translationValue = m_translationFromUsd + m_translationTweak;
            }
          }
          break;

        case kRotate:
          {
            if(hasAnimatedRotation())
            {
              internal_readRotation(m_rotationFromUsd, op);
              MPxTransformationMatrix::rotationValue = m_rotationFromUsd;
              MPxTransformationMatrix::rotationValue.x += m_rotationTweak.x;
              MPxTransformationMatrix::rotationValue.y += m_rotationTweak.y;
              MPxTransformationMatrix::rotationValue.z += m_rotationTweak.z;
            }
          }
          break;

        case kScale:
          {
            if(hasAnimatedScale())
            {
              internal_readVector(m_scaleFromUsd, op);
              MPxTransformationMatrix::scaleValue = m_scaleFromUsd + m_scaleTweak;
            }
          }
          break;

        case kShear:
          {
            if(hasAnimatedShear())
            {
              internal_readShear(m_shearFromUsd, op);
              MPxTransformationMatrix::shearValue = m_shearFromUsd + m_shearTweak;
            }
          }
          break;

        case kTransform:
          {
            if(hasAnimatedMatrix())
            {
              GfMatrix4d matrix;
              op.Get<GfMatrix4d>(&matrix, getTimeCode());
              double T[3], S[3];
              AL::usdmaya::utils::matrixToSRT(matrix, S, m_rotationFromUsd, T);
              m_scaleFromUsd.x = S[0];
              m_scaleFromUsd.y = S[1];
              m_scaleFromUsd.z = S[2];
              m_translationFromUsd.x = T[0];
              m_translationFromUsd.y = T[1];
              m_translationFromUsd.z = T[2];
              MPxTransformationMatrix::rotationValue.x = m_rotationFromUsd.x + m_rotationTweak.x;
              MPxTransformationMatrix::rotationValue.y = m_rotationFromUsd.y + m_rotationTweak.y;
              MPxTransformationMatrix::rotationValue.z = m_rotationFromUsd.z + m_rotationTweak.z;
              MPxTransformationMatrix::translationValue = m_translationFromUsd + m_translationTweak;
              MPxTransformationMatrix::scaleValue = m_scaleFromUsd + m_scaleTweak;
            }
          }
          break;

        default:
          break;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
// Translation
//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertTranslateOp()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::insertTranslateOp\n");
  // generate our translate op, and insert into the correct stack location
  UsdGeomXformOp op = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("translate"));
  m_xformops.insert(m_xformops.begin(), op);
  m_orderedOps.insert(m_orderedOps.begin(), kTranslate);
  m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
  m_flags |= kPrimHasTranslation;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::translateTo(const MVector& vector, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::translateTo %f %f %f\n", vector.x, vector.y, vector.z);
  if(isTranslateLocked())
    return MS::kSuccess;

  MStatus status = MPxTransformationMatrix::translateTo(vector, space);
  if(status)
  {
    m_translationTweak = MPxTransformationMatrix::translationValue - m_translationFromUsd;
  }

  if(pushToPrimAvailable())
  {
    // if the prim does not contain a translation, make sure we insert a transform op for that.
    if(primHasTranslation())
    {
      // helping the branch predictor
    }
    else
    if(!pushPrimToMatrix())
    {
      insertTranslateOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::translateBy(const MVector& vector, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::translateBy %f %f %f\n", vector.x, vector.y, vector.z);
  if(isTranslateLocked())
    return MS::kSuccess;

  MStatus status = MPxTransformationMatrix::translateBy(vector, space);
  if(status)
  {
    m_translationTweak = MPxTransformationMatrix::translationValue - m_translationFromUsd;
  }
  if(pushToPrimAvailable())
  {
    // if the prim does not contain a translation, make sure we insert a transform op for that.
    if(primHasTranslation())
    {
      // helping the branch predictor
    }
    else
    if(!pushPrimToMatrix())
    {
      // generate our translate op, and insert into the correct stack location
      UsdGeomXformOp op = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("translate"));
      m_xformops.insert(m_xformops.begin(), op);
      m_orderedOps.insert(m_orderedOps.begin(), kTranslate);
      m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
      m_flags |= kPrimHasTranslation;
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
// Scale
//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertScaleOp()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::insertScaleOp\n");
  // generate our translate op, and insert into the correct stack location
  UsdGeomXformOp op = m_xform.AddScaleOp(UsdGeomXformOp::PrecisionFloat, TfToken("scale"));

  auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kScale);
  auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());

  m_xformops.insert(posInXfm, op);
  m_orderedOps.insert(posInOps, kScale);
  m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
  m_flags |= kPrimHasScale;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::scaleTo(const MVector& scale, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::scaleTo %f %f %f\n", scale.x, scale.y, scale.z);
  if(isScaleLocked())
    return MStatus::kSuccess;
  MStatus status = MPxTransformationMatrix::scaleTo(scale, space);
  if(status)
  {
    m_scaleTweak = MPxTransformationMatrix::scaleValue - m_scaleFromUsd;
  }
  if(pushToPrimAvailable())
  {
    if(primHasScale())
    {
      // helping the branch predictor
    }
    else
    if(!pushPrimToMatrix())
    {
      // rare case: add a new scale op into the prim
      insertScaleOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::scaleBy(const MVector& scale, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::scaleBy %f %f %f\n", scale.x, scale.y, scale.z);
  if(isScaleLocked())
    return MStatus::kSuccess;
  MStatus status = MPxTransformationMatrix::scaleBy(scale, space);
  if(status)
  {
    m_scaleTweak = MPxTransformationMatrix::scaleValue - m_scaleFromUsd;
  }
  if(pushToPrimAvailable())
  {
    if(primHasScale())
    {
      // helping the branch predictor
    }
    else
    if(!pushPrimToMatrix())
    {
      // rare case: add a new scale op into the prim
      insertScaleOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
// Shear
//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertShearOp()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::insertShearOp\n");
  // generate our translate op, and insert into the correct stack location
  UsdGeomXformOp op = m_xform.AddTransformOp(UsdGeomXformOp::PrecisionDouble, TfToken("shear"));

  auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kShear);
  auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());

  m_xformops.insert(posInXfm, op);
  m_orderedOps.insert(posInOps, kShear);
  m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
  m_flags |= kPrimHasShear;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::shearTo(const MVector& shear, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::shearTo %f %f %f\n", shear.x, shear.y, shear.z);
  MStatus status = MPxTransformationMatrix::shearTo(shear, space);
  if(status)
  {
    m_scaleTweak = MPxTransformationMatrix::shearValue - m_shearFromUsd;
  }
  if(pushToPrimAvailable())
  {
    if(primHasShear())
    {
      // helping the branch predictor
    }
    else
    if(!pushPrimToMatrix())
    {
      // rare case: add a new scale op into the prim
      insertShearOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::shearBy(const MVector& shear, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::shearBy %f %f %f\n", shear.x, shear.y, shear.z);
  MStatus status = MPxTransformationMatrix::shearBy(shear, space);
  if(status)
  {
    m_scaleTweak = MPxTransformationMatrix::shearValue - m_shearFromUsd;
  }
  if(pushToPrimAvailable())
  {
    if(primHasShear())
    {
      // helping the branch predictor
    }
    else
    if(!pushPrimToMatrix())
    {
      // rare case: add a new scale op into the prim
      insertShearOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertScalePivotOp()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::insertScalePivotOp\n");
  // generate our translate op, and insert into the correct stack location
  UsdGeomXformOp op = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivot"));
  UsdGeomXformOp opinv = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivot"), true);

  {
    auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kScalePivot);
    auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());
    m_xformops.insert(posInXfm, op);
    m_orderedOps.insert(posInOps, kScalePivot);
  }
  {
    auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kScalePivotInv);
    auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());
    m_xformops.insert(posInXfm, opinv);
    m_orderedOps.insert(posInOps, kScalePivotInv);
  }
  m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
  m_flags |= kPrimHasScalePivot;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setScalePivot(const MPoint& sp, MSpace::Space space, bool balance)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::setScalePivot %f %f %f\n", sp.x, sp.y, sp.z);
  MStatus status = MPxTransformationMatrix::setScalePivot(sp, space, balance);
  if(status)
  {
    m_scalePivotTweak = MPxTransformationMatrix::scalePivotValue - m_scalePivotFromUsd;
  }
  if(pushToPrimAvailable())
  {
    // Do not insert a scale pivot op if the input prim has a generic pivot.
    if(primHasScalePivot() || primHasPivot())
    {
    }
    else
    if(!pushPrimToMatrix())
    {
      insertScalePivotOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertScalePivotTranslationOp()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::insertScalePivotTranslationOp\n");
  // generate our translate op, and insert into the correct stack location
  UsdGeomXformOp op = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("scalePivotTranslate"));

  auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kScalePivotTranslate);
  auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());

  m_xformops.insert(posInXfm, op);
  m_orderedOps.insert(posInOps, kScalePivotTranslate);
  m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
  m_flags |= kPrimHasScalePivotTranslate;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setScalePivotTranslation(const MVector& sp, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::setScalePivotTranslation %f %f %f\n", sp.x, sp.y, sp.z);
  MStatus status = MPxTransformationMatrix::setScalePivotTranslation(sp, space);
  if(status)
  {
    m_scalePivotTranslationTweak = MPxTransformationMatrix::scalePivotTranslationValue - m_scalePivotTranslationFromUsd;
  }
  if(pushToPrimAvailable())
  {
    if(primHasScalePivotTranslate())
    {
    }
    else
    if(!pushPrimToMatrix())
    {
      insertScalePivotTranslationOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertRotatePivotOp()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::insertRotatePivotOp\n");
  // generate our translate op, and insert into the correct stack location
  UsdGeomXformOp op = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivot"));
  UsdGeomXformOp opinv = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivot"), true);

  {
    auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kRotatePivot);
    auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());
    m_xformops.insert(posInXfm, op);
    m_orderedOps.insert(posInOps, kRotatePivot);
  }
  {
    auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kRotatePivotInv);
    auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());
    m_xformops.insert(posInXfm, opinv);
    m_orderedOps.insert(posInOps, kRotatePivotInv);
  }
  m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
  m_flags |= kPrimHasRotatePivot;
}


//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotatePivot(const MPoint& pivot, MSpace::Space space, bool balance)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::setRotatePivot %f %f %f\n", pivot.x, pivot.y, pivot.z);
  MStatus status = MPxTransformationMatrix::setRotatePivot(pivot, space, balance);
  if(status)
  {
    m_rotatePivotTweak = MPxTransformationMatrix::rotatePivotValue - m_rotatePivotFromUsd;
  }
  if(pushToPrimAvailable())
  {
    // Do not insert a rotate pivot op if the input prim has a generic pivot.
    if(primHasRotatePivot() || primHasPivot())
    {
    }
    else
    if(!pushPrimToMatrix())
    {
      insertRotatePivotOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertRotatePivotTranslationOp()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::insertRotatePivotTranslationOp\n");
  // generate our translate op, and insert into the correct stack location
  UsdGeomXformOp op = m_xform.AddTranslateOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotatePivotTranslate"));

  auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kRotatePivotTranslate);
  auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());

  m_xformops.insert(posInXfm, op);
  m_orderedOps.insert(posInOps, kRotatePivotTranslate);
  m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
  m_flags |= kPrimHasRotatePivotTranslate;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotatePivotTranslation(const MVector &vector, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::setRotatePivotTranslation %f %f %f\n", vector.x, vector.y, vector.z);
  MStatus status = MPxTransformationMatrix::setRotatePivotTranslation(vector, space);
  if(status)
  {
    m_rotatePivotTranslationTweak = MPxTransformationMatrix::rotatePivotTranslationValue - m_rotatePivotTranslationFromUsd;
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotatePivotTranslate())
    {
    }
    else
    if(!pushPrimToMatrix())
    {
      insertRotatePivotTranslationOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertRotateOp()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::insertRotateOp\n");
  // generate our translate op, and insert into the correct stack location
  UsdGeomXformOp op;

  switch(rotationOrder())
  {
  case MTransformationMatrix::kXYZ:
    op = m_xform.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotate"));
    break;

  case MTransformationMatrix::kXZY:
    op = m_xform.AddRotateXZYOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotate"));
    break;

  case MTransformationMatrix::kYXZ:
    op = m_xform.AddRotateYXZOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotate"));
    break;

  case MTransformationMatrix::kYZX:
    op = m_xform.AddRotateYZXOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotate"));
    break;

  case MTransformationMatrix::kZXY:
    op = m_xform.AddRotateZXYOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotate"));
    break;

  case MTransformationMatrix::kZYX:
    op = m_xform.AddRotateZYXOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotate"));
    break;

  default:
    break;
  }

  auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kRotate);
  auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());

  m_xformops.insert(posInXfm, op);
  m_orderedOps.insert(posInOps, kRotate);
  m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
  m_flags |= kPrimHasRotation;

}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::rotateTo(const MQuaternion &q, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::rotateTo %f %f %f %f\n", q.x, q.y, q.z, q.w);
  if(isRotateLocked())
    return MS::kSuccess;
  MStatus status = MPxTransformationMatrix::rotateTo(q, space);
  if(status)
  {
    m_rotationTweak.x = MPxTransformationMatrix::rotationValue.x - m_rotationFromUsd.x;
    m_rotationTweak.y = MPxTransformationMatrix::rotationValue.y - m_rotationFromUsd.y;
    m_rotationTweak.z = MPxTransformationMatrix::rotationValue.z - m_rotationFromUsd.z;
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotation())
    {
    }
    else
    if(!pushPrimToMatrix())
    {
      insertRotateOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::rotateBy(const MQuaternion &q, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::rotateBy %f %f %f %f\n", q.x, q.y, q.z, q.w);
  if(isRotateLocked())
    return MS::kSuccess;
  MStatus status = MPxTransformationMatrix::rotateBy(q, space);
  if(status)
  {
    m_rotationTweak.x = MPxTransformationMatrix::rotationValue.x - m_rotationFromUsd.x;
    m_rotationTweak.y = MPxTransformationMatrix::rotationValue.y - m_rotationFromUsd.y;
    m_rotationTweak.z = MPxTransformationMatrix::rotationValue.z - m_rotationFromUsd.z;
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotation())
    {
    }
    else
    if(!pushPrimToMatrix())
    {
      insertRotateOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::rotateTo(const MEulerRotation &e, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::rotateTo %f %f %f\n", e.x, e.y, e.z);
  if(isRotateLocked())
    return MS::kSuccess;
  MStatus status = MPxTransformationMatrix::rotateTo(e, space);
  if(status)
  {
    m_rotationTweak.x = MPxTransformationMatrix::rotationValue.x - m_rotationFromUsd.x;
    m_rotationTweak.y = MPxTransformationMatrix::rotationValue.y - m_rotationFromUsd.y;
    m_rotationTweak.z = MPxTransformationMatrix::rotationValue.z - m_rotationFromUsd.z;
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotation())
    {
    }
    else
    if(!pushPrimToMatrix())
    {
      insertRotateOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::rotateBy(const MEulerRotation &e, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::rotateBy %f %f %f\n", e.x, e.y, e.z);
  if(isRotateLocked())
    return MS::kSuccess;
  MStatus status = MPxTransformationMatrix::rotateBy(e, space);
  if(status)
  {
    m_rotationTweak.x = MPxTransformationMatrix::rotationValue.x - m_rotationFromUsd.x;
    m_rotationTweak.y = MPxTransformationMatrix::rotationValue.y - m_rotationFromUsd.y;
    m_rotationTweak.z = MPxTransformationMatrix::rotationValue.z - m_rotationFromUsd.z;
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotation())
    {
    }
    else
    if(!pushPrimToMatrix())
    {
      insertRotateOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotationOrder(MTransformationMatrix::RotationOrder, bool)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::setRotationOrder\n");
  // do not allow people to change the rotation order here.
  // It's too hard for my feeble brain to figure out how to remap that to the USD data.
  return MS::kFailure;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::insertRotateAxesOp()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::insertRotateAxesOp\n");
  // generate our translate op, and insert into the correct stack location
  UsdGeomXformOp op = m_xform.AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat, TfToken("rotateAxis"));

  auto posInOps = std::lower_bound(m_orderedOps.begin(), m_orderedOps.end(), kRotateAxis);
  auto posInXfm = m_xformops.begin() + (posInOps - m_orderedOps.begin());

  m_xformops.insert(posInXfm, op);
  m_orderedOps.insert(posInOps, kRotateAxis);
  m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
  m_flags |= kPrimHasRotateAxes;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotateOrientation(const MQuaternion &q, MSpace::Space space, bool balance)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::setRotateOrientation %f %f %f %f\n", q.x, q.y, q.z, q.w);
  MStatus status = MPxTransformationMatrix::setRotateOrientation(q, space, balance);
  if(status)
  {
    m_rotateOrientationFromUsd = MPxTransformationMatrix::rotateOrientationValue * m_rotateOrientationTweak.inverse();
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotateAxes())
    {
    }
    else
    if(!pushPrimToMatrix())
    {
      insertRotateAxesOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotateOrientation(const MEulerRotation& euler, MSpace::Space space, bool balance)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::setRotateOrientation %f %f %f\n", euler.x, euler.y, euler.z);
  MStatus status = MPxTransformationMatrix::setRotateOrientation(euler, space, balance);
  if(status)
  {
    m_rotateOrientationFromUsd = MPxTransformationMatrix::rotateOrientationValue * m_rotateOrientationTweak.inverse();
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotateAxes())
    {
    }
    else
    if(!pushPrimToMatrix())
    {
      insertRotateAxesOp();
    }
    pushToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushToPrim()
{
  // if not yet intiaialised, do not execute this code! (It will crash!).
  if(!m_prim)
    return;
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::pushToPrim\n");

  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    UsdGeomXformOp& op = *it;
    switch(*opIt)
    {
    case kTranslate:
      {
        internal_pushVector(MPxTransformationMatrix::translationValue, op);
        m_translationFromUsd = MPxTransformationMatrix::translationValue;
        m_translationTweak = MVector(0, 0, 0);
      }
      break;

    case kPivot:
      {
        // is this a bug?
        internal_pushPoint(MPxTransformationMatrix::rotatePivotValue, op);
        m_rotatePivotFromUsd = MPxTransformationMatrix::rotatePivotValue;
        m_rotatePivotTweak = MPoint(0, 0, 0);
        m_scalePivotFromUsd = MPxTransformationMatrix::scalePivotValue;
        m_scalePivotTweak = MVector(0, 0, 0);
      }
      break;

    case kRotatePivotTranslate:
      {
        internal_pushPoint(MPxTransformationMatrix::rotatePivotTranslationValue, op);
        m_rotatePivotTranslationFromUsd = MPxTransformationMatrix::rotatePivotTranslationValue;
        m_rotatePivotTranslationTweak = MVector(0, 0, 0);
      }
      break;

    case kRotatePivot:
      {
        internal_pushPoint(MPxTransformationMatrix::rotatePivotValue, op);
        m_rotatePivotFromUsd = MPxTransformationMatrix::rotatePivotValue;
        m_rotatePivotTweak = MPoint(0, 0, 0);
      }
      break;

    case kRotate:
      {
        internal_pushRotation(MPxTransformationMatrix::rotationValue, op);
        m_rotationFromUsd = MPxTransformationMatrix::rotationValue;
        m_rotationTweak = MEulerRotation(0, 0, 0);
      }
      break;

    case kRotateAxis:
      {
        const double radToDeg = 180.0 / 3.141592654;
        MEulerRotation e = m_rotateOrientationFromUsd.asEulerRotation();
        MVector vec(e.x * radToDeg, e.y * radToDeg, e.z * radToDeg);
        internal_pushVector(vec, op);
      }
      break;

    case kRotatePivotInv:
      {
      }
      break;

    case kScalePivotTranslate:
      {
        internal_pushVector(MPxTransformationMatrix::scalePivotTranslationValue, op);
        m_scalePivotTranslationFromUsd = MPxTransformationMatrix::scalePivotTranslationValue;
        m_scalePivotTranslationTweak = MVector(0, 0, 0);
      }
      break;

    case kScalePivot:
      {
        internal_pushPoint(MPxTransformationMatrix::scalePivotValue, op);
        m_scalePivotFromUsd = MPxTransformationMatrix::scalePivotValue;
        m_scalePivotTweak = MPoint(0, 0, 0);
      }
      break;

    case kShear:
      {
        internal_pushShear(MPxTransformationMatrix::shearValue, op);
        m_shearFromUsd = MPxTransformationMatrix::shearValue;
        m_shearTweak = MVector(0, 0, 0);
      }
      break;

    case kScale:
      {
        internal_pushVector(MPxTransformationMatrix::scaleValue, op);
        m_scaleFromUsd = MPxTransformationMatrix::scaleValue;
        m_scaleTweak = MVector(0, 0, 0);
      }
      break;

    case kScalePivotInv:
      {
      }
      break;

    case kPivotInv:
      {
      }
      break;

    case kTransform:
      {
        if(pushPrimToMatrix())
        {
          MMatrix m = MPxTransformationMatrix::asMatrix();
          op.Set(*(const GfMatrix4d*)&m, getTimeCode());
        }
      }
      break;

    case kUnknownOp:
      {
      }
      break;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
MMatrix TransformationMatrix::asMatrix() const
{
  // Get the current transform matrix
  MMatrix m = MPxTransformationMatrix::asMatrix();

  const double x = m_localTranslateOffset.x;
  const double y = m_localTranslateOffset.y;
  const double z = m_localTranslateOffset.z;

  m[3][0] += m[0][0] * x;
  m[3][1] += m[0][1] * x;
  m[3][2] += m[0][2] * x;
  m[3][0] += m[1][0] * y;
  m[3][1] += m[1][1] * y;
  m[3][2] += m[1][2] * y;
  m[3][0] += m[2][0] * z;
  m[3][1] += m[2][1] * z;
  m[3][2] += m[2][2] * z;

  // Let Maya know what the matrix should be
  return m;
}

//----------------------------------------------------------------------------------------------------------------------
MMatrix TransformationMatrix::asMatrix(double percent) const
{
  MMatrix m = MPxTransformationMatrix::asMatrix(percent);

  const double x = m_localTranslateOffset.x * percent;
  const double y = m_localTranslateOffset.y * percent;
  const double z = m_localTranslateOffset.z * percent;

  m[3][0] += m[0][0] * x;
  m[3][1] += m[0][1] * x;
  m[3][2] += m[0][2] * x;
  m[3][0] += m[1][0] * y;
  m[3][1] += m[1][1] * y;
  m[3][2] += m[1][2] * y;
  m[3][0] += m[2][0] * z;
  m[3][1] += m[2][1] * z;
  m[3][2] += m[2][2] * z;

  return m;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::enableReadAnimatedValues(bool enabled)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::enableReadAnimatedValues\n");
  if(enabled) m_flags |= kReadAnimatedValues;
  else m_flags &= ~kReadAnimatedValues;

  // if not yet intiaialised, do not execute this code! (It will crash!).
  if(!m_prim)
    return;

  // if we are enabling push to prim, we need to see if anything has changed on the transform since the last time
  // the values were synced. I'm assuming that if a given transform attribute is not the same as the default, or
  // the prim already has a transform op for that attribute, then just call a method to make a minor adjustment
  // of nothing. This will call my code that will magically construct the transform ops in the right order.
  if(enabled)
  {
    if(!pushPrimToMatrix())
    {
      if(primHasTranslation() || translation() != MVector::zero)
        translateBy(MVector::zero);

      if(primHasScale() || scale() != MVector::one)
        scaleBy(MVector::one);

      if(primHasShear() || shear() != MVector::zero)
        shearBy(MVector::zero);

      if(primHasScalePivot() || scalePivot() != MPoint::origin)
        setScalePivot(scalePivot(), MSpace::kTransform, false);

      if(primHasScalePivot() || scalePivot() != MPoint::origin)
        setScalePivotTranslation(scalePivotTranslation(), MSpace::kTransform);

      if(primHasRotatePivot() || rotatePivot() != MPoint::origin)
        setRotatePivot(rotatePivot(), MSpace::kTransform, false);

      if(primHasRotatePivotTranslate() || rotatePivotTranslation() != MVector::zero)
        setRotatePivotTranslation(rotatePivotTranslation(), MSpace::kTransform);

      if(primHasRotation() || rotation() != MQuaternion::identity)
        rotateBy(MQuaternion::identity);

      if(primHasRotateAxes() || rotateOrientation() != MQuaternion::identity)
        setRotateOrientation(rotateOrientation(), MSpace::kTransform, false);
    }
    else
    if(primHasTransform())
    {
      for(size_t i = 0, n = m_orderedOps.size(); i < n; ++i)
      {
        if(m_orderedOps[i] == kTransform)
        {
          MMatrix m = MPxTransformationMatrix::asMatrix();
          m_xformops[i].Set(*(const GfMatrix4d*)&m, getTimeCode());
          break;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::enablePushToPrim(bool enabled)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::enablePushToPrim\n");
  if(enabled) m_flags |= kPushToPrimEnabled;
  else m_flags &= ~kPushToPrimEnabled;

  // if not yet intiaialised, do not execute this code! (It will crash!).
  if(!m_prim)
    return;

  // if we are enabling push to prim, we need to see if anything has changed on the transform since the last time
  // the values were synced. I'm assuming that if a given transform attribute is not the same as the default, or
  // the prim already has a transform op for that attribute, then just call a method to make a minor adjustment
  // of nothing. This will call my code that will magically construct the transform ops in the right order.
  if(enabled && getTimeCode() == UsdTimeCode::Default())
  {
    if(!pushPrimToMatrix())
    {
      if(primHasTranslation() || translation() != MVector::zero)
        translateBy(MVector::zero);

      if(primHasScale() || scale() != MVector::one)
        scaleBy(MVector::one);

      if(primHasShear() || shear() != MVector::zero)
        shearBy(MVector::zero);

      if(primHasScalePivot() || scalePivot() != MPoint::origin)
        setScalePivot(scalePivot(), MSpace::kTransform, false);

      if(primHasScalePivotTranslate() || scalePivotTranslation() != MPoint::origin)
        setScalePivotTranslation(scalePivotTranslation(), MSpace::kTransform);

      if(primHasRotatePivot() || rotatePivot() != MPoint::origin)
        setRotatePivot(rotatePivot(), MSpace::kTransform, false);

      if(primHasRotatePivotTranslate() || rotatePivotTranslation() != MVector::zero)
        setRotatePivotTranslation(rotatePivotTranslation(), MSpace::kTransform);

      if(primHasRotation() || rotation() != MQuaternion::identity)
        rotateBy(MQuaternion::identity);

      if(primHasRotateAxes() || rotateOrientation() != MQuaternion::identity)
        setRotateOrientation(rotateOrientation(), MSpace::kTransform, false);
    }
    else
    if(primHasTransform())
    {
      for(size_t i = 0, n = m_orderedOps.size(); i < n; ++i)
      {
        if(m_orderedOps[i] == kTransform)
        {
          MMatrix m = MPxTransformationMatrix::asMatrix();
          m_xformops[i].Set(*(const GfMatrix4d*)&m, getTimeCode());
          break;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
