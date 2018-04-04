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

#include "AL/maya/utils/NodeHelper.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/translators/MeshTranslator.h"
#include "AL/usdmaya/fileio/translators/DagNodeTranslator.h"
#include "AL/usdmaya/utils/MeshUtils.h"

using namespace AL::usdmaya::fileio::translators;
//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the mesh translator
//----------------------------------------------------------------------------------------------------------------------
TEST(translators_MeshTranslator, convert3DArrayTo4DArray)
{
  std::vector<float> input;
  std::vector<float> output;
  input.resize(39 * 3);
  output.resize(39 * 4);
  for(uint32_t i = 0; i < 39 * 3; ++i)
  {
    input[i] = float(i);
  }

  AL::usdmaya::utils::convert3DArrayTo4DArray(input.data(), output.data(), 39);
  for(uint32_t i = 0, j = 0; i < 39 * 3; i += 3, j += 4)
  {
    EXPECT_NEAR(input[i], output[j], 1e-5f);
    EXPECT_NEAR(input[i + 1], output[j + 1], 1e-5f);
    EXPECT_NEAR(input[i + 2], output[j + 2], 1e-5f);
    EXPECT_NEAR(1.0f, output[j + 3], 1e-5f);
  }
}

TEST(translators_MeshTranslator, convertFloatVec3ArrayToDoubleVec3Array)
{
  std::vector<float> input;
  std::vector<double> output;
  input.resize(39 * 3);
  output.resize(39 * 3);
  for(uint32_t i = 0; i < 39 * 3; ++i)
  {
    input[i] = float(i);
  }

  AL::usdmaya::utils::convertFloatVec3ArrayToDoubleVec3Array(input.data(), output.data(), 39);
  for(uint32_t i = 0; i < 39 * 3; i += 3)
  {
    EXPECT_NEAR(input[i], output[i], 1e-5f);
    EXPECT_NEAR(input[i + 1], output[i + 1], 1e-5f);
    EXPECT_NEAR(input[i + 2], output[i + 2], 1e-5f);
  }

}

TEST(translators_MeshTranslator, zipunzipUVs)
{
  std::vector<float> u, v, uv(78);
  float f = 0;
  for(uint32_t i = 0; i < 39; ++i, f += 2.0f)
  {
    u.push_back(f);
    v.push_back(f + 1.0f);
  }

  AL::usdmaya::utils::zipUVs(u.data(), v.data(), uv.data(), u.size());

  f = 0;
  for(uint32_t i = 0; i < 78; i += 2, f += 2.0f)
  {
    EXPECT_NEAR(f, uv[i], 1e-5f);
    EXPECT_NEAR(f + 1.0f, uv[i + 1], 1e-5f);
  }

  std::vector<float> u2(39), v2(39);
  AL::usdmaya::utils::unzipUVs(uv.data(), u2.data(), v2.data(), u.size());

  for(uint32_t i = 0; i < 39; i++)
  {
    EXPECT_NEAR(u2[i], u[i], 1e-5f);
    EXPECT_NEAR(v2[i], v[i], 1e-5f);
  }
}

TEST(translators_MeshTranslator, interleaveIndexedUvData)
{
  std::vector<float> output(78), u(39), v(39);
  std::vector<int32_t> indices(39);
  const uint32_t numIndices = 39;

  for(uint32_t i = 0; i < 39; ++i)
  {
    u[i] = i * 2.0f + 1.0f;
    v[i] = i * 2.0f;
    indices[i] = 38 - i;
  }

  AL::usdmaya::utils::interleaveIndexedUvData(output.data(), u.data(), v.data(), indices.data(), numIndices);

  for(uint32_t i = 0; i < 78; ++i)
  {
    float fi = (77 - i);
    EXPECT_NEAR(fi, output[i], 1e-4f);
  }
}

TEST(translators_MeshTranslator, isUvSetDataSparse)
{
  std::vector<int32_t> uvCounts;
  uvCounts.resize(35, 1);


  EXPECT_TRUE(!AL::usdmaya::utils::isUvSetDataSparse(uvCounts.data(), uvCounts.size()));

  uvCounts[4] = 0;
  EXPECT_TRUE(AL::usdmaya::utils::isUvSetDataSparse(uvCounts.data(), uvCounts.size()));
  uvCounts[4] = 1;
  uvCounts[33] = 0;

  EXPECT_TRUE(AL::usdmaya::utils::isUvSetDataSparse(uvCounts.data(), uvCounts.size()));
}

TEST(translators_MeshTranslator, generateIncrementingIndices)
{
  MIntArray indices;
  AL::usdmaya::utils::generateIncrementingIndices(indices, 39);

  for(int32_t i = 0; i < 39; ++i)
  {
    EXPECT_EQ(indices[i], i);
  }
}

