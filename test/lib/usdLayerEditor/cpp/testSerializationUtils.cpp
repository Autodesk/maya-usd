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
#include "layerEditorDCCFunctions.h"
#include "scopedLayerEditorDCCFunctions.h"
#include "scopedTempDir.h"
#include "utilSerialization.h"

#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/base/tf/weakPtr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/stageCache.h>
#include <pxr/usd/usd/stageLoadRules.h>

#include <ghc/fs_std.hpp>
#include <gtest/gtest.h>

#include <cstdio>
#include <string>
#include <vector>

namespace UsdLayerEditor {
namespace Serialization {

namespace {
bool sublayerPathsContain(const PXR_NS::SdfLayerRefPtr& layer, const std::string& path)
{
    for (const std::string p : layer->GetSubLayerPaths()) {
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

// --- generateUniqueLayerFileName ----------------------------------------------

TEST(SerializationUtils, GenerateUniqueLayerFileName_UsesCustomTagAsFileName)
{
    ScopedLayerEditorDCCFunctions guard;
    auto                          layer = PXR_NS::SdfLayer::CreateAnonymous("RenderLayer_1");

    const fs::filesystem::path path = generateUniqueLayerFileName("stage", layer);

    EXPECT_EQ(path.stem().string().find("RenderLayer_1"), 0u);
}

TEST(SerializationUtils, GenerateUniqueLayerFileName_KeepsStageNameWhenTagHasAnonymous)
{
    ScopedLayerEditorDCCFunctions guard;
    auto                          layer = PXR_NS::SdfLayer::CreateAnonymous("anonymousLayer2");

    const std::string name
        = fs::filesystem::path(generateUniqueLayerFileName("stage", layer)).filename().string();

    EXPECT_EQ(name, "stage-layer2.usd");
}

TEST(SerializationUtils, GenerateUniqueLayerFileName_TagMatchIgnoresCase)
{
    ScopedLayerEditorDCCFunctions guard;
    auto                          layer = PXR_NS::SdfLayer::CreateAnonymous("Anonymous3");

    const std::string name
        = fs::filesystem::path(generateUniqueLayerFileName("stage", layer)).filename().string();

    EXPECT_EQ(name, "stage-layer3.usd");
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
    auto parent = PXR_NS::SdfLayer::CreateAnonymous("parent_upd");
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
    auto parent = PXR_NS::SdfLayer::CreateAnonymous("parent_empty");
    auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sub_notadded");
    // sublayer was never added to parent → Replace finds nothing → no crash, no change
    EXPECT_NO_THROW(updateSubLayer(parent, sublayer, "/new/layer.usd"));
    EXPECT_TRUE(parent->GetSubLayerPaths().empty());
}

// --- generateUniqueFileName ---------------------------------------------------

TEST(SerializationUtils, GenerateUniqueFileName_ReturnsNonEmptyString)
{
    std::string first = generateUniqueFileName("test");
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
    setSaveOptionFns(SaveOptionFns {});
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
    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("sublayer_to_save");
    stage->GetRootLayer()->InsertSubLayerPath(sublayer->GetIdentifier(), 0);

    StageLayersToSave info;
    getLayersToSaveFromStage(stage, "test_obj", info);

    // Root layer is anonymous and at least one anonymous sublayer was added.
    EXPECT_GE(info._anonLayers.size(), 1u);
}

namespace {
// A dirty file-backed layer, not part of any stage's layer stack.
PXR_NS::SdfLayerRefPtr createDirtyFileBackedLayer(const ScopedTempDir& dir, const std::string& name)
{
    auto layer = PXR_NS::SdfLayer::CreateNew(dir.file(name));
    layer->SetComment("unsaved edit");
    return layer;
}

ComponentFns componentFnsListing(const std::vector<std::string>& layerIds)
{
    ComponentFns component;
    component.isStageAComponent
        = [](const std::string& objectPath) { return objectPath == "component_obj"; };
    component.getComponentLayersToSave = [layerIds](const std::string&) { return layerIds; };
    return component;
}
} // namespace

// Only the component creator knows which layers saving a component touches, and they can be
// outside the stage's layer stack, so they cannot be found by walking it.
TEST(SerializationUtils, GetLayersToSaveFromStage_ComponentStage_ReportsLayersChosenByTheComponent)
{
    ScopedTempDir tempDir("le_component_layers_to_save_test");
    auto          componentLayer = createDirtyFileBackedLayer(tempDir, "component_layer.usda");
    ASSERT_TRUE(componentLayer->IsDirty());

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    ASSERT_FALSE(stage->HasLocalLayer(componentLayer));

    ScopedLayerEditorDCCFunctions guard;
    setComponentFns(componentFnsListing({ componentLayer->GetIdentifier() }));

    StageLayersToSave info;
    getLayersToSaveFromStage(stage, "component_obj", info);

    ASSERT_EQ(info._dirtyFileBackedLayers.size(), 1u);
    EXPECT_EQ(info._dirtyFileBackedLayers[0], componentLayer);
}

TEST(SerializationUtils, GetLayersToSaveFromStage_ComponentStage_SkipsAnonymousAndUnknownLayers)
{
    auto anonymousLayer = PXR_NS::SdfLayer::CreateAnonymous("component_anon");
    auto stage = PXR_NS::UsdStage::CreateInMemory();

    ScopedLayerEditorDCCFunctions guard;
    setComponentFns(
        componentFnsListing({ anonymousLayer->GetIdentifier(), "does/not/exist.usda" }));

    StageLayersToSave info;
    getLayersToSaveFromStage(stage, "component_obj", info);

    EXPECT_TRUE(info._dirtyFileBackedLayers.empty());
}

TEST(SerializationUtils, GetLayersToSaveFromStage_NonComponentStage_IgnoresTheComponentHook)
{
    ScopedTempDir tempDir("le_non_component_layers_to_save_test");
    auto          localSublayer = createDirtyFileBackedLayer(tempDir, "local_sublayer.usda");
    auto          componentOnlyLayer = createDirtyFileBackedLayer(tempDir, "component_only.usda");

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    stage->GetRootLayer()->InsertSubLayerPath(localSublayer->GetIdentifier(), 0);

    ScopedLayerEditorDCCFunctions guard;
    ComponentFns component = componentFnsListing({ componentOnlyLayer->GetIdentifier() });
    component.isStageAComponent = [](const std::string&) { return false; };
    setComponentFns(component);

    StageLayersToSave info;
    getLayersToSaveFromStage(stage, "component_obj", info);

    ASSERT_EQ(info._dirtyFileBackedLayers.size(), 1u);
    EXPECT_EQ(info._dirtyFileBackedLayers[0], localSublayer);
}

// --- saveAnonymousLayer -------------------------------------------------------

namespace {
struct AnonSubLayersStage
{
    PXR_NS::UsdStageRefPtr stage { PXR_NS::UsdStage::CreateInMemory() };
    PXR_NS::SdfLayerRefPtr first { PXR_NS::SdfLayer::CreateAnonymous("first") };
    PXR_NS::SdfLayerRefPtr second { PXR_NS::SdfLayer::CreateAnonymous("second") };

    AnonSubLayersStage()
    {
        stage->GetRootLayer()->GetSubLayerPaths().push_back(first->GetIdentifier());
        stage->GetRootLayer()->GetSubLayerPaths().push_back(second->GetIdentifier());
    }

    PXR_NS::SdfLayerRefPtr save(const PXR_NS::SdfLayerRefPtr& layer, const PathInfo& pathInfo)
    {
        LayerParent parent;
        parent._layerParent = stage->GetRootLayer();
        return saveAnonymousLayer(stage, layer, pathInfo, parent);
    }
};
} // namespace

TEST(SerializationUtils, SaveAnonymousLayer_NonTargetSubLayer_KeepsEditTarget)
{
    ScopedTempDir                 tempDir("le_save_anon_keep_target_test");
    ScopedLayerEditorDCCFunctions guard;

    AnonSubLayersStage test;
    test.stage->SetEditTarget(test.second);

    PathInfo pathInfo;
    pathInfo.absolutePath = tempDir.file("first.usd");
    auto saved = test.save(test.first, pathInfo);
    ASSERT_TRUE(saved);

    EXPECT_EQ(test.stage->GetEditTarget().GetLayer(), test.second)
        << "saving a layer that is not the edit target must not move the edit target";
}

TEST(SerializationUtils, SaveAnonymousLayer_TargetSubLayer_RetargetsToSavedLayer)
{
    ScopedTempDir                 tempDir("le_save_anon_move_target_test");
    ScopedLayerEditorDCCFunctions guard;

    AnonSubLayersStage test;
    test.stage->SetEditTarget(test.second);

    PathInfo pathInfo;
    pathInfo.absolutePath = tempDir.file("second.usd");
    auto saved = test.save(test.second, pathInfo);
    ASSERT_TRUE(saved);

    EXPECT_EQ(test.stage->GetEditTarget().GetLayer(), saved)
        << "the edit target must follow the target layer to its file-backed replacement";
}

namespace {
struct RootLayerUpdateRecord
{
    int  calls { 0 };
    bool wasTargetLayer { false };

    explicit RootLayerUpdateRecord()
    {
        SerializationFns serialization;
        serialization.updateDCCObjectRootLayer = [this](
                                                     const std::string&,
                                                     const std::string&,
                                                     const PXR_NS::SdfLayerRefPtr&,
                                                     bool wasTarget,
                                                     DccObjectRootLayerPathMode) {
            ++calls;
            wasTargetLayer = wasTarget;
        };
        setSerializationFns(serialization);
    }
};
} // namespace

TEST(SerializationUtils, SaveAnonymousLayer_PlainRootLayer_UpdatesTheDCCObject)
{
    ScopedTempDir                 tempDir("le_plain_root_save_test");
    ScopedLayerEditorDCCFunctions guard;
    RootLayerUpdateRecord         record;

    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto anonLayer = PXR_NS::SdfLayer::CreateAnonymous("plainRoot1");

    PathInfo pathInfo;
    pathInfo.absolutePath = tempDir.file("plainRoot1.usd");

    LayerParent parent;
    parent._objectPath = "test_obj";

    // Putting the user's target back goes through Ufe::PathString, and this binary registers no
    // UFE run-time, so that step throws. Everything asserted below is recorded before it.
    try {
        saveAnonymousLayer(stage, anonLayer, pathInfo, parent);
    } catch (const std::exception&) {
    }

    EXPECT_EQ(record.calls, 1);
    EXPECT_FALSE(record.wasTargetLayer);
}

TEST(SerializationUtils, SaveAnonymousLayer_RootLayerThatIsTheTarget_TellsTheDCC)
{
    ScopedTempDir                 tempDir("le_target_root_save_test");
    ScopedLayerEditorDCCFunctions guard;
    RootLayerUpdateRecord         record;

    auto anonLayer = PXR_NS::SdfLayer::CreateAnonymous("targetRoot1");
    auto stage = PXR_NS::UsdStage::Open(anonLayer);

    PathInfo pathInfo;
    pathInfo.absolutePath = tempDir.file("targetRoot1.usd");

    LayerParent parent;
    parent._objectPath = "test_obj";

    saveAnonymousLayer(stage, anonLayer, pathInfo, parent);

    EXPECT_EQ(record.calls, 1);
    EXPECT_TRUE(record.wasTargetLayer)
        << "the DCC retargets the new stage when the root was the target";
}

// --- updateTargetLayer --------------------------------------------------------

TEST(SerializationUtils, UpdateTargetLayer_TargetsLayerInTheStage)
{
    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("inStage");
    stage->GetRootLayer()->InsertSubLayerPath(sublayer->GetIdentifier(), 0);

    updateTargetLayer(stage, sublayer);

    EXPECT_EQ(stage->GetEditTarget().GetLayer(), sublayer);
}

TEST(SerializationUtils, UpdateTargetLayer_IgnoresLayerOutsideTheStageAndNullArguments)
{
    auto stage = PXR_NS::UsdStage::CreateInMemory();
    auto sublayer = PXR_NS::SdfLayer::CreateAnonymous("inStage");
    stage->GetRootLayer()->InsertSubLayerPath(sublayer->GetIdentifier(), 0);
    stage->SetEditTarget(sublayer);

    updateTargetLayer(stage, PXR_NS::SdfLayer::CreateAnonymous("outside"));
    updateTargetLayer(stage, PXR_NS::SdfLayerRefPtr());
    EXPECT_NO_THROW(updateTargetLayer(PXR_NS::UsdStageRefPtr(), sublayer));

    EXPECT_EQ(stage->GetEditTarget().GetLayer(), sublayer);
}

// --- saveLayerWithFormat ------------------------------------------------------

TEST(SerializationUtils, SaveLayerWithFormat_EmptyPath_ReturnsFalse)
{
    auto layer = PXR_NS::SdfLayer::CreateAnonymous("save_test");
    // Empty path → falls back to GetRealPath() which is also empty for an anonymous
    // layer → Save() with no backing file returns false on all platforms.
    EXPECT_FALSE(saveLayerWithFormat(layer, "", "usda"));
}

// --- saved layers keep the stage state of cached stages ------------------------------------

TEST(SerializationUtils, SaveLayerWithFormat_KeepsLoadRulesAndMutedLayersOfCachedStage)
{
    ScopedLayerEditorDCCFunctions guard;
    PXR_NS::UsdStageCache         cache;
    SerializationFns              serialization = layerEditorDCCFunctions().serialization;
    serialization.getStageCaches
        = [&cache]() { return std::vector<PXR_NS::UsdStageCache*> { &cache }; };
    setSerializationFns(serialization);

    auto root = PXR_NS::SdfLayer::CreateAnonymous("cached_root");
    auto sub = PXR_NS::SdfLayer::CreateAnonymous("cached_sub");
    root->GetSubLayerPaths().push_back(sub->GetIdentifier());
    auto stage = PXR_NS::UsdStage::Open(root, PXR_NS::UsdStage::InitialLoadSet::LoadNone);
    PXR_NS::UsdStageLoadRules rules;
    rules.AddRule(PXR_NS::SdfPath("/Prim"), PXR_NS::UsdStageLoadRules::NoneRule);
    stage->SetLoadRules(rules);
    stage->MuteLayer(sub->GetIdentifier());
    cache.Insert(stage);

    const std::string path
        = (fs::filesystem::temp_directory_path() / "le_cached_stage.usda").string();
    ASSERT_TRUE(saveLayerWithFormat(root, path, "usda"));

    auto newLayer = PXR_NS::SdfLayer::FindOrOpen(path);
    ASSERT_TRUE(newLayer);
    auto found = cache.FindAllMatching(newLayer);
    ASSERT_EQ(found.size(), 1u);
    EXPECT_EQ(found[0]->GetLoadRules(), rules);
    EXPECT_EQ(found[0]->GetMutedLayers(), std::vector<std::string> { sub->GetIdentifier() });

    found.clear();
    stage = nullptr;
    newLayer = nullptr;
    std::remove(path.c_str());
}

namespace {

// Counts the notices that tell a stage was recomposed.
struct RecompositionCounter : public PXR_NS::TfWeakBase
{
    void OnObjectsChanged(const PXR_NS::UsdNotice::ObjectsChanged&) { ++count; }
    void OnContentsChanged(const PXR_NS::UsdNotice::StageContentsChanged&) { ++count; }

    int count { 0 };
};

} // namespace

TEST(SerializationUtils, SaveLayerWithFormat_InPlaceSaveDoesNotRecomposeCachedStage)
{
    ScopedLayerEditorDCCFunctions guard;
    PXR_NS::UsdStageCache         cache;
    SerializationFns              serialization = layerEditorDCCFunctions().serialization;
    serialization.getStageCaches
        = [&cache]() { return std::vector<PXR_NS::UsdStageCache*> { &cache }; };
    setSerializationFns(serialization);

    const std::string path
        = (fs::filesystem::temp_directory_path() / "le_inplace_stage.usda").string();
    auto layer = PXR_NS::SdfLayer::CreateNew(path);
    ASSERT_TRUE(layer);
    ASSERT_TRUE(layer->Save());
    auto stage = PXR_NS::UsdStage::Open(layer, PXR_NS::UsdStage::InitialLoadSet::LoadNone);
    cache.Insert(stage);
    stage->DefinePrim(PXR_NS::SdfPath("/Dirty"));
    ASSERT_TRUE(layer->IsDirty());

    RecompositionCounter  counter;
    auto                  weakCounter = PXR_NS::TfCreateWeakPtr(&counter);
    auto                  weakStage = PXR_NS::UsdStageWeakPtr(stage);
    PXR_NS::TfNotice::Key objectsKey = PXR_NS::TfNotice::Register(
        weakCounter, &RecompositionCounter::OnObjectsChanged, weakStage);
    PXR_NS::TfNotice::Key contentsKey = PXR_NS::TfNotice::Register(
        weakCounter, &RecompositionCounter::OnContentsChanged, weakStage);

    // Saving in place must not touch the cached stage: re-applying the load rules forces a
    // full recomposition of the stage.
    ASSERT_TRUE(saveLayerWithFormat(layer));

    PXR_NS::TfNotice::Revoke(objectsKey);
    PXR_NS::TfNotice::Revoke(contentsKey);
    EXPECT_EQ(counter.count, 0);

    stage = nullptr;
    layer = nullptr;
    std::remove(path.c_str());
}

} // namespace Serialization
} // namespace UsdLayerEditor
