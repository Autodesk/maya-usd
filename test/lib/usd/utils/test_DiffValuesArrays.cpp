#include <mayaUsdUtils/DiffPrims.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesArrays, doubleCompareValueArraysSame)
{
    VtValue baselineValue(VtArray<double>({ double(1), double(5), double(7) }));
    VtValue modifiedValue(VtArray<double>({ double(1), double(5), double(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, doubleCompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<double>({ double(1), double(5), double(7) }));
    VtValue modifiedValue(VtArray<double>({ double(1), double(3), double(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, floatCompareValueArraysSame)
{
    VtValue baselineValue(VtArray<float>({ float(1), float(5), float(7) }));
    VtValue modifiedValue(VtArray<float>({ float(1), float(5), float(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, floatCompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<float>({ float(1), float(5), float(7) }));
    VtValue modifiedValue(VtArray<float>({ float(1), float(3), float(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, gfHalfCompareValueArraysSame)
{
    VtValue baselineValue(VtArray<GfHalf>({ GfHalf(1), GfHalf(5), GfHalf(7) }));
    VtValue modifiedValue(VtArray<GfHalf>({ GfHalf(1), GfHalf(5), GfHalf(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, gfHalfCompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<GfHalf>({ GfHalf(1), GfHalf(5), GfHalf(7) }));
    VtValue modifiedValue(VtArray<GfHalf>({ GfHalf(1), GfHalf(3), GfHalf(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, doubleFloatCompareValueArraysSame)
{
    VtValue baselineValue(VtArray<double>({ double(1), double(5), double(7) }));
    VtValue modifiedValue(VtArray<float>({ float(1), float(5), float(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, doubleFloatCompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<double>({ double(1), double(5), double(7) }));
    VtValue modifiedValue(VtArray<float>({ float(1), float(3), float(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, doubleGfHalfCompareValueArraysSame)
{
    VtValue baselineValue(VtArray<double>({ double(1), double(5), double(7) }));
    VtValue modifiedValue(VtArray<GfHalf>({ GfHalf(1), GfHalf(5), GfHalf(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, doubleGfHalfCompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<double>({ double(1), double(5), double(7) }));
    VtValue modifiedValue(VtArray<GfHalf>({ GfHalf(1), GfHalf(3), GfHalf(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, floatDoubleCompareValueArraysSame)
{
    VtValue baselineValue(VtArray<float>({ float(1), float(5), float(7) }));
    VtValue modifiedValue(VtArray<double>({ double(1), double(5), double(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, floatDoubleCompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<float>({ float(1), float(5), float(7) }));
    VtValue modifiedValue(VtArray<double>({ double(1), double(3), double(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, floatGfHalfCompareValueArraysSame)
{
    VtValue baselineValue(VtArray<float>({ float(1), float(5), float(7) }));
    VtValue modifiedValue(VtArray<GfHalf>({ GfHalf(1), GfHalf(5), GfHalf(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, floatGfHalfCompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<float>({ float(1), float(5), float(7) }));
    VtValue modifiedValue(VtArray<GfHalf>({ GfHalf(1), GfHalf(3), GfHalf(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, gfHalfDoubleCompareValueArraysSame)
{
    VtValue baselineValue(VtArray<GfHalf>({ GfHalf(1), GfHalf(5), GfHalf(7) }));
    VtValue modifiedValue(VtArray<double>({ double(1), double(5), double(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, gfHalfDoubleCompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<GfHalf>({ GfHalf(1), GfHalf(5), GfHalf(7) }));
    VtValue modifiedValue(VtArray<double>({ double(1), double(3), double(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, gfHalfFloatCompareValueArraysSame)
{
    VtValue baselineValue(VtArray<GfHalf>({ GfHalf(1), GfHalf(5), GfHalf(7) }));
    VtValue modifiedValue(VtArray<float>({ float(1), float(5), float(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, gfHalfFloatCompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<GfHalf>({ GfHalf(1), GfHalf(5), GfHalf(7) }));
    VtValue modifiedValue(VtArray<float>({ float(1), float(3), float(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, int8CompareValueArraysSame)
{
    VtValue baselineValue(VtArray<int8_t>({ int8_t(1), int8_t(5), int8_t(7) }));
    VtValue modifiedValue(VtArray<int8_t>({ int8_t(1), int8_t(5), int8_t(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, int8CompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<int8_t>({ int8_t(1), int8_t(5), int8_t(7) }));
    VtValue modifiedValue(VtArray<int8_t>({ int8_t(1), int8_t(3), int8_t(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, uint8CompareValueArraysSame)
{
    VtValue baselineValue(VtArray<uint8_t>({ uint8_t(1), uint8_t(5), uint8_t(7) }));
    VtValue modifiedValue(VtArray<uint8_t>({ uint8_t(1), uint8_t(5), uint8_t(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, uint8CompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<uint8_t>({ uint8_t(1), uint8_t(5), uint8_t(7) }));
    VtValue modifiedValue(VtArray<uint8_t>({ uint8_t(1), uint8_t(3), uint8_t(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, int16CompareValueArraysSame)
{
    VtValue baselineValue(VtArray<int16_t>({ int16_t(1), int16_t(5), int16_t(7) }));
    VtValue modifiedValue(VtArray<int16_t>({ int16_t(1), int16_t(5), int16_t(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, int16CompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<int16_t>({ int16_t(1), int16_t(5), int16_t(7) }));
    VtValue modifiedValue(VtArray<int16_t>({ int16_t(1), int16_t(3), int16_t(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, uint16CompareValueArraysSame)
{
    VtValue baselineValue(VtArray<uint16_t>({ uint16_t(1), uint16_t(5), uint16_t(7) }));
    VtValue modifiedValue(VtArray<uint16_t>({ uint16_t(1), uint16_t(5), uint16_t(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, uint16CompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<uint16_t>({ uint16_t(1), uint16_t(5), uint16_t(7) }));
    VtValue modifiedValue(VtArray<uint16_t>({ uint16_t(1), uint16_t(3), uint16_t(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, int32CompareValueArraysSame)
{
    VtValue baselineValue(VtArray<int32_t>({ int32_t(1), int32_t(5), int32_t(7) }));
    VtValue modifiedValue(VtArray<int32_t>({ int32_t(1), int32_t(5), int32_t(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, int32CompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<int32_t>({ int32_t(1), int32_t(5), int32_t(7) }));
    VtValue modifiedValue(VtArray<int32_t>({ int32_t(1), int32_t(3), int32_t(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, uint32CompareValueArraysSame)
{
    VtValue baselineValue(VtArray<uint32_t>({ uint32_t(1), uint32_t(5), uint32_t(7) }));
    VtValue modifiedValue(VtArray<uint32_t>({ uint32_t(1), uint32_t(5), uint32_t(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, uint32CompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<uint32_t>({ uint32_t(1), uint32_t(5), uint32_t(7) }));
    VtValue modifiedValue(VtArray<uint32_t>({ uint32_t(1), uint32_t(3), uint32_t(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, int64CompareValueArraysSame)
{
    VtValue baselineValue(VtArray<int64_t>({ int64_t(1), int64_t(5), int64_t(7) }));
    VtValue modifiedValue(VtArray<int64_t>({ int64_t(1), int64_t(5), int64_t(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, int64CompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<int64_t>({ int64_t(1), int64_t(5), int64_t(7) }));
    VtValue modifiedValue(VtArray<int64_t>({ int64_t(1), int64_t(3), int64_t(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesArrays, uint64CompareValueArraysSame)
{
    VtValue baselineValue(VtArray<uint64_t>({ uint64_t(1), uint64_t(5), uint64_t(7) }));
    VtValue modifiedValue(VtArray<uint64_t>({ uint64_t(1), uint64_t(5), uint64_t(7) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesArrays, uint64CompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<uint64_t>({ uint64_t(1), uint64_t(5), uint64_t(7) }));
    VtValue modifiedValue(VtArray<uint64_t>({ uint64_t(1), uint64_t(3), uint64_t(4) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

