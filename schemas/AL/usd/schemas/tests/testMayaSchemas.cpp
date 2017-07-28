#include <gtest/gtest.h>

#include <iostream>
#include <functional>
#include <unistd.h>

#include <AL/usd/schemas/MayaReference.h>

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"

PXR_NAMESPACE_USING_DIRECTIVE

/// \brief  Validates the schemas plugin can be loaded and used.

TEST(testMayaSchemas, testMayaReferenceAttributes)
{
  SdfPath       primPath("/TestRoundTrip");
  SdfAssetPath  mayaRefPath("/somewherenice/path.ma");
  std::string   mayaNamespace("nsp");

  auto stageOut = UsdStage::CreateInMemory();
  auto mayaRefPrimOut = AL_usd_MayaReference::Define(stageOut, primPath);
  auto primOut = mayaRefPrimOut.GetPrim();
  primOut.CreateAttribute(
      TfToken("mayaReference"),
      SdfValueTypeNames->Asset).Set(mayaRefPath);
  primOut.CreateAttribute(
      TfToken("mayaNamespace"),
      SdfValueTypeNames->String).Set(mayaNamespace);
  ASSERT_TRUE(primOut.GetAttributes().size() == 2);

  auto stageIn = UsdStage::Open(stageOut->Flatten());
  auto primIn = stageIn->GetPrimAtPath(primPath);
  ASSERT_TRUE(primIn.IsValid());

  AL_usd_MayaReference mayaRefPrimIn(primIn);
  std::string mayaNamespaceIn;
  mayaRefPrimIn.GetMayaNamespaceAttr().Get(&mayaNamespaceIn);
  ASSERT_TRUE(mayaNamespaceIn == mayaNamespace);

  SdfAssetPath mayaRefPathIn;
  mayaRefPrimIn.GetMayaReferenceAttr().Get(&mayaRefPathIn);
  ASSERT_TRUE(mayaRefPathIn.GetAssetPath() == mayaRefPath.GetAssetPath());
}
