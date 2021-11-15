#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/valueTypeName.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

namespace {

const SdfPath          primPath("/A");
const SdfPath          childPath1("/A/B");
const SdfPath          childPath2("/A/C");
const TfToken          testAttrName("test_attr");
const SdfValueTypeName doubleType = SdfValueTypeNames->Double;

UsdPrim createPrim(UsdStageRefPtr& stage, const SdfPath& path)
{
    return stage->DefinePrim(SdfPath(path));
}

UsdPrim createChild(UsdStageRefPtr& stage, const SdfPath& path, double value)
{
    auto child = stage->DefinePrim(SdfPath(path));
    auto childAttr = child.CreateAttribute(testAttrName, doubleType, true);
    childAttr.Set(value);
    return child;
}
} // namespace

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffPrimsChildren, comparePrimsChildrenEmpty)
{
    // Test that empty prims are considered equal.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);

    DiffResultPerPath results = comparePrimsChildren(modifiedPrim, baselinePrim);

    EXPECT_TRUE(results.empty());

    DiffResult quickDiff = DiffResult::Differ;
    comparePrimsChildren(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffPrimsChildren, comparePrimsChildrenSame)
{
    // Test that prims with the same children with the same attributes are considered equal.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    createChild(baselineStage, childPath1, 1.0);
    createChild(baselineStage, childPath2, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 1.0);
    createChild(modifiedStage, childPath2, 1.0);

    DiffResultPerPath results = comparePrimsChildren(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(2));
    EXPECT_NE(results.find(childPath1), results.end());
    EXPECT_NE(results.find(childPath2), results.end());

    EXPECT_EQ(results[childPath1], DiffResult::Same);
    EXPECT_EQ(results[childPath2], DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    comparePrimsChildren(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffPrimsChildren, comparePrimsChildrenDiffDouble)
{
    // Test that prims with the same children with the same attributes but different values are
    // considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    createChild(baselineStage, childPath1, 1.0);
    createChild(baselineStage, childPath2, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 2.0);
    createChild(modifiedStage, childPath2, 3.0);

    DiffResultPerPath results = comparePrimsChildren(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(2));
    EXPECT_NE(results.find(childPath1), results.end());
    EXPECT_NE(results.find(childPath2), results.end());

    EXPECT_EQ(results[childPath1], DiffResult::Differ);
    EXPECT_EQ(results[childPath2], DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    comparePrimsChildren(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffPrimsChildren, comparePrimsChildrenAbsent)
{
    // Test that prims with a missing child are considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    createChild(baselineStage, childPath1, 1.0);
    createChild(baselineStage, childPath2, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 1.0);

    DiffResultPerPath results = comparePrimsChildren(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(2));
    EXPECT_NE(results.find(childPath1), results.end());
    EXPECT_NE(results.find(childPath2), results.end());

    EXPECT_EQ(results[childPath1], DiffResult::Same);
    EXPECT_EQ(results[childPath2], DiffResult::Absent);

    DiffResult quickDiff = DiffResult::Same;
    comparePrimsChildren(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffPrimsChildren, comparePrimsChildrenCreated)
{
    // Test that prims with an extra child are considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    createChild(baselineStage, childPath1, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 1.0);
    createChild(modifiedStage, childPath2, 2.0);

    DiffResultPerPath results = comparePrimsChildren(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(2));
    EXPECT_NE(results.find(childPath1), results.end());
    EXPECT_NE(results.find(childPath2), results.end());

    EXPECT_EQ(results[childPath1], DiffResult::Same);
    EXPECT_EQ(results[childPath2], DiffResult::Created);

    DiffResult quickDiff = DiffResult::Same;
    comparePrimsChildren(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}
