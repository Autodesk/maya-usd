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
#include "maya/MPlug.h"
#include "maya/MAngle.h"
#include "maya/MDistance.h"
#include "maya/MTime.h"

#include <string>
#include <vector>

#include "pxr/base/gf/half.h" //Just for convenient half support
#include "pxr/pxr.h"
#include "AL/usdmaya/utils/ForwardDeclares.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace utils {
//----------------------------------------------------------------------------------------------------------------------
/// \ingroup  mayautils
/// \brief  Utility class that provides support for setting/getting
///         attributes.
//----------------------------------------------------------------------------------------------------------------------
struct DgNodeHelper
{
public:

  /// ctor
  DgNodeHelper() {}

  /// dtor
  virtual ~DgNodeHelper() {}

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Methods to get array data from array attributes
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  retrieve an array of boolean values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values the returned array of values
  /// \return MS::kSuccess if ok
  static MStatus getBoolArray(const MObject& node, const MObject& attr, std::vector<bool>& values);

  /// \brief  retrieve an array of boolean values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values a pointer to a pre-allocated buffer to fill with the attribute values
  /// \param  count the number of elements in the buffer.
  /// \return MS::kSuccess if ok
  static MStatus getBoolArray(MObject node, MObject attr, bool* values, const size_t count);

  /// \brief  retrieve an array of 8 bit char values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values the returned array of values
  /// \return MS::kSuccess if ok
  static MStatus getInt8Array(const MObject& node, const MObject& attr, std::vector<int8_t>& values);

  /// \brief  retrieve an array of 8 bit integer values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values a pointer to a pre-allocated buffer to fill with the attribute values
  /// \param  count the number of elements in the buffer.
  /// \return MS::kSuccess if ok
  static MStatus getInt8Array(MObject node, MObject attr, int8_t* values, size_t count);

  /// \brief  retrieve an array of 16bit integer values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values the returned array of values
  /// \return MS::kSuccess if ok
  static MStatus getInt16Array(const MObject& node, const MObject& attr, std::vector<int16_t>& values);

  /// \brief  retrieve an array of 16 bit integer values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values a pointer to a pre-allocated buffer to fill with the attribute values
  /// \param  count the number of elements in the buffer.
  /// \return MS::kSuccess if ok
  static MStatus getInt16Array(MObject node, MObject attr, int16_t* values, size_t count);

  /// \brief  retrieve an array of 32bit integer values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values the returned array of values
  /// \return MS::kSuccess if ok
  static MStatus getInt32Array(const MObject& node, const MObject& attr, std::vector<int32_t>& values);

  /// \brief  retrieve an array of 32 bit integer values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values a pointer to a pre-allocated buffer to fill with the attribute values
  /// \param  count the number of elements in the buffer.
  /// \return MS::kSuccess if ok
  static MStatus getInt32Array(MObject node, MObject attr, int32_t* values, size_t count);

  /// \brief  retrieve an array of 64bit integer values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values the returned array of values
  /// \return MS::kSuccess if ok
  static MStatus getInt64Array(const MObject& node, const MObject& attr, std::vector<int64_t>& values);

  /// \brief  retrieve an array of 64 bit integer values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values a pointer to a pre-allocated buffer to fill with the attribute values
  /// \param  count the number of elements in the buffer.
  /// \return MS::kSuccess if ok
  static MStatus getInt64Array(MObject node, MObject attr, int64_t* values, size_t count);

  /// \brief  retrieve an array of float values from an attribute in Maya (converted to halfs)
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values the returned array of values
  /// \return MS::kSuccess if ok
  static MStatus getHalfArray(const MObject& node, const MObject& attr, std::vector<GfHalf>& values);

  /// \brief  retrieve an array of half values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values a pointer to a pre-allocated buffer to fill with the attribute values
  /// \param  count the number of elements in the buffer.
  /// \return MS::kSuccess if ok
  static MStatus getHalfArray(MObject node, MObject attr, GfHalf* values, size_t count);

  /// \brief  retrieve an array of float values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values the returned array of values
  /// \return MS::kSuccess if ok
  static MStatus getFloatArray(const MObject& node, const MObject& attr, std::vector<float>& values);

  /// \brief  retrieve an array of float values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values a pointer to a pre-allocated buffer to fill with the attribute values
  /// \param  count the number of elements in the buffer.
  /// \return MS::kSuccess if ok
  static MStatus getFloatArray(MObject node, MObject attr, float* values, size_t count);

  /// \brief  retrieve an array of double values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values the returned array of values
  /// \return MS::kSuccess if ok
  static MStatus getDoubleArray(const MObject& node, const MObject& attr, std::vector<double>& values);

  /// \brief  retrieve an array of double values from an attribute in Maya
  /// \param  node the maya node on which the attribute you are interested in exists
  /// \param  attr the handle to the array attribute. This will either be an MObject for a custom maya attribute,
  ///         a handle queried via the MNodeClass interface, or a dynamically added attribute
  /// \param  values a pointer to a pre-allocated buffer to fill with the attribute values
  /// \param  count the number of elements in the buffer.
  /// \return MS::kSuccess if ok
  static MStatus getDoubleArray(MObject node, MObject attr, double* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 2D half float array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 2x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getVec2Array(MObject node, MObject attr, GfHalf* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 2D float array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 2x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getVec2Array(MObject node, MObject attr, float* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 2D double array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 2x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getVec2Array(MObject node, MObject attr, double* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 2D integer array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 2x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getVec2Array(MObject node, MObject attr, int32_t* values, size_t count);


  /// \brief  given MObjects for an attribute on a node, extract the data as a 3D half float array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 3x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getVec3Array(MObject node, MObject attr, GfHalf* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 3D float array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 3x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getVec3Array(MObject node, MObject attr, float* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 3D double array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 3x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getVec3Array(MObject node, MObject attr, double* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 3D integer array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 3x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getVec3Array(MObject node, MObject attr, int32_t* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 4D half float array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 4x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getVec4Array(MObject node, MObject attr, GfHalf* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 4D float array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 4x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getVec4Array(MObject node, MObject attr, float* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 4D double array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 4x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getVec4Array(MObject node, MObject attr, double* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 4D integer array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 4x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getVec4Array(MObject node, MObject attr, int32_t* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 4D half float array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 4x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getQuatArray(MObject node, MObject attr, GfHalf* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 4D float array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 4x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getQuatArray(MObject node, MObject attr, float* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 4D double array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 4x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getQuatArray(MObject node, MObject attr, double* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 2x2 floating point matrix array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 4x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getMatrix2x2Array(MObject node, MObject attr, float* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 2x2 double matrix array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 4x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getMatrix2x2Array(MObject node, MObject attr, double* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 3x3 floating point matrix array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 9x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getMatrix3x3Array(MObject node, MObject attr, float* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 3x3 double matrix array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 9x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getMatrix3x3Array(MObject node, MObject attr, double* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 4x4 floating point matrix array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 16x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getMatrix4x4Array(MObject node, MObject attr, float* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as a 4x4 double matrix array
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of matrices to extract (values should be 16x this size)
  /// \return MS::kSuccess if everything is OK
  static MStatus getMatrix4x4Array(MObject node, MObject attr, double* values, size_t count);

  /// \brief  given MObjects for an attribute on a node, extract the data as an array of time values scale
  ///         to the specified unit.
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of values to extract
  /// \param  unit the time unit you want the data in
  /// \return MS::kSuccess if everything is OK
  static MStatus getTimeArray(MObject node, MObject attr, float* values, size_t count, MTime::Unit unit);

  /// \brief  given MObjects for an attribute on a node, extract the data as an array of angle values scale
  ///         to the specified unit.
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of values to extract
  /// \param  unit the angle unit you want the data in
  /// \return MS::kSuccess if everything is OK
  static MStatus getAngleArray(MObject node, MObject attr, float* values, size_t count, MAngle::Unit unit);

  /// \brief  given MObjects for an attribute on a node, extract the data as an array of distance values scale
  ///         to the specified unit.
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of values to extract
  /// \param  unit the distance unit you want the data in
  /// \return MS::kSuccess if everything is OK
  static MStatus getDistanceArray(MObject node, MObject attr, float* values, size_t count, MDistance::Unit unit);

  /// \brief  given MObjects for an attribute on a node, extract the data as an array of string values
  /// \param  node a handle to the node to get the attribute from
  /// \param  attr a handle to the attribute that contains the array data you wish to extract
  /// \param  values the pre-allocated buffer into which you wish to get the data
  /// \param  count the number of values to extract
  /// \return MS::kSuccess if everything is OK
  static MStatus getStringArray(MObject node, MObject attr, std::string* values, size_t count);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Methods to get single values from non array attributes
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  extracts a single half float value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getHalf(MObject node, MObject attr, GfHalf& value)
  {
    float f;
    MStatus status = getFloat(node, attr, f);
    value = f;
    return status;
  }

  /// \brief  extracts a single float value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getFloat(MObject node, MObject attr, float& value);

  /// \brief  extracts a single double value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getDouble(MObject node, MObject attr, double& value);

  /// \brief  extracts a single time value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getTime(MObject node, MObject attr, MTime& value);

  /// \brief  extracts a single distance value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getDistance(MObject node, MObject attr, MDistance& value);

  /// \brief  extracts a single angle value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getAngle(MObject node, MObject attr, MAngle& value);

  /// \brief  extracts a single boolean value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getBool(MObject node, MObject attr, bool& value);

  /// \brief  extracts a single 8bit integer value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getInt8(MObject node, MObject attr, int8_t& value);

  /// \brief  extracts a single 16 bit integer value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getInt16(MObject node, MObject attr, int16_t& value);

  /// \brief  extracts a single 32bit integer value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getInt32(MObject node, MObject attr, int32_t& value);

  /// \brief  extracts a single 64bit integer value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getInt64(MObject node, MObject attr, int64_t& value);

  /// \brief  extracts a 2x2 matrix value from the specified node/attribute (as a float)
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the returned matrix value as an array of floats
  /// \return MS::kSuccess if all ok
  static MStatus getMatrix2x2(MObject node, MObject attr, float* values);

  /// \brief  extracts a 3x3 matrix value from the specified node/attribute (as a float)
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the returned matrix value as an array of floats
  /// \return MS::kSuccess if all ok
  static MStatus getMatrix3x3(MObject node, MObject attr, float* values);

  /// \brief  extracts a 4x4 matrix value from the specified node/attribute (as a float)
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the returned matrix value as an array of floats
  /// \return MS::kSuccess if all ok
  static MStatus getMatrix4x4(MObject node, MObject attr, float* values);

  /// \brief  extracts a 4x4 matrix value from the specified node/attribute (as a float)
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the returned matrix value
  /// \return MS::kSuccess if all ok
  static MStatus getMatrix4x4(MObject node, MObject attr, MFloatMatrix& values);

  /// \brief  extracts a 2x2 matrix value from the specified node/attribute (as a double)
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the returned matrix value as an array of doubles
  /// \return MS::kSuccess if all ok
  static MStatus getMatrix2x2(MObject node, MObject attr, double* values);

  /// \brief  extracts a 3x3 matrix value from the specified node/attribute (as a double)
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the returned matrix value as an array of doubles
  /// \return MS::kSuccess if all ok
  static MStatus getMatrix3x3(MObject node, MObject attr, double* values);

  /// \brief  extracts a 4x4 matrix value from the specified node/attribute (as a double)
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the returned matrix value as an array of doubles
  /// \return MS::kSuccess if all ok
  static MStatus getMatrix4x4(MObject node, MObject attr, double* values);

  /// \brief  extracts a 4x4 matrix value from the specified node/attribute (as a double)
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the returned matrix value
  /// \return MS::kSuccess if all ok
  static MStatus getMatrix4x4(MObject node, MObject attr, MMatrix& values);

  /// \brief  extracts a string value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  str the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getString(MObject node, MObject attr, std::string& str);

  /// \brief  extracts a 2D vector value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xy the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getVec2(MObject node, MObject attr, int32_t* xy);

  /// \brief  extracts a 2D vector value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xy the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getVec2(MObject node, MObject attr, float* xy);

  /// \brief  extracts a 2D vector value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xy the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getVec2(MObject node, MObject attr, double* xy);

  /// \brief  extracts a 2D vector value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xy the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getVec2(MObject node, MObject attr, GfHalf* xy);

  /// \brief  extracts a 3D vector value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyz the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getVec3(MObject node, MObject attr, int32_t* xyz);

  /// \brief  extracts a 3D vector value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyz the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getVec3(MObject node, MObject attr, float* xyz);

  /// \brief  extracts a 3D vector value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyz the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getVec3(MObject node, MObject attr, double* xyz);

  /// \brief  extracts a 3D vector value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyz the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getVec3(MObject node, MObject attr, GfHalf* xyz);

  /// \brief  extracts a 4D vector value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getVec4(MObject node, MObject attr, int32_t* xyzw);

  /// \brief  extracts a 4D vector value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getVec4(MObject node, MObject attr, float* xyzw);

  /// \brief  extracts a 4D vector value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getVec4(MObject node, MObject attr, double* xyzw);

  /// \brief  extracts a 4D vector value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getVec4(MObject node, MObject attr, GfHalf* xyzw);

  /// \brief  extracts a 4D quat value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getQuat(MObject node, MObject attr, float* xyzw);

  /// \brief  extracts a 4D quat value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getQuat(MObject node, MObject attr, double* xyzw);

  /// \brief  extracts a 4D quat value from the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the returned value
  /// \return MS::kSuccess if all ok
  static MStatus getQuat(MObject node, MObject attr, GfHalf* xyzw);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Methods to set array attributes with array data
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  sets all values on a boolean array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setBoolArray(const MObject& node, const MObject& attr, const std::vector<bool>& values);

  /// \brief  sets all values on a boolean array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \param  count the number of elements in the values array
  /// \return MS::kSuccess if all ok
  static MStatus setBoolArray(MObject node, MObject attr, const bool* const values, size_t count);

  /// \brief  sets all values on a 8bit integer array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setInt8Array(const MObject& node, const MObject& attr, const std::vector<int8_t>& values);

  /// \brief  sets all values on a 8bit integer array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \param  count the number of elements in the values array
  /// \return MS::kSuccess if all ok
  static MStatus setInt8Array(MObject node, MObject attr, const int8_t* values, size_t count);

  /// \brief  sets all values on a 16bit integer array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setInt16Array(const MObject& node, const MObject& attr, const std::vector<int16_t>& values);

  /// \brief  sets all values on a 16bit integer array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \param  count the number of elements in the values array
  /// \return MS::kSuccess if all ok
  static MStatus setInt16Array(MObject node, MObject attr, const int16_t* values, size_t count);

  /// \brief  sets all values on a 32bit integer array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setInt32Array(const MObject& node, const MObject& attr, const std::vector<int32_t>& values);

  /// \brief  sets all values on a 32bit integer array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \param  count the number of elements in the values array
  /// \return MS::kSuccess if all ok
  static MStatus setInt32Array(MObject node, MObject attr, const int32_t* values, size_t count);

  /// \brief  sets all values on a 64bit integer array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setInt64Array(const MObject& node, const MObject& attr, const std::vector<int64_t>& values);

  /// \brief  sets all values on a 64bit integer array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \param  count the number of elements in the values array
  /// \return MS::kSuccess if all ok
  static MStatus setInt64Array(MObject node, MObject attr, const int64_t* values, size_t count);

  /// \brief  sets all values on a float array attribute on the specified node (but convert from half float data)
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setHalfArray(const MObject& node, const MObject& attr, const std::vector<GfHalf>& values);

  /// \brief  sets all values on a float array attribute on the specified node (but convert from half float data)
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \param  count the number of elements in the values array
  /// \return MS::kSuccess if all ok
  static MStatus setHalfArray(MObject node, MObject attr, const GfHalf* values, size_t count);

  /// \brief  sets all values on a float array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setFloatArray(const MObject& node, const MObject& attr, const std::vector<float>& values);

  /// \brief  sets all values on a float array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \param  count the number of elements in the values array
  /// \return MS::kSuccess if all ok
  static MStatus setFloatArray(MObject node, MObject attr, const float* values, size_t count);

  /// \brief  sets all values on a double array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setDoubleArray(const MObject& node, const MObject& attr, const std::vector<double>& values);

  /// \brief  sets all values on a double array attribute on the specified node
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the array attribute
  /// \param  values the array values to set on the attribute
  /// \param  count the number of elements in the values array
  /// \return MS::kSuccess if all ok
  static MStatus setDoubleArray(MObject node, MObject attr, const double* values, size_t count);

  static MStatus setVec2Array(MObject node, MObject attr, const GfHalf* values, size_t count);
  static MStatus setVec2Array(MObject node, MObject attr, const float* values, size_t count);
  static MStatus setVec2Array(MObject node, MObject attr, const double* values, size_t count);
  static MStatus setVec2Array(MObject node, MObject attr, const int32_t* values, size_t count);

  static MStatus setVec3Array(MObject node, MObject attr, const GfHalf* values, size_t count);
  static MStatus setVec3Array(MObject node, MObject attr, const float* values, size_t count);
  static MStatus setVec3Array(MObject node, MObject attr, const double* values, size_t count);
  static MStatus setVec3Array(MObject node, MObject attr, const int32_t* values, size_t count);

  static MStatus setVec4Array(MObject node, MObject attr, const GfHalf* values, size_t count);
  static MStatus setVec4Array(MObject node, MObject attr, const float* values, size_t count);
  static MStatus setVec4Array(MObject node, MObject attr, const double* values, size_t count);
  static MStatus setVec4Array(MObject node, MObject attr, const int32_t* values, size_t count);

  static MStatus setQuatArray(MObject node, MObject attr, const GfHalf* values, size_t count);
  static MStatus setQuatArray(MObject node, MObject attr, const float* values, size_t count);
  static MStatus setQuatArray(MObject node, MObject attr, const double* values, size_t count);

  static MStatus setMatrix2x2Array(MObject node, MObject attr, const float* values, size_t count);
  static MStatus setMatrix2x2Array(MObject node, MObject attr, const double* values, size_t count);
  static MStatus setMatrix3x3Array(MObject node, MObject attr, const float* values, size_t count);
  static MStatus setMatrix3x3Array(MObject node, MObject attr, const double* values, size_t count);
  static MStatus setMatrix4x4Array(MObject node, MObject attr, const float* values, size_t count);
  static MStatus setMatrix4x4Array(MObject node, MObject attr, const double* values, size_t count);

  static MStatus setStringArray(MObject node, MObject attr, const std::string* values, size_t count);

  static MStatus setTimeArray(MObject node, MObject attr, const float* values, size_t count, MTime::Unit unit);
  static MStatus setAngleArray(MObject node, MObject attr, const float* values, size_t count, MAngle::Unit unit);
  static MStatus setDistanceArray(MObject node, MObject attr, const float* values, size_t count, MDistance::Unit unit);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Methods to set single values on non-array attributes
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  sets a half float value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value for the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setHalf(MObject node, MObject attr, const GfHalf value)
    { return setFloat(node, attr, value); }

  /// \brief  sets a float value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value for the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setFloat(MObject node, MObject attr, float value);

  /// \brief  sets a double value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value for the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setDouble(MObject node, MObject attr, double value);

  /// \brief  sets a time value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value for the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setTime(MObject node, MObject attr, MTime value);

  /// \brief  sets a distance value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value for the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setDistance(MObject node, MObject attr, MDistance value);

  /// \brief  sets an angle value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value for the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setAngle(MObject node, MObject attr, MAngle value);

  /// \brief  sets a boolean value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value for the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setBool(MObject node, MObject attr, bool value);

  /// \brief  sets a 8bit integer value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value for the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setInt8(MObject node, MObject attr, int8_t value);

  /// \brief  sets a 16bit integer value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value for the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setInt16(MObject node, MObject attr, int16_t value);

  /// \brief  sets a 32bit integer value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value for the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setInt32(MObject node, MObject attr, int32_t value);

  /// \brief  sets a 64bit integer value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value for the attribute
  /// \return MS::kSuccess if all ok
  static MStatus setInt64(MObject node, MObject attr, int64_t value);

  /// \brief  sets a 3D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  x the new x value
  /// \param  y the new y value
  /// \param  z the new z value
  /// \return MS::kSuccess if all ok
  static MStatus setVec3(MObject node, MObject attr, float x, float y, float z);

  /// \brief  sets a 3D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  x the new x value
  /// \param  y the new y value
  /// \param  z the new z value
  /// \return MS::kSuccess if all ok
  static MStatus setVec3(MObject node, MObject attr, double x, double y, double z);

  /// \brief  sets a 3D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  x the new x value
  /// \param  y the new y value
  /// \param  z the new z value
  /// \return MS::kSuccess if all ok
  static MStatus setVec3(MObject node, MObject attr, MAngle x, MAngle y, MAngle z);

  /// \brief  sets a 2x2 matrix value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the new value (as an array of 4 floats)
  /// \return MS::kSuccess if all ok
  static MStatus setMatrix2x2(MObject node, MObject attr, const float* values);

  /// \brief  sets a 3x3 matrix value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the new value (as an array of 9 floats)
  /// \return MS::kSuccess if all ok
  static MStatus setMatrix3x3(MObject node, MObject attr, const float* values);

  /// \brief  sets a 4x4 matrix value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the new value (as an array of 16 floats)
  /// \return MS::kSuccess if all ok
  static MStatus setMatrix4x4(MObject node, MObject attr, const float* values);

  /// \brief  sets a 4x4 matrix value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value
  /// \return MS::kSuccess if all ok
  static MStatus setMatrix4x4(MObject node, MObject attr, const MFloatMatrix& value);

  /// \brief  sets a 2x2 matrix value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the new value (as an array of 4 doubles)
  /// \return MS::kSuccess if all ok
  static MStatus setMatrix2x2(MObject node, MObject attr, const double* values);

  /// \brief  sets a 3x3 matrix value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the new value (as an array of 9 doubles)
  /// \return MS::kSuccess if all ok
  static MStatus setMatrix3x3(MObject node, MObject attr, const double* values);

  /// \brief  sets a 4x4 matrix value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  values the new value (as an array of 16 doubles)
  /// \return MS::kSuccess if all ok
  static MStatus setMatrix4x4(MObject node, MObject attr, const double* values);

  /// \brief  sets a 4x4 matrix value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  value the new value
  /// \return MS::kSuccess if all ok
  static MStatus setMatrix4x4(MObject node, MObject attr, const MMatrix& value);

  /// \brief  sets a string value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  str the new value
  /// \return MS::kSuccess if all ok
  static MStatus setString(MObject node, MObject attr, const char* str);

  /// \brief  sets a string value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  str the new value
  /// \return MS::kSuccess if all ok
  static MStatus setString(MObject node, MObject attr, const std::string& str);

  /// \brief  sets a 2D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xy the new value
  /// \return MS::kSuccess if all ok
  static MStatus setVec2(MObject node, MObject attr, const int32_t* xy);

  /// \brief  sets a 2D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xy the new value
  /// \return MS::kSuccess if all ok
  static MStatus setVec2(MObject node, MObject attr, const float* xy);

  /// \brief  sets a 2D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xy the new value
  /// \return MS::kSuccess if all ok
  static MStatus setVec2(MObject node, MObject attr, const double* xy);

  /// \brief  sets a 2D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xy the new value
  /// \return MS::kSuccess if all ok
  static MStatus setVec2(MObject node, MObject attr, const GfHalf* xy);

  /// \brief  sets a 3D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyz the new value
  /// \return MS::kSuccess if all ok
  static MStatus setVec3(MObject node, MObject attr, const int32_t* xyz);

  /// \brief  sets a 3D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyz the new value
  /// \return MS::kSuccess if all ok
  static MStatus setVec3(MObject node, MObject attr, const float* xyz);

  /// \brief  sets a 3D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyz the new value
  /// \return MS::kSuccess if all ok
  static MStatus setVec3(MObject node, MObject attr, const double* xyz);

  /// \brief  sets a 3D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyz the new value
  /// \return MS::kSuccess if all ok
  static MStatus setVec3(MObject node, MObject attr, const GfHalf* xyz);

  /// \brief  sets a 4D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the new value
  /// \return MS::kSuccess if all ok
  static MStatus setVec4(MObject node, MObject attr, const int32_t* xyzw);

  /// \brief  sets a 4D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the new value
  /// \return MS::kSuccess if all ok
  static MStatus setVec4(MObject node, MObject attr, const float* xyzw);

  /// \brief  sets a 4D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the new value
  /// \return MS::kSuccess if all ok
  static MStatus setVec4(MObject node, MObject attr, const double* xyzw);

  /// \brief  sets a 4D vector value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the new value
  /// \return MS::kSuccess if all ok
  static MStatus setVec4(MObject node, MObject attr, const GfHalf* xyzw);

  /// \brief  sets a 4D quat value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the new value
  /// \return MS::kSuccess if all ok
  static MStatus setQuat(MObject node, MObject attr, const float* xyzw);

  /// \brief  sets a 4D quat value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the new value
  /// \return MS::kSuccess if all ok
  static MStatus setQuat(MObject node, MObject attr, const double* xyzw);

  /// \brief  sets a 4D quat value on the specified node/attribute
  /// \param  node the node on which the attribute exists
  /// \param  attr the handle to the attribute
  /// \param  xyzw the new value
  /// \return MS::kSuccess if all ok
  static MStatus setQuat(MObject node, MObject attr, const GfHalf* xyzw);

  //--------------------------------------------------------------------------------------------------------------------
  /// \name   Utilities
  //--------------------------------------------------------------------------------------------------------------------

  /// \brief  adds a new strings attribute of the specified name to the specified node, and sets its value. This is
  ///         primarily used as a utility to tag various maya nodes with some USD specific information, e.g. the
  ///         by adding a prim path, asset info, etc.
  /// \param  node the node to add the attribute to
  /// \param  attrName the name of the attribute to add
  /// \param  stringValue the value for the new atribue
  /// \return MS::kSuccess if ok.
  static MStatus addStringValue(MObject node, const char* attrName, const char* stringValue);
};

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::getInt8Array(const MObject& node, const MObject& attr, std::vector<int8_t>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getInt8Array(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::getInt16Array(const MObject& node, const MObject& attr, std::vector<int16_t>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getInt16Array(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::getInt32Array(const MObject& node, const MObject& attr, std::vector<int32_t>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getInt32Array(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::getInt64Array(const MObject& node, const MObject& attr, std::vector<int64_t>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getInt64Array(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::getHalfArray(const MObject& node, const MObject& attr, std::vector<GfHalf>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getHalfArray(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::getFloatArray(const MObject& node, const MObject& attr, std::vector<float>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getFloatArray(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::getDoubleArray(const MObject& node, const MObject& attr, std::vector<double>& values)
{
  MPlug plug(node, attr);
  if(!plug || !plug.isArray())
    return MS::kFailure;
  const uint32_t num = plug.numElements();
  values.resize(num);
  return getDoubleArray(node, attr, values.data(), num);
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::setInt8Array(const MObject& node, const MObject& attr, const std::vector<int8_t>& values)
{
  return setInt8Array(node, attr, values.data(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::setInt16Array(const MObject& node, const MObject& attr, const std::vector<int16_t>& values)
{
  return setInt16Array(node, attr, values.data(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::setInt32Array(const MObject& node, const MObject& attr, const std::vector<int32_t>& values)
{
  return setInt32Array(node, attr, values.data(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::setInt64Array(const MObject& node, const MObject& attr, const std::vector<int64_t>& values)
{
  return setInt64Array(node, attr, values.data(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::setHalfArray(const MObject& node, const MObject& attr, const std::vector<GfHalf>& values)
{
  return setHalfArray(node, attr, values.data(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::setFloatArray(const MObject& node, const MObject& attr, const std::vector<float>& values)
{
  return setFloatArray(node, attr, values.data(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
inline MStatus DgNodeHelper::setDoubleArray(const MObject& node, const MObject& attr, const std::vector<double>& values)
{
  return setDoubleArray(node, attr, values.data(), values.size());
}

//----------------------------------------------------------------------------------------------------------------------
} // utils
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
