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

#include "AL/usd/utils/MayaTransformAPI.h"
#include <iostream>

namespace AL {
namespace usd {
namespace utils {

namespace {
namespace tokens {
const TfToken rotateXYZ("xformOp:rotateXYZ");
const TfToken rotateXZY("xformOp:rotateXZY");
const TfToken rotateYXZ("xformOp:rotateYXZ");
const TfToken rotateYZX("xformOp:rotateYZX");
const TfToken rotateZXY("xformOp:rotateZXY");
const TfToken rotateZYX("xformOp:rotateZYX");
const TfToken rotateAxis("xformOp:rotateXYZ:rotateAxis");
const TfToken scale("xformOp:scale");
const TfToken translate("xformOp:translate");
const TfToken pivot("xformOp:translate:pivot");
const TfToken pivotINV("!invert!xformOp:translate:pivot");
const TfToken rotatePivot("xformOp:translate:rotatePivot");
const TfToken scalePivot("xformOp:translate:scalePivot");
const TfToken rotatePivotINV("!invert!xformOp:translate:rotatePivot");
const TfToken scalePivotINV("!invert!xformOp:translate:scalePivot");
const TfToken rotatePivotTranslate("xformOp:translate:rotatePivotTranslate");
const TfToken scalePivotTranslate("xformOp:translate:scalePivotTranslate");
const TfToken shear("xformOp:transform:shear");

const TfToken scalePivotName("scalePivot");
const TfToken rotatePivotName("rotatePivot");
const TfToken scalePivotTranslateName("scalePivotTranslate");
const TfToken rotatePivotTranslateName("rotatePivotTranslate");
const TfToken rotateAxisName("rotateAxis");
const TfToken shearName("shear");

#if AL_SUPPORT_LEGACY_NAMES
const TfToken rotateXYZOld("xformOp:rotateXYZ:rotate");
const TfToken rotateXZYOld("xformOp:rotateXZY:rotate");
const TfToken rotateYXZOld("xformOp:rotateYXZ:rotate");
const TfToken rotateYZXOld("xformOp:rotateYZX:rotate");
const TfToken rotateZXYOld("xformOp:rotateZXY:rotate");
const TfToken rotateZYXOld("xformOp:rotateZYX:rotate");
const TfToken translateOld("xformOp:translate:translate");
const TfToken rotatePivotINVOld("xformOp:translate:rotatePivotINV");
const TfToken scalePivotINVOld("xformOp:translate:scalePivotINV");
const TfToken scaleOld("xformOp:scale:scale");
#define XFORM_NAME_CHECK(newName) (name == (newName) || name == (newName ## Old))
#else
#define XFORM_NAME_CHECK(newName) (name == (newName))
#endif
}
}

//----------------------------------------------------------------------------------------------------------------------
bool MayaTransformAPI::matchesMayaTrasformProfile(const std::vector<UsdGeomXformOp>& orderedOps)
{
  bool result = _matchesMayaTrasformProfile(orderedOps);
  if(!result)
  {
    m_api = TransformAPI::kFallback;
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------
bool MayaTransformAPI::_matchesMayaTrasformProfile(const std::vector<UsdGeomXformOp>& orderedOps)
{
  // no ops defined, so we can assume the maya profile can be used. 
  if(orderedOps.empty())
    return true;

  auto iter = orderedOps.begin();
  auto end = orderedOps.end();
  auto type = iter->GetOpType();
  auto name = iter->GetOpName();

  if(type == UsdGeomXformOp::TypeTranslate && XFORM_NAME_CHECK(tokens::translate))
  {
    m_translate = *iter;
    if(++iter == end)
      return true;
    type = iter->GetOpType();
    name = iter->GetOpName();
  }

  if(type == UsdGeomXformOp::TypeTranslate && name == tokens::pivot)
  {
    m_pivot = *iter;
    m_api = TransformAPI::kCommon;
    if(++iter == end)
      return true;
    type = iter->GetOpType();
    name = iter->GetOpName();
  }

  if(type == UsdGeomXformOp::TypeTranslate && name == tokens::rotatePivotTranslate)
  {
    m_rotatePivotTranslate = *iter;
    if(++iter == end)
      return m_api != TransformAPI::kCommon;
    type = iter->GetOpType();
    name = iter->GetOpName();
  }

  if(type == UsdGeomXformOp::TypeTranslate && name == tokens::rotatePivot)
  {
    m_rotatePivot = *iter;
    if(++iter == end)
      return m_api != TransformAPI::kCommon;
    type = iter->GetOpType();
    name = iter->GetOpName();
  }

  switch(type)
  {
  case UsdGeomXformOp::TypeRotateXYZ:
    {
      if(XFORM_NAME_CHECK(tokens::rotateXYZ))
      {
        m_order = RotationOrder::kXYZ;
        m_rotate = *iter;
        if(++iter == end)
          return true;
        type = iter->GetOpType();
        name = iter->GetOpName();
      }
    }
    break;

  case UsdGeomXformOp::TypeRotateXZY:
    {
      if(XFORM_NAME_CHECK(tokens::rotateXZY))
      {
        m_order = RotationOrder::kXZY;
        m_rotate = *iter;
        if(++iter == end)
          return true;
        type = iter->GetOpType();
        name = iter->GetOpName();
      }
    }
    break;

  case UsdGeomXformOp::TypeRotateYXZ:
    {
      if(XFORM_NAME_CHECK(tokens::rotateYXZ))
      {
        m_order = RotationOrder::kYXZ;
        m_rotate = *iter;
        if(++iter == end)
          return true;
        type = iter->GetOpType();
        name = iter->GetOpName();
      }
    }
    break;

  case UsdGeomXformOp::TypeRotateYZX:
    {
      if(XFORM_NAME_CHECK(tokens::rotateYZX))
      {
        m_order = RotationOrder::kYZX;
        m_rotate = *iter;
        if(++iter == end)
          return true;
        type = iter->GetOpType();
        name = iter->GetOpName();
      }
    }
    break;

  case UsdGeomXformOp::TypeRotateZXY:
    {
      if(XFORM_NAME_CHECK(tokens::rotateZXY))
      {
        m_order = RotationOrder::kZXY;
        m_rotate = *iter;
        if(++iter == end)
          return true;
        type = iter->GetOpType();
        name = iter->GetOpName();
      }
    }
    break;

  case UsdGeomXformOp::TypeRotateZYX:
    {
      if(XFORM_NAME_CHECK(tokens::rotateZYX))
      {
        m_order = RotationOrder::kZYX;
        m_rotate = *iter;
        if(++iter == end)
          return true;
        type = iter->GetOpType();
        name = iter->GetOpName();
      }
    }
    break;

  default:
    break;
  }

  if(tokens::rotateAxis == name)
  {
    m_rotateAxis = *iter;
    if(++iter == end)
      return true;
    type = iter->GetOpType();
    name = iter->GetOpName();
  }

  if(type == UsdGeomXformOp::TypeTranslate && XFORM_NAME_CHECK(tokens::rotatePivotINV))
  {
    m_rotatePivotINV = *iter;
    if(++iter == end)
      return m_api != TransformAPI::kCommon;
    type = iter->GetOpType();
    name = iter->GetOpName();
  }

  if(type == UsdGeomXformOp::TypeTranslate && name == tokens::scalePivotTranslate)
  {
    m_scalePivotTranslate = *iter;
    if(++iter == end)
      return m_api != TransformAPI::kCommon;
    type = iter->GetOpType();
    name = iter->GetOpName();
  }

  if(type == UsdGeomXformOp::TypeTranslate && name == tokens::scalePivot)
  {
    m_scalePivot = *iter;
    if(++iter == end)
      return m_api != TransformAPI::kCommon;
    type = iter->GetOpType();
    name = iter->GetOpName();
  }

  if(type == UsdGeomXformOp::TypeTransform && name == tokens::shear)
  {
    m_shear = *iter;
    if(++iter == end)
      return true;
    type = iter->GetOpType();
    name = iter->GetOpName();
  }

  if(type == UsdGeomXformOp::TypeScale && XFORM_NAME_CHECK(tokens::scale))
  {
    m_scale = *iter;
    if(++iter == end)
      return true;
    type = iter->GetOpType();
    name = iter->GetOpName();
  }

  if(type == UsdGeomXformOp::TypeTranslate && XFORM_NAME_CHECK(tokens::scalePivotINV))
  {
    m_scalePivotINV = *iter;
    if(++iter == end)
      return m_api != TransformAPI::kCommon;
    type = iter->GetOpType();
    name = iter->GetOpName();
  }

  if(type == UsdGeomXformOp::TypeTranslate && name == tokens::pivotINV)
  {
    m_pivotINV = *iter;
    if(++iter == end)
      return m_api == TransformAPI::kCommon;
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------
MayaTransformAPI::MayaTransformAPI(UsdPrim prim, bool convertMatrixOpToComponentOps)
  : m_prim()
{
  bool reset = false;
  std::vector<UsdGeomXformOp> ops = UsdGeomXformable(prim).GetOrderedXformOps(&reset);
  if(matchesMayaTrasformProfile(ops))
  {
    // only assign if we can process the data
    m_prim = prim;
  }

  // if the prim is not valid, check to see if we have a transform Op.
  if(convertMatrixOpToComponentOps && !m_prim)
  {
    if(ops.size() == 1 && ops[0].GetOpType() == UsdGeomXformOp::TypeTransform)
    {
      m_prim = prim;
      this->convertMatrixOpToComponentOps(ops[0]);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::inheritsTransform(const bool inherit)
{
  if(m_prim)
  {
    UsdGeomXformable(m_prim).SetResetXformStack(!inherit);
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool MayaTransformAPI::inheritsTransform() const
{
  if(!m_prim)
    return false;

  return !UsdGeomXformable(m_prim).GetResetXformStack();
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::scale(const GfVec3f& value, const UsdTimeCode& time)
{
  if(m_api != TransformAPI::kFallback && m_prim)
  {
    if(!m_scale)
      insertScaleOp();

    m_scale.Set(value, time);
  }
}

//----------------------------------------------------------------------------------------------------------------------
GfVec3f MayaTransformAPI::scale(const UsdTimeCode& time) const
{
  if(m_api != TransformAPI::kFallback)
  {
    if(m_scale)
    {
      GfVec3f value;
      m_scale.Get(&value, time);
      return value;
    }
    return GfVec3f(1.0f);
  }
  return _extractScaleFromMatrix(time);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::rotate(const GfVec3f& value, const RotationOrder order, const UsdTimeCode& time)
{
  const float radToDeg = 180.0f / 3.141592654f;
  if(m_api != TransformAPI::kFallback && m_prim)
  {
    if(!m_rotate)
    {
      insertRotateOp(order);
    }

    m_rotate.Set(value * radToDeg, time);
  }
}

//----------------------------------------------------------------------------------------------------------------------
GfVec3f MayaTransformAPI::rotate(const UsdTimeCode& time) const
{
  if(m_api != TransformAPI::kFallback)
  {
    const float degToRad = 3.141592654f / 180.0f;
    if(m_rotate)
    {
      GfVec3f value;
      m_rotate.Get(&value, time);
      return value * degToRad;
    }
    return GfVec3f(0);
  }
  return _extractRotateFromMatrix(time);
}

//----------------------------------------------------------------------------------------------------------------------
RotationOrder MayaTransformAPI::rotateOrder() const
{
  return m_order;
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::translate(const GfVec3d& value, const UsdTimeCode& time)
{
  if(m_api != TransformAPI::kFallback && m_prim)
  {
    if(!m_translate)
      insertTranslateOp();

    if(m_translate.GetPrecision() == UsdGeomXformOp::PrecisionDouble)
    {
      m_translate.Set(value, time);
    }
    else
    {
      m_translate.Set(GfVec3f(value), time);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
GfVec3d MayaTransformAPI::translate(const UsdTimeCode& time) const
{
  if(m_api != TransformAPI::kFallback)
  {
    if(m_translate)
    {
      if(m_translate.GetPrecision() == UsdGeomXformOp::PrecisionDouble)
      {
        GfVec3d value;
        m_translate.Get(&value, time);
        return value;
      }
      GfVec3f value;
      m_translate.Get(&value, time);
      return GfVec3d(value);
    }
    return GfVec3d(0);
  }
  return _extractTranslateFromMatrix(time);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::scalePivot(const GfVec3f& value, const UsdTimeCode& time)
{
  if(m_api != TransformAPI::kFallback && m_prim)
  {
    if(!m_scalePivot)
      insertScalePivotOp();

    m_scalePivot.Set(value, time);
  }
}

//----------------------------------------------------------------------------------------------------------------------
GfVec3f MayaTransformAPI::scalePivot(const UsdTimeCode& time) const
{
  if(m_api != TransformAPI::kFallback && m_scalePivot)
  {
    GfVec3f value;
    m_scalePivot.Get(&value, time);
    return value;
  }
  return GfVec3f(0, 0, 0);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::rotatePivot(const GfVec3f& value, const UsdTimeCode& time)
{
  if(m_api != TransformAPI::kFallback && m_prim)
  {
    if(m_api == TransformAPI::kMaya)
    {
      if(!m_rotatePivot)
        insertRotatePivotOp();
      m_rotatePivot.Set(value, time);
    }
    else
      m_pivot.Set(value, time);
  }
}

//----------------------------------------------------------------------------------------------------------------------
GfVec3f MayaTransformAPI::rotatePivot(const UsdTimeCode& time) const
{
  if(m_api != TransformAPI::kFallback)
  {
    if(m_rotatePivot)
    {
      GfVec3f value;
      m_rotatePivot.Get(&value, time);
      return value;
    }
    if(m_pivot)
    {
      GfVec3f value;
      m_pivot.Get(&value, time);
      return value;
    }
  }
  return GfVec3f(0);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::rotateAxis(const GfVec3f& value, const UsdTimeCode& time)
{
  const float radToDeg = 180.0f / 3.141592654f;
  if(m_api != TransformAPI::kFallback && m_prim)
  {
    if(!m_rotateAxis)
      insertRotateAxisOp();

    m_rotateAxis.Set(value * radToDeg, time);
  }
}

//----------------------------------------------------------------------------------------------------------------------
GfVec3f MayaTransformAPI::rotateAxis(const UsdTimeCode& time) const
{
  if(m_api != TransformAPI::kFallback)
  {
    const float degToRad = 3.141592654f / 180.0f;
    if(m_rotateAxis)
    {
      GfVec3f value;
      m_rotateAxis.Get(&value, time);
      return value * degToRad;
    }
  }
  return GfVec3f(0);
}



//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::scalePivotTranslate(const GfVec3f& value, const UsdTimeCode& time)
{
  if(m_api != TransformAPI::kFallback && m_prim)
  {
    if(!m_scalePivotTranslate)
      insertScalePivotTranslateOp();

    m_scalePivotTranslate.Set(value, time);
  }
}

//----------------------------------------------------------------------------------------------------------------------
GfVec3f MayaTransformAPI::scalePivotTranslate(const UsdTimeCode& time) const
{
  if(m_api != TransformAPI::kFallback)
  {
    if(m_scalePivotTranslate)
    {
      GfVec3f value;
      m_scalePivotTranslate.Get(&value, time);
      return value;
    }
  }
  return GfVec3f(0);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::rotatePivotTranslate(const GfVec3f& value, const UsdTimeCode& time)
{
  if(m_api != TransformAPI::kFallback && m_prim)
  {
    if(!m_rotatePivotTranslate)
      insertRotatePivotTranslateOp();

    m_rotatePivotTranslate.Set(value, time);
  }
}

//----------------------------------------------------------------------------------------------------------------------
GfVec3f MayaTransformAPI::rotatePivotTranslate(const UsdTimeCode& time) const
{
  if(m_api != TransformAPI::kFallback)
  {
    if(m_rotatePivotTranslate)
    {
      GfVec3f value;
      m_rotatePivotTranslate.Get(&value, time);
      return value;
    }
  }
  return GfVec3f(0);
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::insertScaleOp()
{
  if(!m_scale)
  {
    m_scale = UsdGeomXformable(m_prim).AddScaleOp();
    rebuildXformOrder();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::insertRotateOp(const RotationOrder order)
{
  m_order = order;
  switch(order)
  {
  case RotationOrder::kXYZ: m_rotate = UsdGeomXformable(m_prim).AddRotateXYZOp(); break;
  case RotationOrder::kXZY: m_rotate = UsdGeomXformable(m_prim).AddRotateXZYOp(); break;
  case RotationOrder::kYXZ: m_rotate = UsdGeomXformable(m_prim).AddRotateYXZOp(); break;
  case RotationOrder::kYZX: m_rotate = UsdGeomXformable(m_prim).AddRotateYZXOp(); break;
  case RotationOrder::kZXY: m_rotate = UsdGeomXformable(m_prim).AddRotateZXYOp(); break;
  case RotationOrder::kZYX: m_rotate = UsdGeomXformable(m_prim).AddRotateZYXOp(); break;
  }
  rebuildXformOrder();
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::insertRotateAxisOp()
{
  if(!m_rotateAxis)
  {
    m_rotateAxis = UsdGeomXformable(m_prim).AddRotateXYZOp(UsdGeomXformOp::PrecisionFloat);
    rebuildXformOrder();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::insertTranslateOp()
{
  if(!m_translate)
  {
    m_translate = UsdGeomXformable(m_prim).AddTranslateOp(UsdGeomXformOp::PrecisionDouble);
    rebuildXformOrder();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::insertScalePivotOp()
{
  if(!m_scalePivot)
  {
    m_scalePivot = UsdGeomXformable(m_prim).AddTranslateOp(UsdGeomXformOp::PrecisionFloat, tokens::scalePivotName);
    m_scalePivotINV = UsdGeomXformable(m_prim).AddTranslateOp(UsdGeomXformOp::PrecisionFloat, tokens::scalePivotName, true);
    rebuildXformOrder();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::insertScalePivotTranslateOp()
{
  if(!m_scalePivotTranslate)
  {
    m_scalePivotTranslate = UsdGeomXformable(m_prim).AddTranslateOp(UsdGeomXformOp::PrecisionFloat, tokens::scalePivotTranslateName);
    rebuildXformOrder();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::insertRotatePivotOp()
{
  if(!m_rotatePivot)
  {
    m_rotatePivot = UsdGeomXformable(m_prim).AddTranslateOp(UsdGeomXformOp::PrecisionFloat, tokens::rotatePivotName);
    m_rotatePivotINV = UsdGeomXformable(m_prim).AddTranslateOp(UsdGeomXformOp::PrecisionFloat, tokens::rotatePivotName, true);
    rebuildXformOrder();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::insertRotatePivotTranslateOp()
{
  if(!m_rotatePivotTranslate)
  {
    m_rotatePivotTranslate = UsdGeomXformable(m_prim).AddTranslateOp(UsdGeomXformOp::PrecisionFloat, tokens::rotatePivotTranslateName);
    rebuildXformOrder();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::rebuildXformOrder()
{
  std::vector<UsdGeomXformOp> ops;
  ops.reserve(11);

  if(m_translate) ops.push_back(m_translate);
  if(m_rotatePivotTranslate) ops.push_back(m_rotatePivotTranslate);
  if(m_rotatePivot) ops.push_back(m_rotatePivot);
  if(m_rotate) ops.push_back(m_rotate);
  if(m_rotateAxis) ops.push_back(m_rotateAxis);
  if(m_rotatePivotINV) ops.push_back(m_rotatePivotINV);
  if(m_scalePivot) ops.push_back(m_scalePivot);
  if(m_scalePivotTranslate) ops.push_back(m_scalePivotTranslate);
  if(m_shear) ops.push_back(m_shear);
  if(m_scale) ops.push_back(m_scale);
  if(m_scalePivotINV) ops.push_back(m_scalePivotINV);

  UsdGeomXformable(m_prim).SetXformOpOrder(ops, !inheritsTransform());
}

//----------------------------------------------------------------------------------------------------------------------
GfMatrix4d MayaTransformAPI::asMatrix(const UsdTimeCode& time) const
{
  GfMatrix4d m;
  bool b;
  if(m_prim)
  {
    UsdGeomXformable(m_prim).GetLocalTransformation(&m, &b, time);
    return m;
  }
  return m.SetIdentity();
}

//----------------------------------------------------------------------------------------------------------------------
void extractEuler(const GfVec3f mat[3], RotationOrder rotOrder, GfVec3f& rot)
{
  const float pi = 3.141592654f;
  const int mod3[6] = {0, 1, 2, 0, 1, 2};
  const int k1 = int(rotOrder) > 2 ? 2 : 1;
  const int k2 = 3 - k1;
  const int row = mod3[int(rotOrder)];
  const int col = mod3[k2 + row];
  const int colCos = mod3[col + k1];
  const int colSin = mod3[col + k2];
  const int rowSin = mod3[row + k1];
  const int rowCos = mod3[row + k2];
  const float s  = int(rotOrder) < 3 ? -1.0 : 1.0;

  const float epsilon = std::numeric_limits<float>::epsilon();
  if (std::fabs(mat[row][col] - 1) < epsilon)
  {
    rot[row] = std::atan2(s*mat[rowSin][colCos], mat[rowSin][colSin]);
    rot[rowSin] = s*pi/2;
    rot[rowCos] = 0;
  }
  else
  if (std::fabs(mat[row][col] + 1) < epsilon)
  {    
    rot[row] = std::atan2(-s*mat[rowSin][colCos], mat[rowSin][colSin]);
    rot[rowSin] = -s*pi/2;
    rot[rowCos] = 0;
  }
  else
  {
    rot[row] = std::atan2(-s*mat[rowSin][col], mat[rowCos][col] );
    rot[rowSin] = std::asin ( s*mat[row][col] );
    rot[rowCos] = std::atan2(-s*mat[row][colSin], mat[row][colCos] );
  }
}

//----------------------------------------------------------------------------------------------------------------------
static void rotate(const GfVec3f c, const GfVec3f p[3], GfVec3f& m)
{
  m[0] = p[0][0] * c[0];
  m[1] = p[0][1] * c[0];
  m[2] = p[0][2] * c[0];

  m[0] += p[1][0] * c[1];
  m[1] += p[1][1] * c[1];
  m[2] += p[1][2] * c[1];

  m[0] += p[2][0] * c[2];
  m[1] += p[2][1] * c[2];
  m[2] += p[2][2] * c[2];
}

//----------------------------------------------------------------------------------------------------------------------
static void transpose(GfVec3f m[3])
{
  float t0 = m[0][1];
  float t1 = m[0][2];
  float t2 = m[1][2];
  m[0][1] = m[1][0];
  m[0][2] = m[2][0];
  m[1][2] = m[2][1];
  m[1][0] = t0;
  m[2][0] = t1;
  m[2][1] = t2;
}

//----------------------------------------------------------------------------------------------------------------------
static void multiply(const GfVec3f c[3], const GfVec3f p[3], GfVec3f m[3])
{
  rotate(c[0], p, m[0]);
  rotate(c[1], p, m[1]);
  rotate(c[2], p, m[2]);
}

//----------------------------------------------------------------------------------------------------------------------
void eulerXYZtoMatrix(GfVec3f eulers, GfVec3f m[3])
{
  const float sx = std::sin(eulers[0]);
  const float cx = std::cos(eulers[0]);
  const float sy = std::sin(eulers[1]);
  const float cy = std::cos(eulers[1]);
  const float sz = std::sin(eulers[2]);
  const float cz = std::cos(eulers[2]);
  const float czsx = cz * sx;
  const float cxcz = cx * cz;
  const float sysz = sy * sz;
  m[0][0] =  cy * cz;
  m[0][1] =  cy * sz;
  m[0][2] =  -sy;
  m[1][0] =  czsx * sy - cx*sz;
  m[1][1] =  sysz * sx + cxcz;
  m[1][2] =  sx*cy;
  m[2][0] =  cxcz * sy + sx * sz;
  m[2][1] =  sysz * cx - czsx;
  m[2][2] =  cx*cy;
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::setFromMatrix(const GfMatrix4d& matrix, const UsdTimeCode& time)
{
  GfVec3f m[3] = { 
    GfVec3f(matrix[0][0], matrix[0][1], matrix[0][2]),
    GfVec3f(matrix[1][0], matrix[1][1], matrix[1][2]),
    GfVec3f(matrix[2][0], matrix[2][1], matrix[2][2])
  };

  GfVec3f& rx = m[0];
  GfVec3f& ry = m[1];
  GfVec3f& rz = m[2];

  // extract and remove the scaling.
  float sx = rx.Normalize();
  float sy = ry.Normalize();
  float sz = rz.Normalize();

  // Do we have a negative scaling?
  if(GfDot(GfCross(rx, ry), rz) < 0)
  {
    sz = -sz;
    rz = -rz;
  }
  scale(GfVec3f(sx, sy, sz), time);
  GfVec3d t(matrix[3][0], matrix[3][1], matrix[3][2]);

  // use previous rotation order to set the euler angles
  RotationOrder order = rotateOrder();

  if(m_rotateAxis)
  {
    // grab the rotate axis
    GfVec3f rotAxis = rotateAxis(time);

    // convert to euler matrix and invert
    GfVec3f mrotAxis[3], M[3];
    eulerXYZtoMatrix(rotAxis, mrotAxis);

    // remove effect of rotation axis from rotation. 
    transpose(mrotAxis);
    multiply(mrotAxis, m, M);
    
    // now convert remaining rotation to euler and set.
    GfVec3f rot;
    extractEuler(M, order, rot);
    rotate(rot, order, time);
  }
  else
  {
    // no rotation axis present, just extract the euler rotation.
    GfVec3f rot;
    extractEuler(m, order, rot);
    rotate(rot, order, time);
  }

  // now what remains is the removal of the scale and rotate pivot values from the translation. 
  // effectively we need to evaluate the following transform:
  // 
  //   [SpINV] x [S] x [Sh] x [Sp] x [St] x [RpINV]
  // 
  // The resulting translation can then be rotated, and removed from the translation values
  //
  if(!m_scalePivot && !m_rotatePivot)
  {
    translate(t, time);
  }
  else
  if(!m_scalePivot && m_rotatePivot)
  {
    // 'm' is currently our orientation matrix. 
    // The inverted rotate pivot is the only translation affected by rotation in this case,
    // so transform it, and remove from the translation.
    GfVec3f rp = rotatePivot(time), orp;
    AL::usd::utils::rotate(-rp, m, orp);
    t -= rotatePivotTranslate(time);
    t -= rp;
    t -= orp;
    translate(t, time);
  }
  else
  if(m_scalePivot && !m_rotatePivot)
  {
    GfVec3f sp = scalePivot(time);
    GfVec3f spt = scalePivotTranslate(time);

    // scalePivot inverted, and affected by scale
    GfVec3f R = -sp;
    R[0] *= sx;
    R[1] *= sy;
    R[2] *= sz;

    R += sp;
    R += spt;

    // 'm' is currently our orientation matrix. 
    // The inverted rotate pivot is the only translation affected by rotation in this case,
    // so transform it, and remove from the translation.
    GfVec3f orp;
    AL::usd::utils::rotate(R, m, orp);
    t -= orp;

    translate(t, time);
  }
  else
  if(m_scalePivot && m_rotatePivot)
  {
    GfVec3f rp = rotatePivot(time);
    GfVec3f rpt = rotatePivotTranslate(time);
    GfVec3f sp = scalePivot(time);
    GfVec3f spt = scalePivotTranslate(time);

    // scalePivot inverted, and affected by scale
    GfVec3f R = -sp;
    R[0] *= sx;
    R[1] *= sy;
    R[2] *= sz;

    R += sp;
    R += spt;
    R -= rp;

    // 'm' is currently our orientation matrix. 
    // The inverted rotate pivot is the only translation affected by rotation in this case,
    // so transform it, and remove from the translation.
    GfVec3f orp;
    AL::usd::utils::rotate(R, m, orp);
    t -= rpt;
    t -= rp;
    t -= orp;

    translate(t, time);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MayaTransformAPI::convertMatrixOpToComponentOps(const UsdGeomXformOp& op)
{
  std::vector<double> times;
  op.GetTimeSamples(&times);
  m_api = TransformAPI::kMaya;

  // set up the default values
  {
    GfMatrix4d m;
    op.Get(&m, UsdTimeCode::Default());
    setFromMatrix(m, UsdTimeCode::Default());
  }

  if(!times.empty())
  {
    for(auto t : times)
    {
      GfMatrix4d m;
      op.Get(&m, UsdTimeCode(t));
      setFromMatrix(m, UsdTimeCode(t));
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
GfVec3f MayaTransformAPI::_extractScaleFromMatrix(const UsdTimeCode time) const
{
  UsdGeomXformable xform(m_prim);
  GfMatrix4d transform;
  bool resetsXformStack;
  if(xform.GetLocalTransformation(&transform, &resetsXformStack, time))
  {
    GfVec3f m[3] = { 
      GfVec3f(transform[0][0], transform[0][1], transform[0][2]),
      GfVec3f(transform[1][0], transform[1][1], transform[1][2]),
      GfVec3f(transform[2][0], transform[2][1], transform[2][2])
    };

    GfVec3f& rx = m[0];
    GfVec3f& ry = m[1];
    GfVec3f& rz = m[2];

    // extract and remove the scaling.
    GfVec3f s(rx.Normalize(), ry.Normalize(), rz.Normalize());

    // Do we have a negative scaling?
    if(GfDot(GfCross(rx, ry), rz) < 0)
    {
      s[2] = -s[2];
    }
    return s;
  }
  return GfVec3f(1.0f);
}

//----------------------------------------------------------------------------------------------------------------------
GfVec3f MayaTransformAPI::_extractRotateFromMatrix(const UsdTimeCode time) const
{
  UsdGeomXformable xform(m_prim);
  GfMatrix4d transform;
  bool resetsXformStack;
  if(xform.GetLocalTransformation(&transform, &resetsXformStack, time))
  {
    GfVec3f m[3] = { 
      GfVec3f(transform[0][0], transform[0][1], transform[0][2]),
      GfVec3f(transform[1][0], transform[1][1], transform[1][2]),
      GfVec3f(transform[2][0], transform[2][1], transform[2][2])
    };

    GfVec3f& rx = m[0];
    GfVec3f& ry = m[1];
    GfVec3f& rz = m[2];

    // remove the scaling.
    rx.Normalize();
    ry.Normalize();
    rz.Normalize();

    // Do we have a negative scaling?
    if(GfDot(GfCross(rx, ry), rz) < 0)
    {
      rz = -rz;
    }
    GfVec3f rotation;
    extractEuler(m, RotationOrder::kXYZ, rotation);
    return rotation;
  }
  return GfVec3f(0);
}

//----------------------------------------------------------------------------------------------------------------------
GfVec3d MayaTransformAPI::_extractTranslateFromMatrix(const UsdTimeCode time) const
{
  UsdGeomXformable xform(m_prim);
  GfMatrix4d transform;
  bool resetsXformStack;
  if(xform.GetLocalTransformation(&transform, &resetsXformStack, time))
  {
    return GfVec3d(transform[3][0], transform[3][1], transform[3][2]);
  }
  return GfVec3d(0.0);
}

//----------------------------------------------------------------------------------------------------------------------
} // utils
} // usd
} // AL
//----------------------------------------------------------------------------------------------------------------------