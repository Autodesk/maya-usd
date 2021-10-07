#include <mayaUsdUtils/DiffPrims.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MayaUsdUtils;

//----------------------------------------------------------------------------------------------------------------------
TEST(DiffValues, compareValuesEmpty)
{
    VtValue baselineValue;
    VtValue modifiedValue;

    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, compareValuesSameDouble)
{
    VtValue baselineValue;
    VtValue modifiedValue;

    baselineValue = 1.0;
    modifiedValue = 1.0;
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Same);
}

TEST(DiffValues, compareValuesDiffDouble)
{
    VtValue baselineValue;
    VtValue modifiedValue;

    baselineValue = 1.0;
    modifiedValue = 2.0;
    DiffResult result = compareValues(modifiedValue, baselineValue);

    EXPECT_EQ(result, DiffResult::Differ);
}
