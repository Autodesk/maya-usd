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

#include <ghc/fs_std.hpp>
#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

namespace UsdLayerEditor {
namespace FileSystem {

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

TEST(FileSystemUtils, NumberSuffix_MixedDigitAndLetterInputs)
{
    EXPECT_EQ(size_t(0), getNumberSuffixPosition(""));
    EXPECT_EQ(size_t(0), getNumberSuffixPosition("1"));
    EXPECT_EQ(size_t(2), getNumberSuffixPosition("1a"));
    EXPECT_EQ(size_t(1), getNumberSuffixPosition("a1"));
    EXPECT_EQ(size_t(8), getNumberSuffixPosition("abc35def19753"));
    EXPECT_EQ(std::string("19753"), getNumberSuffix("abc35def19753"));
}

} // namespace FileSystem
} // namespace UsdLayerEditor
