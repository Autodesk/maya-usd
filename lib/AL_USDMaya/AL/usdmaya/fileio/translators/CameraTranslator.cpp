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
#include "AL/usdmaya/fileio/AnimationTranslator.h"
#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/fileio/translators/CameraTranslator.h"

#include "maya/MFnDagNode.h"
#include "maya/MNodeClass.h"
#include "maya/MGlobal.h"
#include <algorithm>

#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdGeom/tokens.h"

// printf debugging
#if 0 || AL_ENABLE_TRACE
# define Trace(X) std::cerr << X << std::endl;
#else
# define Trace(X)
#endif

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {
//----------------------------------------------------------------------------------------------------------------------
MObject CameraTranslator::m_orthographic;
MObject CameraTranslator::m_horizontalFilmAperture;
MObject CameraTranslator::m_verticalFilmAperture;
MObject CameraTranslator::m_horizontalFilmApertureOffset;
MObject CameraTranslator::m_verticalFilmApertureOffset;
MObject CameraTranslator::m_focalLength;
MObject CameraTranslator::m_nearDistance;
MObject CameraTranslator::m_farDistance;
MObject CameraTranslator::m_fstop;
MObject CameraTranslator::m_focusDistance;
MObject CameraTranslator::m_lensSqueezeRatio;

//----------------------------------------------------------------------------------------------------------------------
MStatus CameraTranslator::registerType()
{
  MNodeClass nodeClass("camera");
  m_orthographic = nodeClass.attribute("o");
  m_horizontalFilmAperture = nodeClass.attribute("hfa");
  m_verticalFilmAperture = nodeClass.attribute("vfa");
  m_horizontalFilmApertureOffset = nodeClass.attribute("hfo");
  m_verticalFilmApertureOffset = nodeClass.attribute("vfo");
  m_focalLength = nodeClass.attribute("fl");
  m_nearDistance = nodeClass.attribute("ncp");
  m_farDistance = nodeClass.attribute("fcp");
  m_fstop = nodeClass.attribute("fs");
  m_focusDistance = nodeClass.attribute("fd");
  m_lensSqueezeRatio = nodeClass.attribute("lsr");
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MObject CameraTranslator::createNode(
    const UsdPrim& from,
    MObject parent,
    const char* nodeType,
    const ImporterParams& params)
{
  MStatus status;
  MFnDagNode fn;
  MObject object = fn.create("camera", parent, &status);
  AL_MAYA_CHECK_ERROR2(status, "CameraTranslator: error creating Camera node");
  AL_MAYA_CHECK_ERROR2(copyAttributes(from, object, params), "CameraTranslator: error creating Camera node");
  return object;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus CameraTranslator::copyAttributes(const UsdPrim& from, MObject to, const ImporterParams& params)
{
  const char* const errorString = "CameraTranslator: error setting maya camera parameters";
  UsdGeomCamera usdCamera(from);

  const float mm_to_inches = 0.0393701f;

  // Orthographic camera (attribute cannot be keyed)
  TfToken projection;
  usdCamera.GetProjectionAttr().Get(&projection);
  bool isOrthographic = (projection == UsdGeomTokens->orthographic);
  AL_MAYA_CHECK_ERROR(setBool(to, m_orthographic, isOrthographic), errorString);

  // Horizontal film aperture
  if (not setFloatAttrAnim(to, m_horizontalFilmAperture, usdCamera.GetHorizontalApertureAttr(), mm_to_inches))
  {
      float horizontalAperture;
      usdCamera.GetHorizontalApertureAttr().Get(&horizontalAperture);
      AL_MAYA_CHECK_ERROR(setDouble(to, m_horizontalFilmAperture, mm_to_inches * horizontalAperture), errorString);
  }

  // Vertical film aperture
  if (not setFloatAttrAnim(to, m_verticalFilmAperture, usdCamera.GetVerticalApertureAttr(), mm_to_inches))
  {
      float verticalAperture;
      usdCamera.GetVerticalApertureAttr().Get(&verticalAperture);
      AL_MAYA_CHECK_ERROR(setDouble(to, m_verticalFilmAperture, mm_to_inches * verticalAperture), errorString);
  }

  // Horizontal film aperture offset
  if (not setFloatAttrAnim(to, m_horizontalFilmApertureOffset, usdCamera.GetHorizontalApertureOffsetAttr(), mm_to_inches))
  {
      float horizontalApertureOffset;
      usdCamera.GetHorizontalApertureOffsetAttr().Get(&horizontalApertureOffset);
      AL_MAYA_CHECK_ERROR(setDouble(to, m_horizontalFilmApertureOffset, mm_to_inches * horizontalApertureOffset), errorString);
  }

  // Vertical film aperture offset
  if (not setFloatAttrAnim(to, m_verticalFilmApertureOffset, usdCamera.GetVerticalApertureOffsetAttr(), mm_to_inches))
  {
      float verticalApertureOffset;
      usdCamera.GetVerticalApertureOffsetAttr().Get(&verticalApertureOffset);
      AL_MAYA_CHECK_ERROR(setDouble(to, m_verticalFilmApertureOffset, mm_to_inches * verticalApertureOffset), errorString);
  }

  // Focal length
  if (not setFloatAttrAnim(to, m_focalLength, usdCamera.GetFocalLengthAttr()))
  {
      float focalLength;
      usdCamera.GetFocalLengthAttr().Get(&focalLength);
      AL_MAYA_CHECK_ERROR(setDouble(to, m_focalLength, focalLength), errorString);
  }

  // Near/far clip planes
  // N.B. Animated clip plane values not supported
  GfVec2f clippingRange;
  usdCamera.GetClippingRangeAttr().Get(&clippingRange);
  AL_MAYA_CHECK_ERROR(setDistance(to, m_nearDistance, MDistance(clippingRange[0], MDistance::kCentimeters)), errorString);
  AL_MAYA_CHECK_ERROR(setDistance(to, m_farDistance, MDistance(clippingRange[1], MDistance::kCentimeters)), errorString);

  // F-Stop
  if (not setFloatAttrAnim(to, m_fstop, usdCamera.GetFStopAttr()))
  {
    float fstop;
    usdCamera.GetFStopAttr().Get(&fstop);
    AL_MAYA_CHECK_ERROR(setDouble(to, m_fstop, fstop), errorString);
  }

  // Focus distance
  if (usdCamera.GetFocusDistanceAttr().GetNumTimeSamples())
  {
    // TODO: What unit here?
    MDistance one(1.0, MDistance::kCentimeters);
    double conversionFactor = one.as(MDistance::kCentimeters);
    setFloatAttrAnim(to, m_focusDistance, usdCamera.GetFocusDistanceAttr(), conversionFactor);
  }
  else
  {
    float focusDistance;
    usdCamera.GetFocusDistanceAttr().Get(&focusDistance);
    AL_MAYA_CHECK_ERROR(setDistance(to, m_focusDistance, MDistance(focusDistance, MDistance::kCentimeters)), errorString);
  }

  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus CameraTranslator::copyAttributes(const MObject& from, UsdPrim& prim, const ExporterParams& params)
{
  UsdGeomCamera usdCamera(prim);
  const char* const errorString = "CameraTranslator: error getting maya camera parameters";
  MStatus status;
  const float inches_to_mm = 1.0f / 0.0393701f;

  // set orthographic
  bool isOrthographic;
  double squeezeRatio;
  double horizontalAperture;
  double verticalAperture;
  double horizontalApertureOffset;
  double verticalApertureOffset;
  double focalLength;
  double fstop;
  MDistance nearDistance;
  MDistance farDistance;
  MDistance focusDistance;

  AL_MAYA_CHECK_ERROR(getBool(from, m_orthographic, isOrthographic), errorString);
  AL_MAYA_CHECK_ERROR(getDouble(from, m_horizontalFilmAperture, horizontalAperture), errorString);
  AL_MAYA_CHECK_ERROR(getDouble(from, m_verticalFilmAperture, verticalAperture), errorString);
  AL_MAYA_CHECK_ERROR(getDouble(from, m_horizontalFilmApertureOffset, horizontalApertureOffset), errorString);
  AL_MAYA_CHECK_ERROR(getDouble(from, m_verticalFilmApertureOffset, verticalApertureOffset), errorString);
  AL_MAYA_CHECK_ERROR(getDouble(from, m_focalLength, focalLength), errorString);
  AL_MAYA_CHECK_ERROR(getDistance(from, m_nearDistance, nearDistance), errorString);
  AL_MAYA_CHECK_ERROR(getDistance(from, m_farDistance, farDistance), errorString);
  AL_MAYA_CHECK_ERROR(getDouble(from, m_fstop, fstop), errorString);
  AL_MAYA_CHECK_ERROR(getDistance(from, m_focusDistance, focusDistance), errorString);
  AL_MAYA_CHECK_ERROR(getDouble(from, m_lensSqueezeRatio, squeezeRatio), errorString);

  usdCamera.GetProjectionAttr().Set(isOrthographic ? UsdGeomTokens->orthographic : UsdGeomTokens->perspective);
  usdCamera.GetHorizontalApertureAttr().Set(float(horizontalAperture * squeezeRatio * inches_to_mm));
  usdCamera.GetVerticalApertureAttr().Set(float(verticalAperture * squeezeRatio * inches_to_mm));
  usdCamera.GetHorizontalApertureOffsetAttr().Set(float(horizontalApertureOffset * squeezeRatio * inches_to_mm));
  usdCamera.GetVerticalApertureOffsetAttr().Set(float(verticalApertureOffset * squeezeRatio * inches_to_mm));
  usdCamera.GetFocalLengthAttr().Set(float(focalLength));
  usdCamera.GetClippingRangeAttr().Set(GfVec2f(nearDistance.as(MDistance::kCentimeters), farDistance.as(MDistance::kCentimeters)));
  usdCamera.GetFStopAttr().Set(float(fstop));
  usdCamera.GetFocusDistanceAttr().Set(float(focusDistance.as(MDistance::kCentimeters)));

  AnimationTranslator* animTranslator = params.m_animTranslator;
  if(animTranslator)
  {
    //
    animTranslator->addPlug(MPlug(from, m_horizontalFilmAperture), usdCamera.GetHorizontalApertureAttr(), squeezeRatio * inches_to_mm, true);
    animTranslator->addPlug(MPlug(from, m_verticalFilmAperture), usdCamera.GetVerticalApertureAttr(), squeezeRatio * inches_to_mm, true);
    animTranslator->addPlug(MPlug(from, m_horizontalFilmApertureOffset), usdCamera.GetHorizontalApertureOffsetAttr(), squeezeRatio * inches_to_mm, true);
    animTranslator->addPlug(MPlug(from, m_verticalFilmApertureOffset), usdCamera.GetVerticalApertureOffsetAttr(), squeezeRatio * inches_to_mm, true);
    animTranslator->addPlug(MPlug(from, m_focalLength), usdCamera.GetFocalLengthAttr(), true);
    animTranslator->addPlug(MPlug(from, m_fstop), usdCamera.GetFStopAttr(), true);
    animTranslator->addPlug(MPlug(from, m_focusDistance), usdCamera.GetFocusDistanceAttr(), true);
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
