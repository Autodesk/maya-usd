#include <gtest/gtest.h>

#include <iostream>
#include <functional>
#include <unistd.h>

#include <AL/usd/schemas/ModelAPI.h>
#include <AL/usd/schemas/MayaReference.h>

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"


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
  UsdPrim p = stageOut->GetPrimAtPath(expectedUnselectableParent);
  AL_usd_ModelAPI unselectableParent = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedUnselectableParent));
  unselectableParent.SetSelectability(AL_USDMayaSchemasTokens->selectability_unselectable);
  v = unselectableParent.ComputeSelectabilty();

  ASSERT_TRUE(v == AL_USDMayaSchemasTokens->selectability_unselectable);

  AL_usd_ModelAPI unselectableChild = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedUnselectableChild));
  v = unselectableChild.ComputeSelectabilty();
  ASSERT_TRUE(v == AL_USDMayaSchemasTokens->selectability_unselectable);

  // Check if the selectable part of the hierarchy is computed correctly
  AL_usd_ModelAPI selectableParent = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedSelectableParent));
  v = selectableParent.ComputeSelectabilty();

  ASSERT_TRUE(v != AL_USDMayaSchemasTokens->selectability_unselectable);

  AL_usd_ModelAPI selectableChild = AL_usd_ModelAPI(stageOut->GetPrimAtPath(expectedSelectableChild));
  v = selectableChild.ComputeSelectabilty();
  ASSERT_TRUE(v != AL_USDMayaSchemasTokens->selectability_unselectable);
}


