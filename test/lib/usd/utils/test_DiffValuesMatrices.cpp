#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/gf/matrix3d.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/value.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

namespace {
GfMatrix2d m2d1(1, 12, 15, 16);
GfMatrix2d m2d2(2, 22, 28, 21);
GfMatrix2d m2d3(5, 53, 57, 52);
GfMatrix2d m2d4(7, 72, 74, 70);

GfMatrix3d m3d1(1, 12, 15, 16, 11, 17, 19, 18, 10);
GfMatrix3d m3d2(2, 22, 28, 21, 23, 25, 29, 20, 24);
GfMatrix3d m3d3(5, 53, 57, 52, 58, 55, 54, 54, 53);
GfMatrix3d m3d4(7, 72, 74, 70, 71, 73, 77, 78, 75);

GfMatrix4d m4d1(1, 12, 15, 16, 11, 17, 19, 18, 10, 12, 15, 16, 11, 17, 19, 18);
GfMatrix4d m4d2(2, 22, 28, 21, 23, 25, 29, 20, 24, 22, 28, 21, 23, 25, 29, 20);
GfMatrix4d m4d3(5, 53, 57, 52, 58, 55, 54, 54, 53, 53, 57, 52, 58, 55, 54, 54);
GfMatrix4d m4d4(7, 72, 74, 70, 71, 73, 77, 78, 75, 72, 74, 70, 71, 73, 77, 78);
} // namespace

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesMatrices, matrix2dCompareValuesSame)
{
    VtValue    baselineValue(m2d1);
    VtValue    modifiedValue(m2d1);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesMatrices, matrix2dCompareValuesDiff)
{
    VtValue    baselineValue(m2d1);
    VtValue    modifiedValue(m2d2);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesMatrices, matrix2dCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfMatrix2d>({ m2d1, m2d2, m2d3 }));
    VtValue    modifiedValue(VtArray<GfMatrix2d>({ m2d1, m2d2, m2d3 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesMatrices, matrix2dCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfMatrix2d>({ m2d1, m2d2, m2d3 }));
    VtValue    modifiedValue(VtArray<GfMatrix2d>({ m2d1, m2d4, m2d2 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesMatrices, matrix3dCompareValuesSame)
{
    VtValue    baselineValue(m3d1);
    VtValue    modifiedValue(m3d1);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesMatrices, matrix3dCompareValuesDiff)
{
    VtValue    baselineValue(m3d1);
    VtValue    modifiedValue(m3d2);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesMatrices, matrix3dCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfMatrix3d>({ m3d1, m3d2, m3d3 }));
    VtValue    modifiedValue(VtArray<GfMatrix3d>({ m3d1, m3d2, m3d3 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesMatrices, matrix3dCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfMatrix3d>({ m3d1, m3d2, m3d3 }));
    VtValue    modifiedValue(VtArray<GfMatrix3d>({ m3d1, m3d4, m3d2 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesMatrices, matrix4dCompareValuesSame)
{
    VtValue    baselineValue(m4d1);
    VtValue    modifiedValue(m4d1);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesMatrices, matrix4dCompareValuesDiff)
{
    VtValue    baselineValue(m4d1);
    VtValue    modifiedValue(m4d2);
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesMatrices, matrix4dCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfMatrix4d>({ m4d1, m4d2, m4d3 }));
    VtValue    modifiedValue(VtArray<GfMatrix4d>({ m4d1, m4d2, m4d3 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesMatrices, matrix4dCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfMatrix4d>({ m4d1, m4d2, m4d3 }));
    VtValue    modifiedValue(VtArray<GfMatrix4d>({ m4d1, m4d4, m4d2 }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}
