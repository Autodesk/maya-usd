#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/valueTypeName.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

namespace {
TfToken testRelName("test_rel");

void addThreeTargetPrims(UsdStageRefPtr stage)
{
    auto targetPrim1 = stage->DefinePrim(SdfPath("/target1"));
    auto targetPrim2 = stage->DefinePrim(SdfPath("/target2"));
    auto targetPrim3 = stage->DefinePrim(SdfPath("/target3"));
}
} // namespace

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffRelationships, compareRelationshipsEmpty)
{
    SdfPath primPath("/A");

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineRel = baselinePrim.CreateRelationship(testRelName, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedRel = modifiedPrim.CreateRelationship(testRelName, true);

    DiffResultPerPath results = compareRelationships(modifiedRel, baselineRel);

    EXPECT_TRUE(results.empty());
}

TEST(DiffRelationships, compareRelationshipsSame)
{
    SdfPath primPath("/A");
    SdfPath targetPath("/target1");

    auto baselineStage = UsdStage::CreateInMemory();
    addThreeTargetPrims(baselineStage);
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineRel = baselinePrim.CreateRelationship(testRelName, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    addThreeTargetPrims(baselineStage);
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedRel = modifiedPrim.CreateRelationship(testRelName, true);

    baselineRel.AddTarget(targetPath);
    modifiedRel.AddTarget(targetPath);

    DiffResultPerPath results = compareRelationships(modifiedRel, baselineRel);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(targetPath), results.end());
    EXPECT_EQ(results[targetPath], DiffResult::Same);
}

TEST(DiffRelationships, compareRelationshipsDiff)
{
    SdfPath primPath("/A");
    SdfPath targetPath("/target1");
    SdfPath targetPath2("/target2");

    auto baselineStage = UsdStage::CreateInMemory();
    addThreeTargetPrims(baselineStage);
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineRel = baselinePrim.CreateRelationship(testRelName, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    addThreeTargetPrims(baselineStage);
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedRel = modifiedPrim.CreateRelationship(testRelName, true);

    baselineRel.AddTarget(targetPath);
    modifiedRel.AddTarget(targetPath2);

    DiffResultPerPath results = compareRelationships(modifiedRel, baselineRel);

    EXPECT_EQ(results.size(), std::size_t(2));
    EXPECT_NE(results.find(targetPath), results.end());
    EXPECT_NE(results.find(targetPath2), results.end());
    EXPECT_EQ(results[targetPath], DiffResult::Absent);
    EXPECT_EQ(results[targetPath2], DiffResult::Prepended);
}

TEST(DiffRelationships, compareRelationshipsAbsent)
{
    SdfPath primPath("/A");
    SdfPath targetPath("/target1");

    auto baselineStage = UsdStage::CreateInMemory();
    addThreeTargetPrims(baselineStage);
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineRel = baselinePrim.CreateRelationship(testRelName, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedRel = modifiedPrim.CreateRelationship(testRelName, true);

    baselineRel.AddTarget(targetPath);

    DiffResultPerPath results = compareRelationships(modifiedRel, baselineRel);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(targetPath), results.end());
    EXPECT_EQ(results[targetPath], DiffResult::Absent);
}

TEST(DiffRelationships, compareRelationshipsCreated)
{
    SdfPath primPath("/A");
    SdfPath targetPath("/target1");

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineRel = baselinePrim.CreateRelationship(testRelName, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    addThreeTargetPrims(baselineStage);
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedRel = modifiedPrim.CreateRelationship(testRelName, true);

    modifiedRel.AddTarget(targetPath);

    DiffResultPerPath results = compareRelationships(modifiedRel, baselineRel);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(targetPath), results.end());
    EXPECT_EQ(results[targetPath], DiffResult::Prepended);
}
