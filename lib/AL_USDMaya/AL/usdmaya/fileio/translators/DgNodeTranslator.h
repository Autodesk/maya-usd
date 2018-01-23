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
#pragma once
#include <AL/usdmaya/ForwardDeclares.h>
#include "AL/usdmaya/AttributeType.h"
#include "AL/usdmaya/utils/DgNodeHelper.h"
#include "AL/usdmaya/utils/ForwardDeclares.h"
#include "AL/maya/utils/MayaHelperMacros.h"

#include "maya/MPlug.h"
#include "maya/MAngle.h"
#include "maya/MDistance.h"
#include "maya/MTime.h"
#include "maya/MFnAnimCurve.h"

#include "maya/MGlobal.h"
#include "maya/MStatus.h"

#include "pxr/base/gf/half.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdGeom/xformOp.h"

#include <string>
#include <vector>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {
//----------------------------------------------------------------------------------------------------------------------
/// \brief  Utility class that transfers DgNodes between Maya and USD.
/// \ingroup   translators
//----------------------------------------------------------------------------------------------------------------------
class DgNodeTranslator
  : public usdmaya::utils::DgNodeHelper
{
public:


  /// \brief  static type registration
  /// \return MS::kSuccess if ok
  static MStatus registerType();

  /// \brief  Creates a new maya node of the given type and set attributes based on input prim
  /// \param  from the UsdPrim to copy the data from
  /// \param  parent the parent Dag node to parent the newly created object under
  /// \param  nodeType the maya node type to create
  /// \param  params the importer params that determines what will be imported
  /// \return the newly created node
  virtual MObject createNode(const UsdPrim& from, MObject parent, const char* nodeType, const ImporterParams& params);

  /// \brief  helper method to copy attributes from the UsdPrim to the Maya node
  /// \param  from the UsdPrim to copy the data from
  /// \param  to the maya node to copy the data to
  /// \param  params the importer params to determine what to import
  /// \return MS::kSuccess if ok
  MStatus copyAttributes(const UsdPrim& from, MObject to, const ImporterParams& params);

  /// \brief  Copies data from the maya node onto the usd primitive
  /// \param  from the maya node to copy the data from
  /// \param  to the USD prim to copy the attributes to
  /// \param  params the exporter params to determine what should be exported
  /// \return MS::kSuccess if ok
  static MStatus copyAttributes(const MObject& from, UsdPrim& to, const ExporterParams& params);

  /// \brief  A temporary solution. Given a custom attribute, if a translator handles it somehow (i.e. lazy approach to
  ///         not creating a schema), then overload this method and return true on the attribute you are handling.
  ///         This will prevent the attribute from being imported/exported as a dynamic attribute.
  /// \param  usdAttr the attribute to test
  /// \return true if your translator is handling this attr
  virtual bool attributeHandled(const UsdAttribute& usdAttr);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   animation
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  creates animation curves in maya for the specified attribute
  /// \param  node the node instance the animated attribute belongs to
  /// \param  attr the attribute handle
  /// \param  op the USD geometry operation that contains the animation data
  /// \param  conversionFactor a scaling factor to apply to the source key frames on import.
  /// \return MS::kSuccess on success, error code otherwise
  template<typename T>
  static MStatus setVec3Anim(MObject node, MObject attr, const UsdGeomXformOp op, double conversionFactor = 1.0);

  /// \brief  creates animation curves to animate the specified angle attribute
  /// \param  node the node instance the animated attribute belongs to
  /// \param  attr the attribute handle
  /// \param  op the USD transform op that contains the keyframe data
  /// \return MS::kSuccess on success, error code otherwise
  static MStatus setAngleAnim(MObject node, MObject attr, const UsdGeomXformOp op);

  /// \brief  creates animation curves in maya for the specified attribute
  /// \param  node the node instance the animated attribute belongs to
  /// \param  attr the attribute handle
  /// \param  usdAttr the USD attribute that contains the keyframe data
  /// \param  conversionFactor a scaling to apply to the key frames on import
  /// \return MS::kSuccess on success, error code otherwise
  static MStatus setFloatAttrAnim(MObject node, MObject attr, UsdAttribute usdAttr, double conversionFactor = 1.0);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Get array values from Maya
  //--------------------------------------------------------------------------------------------------------------------

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus getUsdBoolArray(const MObject& node, const MObject& attr, VtArray<bool>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus getUsdInt8Array(const MObject& node, const MObject& attr, VtArray<int8_t>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus getUsdInt16Array(const MObject& node, const MObject& attr, VtArray<int16_t>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus getUsdInt32Array(const MObject& node, const MObject& attr, VtArray<int32_t>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus getUsdInt64Array(const MObject& node, const MObject& attr, VtArray<int64_t>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus getUsdHalfArray(const MObject& node, const MObject& attr, VtArray<GfHalf>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus getUsdFloatArray(const MObject& node, const MObject& attr, VtArray<float>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus getUsdDoubleArray(const MObject& node, const MObject& attr, VtArray<double>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus setUsdBoolArray(const MObject& node, const MObject& attr, const VtArray<bool>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus setUsdInt8Array(const MObject& node, const MObject& attr, const VtArray<int8_t>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus setUsdInt16Array(const MObject& node, const MObject& attr, const VtArray<int16_t>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus setUsdInt32Array(const MObject& node, const MObject& attr, const VtArray<int32_t>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus setUsdInt64Array(const MObject& node, const MObject& attr, const VtArray<int64_t>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus setUsdHalfArray(const MObject& node, const MObject& attr, const VtArray<GfHalf>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus setUsdFloatArray(const MObject& node, const MObject& attr, const VtArray<float>& values);

  /// \name   get data from maya attribute, and store in the USD values array
  /// \param  node the node to get the attribute data from
  /// \param  attr the attribute to get the data from
  /// \param  values the returned array data
  /// \return MS::kSuccess if succeeded
  static MStatus setUsdDoubleArray(const MObject& node, const MObject& attr, const VtArray<double>& values);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Copy single values from USD to Maya
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  copy a boolean value from USD and apply to Maya attribute
  /// \param  node the node to copy the attribute data to
  /// \param  attr the attribute to copy the data to
  /// \param  value the USD attribute to copy the data from
  /// \return MS::kSuccess if succeeded
  static MStatus copyBool(MObject node, MObject attr, const UsdAttribute& value);

  /// \brief  copy a boolean value from USD and apply to Maya attribute
  /// \param  node the node to copy the attribute data to
  /// \param  attr the attribute to copy the data to
  /// \param  value the USD attribute to copy the data from
  /// \return MS::kSuccess if succeeded
  static MStatus copyFloat(MObject node, MObject attr, const UsdAttribute& value);

  /// \brief  copy a boolean value from USD and apply to Maya attribute
  /// \param  node the node to copy the attribute data to
  /// \param  attr the attribute to copy the data to
  /// \param  value the USD attribute to copy the data from
  /// \return MS::kSuccess if succeeded
  static MStatus copyDouble(MObject node, MObject attr, const UsdAttribute& value);

  /// \brief  copy a boolean value from USD and apply to Maya attribute
  /// \param  node the node to copy the attribute data to
  /// \param  attr the attribute to copy the data to
  /// \param  value the USD attribute to copy the data from
  /// \return MS::kSuccess if succeeded
  static MStatus copyInt(MObject node, MObject attr, const UsdAttribute& value);

  /// \brief  copy a boolean value from USD and apply to Maya attribute
  /// \param  node the node to copy the attribute data to
  /// \param  attr the attribute to copy the data to
  /// \param  value the USD attribute to copy the data from
  /// \return MS::kSuccess if succeeded
  static MStatus copyVec3(MObject node, MObject attr, const UsdAttribute& value);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Internal import/export utils
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  copy a non array value from a usd attribute into the maya attribute specified
  /// \param  node the node to copy the attribute data to
  /// \param  attr the attribute to copy the data to
  /// \param  usdAttr the attribute to copy the from
  /// \param  type the attribute type
  /// \return MS::kSuccess if succeeded, error code otherwise
  static MStatus setSingleMayaValue(MObject node, MObject attr, const UsdAttribute& usdAttr, const UsdDataType type);

  /// \brief  copy an array value from a usd attribute into the maya attribute specified
  /// \param  node the node to copy the attribute data to
  /// \param  attr the attribute to copy the data to
  /// \param  usdAttr the attribute to copy the from
  /// \param  type the attribute type of the array elements
  /// \return MS::kSuccess if succeeded, error code otherwise
  static MStatus setArrayMayaValue(MObject node, MObject attr, const UsdAttribute& usdAttr, const UsdDataType type);

  /// \brief  copy the value from the usdAttribute onto the maya attribute value
  /// \param  node the node to copy the attribute data to
  /// \param  attr the attribute to copy the data to
  /// \param  usdAttr the attribute to copy the from
  /// \return MS::kSuccess if succeeded, error code otherwise
  static MStatus setMayaValue(MObject node, MObject attr, const UsdAttribute& usdAttr);

  /// \brief  creates a new dynamic attribute on the Maya node specified which will be initialized from the usdAttr.
  /// \param  node the node to copy the attribute data to
  /// \param  usdAttr the attribute to copy the from
  /// \return MS::kSuccess if succeeded, error code otherwise
  static MStatus addDynamicAttribute(MObject node, const UsdAttribute& usdAttr);

  /// \brief  copy all custom attributes from the usd primitive onto the maya node.
  /// \param  node the node to copy the attributes to
  /// \param  prim the USD prim to copy the attributes from
  /// \return MS::kSuccess if succeeded, error code otherwise
  static MStatus copyDynamicAttributes(MObject node, UsdPrim& prim);

  /// \brief  copy the attribute value from the plug specified, at the given time, and store the data on the usdAttr.
  /// \param  attr the attribute to be copied
  /// \param  usdAttr the attribute to copy the data to
  /// \param  timeCode the timecode to use when setting the data
  static void copyAttributeValue(const MPlug& attr, UsdAttribute& usdAttr, const UsdTimeCode& timeCode);

  /// \brief  copy the attribute value from the plug specified, at the given time, and store the data on the usdAttr.
  /// \param  plug the attribute to be copied
  /// \param  usdAttr the attribute to copy the data to
  /// \param  timeCode the timecode to use when setting the data
  static void copySimpleValue(const MPlug& plug, UsdAttribute& usdAttr, const UsdTimeCode& timeCode);

  /// \brief  copy the attribute value from the plug specified, at the given time, and store the data on the usdAttr.
  /// \param  attr the attribute to be copied
  /// \param  usdAttr the attribute to copy the data to
  /// \param  scale a scaling factor to apply to provide support for
  /// \param  timeCode the timecode to use when setting the data
  static void copyAttributeValue(const MPlug& attr, UsdAttribute& usdAttr, float scale, const UsdTimeCode& timeCode);

  /// \brief  copy the attribute value from the plug specified, at the given time, and store the data on the usdAttr.
  /// \param  plug the attribute to be copied
  /// \param  usdAttr the attribute to copy the data to
  /// \param  scale a scaling factor to apply to provide support for
  /// \param  timeCode the timecode to use when setting the data
  static void copySimpleValue(const MPlug& plug, UsdAttribute& usdAttr, float scale, const UsdTimeCode& timeCode);

  /// \brief  convert value from the plug specified and set it to usd attribute.
  /// \param  plug the plug to copy the attributes value from
  /// \param  usdAttr the USDAttribute to set the attribute value to
  /// \return MS::kSuccess if the conversion success based on certain rules.
  static MStatus convertSpecialValueToUSDAttribute(const MPlug& plug, UsdAttribute& usdAttr);
};

//----------------------------------------------------------------------------------------------------------------------
template<typename T>
MStatus DgNodeTranslator::setVec3Anim(MObject node, MObject attr, const UsdGeomXformOp op, double conversionFactor)
{
  MPlug plug(node, attr);
  MStatus status;
  const char* const xformErrorCreate = "DgNodeTranslator:setVec3Anim error creating animation curve";

  MFnAnimCurve acFnSetX;
  acFnSetX.create(plug.child(0), NULL, &status);
  AL_MAYA_CHECK_ERROR(status, xformErrorCreate);

  MFnAnimCurve acFnSetY;
  acFnSetY.create(plug.child(1), NULL, &status);
  AL_MAYA_CHECK_ERROR(status, xformErrorCreate);

  MFnAnimCurve acFnSetZ;
  acFnSetZ.create(plug.child(2), NULL, &status);
  AL_MAYA_CHECK_ERROR(status, xformErrorCreate);

  std::vector<double> times;
  op.GetTimeSamples(&times);

  const char* const xformErrorKey = "DgNodeTranslator:setVec3Anim error setting key on animation curve";

  T value(0);
  for(auto const& timeValue: times)
  {
    const bool retValue = op.GetAs<T>(&value, timeValue);
    if (!retValue) continue;

    MTime tm(timeValue, MTime::kFilm);

    switch (acFnSetX.animCurveType())
    {
      case MFnAnimCurve::kAnimCurveTL: // time->distance: translation
      case MFnAnimCurve::kAnimCurveTA: // time->angle: rotation
      case MFnAnimCurve::kAnimCurveTU: // time->double: scale
      {
        acFnSetX.addKey(tm, value[0] * conversionFactor, MFnAnimCurve::kTangentGlobal, MFnAnimCurve::kTangentGlobal, NULL, &status);
        AL_MAYA_CHECK_ERROR(status, xformErrorKey);
        acFnSetY.addKey(tm, value[1] * conversionFactor, MFnAnimCurve::kTangentGlobal, MFnAnimCurve::kTangentGlobal, NULL, &status);
        AL_MAYA_CHECK_ERROR(status, xformErrorKey);
        acFnSetZ.addKey(tm, value[2] * conversionFactor, MFnAnimCurve::kTangentGlobal, MFnAnimCurve::kTangentGlobal, NULL, &status);
        AL_MAYA_CHECK_ERROR(status, xformErrorKey);
        break;
      }
      default:
      {
        break;
      }
    }
  }

  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::getUsdInt8Array(const MObject& node, const MObject& attr, VtArray<int8_t>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getInt8Array(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::getUsdInt16Array(const MObject& node, const MObject& attr, VtArray<int16_t>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getInt16Array(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::getUsdInt32Array(const MObject& node, const MObject& attr, VtArray<int32_t>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getInt32Array(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::getUsdInt64Array(const MObject& node, const MObject& attr, VtArray<int64_t>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getInt64Array(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::getUsdHalfArray(const MObject& node, const MObject& attr, VtArray<GfHalf>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getHalfArray(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::getUsdFloatArray(const MObject& node, const MObject& attr, VtArray<float>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getFloatArray(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::getUsdDoubleArray(const MObject& node, const MObject& attr, VtArray<double>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getDoubleArray(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::setUsdInt8Array(const MObject& node, const MObject& attr, const VtArray<int8_t>& values)
{
  return setInt8Array(node, attr, values.cdata(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::setUsdInt16Array(const MObject& node, const MObject& attr, const VtArray<int16_t>& values)
{
  return setInt16Array(node, attr, values.cdata(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::setUsdInt32Array(const MObject& node, const MObject& attr, const VtArray<int32_t>& values)
{
  return setInt32Array(node, attr, values.cdata(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::setUsdInt64Array(const MObject& node, const MObject& attr, const VtArray<int64_t>& values)
{
  return setInt64Array(node, attr, values.cdata(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::setUsdHalfArray(const MObject& node, const MObject& attr, const VtArray<GfHalf>& values)
{
  return setHalfArray(node, attr, values.cdata(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::setUsdFloatArray(const MObject& node, const MObject& attr, const VtArray<float>& values)
{
  return setFloatArray(node, attr, values.cdata(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeTranslator::setUsdDoubleArray(const MObject& node, const MObject& attr, const VtArray<double>& values)
{
  return setDoubleArray(node, attr, values.cdata(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
