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
#include "utilFileSystem.h"

#include <pxr/usd/sdf/layer.h>

#include <ghc/fs_std.hpp>

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {
namespace FileSystem {

// ── getDir ────────────────────────────────────────────────────────────────────

TEST(FileSystemUtils, GetDir_ReturnsParentDirectory)
{
    namespace fss = fs::filesystem;
    // Construct platform-native paths for reliable parent_path() behaviour.
    const fss::path dir  = fss::temp_directory_path() / "le_test_dir";
    const fss::path file = dir / "baz.usd";
    EXPECT_EQ(getDir(file.string()), dir.string());
}

TEST(FileSystemUtils, GetDir_ReturnsEmptyForFilenameOnly)
{
    EXPECT_EQ(getDir("baz.usd"), "");
}

// ── appendPaths ───────────────────────────────────────────────────────────────

TEST(FileSystemUtils, AppendPaths_JoinsWithSeparator)
{
    std::string result = appendPaths("foo", "bar.usd");
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("foo"), std::string::npos);
    EXPECT_NE(result.find("bar.usd"), std::string::npos);
}

TEST(FileSystemUtils, AppendPaths_AbsoluteBasePreserved)
{
    std::string result = appendPaths("/root/dir", "layer.usd");
    EXPECT_NE(result.find("/root/dir"), std::string::npos);
    EXPECT_NE(result.find("layer.usd"), std::string::npos);
}

// ── pathStripPath ─────────────────────────────────────────────────────────────

TEST(FileSystemUtils, PathStripPath_RemovesDirectory)
{
    namespace fss = fs::filesystem;
    std::string path = (fss::temp_directory_path() / "subdir" / "baz.usd").string();
    pathStripPath(path);
    EXPECT_EQ(path, "baz.usd");
}

TEST(FileSystemUtils, PathStripPath_FilenameAloneUnchanged)
{
    std::string path = "baz.usd";
    pathStripPath(path);
    EXPECT_EQ(path, "baz.usd");
}

// ── pathRemoveExtension ───────────────────────────────────────────────────────

TEST(FileSystemUtils, PathRemoveExtension_StripsExtension)
{
    namespace fss = fs::filesystem;
    const fss::path dir   = fss::temp_directory_path() / "le_dir";
    const std::string ext = (dir / "baz.usd").string();
    const std::string noext = (dir / "baz").string();
    std::string path = ext;
    pathRemoveExtension(path);
    EXPECT_EQ(path, noext);
}

TEST(FileSystemUtils, PathRemoveExtension_NoOpWhenNoExtension)
{
    std::string path = "baz";
    std::string expected = path;
    pathRemoveExtension(path);
    EXPECT_EQ(path, expected);
}

// ── pathFindExtension ─────────────────────────────────────────────────────────

TEST(FileSystemUtils, PathFindExtension_IncludesDot)
{
    std::string path = "baz.usd";
    EXPECT_EQ(pathFindExtension(path), ".usd");
}

TEST(FileSystemUtils, PathFindExtension_ReturnsEmptyForNoExtension)
{
    std::string path = "baz";
    EXPECT_EQ(pathFindExtension(path), "");
}

// ── getNumberSuffixPosition ───────────────────────────────────────────────────

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

// ── getNumberSuffix ───────────────────────────────────────────────────────────

TEST(FileSystemUtils, GetNumberSuffix_ExtractsTrailingDigits)
{
    EXPECT_EQ(getNumberSuffix("layer_123"), "123");
}

TEST(FileSystemUtils, GetNumberSuffix_ReturnsEmptyWhenNoDigits)
{
    EXPECT_EQ(getNumberSuffix("layer"), "");
}

// ── increaseNumberSuffix ──────────────────────────────────────────────────────

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

// ── makePathRelativeTo ────────────────────────────────────────────────────────

TEST(FileSystemUtils, MakePathRelativeTo_ReturnsTrueAndRelativePath)
{
    // Build paths using the platform's temp dir so the paths are syntactically valid.
    namespace fss = fs::filesystem;
    const std::string dir  = fss::temp_directory_path().generic_string();
    const std::string file = (fss::temp_directory_path() / "l.usd").generic_string();
    auto [path, ok] = makePathRelativeTo(file, dir);
    EXPECT_TRUE(ok);
    EXPECT_EQ(path, "l.usd");
}

TEST(FileSystemUtils, MakePathRelativeTo_EmptyAnchorReturnsOriginal)
{
    namespace fss = fs::filesystem;
    const std::string file = (fss::temp_directory_path() / "layer.usd").generic_string();
    auto [path, ok] = makePathRelativeTo(file, "");
    EXPECT_TRUE(ok);
    EXPECT_EQ(path, file);
}

// ── getLayerFileDir ───────────────────────────────────────────────────────────

TEST(FileSystemUtils, GetLayerFileDir_ReturnsEmptyForNullLayer)
{
    EXPECT_EQ(getLayerFileDir(SdfLayerHandle()), "");
}

TEST(FileSystemUtils, GetLayerFileDir_ReturnsEmptyForAnonymousLayer)
{
    auto layer = SdfLayer::CreateAnonymous("fileDir_test");
    // Anonymous layers have no real path, so dir is empty.
    EXPECT_EQ(getLayerFileDir(layer), "");
}

// ── FileBackup ────────────────────────────────────────────────────────────────

namespace {
std::string tempPath(const char* filename)
{
    namespace fss = fs::filesystem;
    return (fss::temp_directory_path() / filename).generic_string();
}
} // namespace

TEST(FileSystemUtils, FileBackup_GetBackupFilename_AppendsDotBackup)
{
    const std::string path   = tempPath("le_backup_test.usd");
    const std::string backup = tempPath("le_backup_test.usd.backup");
    FileBackup fb(path);
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
    const std::string path   = tempPath("le_backup_restore_99.usd");
    const std::string backup = tempPath("le_backup_restore_99.usd.backup");
    std::remove(path.c_str());
    std::remove(backup.c_str());
    if (FILE* f = std::fopen(path.c_str(), "w")) { std::fclose(f); }

    {
        FileBackup fb(path);
        EXPECT_TRUE(fb._backed);
    }
    bool origExists   = (std::fopen(path.c_str(),   "r") != nullptr);
    bool backupExists = (std::fopen(backup.c_str(), "r") != nullptr);
    if (origExists)   std::remove(path.c_str());
    if (backupExists) std::remove(backup.c_str());
    EXPECT_TRUE(origExists);
    EXPECT_FALSE(backupExists);
}

TEST(FileSystemUtils, FileBackup_CommitPreventsRestore)
{
    const std::string path   = tempPath("le_backup_commit_99.usd");
    const std::string backup = tempPath("le_backup_commit_99.usd.backup");
    std::remove(path.c_str());
    std::remove(backup.c_str());
    if (FILE* f = std::fopen(path.c_str(), "w")) { std::fclose(f); }

    {
        FileBackup fb(path);
        EXPECT_TRUE(fb._backed);
        fb.commit();
    }
    bool origExists   = (std::fopen(path.c_str(),   "r") != nullptr);
    bool backupExists = (std::fopen(backup.c_str(), "r") != nullptr);
    if (origExists)   std::remove(path.c_str());
    if (backupExists) std::remove(backup.c_str());
    EXPECT_FALSE(origExists);
    EXPECT_TRUE(backupExists);
}

// ── writeToFilePath ───────────────────────────────────────────────────────────

TEST(FileSystemUtils, WriteToFilePath_WritesContentAndReturnsSize)
{
    const std::string path = tempPath("le_write_test.bin");
    std::remove(path.c_str());
    const char data[] = "hello usd";
    const size_t dataSize = sizeof(data) - 1;
    size_t written = writeToFilePath(path.c_str(), data, dataSize);
    EXPECT_EQ(written, dataSize);

    // Verify content was written to disk.
    std::ifstream in(path, std::ios::binary);
    ASSERT_TRUE(in.is_open());
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content, std::string(data, dataSize));
    in.close();
    std::remove(path.c_str());
}

TEST(FileSystemUtils, WriteToFilePath_ZeroForBadPath)
{
    size_t written = writeToFilePath("", "x", 1);
    EXPECT_EQ(written, 0u);
}

// ── pathAppendPath ────────────────────────────────────────────────────────────

TEST(FileSystemUtils, PathAppendPath_ReturnsTrueAndAppends)
{
    namespace fss = fs::filesystem;
    std::string dir = fss::temp_directory_path().string();
    bool ok = pathAppendPath(dir, "sub_file.usd");
    EXPECT_TRUE(ok);
    EXPECT_NE(dir.find("sub_file.usd"), std::string::npos);
}

TEST(FileSystemUtils, PathAppendPath_ReturnsFalseForNonExistentPath)
{
    std::string notADir = "/does/not/exist/at/all";
    bool ok = pathAppendPath(notADir, "file.usd");
    EXPECT_FALSE(ok);
}

// ── getPathRelativeToDirectory ────────────────────────────────────────────────

TEST(FileSystemUtils, GetPathRelativeToDirectory_ReturnsFilename)
{
    namespace fss = fs::filesystem;
    const std::string dir  = fss::temp_directory_path().generic_string();
    const std::string file = (fss::temp_directory_path() / "rel_test.usd").generic_string();
    EXPECT_EQ(getPathRelativeToDirectory(file, dir), "rel_test.usd");
}

TEST(FileSystemUtils, GetPathRelativeToDirectory_EmptyDirReturnsFile)
{
    namespace fss = fs::filesystem;
    const std::string file = (fss::temp_directory_path() / "abs.usd").generic_string();
    EXPECT_EQ(getPathRelativeToDirectory(file, ""), file);
}

// ── getPathRelativeToLayerFile ────────────────────────────────────────────────

TEST(FileSystemUtils, GetPathRelativeToLayerFile_NullLayerReturnsFileName)
{
    const std::string file = "/some/file.usd";
    EXPECT_EQ(getPathRelativeToLayerFile(file, SdfLayerHandle()), file);
}

TEST(FileSystemUtils, GetPathRelativeToLayerFile_AnonymousLayerReturnsFileName)
{
    auto layer = SdfLayer::CreateAnonymous("anon_rel");
    const std::string file = "/some/file.usd";
    EXPECT_EQ(getPathRelativeToLayerFile(file, layer), file);
}

// ── getUniqueFileName ─────────────────────────────────────────────────────────

TEST(FileSystemUtils, GetUniqueFileName_ReturnsNonEmptyString)
{
    namespace fss = fs::filesystem;
    const std::string dir = fss::temp_directory_path().string();
    std::string name = getUniqueFileName(dir, "layer", "usd");
    EXPECT_FALSE(name.empty());
    EXPECT_NE(name.find("layer"), std::string::npos);
}

// ── ensureUniqueFileName ──────────────────────────────────────────────────────

TEST(FileSystemUtils, EnsureUniqueFileName_ReturnsSamePathIfNotExists)
{
    const std::string path = tempPath("le_unique_nonexist_99999.usd");
    std::remove(path.c_str());
    EXPECT_EQ(ensureUniqueFileName(path), path);
}

TEST(FileSystemUtils, EnsureUniqueFileName_ReturnsNewNameIfExists)
{
    const std::string path = tempPath("le_unique_exist.usd");
    if (FILE* f = std::fopen(path.c_str(), "w")) { std::fclose(f); }
    std::string unique = ensureUniqueFileName(path);
    std::remove(path.c_str());
    std::remove(unique.c_str());
    EXPECT_NE(unique, path);
}

} // namespace FileSystem
} // namespace UsdLayerEditor
