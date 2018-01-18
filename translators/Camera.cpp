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

#include "Camera.h"
#include "pxr/usd/usdGeom/camera.h"

#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"

#include "maya/MDagPath.h"
#include "maya/MGlobal.h"
#include "maya/MTime.h"
#include "maya/MDistance.h"
#include "maya/MFileIO.h"
#include "maya/MFnDagNode.h"
#include "maya/MNodeClass.h"
#include "maya/M3dView.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {
  
AL_USDMAYA_DEFINE_TRANSLATOR(CameraTranslator, UsdGeomCamera)

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
MStatus CameraTranslator::initialize()
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
void CameraTranslator::checkCurrentCameras(MObject cameraNode)
{
  MSelectionList sl;
  sl.add("perspShape");
  MDagPath path;
  sl.getDagPath(0, path);
  M3dView view;
  uint32_t nviews = M3dView::numberOf3dViews();
  for(uint32_t i = 0; i < nviews; ++i)
  {
    if(M3dView::get3dView(i, view))
    {
      MDagPath camera;
      if(view.getCamera(camera))
      {
        if(camera.node() == cameraNode)
        {
          if(!view.setCamera(path))
          {
            MGlobal::displayError("Cannot change the camera that is being deleted. Maya will probably crash in a sec!");
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
MStatus CameraTranslator::update(const UsdPrim& prim)
{
  const char* const errorString = "CameraTranslator: error setting maya camera parameters";
  UsdGeomCamera usdCamera(prim);
  
  MObjectHandle handle;
  if(!context()->getMObject(prim, handle, MFn::kCamera))
  {
    MGlobal::displayError("unable to locate camera node");
    return MS::kFailure;
  }
  const float mm_to_inches = 0.0393701f;

  MObject to = handle.object();
  
  // Orthographic camera (attribute cannot be keyed)
  TfToken projection;
  usdCamera.GetProjectionAttr().Get(&projection);
  bool isOrthographic = (projection == UsdGeomTokens->orthographic);
  AL_MAYA_CHECK_ERROR(DgNodeTranslator::setBool(to, m_orthographic, isOrthographic), errorString);

  // Horizontal film aperture
  if(!DgNodeTranslator::setFloatAttrAnim(to, m_horizontalFilmAperture, usdCamera.GetHorizontalApertureAttr(), mm_to_inches))
  {
    float horizontalAperture;
    usdCamera.GetHorizontalApertureAttr().Get(&horizontalAperture);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setDouble(to, m_horizontalFilmAperture, mm_to_inches * horizontalAperture), errorString);
  }

  // Vertical film aperture
  if(!DgNodeTranslator::setFloatAttrAnim(to, m_verticalFilmAperture, usdCamera.GetVerticalApertureAttr(), mm_to_inches))
  {
    float verticalAperture;
    usdCamera.GetVerticalApertureAttr().Get(&verticalAperture);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setDouble(to, m_verticalFilmAperture, mm_to_inches * verticalAperture), errorString);
  }

  // Horizontal film aperture offset
  if(!DgNodeTranslator::setFloatAttrAnim(to, m_horizontalFilmApertureOffset, usdCamera.GetHorizontalApertureOffsetAttr(), mm_to_inches))
  {
    float horizontalApertureOffset;
    usdCamera.GetHorizontalApertureOffsetAttr().Get(&horizontalApertureOffset);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setDouble(to, m_horizontalFilmApertureOffset, mm_to_inches * horizontalApertureOffset), errorString);
  }

  // Vertical film aperture offset
  if(!DgNodeTranslator::setFloatAttrAnim(to, m_verticalFilmApertureOffset, usdCamera.GetVerticalApertureOffsetAttr(), mm_to_inches))
  {
    float verticalApertureOffset;
    usdCamera.GetVerticalApertureOffsetAttr().Get(&verticalApertureOffset);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setDouble(to, m_verticalFilmApertureOffset, mm_to_inches * verticalApertureOffset), errorString);
  }

  // Focal length
  if(!DgNodeTranslator::setFloatAttrAnim(to, m_focalLength, usdCamera.GetFocalLengthAttr()))
  {
    float focalLength;
    usdCamera.GetFocalLengthAttr().Get(&focalLength);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setDouble(to, m_focalLength, focalLength), errorString);
  }

  // Near/far clip planes
  // N.B. Animated clip plane values not supported
  GfVec2f clippingRange;
  usdCamera.GetClippingRangeAttr().Get(&clippingRange);
  AL_MAYA_CHECK_ERROR(DgNodeTranslator::setDistance(to, m_nearDistance, MDistance(clippingRange[0], MDistance::kCentimeters)), errorString);
  AL_MAYA_CHECK_ERROR(DgNodeTranslator::setDistance(to, m_farDistance, MDistance(clippingRange[1], MDistance::kCentimeters)), errorString);
  
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus CameraTranslator::import(const UsdPrim& prim, MObject& parent)
{
  const char* const errorString = "CameraTranslator: error setting maya camera parameters";
  UsdGeomCamera usdCamera(prim);
  
  MStatus status;
  MFnDagNode fn;
  MString name(prim.GetName().GetText() + MString("Shape"));
  MObject to = fn.create("camera", name, parent, &status);
  context()->insertItem(prim, to);
  
  // F-Stop
  if (DgNodeTranslator::setFloatAttrAnim(to, m_fstop, usdCamera.GetFStopAttr()))
  {
    float fstop;
    usdCamera.GetFStopAttr().Get(&fstop);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setDouble(to, m_fstop, fstop), errorString);
  }

  // Focus distance
  if (usdCamera.GetFocusDistanceAttr().GetNumTimeSamples())
  {
    // TODO: What unit here?
    MDistance one(1.0, MDistance::kCentimeters);
    double conversionFactor = one.as(MDistance::kCentimeters);
    DgNodeTranslator::setFloatAttrAnim(to, m_focusDistance, usdCamera.GetFocusDistanceAttr(), conversionFactor);
  }
  else
  {
    float focusDistance;
    usdCamera.GetFocusDistanceAttr().Get(&focusDistance);
    AL_MAYA_CHECK_ERROR(DgNodeTranslator::setDistance(to, m_focusDistance, MDistance(focusDistance, MDistance::kCentimeters)), errorString);
  }

  return update(prim);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus CameraTranslator::tearDown(const SdfPath& path)
{
  MObjectHandle obj;
  context()->getMObject(path, obj, MFn::kCamera);
  checkCurrentCameras(obj.object());
  context()->removeItems(path);
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
