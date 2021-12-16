#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/valueTypeName.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

namespace {

static TfToken testRes1("test_res1");
static TfToken testRes2("test_res2");
static TfToken testRes3("test_res3");

} // namespace

//----------------------------------------------------------------------------------------------------------------------
// Tests with all results being the same.

TEST(DiffComputeOverall, computeOverallEmpty)
{
    DiffResultPerToken results;
    DiffResult         result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffComputeOverall, computeOverallAllSame)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Same },
        { testRes2, DiffResult::Same },
        { testRes3, DiffResult::Same },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffComputeOverall, computeOverallAllAbsent)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Absent },
        { testRes2, DiffResult::Absent },
        { testRes3, DiffResult::Absent },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Absent);
}

TEST(DiffComputeOverall, computeOverallAllCreated)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Created },
        { testRes2, DiffResult::Created },
        { testRes3, DiffResult::Created },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Created);
}

TEST(DiffComputeOverall, computeOverallAllPrepended)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Prepended },
        { testRes2, DiffResult::Prepended },
        { testRes3, DiffResult::Prepended },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Created);
}

TEST(DiffComputeOverall, computeOverallAllAppended)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Appended },
        { testRes2, DiffResult::Appended },
        { testRes3, DiffResult::Appended },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Created);
}

TEST(DiffComputeOverall, computeOverallAllSubset)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Subset },
        { testRes2, DiffResult::Subset },
        { testRes3, DiffResult::Subset },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffComputeOverall, computeOverallAllSuperset)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Superset },
        { testRes2, DiffResult::Superset },
        { testRes3, DiffResult::Superset },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffComputeOverall, computeOverallAllDiffer)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Differ },
        { testRes2, DiffResult::Differ },
        { testRes3, DiffResult::Differ },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
// Tests with two types of results: same and another.

TEST(DiffComputeOverall, computeOverallSameAndCreated)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Same },
        { testRes2, DiffResult::Created },
        { testRes3, DiffResult::Same },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Superset);
}

TEST(DiffComputeOverall, computeOverallSameAndAbsent)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Same },
        { testRes2, DiffResult::Same },
        { testRes3, DiffResult::Absent },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Subset);
}

TEST(DiffComputeOverall, computeOverallSameAndPrepended)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Same },
        { testRes2, DiffResult::Prepended },
        { testRes3, DiffResult::Same },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Prepended);
}

TEST(DiffComputeOverall, computeOverallSameAndAppended)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Same },
        { testRes2, DiffResult::Appended },
        { testRes3, DiffResult::Same },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Appended);
}

TEST(DiffComputeOverall, computeOverallSameAndDiffer)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Same },
        { testRes2, DiffResult::Same },
        { testRes3, DiffResult::Differ },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffComputeOverall, computeOverallSameAndSubset)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Same },
        { testRes2, DiffResult::Subset },
        { testRes3, DiffResult::Same },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffComputeOverall, computeOverallSameAndSuperset)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Same },
        { testRes2, DiffResult::Same },
        { testRes3, DiffResult::Superset },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffComputeOverall, computeOverallSameAndReordered)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Same },
        { testRes2, DiffResult::Same },
        { testRes3, DiffResult::Reordered },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Reordered);
}

//----------------------------------------------------------------------------------------------------------------------
// Tests with two types of results: created and another.

TEST(DiffComputeOverall, computeOverallCreatedAndAbsent)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Absent },
        { testRes2, DiffResult::Created },
        { testRes3, DiffResult::Absent },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffComputeOverall, computeOverallCreatedAndAppended)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Created },
        { testRes2, DiffResult::Appended },
        { testRes3, DiffResult::Created },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Created);
}

TEST(DiffComputeOverall, computeOverallCreatedAndPrepended)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Prepended },
        { testRes2, DiffResult::Created },
        { testRes3, DiffResult::Prepended },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Created);
}

TEST(DiffComputeOverall, computeOverallCreatedAndReordered)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Reordered },
        { testRes2, DiffResult::Created },
        { testRes3, DiffResult::Reordered },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
// Tests with two types of results: absent and another.

TEST(DiffComputeOverall, computeOverallAbsentAndAppended)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Absent },
        { testRes2, DiffResult::Absent },
        { testRes3, DiffResult::Appended },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffComputeOverall, computeOverallAbsentAndPrepended)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Absent },
        { testRes2, DiffResult::Absent },
        { testRes3, DiffResult::Prepended },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffComputeOverall, computeOverallAbsentAndReordered)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Absent },
        { testRes2, DiffResult::Absent },
        { testRes3, DiffResult::Reordered },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Subset);
}

//----------------------------------------------------------------------------------------------------------------------
// Tests with two types of results.

TEST(DiffComputeOverall, computeOverallPrependedAndAppended)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Prepended },
        { testRes2, DiffResult::Appended },
        { testRes3, DiffResult::Prepended },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Created);
}

//----------------------------------------------------------------------------------------------------------------------
// Tests with three types of results.

TEST(DiffComputeOverall, computeOverallSameCreatedAbsent)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Same },
        { testRes2, DiffResult::Created },
        { testRes3, DiffResult::Absent },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffComputeOverall, computeOverallSameCreatedReordered)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Same },
        { testRes2, DiffResult::Created },
        { testRes3, DiffResult::Reordered },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffComputeOverall, computeOverallSameAbsentReordered)
{
    DiffResultPerToken results = {
        { testRes1, DiffResult::Same },
        { testRes2, DiffResult::Reordered },
        { testRes3, DiffResult::Absent },
    };
    DiffResult result = computeOverallResult(results);

    EXPECT_EQ(result, DiffResult::Subset);
}
