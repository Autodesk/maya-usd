#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/gf/quatf.h>
#include <pxr/base/gf/quath.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/value.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

namespace {
GfQuatd qd1(1, 12, 15, 16);
GfQuatd qd2(2, 22, 28, 21);
GfQuatd qd3(5, 53, 57, 52);
GfQuatd qd4(7, 72, 74, 70);

GfQuatf qf1(1, 12, 15, 16);
GfQuatf qf2(2, 22, 28, 21);
GfQuatf qf3(5, 53, 57, 52);
GfQuatf qf4(7, 72, 74, 70);

GfQuath qh1(1, 12, 15, 16);
GfQuath qh2(2, 22, 28, 21);
GfQuath qh3(5, 53, 57, 52);
GfQuath qh4(7, 72, 74, 70);
} // namespace

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesQuaternions, quatdCompareValuesSame)
{
    VtValue    baselineValue(qd1);
    VtValue    modifiedValue(qd1);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quatdCompareValuesDiff)
{
    VtValue    baselineValue(qd1);
    VtValue    modifiedValue(qd2);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesQuaternions, quatdCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfQuatd>({ qd1, qd2, qd3 }));
    VtValue    modifiedValue(VtArray<GfQuatd>({ qd1, qd2, qd3 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quatdCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfQuatd>({ qd1, qd2, qd3 }));
    VtValue    modifiedValue(VtArray<GfQuatd>({ qd1, qd4, qd2 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesQuaternions, quatdQuatFCompareValuesSame)
{
    VtValue    baselineValue(qd1);
    VtValue    modifiedValue(qf1);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quatdQuatFCompareValuesDiff)
{
    VtValue    baselineValue(qd1);
    VtValue    modifiedValue(qf2);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesQuaternions, quatdQuatFCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfQuatd>({ qd1, qd2, qd3 }));
    VtValue    modifiedValue(VtArray<GfQuatf>({ qf1, qf2, qf3 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quatdQuatFCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfQuatd>({ qd1, qd2, qd3 }));
    VtValue    modifiedValue(VtArray<GfQuatf>({ qf1, qf4, qf2 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesQuaternions, quatdQuathCompareValuesSame)
{
    VtValue    baselineValue(qd1);
    VtValue    modifiedValue(qh1);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quatdQuathCompareValuesDiff)
{
    VtValue    baselineValue(qd1);
    VtValue    modifiedValue(qh2);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesQuaternions, quatdQuathCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfQuatd>({ qd1, qd2, qd3 }));
    VtValue    modifiedValue(VtArray<GfQuath>({ qh1, qh2, qh3 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quatdQuathCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfQuatd>({ qd1, qd2, qd3 }));
    VtValue    modifiedValue(VtArray<GfQuath>({ qh1, qh4, qh2 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesQuaternions, quatfCompareValuesSame)
{
    VtValue    baselineValue(qf1);
    VtValue    modifiedValue(qf1);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quatfCompareValuesDiff)
{
    VtValue    baselineValue(qf1);
    VtValue    modifiedValue(qf2);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesQuaternions, quatfCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfQuatf>({ qf1, qf2, qf3 }));
    VtValue    modifiedValue(VtArray<GfQuatf>({ qf1, qf2, qf3 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quatfCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfQuatf>({ qf1, qf2, qf3 }));
    VtValue    modifiedValue(VtArray<GfQuatf>({ qf1, qf4, qf2 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesQuaternions, quatfQuatdCompareValuesSame)
{
    VtValue    baselineValue(qf1);
    VtValue    modifiedValue(qd1);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quatfQuatdCompareValuesDiff)
{
    VtValue    baselineValue(qf1);
    VtValue    modifiedValue(qd2);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesQuaternions, quatfQuatdCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfQuatf>({ qf1, qf2, qf3 }));
    VtValue    modifiedValue(VtArray<GfQuatd>({ qd1, qd2, qd3 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quatfQuatdCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfQuatf>({ qf1, qf2, qf3 }));
    VtValue    modifiedValue(VtArray<GfQuatd>({ qd1, qd4, qd2 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesQuaternions, quatfQuathCompareValuesSame)
{
    VtValue    baselineValue(qf1);
    VtValue    modifiedValue(qh1);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quatfQuathCompareValuesDiff)
{
    VtValue    baselineValue(qf1);
    VtValue    modifiedValue(qh2);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesQuaternions, quatfQuathCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfQuatf>({ qf1, qf2, qf3 }));
    VtValue    modifiedValue(VtArray<GfQuath>({ qh1, qh2, qh3 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quatfQuathCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfQuatf>({ qf1, qf2, qf3 }));
    VtValue    modifiedValue(VtArray<GfQuath>({ qh1, qh4, qh2 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesQuaternions, quathCompareValuesSame)
{
    VtValue    baselineValue(qh1);
    VtValue    modifiedValue(qh1);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quathCompareValuesDiff)
{
    VtValue    baselineValue(qh1);
    VtValue    modifiedValue(qh2);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesQuaternions, quathCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfQuath>({ qh1, qh2, qh3 }));
    VtValue    modifiedValue(VtArray<GfQuath>({ qh1, qh2, qh3 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quathCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfQuath>({ qh1, qh2, qh3 }));
    VtValue    modifiedValue(VtArray<GfQuath>({ qh1, qh4, qh2 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesQuaternions, quathQuatdCompareValuesSame)
{
    VtValue    baselineValue(qh1);
    VtValue    modifiedValue(qd1);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quathQuatdCompareValuesDiff)
{
    VtValue    baselineValue(qh1);
    VtValue    modifiedValue(qd2);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesQuaternions, quathQuatdCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfQuath>({ qh1, qh2, qh3 }));
    VtValue    modifiedValue(VtArray<GfQuatd>({ qd1, qd2, qd3 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quathQuatdCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfQuath>({ qh1, qh2, qh3 }));
    VtValue    modifiedValue(VtArray<GfQuatd>({ qd1, qd4, qd2 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesQuaternions, quathQuatfCompareValuesSame)
{
    VtValue    baselineValue(qh1);
    VtValue    modifiedValue(qf1);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quathQuatfCompareValuesDiff)
{
    VtValue    baselineValue(qh1);
    VtValue    modifiedValue(qf2);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesQuaternions, quathQuatfCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfQuath>({ qh1, qh2, qh3 }));
    VtValue    modifiedValue(VtArray<GfQuatf>({ qf1, qf2, qf3 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesQuaternions, quathQuatfCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfQuath>({ qh1, qh2, qh3 }));
    VtValue    modifiedValue(VtArray<GfQuatf>({ qf1, qf4, qf2 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}
