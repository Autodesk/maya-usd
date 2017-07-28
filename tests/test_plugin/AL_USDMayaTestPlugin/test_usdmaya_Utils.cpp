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

using namespace AL;
using namespace AL::usdmaya;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the alUsdNodeHelper.
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

