#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/valueTypeName.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

namespace {

const SdfPath primPath("/A");

const SdfPath childPath1("/A/B");
const SdfPath childPath2("/A/C");

const SdfPath targetPath1("/target1");
const SdfPath targetPath2("/target2");
const SdfPath targetPath3("/target3");

const TfToken testAttrName("test_attr");
const TfToken otherAttrName("other_attr");

static TfToken testRelName("test_rel");
static TfToken otherRelName("other_rel");

const SdfValueTypeName doubleType = SdfValueTypeNames->Double;

UsdPrim createPrim(UsdStageRefPtr& stage, const SdfPath& path)
{
    return stage->DefinePrim(SdfPath(path));
}

UsdAttribute createAttr(UsdPrim& prim, const TfToken& attrName, double value)
{
    auto attr = prim.CreateAttribute(attrName, doubleType, true);
    attr.Set(value);
    return attr;
}

UsdAttribute createAttr(UsdPrim& prim, double value)
{
    return createAttr(prim, testAttrName, value);
}

UsdRelationship createRel(UsdPrim& prim, const TfToken& relName, const SdfPath& target)
{
    auto rel = prim.CreateRelationship(relName, true);
    rel.AddTarget(target);
    return rel;
}

UsdPrim createChild(UsdStageRefPtr& stage, const SdfPath& path, double value)
{
    auto child = stage->DefinePrim(SdfPath(path));
    createAttr(child, value);
    return child;
}

void addThreeTargetPrims(UsdStageRefPtr stage)
{
    createPrim(stage, targetPath1);
    createPrim(stage, targetPath2);
    createPrim(stage, targetPath3);
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------
/// Children difference.

TEST(DiffPrims, comparePrimsEmpty)
{
    // Test that prims with no attribute and no children are considered identical.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);

    DiffResult result = comparePrims(modifiedPrim, baselinePrim);

    EXPECT_EQ(result, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    comparePrims(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffPrims, comparePrimsSameChildren)
{
    // Test that prims with no attribute and identical children are considered identical.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    createChild(baselineStage, childPath1, 1.0);
    createChild(baselineStage, childPath2, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 1.0);
    createChild(modifiedStage, childPath2, 1.0);

    DiffResult result = comparePrims(modifiedPrim, baselinePrim);

    EXPECT_EQ(result, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    comparePrims(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffPrims, comparePrimsDiffChildren)
{
    // Test that prims with no attribute and children with different atttribute values are
    // considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    createChild(baselineStage, childPath1, 1.0);
    createChild(baselineStage, childPath2, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 2.0);
    createChild(modifiedStage, childPath2, 3.0);

    DiffResult result = comparePrims(modifiedPrim, baselinePrim);

    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    comparePrims(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffPrims, comparePrimsAbsentChild)
{
    // Test that prims with no attribute and a mssing child are considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    createChild(baselineStage, childPath1, 1.0);
    createChild(baselineStage, childPath2, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 1.0);

    DiffResult result = comparePrims(modifiedPrim, baselinePrim);

    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    comparePrims(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffPrims, comparePrimsCreatedChild)
{
    // Test that prims with no attribute and an extra child are considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    createChild(baselineStage, childPath1, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    createChild(modifiedStage, childPath1, 1.0);
    createChild(modifiedStage, childPath2, 2.0);

    DiffResult result = comparePrims(modifiedPrim, baselinePrim);

    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    comparePrims(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

//----------------------------------------------------------------------------------------------------------------------
/// Children attribute differences.

TEST(DiffPrims, comparePrimsAbsentChildAttribute)
{
    // Test that prims with no attribute and the same children but with a missing attribute are
    // considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild = createChild(baselineStage, childPath1, 1.0);
    createAttr(baselineChild, otherAttrName, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    auto modifiedChild = createChild(modifiedStage, childPath1, 1.0);

    DiffResult result = comparePrims(modifiedPrim, baselinePrim);

    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    comparePrims(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffPrims, comparePrimsCreatedChildAttribute)
{
    // Test that prims with no attribute and the same children but with an extra attribute are
    // considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    createChild(baselineStage, childPath1, 1.0);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    auto modifiedChild = createChild(modifiedStage, childPath1, 1.0);
    createAttr(modifiedChild, otherAttrName, 1.0);

    DiffResult result = comparePrims(modifiedPrim, baselinePrim);

    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    comparePrims(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

//----------------------------------------------------------------------------------------------------------------------
/// Children relationship differences.

TEST(DiffPrims, comparePrimsAbsentChildRelationship)
{
    // Test that prims with the same children but with a missing relationship are
    // considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild = createChild(baselineStage, childPath1, 1.0);
    createRel(baselineChild, testRelName, targetPath1);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    auto modifiedChild = createChild(modifiedStage, childPath1, 1.0);

    DiffResult result = comparePrims(modifiedPrim, baselinePrim);

    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    comparePrims(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffPrims, comparePrimsCreatedChildRelationship)
{
    // Test that prims with no relationship and the same children but with an extra relationship are
    // considered different.

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = createPrim(baselineStage, primPath);
    auto baselineChild = createChild(baselineStage, childPath1, 1.0);
    createRel(baselineChild, testRelName, targetPath1);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = createPrim(modifiedStage, primPath);
    auto modifiedChild = createChild(modifiedStage, childPath1, 1.0);
    createRel(modifiedChild, testRelName, targetPath1);
    createRel(modifiedChild, otherRelName, targetPath2);

    DiffResult result = comparePrims(modifiedPrim, baselinePrim);

    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    comparePrims(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}
