#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

#include <gtest/gtest.h>
#include <mayaUsd_Schemas/MayaReference.h>

#include <functional>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE
// TF_DECLARE_WEAK_PTRS(PlugPlugin);

/// Validates the schemas plugin has been registered
TEST(testMayaSchemas, verifyPlugin)
{
    PlugPluginPtr plug = PlugRegistry::GetInstance().GetPluginWithName("AL_USDMayaSchemas");
    EXPECT_TRUE(!plug.IsInvalid());
}

TEST(testMayaSchemas, testMayaReferenceAttributes)
{
    SdfPath      primPath("/TestRoundTrip");
    SdfAssetPath mayaRefPath("/somewherenice/path.ma");
    std::string  mayaNamespace("nsp");

    auto stageOut = UsdStage::CreateInMemory();
    auto mayaRefPrimOut = MayaUsd_SchemasMayaReference::Define(stageOut, primPath);
    auto primOut = mayaRefPrimOut.GetPrim();
    primOut.CreateAttribute(TfToken("mayaReference"), SdfValueTypeNames->Asset).Set(mayaRefPath);
    primOut.CreateAttribute(TfToken("mayaNamespace"), SdfValueTypeNames->String).Set(mayaNamespace);

    auto stageIn = UsdStage::Open(stageOut->Flatten());
    auto primIn = stageIn->GetPrimAtPath(primPath);
    ASSERT_TRUE(primIn.IsValid());

    MayaUsd_SchemasMayaReference mayaRefPrimIn(primIn);
    std::string                  mayaNamespaceIn;
    mayaRefPrimIn.GetMayaNamespaceAttr().Get(&mayaNamespaceIn);
    ASSERT_TRUE(mayaNamespaceIn == mayaNamespace);

    SdfAssetPath mayaRefPathIn;
    mayaRefPrimIn.GetMayaReferenceAttr().Get(&mayaRefPathIn);
    ASSERT_TRUE(mayaRefPathIn.GetAssetPath() == mayaRefPath.GetAssetPath());
}
