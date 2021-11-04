#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/valueTypeName.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

static TfToken testAttrName("test_attr");

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffPrims, comparePrimsEmpty)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));

    DiffResultMap results = comparePrimsAttributes(modifiedPrim, baselinePrim);

    EXPECT_TRUE(results.empty());
}

TEST(DiffPrims, comparePrimsSameDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(testAttrName, doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(testAttrName, doubleType, true);

    baselineAttr.Set(1.0);
    modifiedAttr.Set(1.0);
    DiffResultMap results = comparePrimsAttributes(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(testAttrName), results.end());

    DiffResult result = results[testAttrName];
    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffPrims, comparePrimsDiffDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(testAttrName, doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(testAttrName, doubleType, true);

    baselineAttr.Set(1.0);
    modifiedAttr.Set(2.0);
    DiffResultMap results = comparePrimsAttributes(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(testAttrName), results.end());

    DiffResult result = results[testAttrName];
    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffPrims, comparePrimsAbsentDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(testAttrName, doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));

    baselineAttr.Set(1.0);
    DiffResultMap results = comparePrimsAttributes(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(testAttrName), results.end());

    DiffResult result = results[testAttrName];
    EXPECT_EQ(result, DiffResult::Absent);
}

TEST(DiffPrims, comparePrimsCreatedDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(testAttrName, doubleType, true);

    modifiedAttr.Set(1.0);
    DiffResultMap results = comparePrimsAttributes(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(testAttrName), results.end());

    DiffResult result = results[testAttrName];
    EXPECT_EQ(result, DiffResult::Created);
}
