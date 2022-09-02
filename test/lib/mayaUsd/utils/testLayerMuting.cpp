#include <mayaUsd/utils/layerMuting.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MAYAUSD_NS_DEF;

TEST(ConvertLayerMuting, convertEmptyStageLayerMuting)
{
    auto originalStage = UsdStage::CreateInMemory();

    const MString text = convertLayerMutingToText(*originalStage);

    auto convertedStage = UsdStage::CreateInMemory();
    setLayerMutingFromText(*convertedStage, text);

    EXPECT_EQ(originalStage->GetMutedLayers(), convertedStage->GetMutedLayers());
}

TEST(ConvertLayerMuting, convertSimpleStageLayerMuting)
{
    auto originalStage = UsdStage::CreateInMemory();
    originalStage->MuteLayer("a.b>c|#@d");
    originalStage->MuteLayer("d/e/f&g*h");
    originalStage->MuteLayer("g:h?i'\"");

    const MString text = convertLayerMutingToText(*originalStage);

    auto convertedStage = UsdStage::CreateInMemory();
    setLayerMutingFromText(*convertedStage, text);

    EXPECT_EQ(originalStage->GetMutedLayers(), convertedStage->GetMutedLayers());
}
