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

#include <gtest/gtest.h>

#include <string>

#include <scopedTempDir.h>

namespace UsdUfe {
namespace FileSystem {

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

TEST(AssetPathsRelativeToFile, PathsThatCannotOrNeedNotChangeAreLeftAlone)
{
    AssetPathsFixture f("le_rel_untouched");
    // Relative paths of an anonymous layer, tokens, environment variables and URIs.
    const char* kept[] = { "textures/wood.exr",
                           "${ASSETS}/hdri.exr",
                           "$ASSETS/hdri.exr",
                           "%ASSETS%/hdri.exr",
                           "https://example.com/hdri.exr" };
    for (const char* path : kept) {
        f.prim->GetReferenceList().Append(SdfReference(path));
    }

    EXPECT_TRUE(makeAssetPathsRelativeToFile(f.layer, f.target).changes.empty());

    const auto items = f.prim->GetReferenceList().GetAppendedItems();
    ASSERT_EQ(items.size(), std::size(kept));
    for (size_t i = 0; i < items.size(); ++i) {
        EXPECT_EQ(items[i].GetAssetPath(), kept[i]);
    }

#ifdef _WIN32
    // No relative path exists between two drives; the path is kept and a warning is logged.
    auto attr = f.attribute("tex");
    attr->SetDefaultValue(VtValue(SdfAssetPath("Z:/elsewhere/hdri.exr")));
    EXPECT_TRUE(makeAssetPathsRelativeToFile(f.layer, "C:/layers/saved.usda").changes.empty());
    EXPECT_EQ(assetPathOf(attr->GetDefaultValue()), "Z:/elsewhere/hdri.exr");
#endif

    // Nothing to do without a layer or a target.
    f.prim->GetReferenceList().Prepend(SdfReference(f.farAsset));
    EXPECT_TRUE(makeAssetPathsRelativeToFile(SdfLayerHandle(), f.target).changes.empty());
    EXPECT_TRUE(makeAssetPathsRelativeToFile(f.layer, "").changes.empty());
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
    EXPECT_TRUE(makeAssetPathsRelativeToFile(saved, f.dir.file("a/other.usda")).changes.empty());
    EXPECT_EQ(firstReference(prim), "../assets/model.usd");

    // Other folder: still the same file, <root>/assets/model.usd.
    makeAssetPathsRelativeToFile(saved, f.dir.file("b/c/copy.usda"));
    EXPECT_EQ(firstReference(prim), "../../assets/model.usd");
}

} // namespace FileSystem
} // namespace UsdUfe
