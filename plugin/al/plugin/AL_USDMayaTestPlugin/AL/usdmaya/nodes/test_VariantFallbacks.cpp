//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "test_usdmaya.h"

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdUtils/stageCache.h>

#include <maya/MDagPath.h>
#include <maya/MFileIO.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>

namespace {
//! create a ProxyShape node from file path
AL::usdmaya::nodes::ProxyShape* createProxy(const MString& path)
{
    MString      importCommand = "AL_usdmaya_ProxyShapeImport -f \"" + path + "\"";
    MStringArray cmdResults;
    MGlobal::executeCommand(importCommand, cmdResults, true);
    MString proxyName = cmdResults[0];

    MSelectionList sel;
    sel.add(proxyName);
    MDagPath proxyDagPath;
    sel.getDagPath(0, proxyDagPath);
    MFnDagNode proxyMFn(proxyDagPath);
    return dynamic_cast<AL::usdmaya::nodes::ProxyShape*>(proxyMFn.userNode());
}

//! create a ProxyShape node without any file path
AL::usdmaya::nodes::ProxyShape* createProxy()
{
    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
    return dynamic_cast<AL::usdmaya::nodes::ProxyShape*>(fn.userNode());
}

//! create a ProxyShape node with specified name and stage cache id
AL::usdmaya::nodes::ProxyShape* createProxy(const MString& name, long stageCacheId)
{
    MString importCommand = MString("AL_usdmaya_ProxyShapeImport  ") + "-name \"" + name + "\" "
        + "-stageId " + stageCacheId;
    MStringArray cmdResults;
    MGlobal::executeCommand(importCommand, cmdResults, true);
    MString proxyName = cmdResults[0];

    MSelectionList sel;
    sel.add(proxyName);
    MDagPath proxyDagPath;
    sel.getDagPath(0, proxyDagPath);
    MFnDagNode proxyMFn(proxyDagPath);
    return dynamic_cast<AL::usdmaya::nodes::ProxyShape*>(proxyMFn.userNode());
}
} // namespace

// Test variant fallback config from global setting, the attribute
// ".variantFallbacks" should have no change (compatible with previous
// behaviour).
TEST(VariantFallbacks, fromGlobal)
{
    MString filePath = MString(AL_USDMAYA_TEST_DATA) + "/variant_fallbacks.usda";
    MFileIO::newFile(true);

    PXR_NS::PcpVariantFallbackMap defaultVariantFallbacks(
        PXR_NS::UsdStage::GetGlobalVariantFallbacks());
    // set global variant fallback to custom value
    PXR_NS::PcpVariantFallbackMap fallbacks;
    fallbacks["geo"] = { { "plane" } };
    PXR_NS::UsdStage::SetGlobalVariantFallbacks(fallbacks);

    auto            proxy = createProxy(filePath);
    PXR_NS::SdfPath primPath("/root/GEO/plane1/planeShape1");
    EXPECT_TRUE(proxy->getUsdStage()->GetPrimAtPath(primPath).IsValid());
    // the attribute should be empty because no custom variant provided
    EXPECT_TRUE(proxy->variantFallbacksPlug().asString().length() == 0);

    // restore to default
    PXR_NS::UsdStage::SetGlobalVariantFallbacks(defaultVariantFallbacks);
}

// Test variant fallback from ".variantFallbacks" attribute.
// This is to cover the case when user reopen a Maya scene file with a
// predefined variant fallback setting; global variant fallback should remain
// the same.
TEST(VariantFallbacks, fromAttribute)
{
    MString filePath = MString(AL_USDMAYA_TEST_DATA) + "/variant_fallbacks.usda";
    MFileIO::newFile(true);

    PXR_NS::PcpVariantFallbackMap defaultVariantFallbacks(
        PXR_NS::UsdStage::GetGlobalVariantFallbacks());

    auto proxy = createProxy();
    // Note: MString below is a pre-formatted JSON data
    MString variantAttrVal("{\n    \"geo\": [\"non_exist_variant\", \"cube\"]\n}");
    // set the variant fallback config
    proxy->variantFallbacksPlug().setString(variantAttrVal);
    // trigger stage loading, the "cube" variant should be picked
    proxy->filePathPlug().setString(filePath);

    // verify loaded prim
    PXR_NS::SdfPath primPath("/root/GEO/cube1/cubeShape1");
    EXPECT_TRUE(proxy->getUsdStage()->GetPrimAtPath(primPath).IsValid());
    // proxy node should have saved the config to ".variantFallbacks"
    EXPECT_EQ(proxy->variantFallbacksPlug().asString(), variantAttrVal);
    // default global variant should not be changed
    EXPECT_EQ(PXR_NS::UsdStage::GetGlobalVariantFallbacks(), defaultVariantFallbacks);
}

// Test variant fallback from ".variantFallbacks" attribute with invalid
// format.
TEST(VariantFallbacks, invalidAttributeFormat)
{
    MString filePath = MString(AL_USDMAYA_TEST_DATA) + "/variant_fallbacks.usda";
    MFileIO::newFile(true);

    PXR_NS::PcpVariantFallbackMap defaultVariantFallbacks(
        PXR_NS::UsdStage::GetGlobalVariantFallbacks());

    auto    proxy = createProxy();
    MString variantAttrVal("<invalid json format>");
    // set the variant fallback config
    proxy->variantFallbacksPlug().setString(variantAttrVal);
    // trigger stage loading, the "cube" variant should be picked
    proxy->filePathPlug().setString(filePath);
    // ".variantFallbacks" attribute should have no change
    EXPECT_EQ(proxy->variantFallbacksPlug().asString(), variantAttrVal);
    // default global variant should have no change
    EXPECT_EQ(PXR_NS::UsdStage::GetGlobalVariantFallbacks(), defaultVariantFallbacks);
}

// Test variant fallback from ".variantFallbacks" attribute with invalid
// type.
TEST(VariantFallbacks, incorrectVariantType)
{
    MString filePath = MString(AL_USDMAYA_TEST_DATA) + "/variant_fallbacks.usda";
    MFileIO::newFile(true);

    PXR_NS::PcpVariantFallbackMap defaultVariantFallbacks(
        PXR_NS::UsdStage::GetGlobalVariantFallbacks());

    auto proxy = createProxy();
    // Note: MString below is a pre-formatted JSON data
    MString variantAttrVal("{\n    \"geo\": \"incorrect type\"\n}");
    // set the variant fallback config
    proxy->variantFallbacksPlug().setString(variantAttrVal);
    // trigger stage loading, the "cube" variant should be picked
    proxy->filePathPlug().setString(filePath);
    // ".variantFallbacks" attribute should have no change
    EXPECT_EQ(proxy->variantFallbacksPlug().asString(), variantAttrVal);
    // default global variant should have no change
    EXPECT_EQ(PXR_NS::UsdStage::GetGlobalVariantFallbacks(), defaultVariantFallbacks);
}

// Test variant fallback from session layer.
// This is to cover the case where department workflow / etc. apps that had
// variant fallback overrides saved on session layer, the ".variantFallbacks"
// attribute should save that overrides value
TEST(VariantFallbacks, fromSessionLayer)
{
    std::string filePath = std::string(AL_USDMAYA_TEST_DATA) + "/variant_fallbacks.usda";
    MFileIO::newFile(true);

    PXR_NS::PcpVariantFallbackMap defaultVariantFallbacks(
        PXR_NS::UsdStage::GetGlobalVariantFallbacks());
    PXR_NS::UsdStageCache::Id stageCacheId;
    {
        // simulate the workflow that Forge2 would do, which set the variants
        // before loading then restore to default
        // set variant fallback globally
        PXR_NS::PcpVariantFallbackMap fallbacks;
        fallbacks["geo"] = { "non_exist_variant", "sphere" };
        PXR_NS::UsdStage::SetGlobalVariantFallbacks(fallbacks);

        // open a stage and put it to stage cache
        auto stage = PXR_NS::UsdStage::Open(filePath);
        stageCacheId = PXR_NS::UsdUtilsStageCache::Get().Insert(stage);
        EXPECT_TRUE(stageCacheId.IsValid());
        // set the same variant fallbacks on session layer
        auto                 sessionLayer = stage->GetSessionLayer();
        PXR_NS::VtDictionary layerData;
        // NOTE: the value is a pre-formatted string form of JSON data
        layerData["variant_fallbacks"]
            = PXR_NS::VtValue("{\n    \"geo\": [\"non_exist_variant\", \"sphere\"]\n}");
        sessionLayer->SetCustomLayerData(layerData);

        // restore to default
        PXR_NS::UsdStage::SetGlobalVariantFallbacks(defaultVariantFallbacks);
    }

    auto proxy = createProxy("testProxy", stageCacheId.ToLongInt());
    // verify loaded prim
    PXR_NS::SdfPath primPath("/root/GEO/sphere1/sphereShape1");
    EXPECT_TRUE(proxy->getUsdStage()->GetPrimAtPath(primPath).IsValid());
    // Note: MString below is a pre-formatted JSON data
    MString variantAttrVal("{\n    \"geo\": [\"non_exist_variant\", \"sphere\"]\n}");
    EXPECT_EQ(proxy->variantFallbacksPlug().asString(), variantAttrVal);

    // verify Maya node existence via selection
    MString translateCommand
        = MString("AL_usdmaya_TranslatePrim -importPaths \"/root/GEO/sphere1/sphereShape1\" "
                  "-forceImport -pushToPrim 0 ")
        + "-proxy \"" + proxy->name() + "\"";
    EXPECT_TRUE(MGlobal::executeCommand(translateCommand, true, false) == MStatus::kSuccess);
    MString  spherePath("|testProxy|root|GEO|sphere1|sphereShape1");
    MDagPath meshDagPath;
    {
        MSelectionList sel;
        sel.add(spherePath);
        sel.getDagPath(0, meshDagPath);
    }
    EXPECT_EQ(meshDagPath.fullPathName(), spherePath);
    // verify default global variant fallback
    EXPECT_EQ(PXR_NS::UsdStage::GetGlobalVariantFallbacks(), defaultVariantFallbacks);
}
