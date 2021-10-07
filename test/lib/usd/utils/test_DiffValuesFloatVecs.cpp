#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/gf/vec2d.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec2h.h>
#include <pxr/base/gf/vec2i.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3h.h>
#include <pxr/base/gf/vec3i.h>
#include <pxr/base/gf/vec4d.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/gf/vec4h.h>
#include <pxr/base/gf/vec4i.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/value.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesFloatVecs, vec2fCompareValuesSame)
{
    VtValue    baselineValue(GfVec2f(1, 12));
    VtValue    modifiedValue(GfVec2f(1, 12));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatVecs, vec2fCompareValuesDiff)
{
    VtValue    baselineValue(GfVec2f(1, 12));
    VtValue    modifiedValue(GfVec2f(2, 22));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatVecs, vec2fCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfVec2f>({ GfVec2f(1, 12), GfVec2f(5, 53), GfVec2f(7, 71) }));
    VtValue    modifiedValue(VtArray<GfVec2f>({ GfVec2f(1, 12), GfVec2f(5, 53), GfVec2f(7, 71) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatVecs, vec2fCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfVec2f>({ GfVec2f(1, 12), GfVec2f(5, 53), GfVec2f(7, 71) }));
    VtValue    modifiedValue(VtArray<GfVec2f>({ GfVec2f(1, 12), GfVec2f(3, 38), GfVec2f(4, 46) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatVecs, vec3fCompareValuesSame)
{
    VtValue    baselineValue(GfVec3f(1, 12, 21));
    VtValue    modifiedValue(GfVec3f(1, 12, 21));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatVecs, vec3fCompareValuesDiff)
{
    VtValue    baselineValue(GfVec3f(1, 12, 21));
    VtValue    modifiedValue(GfVec3f(2, 22, 25));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatVecs, vec3fCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfVec3f>({ GfVec3f(1, 12, 21), GfVec3f(5, 53, 35), GfVec3f(7, 71, 17) }));
    VtValue    modifiedValue(VtArray<GfVec3f>({ GfVec3f(1, 12, 21), GfVec3f(5, 53, 35), GfVec3f(7, 71, 17) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatVecs, vec3fCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfVec3f>({ GfVec3f(1, 12, 21), GfVec3f(5, 53, 35), GfVec3f(7, 71, 17) }));
    VtValue    modifiedValue(VtArray<GfVec3f>({ GfVec3f(1, 12, 21), GfVec3f(3, 38, 83), GfVec3f(4, 46, 64) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatVecs, vec4fCompareValuesSame)
{
    VtValue    baselineValue(GfVec4f(1, 12, 21, 32));
    VtValue    modifiedValue(GfVec4f(1, 12, 21, 32));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatVecs, vec4fCompareValuesDiff)
{
    VtValue    baselineValue(GfVec4f(1, 12, 21, 32));
    VtValue    modifiedValue(GfVec4f(2, 22, 33, 44));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatVecs, vec4fCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfVec4f>({ GfVec4f(1, 12, 21, 32), GfVec4f(5, 53, 35, 46), GfVec4f(7, 71, 72, 73) }));
    VtValue    modifiedValue(VtArray<GfVec4f>({ GfVec4f(1, 12, 21, 32), GfVec4f(5, 53, 35, 46), GfVec4f(7, 71, 72, 73) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatVecs, vec4fCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfVec4f>({ GfVec4f(1, 12, 21, 32), GfVec4f(5, 53, 35, 46), GfVec4f(7, 71, 72, 73) }));
    VtValue    modifiedValue(VtArray<GfVec4f>({ GfVec4f(1, 12, 21, 32), GfVec4f(3, 38, 83, 37), GfVec4f(4, 46, 47, 48) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesFloatDoubleVecs, vec2fCompareValuesSame)
{
    VtValue    baselineValue(GfVec2f(1, 12));
    VtValue    modifiedValue(GfVec2d(1, 12));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatDoubleVecs, vec2fCompareValuesDiff)
{
    VtValue    baselineValue(GfVec2f(1, 12));
    VtValue    modifiedValue(GfVec2d(2, 22));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatDoubleVecs, vec2fCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfVec2f>({ GfVec2f(1, 12), GfVec2f(5, 53), GfVec2f(7, 71) }));
    VtValue    modifiedValue(VtArray<GfVec2d>({ GfVec2d(1, 12), GfVec2d(5, 53), GfVec2d(7, 71) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatDoubleVecs, vec2fCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfVec2f>({ GfVec2f(1, 12), GfVec2f(5, 53), GfVec2f(7, 71) }));
    VtValue    modifiedValue(VtArray<GfVec2d>({ GfVec2d(1, 12), GfVec2d(3, 38), GfVec2d(4, 46) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatDoubleVecs, vec3fCompareValuesSame)
{
    VtValue    baselineValue(GfVec3f(1, 12, 21));
    VtValue    modifiedValue(GfVec3d(1, 12, 21));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatDoubleVecs, vec3fCompareValuesDiff)
{
    VtValue    baselineValue(GfVec3f(1, 12, 21));
    VtValue    modifiedValue(GfVec3d(2, 22, 25));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatDoubleVecs, vec3fCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfVec3f>({ GfVec3f(1, 12, 21), GfVec3f(5, 53, 35), GfVec3f(7, 71, 17) }));
    VtValue    modifiedValue(VtArray<GfVec3d>({ GfVec3d(1, 12, 21), GfVec3d(5, 53, 35), GfVec3d(7, 71, 17) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatDoubleVecs, vec3fCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfVec3f>({ GfVec3f(1, 12, 21), GfVec3f(5, 53, 35), GfVec3f(7, 71, 17) }));
    VtValue    modifiedValue(VtArray<GfVec3d>({ GfVec3d(1, 12, 21), GfVec3d(3, 38, 83), GfVec3d(4, 46, 64) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatDoubleVecs, vec4fCompareValuesSame)
{
    VtValue    baselineValue(GfVec4f(1, 12, 21, 32));
    VtValue    modifiedValue(GfVec4d(1, 12, 21, 32));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatDoubleVecs, vec4fCompareValuesDiff)
{
    VtValue    baselineValue(GfVec4f(1, 12, 21, 32));
    VtValue    modifiedValue(GfVec4d(2, 22, 33, 44));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatDoubleVecs, vec4fCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfVec4f>({ GfVec4f(1, 12, 21, 32), GfVec4f(5, 53, 35, 46), GfVec4f(7, 71, 72, 73) }));
    VtValue    modifiedValue(VtArray<GfVec4d>({ GfVec4d(1, 12, 21, 32), GfVec4d(5, 53, 35, 46), GfVec4d(7, 71, 72, 73) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatDoubleVecs, vec4fCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfVec4f>({ GfVec4f(1, 12, 21, 32), GfVec4f(5, 53, 35, 46), GfVec4f(7, 71, 72, 73) }));
    VtValue    modifiedValue(VtArray<GfVec4d>({ GfVec4d(1, 12, 21, 32), GfVec4d(3, 38, 83, 37), GfVec4d(4, 46, 47, 48) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesFloatHalfVecs, vec2fCompareValuesSame)
{
    VtValue    baselineValue(GfVec2f(1, 12));
    VtValue    modifiedValue(GfVec2h(1, 12));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatHalfVecs, vec2fCompareValuesDiff)
{
    VtValue    baselineValue(GfVec2f(1, 12));
    VtValue    modifiedValue(GfVec2h(2, 22));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatHalfVecs, vec2fCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfVec2f>({ GfVec2f(1, 12), GfVec2f(5, 53), GfVec2f(7, 71) }));
    VtValue    modifiedValue(VtArray<GfVec2h>({ GfVec2h(1, 12), GfVec2h(5, 53), GfVec2h(7, 71) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatHalfVecs, vec2fCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfVec2f>({ GfVec2f(1, 12), GfVec2f(5, 53), GfVec2f(7, 71) }));
    VtValue    modifiedValue(VtArray<GfVec2h>({ GfVec2h(1, 12), GfVec2h(3, 38), GfVec2h(4, 46) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatHalfVecs, vec3fCompareValuesSame)
{
    VtValue    baselineValue(GfVec3f(1, 12, 21));
    VtValue    modifiedValue(GfVec3h(1, 12, 21));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatHalfVecs, vec3fCompareValuesDiff)
{
    VtValue    baselineValue(GfVec3f(1, 12, 21));
    VtValue    modifiedValue(GfVec3h(2, 22, 25));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatHalfVecs, vec3fCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfVec3f>({ GfVec3f(1, 12, 21), GfVec3f(5, 53, 35), GfVec3f(7, 71, 17) }));
    VtValue    modifiedValue(VtArray<GfVec3h>({ GfVec3h(1, 12, 21), GfVec3h(5, 53, 35), GfVec3h(7, 71, 17) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatHalfVecs, vec3fCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfVec3f>({ GfVec3f(1, 12, 21), GfVec3f(5, 53, 35), GfVec3f(7, 71, 17) }));
    VtValue    modifiedValue(VtArray<GfVec3h>({ GfVec3h(1, 12, 21), GfVec3h(3, 38, 83), GfVec3h(4, 46, 64) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatHalfVecs, vec4fCompareValuesSame)
{
    VtValue    baselineValue(GfVec4f(1, 12, 21, 32));
    VtValue    modifiedValue(GfVec4h(1, 12, 21, 32));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatHalfVecs, vec4fCompareValuesDiff)
{
    VtValue    baselineValue(GfVec4f(1, 12, 21, 32));
    VtValue    modifiedValue(GfVec4h(2, 22, 33, 44));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesFloatHalfVecs, vec4fCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfVec4f>({ GfVec4f(1, 12, 21, 32), GfVec4f(5, 53, 35, 46), GfVec4f(7, 71, 72, 73) }));
    VtValue    modifiedValue(VtArray<GfVec4h>({ GfVec4h(1, 12, 21, 32), GfVec4h(5, 53, 35, 46), GfVec4h(7, 71, 72, 73) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesFloatHalfVecs, vec4fCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfVec4f>({ GfVec4f(1, 12, 21, 32), GfVec4f(5, 53, 35, 46), GfVec4f(7, 71, 72, 73) }));
    VtValue    modifiedValue(VtArray<GfVec4h>({ GfVec4h(1, 12, 21, 32), GfVec4h(3, 38, 83, 37), GfVec4h(4, 46, 47, 48) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}
