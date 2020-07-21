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
#include <pxr/base/tf/token.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <immintrin.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MayaUsdUtils {

//----------------------------------------------------------------------------------------------------------------------------------------------------------
class alignas(32) TransformOpProcessor
{
public:

  // when processing matrix transform ops, the coordinate frame for the manipulator will change 
  // depending on whether we are setting up for scale, rotatation, or translation
  enum ManipulatorMode
  {
    kTranslate, 
    kRotate, 
    kScale,
    kGuess //< for most ops, this will just work. For matrix ops, you'll need to be more specific
  };
  enum Space
  {
    kTransform,
    kWorld,
    kParent
  };

  MAYA_USD_UTILS_PUBLIC
  TransformOpProcessor(const UsdPrim prim, const TfToken opName, ManipulatorMode mode = kGuess, const UsdTimeCode& tc = UsdTimeCode::Default());

  MAYA_USD_UTILS_PUBLIC
  TransformOpProcessor(const UsdPrim prim, const uint32_t opIndex, ManipulatorMode mode = kGuess, const UsdTimeCode& tc = UsdTimeCode::Default());

  /// re-evaluate the internal coordinate frames on time change
  MAYA_USD_UTILS_PUBLIC
  void UpdateToTime(const UsdTimeCode& tc, UsdGeomXformCache& cache, ManipulatorMode mode = kGuess);
  void UpdateToTime(const UsdTimeCode& tc, ManipulatorMode mode = kGuess) { UsdGeomXformCache cache; UpdateToTime(tc, cache, mode); }

  //--------------------------------------------------------------------------------------------------------------------------------------------------------
  // given the xform op currently assigned to this processor, can we scale, rotate, and/or translate the op? (In some cases, e.g. matrices, all may be supported)
  //--------------------------------------------------------------------------------------------------------------------------------------------------------

  /// returns true if the current xfrom op can be rotated
  MAYA_USD_UTILS_PUBLIC
  bool CanRotate() const;

  /// returns true if the current xfrom op can be rotated in the local x axis
  bool CanRotateX() const
    { return CanRotate() && op().GetOpType() != UsdGeomXformOp::TypeRotateY && op().GetOpType() != UsdGeomXformOp::TypeRotateZ; }

  /// returns true if the current xfrom op can be rotated in the local y axis
  bool CanRotateY() const
    { return CanRotate() && op().GetOpType() != UsdGeomXformOp::TypeRotateX && op().GetOpType() != UsdGeomXformOp::TypeRotateZ; }

  /// returns true if the current xfrom op can be rotated in the local z axis
  bool CanRotateZ() const
    { return CanRotate() && op().GetOpType() != UsdGeomXformOp::TypeRotateX && op().GetOpType() != UsdGeomXformOp::TypeRotateY; }

  /// returns true if the current xfrom op can be translated
  MAYA_USD_UTILS_PUBLIC
  bool CanTranslate() const;

  /// returns true if the current xfrom op can be scaled
  MAYA_USD_UTILS_PUBLIC
  bool CanScale() const;

  //--------------------------------------------------------------------------------------------------------------------------------------------------------
  // Compute the current transform op value - all values in local space
  //--------------------------------------------------------------------------------------------------------------------------------------------------------

  /// returns the current orientation as a quat (If CanRotate() returns false, the identity quat is returned)
  MAYA_USD_UTILS_PUBLIC
  GfQuatd Rotation() const;

  /// returns the current translation as a vec3 (If CanTranslate() returns false, [0,0,0] is returned)
  MAYA_USD_UTILS_PUBLIC
  GfVec3d Translation() const;

  /// returns the current scale as a vec3 (If CanScale() returns false, [1,1,1] is returned)
  MAYA_USD_UTILS_PUBLIC
  GfVec3d Scale() const;

  //--------------------------------------------------------------------------------------------------------------------------------------------------------
  // Compute the current transform op value. 
  //--------------------------------------------------------------------------------------------------------------------------------------------------------

  /// returns the coordinate frame for this manipulator where the transformation is the identity (e.g. the local origin)
  inline const GfMatrix4d& ManipulatorFrame() const
    { return _coordFrame; }

  /// returns the inclusive matrix of the manipulator frame, and the transformation of the xform op applied 
  inline GfMatrix4d ManipulatorMatrix() const
    { return EvaluateCoordinateFrameForIndex(_ops, _opIndex + 1, _timeCode); }

  /// returns the current manipulator mode
  MAYA_USD_UTILS_PUBLIC
  ManipulatorMode ManipMode() const;

  //--------------------------------------------------------------------------------------------------------------------------------------------------------
  // Apply relative transformations to the Transform Op
  //--------------------------------------------------------------------------------------------------------------------------------------------------------

  /// apply a translation offset to the xform op
  MAYA_USD_UTILS_PUBLIC
  bool Translate(const GfVec3d& translateChange, Space space = kTransform);

  /// apply a scale offset to the xform op
  MAYA_USD_UTILS_PUBLIC
  bool Scale(const GfVec3d& scaleChange, Space space = kTransform);

  /// apply a rotational offset to the X axis
  MAYA_USD_UTILS_PUBLIC
  bool RotateX(const double radianChange, Space space = kTransform);

  /// apply a rotational offset to the Y axis
  MAYA_USD_UTILS_PUBLIC
  bool RotateY(const double radianChange, Space space = kTransform);

  /// apply a rotational offset to the Z axis
  MAYA_USD_UTILS_PUBLIC
  bool RotateZ(const double radianChange, Space space = kTransform);

  /// apply a rotational offset to the xform op. 
  /// NOTE: This is primarily useful for rotating objects via the sphere (rather than axis rings of the rotate manip)
  /// It's likely that using this method won't result in 'nice' eulers afterwards. 
  /// If you want 'nice' eulers (as much as is possible with a rotate tool), then prefer to use the axis rotation 
  /// methods, RotateX etc. 
  /// It should also be noted that this method may end up being called by the RotateX/RotateY/RotateZ methods if 
  /// either the rotation is not a simple one - i.e. a simple RotateX xform op 
  MAYA_USD_UTILS_PUBLIC
  bool Rotate(const GfQuatd& quatChange, Space space);

  //--------------------------------------------------------------------------------------------------------------------------------------------------------
  // Coordinate frames for manipulators
  //--------------------------------------------------------------------------------------------------------------------------------------------------------

  /// return the coordinate frame for the transform op - i.e. the 'origin' for the manipulator
  const GfMatrix4d& WorldFrame() const 
    { return _worldFrame; }

  /// return the coordinate frame for the transform op - i.e. the 'origin' for the manipulator
  const GfMatrix4d& CoordinateFrame() const 
    { return _coordFrame; }

  /// given some list of UsdGeomXformOp's, evaluate the coordinate frame needed for the op at the given index. 
  /// This does not evaluate the xform op at that index (i.e. If the first op in ops is a translate, then requesting
  /// index zero will return the identity)
  /// I should really extract this method and pass to Pixar. The code is more performant than UsdGeomXformable::GetLocalTransformation.
  MAYA_USD_UTILS_PUBLIC
  static GfMatrix4d EvaluateCoordinateFrameForIndex(const std::vector<UsdGeomXformOp>& ops, uint32_t index, const UsdTimeCode& timeCode);

protected:
  // helper methods to extract scale/rotation/translation from the transform op
  static __m256d _Scale(const UsdGeomXformOp& op, const UsdTimeCode& timeCode);
  static __m256d _Rotation(const UsdGeomXformOp& op, const UsdTimeCode& timeCode);
  static __m256d _Translation(const UsdGeomXformOp& op, const UsdTimeCode& timeCode);
  GfMatrix4d _coordFrame;
  GfMatrix4d _worldFrame;
  GfMatrix4d _invCoordFrame;
  GfMatrix4d _invWorldFrame;
  __m256d _qcoordFrame;
  __m256d _qworldFrame;
  __m256d _qparentFrame;
  std::vector<UsdGeomXformOp> _ops;
  uint32_t _opIndex;
  UsdTimeCode _timeCode = UsdTimeCode::Default();
  UsdPrim _prim;
  ManipulatorMode _manipMode;
  bool _resetsXformStack = false;
  UsdGeomXformOp op() const { return _ops[_opIndex]; }
};

//----------------------------------------------------------------------------------------------------------------------------------------------------------
/// All methods in TransformOpProcessor deal with relative offsets. This class extends the base and adds support to set xform ops to specific positions
/// and orientations. All methods in this class are implemented using methods from the base class.  
//----------------------------------------------------------------------------------------------------------------------------------------------------------
class TransformOpProcessorEx : public TransformOpProcessor
{
public:

  TransformOpProcessorEx(const UsdPrim prim, const TfToken opName, ManipulatorMode mode = kGuess, const UsdTimeCode& tc = UsdTimeCode::Default())
    : TransformOpProcessor(prim, opName, mode, tc) {}

  TransformOpProcessorEx(const UsdPrim prim, const uint32_t opIndex, ManipulatorMode mode = kGuess, const UsdTimeCode& tc = UsdTimeCode::Default())
    : TransformOpProcessor(prim, opIndex, mode, tc) {}

  /// set the translate value on the translate op xform op
  MAYA_USD_UTILS_PUBLIC
  bool SetTranslate(const GfVec3d& position, Space space = kTransform);

  /// set the scale value on the xform op
  MAYA_USD_UTILS_PUBLIC
  bool SetScale(const GfVec3d& scale, Space space = kTransform);

  /// set transform op to world space orientation
  MAYA_USD_UTILS_PUBLIC
  bool SetRotate(const GfQuatd& orientation, Space space = kTransform);

  //--------------------------------------------------------------------------------------------------------------------------------------------------------
  // static 'one-hit' versions
  //--------------------------------------------------------------------------------------------------------------------------------------------------------

  /// apply a translation offset to the xform op
  MAYA_USD_UTILS_PUBLIC
  static bool Translate(UsdPrim prim, TfToken rotateAttr, UsdTimeCode timeCode, const GfVec3d& translateChange, Space space = kTransform);

  /// apply a scale offset to the xform op
  MAYA_USD_UTILS_PUBLIC
  static bool Scale(UsdPrim prim, TfToken rotateAttr, UsdTimeCode timeCode, const GfVec3d& scaleChange, Space space = kTransform);

  /// apply a rotational offset to the X axis
  MAYA_USD_UTILS_PUBLIC
  static bool RotateX(UsdPrim prim, TfToken rotateAttr, UsdTimeCode timeCode, const double radianChange, Space space = kTransform);

  /// apply a rotational offset to the Y axis
  MAYA_USD_UTILS_PUBLIC
  static bool RotateY(UsdPrim prim, TfToken rotateAttr, UsdTimeCode timeCode, const double radianChange, Space space = kTransform);

  /// apply a rotational offset to the Z axis
  MAYA_USD_UTILS_PUBLIC
  static bool RotateZ(UsdPrim prim, TfToken rotateAttr, UsdTimeCode timeCode, const double radianChange, Space space = kTransform);

  /// apply a rotational offset to the xform op. 
  /// NOTE: This is primarily useful for rotating objects via the sphere (rather than axis rings of the rotate manip)
  /// It's likely that using this method won't result in 'nice' eulers afterwards. 
  /// If you want 'nice' eulers (as much as is possible with a rotate tool), then prefer to use the axis rotation 
  /// methods, RotateX etc. 
  /// It should also be noted that this method may end up being called by the RotateX/RotateY/RotateZ methods if 
  /// either the rotation is not a simple one - i.e. a simple RotateX xform op 
  MAYA_USD_UTILS_PUBLIC
  static bool Rotate(UsdPrim prim, TfToken rotateAttr, UsdTimeCode timeCode, const GfQuatd& quatChange, Space space);

  /// set the translate value on the translate op xform op
  MAYA_USD_UTILS_PUBLIC
  static bool SetTranslate(UsdPrim prim, TfToken rotateAttr, UsdTimeCode timeCode, const GfVec3d& position, Space space = kTransform);

  /// set the scale value on the xform op
  MAYA_USD_UTILS_PUBLIC
  static bool SetScale(UsdPrim prim, TfToken rotateAttr, UsdTimeCode timeCode, const GfVec3d& scale, Space space = kTransform);

  /// set transform op to world space orientation
  MAYA_USD_UTILS_PUBLIC
  static bool SetRotate(UsdPrim prim, TfToken rotateAttr, UsdTimeCode timeCode, const GfQuatd& orientation, Space space = kTransform);

  //--------------------------------------------------------------------------------------------------------------------------------------------------------
  // The static version of Rotate/Translate etc, end up hiding the base class implementations. inline them here. 
  //--------------------------------------------------------------------------------------------------------------------------------------------------------

  /// apply a translation offset to the xform op
  inline
  bool Translate(const GfVec3d& translateChange, Space space = kTransform)
    { return TransformOpProcessor::Translate(translateChange, space); }

  /// apply a scale offset to the xform op
  inline
  bool Scale(const GfVec3d& scaleChange, Space space = kTransform)
    { return TransformOpProcessor::Scale(scaleChange, space); }

  /// apply a rotational offset to the X axis
  inline
  bool RotateX(const double radianChange, Space space = kTransform)
    { return TransformOpProcessor::RotateX(radianChange, space); }

  /// apply a rotational offset to the Y axis
  inline
  bool RotateY(const double radianChange, Space space = kTransform)
    { return TransformOpProcessor::RotateY(radianChange, space); }

  /// apply a rotational offset to the Z axis
  inline
  bool RotateZ(const double radianChange, Space space = kTransform)
    { return TransformOpProcessor::RotateZ(radianChange, space); }

  /// apply a rotational offset to the xform op. 
  /// NOTE: This is primarily useful for rotating objects via the sphere (rather than axis rings of the rotate manip)
  /// It's likely that using this method won't result in 'nice' eulers afterwards. 
  /// If you want 'nice' eulers (as much as is possible with a rotate tool), then prefer to use the axis rotation 
  /// methods, RotateX etc. 
  /// It should also be noted that this method may end up being called by the RotateX/RotateY/RotateZ methods if 
  /// either the rotation is not a simple one - i.e. a simple RotateX xform op 
  inline
  bool Rotate(const GfQuatd& quatChange, Space space)
    { return TransformOpProcessor::Rotate(quatChange, space); }
};

} // MayaUsdUtils
