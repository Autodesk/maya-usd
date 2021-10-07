#include <mayaUsdUtils/DiffPrims.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValues, compareValuesEmpty)
{
    VtValue baselineValue;
    VtValue modifiedValue;
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, doubleCompareValuesSame)
{
    VtValue baselineValue(double(1));
    VtValue modifiedValue(double(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, doubleCompareValuesDiff)
{
    VtValue baselineValue(double(1));
    VtValue modifiedValue(double(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, floatCompareValuesSame)
{
    VtValue baselineValue(float(1));
    VtValue modifiedValue(float(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, floatCompareValuesDiff)
{
    VtValue baselineValue(float(1));
    VtValue modifiedValue(float(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, doubleFloatCompareValuesSame)
{
    VtValue baselineValue(double(1));
    VtValue modifiedValue(float(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, doubleFloatCompareValuesDiff)
{
    VtValue baselineValue(double(1));
    VtValue modifiedValue(float(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, doubleGfHalfCompareValuesSame)
{
    VtValue baselineValue(double(1));
    VtValue modifiedValue(GfHalf(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, doubleGfHalfCompareValuesDiff)
{
    VtValue baselineValue(double(1));
    VtValue modifiedValue(GfHalf(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, floatDoubleCompareValuesSame)
{
    VtValue baselineValue(float(1));
    VtValue modifiedValue(double(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, floatDoubleCompareValuesDiff)
{
    VtValue baselineValue(float(1));
    VtValue modifiedValue(double(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, floatGfHalfCompareValuesSame)
{
    VtValue baselineValue(float(1));
    VtValue modifiedValue(GfHalf(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, floatGfHalfCompareValuesDiff)
{
    VtValue baselineValue(float(1));
    VtValue modifiedValue(GfHalf(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, gfHalfDoubleCompareValuesSame)
{
    VtValue baselineValue(GfHalf(1));
    VtValue modifiedValue(double(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, gfHalfDoubleCompareValuesDiff)
{
    VtValue baselineValue(GfHalf(1));
    VtValue modifiedValue(double(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, gfHalfFloatCompareValuesSame)
{
    VtValue baselineValue(GfHalf(1));
    VtValue modifiedValue(float(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, gfHalfFloatCompareValuesDiff)
{
    VtValue baselineValue(GfHalf(1));
    VtValue modifiedValue(float(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, int8CompareValuesSame)
{
    VtValue baselineValue(int8_t(1));
    VtValue modifiedValue(int8_t(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, int8CompareValuesDiff)
{
    VtValue baselineValue(int8_t(1));
    VtValue modifiedValue(int8_t(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, uint8CompareValuesSame)
{
    VtValue baselineValue(uint8_t(1));
    VtValue modifiedValue(uint8_t(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, uint8CompareValuesDiff)
{
    VtValue baselineValue(uint8_t(1));
    VtValue modifiedValue(uint8_t(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, int16CompareValuesSame)
{
    VtValue baselineValue(int16_t(1));
    VtValue modifiedValue(int16_t(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, int16CompareValuesDiff)
{
    VtValue baselineValue(int16_t(1));
    VtValue modifiedValue(int16_t(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, uint16CompareValuesSame)
{
    VtValue baselineValue(uint16_t(1));
    VtValue modifiedValue(uint16_t(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, uint16CompareValuesDiff)
{
    VtValue baselineValue(uint16_t(1));
    VtValue modifiedValue(uint16_t(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, int32CompareValuesSame)
{
    VtValue baselineValue(int32_t(1));
    VtValue modifiedValue(int32_t(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, int32CompareValuesDiff)
{
    VtValue baselineValue(int32_t(1));
    VtValue modifiedValue(int32_t(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, uint32CompareValuesSame)
{
    VtValue baselineValue(uint32_t(1));
    VtValue modifiedValue(uint32_t(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, uint32CompareValuesDiff)
{
    VtValue baselineValue(uint32_t(1));
    VtValue modifiedValue(uint32_t(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, int64CompareValuesSame)
{
    VtValue baselineValue(int64_t(1));
    VtValue modifiedValue(int64_t(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, int64CompareValuesDiff)
{
    VtValue baselineValue(int64_t(1));
    VtValue modifiedValue(int64_t(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValues, uint64CompareValuesSame)
{
    VtValue baselineValue(uint64_t(1));
    VtValue modifiedValue(uint64_t(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, uint64CompareValuesDiff)
{
    VtValue baselineValue(uint64_t(1));
    VtValue modifiedValue(uint64_t(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

