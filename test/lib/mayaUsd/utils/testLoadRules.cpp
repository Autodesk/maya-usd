#include <mayaUsd/utils/loadRules.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MAYAUSD_NS_DEF;

TEST(ConvertLoadRules, convertEmptyLoadRules)
{
    UsdStageLoadRules originalLoadRules;

    const MString text = convertLoadRulesToText(originalLoadRules);

    UsdStageLoadRules convertedLoadRules = createLoadRulesFromText(text);

    EXPECT_EQ(originalLoadRules, convertedLoadRules);
}

TEST(ConvertLoadRules, convertSimpleLoadRules)
{
    UsdStageLoadRules originalLoadRules;
    originalLoadRules.AddRule(SdfPath("/a/b/c"), UsdStageLoadRules::AllRule);
    originalLoadRules.AddRule(SdfPath("/a/b"), UsdStageLoadRules::NoneRule);
    originalLoadRules.AddRule(SdfPath("/d"), UsdStageLoadRules::OnlyRule);

    const MString text = convertLoadRulesToText(originalLoadRules);

    UsdStageLoadRules convertedLoadRules = createLoadRulesFromText(text);

    EXPECT_EQ(originalLoadRules, convertedLoadRules);
}

TEST(ConvertLoadRules, convertEmptyStageLoadRules)
{
    auto originalStage = UsdStage::CreateInMemory();

    const MString text = convertLoadRulesToText(*originalStage);

    auto convertedStage = UsdStage::CreateInMemory();
    setLoadRulesFromText(*convertedStage, text);

    EXPECT_EQ(originalStage->GetLoadRules(), convertedStage->GetLoadRules());
}

TEST(ConvertLoadRules, convertSimpleStageLoadRules)
{
    UsdStageLoadRules originalLoadRules;
    originalLoadRules.AddRule(SdfPath("/a/b/c"), UsdStageLoadRules::AllRule);
    originalLoadRules.AddRule(SdfPath("/a/b"), UsdStageLoadRules::NoneRule);
    originalLoadRules.AddRule(SdfPath("/d"), UsdStageLoadRules::OnlyRule);
    originalLoadRules.AddRule(SdfPath("/d/e"), UsdStageLoadRules::AllRule);
    auto originalStage = UsdStage::CreateInMemory();
    originalStage->SetLoadRules(originalLoadRules);

    const MString text = convertLoadRulesToText(*originalStage);

    auto convertedStage = UsdStage::CreateInMemory();
    setLoadRulesFromText(*convertedStage, text);

    EXPECT_EQ(originalStage->GetLoadRules(), convertedStage->GetLoadRules());
}
