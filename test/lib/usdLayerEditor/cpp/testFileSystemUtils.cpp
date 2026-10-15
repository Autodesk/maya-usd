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
#include <usdLayerEditor/lib/utilFileSystem.h>

#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/schema.h>

#include <ghc/fs_std.hpp>
#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

namespace UsdLayerEditor {
namespace FileSystem {

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

// --- getNumberSuffixPosition --------------------------------------------------

TEST(FileSystemUtils, GetNumberSuffixPosition_LocatesTrailingDigit)
{
    EXPECT_EQ(getNumberSuffixPosition("layer_1"), 6u);
}

TEST(FileSystemUtils, GetNumberSuffixPosition_LocatesMultiDigitSuffix)
{
    EXPECT_EQ(getNumberSuffixPosition("layer_123"), 6u);
}

TEST(FileSystemUtils, GetNumberSuffixPosition_ReturnsLengthWhenNoTrailingDigits)
{
    // "layer" has 5 chars; no trailing digit → suffix starts at end (position 5)
    EXPECT_EQ(getNumberSuffixPosition("layer"), 5u);
}

TEST(FileSystemUtils, GetNumberSuffixPosition_SingleCharSuffix)
{
    // "a9" → suffix at position 1
    EXPECT_EQ(getNumberSuffixPosition("a9"), 1u);
}

// --- getNumberSuffix ----------------------------------------------------------

TEST(FileSystemUtils, GetNumberSuffix_ExtractsTrailingDigits)
{
    EXPECT_EQ(getNumberSuffix("layer_123"), "123");
}

TEST(FileSystemUtils, GetNumberSuffix_ReturnsEmptyWhenNoDigits)
{
    EXPECT_EQ(getNumberSuffix("layer"), "");
}

// --- increaseNumberSuffix -----------------------------------------------------

TEST(FileSystemUtils, IncreaseNumberSuffix_IncrementsTrailingDigit)
{
    EXPECT_EQ(increaseNumberSuffix("layer_1"), "layer_2");
}

TEST(FileSystemUtils, IncreaseNumberSuffix_HandlesRollover)
{
    EXPECT_EQ(increaseNumberSuffix("layer_9"), "layer_10");
}

TEST(FileSystemUtils, IncreaseNumberSuffix_AppendsOneWhenNoSuffix)
{
    EXPECT_EQ(increaseNumberSuffix("layer"), "layer1");
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

// --- FileBackup ---------------------------------------------------------------

namespace {
std::string tempPath(const char* filename)
{
    namespace fss = fs::filesystem;
    return (fss::temp_directory_path() / filename).generic_string();
}
} // namespace

TEST(FileSystemUtils, FileBackup_GetBackupFilename_AppendsDotBackup)
{
    const std::string path = tempPath("le_backup_test.usd");
    const std::string backup = tempPath("le_backup_test.usd.backup");
    FileBackup        fb(path);
    EXPECT_EQ(fb.getBackupFilename(), backup);
}

TEST(FileSystemUtils, FileBackup_BackedFlagFalseWhenFileAbsent)
{
    const std::string path = tempPath("le_backup_absent_12345.usd");
    std::remove(path.c_str());
    FileBackup fb(path);
    EXPECT_FALSE(fb._backed);
}

TEST(FileSystemUtils, FileBackup_DestructorRestoresFileWhenNotCommitted)
{
    const std::string path = tempPath("le_backup_restore_99.usd");
    const std::string backup = tempPath("le_backup_restore_99.usd.backup");
    std::remove(path.c_str());
    std::remove(backup.c_str());
    if (FILE* f = std::fopen(path.c_str(), "w")) {
        std::fclose(f);
    }

    {
        FileBackup fb(path);
        EXPECT_TRUE(fb._backed);
    }
    bool origExists = (std::fopen(path.c_str(), "r") != nullptr);
    bool backupExists = (std::fopen(backup.c_str(), "r") != nullptr);
    if (origExists)
        std::remove(path.c_str());
    if (backupExists)
        std::remove(backup.c_str());
    EXPECT_TRUE(origExists);
    EXPECT_FALSE(backupExists);
}

TEST(FileSystemUtils, FileBackup_CommitPreventsRestore)
{
    const std::string path = tempPath("le_backup_commit_99.usd");
    const std::string backup = tempPath("le_backup_commit_99.usd.backup");
    std::remove(path.c_str());
    std::remove(backup.c_str());
    if (FILE* f = std::fopen(path.c_str(), "w")) {
        std::fclose(f);
    }

    {
        FileBackup fb(path);
        EXPECT_TRUE(fb._backed);
        fb.commit();
    }
    bool origExists = (std::fopen(path.c_str(), "r") != nullptr);
    bool backupExists = (std::fopen(backup.c_str(), "r") != nullptr);
    if (origExists)
        std::remove(path.c_str());
    if (backupExists)
        std::remove(backup.c_str());
    EXPECT_FALSE(origExists);
    EXPECT_TRUE(backupExists);
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

// --- getUniqueFileName --------------------------------------------------------

TEST(FileSystemUtils, GetUniqueFileName_ReturnsNonEmptyString)
{
    namespace fss = fs::filesystem;
    const std::string dir = fss::temp_directory_path().string();
    std::string       name = getUniqueFileName(dir, "layer", "usd");
    EXPECT_FALSE(name.empty());
    EXPECT_NE(name.find("layer"), std::string::npos);
}

// --- ensureUniqueFileName -----------------------------------------------------

TEST(FileSystemUtils, EnsureUniqueFileName_ReturnsSamePathIfNotExists)
{
    const std::string path = tempPath("le_unique_nonexist_99999.usd");
    std::remove(path.c_str());
    EXPECT_EQ(ensureUniqueFileName(path), path);
}

TEST(FileSystemUtils, EnsureUniqueFileName_ReturnsNewNameIfExists)
{
    const std::string path = tempPath("le_unique_exist.usd");
    if (FILE* f = std::fopen(path.c_str(), "w")) {
        std::fclose(f);
    }
    std::string unique = ensureUniqueFileName(path);
    std::remove(path.c_str());
    std::remove(unique.c_str());
    EXPECT_NE(unique, path);
}

// --- makePathRelativeTo: normalization and relative input ------------------------------

TEST(FileSystemUtils, MakePathRelativeTo_RelativeInputIsAFailure)
{
    const auto result = makePathRelativeTo("rel/layer.usd", "rel");
    EXPECT_EQ(result.first, "rel/layer.usd");
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

} // namespace FileSystem
} // namespace UsdLayerEditor
