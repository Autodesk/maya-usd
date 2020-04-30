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
#pragma once

#include "Api.h"
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/matrix4d.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xformable.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MayaUsdUtils {

#ifndef AL_SUPPORT_LEGACY_NAMES
#define AL_SUPPORT_LEGACY_NAMES 1
#endif

//----------------------------------------------------------------------------------------------------------------------
/// \brief  defines the euler rotation order
//----------------------------------------------------------------------------------------------------------------------
enum class RotationOrder
{
  kXYZ, 
  kYZX, 
  kZXY, 
  kXZY, 
  kYXZ, 
  kZYX
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  defines the euler rotation order
//----------------------------------------------------------------------------------------------------------------------
enum class TransformAPI
{
  kFallback, ///< no transform profile could be matched
  kCommon,   ///< transform matches the pixar common profile
  kMaya,     ///< transform matches the maya transform profile (i.e. "transform" node)
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  This class provides an interface to simplify the general case 
//----------------------------------------------------------------------------------------------------------------------
class MayaTransformAPI
{
public:

  /// \brief  ctor. This will check to see whether the xform op order of the prim specified matches a maya xform. 
  ///         If it does, this class allows for easy manipulation of that USD transform object.
  ///         If convertMatrixOpToComponentOps is true, and there is a single matrix xform op defining the transform,
  ///         then the matrix xform will be converted to separate TRS components. 
  /// \param  prim the prim we wish to manipulate
  /// \param  convertMatrixOpToComponentOps if true, and if the prim contains a single matrix transform, then 
  ///         the transform op will be converted to TRS data. 
  MAYA_USD_UTILS_PUBLIC
  MayaTransformAPI(UsdPrim prim, bool convertMatrixOpToComponentOps = false);

  /// \brief  dtor
  MAYA_USD_UTILS_PUBLIC
  ~MayaTransformAPI() = default;

  /// \brief  applies a scaling at the specified time code
  /// \param  scale value of the scaling at the specified time
  /// \param  time the time at which to set the scaling
  MAYA_USD_UTILS_PUBLIC
  void scale(const GfVec3f& scale, const UsdTimeCode& time = UsdTimeCode::Default());

  /// \brief  applies a rotateAxis value at the specified time code
  /// \param  rotateAxis value of the rotateAxis at the specified time
  /// \param  time the time at which to set the rotateAxis
  MAYA_USD_UTILS_PUBLIC
  void rotateAxis(const GfVec3f& rotateAxis, const UsdTimeCode& time = UsdTimeCode::Default());

  /// \brief  applies a rotation value at the specified time code
  /// \param  order the rotation order to use
  /// \param  rotation value of the rotation at the specified time
  /// \param  time the time at which to set the rotation
  MAYA_USD_UTILS_PUBLIC
  void rotate(const GfVec3f& rotation, RotationOrder order, const UsdTimeCode& time = UsdTimeCode::Default());

  /// \brief  applies a translation value at the specified time code
  /// \param  translation value of the translation at the specified time
  /// \param  time the time at which to set the translation
  MAYA_USD_UTILS_PUBLIC
  void translate(const GfVec3d& translation, const UsdTimeCode& time = UsdTimeCode::Default());

  /// \brief  applies a scalePivot value at the specified time code
  /// \param  scalePivot value of the scalePivot at the specified time
  /// \param  time the time at which to set the scalePivot
  MAYA_USD_UTILS_PUBLIC
  void scalePivot(const GfVec3f& scalePivot, const UsdTimeCode& time = UsdTimeCode::Default());

  /// \brief  applies a rotatePivot value at the specified time code
  /// \param  rotatePivot value of the rotatePivot at the specified time
  /// \param  time the time at which to set the rotatePivot
  MAYA_USD_UTILS_PUBLIC
  void rotatePivot(const GfVec3f& rotatePivot, const UsdTimeCode& time = UsdTimeCode::Default());

  /// \brief  applies a scalePivotTranslate value at the specified time code
  /// \param  scalePivotTranslate value of the scalePivotTranslate at the specified time
  /// \param  time the time at which to set the scalePivotTranslate
  MAYA_USD_UTILS_PUBLIC
  void scalePivotTranslate(const GfVec3f& scalePivotTranslate, const UsdTimeCode& time = UsdTimeCode::Default());

  /// \brief  applies a rotatePivotTranslate value at the specified time code
  /// \param  rotatePivotTranslate value of the rotatePivotTranslate at the specified time
  /// \param  time the time at which to set the rotatePivotTranslate
  MAYA_USD_UTILS_PUBLIC
  void rotatePivotTranslate(const GfVec3f& rotatePivotTranslate, const UsdTimeCode& time = UsdTimeCode::Default());

  /// \brief  sets whether the parent transforms affect this UsdGeomXform. 
  /// \param  inherit sets the resetStack parameter within UsdGeomXform
  MAYA_USD_UTILS_PUBLIC
  void inheritsTransform(bool inherit);

  /// \brief  returns the scaling at the specified time code
  /// \param  time the time value to query the UsdGeomXform
  /// \return the scaling at the specified time code
  MAYA_USD_UTILS_PUBLIC
  GfVec3f scale(const UsdTimeCode& time = UsdTimeCode::Default()) const;

  /// \brief  returns the rotation at the specified time code
  /// \param  time the time value to query the UsdGeomXform
  /// \return the rotation at the specified time code
  MAYA_USD_UTILS_PUBLIC
  GfVec3f rotate(const UsdTimeCode& time = UsdTimeCode::Default()) const;

  /// \brief  returns the rotateAxis at the specified time code
  /// \param  time the time value to query the UsdGeomXform
  /// \return the rotateAxis at the specified time code
  MAYA_USD_UTILS_PUBLIC
  GfVec3f rotateAxis(const UsdTimeCode& time = UsdTimeCode::Default()) const;

  /// \brief  returns the rotationOrder of the rotation xform op
  /// \return the rotation order
  MAYA_USD_UTILS_PUBLIC
  RotationOrder rotateOrder() const;

  /// \brief  returns the translation at the specified time code
  /// \param  time the time value to query the UsdGeomXform
  /// \return the translation at the specified time code
  MAYA_USD_UTILS_PUBLIC
  GfVec3d translate(const UsdTimeCode& time = UsdTimeCode::Default()) const;

  /// \brief  returns the scalePivot at the specified time code
  /// \param  time the time value to query the UsdGeomXform
  /// \return the scalePivot at the specified time code
  MAYA_USD_UTILS_PUBLIC
  GfVec3f scalePivot(const UsdTimeCode& time = UsdTimeCode::Default()) const;

  /// \brief  returns the rotatePivot at the specified time code
  /// \param  time the time value to query the UsdGeomXform
  /// \return the rotatePivot at the specified time code
  MAYA_USD_UTILS_PUBLIC
  GfVec3f rotatePivot(const UsdTimeCode& time = UsdTimeCode::Default()) const;

  /// \brief  returns the scalePivotTranslate at the specified time code
  /// \param  time the time value to query the UsdGeomXform
  /// \return the scalePivotTranslate at the specified time code
  MAYA_USD_UTILS_PUBLIC
  GfVec3f scalePivotTranslate(const UsdTimeCode& time = UsdTimeCode::Default()) const;

  /// \brief  returns the rotatePivotTranslate at the specified time code
  /// \param  time the time value to query the UsdGeomXform
  /// \return the rotatePivotTranslate at the specified time code
  MAYA_USD_UTILS_PUBLIC
  GfVec3f rotatePivotTranslate(const UsdTimeCode& time = UsdTimeCode::Default()) const;

  /// \brief  returns true if the parent transforms affect this UsdGeomXform
  /// \return true if this xform is influenced by the parent transform
  MAYA_USD_UTILS_PUBLIC
  bool inheritsTransform() const;

  /// \brief  evaluates the matrix at the specified time code
  /// \param  time the time at which to evaluate the transform
  /// \return the local space matrix for this prim
  MAYA_USD_UTILS_PUBLIC
  GfMatrix4d asMatrix(const UsdTimeCode& time = UsdTimeCode::Default()) const;

  /// \brief  sets the current transform from a matrix at the specified time. The matrix will be decomposed
  ///         and set as TRS components. 
  /// \param  matrix the new local space matrix for the prim
  /// \param  time the time code at which to set the transform
  MAYA_USD_UTILS_PUBLIC
  void setFromMatrix(const GfMatrix4d& matrix, const UsdTimeCode& time = UsdTimeCode::Default());

  /// \brief  checks to see if the transform can be manipulated by this class. 
  /// \return If this method returns true, then this class can manipulate the transform. 
  ///         If this returns false, we cannot modify the transform (i.e. it doesn't match the maya xform profile,
  ///         or the prim contains a single matrix transform, but convertMatrixOpToComponentOps was set to false)
  operator bool () const 
    { return isValid(); }

  /// \brief  checks to see if the transform can be manipulated by this class. 
  /// \return If this method returns true, then this class can manipulate the transform. 
  ///         If this returns false, we cannot modify the transform (i.e. it doesn't match the maya xform profile,
  ///         or the prim contains a single matrix transform, but convertMatrixOpToComponentOps was set to false)
  bool isValid() const 
    { return bool(m_prim); }

  /// \brief  returns the API this prim is compatible with
  /// \return the transform API that was found
  TransformAPI api() const
    { return m_api; }

protected:
  void convertMatrixOpToComponentOps(const UsdGeomXformOp& op);
  bool initializeMayaTransformProfile(const std::vector<UsdGeomXformOp>& orderedOps);
  void rebuildXformOrder();
  void insertScaleOp();  
  void insertRotateOp(RotationOrder order);  
  void insertTranslateOp(); 
  void insertScalePivotOp();  
  void insertRotateAxisOp();  
  void insertScalePivotTranslateOp(); 
  void insertRotatePivotOp();  
  void insertRotatePivotTranslateOp();
private:
  GfVec3f _extractScaleFromMatrix(const UsdTimeCode time) const;
  GfVec3f _extractRotateFromMatrix(const UsdTimeCode time) const;
  GfVec3d _extractTranslateFromMatrix(const UsdTimeCode time) const;
  bool _initializeMayaTransformProfile(const std::vector<UsdGeomXformOp>& orderedOps);
  UsdPrim m_prim;
  UsdGeomXformOp m_pivot = UsdGeomXformOp();
  UsdGeomXformOp m_pivotINV = UsdGeomXformOp();
  UsdGeomXformOp m_scale = UsdGeomXformOp();
  UsdGeomXformOp m_rotate = UsdGeomXformOp();
  UsdGeomXformOp m_rotateAxis = UsdGeomXformOp();
  UsdGeomXformOp m_translate = UsdGeomXformOp();
  UsdGeomXformOp m_scalePivot = UsdGeomXformOp();
  UsdGeomXformOp m_scalePivotINV = UsdGeomXformOp();
  UsdGeomXformOp m_scalePivotTranslate = UsdGeomXformOp();
  UsdGeomXformOp m_rotatePivot = UsdGeomXformOp();
  UsdGeomXformOp m_rotatePivotINV = UsdGeomXformOp();
  UsdGeomXformOp m_rotatePivotTranslate = UsdGeomXformOp();
  UsdGeomXformOp m_shear = UsdGeomXformOp();
  RotationOrder m_order = RotationOrder::kXYZ;
  TransformAPI m_api = TransformAPI::kMaya;
};

MAYA_USD_UTILS_PUBLIC
void eulerXYZtoMatrix(GfVec3f eulers, GfVec3f m[3]);

//----------------------------------------------------------------------------------------------------------------------
} // MayaUsdUtils
//----------------------------------------------------------------------------------------------------------------------
