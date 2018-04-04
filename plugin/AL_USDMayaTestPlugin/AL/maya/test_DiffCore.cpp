
#include "AL/usd/utils/DiffCore.h"
#include "AL/usd/utils/ALHalf.h"
#include <gtest/gtest.h>

static inline float randFloat()
{
  return float(rand()) / RAND_MAX;
}
static inline double randDouble()
{
  return double(rand()) / RAND_MAX;
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DataDiff, compareHalfFloatArray)
{
  std::vector<float> a(47);
  std::vector<GfHalf> b(47);
  for(int i = 0; i < 47; ++i)
  {
    b[i] = a[i] = randFloat();
  }

  // should pass
  EXPECT_TRUE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47, 1e-3f));
  EXPECT_TRUE(AL::usd::utils::compareArray(b.data(), a.data(), 47, 47, 1e-3f));

  // fail on differing array sizes
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 46, 47, 1e-5f));
  EXPECT_FALSE(AL::usd::utils::compareArray(b.data(), a.data(), 46, 47, 1e-5f));

  // test the switch cases at the ends of the array
  for(int i = 0; i < 7; ++i)
  {
    // modify value at end of array
    a[40 + i] += 1.0f;

    // should now fail
    EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));
    EXPECT_FALSE(AL::usd::utils::compareArray(b.data(), a.data(), 47, 47, 1e-5f));

    a[40 + i] -= 1.0f;
  }

  // modify value in SIMD blocks
  a[22] += 1.0f;
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));
  EXPECT_FALSE(AL::usd::utils::compareArray(b.data(), a.data(), 47, 47, 1e-5f));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DataDiff, compareHalfDoubleArray)
{
  std::vector<double> a(47);
  std::vector<GfHalf> b(47);
  for(int i = 0; i < 47; ++i)
  {
    a[i] = randFloat();
    b[i] = float(a[i]);
  }

  // should pass
  EXPECT_TRUE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47, 1e-3f));
  EXPECT_TRUE(AL::usd::utils::compareArray(b.data(), a.data(), 47, 47, 1e-3f));

  // fail on differing array sizes
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 46, 47, 1e-5f));
  EXPECT_FALSE(AL::usd::utils::compareArray(b.data(), a.data(), 46, 47, 1e-5f));

  // test the switch cases at the ends of the array
  for(int i = 0; i < 7; ++i)
  {
    // modify value at end of array
    a[40 + i] += 1.0f;

    // should now fail
    EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));
    EXPECT_FALSE(AL::usd::utils::compareArray(b.data(), a.data(), 47, 47, 1e-5f));

    a[40 + i] -= 1.0f;
  }

  // modify value in SIMD blocks
  a[22] += 1.0f;
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));
  EXPECT_FALSE(AL::usd::utils::compareArray(b.data(), a.data(), 47, 47, 1e-5f));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DataDiff, compareFloatArray)
{
  std::vector<float> a(47), b(47);
  for(int i = 0; i < 47; ++i)
  {
    a[i] = b[i] = randFloat();
  }

  // should pass
  EXPECT_TRUE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));

  // fail on differing array sizes
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 46, 47, 1e-5f));

  // test the switch cases at the ends of the array
  for(int i = 0; i < 7; ++i)
  {
    // modify value at end of array
    a[40 + i] += 1.0f;

    // should now fail
    EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));

    a[40 + i] -= 1.0f;
  }

  // modify value in SIMD blocks
  a[22] += 1.0f;
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DataDiff, compareDoubleArray)
{
  std::vector<double> a(47), b(47);
  for(int i = 0; i < 47; ++i)
  {
    a[i] = b[i] = randDouble();
  }

  // should pass
  EXPECT_TRUE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));

  // fail on differing array sizes
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 46, 47, 1e-5f));

  // test the switch cases at the ends of the array
  for(int i = 0; i < 7; ++i)
  {
    // modify value at end of array
    a[40 + i] += 1.0;

    // should now fail
    EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));

    a[40 + i] -= 1.0;
  }

  // modify value in SIMD blocks
  a[22] += 1.0;
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DataDiff, compareInt8Array)
{
  std::vector<int8_t> a(47), b(47);
  for(int i = 0; i < 47; ++i)
  {
    a[i] = b[i] = rand();
  }

  // should pass
  EXPECT_TRUE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47));

  // fail on differing array sizes
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 46, 47));

  // test the switch cases at the ends of the array
  for(int i = 0; i < 7; ++i)
  {
    // modify value at end of array
    a[40 + i] += 1;

    // should now fail
    EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47));

    a[40 + i] -= 1;
  }

  // modify value in SIMD blocks
  a[22] += 1;
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DataDiff, compareInt16Array)
{
  std::vector<int16_t> a(47), b(47);
  for(int i = 0; i < 47; ++i)
  {
    a[i] = b[i] = rand();
  }

  // should pass
  EXPECT_TRUE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47));

  // fail on differing array sizes
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 46, 47));

  // test the switch cases at the ends of the array
  for(int i = 0; i < 7; ++i)
  {
    // modify value at end of array
    a[40 + i] += 1;

    // should now fail
    EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47));

    a[40 + i] -= 1;
  }

  // modify value in SIMD blocks
  a[22] += 1;
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DataDiff, compareInt32Array)
{
  std::vector<int32_t> a(47), b(47);
  for(int i = 0; i < 47; ++i)
  {
    a[i] = b[i] = rand();
  }

  // should pass
  EXPECT_TRUE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47));

  // fail on differing array sizes
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 46, 47));

  // test the switch cases at the ends of the array
  for(int i = 0; i < 7; ++i)
  {
    // modify value at end of array
    a[40 + i] += 1;

    // should now fail
    EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47));

    a[40 + i] -= 1;
  }

  // modify value in SIMD blocks
  a[22] += 1;
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DataDiff, compareInt64Array)
{
  std::vector<int64_t> a(47), b(47);
  for(int i = 0; i < 47; ++i)
  {
    a[i] = b[i] = rand();
  }

  // should pass
  EXPECT_TRUE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47));

  // fail on differing array sizes
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 46, 47));

  // test the switch cases at the ends of the array
  for(int i = 0; i < 7; ++i)
  {
    // modify value at end of array
    a[40 + i] += 1;

    // should now fail
    EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47));

    a[40 + i] -= 1;
  }

  // modify value in SIMD blocks
  a[22] += 1;
  EXPECT_FALSE(AL::usd::utils::compareArray(a.data(), b.data(), 47, 47));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DataDiff, compareUvArray)
{
  std::vector<float> u(47), v(47), uv(47 * 2);
  for(int i = 0, j = 0; i < 47; ++i, j += 2)
  {
    float a = randFloat();
    float b = randFloat();
    u[i] = uv[j + 0] = a;
    v[i] = uv[j + 1] = b;
  }

  // should pass
  EXPECT_TRUE(AL::usd::utils::compareUvArray(u.data(), v.data(), uv.data(), 47, 47, 1e-5f));

  // fail on differing array sizes
  EXPECT_FALSE(AL::usd::utils::compareUvArray(u.data(), v.data(), uv.data(), 46, 47, 1e-5f));

  // test the switch cases at the ends of the array
  for(int i = 0; i < 7; ++i)
  {
    // modify value at end of array
    u[40 + i] += 1.0f;

    // should now fail
    EXPECT_FALSE(AL::usd::utils::compareUvArray(u.data(), v.data(), uv.data(), 47, 47, 1e-5f));

    u[40 + i] -= 1.0f;

    // modify value at end of array
    v[40 + i] += 1.0f;

    // should now fail
    EXPECT_FALSE(AL::usd::utils::compareUvArray(u.data(), v.data(), uv.data(), 47, 47, 1e-5f));

    v[40 + i] -= 1.0f;
  }

  // modify value in SIMD blocks
  v[22] += 1.0f;
  EXPECT_FALSE(AL::usd::utils::compareUvArray(u.data(), v.data(), uv.data(), 47, 47, 1e-5f));
  v[22] -= 1.0f;
  u[22] += 1.0f;
  EXPECT_FALSE(AL::usd::utils::compareUvArray(u.data(), v.data(), uv.data(), 47, 47, 1e-5f));
  u[22] -= 1.0f;
}

