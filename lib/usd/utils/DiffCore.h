//
// Copyright 2018 Animal Logic
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

#include <mayaUsdUtils/ALHalf.h>
#include <mayaUsdUtils/Api.h>

#include <cstdint>

namespace MayaUsdUtils {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests to see whether the U & V coordinates are identical
/// \param  u the U coordinate array
/// \param  v the V coordinate array
/// \param  count the number of elements to test
/// \return true if all elements in the arrays are identical
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool vec2AreAllTheSame(const float* u, const float* v, size_t count);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests to see whether the array elements are identical
/// \param  array the 2D array to test
/// \param  count the number of elements to test
/// \return true if all elements in the arrays are identical
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool vec2AreAllTheSame(const float* array, size_t count);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests to see whether the array elements are identical
/// \param  array the 3D array to test
/// \param  count the number of elements to test
/// \return true if all elements in the arrays are identical
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool vec3AreAllTheSame(const float* array, size_t count);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests to see whether the array elements are identical
/// \param  array the 4D array to test
/// \param  count the number of elements to test
/// \return true if all elements in the arrays are identical
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool vec4AreAllTheSame(const float* array, size_t count);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests to see whether the array elements are identical
/// \param  array the 2D array to test
/// \param  count the number of elements to test
/// \return true if all elements in the arrays are identical
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool vec2AreAllTheSame(const double* array, size_t count);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests to see whether the array elements are identical
/// \param  array the 3D array to test
/// \param  count the number of elements to test
/// \return true if all elements in the arrays are identical
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool vec3AreAllTheSame(const double* array, size_t count);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests to see whether the array elements are identical
/// \param  array the 4D array to test
/// \param  count the number of elements to test
/// \return true if all elements in the arrays are identical
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool vec4AreAllTheSame(const double* array, size_t count);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool compareArray(
    const GfHalf* const input0,
    const float* const  input1,
    const size_t        count0,
    const size_t        count1,
    const float         eps = 1e-3f);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
inline bool compareArray(
    const float* const  input0,
    const GfHalf* const input1,
    const size_t        count0,
    const size_t        count1,
    const float         eps = 1e-3f)
{
    return compareArray(input1, input0, count1, count0, eps);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool compareArray(
    const GfHalf* const input0,
    const double* const input1,
    const size_t        count0,
    const size_t        count1,
    const double        eps = 1e-5f);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
inline bool compareArray(
    const double* const input0,
    const GfHalf* const input1,
    const size_t        count0,
    const size_t        count1,
    const float         eps = 1e-3f)
{
    return compareArray(input1, input0, count1, count0, eps);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool compareArray(
    const float* const input0,
    const float* const input1,
    const size_t       count0,
    const size_t       count1,
    const float        eps = 1e-5f);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Compares a vector 3 array against a vector 4D array, ignoring the 4d component, and
/// checking for equality
///         in all of the remaining 3d components.
/// \param  input3d the first input array to test
/// \param  input4d the second input array to test
/// \param  count3d number of elements in the first array
/// \param  count4d number of elements in the second array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool compareArray3Dto4D(
    const float* const input3d,
    const float* const input4d,
    const size_t       count3d,
    const size_t       count4d,
    const float        eps = 1e-5f);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Compares a vector 3 array against a vector 4D array, ignoring the 4d component, and
/// checking for equality
///         in all of the remaining 3d components.
/// \param  input3d the first input array to test
/// \param  input4d the second input array to test
/// \param  count3d number of elements in the first array
/// \param  count4d number of elements in the second array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool compareArray3Dto4D(
    const float* const  input3d,
    const double* const input4d,
    const size_t        count3d,
    const size_t        count4d,
    const float         eps = 1e-5f);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool compareArray(
    const double* const input0,
    const double* const input1,
    const size_t        count0,
    const size_t        count1,
    const double        eps = 1e-5);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool compareArray(
    const double* const input0,
    const float* const  input1,
    const size_t        count0,
    const size_t        count1,
    const float         eps = 1e-5f);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
inline bool compareArray(
    const float* const  input0,
    const double* const input1,
    const size_t        count0,
    const size_t        count1,
    const float         eps = 1e-5f)
{
    return compareArray(input1, input0, count1, count0, eps);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool compareArray(
    const int8_t* const input0,
    const int8_t* const input1,
    const size_t        count0,
    const size_t        count1);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
inline bool compareArray(
    const uint8_t* const input0,
    const uint8_t* const input1,
    const size_t         count0,
    const size_t         count1)
{
    return compareArray((const int8_t*)input0, (const int8_t*)input1, count0, count1);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
inline bool compareArray(
    const int16_t* const input0,
    const int16_t* const input1,
    const size_t         count0,
    const size_t         count1)
{
    return compareArray((const int8_t*)input0, (const int8_t*)input1, count0 << 1, count1 << 1);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
inline bool compareArray(
    const uint16_t* const input0,
    const uint16_t* const input1,
    const size_t          count0,
    const size_t          count1)
{
    return compareArray((const int8_t*)input0, (const int8_t*)input1, count0 << 1, count1 << 1);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool compareArray(
    const int32_t* const input0,
    const int32_t* const input1,
    const size_t         count0,
    const size_t         count1);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
inline bool compareArray(
    const uint32_t* const input0,
    const uint32_t* const input1,
    const size_t          count0,
    const size_t          count1)
{
    return compareArray((const int32_t*)input0, (const int32_t*)input1, count0, count1);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
inline bool compareArray(
    const int64_t* const input0,
    const int64_t* const input1,
    const size_t         count0,
    const size_t         count1)
{
    return compareArray((const int32_t*)input0, (const int32_t*)input1, count0 << 1, count1 << 1);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays.
/// \param  input0 the first input array to test
/// \param  input1 the second input array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
inline bool compareArray(
    const uint64_t* const input0,
    const uint64_t* const input1,
    const size_t          count0,
    const size_t          count1)
{
    return compareArray((const int32_t*)input0, (const int32_t*)input1, count0 << 1, count1 << 1);
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  tests the differences between a pair of arrays of UV.
/// \param  u0 the U values of the first input array to test
/// \param  v0 the V values of the first input array to test
/// \param  uv1 the second input UV array to test
/// \param  count0 number of elements in the first array
/// \param  count1 number of elements in the second array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool compareUvArray(
    const float* const u0,
    const float* const v0,
    const float* const uv1,
    const size_t       count0,
    const size_t       count1,
    const float        eps = 1e-5f);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a simple method to check if an array of UV is all equal to a given UV value.
/// \param  u0 the U values to test
/// \param  v0 the V values to test
/// \param  u1 the U values of the input array to test
/// \param  v1 the V values of the input array to test
/// \param  count number of elements in the array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool compareUvArray(
    const float        u0,
    const float        v0,
    const float* const u1,
    const float* const v1,
    const size_t       count,
    const float        eps = 1e-5f);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a simple method to check if an array of RGBA is all equal to a given RGBA value.
/// \param  r the red values to test
/// \param  g the green values to test
/// \param  b the blue values to test
/// \param  a the alpha values to test
/// \param  rgba the RGBA values of the input array to test
/// \param  count number of elements in the array
/// \param  eps and epsilon value for the element comparisons
/// \return true if the two arrays are similar to each other, false if their sizes don't match,
///         or the contents of the arrays differ
//----------------------------------------------------------------------------------------------------------------------
MAYA_USD_UTILS_PUBLIC
bool compareRGBAArray(
    const float        r,
    const float        g,
    const float        b,
    const float        a,
    const float* const rgba,
    const size_t       count,
    const float        eps = 1e-5f);

//----------------------------------------------------------------------------------------------------------------------
} // namespace MayaUsdUtils
