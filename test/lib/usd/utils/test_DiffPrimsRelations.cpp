#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/valueTypeName.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

namespace {

static TfToken testRelName("test_rel");

void addThreeTargetPrims(UsdStageRefPtr stage)
{
    auto targetPrim1 = stage->DefinePrim(SdfPath("/target1"));
    auto targetPrim2 = stage->DefinePrim(SdfPath("/target2"));
    auto targetPrim3 = stage->DefinePrim(SdfPath("/target3"));
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffPrimsRelationships, comparePrimsRelsEmpty)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));

    DiffResultPerPathPerToken results = comparePrimsRelationships(modifiedPrim, baselinePrim);

    EXPECT_TRUE(results.empty());

    DiffResult quickDiff = DiffResult::Differ;
    comparePrimsRelationships(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffPrimsRelationships, comparePrimsRelsSame)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    addThreeTargetPrims(baselineStage);
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineRel = baselinePrim.CreateRelationship(testRelName, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    addThreeTargetPrims(baselineStage);
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedRel = modifiedPrim.CreateRelationship(testRelName, true);

    baselineRel.AddTarget(SdfPath("/target1"));
    modifiedRel.AddTarget(SdfPath("/target1"));
    DiffResultPerPathPerToken results = comparePrimsRelationships(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(testRelName), results.end());

    DiffResult result = computeOverallResult(results[testRelName]);
    EXPECT_EQ(result, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    comparePrimsRelationships(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffPrimsRelationships, comparePrimsRelsDiff)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    addThreeTargetPrims(baselineStage);
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineRel = baselinePrim.CreateRelationship(testRelName, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    addThreeTargetPrims(baselineStage);
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedRel = modifiedPrim.CreateRelationship(testRelName, true);

    baselineRel.AddTarget(SdfPath("/target1"));
    modifiedRel.AddTarget(SdfPath("/target2"));
    DiffResultPerPathPerToken results = comparePrimsRelationships(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(testRelName), results.end());

    DiffResult result = computeOverallResult(results[testRelName]);
    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    comparePrimsRelationships(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffPrimsRelationships, comparePrimsRelsAbsent)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    addThreeTargetPrims(baselineStage);
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineRel = baselinePrim.CreateRelationship(testRelName, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));

    baselineRel.AddTarget(SdfPath("/target1"));
    DiffResultPerPathPerToken results = comparePrimsRelationships(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(testRelName), results.end());

    DiffResult result = computeOverallResult(results[testRelName]);
    EXPECT_EQ(result, DiffResult::Absent);

    DiffResult quickDiff = DiffResult::Same;
    comparePrimsRelationships(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffPrimsRelationships, comparePrimsRelsCreated)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));

    auto modifiedStage = UsdStage::CreateInMemory();
    addThreeTargetPrims(baselineStage);
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedRel = modifiedPrim.CreateRelationship(testRelName, true);

    modifiedRel.AddTarget(SdfPath("/target1"));
    DiffResultPerPathPerToken results = comparePrimsRelationships(modifiedPrim, baselinePrim);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(testRelName), results.end());

    DiffResult result = computeOverallResult(results[testRelName]);
    EXPECT_EQ(result, DiffResult::Created);

    DiffResult quickDiff = DiffResult::Same;
    comparePrimsRelationships(modifiedPrim, baselinePrim, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}
