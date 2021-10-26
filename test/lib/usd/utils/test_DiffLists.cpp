#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/valueTypeName.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

//----------------------------------------------------------------------------------------------------------------------
// Integers list.

TEST(DiffLists, compareIntListsEmpty)
{
    std::vector<int> baselineList;
    std::vector<int> modifiedList;

    std::map<int, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_TRUE(results.empty());

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareIntListsSame)
{
    std::vector<int> baselineList({ 1, 2, 3 });
    std::vector<int> modifiedList({ 1, 2, 3 });

    std::map<int, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find(1), results.end());
    EXPECT_EQ(results[1], DiffResult::Same);
    EXPECT_NE(results.find(2), results.end());
    EXPECT_EQ(results[2], DiffResult::Same);
    EXPECT_NE(results.find(3), results.end());
    EXPECT_EQ(results[3], DiffResult::Same);

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareIntListsDiff1)
{
    std::vector<int> baselineList({ 1, 2, 3 });
    std::vector<int> modifiedList({ 1, 4, 3 });

    std::map<int, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(4));
    EXPECT_NE(results.find(1), results.end());
    EXPECT_EQ(results[1], DiffResult::Same);
    EXPECT_NE(results.find(2), results.end());
    EXPECT_EQ(results[2], DiffResult::Absent);
    // Note: result for 3 differ because the modified list placed it after the new item 4, but it
    // is also in the baseline list and needs to be removed from its old positions, so it must both
    // be deleted and appended.
    EXPECT_NE(results.find(3), results.end());
    EXPECT_EQ(results[3], DiffResult::Reordered);
    EXPECT_NE(results.find(4), results.end());
    EXPECT_EQ(results[4], DiffResult::Appended);

    DiffResult overall = computeOverallResult(results);

    // Reordered + absent + appended: overall differ.
    EXPECT_EQ(overall, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareIntListsDiff2)
{
    std::vector<int> baselineList({ 1, 2, 3 });
    std::vector<int> modifiedList({ 2, 1, 3 });

    std::map<int, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find(1), results.end());
    EXPECT_EQ(results[1], DiffResult::Same);
    // Note: result for 2 differ because the modified list placed it at the start, but it
    // was later in the list and had to be deleted from there, so it must both be prepended and
    // deleted.
    EXPECT_NE(results.find(2), results.end());
    EXPECT_EQ(results[2], DiffResult::Reordered);
    // Note: result for 3 differ because the modified list placed it after item 1, but it
    // was already in the baseline list and had to be removed, so it must both be deleted and
    // appended.
    EXPECT_NE(results.find(3), results.end());
    EXPECT_EQ(results[3], DiffResult::Reordered);

    DiffResult overall = computeOverallResult(results);

    // Same + Reordered: overall reordered.
    EXPECT_EQ(overall, DiffResult::Reordered);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareIntListsDiff3)
{
    std::vector<int> baselineList({ 1, 2, 3 });
    std::vector<int> modifiedList({ 3, 2, 1 });

    std::map<int, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find(1), results.end());
    EXPECT_EQ(results[1], DiffResult::Same);
    // Note: result for 2 differ because the modified list placed it at the start, but it
    // was later in the list and had to be deleted from there, so it must both be prepended and
    // deleted.
    EXPECT_NE(results.find(2), results.end());
    EXPECT_EQ(results[2], DiffResult::Reordered);
    // Note: result for 3 differ because the modified list placed it at the start, but it
    // was later in the list and had to be deleted from there, so it must both be prepended and
    // deleted.
    EXPECT_NE(results.find(3), results.end());
    EXPECT_EQ(results[3], DiffResult::Reordered);

    DiffResult overall = computeOverallResult(results);

    // Same + Reordered: overall reordered.
    EXPECT_EQ(overall, DiffResult::Reordered);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareIntListsAbsent)
{
    std::vector<int> baselineList({ 1, 2, 3 });
    std::vector<int> modifiedList;

    std::map<int, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find(1), results.end());
    EXPECT_EQ(results[1], DiffResult::Absent);
    EXPECT_NE(results.find(2), results.end());
    EXPECT_EQ(results[2], DiffResult::Absent);
    EXPECT_NE(results.find(3), results.end());
    EXPECT_EQ(results[3], DiffResult::Absent);

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Absent);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareIntListsPrepended)
{
    std::vector<int> baselineList;
    std::vector<int> modifiedList({ 1, 2, 3 });

    std::map<int, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find(1), results.end());
    EXPECT_EQ(results[1], DiffResult::Prepended);
    EXPECT_NE(results.find(2), results.end());
    EXPECT_EQ(results[2], DiffResult::Prepended);
    EXPECT_NE(results.find(3), results.end());
    EXPECT_EQ(results[3], DiffResult::Prepended);

    DiffResult overall = computeOverallResult(results);

    // Note: while we set each item as prepended because they don't have an overall view
    // to decide which kind of results there will be, once we calculate the overall result
    // all-prepended is returned as created as that is the more general term, because
    // all-prepended is equivalent to all-appended.
    EXPECT_EQ(overall, DiffResult::Created);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

//----------------------------------------------------------------------------------------------------------------------
// strings list.

TEST(DiffLists, compareStringListsEmpty)
{
    std::vector<std::string> baselineList;
    std::vector<std::string> modifiedList;

    std::map<std::string, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_TRUE(results.empty());

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareStringListsSame)
{
    std::vector<std::string> baselineList({ "1", "2", "3" });
    std::vector<std::string> modifiedList({ "1", "2", "3" });

    std::map<std::string, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find("1"), results.end());
    EXPECT_EQ(results["1"], DiffResult::Same);
    EXPECT_NE(results.find("2"), results.end());
    EXPECT_EQ(results["2"], DiffResult::Same);
    EXPECT_NE(results.find("3"), results.end());
    EXPECT_EQ(results["3"], DiffResult::Same);

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareStringListsDiff1)
{
    std::vector<std::string> baselineList({ "1", "2", "3" });
    std::vector<std::string> modifiedList({ "1", "4", "3" });

    std::map<std::string, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(4));
    EXPECT_NE(results.find("1"), results.end());
    EXPECT_EQ(results["1"], DiffResult::Same);
    EXPECT_NE(results.find("2"), results.end());
    EXPECT_EQ(results["2"], DiffResult::Absent);
    // Note: result for "3" differ because the modified list placed it after the new item 4, but it
    // is also in the baseline list and needs to be removed from its old positions, so it must both
    // be deleted and appended.
    EXPECT_NE(results.find("3"), results.end());
    EXPECT_EQ(results["3"], DiffResult::Reordered);
    EXPECT_NE(results.find("4"), results.end());
    EXPECT_EQ(results["4"], DiffResult::Appended);

    DiffResult overall = computeOverallResult(results);

    // Reordered + absent + appended: overall differ.
    EXPECT_EQ(overall, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareStringListsDiff2)
{
    std::vector<std::string> baselineList({ "1", "2", "3" });
    std::vector<std::string> modifiedList({ "2", "1", "3" });

    std::map<std::string, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find("1"), results.end());
    EXPECT_EQ(results["1"], DiffResult::Same);
    // Note: result for "2" differ because the modified list placed it at the start, but it
    // was later in the list and had to be deleted from there, so it must both be prepended and
    // deleted.
    EXPECT_NE(results.find("2"), results.end());
    EXPECT_EQ(results["2"], DiffResult::Reordered);
    // Note: result for "3" differ because the modified list placed it after item "1", but it
    // was already in the baseline list and had to be removed, so it must both be deleted and
    // appended.
    EXPECT_NE(results.find("3"), results.end());
    EXPECT_EQ(results["3"], DiffResult::Reordered);

    DiffResult overall = computeOverallResult(results);

    // Same + Reordered: overall reordered.
    EXPECT_EQ(overall, DiffResult::Reordered);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareStringListsDiff3)
{
    std::vector<std::string> baselineList({ "1", "2", "3" });
    std::vector<std::string> modifiedList({ "3", "2", "1" });

    std::map<std::string, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find("1"), results.end());
    EXPECT_EQ(results["1"], DiffResult::Same);
    // Note: result for "2" differ because the modified list placed it at the start, but it
    // was later in the list and had to be deleted from there, so it must both be prepended and
    // deleted.
    EXPECT_NE(results.find("2"), results.end());
    EXPECT_EQ(results["2"], DiffResult::Reordered);
    // Note: result for "3" differ because the modified list placed it at the start, but it
    // was later in the list and had to be deleted from there, so it must both be prepended and
    // deleted.
    EXPECT_NE(results.find("3"), results.end());
    EXPECT_EQ(results["3"], DiffResult::Reordered);

    DiffResult overall = computeOverallResult(results);

    // Same + Reordered: overall reordered.
    EXPECT_EQ(overall, DiffResult::Reordered);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareStringListsAbsent)
{
    std::vector<std::string> baselineList({ "1", "2", "3" });
    std::vector<std::string> modifiedList;

    std::map<std::string, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find("1"), results.end());
    EXPECT_EQ(results["1"], DiffResult::Absent);
    EXPECT_NE(results.find("2"), results.end());
    EXPECT_EQ(results["2"], DiffResult::Absent);
    EXPECT_NE(results.find("3"), results.end());
    EXPECT_EQ(results["3"], DiffResult::Absent);

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Absent);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareStringListsPrepended)
{
    std::vector<std::string> baselineList;
    std::vector<std::string> modifiedList({ "1", "2", "3" });

    std::map<std::string, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find("1"), results.end());
    EXPECT_EQ(results["1"], DiffResult::Prepended);
    EXPECT_NE(results.find("2"), results.end());
    EXPECT_EQ(results["2"], DiffResult::Prepended);
    EXPECT_NE(results.find("3"), results.end());
    EXPECT_EQ(results["3"], DiffResult::Prepended);

    DiffResult overall = computeOverallResult(results);

    // Note: while we set each item as prepended because they don't have an overall view
    // to decide which kind of results there will be, once we calculate the overall result
    // all-prepended is returned as created as that is the more general term, because
    // all-prepended is equivalent to all-appended.
    EXPECT_EQ(overall, DiffResult::Created);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

//----------------------------------------------------------------------------------------------------------------------
// Tokens list.

TEST(DiffLists, compareTokenListsEmpty)
{
    std::vector<TfToken> baselineList;
    std::vector<TfToken> modifiedList;

    std::map<TfToken, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_TRUE(results.empty());

    DiffResult quickDiff = DiffResult::Differ;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareTokenListsSame)
{
    std::vector<TfToken> baselineList({ TfToken("1"), TfToken("2"), TfToken("3") });
    std::vector<TfToken> modifiedList({ TfToken("1"), TfToken("2"), TfToken("3") });

    std::map<TfToken, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find(TfToken("1")), results.end());
    EXPECT_EQ(results[TfToken("1")], DiffResult::Same);
    EXPECT_NE(results.find(TfToken("2")), results.end());
    EXPECT_EQ(results[TfToken("2")], DiffResult::Same);
    EXPECT_NE(results.find(TfToken("3")), results.end());
    EXPECT_EQ(results[TfToken("3")], DiffResult::Same);

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareTokenListsDiff1)
{
    std::vector<TfToken> baselineList({ TfToken("1"), TfToken("2"), TfToken("3") });
    std::vector<TfToken> modifiedList({ TfToken("1"), TfToken("4"), TfToken("3") });

    std::map<TfToken, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(4));
    EXPECT_NE(results.find(TfToken("1")), results.end());
    EXPECT_EQ(results[TfToken("1")], DiffResult::Same);
    EXPECT_NE(results.find(TfToken("2")), results.end());
    EXPECT_EQ(results[TfToken("2")], DiffResult::Absent);
    // Note: result for TfToken("3") differ because the modified list placed it after the new item 4, but it
    // is also in the baseline list and needs to be removed from its old positions, so it must both
    // be deleted and appended.
    EXPECT_NE(results.find(TfToken("3")), results.end());
    EXPECT_EQ(results[TfToken("3")], DiffResult::Reordered);
    EXPECT_NE(results.find(TfToken("4")), results.end());
    EXPECT_EQ(results[TfToken("4")], DiffResult::Appended);

    DiffResult overall = computeOverallResult(results);

    // Reordered + absent + appended: overall differ.
    EXPECT_EQ(overall, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareTokenListsDiff2)
{
    std::vector<TfToken> baselineList({ TfToken("1"), TfToken("2"), TfToken("3") });
    std::vector<TfToken> modifiedList({ TfToken("2"), TfToken("1"), TfToken("3") });

    std::map<TfToken, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find(TfToken("1")), results.end());
    EXPECT_EQ(results[TfToken("1")], DiffResult::Same);
    // Note: result for TfToken("2") differ because the modified list placed it at the start, but it
    // was later in the list and had to be deleted from there, so it must both be prepended and
    // deleted.
    EXPECT_NE(results.find(TfToken("2")), results.end());
    EXPECT_EQ(results[TfToken("2")], DiffResult::Reordered);
    // Note: result for TfToken("3") differ because the modified list placed it after item TfToken("1"), but it
    // was already in the baseline list and had to be removed, so it must both be deleted and
    // appended.
    EXPECT_NE(results.find(TfToken("3")), results.end());
    EXPECT_EQ(results[TfToken("3")], DiffResult::Reordered);

    DiffResult overall = computeOverallResult(results);

    // Same + Reordered: overall reordered.
    EXPECT_EQ(overall, DiffResult::Reordered);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareTokenListsDiff3)
{
    std::vector<TfToken> baselineList({ TfToken("1"), TfToken("2"), TfToken("3") });
    std::vector<TfToken> modifiedList({ TfToken("3"), TfToken("2"), TfToken("1") });

    std::map<TfToken, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find(TfToken("1")), results.end());
    EXPECT_EQ(results[TfToken("1")], DiffResult::Same);
    // Note: result for TfToken("2") differ because the modified list placed it at the start, but it
    // was later in the list and had to be deleted from there, so it must both be prepended and
    // deleted.
    EXPECT_NE(results.find(TfToken("2")), results.end());
    EXPECT_EQ(results[TfToken("2")], DiffResult::Reordered);
    // Note: result for TfToken("3") differ because the modified list placed it at the start, but it
    // was later in the list and had to be deleted from there, so it must both be prepended and
    // deleted.
    EXPECT_NE(results.find(TfToken("3")), results.end());
    EXPECT_EQ(results[TfToken("3")], DiffResult::Reordered);

    DiffResult overall = computeOverallResult(results);

    // Same + Reordered: overall reordered.
    EXPECT_EQ(overall, DiffResult::Reordered);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareTokenListsAbsent)
{
    std::vector<TfToken> baselineList({ TfToken("1"), TfToken("2"), TfToken("3") });
    std::vector<TfToken> modifiedList;

    std::map<TfToken, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find(TfToken("1")), results.end());
    EXPECT_EQ(results[TfToken("1")], DiffResult::Absent);
    EXPECT_NE(results.find(TfToken("2")), results.end());
    EXPECT_EQ(results[TfToken("2")], DiffResult::Absent);
    EXPECT_NE(results.find(TfToken("3")), results.end());
    EXPECT_EQ(results[TfToken("3")], DiffResult::Absent);

    DiffResult overall = computeOverallResult(results);

    EXPECT_EQ(overall, DiffResult::Absent);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffLists, compareTokenListsPrepended)
{
    std::vector<TfToken> baselineList;
    std::vector<TfToken> modifiedList({ TfToken("1"), TfToken("2"), TfToken("3") });

    std::map<TfToken, DiffResult> results = compareLists(modifiedList, baselineList);

    EXPECT_EQ(results.size(), std::size_t(3));
    EXPECT_NE(results.find(TfToken("1")), results.end());
    EXPECT_EQ(results[TfToken("1")], DiffResult::Prepended);
    EXPECT_NE(results.find(TfToken("2")), results.end());
    EXPECT_EQ(results[TfToken("2")], DiffResult::Prepended);
    EXPECT_NE(results.find(TfToken("3")), results.end());
    EXPECT_EQ(results[TfToken("3")], DiffResult::Prepended);

    DiffResult overall = computeOverallResult(results);

    // Note: while we set each item as prepended because they don't have an overall view
    // to decide which kind of results there will be, once we calculate the overall result
    // all-prepended is returned as created as that is the more general term, because
    // all-prepended is equivalent to all-appended.
    EXPECT_EQ(overall, DiffResult::Created);

    DiffResult quickDiff = DiffResult::Same;
    compareLists(modifiedList, baselineList, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}
