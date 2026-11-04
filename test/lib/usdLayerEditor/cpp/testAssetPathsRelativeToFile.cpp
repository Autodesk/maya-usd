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
    prim->GetReferenceList().Append(SdfReference("../assets/model.usd"));

    {
        // An output file in a sub folder: <root>/b/c/copy.usda.
        ScopedCurrentDirectory cwd(f.dir.path());
        makeAssetPathsRelativeToFile(saved, "b/c/copy.usda");
    }
    EXPECT_EQ(firstReference(prim), "../../assets/hdri.exr");
    const auto appended = prim->GetReferenceList().GetAppendedItems();
    EXPECT_EQ(appended[0].GetAssetPath(), "../../assets/model.usd");

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
    const char* kept[] = { "textures/wood.exr",
                           "${ASSETS}/hdri.exr",
                           "$ASSETS/hdri.exr",
                           "%ASSETS%/hdri.exr",
                           "https://example.com/hdri.exr",
                           "file:/assets/model.usd",
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

TEST(AssetPathsRelativeToFile, RelativePathsOfASavedLayerAreOnlyReanchoredWhenTheFolderChanges)
{
    AssetPathsFixture f("le_rel_reanchor");
    auto              saved = SdfLayer::CreateNew(f.dir.file("a/original.usda"));
    ASSERT_TRUE(saved);
    auto prim = SdfCreatePrimInLayer(saved, SdfPath("/Prim"));
    prim->GetReferenceList().Prepend(SdfReference("../assets/model.usd"));

    // Same folder: left alone.
    EXPECT_TRUE(makeAssetPathsRelativeToFile(saved, f.dir.file("a/other.usda")).empty());
    EXPECT_EQ(firstReference(prim), "../assets/model.usd");

    // Other folder: still the same file, <root>/assets/model.usd.
    makeAssetPathsRelativeToFile(saved, f.dir.file("b/c/copy.usda"));
    EXPECT_EQ(firstReference(prim), "../../assets/model.usd");
}

TEST(AssetPathsRelativeToFile, UriIdentifiersOfASavedLayerAreKeptWhenTheFolderChanges)
{
    AssetPathsFixture f("le_rel_uri");
    auto              saved = SdfLayer::CreateNew(f.dir.file("a/original.usda"));
    ASSERT_TRUE(saved);
    auto prim = SdfCreatePrimInLayer(saved, SdfPath("/Prim"));

    // Schemes with and without "//"; none of them is a file system path.
    const char* uris[] = { "https://example.com/hdri.exr", "file:/assets/model.usd",
                           "urn:asset:model" };
    for (const char* uri : uris) {
        prim->GetReferenceList().Append(SdfReference(uri));
    }
    // A plain relative path next to them is still re-anchored.
    prim->GetReferenceList().Append(SdfReference("../assets/model.usd"));

    makeAssetPathsRelativeToFile(saved, f.dir.file("b/c/copy.usda"));

    const auto items = prim->GetReferenceList().GetAppendedItems();
    ASSERT_EQ(items.size(), std::size(uris) + 1);
    for (size_t i = 0; i < std::size(uris); ++i) {
        EXPECT_EQ(items[i].GetAssetPath(), uris[i]);
    }
    EXPECT_EQ(items[std::size(uris)].GetAssetPath(), "../../assets/model.usd");
}

TEST(AssetPathsRelativeToFile, SearchPathIdentifiersAreNotReanchoredWhenTheFolderChanges)
{
    AssetPathsFixture f("le_rel_search_path");
    auto              saved = SdfLayer::CreateNew(f.dir.file("a/original.usda"));
    ASSERT_TRUE(saved);

    // "present.usd" exists beside the layer, so the resolver anchors it. "missing.usd" does not,
    // so it is a search-path identifier: moving the layer must not turn it into a path to a
    // file that does not exist.
    {
        auto present = SdfLayer::CreateNew(f.dir.file("a/present.usd"));
        ASSERT_TRUE(present);
        ASSERT_TRUE(present->Save());
    }
    auto prim = SdfCreatePrimInLayer(saved, SdfPath("/Prim"));
    prim->GetReferenceList().Append(SdfReference("present.usd"));
    prim->GetReferenceList().Append(SdfReference("missing.usd"));

    makeAssetPathsRelativeToFile(saved, f.dir.file("b/copy.usda"));

    const auto items = prim->GetReferenceList().GetAppendedItems();
    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0].GetAssetPath(), "../a/present.usd");
    EXPECT_EQ(items[1].GetAssetPath(), "missing.usd");
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
    prim->GetReferenceList().Append(SdfReference("./link/../asset.usd"));

    // The saved file lives in a folder reached through the same link.
    makeAssetPathsRelativeToFile(saved, f.dir.file("b/copy.usda"));

    const auto items = prim->GetReferenceList().GetAppendedItems();
    ASSERT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0].GetAssetPath(), "../elsewhere/asset.usd");
}

#ifdef _WIN32
TEST(AssetPathsRelativeToFile, RelativePathsAreWrittenAbsoluteWhenTheLayerMovesToAnotherDrive)
{
    AssetPathsFixture f("le_rel_other_drive");
    auto              saved = SdfLayer::CreateNew(f.dir.file("a/original.usda"));
    ASSERT_TRUE(saved);
    auto prim = SdfCreatePrimInLayer(saved, SdfPath("/Prim"));
    prim->GetReferenceList().Prepend(SdfReference("../assets/model.usd"));
    auto attr = SdfAttributeSpec::New(prim, "tex", SdfValueTypeNames->Asset);
    attr->SetDefaultValue(VtValue(SdfAssetPath("./textures/wood.exr")));

    // A drive other than the one holding the layer; no relative path exists between them.
    const std::string otherDrive
        = fs::filesystem::path(f.dir.file("a")).root_name().string() == "D:" ? "C:" : "D:";
    const auto rewritten
        = makeAssetPathsRelativeToFile(saved, otherDrive + "/publish/copy.usda");

    // The assets stay where they are: absolute paths, not the same text on the new drive.
    EXPECT_FALSE(rewritten.empty());
    EXPECT_EQ(firstReference(prim), f.dir.file("assets/model.usd"));
    EXPECT_EQ(assetPathOf(attr->GetDefaultValue()), f.dir.file("a/textures/wood.exr"));

    // Restoring brings back what was authored.
    restoreAssetPaths(saved, rewritten);
    EXPECT_EQ(firstReference(prim), "../assets/model.usd");
    EXPECT_EQ(assetPathOf(attr->GetDefaultValue()), "./textures/wood.exr");
}
#endif
