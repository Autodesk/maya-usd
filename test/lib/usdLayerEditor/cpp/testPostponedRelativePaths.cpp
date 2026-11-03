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
// Pins what the postponed relative path mechanism does: a path is rewritten relative to the
// file being saved only when it was marked beforehand, and the mark is consumed by the save.
//
#include <usdLayerEditor/lib/utilFileSystem.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/payload.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/reference.h>

#include <gtest/gtest.h>

#include <string>

#include <scopedTempDir.h>

namespace UsdLayerEditor {
namespace FileSystem {

namespace {

PXR_NAMESPACE_USING_DIRECTIVE

// The layer is saved to <root>/layers/saved.usda; assets live in <root>/assets.
struct PostponedPathsFixture
{
    explicit PostponedPathsFixture(const std::string& name)
        : dir(name)
        , savedFile(dir.file("layers/saved.usda"))
        , asset(dir.file("assets/model.usd"))
        , layer(SdfLayer::CreateAnonymous("postponed"))
        , prim(SdfCreatePrimInLayer(layer, SdfPath("/Prim")))
    {
    }

    ScopedTempDir     dir;
    std::string       savedFile;
    std::string       asset;
    SdfLayerRefPtr    layer;
    SdfPrimSpecHandle prim;
};

template <class ListProxy> std::string firstPrepended(const ListProxy& list)
{
    const auto items = list.GetPrependedItems();
    return items[0].GetAssetPath();
}

} // namespace

TEST(PostponedRelativePaths, MarkedPathsBecomeRelativeToTheSavedFile)
{
    PostponedPathsFixture f("le_postponed_marked");
    // A sublayer is matched by the file it resolves to, so it has to exist.
    auto sub = SdfLayer::CreateNew(f.dir.file("assets/sub.usda"));
    ASSERT_TRUE(sub);
    ASSERT_TRUE(sub->Save());
    auto nested = SdfCreatePrimInLayer(f.layer, SdfPath("/Prim/Child/Leaf"));

    f.prim->GetReferenceList().Prepend(SdfReference(f.asset));
    nested->GetReferenceList().Prepend(SdfReference(f.asset));
    f.prim->GetPayloadList().Prepend(SdfPayload(f.asset));
    f.layer->GetSubLayerPaths().push_back(sub->GetRealPath());

    markPathAsPostponedRelative(f.layer, f.asset);
    markPathAsPostponedRelative(f.layer, sub->GetRealPath());
    updatePostponedRelativePaths(f.layer, f.savedFile);

    EXPECT_EQ(firstPrepended(f.prim->GetReferenceList()), "../assets/model.usd");
    EXPECT_EQ(firstPrepended(nested->GetReferenceList()), "../assets/model.usd");
    EXPECT_EQ(firstPrepended(f.prim->GetPayloadList()), "../assets/model.usd");
    EXPECT_EQ(f.layer->GetSubLayerPaths()[0], "../assets/sub.usda");
}

TEST(PostponedRelativePaths, UnmarkedPathsAreLeftAlone)
{
    PostponedPathsFixture f("le_postponed_unmarked");
    const std::string     unmarked = f.dir.file("assets/unmarked.usd");
    const std::string     neverMarked = f.dir.file("assets/never.usd");
    f.prim->GetReferenceList().Prepend(SdfReference(f.asset));
    f.prim->GetReferenceList().Append(SdfReference(unmarked));
    f.prim->GetReferenceList().Append(SdfReference(neverMarked));

    markPathAsPostponedRelative(f.layer, f.asset);
    markPathAsPostponedRelative(f.layer, unmarked);
    unmarkPathAsPostponedRelative(f.layer, unmarked);
    updatePostponedRelativePaths(f.layer, f.savedFile);

    const auto appended = f.prim->GetReferenceList().GetAppendedItems();
    EXPECT_EQ(firstPrepended(f.prim->GetReferenceList()), "../assets/model.usd");
    ASSERT_EQ(appended.size(), 2u);
    EXPECT_EQ(appended[0].GetAssetPath(), unmarked);
    EXPECT_EQ(appended[1].GetAssetPath(), neverMarked);
}

TEST(PostponedRelativePaths, SavingConsumesTheMarks)
{
    PostponedPathsFixture f("le_postponed_consumed");
    f.prim->GetReferenceList().Prepend(SdfReference(f.asset));

    markPathAsPostponedRelative(f.layer, f.asset);
    updatePostponedRelativePaths(f.layer, f.savedFile);

    // The same absolute path authored after the save is not converted by the next one.
    f.prim->GetReferenceList().Append(SdfReference(f.asset));
    updatePostponedRelativePaths(f.layer, f.savedFile);

    const auto appended = f.prim->GetReferenceList().GetAppendedItems();
    ASSERT_EQ(appended.size(), 1u);
    EXPECT_EQ(appended[0].GetAssetPath(), f.asset);
}

} // namespace FileSystem
} // namespace UsdLayerEditor
