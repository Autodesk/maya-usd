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
#include <usdUfe/utils/fileSystem.h>

#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/schema.h>

#include <ghc/fs_std.hpp>
#include <gtest/gtest.h>

#include <string>

using namespace UsdUfe::FileSystem;

// --- getDir -------------------------------------------------------------------

TEST(FileSystemUtils, GetDir_ReturnsParentDirectory)
{
    namespace fss = fs::filesystem;
    // Construct platform-native paths for reliable parent_path() behaviour.
    const fss::path dir = fss::temp_directory_path() / "le_test_dir";
    const fss::path file = dir / "baz.usd";
    EXPECT_EQ(getDir(file.string()), dir.string());
}

TEST(FileSystemUtils, GetDir_ReturnsEmptyForFilenameOnly) { EXPECT_EQ(getDir("baz.usd"), ""); }

// --- appendPaths --------------------------------------------------------------

TEST(FileSystemUtils, AppendPaths_JoinsWithSeparator)
{
    namespace fss = fs::filesystem;
    // appendPaths uses native separators, so build the expected value the same way.
    const std::string expected = (fss::path("foo") / "bar.usd").string();
    EXPECT_EQ(appendPaths("foo", "bar.usd"), expected);
}

TEST(FileSystemUtils, AppendPaths_AbsoluteBasePreserved)
{
    namespace fss = fs::filesystem;
    const std::string expected = (fss::path("/root/dir") / "layer.usd").string();
    EXPECT_EQ(appendPaths("/root/dir", "layer.usd"), expected);
}

// --- makePathRelativeTo -------------------------------------------------------

TEST(FileSystemUtils, MakePathRelativeTo_ReturnsTrueAndRelativePath)
{
    // Build paths using the platform's temp dir so the paths are syntactically valid.
    namespace fss = fs::filesystem;
    const std::string dir = fss::temp_directory_path().generic_string();
    const std::string file = (fss::temp_directory_path() / "l.usd").generic_string();
    const auto        result = makePathRelativeTo(file, dir);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first, "l.usd");
}

TEST(FileSystemUtils, MakePathRelativeTo_EmptyAnchorReturnsOriginal)
{
    namespace fss = fs::filesystem;
    const std::string file = (fss::temp_directory_path() / "layer.usd").generic_string();
    const auto        result = makePathRelativeTo(file, "");
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first, file);
}

TEST(FileSystemUtils, MakePathRelativeTo_RelativeInputIsAFailure)
{
    const auto result = makePathRelativeTo("rel/layer.usd", "rel");
    EXPECT_EQ(result.first, "rel/layer.usd");
    EXPECT_FALSE(result.second);
}

TEST(FileSystemUtils, MakePathRelativeTo_RelativeAnchorIsAFailure)
{
    const auto file = (fs::filesystem::temp_directory_path() / "layer.usd").generic_string();
    const auto result = makePathRelativeTo(file, "rel");
    EXPECT_EQ(result.first, file);
    EXPECT_FALSE(result.second);
}

TEST(FileSystemUtils, MakePathRelativeTo_ResolvesDotDotInAbsoluteInput)
{
    namespace fss = fs::filesystem;
    const fss::path base = fss::temp_directory_path() / "le_rel_base";
    const fss::path file = base / "sub" / ".." / "layer.usd";
    const auto      result = makePathRelativeTo(file.string(), base.string());
    EXPECT_TRUE(result.second);
    EXPECT_EQ(result.first, "layer.usd");
}

#ifdef _WIN32
// A missing network share makes the path resolution report an error other than "not found".

TEST(FileSystemUtils, MakePathRelativeTo_UnresolvableFileIsAFailure)
{
    const std::string            file = "//localhost/no_such_share_xyz/layer.usd";
    const auto                   dir = fs::filesystem::temp_directory_path().generic_string();
    std::pair<std::string, bool> result;
    EXPECT_NO_THROW(result = makePathRelativeTo(file, dir));
    EXPECT_EQ(result.first, file);
    EXPECT_FALSE(result.second);
}

TEST(FileSystemUtils, MakePathRelativeTo_UnresolvableAnchorIsAFailure)
{
    const auto file = (fs::filesystem::temp_directory_path() / "layer.usd").generic_string();
    std::pair<std::string, bool> result;
    EXPECT_NO_THROW(result = makePathRelativeTo(file, "//localhost/no_such_share_xyz/dir"));
    EXPECT_EQ(result.first, file);
    EXPECT_FALSE(result.second);
}
#endif

// --- getLayerFileDir ----------------------------------------------------------

TEST(FileSystemUtils, GetLayerFileDir_ReturnsEmptyForNullLayer)
{
    EXPECT_EQ(getLayerFileDir(PXR_NS::SdfLayerHandle()), "");
}

TEST(FileSystemUtils, GetLayerFileDir_ReturnsEmptyForAnonymousLayer)
{
    auto layer = PXR_NS::SdfLayer::CreateAnonymous("fileDir_test");
    // Anonymous layers have no real path, so dir is empty.
    EXPECT_EQ(getLayerFileDir(layer), "");
}

// --- getPathRelativeToDirectory -----------------------------------------------

TEST(FileSystemUtils, GetPathRelativeToDirectory_ReturnsFilename)
{
    namespace fss = fs::filesystem;
    const std::string dir = fss::temp_directory_path().generic_string();
    const std::string file = (fss::temp_directory_path() / "rel_test.usd").generic_string();
    EXPECT_EQ(getPathRelativeToDirectory(file, dir), "rel_test.usd");
}

TEST(FileSystemUtils, GetPathRelativeToDirectory_EmptyDirReturnsFile)
{
    namespace fss = fs::filesystem;
    const std::string file = (fss::temp_directory_path() / "abs.usd").generic_string();
    EXPECT_EQ(getPathRelativeToDirectory(file, ""), file);
}

// --- getPathRelativeToLayerFile -----------------------------------------------

TEST(FileSystemUtils, GetPathRelativeToLayerFile_NullLayerReturnsFileName)
{
    const std::string file = "/some/file.usd";
    EXPECT_EQ(getPathRelativeToLayerFile(file, PXR_NS::SdfLayerHandle()), file);
}

TEST(FileSystemUtils, GetPathRelativeToLayerFile_AnonymousLayerReturnsFileName)
{
    auto              layer = PXR_NS::SdfLayer::CreateAnonymous("anon_rel");
    const std::string file = "/some/file.usd";
    EXPECT_EQ(getPathRelativeToLayerFile(file, layer), file);
}

// --- postponed relative paths ----------------------------------------------------------

namespace {

// A saved child layer in a fresh temp directory, removed on destruction.
struct PostponedFixture
{
    explicit PostponedFixture(const char* name)
        : dir(fs::filesystem::temp_directory_path() / name)
    {
        std::error_code ec;
        fs::filesystem::remove_all(dir, ec);
        fs::filesystem::create_directories(dir);
        childPath = (dir / "child.usda").generic_string();
        child = PXR_NS::SdfLayer::CreateNew(childPath);
        child->Save();
    }
    ~PostponedFixture()
    {
        child = nullptr;
        std::error_code ec;
        fs::filesystem::remove_all(dir, ec);
    }

    fs::filesystem::path   dir;
    std::string            childPath;
    PXR_NS::SdfLayerRefPtr child;
};

} // namespace

TEST(FileSystemUtils, PostponedRelative_NonNormalizedMarkedPathIsMatched)
{
    PostponedFixture fixture("le_postponed_sublayer");
    auto             parent = PXR_NS::SdfLayer::CreateAnonymous("parent");
    parent->GetSubLayerPaths().push_back(fixture.childPath);

    const auto nonNormalized = (fixture.dir / "sub" / ".." / "child.usda").generic_string();
    markPathAsPostponedRelative(parent, nonNormalized);
    updatePostponedRelativePaths(parent, (fixture.dir / "parent.usda").generic_string());

    ASSERT_EQ(parent->GetSubLayerPaths().size(), 1u);
    EXPECT_EQ(parent->GetSubLayerPaths()[0], "child.usda");
}

TEST(FileSystemUtils, PostponedRelative_MarkedAttributeBecomesRelative)
{
    PostponedFixture fixture("le_postponed_attr");
    auto             parent = PXR_NS::SdfLayer::CreateAnonymous("parentattr");
    auto             prim = PXR_NS::SdfPrimSpec::New(parent, "Root", PXR_NS::SdfSpecifierDef);
    auto attr = PXR_NS::SdfAttributeSpec::New(prim, "file", PXR_NS::SdfValueTypeNames->Asset);
    attr->SetDefaultValue(PXR_NS::VtValue(PXR_NS::SdfAssetPath(fixture.childPath)));

    markPathAsPostponedRelative(parent, fixture.childPath);
    markAttributeAsPostponedRelative(parent, "file");
    updatePostponedRelativePaths(parent, (fixture.dir / "parent.usda").generic_string());

    EXPECT_EQ(attr->GetDefaultValue().Get<PXR_NS::SdfAssetPath>().GetAssetPath(), "child.usda");
}

TEST(FileSystemUtils, PostponedRelative_NonNormalizedAttributeIsMatched)
{
    PostponedFixture fixture("le_postponed_attr_non_normalized");
    auto             parent = PXR_NS::SdfLayer::CreateAnonymous("parentattr");
    auto             prim = PXR_NS::SdfPrimSpec::New(parent, "Root", PXR_NS::SdfSpecifierDef);
    auto attr = PXR_NS::SdfAttributeSpec::New(prim, "file", PXR_NS::SdfValueTypeNames->Asset);

    const auto nonNormalized = (fixture.dir / "sub" / ".." / "child.usda").generic_string();
    attr->SetDefaultValue(PXR_NS::VtValue(PXR_NS::SdfAssetPath(nonNormalized)));

    markPathAsPostponedRelative(parent, nonNormalized);
    markAttributeAsPostponedRelative(parent, "file");
    updatePostponedRelativePaths(parent, (fixture.dir / "parent.usda").generic_string());

    EXPECT_EQ(attr->GetDefaultValue().Get<PXR_NS::SdfAssetPath>().GetAssetPath(), "child.usda");
}

TEST(FileSystemUtils, PostponedRelative_EmptyAnchorKeepsMarks)
{
    PostponedFixture fixture("le_postponed_empty_anchor");
    auto             parent = PXR_NS::SdfLayer::CreateAnonymous("parent");
    parent->GetSubLayerPaths().push_back(fixture.childPath);

    markPathAsPostponedRelative(parent, fixture.childPath);
    updatePostponedRelativePaths(parent, "");

    ASSERT_EQ(parent->GetSubLayerPaths().size(), 1u);
    EXPECT_EQ(parent->GetSubLayerPaths()[0], fixture.childPath);

    updatePostponedRelativePaths(parent, (fixture.dir / "parent.usda").generic_string());
    EXPECT_EQ(parent->GetSubLayerPaths()[0], "child.usda");
}
