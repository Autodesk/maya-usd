#include <mayaUsd/utils/utilFileSystem.h>

#include <ghc/fs_std.hpp>
#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using UsdMayaUtilFileSystem::getNumberSuffix;
using UsdMayaUtilFileSystem::getNumberSuffixPosition;
using UsdMayaUtilFileSystem::increaseNumberSuffix;
using UsdMayaUtilFileSystem::pathAppendPath;
using UsdMayaUtilFileSystem::pathFindExtension;
using UsdMayaUtilFileSystem::pathRemoveExtension;
using UsdMayaUtilFileSystem::pathStripPath;
using UsdMayaUtilFileSystem::writeToFilePath;

namespace {

std::string tempPath(const char* filename)
{
    namespace fss = fs::filesystem;
    return (fss::temp_directory_path() / filename).generic_string();
}

} // namespace

TEST(ConvertLoadRules, getNumberSuffixPosition)
{
    EXPECT_EQ(size_t(0), getNumberSuffixPosition(""));
    EXPECT_EQ(size_t(0), getNumberSuffixPosition("1"));
    EXPECT_EQ(size_t(2), getNumberSuffixPosition("1a"));
    EXPECT_EQ(size_t(1), getNumberSuffixPosition("a1"));
    EXPECT_EQ(size_t(8), getNumberSuffixPosition("abc35def19753"));
}

TEST(ConvertLoadRules, getNumberSuffix)
{
    EXPECT_EQ(std::string(""), getNumberSuffix(""));
    EXPECT_EQ(std::string("1"), getNumberSuffix("1"));
    EXPECT_EQ(std::string(""), getNumberSuffix("1a"));
    EXPECT_EQ(std::string("1"), getNumberSuffix("a1"));
    EXPECT_EQ(std::string("19753"), getNumberSuffix("abc35def19753"));
}

TEST(ConvertLoadRules, increaseNumberSuffix)
{
    EXPECT_EQ(std::string("1"), increaseNumberSuffix(""));
    EXPECT_EQ(std::string("2"), increaseNumberSuffix("1"));
    EXPECT_EQ(std::string("1a1"), increaseNumberSuffix("1a"));
    EXPECT_EQ(std::string("a2"), increaseNumberSuffix("a1"));
    EXPECT_EQ(std::string("abc35def19754"), increaseNumberSuffix("abc35def19753"));
}

TEST(UtilsFileSystem, PathStripPath_RemovesDirectory)
{
    namespace fss = fs::filesystem;
    std::string path = (fss::temp_directory_path() / "subdir" / "baz.usd").string();
    pathStripPath(path);
    EXPECT_EQ(path, "baz.usd");
}

TEST(UtilsFileSystem, PathStripPath_FilenameAloneUnchanged)
{
    std::string path = "baz.usd";
    pathStripPath(path);
    EXPECT_EQ(path, "baz.usd");
}

TEST(UtilsFileSystem, PathRemoveExtension_StripsExtension)
{
    namespace fss = fs::filesystem;
    const fss::path   dir = fss::temp_directory_path() / "le_dir";
    const std::string ext = (dir / "baz.usd").string();
    const std::string noext = (dir / "baz").string();
    std::string       path = ext;
    pathRemoveExtension(path);
    EXPECT_EQ(path, noext);
}

TEST(UtilsFileSystem, PathRemoveExtension_NoOpWhenNoExtension)
{
    std::string path = "baz";
    std::string expected = path;
    pathRemoveExtension(path);
    EXPECT_EQ(path, expected);
}

TEST(UtilsFileSystem, PathFindExtension_IncludesDot)
{
    std::string path = "baz.usd";
    EXPECT_EQ(pathFindExtension(path), ".usd");
}

TEST(UtilsFileSystem, PathFindExtension_ReturnsEmptyForNoExtension)
{
    std::string path = "baz";
    EXPECT_EQ(pathFindExtension(path), "");
}

TEST(UtilsFileSystem, WriteToFilePath_WritesContentAndReturnsSize)
{
    const std::string path = tempPath("le_write_test.bin");
    std::remove(path.c_str());
    const char   data[] = "hello usd";
    const size_t dataSize = sizeof(data) - 1;
    size_t       written = writeToFilePath(path.c_str(), data, dataSize);
    EXPECT_EQ(written, dataSize);

    // Verify content was written to disk.
    std::ifstream in(path, std::ios::binary);
    ASSERT_TRUE(in.is_open());
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content, std::string(data, dataSize));
    in.close();
    std::remove(path.c_str());
}

TEST(UtilsFileSystem, WriteToFilePath_ZeroForBadPath)
{
    size_t written = writeToFilePath("", "x", 1);
    EXPECT_EQ(written, 0u);
}

TEST(UtilsFileSystem, PathAppendPath_ReturnsTrueAndAppends)
{
    namespace fss = fs::filesystem;
    std::string dir = fss::temp_directory_path().string();
    bool        ok = pathAppendPath(dir, "sub_file.usd");
    EXPECT_TRUE(ok);
    EXPECT_NE(dir.find("sub_file.usd"), std::string::npos);
}

TEST(UtilsFileSystem, PathAppendPath_ReturnsFalseForNonExistentPath)
{
    std::string notADir = "/does/not/exist/at/all";
    bool        ok = pathAppendPath(notADir, "file.usd");
    EXPECT_FALSE(ok);
}
