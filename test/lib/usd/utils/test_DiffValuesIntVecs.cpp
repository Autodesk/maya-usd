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
TEST(DiffValuesIntVecs, vec2iCompareValuesSame)
{
    VtValue    baselineValue(GfVec2i(1, 12));
    VtValue    modifiedValue(GfVec2i(1, 12));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesIntVecs, vec2iCompareValuesDiff)
{
    VtValue    baselineValue(GfVec2i(1, 12));
    VtValue    modifiedValue(GfVec2i(2, 22));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesIntVecs, vec2iCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfVec2i>({ GfVec2i(1, 12), GfVec2i(5, 53), GfVec2i(7, 71) }));
    VtValue    modifiedValue(VtArray<GfVec2i>({ GfVec2i(1, 12), GfVec2i(5, 53), GfVec2i(7, 71) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesIntVecs, vec2iCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfVec2i>({ GfVec2i(1, 12), GfVec2i(5, 53), GfVec2i(7, 71) }));
    VtValue    modifiedValue(VtArray<GfVec2i>({ GfVec2i(1, 12), GfVec2i(3, 38), GfVec2i(4, 46) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesIntVecs, vec3iCompareValuesSame)
{
    VtValue    baselineValue(GfVec3i(1, 12, 21));
    VtValue    modifiedValue(GfVec3i(1, 12, 21));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesIntVecs, vec3iCompareValuesDiff)
{
    VtValue    baselineValue(GfVec3i(1, 12, 21));
    VtValue    modifiedValue(GfVec3i(2, 22, 25));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesIntVecs, vec3iCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfVec3i>({ GfVec3i(1, 12, 21), GfVec3i(5, 53, 35), GfVec3i(7, 71, 17) }));
    VtValue    modifiedValue(VtArray<GfVec3i>({ GfVec3i(1, 12, 21), GfVec3i(5, 53, 35), GfVec3i(7, 71, 17) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesIntVecs, vec3iCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfVec3i>({ GfVec3i(1, 12, 21), GfVec3i(5, 53, 35), GfVec3i(7, 71, 17) }));
    VtValue    modifiedValue(VtArray<GfVec3i>({ GfVec3i(1, 12, 21), GfVec3i(3, 38, 83), GfVec3i(4, 46, 64) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesIntVecs, vec4iCompareValuesSame)
{
    VtValue    baselineValue(GfVec4i(1, 12, 21, 32));
    VtValue    modifiedValue(GfVec4i(1, 12, 21, 32));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesIntVecs, vec4iCompareValuesDiff)
{
    VtValue    baselineValue(GfVec4i(1, 12, 21, 32));
    VtValue    modifiedValue(GfVec4i(2, 22, 33, 44));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesIntVecs, vec4iCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<GfVec4i>({ GfVec4i(1, 12, 21, 32), GfVec4i(5, 53, 35, 46), GfVec4i(7, 71, 72, 73) }));
    VtValue    modifiedValue(VtArray<GfVec4i>({ GfVec4i(1, 12, 21, 32), GfVec4i(5, 53, 35, 46), GfVec4i(7, 71, 72, 73) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesIntVecs, vec4iCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<GfVec4i>({ GfVec4i(1, 12, 21, 32), GfVec4i(5, 53, 35, 46), GfVec4i(7, 71, 72, 73) }));
    VtValue    modifiedValue(VtArray<GfVec4i>({ GfVec4i(1, 12, 21, 32), GfVec4i(3, 38, 83, 37), GfVec4i(4, 46, 47, 48) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

