#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/valueTypeName.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

//----------------------------------------------------------------------------------------------------------------------
// Single-item tests.

TEST(DiffDictionaries, compareDictionariesEmpty)
{
    VtDictionary baselineDict;
    VtDictionary modifiedDict;

    DiffResultPerKey results = compareDictionaries(modifiedDict, baselineDict);

    EXPECT_TRUE(results.empty());

    DiffResult quickDiff = DiffResult::Differ;
    compareDictionaries(modifiedDict, baselineDict, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffDictionaries, compareDictionariesSameDouble)
{
    const std::string keyName = "A";

    VtDictionary baselineDict;
    VtDictionary modifiedDict;

    baselineDict[keyName] = 1.0;
    modifiedDict[keyName] = 1.0;

    DiffResultPerKey results = compareDictionaries(modifiedDict, baselineDict);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(keyName), results.end());

    DiffResult result = results[keyName];
    EXPECT_EQ(result, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareDictionaries(modifiedDict, baselineDict, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffDictionaries, compareDictionariesDiffDouble)
{
    const std::string keyName = "A";

    VtDictionary baselineDict;
    VtDictionary modifiedDict;

    baselineDict[keyName] = 1.0;
    modifiedDict[keyName] = 2.0;

    DiffResultPerKey results = compareDictionaries(modifiedDict, baselineDict);
    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(keyName), results.end());

    DiffResult result = results[keyName];
    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    compareDictionaries(modifiedDict, baselineDict, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffDictionaries, compareDictionariesAbsentDouble)
{
    const std::string keyName = "A";

    VtDictionary baselineDict;
    VtDictionary modifiedDict;

    baselineDict[keyName] = 1.0;

    DiffResultPerKey results = compareDictionaries(modifiedDict, baselineDict);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(keyName), results.end());

    DiffResult result = results[keyName];
    EXPECT_EQ(result, DiffResult::Absent);

    DiffResult quickDiff = DiffResult::Same;
    compareDictionaries(modifiedDict, baselineDict, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffDictionaries, compareDictionariesCreatedDouble)
{
    const std::string keyName = "A";

    VtDictionary baselineDict;
    VtDictionary modifiedDict;

    modifiedDict[keyName] = 2.0;

    DiffResultPerKey results = compareDictionaries(modifiedDict, baselineDict);

    EXPECT_EQ(results.size(), std::size_t(1));
    EXPECT_NE(results.find(keyName), results.end());

    DiffResult result = results[keyName];
    EXPECT_EQ(result, DiffResult::Created);

    DiffResult quickDiff = DiffResult::Same;
    compareDictionaries(modifiedDict, baselineDict, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

//----------------------------------------------------------------------------------------------------------------------
// Multi-items tests.

TEST(DiffDictionaries, compareDictionariesSubset)
{
    const std::string keyName1 = "A";
    const std::string keyName2 = "B";
    const std::string keyName3 = "C";

    VtDictionary baselineDict;
    VtDictionary modifiedDict;

    baselineDict[keyName1] = 1.0;
    baselineDict[keyName2] = 2.0;
    baselineDict[keyName3] = 3.0;

    modifiedDict[keyName1] = 1.0;

    DiffResultPerKey results = compareDictionaries(modifiedDict, baselineDict);

    EXPECT_EQ(results.size(), std::size_t(3));

    EXPECT_NE(results.find(keyName1), results.end());
    EXPECT_EQ(results[keyName1], DiffResult::Same);
    EXPECT_NE(results.find(keyName2), results.end());
    EXPECT_EQ(results[keyName2], DiffResult::Absent);
    EXPECT_NE(results.find(keyName3), results.end());
    EXPECT_EQ(results[keyName3], DiffResult::Absent);

    DiffResult overall = computeOverallResult(results);
    EXPECT_EQ(overall, DiffResult::Subset);

    DiffResult quickDiff = DiffResult::Same;
    compareDictionaries(modifiedDict, baselineDict, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffDictionaries, compareDictionariesSuperset)
{
    const std::string keyName1 = "A";
    const std::string keyName2 = "B";
    const std::string keyName3 = "C";

    VtDictionary baselineDict;
    VtDictionary modifiedDict;

    baselineDict[keyName2] = 2.0;

    modifiedDict[keyName1] = 1.0;
    modifiedDict[keyName2] = 2.0;
    modifiedDict[keyName3] = 3.0;

    DiffResultPerKey results = compareDictionaries(modifiedDict, baselineDict);

    EXPECT_EQ(results.size(), std::size_t(3));

    EXPECT_NE(results.find(keyName1), results.end());
    EXPECT_EQ(results[keyName1], DiffResult::Created);
    EXPECT_NE(results.find(keyName2), results.end());
    EXPECT_EQ(results[keyName2], DiffResult::Same);
    EXPECT_NE(results.find(keyName3), results.end());
    EXPECT_EQ(results[keyName3], DiffResult::Created);

    DiffResult overall = computeOverallResult(results);
    EXPECT_EQ(overall, DiffResult::Superset);

    DiffResult quickDiff = DiffResult::Same;
    compareDictionaries(modifiedDict, baselineDict, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffDictionaries, compareDictionariesCreatedAbsent)
{
    const std::string keyName1 = "A";
    const std::string keyName2 = "B";
    const std::string keyName3 = "C";

    VtDictionary baselineDict;
    VtDictionary modifiedDict;

    baselineDict[keyName1] = 1.0;
    baselineDict[keyName2] = 2.0;

    modifiedDict[keyName2] = 2.0;
    modifiedDict[keyName3] = 3.0;

    DiffResultPerKey results = compareDictionaries(modifiedDict, baselineDict);

    EXPECT_EQ(results.size(), std::size_t(3));

    EXPECT_NE(results.find(keyName1), results.end());
    EXPECT_EQ(results[keyName1], DiffResult::Absent);
    EXPECT_NE(results.find(keyName2), results.end());
    EXPECT_EQ(results[keyName2], DiffResult::Same);
    EXPECT_NE(results.find(keyName3), results.end());
    EXPECT_EQ(results[keyName3], DiffResult::Created);

    DiffResult overall = computeOverallResult(results);
    EXPECT_EQ(overall, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    compareDictionaries(modifiedDict, baselineDict, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffDictionaries, compareDictionariesOverallCreated)
{
    const std::string keyName1 = "A";
    const std::string keyName2 = "B";
    const std::string keyName3 = "C";

    VtDictionary baselineDict;
    VtDictionary modifiedDict;

    modifiedDict[keyName1] = 1.0;
    modifiedDict[keyName2] = 2.0;
    modifiedDict[keyName3] = 3.0;

    DiffResultPerKey results = compareDictionaries(modifiedDict, baselineDict);

    EXPECT_EQ(results.size(), std::size_t(3));

    EXPECT_NE(results.find(keyName1), results.end());
    EXPECT_EQ(results[keyName1], DiffResult::Created);
    EXPECT_NE(results.find(keyName2), results.end());
    EXPECT_EQ(results[keyName2], DiffResult::Created);
    EXPECT_NE(results.find(keyName3), results.end());
    EXPECT_EQ(results[keyName3], DiffResult::Created);

    DiffResult overall = computeOverallResult(results);
    EXPECT_EQ(overall, DiffResult::Created);

    DiffResult quickDiff = DiffResult::Same;
    compareDictionaries(modifiedDict, baselineDict, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffDictionaries, compareDictionariesOverallAbsent)
{
    const std::string keyName1 = "A";
    const std::string keyName2 = "B";
    const std::string keyName3 = "C";

    VtDictionary baselineDict;
    VtDictionary modifiedDict;

    baselineDict[keyName1] = 1.0;
    baselineDict[keyName2] = 2.0;
    baselineDict[keyName3] = 3.0;

    DiffResultPerKey results = compareDictionaries(modifiedDict, baselineDict);

    EXPECT_EQ(results.size(), std::size_t(3));

    EXPECT_NE(results.find(keyName1), results.end());
    EXPECT_EQ(results[keyName1], DiffResult::Absent);
    EXPECT_NE(results.find(keyName2), results.end());
    EXPECT_EQ(results[keyName2], DiffResult::Absent);
    EXPECT_NE(results.find(keyName3), results.end());
    EXPECT_EQ(results[keyName3], DiffResult::Absent);

    DiffResult overall = computeOverallResult(results);
    EXPECT_EQ(overall, DiffResult::Absent);

    DiffResult quickDiff = DiffResult::Same;
    compareDictionaries(modifiedDict, baselineDict, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}
