#include <mayaUsdUtils/ALHalf.h>
#include <mayaUsdUtils/DiffCore.h>

#include <gtest/gtest.h>

static inline float  randFloat() { return float(rand()) / RAND_MAX; }
static inline double randDouble() { return double(rand()) / RAND_MAX; }

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, vec2AreAllTheSame)
{
    std::vector<float> a(16 + 4 + 2);
    for (int i = 0; i < 22; i += 2) {
        a[i] = 2;
        a[i + 1] = 3;
    }
    a.push_back(0);
    a.push_back(0);

    EXPECT_TRUE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 8));
    EXPECT_TRUE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 10));
    EXPECT_TRUE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 11));
    a[2] = 4;
    EXPECT_FALSE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 8));
    EXPECT_FALSE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 10));
    EXPECT_FALSE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 11));

    a[2] = 2;
    a[16] = 4;
    EXPECT_TRUE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 8));
    EXPECT_FALSE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 10));
    EXPECT_FALSE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 11));
    a[16] = 2;
    a[20] = 4;
    EXPECT_TRUE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 8));
    EXPECT_TRUE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 10));
    EXPECT_FALSE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 11));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, vec3AreAllTheSame)
{
    std::vector<float> a(8 * 3 + 4 * 3 + 3);
    for (uint32_t i = 0; i < a.size(); i += 3) {
        a[i] = 2;
        a[i + 1] = 3;
        a[i + 2] = 4;
    }
    a.push_back(0);
    a.push_back(0);
    a.push_back(0);

    EXPECT_TRUE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 8));
    EXPECT_TRUE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 12));
    EXPECT_TRUE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 13));
    a[3] = 4;
    EXPECT_FALSE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 8));
    EXPECT_FALSE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 12));
    EXPECT_FALSE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 13));

    a[3] = 2;
    a[24] = 4;
    EXPECT_TRUE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 8));
    EXPECT_FALSE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 12));
    EXPECT_FALSE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 13));
    a[24] = 2;
    a[36] = 4;
    EXPECT_TRUE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 8));
    EXPECT_TRUE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 12));
    EXPECT_FALSE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 13));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, vec4AreAllTheSame)
{
    std::vector<float> a(8 * 4 + 4);
    for (uint32_t i = 0; i < a.size(); i += 4) {
        a[i] = 2;
        a[i + 1] = 3;
        a[i + 2] = 4;
        a[i + 3] = 5;
    }
    a.push_back(0);
    a.push_back(0);
    a.push_back(0);
    a.push_back(0);

    EXPECT_TRUE(MayaUsdUtils::vec4AreAllTheSame(a.data(), 8));
    EXPECT_TRUE(MayaUsdUtils::vec4AreAllTheSame(a.data(), 9));
    a[4] = 4;
    EXPECT_FALSE(MayaUsdUtils::vec4AreAllTheSame(a.data(), 8));
    EXPECT_FALSE(MayaUsdUtils::vec4AreAllTheSame(a.data(), 9));
    a[4] = 2;
    a[32] = 3;
    EXPECT_TRUE(MayaUsdUtils::vec4AreAllTheSame(a.data(), 8));
    EXPECT_FALSE(MayaUsdUtils::vec4AreAllTheSame(a.data(), 9));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, vec2AreAllTheSameDouble)
{
    std::vector<double> a(16 + 4 + 2);
    for (int i = 0; i < 22; i += 2) {
        a[i] = 2;
        a[i + 1] = 3;
    }
    a.push_back(0);
    a.push_back(0);

    EXPECT_TRUE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 8));
    EXPECT_TRUE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 10));
    EXPECT_TRUE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 11));
    a[2] = 4;
    EXPECT_FALSE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 8));
    EXPECT_FALSE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 10));
    EXPECT_FALSE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 11));

    a[2] = 2;
    a[16] = 4;
    EXPECT_TRUE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 8));
    EXPECT_FALSE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 10));
    EXPECT_FALSE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 11));
    a[16] = 2;
    a[20] = 4;
    EXPECT_TRUE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 8));
    EXPECT_TRUE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 10));
    EXPECT_FALSE(MayaUsdUtils::vec2AreAllTheSame(a.data(), 11));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, vec3AreAllTheSameDouble)
{
    std::vector<double> a(8 * 3 + 4 * 3 + 3);
    for (uint32_t i = 0; i < a.size(); i += 3) {
        a[i] = 2;
        a[i + 1] = 3;
        a[i + 2] = 4;
    }
    a.push_back(0);
    a.push_back(0);
    a.push_back(0);

    EXPECT_TRUE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 8));
    EXPECT_TRUE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 12));
    EXPECT_TRUE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 13));
    a[3] = 4;
    EXPECT_FALSE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 8));
    EXPECT_FALSE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 12));
    EXPECT_FALSE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 13));

    a[3] = 2;
    a[24] = 4;
    EXPECT_TRUE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 8));
    EXPECT_FALSE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 12));
    EXPECT_FALSE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 13));
    a[24] = 2;
    a[36] = 4;
    EXPECT_TRUE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 8));
    EXPECT_TRUE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 12));
    EXPECT_FALSE(MayaUsdUtils::vec3AreAllTheSame(a.data(), 13));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, vec4AreAllTheSameDouble)
{
    std::vector<double> a(8 * 4 + 4);
    for (uint32_t i = 0; i < a.size(); i += 4) {
        a[i] = 2;
        a[i + 1] = 3;
        a[i + 2] = 4;
        a[i + 3] = 5;
    }
    a.push_back(0);
    a.push_back(0);
    a.push_back(0);
    a.push_back(0);

    EXPECT_TRUE(MayaUsdUtils::vec4AreAllTheSame(a.data(), 8));
    EXPECT_TRUE(MayaUsdUtils::vec4AreAllTheSame(a.data(), 9));
    a[4] = 4;
    EXPECT_FALSE(MayaUsdUtils::vec4AreAllTheSame(a.data(), 8));
    EXPECT_FALSE(MayaUsdUtils::vec4AreAllTheSame(a.data(), 9));
    a[4] = 2;
    a[32] = 3;
    EXPECT_TRUE(MayaUsdUtils::vec4AreAllTheSame(a.data(), 8));
    EXPECT_FALSE(MayaUsdUtils::vec4AreAllTheSame(a.data(), 9));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, compareHalfFloatArray)
{
    std::vector<float>  a(47);
    std::vector<GfHalf> b(47);
    for (int i = 0; i < 47; ++i) {
        b[i] = a[i] = randFloat();
    }

    // should pass
    EXPECT_TRUE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47, 1e-3f));
    EXPECT_TRUE(MayaUsdUtils::compareArray(b.data(), a.data(), 47, 47, 1e-3f));

    // fail on differing array sizes
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 46, 47, 1e-5f));
    EXPECT_FALSE(MayaUsdUtils::compareArray(b.data(), a.data(), 46, 47, 1e-5f));

    // test the switch cases at the ends of the array
    for (int i = 0; i < 7; ++i) {
        // modify value at end of array
        a[40 + i] += 1.0f;

        // should now fail
        EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));
        EXPECT_FALSE(MayaUsdUtils::compareArray(b.data(), a.data(), 47, 47, 1e-5f));

        a[40 + i] -= 1.0f;
    }

    // modify value in SIMD blocks
    a[22] += 1.0f;
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));
    EXPECT_FALSE(MayaUsdUtils::compareArray(b.data(), a.data(), 47, 47, 1e-5f));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, compareHalfDoubleArray)
{
    std::vector<double> a(47);
    std::vector<GfHalf> b(47);
    for (int i = 0; i < 47; ++i) {
        a[i] = randFloat();
        b[i] = float(a[i]);
    }

    // should pass
    EXPECT_TRUE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47, 1e-3f));
    EXPECT_TRUE(MayaUsdUtils::compareArray(b.data(), a.data(), 47, 47, 1e-3f));

    // fail on differing array sizes
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 46, 47, 1e-5f));
    EXPECT_FALSE(MayaUsdUtils::compareArray(b.data(), a.data(), 46, 47, 1e-5f));

    // test the switch cases at the ends of the array
    for (int i = 0; i < 7; ++i) {
        // modify value at end of array
        a[40 + i] += 1.0f;

        // should now fail
        EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));
        EXPECT_FALSE(MayaUsdUtils::compareArray(b.data(), a.data(), 47, 47, 1e-5f));

        a[40 + i] -= 1.0f;
    }

    // modify value in SIMD blocks
    a[22] += 1.0f;
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));
    EXPECT_FALSE(MayaUsdUtils::compareArray(b.data(), a.data(), 47, 47, 1e-5f));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, compareFloatArray)
{
    std::vector<float> a(47), b(47);
    for (int i = 0; i < 47; ++i) {
        a[i] = b[i] = randFloat();
    }

    // should pass
    EXPECT_TRUE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));

    // fail on differing array sizes
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 46, 47, 1e-5f));

    // test the switch cases at the ends of the array
    for (int i = 0; i < 7; ++i) {
        // modify value at end of array
        a[40 + i] += 1.0f;

        // should now fail
        EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));

        a[40 + i] -= 1.0f;
    }

    // modify value in SIMD blocks
    a[22] += 1.0f;
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, compareDoubleArray)
{
    std::vector<double> a(47), b(47);
    for (int i = 0; i < 47; ++i) {
        a[i] = b[i] = randDouble();
    }

    // should pass
    EXPECT_TRUE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));

    // fail on differing array sizes
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 46, 47, 1e-5f));

    // test the switch cases at the ends of the array
    for (int i = 0; i < 7; ++i) {
        // modify value at end of array
        a[40 + i] += 1.0;

        // should now fail
        EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));

        a[40 + i] -= 1.0;
    }

    // modify value in SIMD blocks
    a[22] += 1.0;
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47, 1e-5f));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, compareInt8Array)
{
    std::vector<int8_t> a(47), b(47);
    for (int i = 0; i < 47; ++i) {
        a[i] = b[i] = rand();
    }

    // should pass
    EXPECT_TRUE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47));

    // fail on differing array sizes
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 46, 47));

    // test the switch cases at the ends of the array
    for (int i = 0; i < 7; ++i) {
        // modify value at end of array
        a[40 + i] += 1;

        // should now fail
        EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47));

        a[40 + i] -= 1;
    }

    // modify value in SIMD blocks
    a[22] += 1;
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, compareInt16Array)
{
    std::vector<int16_t> a(47), b(47);
    for (int i = 0; i < 47; ++i) {
        a[i] = b[i] = rand();
    }

    // should pass
    EXPECT_TRUE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47));

    // fail on differing array sizes
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 46, 47));

    // test the switch cases at the ends of the array
    for (int i = 0; i < 7; ++i) {
        // modify value at end of array
        a[40 + i] += 1;

        // should now fail
        EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47));

        a[40 + i] -= 1;
    }

    // modify value in SIMD blocks
    a[22] += 1;
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, compareInt32Array)
{
    std::vector<int32_t> a(47), b(47);
    for (int i = 0; i < 47; ++i) {
        a[i] = b[i] = rand();
    }

    // should pass
    EXPECT_TRUE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47));

    // fail on differing array sizes
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 46, 47));

    // test the switch cases at the ends of the array
    for (int i = 0; i < 7; ++i) {
        // modify value at end of array
        a[40 + i] += 1;

        // should now fail
        EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47));

        a[40 + i] -= 1;
    }

    // modify value in SIMD blocks
    a[22] += 1;
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, compareInt64Array)
{
    std::vector<int64_t> a(47), b(47);
    for (int i = 0; i < 47; ++i) {
        a[i] = b[i] = rand();
    }

    // should pass
    EXPECT_TRUE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47));

    // fail on differing array sizes
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 46, 47));

    // test the switch cases at the ends of the array
    for (int i = 0; i < 7; ++i) {
        // modify value at end of array
        a[40 + i] += 1;

        // should now fail
        EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47));

        a[40 + i] -= 1;
    }

    // modify value in SIMD blocks
    a[22] += 1;
    EXPECT_FALSE(MayaUsdUtils::compareArray(a.data(), b.data(), 47, 47));
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffCore, compareUvArray)
{
    std::vector<float> u(47), v(47), uv(47 * 2);
    for (int i = 0, j = 0; i < 47; ++i, j += 2) {
        float a = randFloat();
        float b = randFloat();
        u[i] = uv[j + 0] = a;
        v[i] = uv[j + 1] = b;
    }

    // should pass
    EXPECT_TRUE(MayaUsdUtils::compareUvArray(u.data(), v.data(), uv.data(), 47, 47, 1e-5f));

    // fail on differing array sizes
    EXPECT_FALSE(MayaUsdUtils::compareUvArray(u.data(), v.data(), uv.data(), 46, 47, 1e-5f));

    // test the switch cases at the ends of the array
    for (int i = 0; i < 7; ++i) {
        // modify value at end of array
        u[40 + i] += 1.0f;

        // should now fail
        EXPECT_FALSE(MayaUsdUtils::compareUvArray(u.data(), v.data(), uv.data(), 47, 47, 1e-5f));

        u[40 + i] -= 1.0f;

        // modify value at end of array
        v[40 + i] += 1.0f;

        // should now fail
        EXPECT_FALSE(MayaUsdUtils::compareUvArray(u.data(), v.data(), uv.data(), 47, 47, 1e-5f));

        v[40 + i] -= 1.0f;
    }

    // modify value in SIMD blocks
    v[22] += 1.0f;
    EXPECT_FALSE(MayaUsdUtils::compareUvArray(u.data(), v.data(), uv.data(), 47, 47, 1e-5f));
    v[22] -= 1.0f;
    u[22] += 1.0f;
    EXPECT_FALSE(MayaUsdUtils::compareUvArray(u.data(), v.data(), uv.data(), 47, 47, 1e-5f));
    u[22] -= 1.0f;
}
