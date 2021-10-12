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

    DiffResultMap results = compareObjectsMetadatas(modifiedPrim, baselinePrim);

    // An empty prim still contains a "specifier" metadata containing if it is a "def", "class",
    // "over", etc.
    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(SdfFieldKeys->Specifier), results.end());
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
    DiffResultMap results = compareObjectsMetadatas(modifiedPrim, baselinePrim);

    // An empty prim still contains a "specifier" metadata containing if it is a "def", "class",
    // "over", etc.
    EXPECT_EQ(results.size(), std::size_t(2));
    EXPECT_NE(results.find(testMetaName), results.end());

    DiffResult result = results[testMetaName];
    EXPECT_EQ(result, DiffResult::Same);
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
    DiffResultMap results = compareObjectsMetadatas(modifiedPrim, baselinePrim);

    // An empty prim still contains a "specifier" metadata containing if it is a "def", "class",
    // "over", etc.
    EXPECT_EQ(results.size(), std::size_t(2));
    EXPECT_NE(results.find(testMetaName), results.end());

    DiffResult result = results[testMetaName];
    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffMetadatas, compareMetadatasAbsentDouble)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));

    baselinePrim.SetMetadata<std::string>(testMetaName, "1.0");
    DiffResultMap results = compareObjectsMetadatas(modifiedPrim, baselinePrim);

    // An empty prim still contains a "specifier" metadata containing if it is a "def", "class",
    // "over", etc.
    EXPECT_EQ(results.size(), std::size_t(2));
    EXPECT_NE(results.find(testMetaName), results.end());

    DiffResult result = results[testMetaName];
    EXPECT_EQ(result, DiffResult::Absent);
}

TEST(DiffMetadatas, compareMetadatasCreatedDouble)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));

    modifiedPrim.SetMetadata<std::string>(testMetaName, "1.0");
    DiffResultMap results = compareObjectsMetadatas(modifiedPrim, baselinePrim);

    // An empty prim still contains a "specifier" metadata containing if it is a "def", "class",
    // "over", etc.
    EXPECT_EQ(results.size(), std::size_t(2));
    EXPECT_NE(results.find(testMetaName), results.end());

    DiffResult result = results[testMetaName];
    EXPECT_EQ(result, DiffResult::Created);
}
