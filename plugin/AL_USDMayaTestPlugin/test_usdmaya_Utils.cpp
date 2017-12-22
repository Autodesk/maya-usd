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
#include "test_usdmaya.h"

#include "AL/usdmaya/Utils.h"

#include "pxr/base/gf/rotation.h"
#include "maya/MEulerRotation.h"

using namespace AL;
using namespace AL::usdmaya;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of guid_compare.
//----------------------------------------------------------------------------------------------------------------------
TEST(usdmaya_Utils, guid_compare)
{
#if AL_MAYA_ENABLE_SIMD
  guid_compare gcmp;
  i128 a = set16i8(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
  i128 b = set16i8(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);

  // identical guids should always return false
  EXPECT_EQ(true, !gcmp(a, b) && !gcmp(b, a));

  for(uint32_t i = 0; i < 16; ++i)
  {
    ALIGN16(uint8_t values[]) = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    values[i] += 1;

    i128 c = load4i(values);
    EXPECT_EQ(true, gcmp(a, c) && !gcmp(c, a));
  }

  for(uint32_t i = 0; i < 16; ++i)
  {
    ALIGN16(uint8_t values[]) = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    values[i] -= 1;

    i128 c = load4i(values);
    EXPECT_EQ(true, !gcmp(a, c) && gcmp(c, a));
  }
#else
  guid_compare gcmp;
  guid a = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}};
  guid b = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}};

  // identical guids should always return false
  EXPECT_EQ(true, !gcmp(a, b) && !gcmp(b, a));

  for(uint32_t i = 0; i < 16; ++i)
  {
    guid values = {{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }};
    values.uuid[i] += 1;

    EXPECT_EQ(true, gcmp(a, values) && !gcmp(values, a));
  }

  for(uint32_t i = 0; i < 16; ++i)
  {
    guid values = {{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }};
    values.uuid[i] -= 1;
    EXPECT_EQ(true, !gcmp(a, values) && gcmp(values, a));
  }
#endif
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test matrixToSRT
//----------------------------------------------------------------------------------------------------------------------
TEST(usdmaya_Utils, matrixToSRT)
{
  // Test one-axis negative scale
  {
    constexpr double epsilon = 1e-5;

    GfMatrix4d inputMatrix(
        1, 0,  0, 0,
        0, 1,  0, 0,
        0, 0, -1, 0,
        0, 0,  0, 1);
    double S[3];
    MEulerRotation R;
    double T[3];
    matrixToSRT(inputMatrix, S, R, T);

    EXPECT_EQ(0.0, T[0]);
    EXPECT_EQ(0.0, T[1]);
    EXPECT_EQ(0.0, T[2]);

    GfMatrix4d rotXMat;
    rotXMat.SetRotate(GfRotation(GfVec3d(1, 0, 0), R.x));
    GfMatrix4d rotYMat;
    rotYMat.SetRotate(GfRotation(GfVec3d(0, 1, 0), R.y));
    GfMatrix4d rotZMat;
    rotZMat.SetRotate(GfRotation(GfVec3d(0, 0, 1), R.z));
    GfMatrix4d scaleMat;
    scaleMat.SetScale(GfVec3d(S));

    GfMatrix4d resultMatrix = rotXMat * rotYMat * rotZMat * scaleMat;
    EXPECT_NEAR(inputMatrix[0][0], resultMatrix[0][0], epsilon);
    EXPECT_NEAR(inputMatrix[0][1], resultMatrix[0][1], epsilon);
    EXPECT_NEAR(inputMatrix[0][2], resultMatrix[0][2], epsilon);
    EXPECT_NEAR(inputMatrix[0][3], resultMatrix[0][3], epsilon);
    EXPECT_NEAR(inputMatrix[1][0], resultMatrix[1][0], epsilon);
    EXPECT_NEAR(inputMatrix[1][1], resultMatrix[1][1], epsilon);
    EXPECT_NEAR(inputMatrix[1][2], resultMatrix[1][2], epsilon);
    EXPECT_NEAR(inputMatrix[1][3], resultMatrix[1][3], epsilon);
    EXPECT_NEAR(inputMatrix[2][0], resultMatrix[2][0], epsilon);
    EXPECT_NEAR(inputMatrix[2][1], resultMatrix[2][1], epsilon);
    EXPECT_NEAR(inputMatrix[2][2], resultMatrix[2][2], epsilon);
    EXPECT_NEAR(inputMatrix[2][3], resultMatrix[2][3], epsilon);
    EXPECT_NEAR(inputMatrix[3][0], resultMatrix[3][0], epsilon);
    EXPECT_NEAR(inputMatrix[3][1], resultMatrix[3][1], epsilon);
    EXPECT_NEAR(inputMatrix[3][2], resultMatrix[3][2], epsilon);
    EXPECT_NEAR(inputMatrix[3][3], resultMatrix[3][3], epsilon);
  }
}

//matrixToSRT(GfMatrix4d& value, double S[3], MEulerRotation& R, double T[3])