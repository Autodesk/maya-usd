#include <mayaUsd/utils/utilFileSystem.h>

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace UsdMayaUtilFileSystem;

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
