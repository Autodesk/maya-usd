#include "AL/usd/transaction/Notice.h"
#include "AL/usd/transaction/Transaction.h"
#include "AL/usd/transaction/TransactionManager.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

#include <gtest/gtest.h>

using namespace AL::usd::transaction;
PXR_NAMESPACE_USING_DIRECTIVE

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the AL_USDTransaction
//----------------------------------------------------------------------------------------------------------------------

// The fixture for testing transaction manager.
class TransactionManagerTest
    : public TfWeakBase
    , public ::testing::Test
{
public:
    TransactionManagerTest() { }
    ~TransactionManagerTest() override { }
};

/// Test that TransactionManager works with deleted stage as expected
TEST_F(TransactionManagerTest, StageLifetime)
{
    UsdStageWeakPtr stagePtr;
    auto            layer = SdfLayer::CreateAnonymous();
    {
        auto stage = UsdStage::CreateInMemory();
        stagePtr = stage;
        EXPECT_TRUE(stagePtr);
        EXPECT_TRUE(TransactionManager::Open(stagePtr, layer));
    }
    EXPECT_FALSE(stagePtr);
    EXPECT_FALSE(TransactionManager::Open(stagePtr, layer));
}

/// Test that TransactionManager works with deleted layer as expected
TEST_F(TransactionManagerTest, LayerLifetime)
{
    auto           stage = UsdStage::CreateInMemory();
    SdfLayerHandle layerPtr;
    {
        auto layer = SdfLayer::CreateAnonymous();
        layerPtr = layer;
        EXPECT_TRUE(layerPtr);
        EXPECT_TRUE(TransactionManager::Open(stage, layerPtr));
    }
    EXPECT_FALSE(layerPtr);
    EXPECT_FALSE(TransactionManager::Open(stage, layerPtr));
}

/// Test that TransactionManager reports transactions for multiple stages as expected
TEST_F(TransactionManagerTest, InProgress_Stage)
{
    auto stageA = UsdStage::CreateInMemory();
    auto stageB = UsdStage::CreateInMemory();
    auto layer = SdfLayer::CreateAnonymous();

    EXPECT_FALSE(TransactionManager::InProgress(stageA));
    EXPECT_FALSE(TransactionManager::InProgress(stageB));

    EXPECT_TRUE(TransactionManager::Open(stageA, layer));

    EXPECT_TRUE(TransactionManager::InProgress(stageA));
    EXPECT_FALSE(TransactionManager::InProgress(stageB));

    EXPECT_TRUE(TransactionManager::Open(stageB, layer));

    EXPECT_TRUE(TransactionManager::InProgress(stageA));
    EXPECT_TRUE(TransactionManager::InProgress(stageB));

    EXPECT_TRUE(TransactionManager::Close(stageA, layer));

    EXPECT_FALSE(TransactionManager::InProgress(stageA));
    EXPECT_TRUE(TransactionManager::InProgress(stageB));

    EXPECT_TRUE(TransactionManager::Close(stageB, layer));

    EXPECT_FALSE(TransactionManager::InProgress(stageA));
    EXPECT_FALSE(TransactionManager::InProgress(stageB));
}

/// Test that TransactionManager reports transactions for multiple layers as expected
TEST_F(TransactionManagerTest, InProgress_Layer)
{
    auto stage = UsdStage::CreateInMemory();
    auto layerA = SdfLayer::CreateAnonymous();
    auto layerB = SdfLayer::CreateAnonymous();

    EXPECT_FALSE(TransactionManager::InProgress(stage));
    EXPECT_FALSE(TransactionManager::InProgress(stage, layerA));
    EXPECT_FALSE(TransactionManager::InProgress(stage, layerB));

    EXPECT_TRUE(TransactionManager::Open(stage, layerA));

    EXPECT_TRUE(TransactionManager::InProgress(stage));
    EXPECT_TRUE(TransactionManager::InProgress(stage, layerA));
    EXPECT_FALSE(TransactionManager::InProgress(stage, layerB));

    EXPECT_TRUE(TransactionManager::Open(stage, layerB));

    EXPECT_TRUE(TransactionManager::InProgress(stage));
    EXPECT_TRUE(TransactionManager::InProgress(stage, layerA));
    EXPECT_TRUE(TransactionManager::InProgress(stage, layerB));

    EXPECT_TRUE(TransactionManager::Close(stage, layerA));

    EXPECT_TRUE(TransactionManager::InProgress(stage));
    EXPECT_FALSE(TransactionManager::InProgress(stage, layerA));
    EXPECT_TRUE(TransactionManager::InProgress(stage, layerB));

    EXPECT_TRUE(TransactionManager::Close(stage, layerB));

    EXPECT_FALSE(TransactionManager::InProgress(stage));
    EXPECT_FALSE(TransactionManager::InProgress(stage, layerA));
    EXPECT_FALSE(TransactionManager::InProgress(stage, layerB));
}
