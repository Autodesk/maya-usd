#include <mayaUsdUtils/DiffPrims.h>

#include <pxr/base/tf/type.h>
#include <pxr/usd/sdf/valueTypeName.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffAttributes, compareAttributesEmpty)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesSameDefaultDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    baselineAttr.Set(1.0);
    modifiedAttr.Set(1.0);

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesDiffDefaultDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    baselineAttr.Set(1.0);
    modifiedAttr.Set(2.0);

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesAbsentDefaultDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    baselineAttr.Set(1.0);

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Absent);

    DiffResult quickDiff = DiffResult::Same;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesCreatedDefaultDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    modifiedAttr.Set(1.0);

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Created);

    DiffResult quickDiff = DiffResult::Same;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

//----------------------------------------------------------------------------------------------------------------------

TEST(DiffAttributes, compareAttributesSameSampledDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    for (double time = 0.; time < 10.1; time += 1.0) {
        baselineAttr.Set(1.0 * time, UsdTimeCode(time));
        modifiedAttr.Set(1.0 * time, UsdTimeCode(time));
    }

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesSameSampledDoubleAndFloat)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;
    auto    floatType = SdfValueTypeNames->Float;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), floatType, true);

    for (double time = 0.; time < 10.1; time += 1.0) {
        baselineAttr.Set(1.0 * time, UsdTimeCode(time));
        modifiedAttr.Set(float(1.0 * time), UsdTimeCode(time));
    }

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesSameInterpolatedSampledDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    for (double time = 0.; time < 10.1; time += 1.0) {
        baselineAttr.Set(1.0 * time, UsdTimeCode(time));
    }

    for (double time = 0.; time < 10.1; time += 2.0) {
        modifiedAttr.Set(1.0 * time, UsdTimeCode(time));
    }

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesSameInterpolatedSampledFloatAndDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;
    auto    floatType = SdfValueTypeNames->Float;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), floatType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    for (double time = 0.; time < 10.1; time += 1.0) {
        baselineAttr.Set(float(1.0 * time), UsdTimeCode(time));
    }

    for (double time = 0.; time < 10.1; time += 2.0) {
        modifiedAttr.Set(1.0 * time, UsdTimeCode(time));
    }

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesSameDefaultAndSampledDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    // All baseline values are the same and modified simply uses a default.
    for (double time = 0.; time < 10.1; time += 1.0) {
        baselineAttr.Set(1.0, UsdTimeCode(time));
    }

    modifiedAttr.Set(1.0, UsdTimeCode::Default());

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Same);

    DiffResult quickDiff = DiffResult::Differ;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_EQ(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesDiffSampledDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    for (double time = 0.; time < 10.1; time += 1.0) {
        baselineAttr.Set(1.0 * time, UsdTimeCode(time));
        modifiedAttr.Set(2.0 * time, UsdTimeCode(time));
    }

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesSingleModDiffSampledDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    // All values are the same except a single one in the modified attributes.
    for (double time = 0.; time < 10.1; time += 1.0) {
        baselineAttr.Set(1.0 * time, UsdTimeCode(time));
        modifiedAttr.Set(1.0 * time, UsdTimeCode(time));
    }

    modifiedAttr.Set(5555.0, UsdTimeCode(5.));

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesSingleBaseDiffSampledDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    // All values are the same except a single one in the baseline attributes.
    for (double time = 0.; time < 10.1; time += 1.0) {
        baselineAttr.Set(1.0 * time, UsdTimeCode(time));
        modifiedAttr.Set(1.0 * time, UsdTimeCode(time));
    }

    baselineAttr.Set(5555.0, UsdTimeCode(7.));

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Differ);

    DiffResult quickDiff = DiffResult::Same;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesAbsentSampledDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    for (double time = 0.; time < 10.1; time += 1.0) {
        baselineAttr.Set(1.0 * time, UsdTimeCode(time));
    }

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Absent);

    DiffResult quickDiff = DiffResult::Same;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

TEST(DiffAttributes, compareAttributesCreatedSampledDouble)
{
    SdfPath primPath("/A");
    auto    doubleType = SdfValueTypeNames->Double;

    auto baselineStage = UsdStage::CreateInMemory();
    auto baselinePrim = baselineStage->DefinePrim(SdfPath(primPath));
    auto baselineAttr = baselinePrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    auto modifiedStage = UsdStage::CreateInMemory();
    auto modifiedPrim = modifiedStage->DefinePrim(SdfPath(primPath));
    auto modifiedAttr = modifiedPrim.CreateAttribute(TfToken("test_attr"), doubleType, true);

    for (double time = 0.; time < 10.1; time += 1.0) {
        modifiedAttr.Set(1.0 * time, UsdTimeCode(time));
    }

    DiffResult result = compareAttributes(modifiedAttr, baselineAttr);

    EXPECT_EQ(result, DiffResult::Created);

    DiffResult quickDiff = DiffResult::Same;
    compareAttributes(modifiedAttr, baselineAttr, &quickDiff);
    EXPECT_NE(quickDiff, DiffResult::Same);
}

