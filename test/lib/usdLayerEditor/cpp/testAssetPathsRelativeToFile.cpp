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
// makeAssetPathsRelativeToFile rewrites every absolute asset path authored in a layer so it is
// relative to the file the layer is about to be written to. These tests pin what it converts,
// what it leaves alone, and that the conversion can be undone.
//
#include <usdUfe/utils/fileSystem.h>

#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/payload.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/reference.h>

#include <ghc/fs_std.hpp>
#include <gtest/gtest.h>

#include <string>

#include <scopedTempDir.h>

using namespace UsdUfe::FileSystem;
using UsdLayerEditor::ScopedTempDir;

namespace {

PXR_NAMESPACE_USING_DIRECTIVE

// Layers are written to <root>/layers/saved.usda; assets live in <root>/assets and
// <root>/layers/textures.
struct AssetPathsFixture
{
    explicit AssetPathsFixture(const std::string& name)
        : dir(name)
        , target(dir.file("layers/saved.usda"))
        , farAsset(dir.file("assets/hdri.exr"))
        , nearAsset(dir.file("layers/textures/wood.exr"))
        , layer(SdfLayer::CreateAnonymous("render_layer"))
        , prim(SdfCreatePrimInLayer(layer, SdfPath("/Prim")))
    {
    }

    SdfAttributeSpecHandle
    attribute(const std::string& name, const SdfValueTypeName& type = SdfValueTypeNames->Asset)
    {
        return SdfAttributeSpec::New(prim, name, type);
    }

    ScopedTempDir     dir;
    std::string       target;
    std::string       farAsset;
    std::string       nearAsset;
    SdfLayerRefPtr    layer;
    SdfPrimSpecHandle prim;
};

// Runs with the given current directory, as a relative output path is relative to it.
struct ScopedCurrentDirectory
{
    explicit ScopedCurrentDirectory(const std::string& dir)
        : previous(fs::filesystem::current_path())
    {
        fs::filesystem::create_directories(dir);
        fs::filesystem::current_path(dir);
    }
    ~ScopedCurrentDirectory() { fs::filesystem::current_path(previous); }

    fs::filesystem::path previous;
};

std::string assetPathOf(const VtValue& value) { return value.Get<SdfAssetPath>().GetAssetPath(); }

std::string firstReference(const SdfPrimSpecHandle& prim)
{
    const auto items = prim->GetReferenceList().GetPrependedItems();
    return items[0].GetAssetPath();
}

std::string firstPayload(const SdfPrimSpecHandle& prim)
{
    const auto items = prim->GetPayloadList().GetPrependedItems();
    return items[0].GetAssetPath();
}

} // namespace

TEST(AssetPathsRelativeToFile, AuthoredPathsBecomeRelativeToTheTargetFile)
{
    AssetPathsFixture f("le_rel_authored");
    auto              value = f.attribute("value");
    auto              animated = f.attribute("animated");
    auto              array = f.attribute("array", SdfValueTypeNames->AssetArray);
    value->SetDefaultValue(VtValue(SdfAssetPath(f.farAsset)));
    f.layer->SetTimeSample(animated->GetPath(), 1.0, SdfAssetPath(f.farAsset));
    f.layer->SetTimeSample(animated->GetPath(), 2.0, SdfAssetPath(f.nearAsset));
    array->SetDefaultValue(
        VtValue(VtArray<SdfAssetPath> { SdfAssetPath(f.farAsset), SdfAssetPath(f.nearAsset) }));
    f.layer->GetSubLayerPaths().push_back(f.farAsset);
    f.prim->GetReferenceList().Prepend(SdfReference(f.farAsset));
    f.prim->GetPayloadList().Prepend(SdfPayload(f.nearAsset));

    const auto rewritten = makeAssetPathsRelativeToFile(f.layer, f.target);

    VtValue first, second;
    ASSERT_TRUE(f.layer->QueryTimeSample(animated->GetPath(), 1.0, &first));
    ASSERT_TRUE(f.layer->QueryTimeSample(animated->GetPath(), 2.0, &second));
    const auto paths = array->GetDefaultValue().Get<VtArray<SdfAssetPath>>();
    ASSERT_EQ(paths.size(), 2u);
    ASSERT_EQ(f.layer->GetSubLayerPaths().size(), 1u);
    EXPECT_EQ(assetPathOf(value->GetDefaultValue()), "../assets/hdri.exr");
    EXPECT_EQ(assetPathOf(first), "../assets/hdri.exr");
    EXPECT_EQ(assetPathOf(second), "./textures/wood.exr");
    EXPECT_EQ(paths[0].GetAssetPath(), "../assets/hdri.exr");
    EXPECT_EQ(paths[1].GetAssetPath(), "./textures/wood.exr");
    EXPECT_EQ(f.layer->GetSubLayerPaths()[0], "../assets/hdri.exr");
    EXPECT_EQ(firstReference(f.prim), "../assets/hdri.exr");
    EXPECT_EQ(firstPayload(f.prim), "./textures/wood.exr");

    // The rewrite can be undone, which is what a failed write relies on.
    restoreAssetPaths(f.layer, rewritten);

    ASSERT_TRUE(f.layer->QueryTimeSample(animated->GetPath(), 2.0, &second));
    EXPECT_EQ(assetPathOf(value->GetDefaultValue()), f.farAsset);
    EXPECT_EQ(assetPathOf(second), f.nearAsset);
    EXPECT_EQ(firstReference(f.prim), f.farAsset);
}

TEST(AssetPathsRelativeToFile, RestoreOnlyTouchesThePathsThatWereRewritten)
{
    AssetPathsFixture f("le_rel_restore_scope");
    // One path is rewritten to the exact text the other already has, in the same layer.
    const std::string absolute = f.dir.file("assets/model.usd");
    auto              rewritten = f.attribute("rewritten");
    auto              untouched = f.attribute("untouched");
    rewritten->SetDefaultValue(VtValue(SdfAssetPath(absolute)));
    untouched->SetDefaultValue(VtValue(SdfAssetPath("../assets/model.usd")));

    const auto changes = makeAssetPathsRelativeToFile(f.layer, f.target);
    EXPECT_EQ(assetPathOf(rewritten->GetDefaultValue()), "../assets/model.usd");
    EXPECT_EQ(assetPathOf(untouched->GetDefaultValue()), "../assets/model.usd");

    restoreAssetPaths(f.layer, changes);

    EXPECT_EQ(assetPathOf(rewritten->GetDefaultValue()), absolute);
    EXPECT_EQ(assetPathOf(untouched->GetDefaultValue()), "../assets/model.usd");
}

TEST(AssetPathsRelativeToFile, RelativeOutputPathIsResolvedAgainstTheCurrentDirectory)
{
    AssetPathsFixture f("le_rel_cwd");
    auto              saved = SdfLayer::CreateNew(f.dir.file("a/original.usda"));
    ASSERT_TRUE(saved);
    auto prim = SdfCreatePrimInLayer(saved, SdfPath("/Prim"));
    prim->GetReferenceList().Prepend(SdfReference(f.farAsset));

    {
        // An output file in a sub folder: <root>/b/c/copy.usda.
        ScopedCurrentDirectory cwd(f.dir.path());
        makeAssetPathsRelativeToFile(saved, "b/c/copy.usda");
    }
    EXPECT_EQ(firstReference(prim), "../../assets/hdri.exr");

    // A bare file name: <root>/layers/saved.usda.
    auto bare = SdfCreatePrimInLayer(f.layer, SdfPath("/Bare"));
    bare->GetReferenceList().Prepend(SdfReference(f.nearAsset));
    {
        ScopedCurrentDirectory cwd(f.dir.file("layers"));
        makeAssetPathsRelativeToFile(f.layer, "saved.usda");
    }
    EXPECT_EQ(firstReference(bare), "./textures/wood.exr");
}

TEST(AssetPathsRelativeToFile, PathsThatCannotOrNeedNotChangeAreLeftAlone)
{
    AssetPathsFixture f("le_rel_untouched");
    // Relative paths of an anonymous layer, tokens, environment variables and URIs (with or
    // without "//" after the scheme).
    const char* kept[]
        = { "textures/wood.exr", "${ASSETS}/hdri.exr",           "$ASSETS/hdri.exr",
            "%ASSETS%/hdri.exr", "https://example.com/hdri.exr", "file:/assets/model.usd",
            "urn:asset:model" };
    for (const char* path : kept) {
        f.prim->GetReferenceList().Append(SdfReference(path));
    }

    EXPECT_TRUE(makeAssetPathsRelativeToFile(f.layer, f.target).empty());

    const auto items = f.prim->GetReferenceList().GetAppendedItems();
    ASSERT_EQ(items.size(), std::size(kept));
    for (size_t i = 0; i < items.size(); ++i) {
        EXPECT_EQ(items[i].GetAssetPath(), kept[i]);
    }

#ifdef _WIN32
    // No relative path exists between two drives; the path is kept and a warning is logged.
    auto attr = f.attribute("tex");
    attr->SetDefaultValue(VtValue(SdfAssetPath("Z:/elsewhere/hdri.exr")));
    EXPECT_TRUE(makeAssetPathsRelativeToFile(f.layer, "C:/layers/saved.usda").empty());
    EXPECT_EQ(assetPathOf(attr->GetDefaultValue()), "Z:/elsewhere/hdri.exr");
#endif

    // Nothing to do without a layer or a target.
    f.prim->GetReferenceList().Prepend(SdfReference(f.farAsset));
    EXPECT_TRUE(makeAssetPathsRelativeToFile(SdfLayerHandle(), f.target).empty());
    EXPECT_TRUE(makeAssetPathsRelativeToFile(f.layer, "").empty());
    EXPECT_EQ(firstReference(f.prim), f.farAsset);
    restoreAssetPaths(SdfLayerHandle(), {});
}

TEST(AssetPathsRelativeToFile, OnlyAbsolutePathsAreConvertedWhenTheFolderChanges)
{
    AssetPathsFixture f("le_rel_only_absolute");
    auto              saved = SdfLayer::CreateNew(f.dir.file("a/original.usda"));
    ASSERT_TRUE(saved);
    auto prim = SdfCreatePrimInLayer(saved, SdfPath("/Prim"));

    // Not absolute, so not touched, whatever token syntax they use.
    const char* kept[] = { "../assets/model.usd", "./textures/wood.exr",  "model.usd",
                           "<ASSETS>/model.usd",  "{{assets}}/model.usd", "@ASSETS@/model.usd" };
    for (const char* path : kept) {
        prim->GetReferenceList().Append(SdfReference(path));
    }
    prim->GetReferenceList().Prepend(SdfReference(f.farAsset));

    makeAssetPathsRelativeToFile(saved, f.dir.file("b/c/copy.usda"));

    EXPECT_EQ(firstReference(prim), "../../assets/hdri.exr");
    const auto items = prim->GetReferenceList().GetAppendedItems();
    ASSERT_EQ(items.size(), std::size(kept));
    for (size_t i = 0; i < items.size(); ++i) {
        EXPECT_EQ(items[i].GetAssetPath(), kept[i]);
    }
}

TEST(AssetPathsRelativeToFile, DotDotAfterASymlinkIsResolvedThroughTheLink)
{
#ifdef _WIN32
    // Win32 path APIs collapse ".." lexically before following any link, so "link/.." really is
    // the folder holding the link there. The behavior under test is POSIX only.
    GTEST_SKIP() << "Windows resolves \"..\" lexically.";
#endif
    AssetPathsFixture f("le_rel_symlink");
    auto              saved = SdfLayer::CreateNew(f.dir.file("a/original.usda"));
    ASSERT_TRUE(saved);

    // a/link -> elsewhere/nested, so "a/link/.." is "elsewhere", not "a".
    fs::filesystem::create_directories(f.dir.file("elsewhere/nested"));
    std::error_code error;
    fs::filesystem::create_directory_symlink(
        fs::filesystem::path(f.dir.file("elsewhere/nested")), f.dir.file("a/link"), error);
    if (error) {
        GTEST_SKIP() << "Cannot create symbolic links here: " << error.message();
    }

    auto prim = SdfCreatePrimInLayer(saved, SdfPath("/Prim"));
    prim->GetReferenceList().Append(SdfReference(f.dir.file("a/link/../asset.usd")));

    // The saved file lives in a folder reached through the same link.
    makeAssetPathsRelativeToFile(saved, f.dir.file("b/copy.usda"));

    const auto items = prim->GetReferenceList().GetAppendedItems();
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0].GetAssetPath(), "../elsewhere/asset.usd");
}
