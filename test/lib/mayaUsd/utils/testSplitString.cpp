#include <mayaUsd/ufe/Utils.h>

#include <gtest/gtest.h>

using namespace MAYAUSD_NS_DEF;

TEST(SplitString, basicTest)
{
    std::string              testString = "Hi, how are you?";
    std::vector<std::string> tokens = ufe::splitString(testString, ", ?");
    ASSERT_EQ(static_cast<int>(tokens.size()), 4);
    ASSERT_STREQ(tokens[0].c_str(), "Hi");
    ASSERT_STREQ(tokens[1].c_str(), "how");
    ASSERT_STREQ(tokens[2].c_str(), "are");
    ASSERT_STREQ(tokens[3].c_str(), "you");

    std::vector<std::string> tokens2 = ufe::splitString(testString, "!");
    ASSERT_EQ(static_cast<int>(tokens2.size()), 1);
    ASSERT_STREQ(tokens2[0].c_str(), testString.c_str());

    std::vector<std::string> tokens3 = ufe::splitString(testString, "");
    ASSERT_EQ(static_cast<int>(tokens3.size()), 1);
    ASSERT_STREQ(tokens3[0].c_str(), testString.c_str());
}
