#include <AL/usd/schemas/maya/ModelAPI.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

#include <gtest/gtest.h>
#include <mayaUsd_Schemas/MayaReference.h>

#include <functional>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

/// \brief  Validate that the ModelAPI is working correctly

/// \brief  Test that the compute selectability is computing the correct value
TEST(testModelAPI, testComputeSelectability)
{
    SdfPath expectedUnselectableParent("/A/B");
    SdfPath expectedUnselectableChild("/A/B/C");

    SdfPath expectedSelectableParent("/A/D");
    SdfPath expectedSelectableChild("/A/D/E");

    auto stageOut = UsdStage::CreateInMemory();
    stageOut->DefinePrim(SdfPath(expectedUnselectableChild));
    stageOut->DefinePrim(SdfPath(expectedSelectableChild));

    TfToken v;
    // Check if the unselectable part of the hierarchy is computed correctly
    UsdPrim         p = stageOut->GetPrimAtPath(expectedUnselectableParent);
    AL_usd_ModelAPI unselectableParent
        = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedUnselectableParent));
    unselectableParent.SetSelectability(AL_USDMayaSchemasTokens->selectability_unselectable);
    v = unselectableParent.ComputeSelectabilty();

    ASSERT_TRUE(v == AL_USDMayaSchemasTokens->selectability_unselectable);

    AL_usd_ModelAPI unselectableChild
        = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedUnselectableChild));
    v = unselectableChild.ComputeSelectabilty();
    ASSERT_TRUE(v == AL_USDMayaSchemasTokens->selectability_unselectable);

    // Check if the selectable part of the hierarchy is computed correctly
    AL_usd_ModelAPI selectableParent
        = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedSelectableParent));
    v = selectableParent.ComputeSelectabilty();

    ASSERT_TRUE(v != AL_USDMayaSchemasTokens->selectability_unselectable);

    AL_usd_ModelAPI selectableChild
        = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedSelectableChild));
    v = selectableChild.ComputeSelectabilty();
    ASSERT_TRUE(v != AL_USDMayaSchemasTokens->selectability_unselectable);
}

/// \brief  Test that the compute lock is computing the correct value
TEST(testModelAPI, testComputeLock)
{
    SdfPath expectedLocked("/A");
    SdfPath expectedInheritedLocked("/A/B");
    SdfPath expectedUnlocked("/A/B/C");
    SdfPath expectedInheritedUnlocked("/A/B/C/D");
    SdfPath expectedInherited("/E/F");

    auto stageOut = UsdStage::CreateInMemory();
    stageOut->DefinePrim(SdfPath(expectedInheritedUnlocked));
    stageOut->DefinePrim(SdfPath(expectedInherited));

    TfToken         v;
    AL_usd_ModelAPI lockedModel = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedLocked));
    lockedModel.SetLock(AL_USDMayaSchemasTokens->lock_transform);
    v = lockedModel.ComputeLock();
    ASSERT_TRUE(v == AL_USDMayaSchemasTokens->lock_transform);

    // Check if a child of locked prim inherits lock by default
    AL_usd_ModelAPI inheritedLockedModel
        = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedInheritedLocked));
    v = inheritedLockedModel.ComputeLock();
    ASSERT_TRUE(v == AL_USDMayaSchemasTokens->lock_transform);

    // Check if the unlocked state in the hierarchy computed correctly
    AL_usd_ModelAPI unlockedModel = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedUnlocked));
    lockedModel.SetLock(AL_USDMayaSchemasTokens->lock_unlocked);
    v = unlockedModel.ComputeLock();
    ASSERT_TRUE(v == AL_USDMayaSchemasTokens->lock_unlocked);

    // Check if a child of unlocked prim inherits unlocked by default
    AL_usd_ModelAPI inheritedUnlockedModel
        = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedInheritedUnlocked));
    v = inheritedUnlockedModel.ComputeLock();
    ASSERT_TRUE(v == AL_USDMayaSchemasTokens->lock_unlocked);

    // Check if lock along prim hierarchy is computed to "inherited" by default
    AL_usd_ModelAPI inheritedModel = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedInherited));
    v = inheritedModel.ComputeLock();
    ASSERT_TRUE(v == AL_USDMayaSchemasTokens->lock_inherited);
}
