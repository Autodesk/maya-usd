#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/valueTypeName.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

// Note: metadata must be registered, for tests we used a pre-registered one.
static TfToken testMetaName(SdfFieldKeys->Comment);

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffMetadatas, compareMetadatasEmpty)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));

    DiffResultPerToken results = compareObjectsMetadatas(modifiedPrim, baselinePrim);

    EXPECT_TRUE(results.empty());

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareObjectsMetadatas(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffMetadatas, compareMetadatasSameDouble)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));

    baselinePrim.SetMetadata<std::string>(testMetaName, "1.0");
    modifiedPrim.SetMetadata<std::string>(testMetaName, "1.0");
    DiffResultPerToken results = compareObjectsMetadatas(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(testMetaName), results.end());

    DiffResult result = results[testMetaName];
    EXPECT_EQ(result, DiffResult::Same);

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareObjectsMetadatas(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffMetadatas, compareMetadatasDiffDouble)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));

    baselinePrim.SetMetadata<std::string>(testMetaName, "1.0");
    modifiedPrim.SetMetadata<std::string>(testMetaName, "2.0");
    DiffResultPerToken results = compareObjectsMetadatas(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(testMetaName), results.end());

    DiffResult result = results[testMetaName];
    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    compareObjectsMetadatas(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffMetadatas, compareMetadatasAbsentDouble)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));

    baselinePrim.SetMetadata<std::string>(testMetaName, "1.0");
    DiffResultPerToken results = compareObjectsMetadatas(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(testMetaName), results.end());

    DiffResult result = results[testMetaName];
    EXPECT_EQ(result, DiffResult::Absent);

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Absent);

    DiffResult quickDiff = DiffResult::Same;
    compareObjectsMetadatas(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffMetadatas, compareMetadatasCreatedDouble)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));

    modifiedPrim.SetMetadata<std::string>(testMetaName, "1.0");
    DiffResultPerToken results = compareObjectsMetadatas(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(testMetaName), results.end());

    DiffResult result = results[testMetaName];
    EXPECT_EQ(result, DiffResult::Created);

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Created);

    DiffResult quickDiff = DiffResult::Same;
    compareObjectsMetadatas(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}
