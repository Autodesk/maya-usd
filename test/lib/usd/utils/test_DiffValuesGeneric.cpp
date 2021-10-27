#include <mayaUsdUtils/DiffPrims.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesGeneric, boolCompareValuesSame)
{
    VtValue    baselineValue(bool(true));
    VtValue    modifiedValue(bool(true));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesGeneric, boolCompareValuesDiff)
{
    VtValue    baselineValue(bool(true));
    VtValue    modifiedValue(bool(false));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesGeneric, boolCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<bool>({ bool(true), bool(false), bool(false) }));
    VtValue    modifiedValue(VtArray<bool>({ bool(true), bool(false), bool(false) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesGeneric, boolCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<bool>({ bool(true), bool(false), bool(false) }));
    VtValue    modifiedValue(VtArray<bool>({ bool(true), bool(true), bool(false) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesGeneric, sdfTimeCodeCompareValuesSame)
{
    VtValue    baselineValue(SdfTimeCode(1));
    VtValue    modifiedValue(SdfTimeCode(1));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesGeneric, sdfTimeCodeCompareValuesDiff)
{
    VtValue    baselineValue(SdfTimeCode(1));
    VtValue    modifiedValue(SdfTimeCode(2));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesGeneric, sdfTimeCodeCompareValueArraysSame)
{
    VtValue baselineValue(VtArray<SdfTimeCode>({ SdfTimeCode(1), SdfTimeCode(2), SdfTimeCode(3) }));
    VtValue modifiedValue(VtArray<SdfTimeCode>({ SdfTimeCode(1), SdfTimeCode(2), SdfTimeCode(3) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesGeneric, sdfTimeCodeCompareValueArraysDiff)
{
    VtValue baselineValue(VtArray<SdfTimeCode>({ SdfTimeCode(1), SdfTimeCode(2), SdfTimeCode(3) }));
    VtValue modifiedValue(VtArray<SdfTimeCode>({ SdfTimeCode(1), SdfTimeCode(4), SdfTimeCode(5) }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesGeneric, stringCompareValuesSame)
{
    VtValue    baselineValue(std::string("aaa"));
    VtValue    modifiedValue(std::string("aaa"));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesGeneric, stringCompareValuesDiff)
{
    VtValue    baselineValue(std::string("aaa"));
    VtValue    modifiedValue(std::string("bbb"));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesGeneric, stringCompareValueArraysSame)
{
    VtValue baselineValue(
        VtArray<std::string>({ std::string("aaa"), std::string("bbb"), std::string("ccc") }));
    VtValue modifiedValue(
        VtArray<std::string>({ std::string("aaa"), std::string("bbb"), std::string("ccc") }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesGeneric, stringCompareValueArraysDiff)
{
    VtValue baselineValue(
        VtArray<std::string>({ std::string("aaa"), std::string("bbb"), std::string("ccc") }));
    VtValue modifiedValue(
        VtArray<std::string>({ std::string("aaa"), std::string("ccc"), std::string("ccc") }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesGeneric, tfTokenCompareValuesSame)
{
    VtValue    baselineValue(TfToken("aaa"));
    VtValue    modifiedValue(TfToken("aaa"));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesGeneric, tfTokenCompareValuesDiff)
{
    VtValue    baselineValue(TfToken("aaa"));
    VtValue    modifiedValue(TfToken("bbb"));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesGeneric, tfTokenCompareValueArraysSame)
{
    VtValue    baselineValue(VtArray<TfToken>({ TfToken("aaa"), TfToken("bbb"), TfToken("ccc") }));
    VtValue    modifiedValue(VtArray<TfToken>({ TfToken("aaa"), TfToken("bbb"), TfToken("ccc") }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesGeneric, tfTokenCompareValueArraysDiff)
{
    VtValue    baselineValue(VtArray<TfToken>({ TfToken("aaa"), TfToken("bbb"), TfToken("ccc") }));
    VtValue    modifiedValue(VtArray<TfToken>({ TfToken("aaa"), TfToken("ccc"), TfToken("ccc") }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValuesGeneric, sdfAssetPathCompareValuesSame)
{
    VtValue    baselineValue(SdfAssetPath("aaa"));
    VtValue    modifiedValue(SdfAssetPath("aaa"));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesGeneric, sdfAssetPathCompareValuesDiff)
{
    VtValue    baselineValue(SdfAssetPath("aaa"));
    VtValue    modifiedValue(SdfAssetPath("bbb"));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}

TEST(DiffValuesGeneric, sdfAssetPathCompareValueArraysSame)
{
    VtValue baselineValue(
        VtArray<SdfAssetPath>({ SdfAssetPath("aaa"), SdfAssetPath("bbb"), SdfAssetPath("ccc") }));
    VtValue modifiedValue(
        VtArray<SdfAssetPath>({ SdfAssetPath("aaa"), SdfAssetPath("bbb"), SdfAssetPath("ccc") }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValuesGeneric, sdfAssetPathCompareValueArraysDiff)
{
    VtValue baselineValue(
        VtArray<SdfAssetPath>({ SdfAssetPath("aaa"), SdfAssetPath("bbb"), SdfAssetPath("ccc") }));
    VtValue modifiedValue(
        VtArray<SdfAssetPath>({ SdfAssetPath("aaa"), SdfAssetPath("ccc"), SdfAssetPath("ccc") }));
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}
