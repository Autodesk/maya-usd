#include <mayaUsd/utils/targetLayer.h>

#include <pxr/usd/sdf/layer.h>

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

TEST(ConvertTargetLayer, convertDefaultTargetLayer)
{
    auto originalStage = UsdStage::CreateInMemory();
    auto originalTargetLayer = originalStage->GetEditTarget();

    const MString text = MayaUsd::convertTargetLayerToText(*originalStage);
    MayaUsd::setTargetLayerFromText(*originalStage, text);

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

    const MString text = MayaUsd::convertTargetLayerToText(*originalStage);
    MayaUsd::setTargetLayerFromText(*originalStage, text);

    auto restoredTargetLayer = originalStage->GetEditTarget();
    EXPECT_EQ(originalTargetLayer, restoredTargetLayer);
}
