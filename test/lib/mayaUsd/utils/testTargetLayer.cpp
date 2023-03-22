#include <mayaUsd/utils/targetLayer.h>

#include <pxr/usd/sdf/layer.h>

#include <gtest/gtest.h>

using namespace PXR_NS;
using namespace MAYAUSD_NS_DEF;

TEST(ConvertTargetLayer, convertDefaultTargetLayer)
{
    auto originalStage = UsdStage::CreateInMemory();
    auto originalTargetLayer = originalStage->GetEditTarget();

    const MString text = convertTargetLayerToText(*originalStage);
    setTargetLayerFromText(*originalStage, text);

    auto restoredTargetLayer = originalStage->GetEditTarget();
    EXPECT_EQ(originalTargetLayer, restoredTargetLayer);
}

TEST(ConvertTargetLayer, convertSubLayerTargetLayer)
{
    auto originalStage = UsdStage::CreateInMemory();
    auto subLayer = SdfLayer::CreateAnonymous();
    originalStage->GetRootLayer()->InsertSubLayerPath(subLayer->GetIdentifier());
    originalStage->SetEditTarget(subLayer);

    auto originalTargetLayer = originalStage->GetEditTarget();

    const MString text = convertTargetLayerToText(*originalStage);
    setTargetLayerFromText(*originalStage, text);

    auto restoredTargetLayer = originalStage->GetEditTarget();
    EXPECT_EQ(originalTargetLayer, restoredTargetLayer);
}
