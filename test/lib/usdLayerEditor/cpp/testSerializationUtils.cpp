// Copyright 2026 Autodesk
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
#include "utilSerialization.h"

#include "layerEditorDCCFunctions.h"
#include "scopedLayerEditorDCCFunctions.h"

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <ghc/fs_std.hpp>

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

namespace UsdLayerEditor {
namespace Serialization {

namespace {
bool sublayerPathsContain(const PXR_NS::SdfLayerRefPtr& layer, const std::string& path)
{
    for (const auto& p : layer->GetSubLayerPaths()) {
        if (p == path)
            return true;
    }
    return false;
}
} // namespace

// --- ensureUSDFileExtension ---------------------------------------------------

TEST(SerializationUtils, EnsureUSDFileExtension_AppendsUsdWhenNoExtension)
{
    std::string path = "myfile";
    ensureUSDFileExtension(path);
    EXPECT_EQ(path, "myfile.usd");
}

TEST(SerializationUtils, EnsureUSDFileExtension_NoOpForUsd)
{
    std::string path = "myfile.usd";
    ensureUSDFileExtension(path);
    EXPECT_EQ(path, "myfile.usd");
}

TEST(SerializationUtils, EnsureUSDFileExtension_NoOpForUsdc)
{
    std::string path = "myfile.usdc";
    ensureUSDFileExtension(path);
    EXPECT_EQ(path, "myfile.usdc");
}

TEST(SerializationUtils, EnsureUSDFileExtension_NoOpForUsda)
{
    std::string path = "myfile.usda";
    ensureUSDFileExtension(path);
    EXPECT_EQ(path, "myfile.usda");
}

TEST(SerializationUtils, EnsureUSDFileExtension_NoOpForUsdz)
{
    std::string path = "myfile.usdz";
    ensureUSDFileExtension(path);
    EXPECT_EQ(path, "myfile.usdz");
}

TEST(SerializationUtils, EnsureUSDFileExtension_AppendsUsdForForeignExtension)
{
    std::string path = "myfile.txt";
    ensureUSDFileExtension(path);
    EXPECT_EQ(path, "myfile.txt.usd");
}

// --- updateSubLayer -----------------------------------------------------------

TEST(SerializationUtils, UpdateSubLayer_NoOpForNullParent)
{
    auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sub_noparent");
    EXPECT_NO_THROW(updateSubLayer(nullptr, sublayer, "/new/path.usd"));
    EXPECT_TRUE(sublayer->GetSubLayerPaths().empty());
}

TEST(SerializationUtils, UpdateSubLayer_NoOpForNullOldLayer)
{
    auto parent = PXR_NS::SdfLayer::CreateAnonymous("parent_nosub");
    EXPECT_NO_THROW(updateSubLayer(parent, nullptr, "/new/path.usd"));
    EXPECT_TRUE(parent->GetSubLayerPaths().empty());
}

TEST(SerializationUtils, UpdateSubLayer_ReplacesIdentifierInParent)
{
    auto parent   = PXR_NS::SdfLayer::CreateAnonymous("parent_upd");
    auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sub_upd");

    parent->InsertSubLayerPath(sublayer->GetIdentifier(), 0);
    ASSERT_TRUE(sublayerPathsContain(parent, sublayer->GetIdentifier()))
        << "precondition: sublayer identifier must be in parent's paths";

    const std::string newPath = "/new/layer.usd";
    updateSubLayer(parent, sublayer, newPath);

    // Old identifier removed; new path present.
    EXPECT_FALSE(sublayerPathsContain(parent, sublayer->GetIdentifier()));
    EXPECT_TRUE(sublayerPathsContain(parent, newPath));
}

TEST(SerializationUtils, UpdateSubLayer_NewParentHasNoSubLayerBecomesNoOp)
{
    auto parent   = PXR_NS::SdfLayer::CreateAnonymous("parent_empty");
    auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sub_notadded");
    // sublayer was never added to parent → Replace finds nothing → no crash, no change
    EXPECT_NO_THROW(updateSubLayer(parent, sublayer, "/new/layer.usd"));
    EXPECT_TRUE(parent->GetSubLayerPaths().empty());
}

// --- generateUniqueFileName ---------------------------------------------------

TEST(SerializationUtils, GenerateUniqueFileName_ReturnsNonEmptyString)
{
    std::string first  = generateUniqueFileName("test");
    std::string second = generateUniqueFileName("test");
    EXPECT_FALSE(first.empty());
    EXPECT_NE(first.find("test"), std::string::npos);
    EXPECT_EQ(first.substr(first.size() - 4), ".usd");
    EXPECT_FALSE(second.empty());
    EXPECT_NE(second.find("test"), std::string::npos);
    EXPECT_EQ(second.substr(second.size() - 4), ".usd");
    // A random suffix is appended, so successive calls never collide.
    EXPECT_NE(first, second);
}

// --- generateUniqueLayerFileName ----------------------------------------------

TEST(SerializationUtils, GenerateUniqueLayerFileName_WithLayer_AvoidsExistingFile)
{
    namespace fss = fs::filesystem;

    // Point the scene folder at a dedicated temp dir we control, restored on scope exit.
    const fss::path sceneDir = fss::temp_directory_path() / "le_unique_layer_name_test";
    fss::create_directories(sceneDir);

    ScopedLayerEditorDCCFunctions guard;
    FileSystemFns                 fns;
    fns.getDCCSceneDir = [dir = sceneDir.generic_string()]() { return dir; };
    setFileSystemFns(fns);

    auto layer = PXR_NS::SdfLayer::CreateAnonymous("sublayer0");

    // First call yields the deterministic candidate; pre-create it to force a conflict.
    const std::string firstCandidate = generateUniqueLayerFileName("scene", layer);
    ASSERT_FALSE(firstCandidate.empty());
    if (FILE* f = std::fopen(firstCandidate.c_str(), "w")) {
        std::fclose(f);
    }

    const std::string result = generateUniqueLayerFileName("scene", layer);
    EXPECT_NE(result, firstCandidate);
    EXPECT_FALSE(fss::exists(fss::path(result)));

    fss::remove(fss::path(firstCandidate));
    fss::remove_all(sceneDir);
}

// --- usdFormatArgOption -------------------------------------------------------

TEST(SerializationUtils, UsdFormatArgOption_DefaultsToUsdc)
{
    // No DCC handler installed: getSaveLayerFormatBinary defaults to true → binary → usdc.
    ScopedLayerEditorDCCFunctions guard;
    setSaveOptionFns(SaveOptionFns{});
    EXPECT_EQ(usdFormatArgOption(), "usdc");
}

TEST(SerializationUtils, UsdFormatArgOption_NonBinaryReturnsUsda)
{
    ScopedLayerEditorDCCFunctions guard;
    SaveOptionFns                 fns;
    fns.getSaveLayerFormatBinary = []() { return false; };
    setSaveOptionFns(fns);
    EXPECT_EQ(usdFormatArgOption(), "usda");
}

// --- getLayersToSaveFromStage -------------------------------------------------

TEST(SerializationUtils, GetLayersToSaveFromStage_NullStage_DoesNotCrash)
{
    StageLayersToSave info;
    EXPECT_NO_THROW(getLayersToSaveFromStage(nullptr, "obj", info));
    EXPECT_TRUE(info._anonLayers.empty());
    EXPECT_TRUE(info._dirtyFileBackedLayers.empty());
}

TEST(SerializationUtils, GetLayersToSaveFromStage_ValidStage_PopulatesAnonLayers)
{
    auto stage    = PXR_NS::UsdStage::CreateInMemory();
    auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sublayer_to_save");
    stage->GetRootLayer()->InsertSubLayerPath(sublayer->GetIdentifier(), 0);

    StageLayersToSave info;
    getLayersToSaveFromStage(stage, "test_obj", info);

    // Root layer is anonymous and at least one anonymous sublayer was added.
    EXPECT_GE(info._anonLayers.size(), 1u);
}

// --- saveLayerWithFormat ------------------------------------------------------

TEST(SerializationUtils, SaveLayerWithFormat_EmptyPath_ReturnsFalse)
{
    auto layer = PXR_NS::SdfLayer::CreateAnonymous("save_test");
    // Empty path → falls back to GetRealPath() which is also empty for an anonymous
    // layer → Save() with no backing file returns false on all platforms.
    EXPECT_FALSE(saveLayerWithFormat(layer, "", "usda"));
}

} // namespace Serialization
} // namespace UsdLayerEditor
