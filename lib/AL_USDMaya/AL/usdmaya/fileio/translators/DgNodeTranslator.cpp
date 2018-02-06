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
#include "AL/maya/utils/NodeHelper.h"
#include "AL/usdmaya/utils/SIMD.h"
#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"

#include "maya/MObject.h"
#include "maya/MStatus.h"
#include "maya/MGlobal.h"
#include "maya/MPlug.h"
#include "maya/MFnDependencyNode.h"
#include "maya/MDGModifier.h"
#include "maya/MMatrixArray.h"
#include "maya/MFnMatrixData.h"
#include "maya/MFnMatrixArrayData.h"
#include "maya/MMatrix.h"
#include "maya/MFloatMatrix.h"
#include "maya/MFnNumericAttribute.h"
#include "maya/MFnMatrixAttribute.h"
#include "maya/MFnTypedAttribute.h"
#include "maya/MFnCompoundAttribute.h"

#include <unordered_map>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::registerType()
{
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MObject DgNodeTranslator::createNode(const UsdPrim& from, MObject parent, const char* nodeType, const ImporterParams& params)
{
  MFnDependencyNode fn;
  MObject to = fn.create(nodeType);

  MStatus status = copyAttributes(from, to, params);
  AL_MAYA_CHECK_ERROR_RETURN_NULL_MOBJECT(status, "Dg node translator: unable to get attributes");

  return to;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::copyAttributes(const UsdPrim& from, MObject to, const ImporterParams& params)
{
  if(params.m_dynamicAttributes)
  {
    const std::vector<UsdAttribute> attributes = from.GetAttributes();
    for(size_t i = 0; i < attributes.size(); ++i)
    {
      if(attributes[i].IsAuthored() && attributes[i].HasValue() && attributes[i].IsCustom())
      {
        if(!attributeHandled(attributes[i]))
          addDynamicAttribute(to, attributes[i]);
      }
    }
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::copyAttributes(const MObject& from, UsdPrim& to, const ExporterParams& params)
{
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::setAngleAnim(const MObject node, const MObject attr, const UsdGeomXformOp op)
{
  MStatus status;
  const char* const errorString = "DgNodeTranslator::setAngleAnim";

  MPlug plug(node, attr);
  MFnAnimCurve fnCurve;
  fnCurve.create(plug, NULL, &status);
  AL_MAYA_CHECK_ERROR(status, errorString);

  std::vector<double> times;
  op.GetTimeSamples(&times);

  const float conversionFactor = 0.0174533f;

  float value = 0;
  for(auto const& timeValue: times)
  {
    const bool retValue = op.GetAs<float>(&value, timeValue);
    if (!retValue) continue;

    MTime tm(timeValue, MTime::kFilm);

    switch (fnCurve.animCurveType())
    {
      case MFnAnimCurve::kAnimCurveTL:
      case MFnAnimCurve::kAnimCurveTA:
      case MFnAnimCurve::kAnimCurveTU:
      {
        fnCurve.addKey(tm, value * conversionFactor, MFnAnimCurve::kTangentGlobal, MFnAnimCurve::kTangentGlobal, NULL, &status);
        AL_MAYA_CHECK_ERROR(status, errorString);
        break;
      }
      default:
      {
        std::cout << "[DgNodeTranslator::setAngleAnim] Unexpected anim curve type: " << fnCurve.animCurveType() << std::endl;
        break;
      }
    }
  }

  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------

MStatus DgNodeTranslator::setFloatAttrAnim(const MObject node, const MObject attr, UsdAttribute usdAttr, double conversionFactor)
{
  if (not usdAttr.GetNumTimeSamples())
  {
    return MS::kFailure;
  }

  const char* const errorString = "DgNodeTranslator::setFloatAttrAnim";
  MStatus status;

  MPlug plug(node, attr);
  MPlug srcPlug;
  MFnAnimCurve fnCurve;
  MDGModifier dgmod;

  srcPlug = plug.source(&status);
  AL_MAYA_CHECK_ERROR(status, errorString);
  if(!srcPlug.isNull())
  {
    std::cout << "[DgNodeTranslator::setFloatAttrAnim] disconnecting curve! = " << srcPlug.name().asChar() << std::endl;
    dgmod.disconnect(srcPlug, plug);
    dgmod.doIt();
  }
  fnCurve.create(plug, NULL, &status);
  AL_MAYA_CHECK_ERROR(status, errorString);

  std::vector<double> times;
  usdAttr.GetTimeSamples(&times);

  float value;
  for(auto const& timeValue: times)
  {
    const bool retValue = usdAttr.Get(&value, timeValue);
    if(!retValue) continue;

    MTime tm(timeValue, MTime::kFilm);

    switch(fnCurve.animCurveType())
    {
      case MFnAnimCurve::kAnimCurveTL:
      case MFnAnimCurve::kAnimCurveTA:
      case MFnAnimCurve::kAnimCurveTU:
      {
        fnCurve.addKey(tm, value * conversionFactor, MFnAnimCurve::kTangentGlobal, MFnAnimCurve::kTangentGlobal, NULL, &status);
        AL_MAYA_CHECK_ERROR(status, errorString);
        break;
      }
      default:
      {
        std::cout << "[DgNodeTranslator::setFloatAttrAnim] OTHER ANIM CURVE TYPE! = " << fnCurve.animCurveType() << std::endl;
        break;
      }
    }
  }

  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::copyBool(MObject node, MObject attr, const UsdAttribute& value)
{
  if(value.IsAuthored() && value.HasValue())
  {
    bool data;
    value.Get<bool> (&data);
    return setBool(node, attr, data);
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::copyFloat(MObject node, MObject attr, const UsdAttribute& value)
{
  if(value.IsAuthored() && value.HasValue())
  {
    float data;
    value.Get<float> (&data);
    return setFloat(node, attr, data);
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::copyDouble(MObject node, MObject attr, const UsdAttribute& value)
{
  if(value.IsAuthored() && value.HasValue())
  {
    double data;
    value.Get<double> (&data);
    return setDouble(node, attr, data);
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::copyInt(MObject node, MObject attr, const UsdAttribute& value)
{
  if(value.IsAuthored() && value.HasValue())
  {
    int data;
    value.Get<int> (&data);
    return setBool(node, attr, data);
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::copyVec3(MObject node, MObject attr, const UsdAttribute& value)
{
  if(value.IsAuthored() && value.HasValue())
  {
    int data;
    value.Get<int> (&data);
    return setBool(node, attr, data);
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::setUsdBoolArray(const MObject& node, const MObject& attribute, const VtArray<bool>& values)
{
  MPlug plug(node, attribute);
  if(!plug || !plug.isArray())
    return MS::kFailure;

  AL_MAYA_CHECK_ERROR(plug.setNumElements(values.size()), "DgNodeTranslator: attribute array could not be resized");

  for(size_t i = 0, n = values.size(); i != n; ++i)
  {
    plug.elementByLogicalIndex(i).setBool(values[i]);
  }

  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::getUsdBoolArray(const MObject& node, const MObject& attr, VtArray<bool>& values)
{
  //
  // Handle the oddity that is std::vector<bool>
  //

  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;

  uint32_t num = plug.numElements();
  values.resize(num);
  for(uint32_t i = 0; i < num; ++i)
  {
    values[i] = plug.elementByLogicalIndex(i).asBool();
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::addDynamicAttribute(MObject node, const UsdAttribute& usdAttr)
{
  const SdfValueTypeName typeName = usdAttr.GetTypeName();
  const bool isArray = typeName.IsArray();
  const UsdDataType dataType = getAttributeType(usdAttr);
  MObject attribute = MObject::kNullObj;
  const char* attrName = usdAttr.GetName().GetString().c_str();
  const uint32_t flags = (isArray ? AL::maya::utils::NodeHelper::kArray : 0) | AL::maya::utils::NodeHelper::kReadable | AL::maya::utils::NodeHelper::kWritable | AL::maya::utils::NodeHelper::kStorable | AL::maya::utils::NodeHelper::kConnectable;
  switch(dataType)
  {
  case UsdDataType::kAsset:
    {
      return MS::kSuccess;
    }
    break;

  case UsdDataType::kBool:
    {
      AL::maya::utils::NodeHelper::addBoolAttr(node, attrName, attrName, false, flags, &attribute);
    }
    break;

  case UsdDataType::kUChar:
    {
      AL::maya::utils::NodeHelper::addInt8Attr(node, attrName, attrName, 0, flags, &attribute);
    }
    break;

  case UsdDataType::kInt:
  case UsdDataType::kUInt:
    {
      AL::maya::utils::NodeHelper::addInt32Attr(node, attrName, attrName, 0, flags, &attribute);
    }
    break;

  case UsdDataType::kInt64:
  case UsdDataType::kUInt64:
    {
      AL::maya::utils::NodeHelper::addInt64Attr(node, attrName, attrName, 0, flags, &attribute);
    }
    break;

  case UsdDataType::kHalf:
  case UsdDataType::kFloat:
    {
      AL::maya::utils::NodeHelper::addFloatAttr(node, attrName, attrName, 0, flags, &attribute);
    }
    break;

  case UsdDataType::kDouble:
    {
      AL::maya::utils::NodeHelper::addDoubleAttr(node, attrName, attrName, 0, flags, &attribute);
    }
    break;

  case UsdDataType::kString:
    {
      AL::maya::utils::NodeHelper::addStringAttr(node, attrName, attrName, flags, true, &attribute);
    }
    break;

  case UsdDataType::kMatrix2d:
    {
      const float defValue[2][2] = {{0, 0}, {0, 0}};
      AL::maya::utils::NodeHelper::addMatrix2x2Attr(node, attrName, attrName, defValue, flags, &attribute);
    }
    break;

  case UsdDataType::kMatrix3d:
    {
      const float defValue[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
      AL::maya::utils::NodeHelper::addMatrix3x3Attr(node, attrName, attrName, defValue, flags, &attribute);
    }
    break;

  case UsdDataType::kMatrix4d:
    {
      AL::maya::utils::NodeHelper::addMatrixAttr(node, attrName, attrName, MMatrix(), flags, &attribute);
    }
    break;

  case UsdDataType::kQuatd:
    {
      AL::maya::utils::NodeHelper::addVec4dAttr(node, attrName, attrName, flags, &attribute);
    }
    break;

  case UsdDataType::kQuatf:
  case UsdDataType::kQuath:
    {
      AL::maya::utils::NodeHelper::addVec4fAttr(node, attrName, attrName, flags, &attribute);
    }
    break;

  case UsdDataType::kVec2d:
    {
      AL::maya::utils::NodeHelper::addVec2dAttr(node, attrName, attrName, flags, &attribute);
    }
    break;

  case UsdDataType::kVec2f:
  case UsdDataType::kVec2h:
    {
      AL::maya::utils::NodeHelper::addVec2fAttr(node, attrName, attrName, flags, &attribute);
    }
    break;

  case UsdDataType::kVec2i:
    {
      AL::maya::utils::NodeHelper::addVec2iAttr(node, attrName, attrName, flags, &attribute);
    }
    break;

  case UsdDataType::kVec3d:
    {
      AL::maya::utils::NodeHelper::addVec3dAttr(node, attrName, attrName, flags, &attribute);
    }
    break;

  case UsdDataType::kVec3f:
  case UsdDataType::kVec3h:
    {
      AL::maya::utils::NodeHelper::addVec3fAttr(node, attrName, attrName, flags, &attribute);
    }
    break;

  case UsdDataType::kVec3i:
    {
      AL::maya::utils::NodeHelper::addVec3iAttr(node, attrName, attrName, flags, &attribute);
    }
    break;

  case UsdDataType::kVec4d:
    {
      AL::maya::utils::NodeHelper::addVec4dAttr(node, attrName, attrName, flags, &attribute);
    }
    break;

  case UsdDataType::kVec4f:
  case UsdDataType::kVec4h:
    {
      AL::maya::utils::NodeHelper::addVec4fAttr(node, attrName, attrName, flags, &attribute);
    }
    break;

  case UsdDataType::kVec4i:
    {
      AL::maya::utils::NodeHelper::addVec4iAttr(node, attrName, attrName, flags, &attribute);
    }
    break;

  default:
    MGlobal::displayError("DgNodeTranslator::addDynamicAttribute - unsupported USD data type");
    return MS::kFailure;
  }

  if(isArray)
  {
    return setArrayMayaValue(node, attribute, usdAttr, dataType);
  }
  return setSingleMayaValue(node, attribute, usdAttr, dataType);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::setMayaValue(MObject node, MObject attr, const UsdAttribute& usdAttr)
{
  const SdfValueTypeName typeName = usdAttr.GetTypeName();
  UsdDataType dataType = getAttributeType(usdAttr);

  if(typeName.IsArray())
  {
    return setArrayMayaValue(node, attr, usdAttr, dataType);
  }
  return setSingleMayaValue(node, attr, usdAttr, dataType);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::setArrayMayaValue(MObject node, MObject attr, const UsdAttribute& usdAttr, const UsdDataType type)
{
  switch(type)
  {
  case UsdDataType::kBool:
    {
      VtArray<bool> value;
      usdAttr.Get(&value);
      return setUsdBoolArray(node, attr, value);
    }

  case UsdDataType::kUChar:
    {
      VtArray<unsigned char> value;
      usdAttr.Get(&value);
      return setInt8Array(node, attr, (const int8_t*)value.cdata(), value.size());
    }

  case UsdDataType::kInt:
    {
      VtArray<int32_t> value;
      usdAttr.Get(&value);
      return setInt32Array(node, attr, (const int32_t*)value.cdata(), value.size());
    }

  case UsdDataType::kUInt:
    {
      VtArray<uint32_t> value;
      usdAttr.Get(&value);
      return setInt32Array(node, attr, (const int32_t*)value.cdata(), value.size());
    }

  case UsdDataType::kInt64:
    {
      VtArray<int64_t> value;
      usdAttr.Get(&value);
      return setInt64Array(node, attr, (const int64_t*)value.cdata(), value.size());
    }

  case UsdDataType::kUInt64:
    {
      VtArray<uint64_t> value;
      usdAttr.Get(&value);
      return setInt64Array(node, attr, (const int64_t*)value.cdata(), value.size());
    }

  case UsdDataType::kHalf:
    {
      VtArray<GfHalf> value;
      usdAttr.Get(&value);
      return setHalfArray(node, attr, (const GfHalf*)value.cdata(), value.size());
    }

  case UsdDataType::kFloat:
    {
      VtArray<float> value;
      usdAttr.Get(&value);
      return setFloatArray(node, attr, (const float*)value.cdata(), value.size());
    }

  case UsdDataType::kDouble:
    {
      VtArray<double> value;
      usdAttr.Get(&value);
      return setDoubleArray(node, attr, (const double*)value.cdata(), value.size());
    }

  case UsdDataType::kString:
    {
      VtArray<std::string> value;
      usdAttr.Get(&value);
      return setStringArray(node, attr, (const std::string*)value.cdata(), value.size());
    }

  case UsdDataType::kMatrix2d:
    {
      VtArray<GfMatrix2d> value;
      usdAttr.Get(&value);
      return setMatrix2x2Array(node, attr, (const double*)value.cdata(), value.size());
    }

  case UsdDataType::kMatrix3d:
    {
      VtArray<GfMatrix3d> value;
      usdAttr.Get(&value);
      return setMatrix3x3Array(node, attr, (const double*)value.cdata(), value.size());
    }

  case UsdDataType::kMatrix4d:
    {
      VtArray<GfMatrix4d> value;
      usdAttr.Get(&value);
      return setMatrix4x4Array(node, attr, (const double*)value.cdata(), value.size());
    }

  case UsdDataType::kQuatd:
    {
      VtArray<GfQuatd> value;
      usdAttr.Get(&value);
      return setQuatArray(node, attr, (const double*)value.cdata(), value.size());
    }

  case UsdDataType::kQuatf:
    {
      VtArray<GfQuatf> value;
      usdAttr.Get(&value);
      return setQuatArray(node, attr, (const float*)value.cdata(), value.size());
    }

  case UsdDataType::kQuath:
    {
      VtArray<GfQuath> value;
      usdAttr.Get(&value);
      return setQuatArray(node, attr, (const GfHalf*)value.cdata(), value.size());
    }

  case UsdDataType::kVec2d:
    {
      VtArray<GfVec2d> value;
      usdAttr.Get(&value);
      return setVec2Array(node, attr, (const double*)value.cdata(), value.size());
    }

  case UsdDataType::kVec2f:
    {
      VtArray<GfVec2f> value;
      usdAttr.Get(&value);
      return setVec2Array(node, attr, (const float*)value.cdata(), value.size());
    }

  case UsdDataType::kVec2h:
    {
      VtArray<GfVec2h> value;
      usdAttr.Get(&value);
      return setVec2Array(node, attr, (const GfHalf*)value.cdata(), value.size());
    }

  case UsdDataType::kVec2i:
    {
      VtArray<GfVec2i> value;
      usdAttr.Get(&value);
      return setVec2Array(node, attr, (const int32_t*)value.cdata(), value.size());
    }

  case UsdDataType::kVec3d:
    {
      VtArray<GfVec3d> value;
      usdAttr.Get(&value);
      return setVec3Array(node, attr, (const double*)value.cdata(), value.size());
    }

  case UsdDataType::kVec3f:
    {
      VtArray<GfVec3f> value;
      usdAttr.Get(&value);
      return setVec3Array(node, attr, (const float*)value.cdata(), value.size());
    }

  case UsdDataType::kVec3h:
    {
      VtArray<GfVec3h> value;
      usdAttr.Get(&value);
      return setVec3Array(node, attr, (const GfHalf*)value.cdata(), value.size());
    }

  case UsdDataType::kVec3i:
    {
      VtArray<GfVec3i> value;
      usdAttr.Get(&value);
      return setVec3Array(node, attr, (const int32_t*)value.cdata(), value.size());
    }

  case UsdDataType::kVec4d:
    {
      VtArray<GfVec4d> value;
      usdAttr.Get(&value);
      return setVec4Array(node, attr, (const double*)value.cdata(), value.size());
    }

  case UsdDataType::kVec4f:
    {
      VtArray<GfVec4f> value;
      usdAttr.Get(&value);
      return setVec4Array(node, attr, (const float*)value.cdata(), value.size());
    }

  case UsdDataType::kVec4h:
    {
      VtArray<GfVec4h> value;
      usdAttr.Get(&value);
      return setVec4Array(node, attr, (const GfHalf*)value.cdata(), value.size());
    }

  case UsdDataType::kVec4i:
    {
      VtArray<GfVec4i> value;
      usdAttr.Get(&value);
      return setVec4Array(node, attr, (const int32_t*)value.cdata(), value.size());
    }

  default:
    MGlobal::displayError("DgNodeTranslator::setArrayMayaValue - unsupported USD data type");
    break;
  }
  return MS::kFailure;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::setSingleMayaValue(MObject node, MObject attr, const UsdAttribute& usdAttr, const UsdDataType type)
{
  switch(type)
  {
  case UsdDataType::kBool:
    {
      bool value;
      usdAttr.Get<bool>(&value);
      return setBool(node, attr, value);
    }

  case UsdDataType::kUChar:
    {
      unsigned char value;
      usdAttr.Get<unsigned char>(&value);
      return setInt8(node, attr, value);
    }

  case UsdDataType::kInt:
    {
      int32_t value;
      usdAttr.Get<int32_t>(&value);
      return setInt32(node, attr, value);
    }

  case UsdDataType::kUInt:
    {
      uint32_t value;
      usdAttr.Get<uint32_t>(&value);
      return setInt32(node, attr, value);
    }

  case UsdDataType::kInt64:
    {
      int64_t value;
      usdAttr.Get<int64_t>(&value);
      return setInt64(node, attr, value);
    }

  case UsdDataType::kUInt64:
    {
      uint64_t value;
      usdAttr.Get<uint64_t>(&value);
      return setInt64(node, attr, value);
    }

  case UsdDataType::kHalf:
    {
      GfHalf value;
      usdAttr.Get<GfHalf>(&value);
      return setFloat(node, attr, value);
    }

  case UsdDataType::kFloat:
    {
      float value;
      usdAttr.Get<float>(&value);
      return setFloat(node, attr, value);
    }

  case UsdDataType::kDouble:
    {
      double value;
      usdAttr.Get<double>(&value);
      return setDouble(node, attr, value);
    }

  case UsdDataType::kString:
    {
      std::string value;
      usdAttr.Get<std::string>(&value);
      return setString(node, attr, value.c_str());
    }

  case UsdDataType::kMatrix2d:
    {
      GfMatrix2d value;
      usdAttr.Get<GfMatrix2d>(&value);
      return setMatrix2x2(node, attr, value.GetArray());
    }

  case UsdDataType::kMatrix3d:
    {
      GfMatrix3d value;
      usdAttr.Get<GfMatrix3d>(&value);
      return setMatrix3x3(node, attr, value.GetArray());
    }

  case UsdDataType::kMatrix4d:
    {
      GfMatrix4d value;
      usdAttr.Get<GfMatrix4d>(&value);
      return setMatrix4x4(node, attr, value.GetArray());
    }

  case UsdDataType::kQuatd:
    {
      GfQuatd value;
      usdAttr.Get<GfQuatd>(&value);
      return setQuat(node, attr, reinterpret_cast<const double*>(&value));
    }

  case UsdDataType::kQuatf:
    {
      GfQuatf value;
      usdAttr.Get<GfQuatf>(&value);
      return setQuat(node, attr, reinterpret_cast<const float*>(&value));
    }

  case UsdDataType::kQuath:
    {
      GfQuath value;
      usdAttr.Get<GfQuath>(&value);
      float xyzw[4];
      xyzw[0] = value.GetImaginary()[0];
      xyzw[1] = value.GetImaginary()[1];
      xyzw[2] = value.GetImaginary()[2];
      xyzw[3] = value.GetReal();
      return setQuat(node, attr, xyzw);
    }

  case UsdDataType::kVec2d:
    {
      GfVec2d value;
      usdAttr.Get<GfVec2d>(&value);
      return setVec2(node, attr, reinterpret_cast<const double*>(&value));
    }

  case UsdDataType::kVec2f:
    {
      GfVec2f value;
      usdAttr.Get<GfVec2f>(&value);
      return setVec2(node, attr, reinterpret_cast<const float*>(&value));
    }

  case UsdDataType::kVec2h:
    {
      GfVec2h value;
      usdAttr.Get<GfVec2h>(&value);
      float data[2];
      data[0] = value[0];
      data[1] = value[1];
      return setVec2(node, attr, data);
    }

  case UsdDataType::kVec2i:
    {
      GfVec2i value;
      usdAttr.Get<GfVec2i>(&value);
      return setVec2(node, attr, reinterpret_cast<const int32_t*>(&value));
    }

  case UsdDataType::kVec3d:
    {
      GfVec3d value;
      usdAttr.Get<GfVec3d>(&value);
      return setVec3(node, attr, reinterpret_cast<const double*>(&value));
    }

  case UsdDataType::kVec3f:
    {
      GfVec3f value;
      usdAttr.Get<GfVec3f>(&value);
      return setVec3(node, attr, reinterpret_cast<const float*>(&value));
    }

  case UsdDataType::kVec3h:
    {
      GfVec3h value;
      usdAttr.Get<GfVec3h>(&value);
      return setVec3(node, attr, value[0], value[1], value[2]);
    }

  case UsdDataType::kVec3i:
    {
      GfVec3i value;
      usdAttr.Get<GfVec3i>(&value);
      return setVec3(node, attr, reinterpret_cast<const int32_t*>(&value));
    }

  case UsdDataType::kVec4d:
    {
      GfVec4d value;
      usdAttr.Get<GfVec4d>(&value);
      return setVec4(node, attr, reinterpret_cast<const double*>(&value));
    }

  case UsdDataType::kVec4f:
    {
      GfVec4f value;
      usdAttr.Get<GfVec4f>(&value);
      return setVec4(node, attr, reinterpret_cast<const float*>(&value));
    }

  case UsdDataType::kVec4h:
    {
      GfVec4h value;
      usdAttr.Get<GfVec4h>(&value);
      float xyzw[4];
      xyzw[0] = value[0];
      xyzw[1] = value[1];
      xyzw[2] = value[2];
      xyzw[3] = value[3];
      return setVec4(node, attr, xyzw);
    }

  case UsdDataType::kVec4i:
    {
      GfVec4i value;
      usdAttr.Get<GfVec4i>(&value);
      return setVec4(node, attr, reinterpret_cast<const int32_t*>(&value));
    }

  default:
    MGlobal::displayError("DgNodeTranslator::setArrayMayaValue - unsupported USD data type");
    break;
  }
  return MS::kFailure;
}

//----------------------------------------------------------------------------------------------------------------------
bool DgNodeTranslator::attributeHandled(const UsdAttribute& usdAttr)
{
  return false;
}



//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::convertSpecialValueToUSDAttribute(const MPlug& plug, UsdAttribute& usdAttr)
{
  // now we start some hard-coded special attribute value type conversion, no better way found:
  // interpolateBoundary: This property comes from alembic, in maya it is boolean type:
  if(usdAttr.GetName() == UsdGeomTokens->interpolateBoundary)
  {
    if(plug.asBool())
      usdAttr.Set(UsdGeomTokens->edgeAndCorner);
    else
      usdAttr.Set(UsdGeomTokens->edgeOnly);

    return MS::kSuccess;
  }
  // more special type conversion rules might come here..
  return MS::kFailure;
}
//----------------------------------------------------------------------------------------------------------------------
MStatus DgNodeTranslator::copyDynamicAttributes(MObject node, UsdPrim& prim)
{
  MFnDependencyNode fn(node);
  uint32_t numAttributes = fn.attributeCount();
  for(uint32_t i = 0; i < numAttributes; ++i)
  {
    MObject attribute = fn.attribute(i);
    MPlug plug(node, attribute);

    // skip child attributes (only export from highest level)
    if(plug.isChild())
      continue;

    bool isDynamic = plug.isDynamic();
    if(isDynamic)
    {
      TfToken attributeName = TfToken(plug.partialName(false, false, false, false, false, true).asChar());
      // first test if the attribute happen to come with the prim by nature and we have a mapping rule for it:
      if(prim.HasAttribute(attributeName))
      {
        UsdAttribute usdAttr = prim.GetAttribute(attributeName);
        // if the conversion works, we are done:
        if(convertSpecialValueToUSDAttribute(plug, usdAttr))
        {
          continue;
        }
        // if not, then we count on CreateAttribute codes below since that will return the USDAttribute if
        // already exists and hopefully the type conversions below will work.
      }

      bool isArray = plug.isArray();
      switch(attribute.apiType())
      {
      case MFn::kAttribute2Double:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Double2);
            GfVec2d m;
            getVec2(node, attribute, (double*)&m);
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Double2Array);
            VtArray<GfVec2d> m;
            m.resize(plug.numElements());
            getVec2Array(node, attribute, (double*)m.data(), m.size());
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kAttribute2Float:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Float2);
            GfVec2f m;
            getVec2(node, attribute, (float*)&m);
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Float2Array);
            VtArray<GfVec2f> m;
            m.resize(plug.numElements());
            getVec2Array(node, attribute, (float*)m.data(), m.size());
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kAttribute2Int:
      case MFn::kAttribute2Short:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Int2);
            GfVec2i m;
            getVec2(node, attribute, (int32_t*)&m);
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Int2Array);
            VtArray<GfVec2i> m;
            m.resize(plug.numElements());
            getVec2Array(node, attribute, (int32_t*)m.data(), m.size());
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kAttribute3Double:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Double3);
            GfVec3d m;
            getVec3(node, attribute, (double*)&m);
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Double3Array);
            VtArray<GfVec3d> m;
            m.resize(plug.numElements());
            getVec3Array(node, attribute, (double*)m.data(), m.size());
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kAttribute3Float:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Float3);
            GfVec3f m;
            getVec3(node, attribute, (float*)&m);
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Float3Array);
            VtArray<GfVec3f> m;
            m.resize(plug.numElements());
            getVec3Array(node, attribute, (float*)m.data(), m.size());
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kAttribute3Long:
      case MFn::kAttribute3Short:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Int3);
            GfVec3i m;
            getVec3(node, attribute, (int32_t*)&m);
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Int3Array);
            VtArray<GfVec3i> m;
            m.resize(plug.numElements());
            getVec3Array(node, attribute, (int32_t*)m.data(), m.size());
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kAttribute4Double:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Double4);
            GfVec4d m;
            getVec4(node, attribute, (double*)&m);
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Double4Array);
            VtArray<GfVec4d> m;
            m.resize(plug.numElements());
            getVec4Array(node, attribute, (double*)m.data(), m.size());
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kNumericAttribute:
        {
          MFnNumericAttribute fn(attribute);
          switch(fn.unitType())
          {
          case MFnNumericData::kBoolean:
            {
              if(!isArray)
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Bool);
                bool value;
                getBool(node, attribute, value);
                usdAttr.Set(value);
                usdAttr.SetCustom(true);
              }
              else
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->BoolArray);
                VtArray<bool> m;
                m.resize(plug.numElements());
                getUsdBoolArray(node, attribute, m);
                usdAttr.Set(m);
                usdAttr.SetCustom(true);
              }
            }
            break;

          case MFnNumericData::kFloat:
            {
              if(!isArray)
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Float);
                float value;
                getFloat(node, attribute, value);
                usdAttr.Set(value);
                usdAttr.SetCustom(true);
              }
              else
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->FloatArray);
                VtArray<float> m;
                m.resize(plug.numElements());
                getFloatArray(node, attribute, (float*)m.data(), m.size());
                usdAttr.Set(m);
                usdAttr.SetCustom(true);
              }
            }
            break;

          case MFnNumericData::kDouble:
            {
              if(!isArray)
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Double);
                double value;
                getDouble(node, attribute, value);
                usdAttr.Set(value);
                usdAttr.SetCustom(true);
              }
              else
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->DoubleArray);
                VtArray<double> m;
                m.resize(plug.numElements());
                getDoubleArray(node, attribute, (double*)m.data(), m.size());
                usdAttr.Set(m);
                usdAttr.SetCustom(true);
              }
            }
            break;

          case MFnNumericData::kInt:
          case MFnNumericData::kShort:
            {
              if(!isArray)
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Int);
                int32_t value;
                getInt32(node, attribute, value);
                usdAttr.Set(value);
                usdAttr.SetCustom(true);
              }
              else
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->IntArray);
                VtArray<int> m;
                m.resize(plug.numElements());
                getInt32Array(node, attribute, (int32_t*)m.data(), m.size());
                usdAttr.Set(m);
                usdAttr.SetCustom(true);
              }
            }
            break;

          case MFnNumericData::kInt64:
            {
              if(!isArray)
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Int64);
                int64_t value;
                getInt64(node, attribute, value);
                usdAttr.Set(value);
                usdAttr.SetCustom(true);
              }
              else
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Int64Array);
                VtArray<int64_t> m;
                m.resize(plug.numElements());
                getInt64Array(node, attribute, (int64_t*)m.data(), m.size());
                usdAttr.Set(m);
                usdAttr.SetCustom(true);
              }
            }
            break;

          case MFnNumericData::kByte:
          case MFnNumericData::kChar:
            {
              if(!isArray)
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->UChar);
                int16_t value;
                getInt16(node, attribute, value);
                usdAttr.Set(uint8_t(value));
                usdAttr.SetCustom(true);
              }
              else
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->UCharArray);
                VtArray<uint8_t> m;
                m.resize(plug.numElements());
                getInt8Array(node, attribute, (int8_t*)m.data(), m.size());
                usdAttr.Set(m);
                usdAttr.SetCustom(true);
              }
            }
            break;

          default:
            {
              std::cout << "Unhandled numeric attribute: " << fn.name().asChar() << " " << fn.unitType() << std::endl;
            }
            break;
          }
        }
        break;

      case MFn::kDoubleAngleAttribute:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Double);
            double value;
            getDouble(node, attribute, value);
            usdAttr.Set(value);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->DoubleArray);
            VtArray<double> value;
            value.resize(plug.numElements());
            getDoubleArray(node, attribute, (double*)value.data(), value.size());
            usdAttr.Set(value);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kFloatAngleAttribute:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Float);
            float value;
            getFloat(node, attribute, value);
            usdAttr.Set(value);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->FloatArray);
            VtArray<float> value;
            value.resize(plug.numElements());
            getFloatArray(node, attribute, (float*)value.data(), value.size());
            usdAttr.Set(value);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kDoubleLinearAttribute:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Double);
            double value;
            getDouble(node, attribute, value);
            usdAttr.Set(value);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->DoubleArray);
            VtArray<double> value;
            value.resize(plug.numElements());
            getDoubleArray(node, attribute, (double*)value.data(), value.size());
            usdAttr.Set(value);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kFloatLinearAttribute:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Float);
            float value;
            getFloat(node, attribute, value);
            usdAttr.Set(value);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->FloatArray);
            VtArray<float> value;
            value.resize(plug.numElements());
            getFloatArray(node, attribute, (float*)value.data(), value.size());
            usdAttr.Set(value);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kTimeAttribute:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Double);
            double value;
            getDouble(node, attribute, value);
            usdAttr.Set(value);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->DoubleArray);
            VtArray<double> value;
            value.resize(plug.numElements());
            getDoubleArray(node, attribute, (double*)value.data(), value.size());
            usdAttr.Set(value);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kEnumAttribute:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Int);
            int32_t value;
            getInt32(node, attribute, value);
            usdAttr.Set(value);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->IntArray);
            VtArray<int> m;
            m.resize(plug.numElements());
            getInt32Array(node, attribute, (int32_t*)m.data(), m.size());
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
        }
        break;

      case MFn::kTypedAttribute:
        {
          MFnTypedAttribute fnTyped(plug.attribute());
          MFnData::Type type = fnTyped.attrType();

          switch(type)
          {
          case MFnData::kString:
            {
              if(!isArray)
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->String);
                std::string value;
                getString(node, attribute, value);
                usdAttr.Set(value);
                usdAttr.SetCustom(true);
              }
              else
              {
                UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->StringArray);
                VtArray<std::string> value;
                value.resize(plug.numElements());
                getStringArray(node, attribute, (std::string*)value.data(), value.size());
                usdAttr.Set(value);
                usdAttr.SetCustom(true);
              }
            }
            break;

          case MFnData::kMatrixArray:
            {
              MFnMatrixArrayData fnData(plug.asMObject());
              UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Matrix4dArray);
              VtArray<GfMatrix4d> m;
              m.assign((const GfMatrix4d*)&fnData.array()[0], ((const GfMatrix4d*)&fnData.array()[0]) + fnData.array().length());
              usdAttr.Set(m);
              usdAttr.SetCustom(true);
            }
            break;

          default:
            {
              std::cout << "Unhandled typed attribute: " << fn.name().asChar() << " " << fn.typeName().asChar() << std::endl;
            }
            break;
          }
        }
        break;

      case MFn::kCompoundAttribute:
        {
          MFnCompoundAttribute fnCompound(plug.attribute());
          {
            if(fnCompound.numChildren() == 2)
            {
              MObject x = fnCompound.child(0);
              MObject y = fnCompound.child(1);
              if(x.apiType() == MFn::kCompoundAttribute &&
                 y.apiType() == MFn::kCompoundAttribute)
              {
                MFnCompoundAttribute fnCompoundX(x);
                MFnCompoundAttribute fnCompoundY(y);

                if(fnCompoundX.numChildren() == 2 && fnCompoundY.numChildren() == 2)
                {
                  MObject xx = fnCompoundX.child(0);
                  MObject xy = fnCompoundX.child(1);
                  MObject yx = fnCompoundY.child(0);
                  MObject yy = fnCompoundY.child(1);
                  if(xx.apiType() == MFn::kNumericAttribute &&
                     xy.apiType() == MFn::kNumericAttribute &&
                     yx.apiType() == MFn::kNumericAttribute &&
                     yy.apiType() == MFn::kNumericAttribute)
                  {
                    if(!isArray)
                    {
                      UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Matrix2d);
                      GfMatrix2d value;
                      getMatrix2x2(node, attribute, (double*)&value);
                      usdAttr.Set(value);
                      usdAttr.SetCustom(true);
                    }
                    else
                    {
                      UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Matrix2dArray);
                      VtArray<GfMatrix2d> value;
                      value.resize(plug.numElements());
                      getMatrix2x2Array(node, attribute, (double*)value.data(), plug.numElements());
                      usdAttr.Set(value);
                      usdAttr.SetCustom(true);
                    }
                  }
                }
              }
            }
            else
            if(fnCompound.numChildren() == 3)
            {
              MObject x = fnCompound.child(0);
              MObject y = fnCompound.child(1);
              MObject z = fnCompound.child(2);
              if(x.apiType() == MFn::kCompoundAttribute &&
                 y.apiType() == MFn::kCompoundAttribute &&
                 z.apiType() == MFn::kCompoundAttribute)
              {
                MFnCompoundAttribute fnCompoundX(x);
                MFnCompoundAttribute fnCompoundY(y);
                MFnCompoundAttribute fnCompoundZ(z);

                if(fnCompoundX.numChildren() == 3 && fnCompoundY.numChildren() == 3 && fnCompoundZ.numChildren() == 3)
                {
                  MObject xx = fnCompoundX.child(0);
                  MObject xy = fnCompoundX.child(1);
                  MObject xz = fnCompoundX.child(2);
                  MObject yx = fnCompoundY.child(0);
                  MObject yy = fnCompoundY.child(1);
                  MObject yz = fnCompoundY.child(2);
                  MObject zx = fnCompoundZ.child(0);
                  MObject zy = fnCompoundZ.child(1);
                  MObject zz = fnCompoundZ.child(2);
                  if(xx.apiType() == MFn::kNumericAttribute &&
                     xy.apiType() == MFn::kNumericAttribute &&
                     xz.apiType() == MFn::kNumericAttribute &&
                     yx.apiType() == MFn::kNumericAttribute &&
                     yy.apiType() == MFn::kNumericAttribute &&
                     yz.apiType() == MFn::kNumericAttribute &&
                     zx.apiType() == MFn::kNumericAttribute &&
                     zy.apiType() == MFn::kNumericAttribute &&
                     zz.apiType() == MFn::kNumericAttribute)
                  {
                    if(!isArray)
                    {
                      UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Matrix3d);
                      GfMatrix3d value;
                      getMatrix3x3(node, attribute, (double*)&value);
                      usdAttr.Set(value);
                      usdAttr.SetCustom(true);
                    }
                    else
                    {
                      UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Matrix3dArray);
                      VtArray<GfMatrix3d> value;
                      value.resize(plug.numElements());
                      getMatrix3x3Array(node, attribute, (double*)value.data(), plug.numElements());
                      usdAttr.Set(value);
                      usdAttr.SetCustom(true);
                    }
                  }
                }
              }
            }
            else
            if(fnCompound.numChildren() == 4)
            {
              MObject x = fnCompound.child(0);
              MObject y = fnCompound.child(1);
              MObject z = fnCompound.child(2);
              MObject w = fnCompound.child(3);
              if(x.apiType() == MFn::kNumericAttribute &&
                 y.apiType() == MFn::kNumericAttribute &&
                 z.apiType() == MFn::kNumericAttribute &&
                 w.apiType() == MFn::kNumericAttribute)
              {
                MFnNumericAttribute fnx(x);
                MFnNumericAttribute fny(y);
                MFnNumericAttribute fnz(z);
                MFnNumericAttribute fnw(w);
                MFnNumericData::Type typex = fnx.unitType();
                MFnNumericData::Type typey = fny.unitType();
                MFnNumericData::Type typez = fnz.unitType();
                MFnNumericData::Type typew = fnw.unitType();
                if(typex == typey && typex == typez && typex == typew)
                {
                  switch(typex)
                  {
                  case MFnNumericData::kInt:
                    {
                      if(!isArray)
                      {
                        UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Int4);
                        GfVec4i value;
                        getVec4(node, attribute, (int32_t*)&value);
                        usdAttr.Set(value);
                        usdAttr.SetCustom(true);
                      }
                      else
                      {
                        UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Int4Array);
                        VtArray<GfVec4i> value;
                        value.resize(plug.numElements());
                        getVec4Array(node, attribute, (int32_t*)value.data(), value.size());
                        usdAttr.Set(value);
                        usdAttr.SetCustom(true);
                      }
                    }
                    break;

                  case MFnNumericData::kFloat:
                    {
                      if(!isArray)
                      {
                        UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Float4);
                        GfVec4f value;
                        getVec4(node, attribute, (float*)&value);
                        usdAttr.Set(value);
                        usdAttr.SetCustom(true);
                      }
                      else
                      {
                        UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Float4Array);
                        VtArray<GfVec4f> value;
                        value.resize(plug.numElements());
                        getVec4Array(node, attribute, (float*)value.data(), value.size());
                        usdAttr.Set(value);
                        usdAttr.SetCustom(true);
                      }
                    }
                    break;

                  case MFnNumericData::kDouble:
                    {
                      if(!isArray)
                      {
                        UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Double4);
                        GfVec4d value;
                        getVec4(node, attribute, (double*)&value);
                        usdAttr.Set(value);
                        usdAttr.SetCustom(true);
                      }
                      else
                      {
                        UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Double4Array);
                        VtArray<GfVec4d> value;
                        value.resize(plug.numElements());
                        getVec4Array(node, attribute, (double*)value.data(), value.size());
                        usdAttr.Set(value);
                        usdAttr.SetCustom(true);
                      }
                    }
                    break;

                  default: break;
                  }
                }
              }
            }
          }
        }
        break;

      case MFn::kFloatMatrixAttribute:
      case MFn::kMatrixAttribute:
        {
          if(!isArray)
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Matrix4d);
            GfMatrix4d m;
            getMatrix4x4(node, attribute, (double*)&m);
            usdAttr.Set(m);
            usdAttr.SetCustom(true);
          }
          else
          {
            UsdAttribute usdAttr = prim.CreateAttribute(attributeName, SdfValueTypeNames->Matrix4dArray);
            VtArray<GfMatrix4d> value;
            value.resize(plug.numElements());
            getMatrix4x4Array(node, attribute, (double*)value.data(), value.size());
            usdAttr.Set(value);
            usdAttr.SetCustom(true);
          }
        }
        break;

      default: break;
      }
    }
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
void DgNodeTranslator::copySimpleValue(const MPlug& plug, UsdAttribute& usdAttr, const UsdTimeCode& timeCode)
{
  MObject node = plug.node();
  MObject attribute = plug.attribute();
  bool isArray = plug.isArray();
  switch(getAttributeType(usdAttr))
  {
  case UsdDataType::kUChar:
    if(!isArray)
    {
      int8_t value;
      getInt8(node, attribute, value);
      usdAttr.Set(uint8_t(value), timeCode);
    }
    else
    {
      VtArray<uint8_t> m;
      m.resize(plug.numElements());
      getInt8Array(node, attribute, (int8_t*)m.data(), m.size());
      usdAttr.Set(m, timeCode);
    }
    break;

  case UsdDataType::kInt:
    if(!isArray)
    {
      int32_t value;
      getInt32(node, attribute, value);
      usdAttr.Set(value, timeCode);
    }
    else
    {
      VtArray<int32_t> m;
      m.resize(plug.numElements());
      getInt32Array(node, attribute, (int32_t*)m.data(), m.size());
      usdAttr.Set(m, timeCode);
    }
    break;

  case UsdDataType::kUInt:
    if(!isArray)
    {
      int32_t value;
      getInt32(node, attribute, value);
      usdAttr.Set(uint32_t(value), timeCode);
    }
    else
    {
      VtArray<uint32_t> m;
      m.resize(plug.numElements());
      getInt32Array(node, attribute, (int32_t*)m.data(), m.size());
      usdAttr.Set(m, timeCode);
    }
    break;

  case UsdDataType::kInt64:
    if(!isArray)
    {
      int64_t value;
      getInt64(node, attribute, value);
      usdAttr.Set(value, timeCode);
    }
    else
    {
      VtArray<int64_t> m;
      m.resize(plug.numElements());
      getInt64Array(node, attribute, (int64_t*)m.data(), m.size());
      usdAttr.Set(m, timeCode);
    }
    break;

  case UsdDataType::kUInt64:
    if(!isArray)
    {
      int64_t value;
      getInt64(node, attribute, value);
      usdAttr.Set(value, timeCode);
    }
    else
    {
      VtArray<int64_t> m;
      m.resize(plug.numElements());
      getInt64Array(node, attribute, (int64_t*)m.data(), m.size());
      usdAttr.Set(m, timeCode);
    }
    break;

  case UsdDataType::kFloat:
    if(!isArray)
    {
      float value;
      getFloat(node, attribute, value);
      usdAttr.Set(value, timeCode);
    }
    else
    {
      VtArray<float> m;
      m.resize(plug.numElements());
      getFloatArray(node, attribute, (float*)m.data(), m.size());
      usdAttr.Set(m, timeCode);
    }
    break;

  case UsdDataType::kDouble:
    if(!isArray)
    {
      double value;
      getDouble(node, attribute, value);
      usdAttr.Set(value, timeCode);
    }
    else
    {
      VtArray<double> m;
      m.resize(plug.numElements());
      getDoubleArray(node, attribute, (double*)m.data(), m.size());
      usdAttr.Set(m, timeCode);
    }
    break;

  case UsdDataType::kHalf:
    if(!isArray)
    {
      GfHalf value;
      getHalf(node, attribute, value);
      usdAttr.Set(value, timeCode);
    }
    else
    {
      VtArray<GfHalf> m;
      m.resize(plug.numElements());
      getHalfArray(node, attribute, (GfHalf*)m.data(), m.size());
      usdAttr.Set(m, timeCode);
    }
    break;

  default:
    break;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void DgNodeTranslator::copyAttributeValue(const MPlug& plug, UsdAttribute& usdAttr, const UsdTimeCode& timeCode)
{
  MObject node = plug.node();
  MObject attribute = plug.attribute();
  bool isArray = plug.isArray();
  switch(attribute.apiType())
  {
  case MFn::kAttribute2Double:
  case MFn::kAttribute2Float:
  case MFn::kAttribute2Int:
  case MFn::kAttribute2Short:
    {
      switch(getAttributeType(usdAttr))
      {
      case UsdDataType::kVec2d:
        if(!isArray)
        {
          GfVec2d m;
          getVec2(node, attribute, (double*)&m);
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec2d> m;
          m.resize(plug.numElements());
          getVec2Array(node, attribute, (double*)m.data(), m.size());
          usdAttr.Set(m, timeCode);
        }
        break;

      case UsdDataType::kVec2f:
        if(!isArray)
        {
          GfVec2f m;
          getVec2(node, attribute, (float*)&m);
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec2f> m;
          m.resize(plug.numElements());
          getVec2Array(node, attribute, (float*)m.data(), m.size());
          usdAttr.Set(m, timeCode);
        }
        break;

      case UsdDataType::kVec2i:
        if(!isArray)
        {
          GfVec2i m;
          getVec2(node, attribute, (int*)&m);
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec2i> m;
          m.resize(plug.numElements());
          getVec2Array(node, attribute, (int*)m.data(), m.size());
          usdAttr.Set(m, timeCode);
        }
        break;

      case UsdDataType::kVec2h:
        if(!isArray)
        {
          GfVec2h m;
          getVec2(node, attribute, (GfHalf*)&m);
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec2h> m;
          m.resize(plug.numElements());
          getVec2Array(node, attribute, (GfHalf*)m.data(), m.size());
          usdAttr.Set(m, timeCode);
        }
        break;

      default:
        break;
      }
    }
    break;

  case MFn::kAttribute3Double:
  case MFn::kAttribute3Float:
  case MFn::kAttribute3Long:
  case MFn::kAttribute3Short:
    {
      switch(getAttributeType(usdAttr))
      {
      case UsdDataType::kVec3d:
        if(!isArray)
        {
          GfVec3d m;
          getVec3(node, attribute, (double*)&m);
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec3d> m;
          m.resize(plug.numElements());
          getVec3Array(node, attribute, (double*)m.data(), m.size());
          usdAttr.Set(m, timeCode);
        }
        break;

      case UsdDataType::kVec3f:
        if(!isArray)
        {
          GfVec3f m;
          getVec3(node, attribute, (float*)&m);
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec3f> m;
          m.resize(plug.numElements());
          getVec3Array(node, attribute, (float*)m.data(), m.size());
          usdAttr.Set(m, timeCode);
        }
        break;

      case UsdDataType::kVec3i:
        if(!isArray)
        {
          GfVec3i m;
          getVec3(node, attribute, (int*)&m);
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec3i> m;
          m.resize(plug.numElements());
          getVec3Array(node, attribute, (int*)m.data(), m.size());
          usdAttr.Set(m, timeCode);
        }
        break;

      case UsdDataType::kVec3h:
        if(!isArray)
        {
          GfVec3h m;
          getVec3(node, attribute, (GfHalf*)&m);
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec3h> m;
          m.resize(plug.numElements());
          getVec3Array(node, attribute, (GfHalf*)m.data(), m.size());
          usdAttr.Set(m, timeCode);
        }
        break;

      default:
        break;
      }
    }
    break;

  case MFn::kAttribute4Double:
    {
      switch(getAttributeType(usdAttr))
      {
      case UsdDataType::kVec4d:
        if(!isArray)
        {
          GfVec4d m;
          getVec4(node, attribute, (double*)&m);
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec4d> m;
          m.resize(plug.numElements());
          getVec4Array(node, attribute, (double*)m.data(), m.size());
          usdAttr.Set(m, timeCode);
        }
        break;

      case UsdDataType::kVec4f:
        if(!isArray)
        {
          GfVec4f m;
          getVec4(node, attribute, (float*)&m);
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec4f> m;
          m.resize(plug.numElements());
          getVec4Array(node, attribute, (float*)m.data(), m.size());
          usdAttr.Set(m, timeCode);
        }
        break;

      case UsdDataType::kVec4i:
        if(!isArray)
        {
          GfVec4i m;
          getVec4(node, attribute, (int*)&m);
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec4i> m;
          m.resize(plug.numElements());
          getVec4Array(node, attribute, (int*)m.data(), m.size());
          usdAttr.Set(m, timeCode);
        }
        break;

      case UsdDataType::kVec4h:
        if(!isArray)
        {
          GfVec4h m;
          getVec4(node, attribute, (GfHalf*)&m);
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec4h> m;
          m.resize(plug.numElements());
          getVec4Array(node, attribute, (GfHalf*)m.data(), m.size());
          usdAttr.Set(m, timeCode);
        }
        break;

      default:
        break;
      }
    }
    break;

  case MFn::kNumericAttribute:
    {
      MFnNumericAttribute fn(attribute);
      switch(fn.unitType())
      {
      case MFnNumericData::kBoolean:
        {
          if(!isArray)
          {
            bool value;
            getBool(node, attribute, value);
            usdAttr.Set(value, timeCode);
          }
          else
          {
            VtArray<bool> m;
            m.resize(plug.numElements());
            getUsdBoolArray(node, attribute, m);
            usdAttr.Set(m, timeCode);
          }
        }
        break;

      case MFnNumericData::kFloat:
      case MFnNumericData::kDouble:
      case MFnNumericData::kInt:
      case MFnNumericData::kShort:
      case MFnNumericData::kInt64:
      case MFnNumericData::kByte:
      case MFnNumericData::kChar:
        {
          copySimpleValue(plug, usdAttr, timeCode);
        }
        break;

      default:
        {
        }
        break;
      }
    }
    break;

  case MFn::kTimeAttribute:
  case MFn::kFloatAngleAttribute:
  case MFn::kDoubleAngleAttribute:
  case MFn::kDoubleLinearAttribute:
  case MFn::kFloatLinearAttribute:
    {
      copySimpleValue(plug, usdAttr, timeCode);
    }
    break;

  case MFn::kEnumAttribute:
    {
      if(!isArray)
      {
        int32_t value;
        getInt32(node, attribute, value);
        usdAttr.Set(value, timeCode);
      }
      else
      {
        VtArray<int> m;
        m.resize(plug.numElements());
        getInt32Array(node, attribute, (int32_t*)m.data(), m.size());
        usdAttr.Set(m, timeCode);
      }
    }
    break;

  case MFn::kTypedAttribute:
    {
      MFnTypedAttribute fnTyped(plug.attribute());
      MFnData::Type type = fnTyped.attrType();

      switch(type)
      {
      case MFnData::kString:
        {
          // don't animate strings
        }
        break;

      case MFnData::kMatrixArray:
        {
          MFnMatrixArrayData fnData(plug.asMObject());
          VtArray<GfMatrix4d> m;
          m.assign((const GfMatrix4d*)&fnData.array()[0], ((const GfMatrix4d*)&fnData.array()[0]) + fnData.array().length());
          usdAttr.Set(m, timeCode);
        }
        break;

      default:
        {
        }
        break;
      }
    }
    break;

  case MFn::kCompoundAttribute:
    {
      MFnCompoundAttribute fnCompound(plug.attribute());
      {
        if(fnCompound.numChildren() == 2)
        {
          MObject x = fnCompound.child(0);
          MObject y = fnCompound.child(1);
          if(x.apiType() == MFn::kCompoundAttribute &&
             y.apiType() == MFn::kCompoundAttribute)
          {
            MFnCompoundAttribute fnCompoundX(x);
            MFnCompoundAttribute fnCompoundY(y);

            if(fnCompoundX.numChildren() == 2 && fnCompoundY.numChildren() == 2)
            {
              MObject xx = fnCompoundX.child(0);
              MObject xy = fnCompoundX.child(1);
              MObject yx = fnCompoundY.child(0);
              MObject yy = fnCompoundY.child(1);
              if(xx.apiType() == MFn::kNumericAttribute &&
                 xy.apiType() == MFn::kNumericAttribute &&
                 yx.apiType() == MFn::kNumericAttribute &&
                 yy.apiType() == MFn::kNumericAttribute)
              {
                if(!isArray)
                {
                  GfMatrix2d value;
                  getMatrix2x2(node, attribute, (double*)&value);
                  usdAttr.Set(value, timeCode);
                }
                else
                {
                  VtArray<GfMatrix2d> value;
                  value.resize(plug.numElements());
                  getMatrix2x2Array(node, attribute, (double*)value.data(), plug.numElements());
                  usdAttr.Set(value, timeCode);
                }
              }
            }
          }
        }
        else
        if(fnCompound.numChildren() == 3)
        {
          MObject x = fnCompound.child(0);
          MObject y = fnCompound.child(1);
          MObject z = fnCompound.child(2);
          if(x.apiType() == MFn::kCompoundAttribute &&
             y.apiType() == MFn::kCompoundAttribute &&
             z.apiType() == MFn::kCompoundAttribute)
          {
            MFnCompoundAttribute fnCompoundX(x);
            MFnCompoundAttribute fnCompoundY(y);
            MFnCompoundAttribute fnCompoundZ(z);

            if(fnCompoundX.numChildren() == 3 && fnCompoundY.numChildren() == 3 && fnCompoundZ.numChildren() == 3)
            {
              MObject xx = fnCompoundX.child(0);
              MObject xy = fnCompoundX.child(1);
              MObject xz = fnCompoundX.child(2);
              MObject yx = fnCompoundY.child(0);
              MObject yy = fnCompoundY.child(1);
              MObject yz = fnCompoundY.child(2);
              MObject zx = fnCompoundZ.child(0);
              MObject zy = fnCompoundZ.child(1);
              MObject zz = fnCompoundZ.child(2);
              if(xx.apiType() == MFn::kNumericAttribute &&
                 xy.apiType() == MFn::kNumericAttribute &&
                 xz.apiType() == MFn::kNumericAttribute &&
                 yx.apiType() == MFn::kNumericAttribute &&
                 yy.apiType() == MFn::kNumericAttribute &&
                 yz.apiType() == MFn::kNumericAttribute &&
                 zx.apiType() == MFn::kNumericAttribute &&
                 zy.apiType() == MFn::kNumericAttribute &&
                 zz.apiType() == MFn::kNumericAttribute)
              {
                if(!isArray)
                {
                  GfMatrix3d value;
                  getMatrix3x3(node, attribute, (double*)&value);
                  usdAttr.Set(value, timeCode);
                }
                else
                {
                  VtArray<GfMatrix3d> value;
                  value.resize(plug.numElements());
                  getMatrix3x3Array(node, attribute, (double*)value.data(), plug.numElements());
                  usdAttr.Set(value, timeCode);
                }
              }
            }
          }
        }
        else
        if(fnCompound.numChildren() == 4)
        {
          MObject x = fnCompound.child(0);
          MObject y = fnCompound.child(1);
          MObject z = fnCompound.child(2);
          MObject w = fnCompound.child(3);
          if(x.apiType() == MFn::kNumericAttribute &&
             y.apiType() == MFn::kNumericAttribute &&
             z.apiType() == MFn::kNumericAttribute &&
             w.apiType() == MFn::kNumericAttribute)
          {
            MFnNumericAttribute fnx(x);
            MFnNumericAttribute fny(y);
            MFnNumericAttribute fnz(z);
            MFnNumericAttribute fnw(w);
            MFnNumericData::Type typex = fnx.unitType();
            MFnNumericData::Type typey = fny.unitType();
            MFnNumericData::Type typez = fnz.unitType();
            MFnNumericData::Type typew = fnw.unitType();
            if(typex == typey && typex == typez && typex == typew)
            {
              switch(typex)
              {
              case MFnNumericData::kInt:
                {
                  if(!isArray)
                  {
                    GfVec4i value;
                    getVec4(node, attribute, (int32_t*)&value);
                    usdAttr.Set(value, timeCode);
                  }
                  else
                  {
                    VtArray<GfVec4i> value;
                    value.resize(plug.numElements());
                    getVec4Array(node, attribute, (int32_t*)value.data(), value.size());
                    usdAttr.Set(value, timeCode);
                  }
                }
                break;

              case MFnNumericData::kFloat:
                {
                  if(!isArray)
                  {
                    GfVec4f value;
                    getVec4(node, attribute, (float*)&value);
                    usdAttr.Set(value, timeCode);
                  }
                  else
                  {
                    VtArray<GfVec4f> value;
                    value.resize(plug.numElements());
                    getVec4Array(node, attribute, (float*)value.data(), value.size());
                    usdAttr.Set(value, timeCode);
                  }
                }
                break;

              case MFnNumericData::kDouble:
                {
                  if(!isArray)
                  {
                    GfVec4d value;
                    getVec4(node, attribute, (double*)&value);
                    usdAttr.Set(value, timeCode);
                  }
                  else
                  {
                    VtArray<GfVec4d> value;
                    value.resize(plug.numElements());
                    getVec4Array(node, attribute, (double*)value.data(), value.size());
                    usdAttr.Set(value, timeCode);
                  }
                }
                break;

              default: break;
              }
            }
          }
        }
      }
    }
    break;

  case MFn::kFloatMatrixAttribute:
  case MFn::kMatrixAttribute:
    {
      if(!isArray)
      {
        GfMatrix4d m;
        getMatrix4x4(node, attribute, (double*)&m);
        usdAttr.Set(m, timeCode);
      }
      else
      {
        VtArray<GfMatrix4d> value;
        value.resize(plug.numElements());
        getMatrix4x4Array(node, attribute, (double*)value.data(), value.size());
        usdAttr.Set(value, timeCode);
      }
    }
    break;

  default: break;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void DgNodeTranslator::copySimpleValue(const MPlug& plug, UsdAttribute& usdAttr, const float scale, const UsdTimeCode& timeCode)
{
  MObject node = plug.node();
  MObject attribute = plug.attribute();
  bool isArray = plug.isArray();
  switch(getAttributeType(usdAttr))
  {
  case UsdDataType::kFloat:
    if(!isArray)
    {
      float value;
      getFloat(node, attribute, value);
      usdAttr.Set(value * scale, timeCode);
    }
    else
    {
      VtArray<float> m;
      m.resize(plug.numElements());
      getFloatArray(node, attribute, (float*)m.data(), m.size());
      for(auto it = m.begin(), e = m.end(); it != e; ++it)
      {
        *it *= scale;
      }
      usdAttr.Set(m, timeCode);
    }
    break;

  case UsdDataType::kDouble:
    if(!isArray)
    {
      double value;
      getDouble(node, attribute, value);
      usdAttr.Set(value * scale, timeCode);
    }
    else
    {
      VtArray<double> m;
      m.resize(plug.numElements());
      getDoubleArray(node, attribute, (double*)m.data(), m.size());
      double temp = scale;
      for(auto it = m.begin(), e = m.end(); it != e; ++it)
      {
        *it *= temp;
      }
      usdAttr.Set(m, timeCode);
    }
    break;

  default:
    break;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void DgNodeTranslator::copyAttributeValue(const MPlug& plug, UsdAttribute& usdAttr, const float scale, const UsdTimeCode& timeCode)
{
  MObject node = plug.node();
  MObject attribute = plug.attribute();
  bool isArray = plug.isArray();
  switch(attribute.apiType())
  {
  case MFn::kAttribute2Double:
  case MFn::kAttribute2Float:
  case MFn::kAttribute2Int:
  case MFn::kAttribute2Short:
    {
      switch(getAttributeType(usdAttr))
      {
      case UsdDataType::kVec2d:
        if(!isArray)
        {
          GfVec2d m;
          getVec2(node, attribute, (double*)&m);
          m *= scale;
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec2d> m;
          m.resize(plug.numElements());
          getVec2Array(node, attribute, (double*)m.data(), m.size());
          double temp = scale;
          for(auto it = m.begin(), e = m.end(); it != e; ++it)
          {
            *it *= temp;
          }
          usdAttr.Set(m, timeCode);
        }
        break;

      case UsdDataType::kVec2f:
        if(!isArray)
        {
          GfVec2f m;
          getVec2(node, attribute, (float*)&m);
          m *= scale;
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec2f> m;
          m.resize(plug.numElements());
          getVec2Array(node, attribute, (float*)m.data(), m.size());
          for(auto it = m.begin(), e = m.end(); it != e; ++it)
          {
            *it *= scale;
          }
          usdAttr.Set(m, timeCode);
        }
        break;

      default:
        break;
      }
    }
    break;

  case MFn::kAttribute3Double:
  case MFn::kAttribute3Float:
  case MFn::kAttribute3Long:
  case MFn::kAttribute3Short:
    {
      switch(getAttributeType(usdAttr))
      {
      case UsdDataType::kVec3d:
        if(!isArray)
        {
          GfVec3d m;
          getVec3(node, attribute, (double*)&m);
          m *= scale;
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec3d> m;
          m.resize(plug.numElements());
          getVec3Array(node, attribute, (double*)m.data(), m.size());
          double temp = scale;
          for(auto it = m.begin(), e = m.end(); it != e; ++it)
          {
            *it *= temp;
          }
          usdAttr.Set(m, timeCode);
        }
        break;

      case UsdDataType::kVec3f:
        if(!isArray)
        {
          GfVec3f m;
          getVec3(node, attribute, (float*)&m);
          m *= scale;
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec3f> m;
          m.resize(plug.numElements());
          getVec3Array(node, attribute, (float*)m.data(), m.size());
          for(auto it = m.begin(), e = m.end(); it != e; ++it)
          {
            *it *= scale;
          }
          usdAttr.Set(m, timeCode);
        }
        break;

      default:
        break;
      }
    }
    break;

  case MFn::kAttribute4Double:
    {
      switch(getAttributeType(usdAttr))
      {
      case UsdDataType::kVec4d:
        if(!isArray)
        {
          GfVec4d m;
          getVec4(node, attribute, (double*)&m);
          m *= scale;
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec4d> m;
          m.resize(plug.numElements());
          getVec4Array(node, attribute, (double*)m.data(), m.size());
          double temp = scale;
          for(auto it = m.begin(), e = m.end(); it != e; ++it)
          {
            *it *= temp;
          }
          usdAttr.Set(m, timeCode);
        }
        break;

      case UsdDataType::kVec4f:
        if(!isArray)
        {
          GfVec4f m;
          getVec4(node, attribute, (float*)&m);
          m *= scale;
          usdAttr.Set(m, timeCode);
        }
        else
        {
          VtArray<GfVec4f> m;
          m.resize(plug.numElements());
          getVec4Array(node, attribute, (float*)m.data(), m.size());
          for(auto it = m.begin(), e = m.end(); it != e; ++it)
          {
            *it *= scale;
          }
          usdAttr.Set(m, timeCode);
        }
        break;

      default:
        break;
      }
    }
    break;

  case MFn::kNumericAttribute:
    {
      MFnNumericAttribute fn(attribute);
      switch(fn.unitType())
      {
      case MFnNumericData::kFloat:
      case MFnNumericData::kDouble:
      case MFnNumericData::kInt:
      case MFnNumericData::kShort:
      case MFnNumericData::kInt64:
      case MFnNumericData::kByte:
      case MFnNumericData::kChar:
        {
          copySimpleValue(plug, usdAttr, scale, timeCode);
        }
        break;

      default:
        {
        }
        break;
      }
    }
    break;

  case MFn::kTimeAttribute:
  case MFn::kFloatAngleAttribute:
  case MFn::kDoubleAngleAttribute:
  case MFn::kDoubleLinearAttribute:
  case MFn::kFloatLinearAttribute:
    {
      copySimpleValue(plug, usdAttr, scale, timeCode);
    }
    break;

  default: break;
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
