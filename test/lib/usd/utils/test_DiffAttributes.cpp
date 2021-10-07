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
}

TEST(DiffAttributes, compareAttributesSameDouble)
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
}

TEST(DiffAttributes, compareAttributesDiffDouble)
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
}

TEST(DiffAttributes, compareAttributesAbsentDouble)
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
}

TEST(DiffAttributes, compareAttributesCreatedDouble)
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
}
